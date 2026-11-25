#include "AndroidPlatform.h"
#include "AndroidAssetCatalog.h"

#include "shell/ServerNetwork.h"
#include "shell/RoomManager.h"
#include "shell/JsonConfigProvider.h"
#include "shell/JsonUserRepository.h"
#include "shell/Mt19937Random.h"
#include "shell/ChronoClock.h"
#include "shell/FilesystemAssetCatalog.h"
#include "shell/UserDB.h"
#include "core/Services.h"
#include "core/AccountService.h"
#include "core/ShopService.h"
#include "platform/Platform.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <print>
#include <string>
#include <system_error>
#include <thread>
#include <unistd.h>

namespace {
	constexpr unsigned short kDefaultPort = 8888;
	constexpr int kIdleSleepMs = 2;

	std::atomic<bool> running{false};
	std::atomic<bool> stopRequested{false};

	JavaVM* javaVm = nullptr;
	jclass bridgeClass = nullptr;
	jmethodID logMethod = nullptr;

	int logPipe[2] = { -1, -1 };
	bool logsReady = false;
	std::thread logReader;

	void extractAsset(AAssetManager* manager, const char* name, const std::string& dataDir) {
		const auto target = std::filesystem::path(dataDir) / name;
		// 本地文件已存在且非空时视为用户配置，启动时不再用 APK 内置模板覆盖。
		if (std::filesystem::exists(target)) {
			std::error_code sizeError{};
			const auto size = std::filesystem::file_size(target, sizeError);
			if (!sizeError && size > 0) {
				std::println("[AndroidServer] 使用已有配置 {}，跳过 assets 释放", target.string());
				return;
			}
		}

		AAsset* asset = AAssetManager_open(manager, name, AASSET_MODE_BUFFER);
		if (asset == nullptr) {
			std::println(stderr, "[AndroidServer] assets 缺少 {}", name);
			return;
		}
		const void* buffer = AAsset_getBuffer(asset);
		const off_t length = AAsset_getLength(asset);
		if (buffer != nullptr && length > 0) {
			std::ofstream output(target, std::ios::binary | std::ios::trunc);
			output.write(static_cast<const char*>(buffer), length);
			std::println("[AndroidServer] 已释放配置 {} ({} 字节)", target.string(), length);
		}
		AAsset_close(asset);
	}

	bool runServer(const unsigned short port, AAssetManager* assetManager) {
		unool::platform::setupConsoleUtf8();
		std::println("[AndroidServer] 启动服务端（房间模式，协议版本 3）...");

		// 与 PC 版 ServerMain.cpp 完全相同的组合根：五种出站端口 + 唯一随机引擎。
		unool::shell::JsonConfigProvider configService;
		UserDB userDatabase;
		unool::shell::JsonUserRepository userRepository(userDatabase);
		unool::shell::Mt19937Random randomService;
		unool::shell::ChronoClock clockService;
		// 安卓角色图片在 APK assets 里，不是 filesDir 下的真实目录。
		// 用 AAssetManager 扫描，避免误报“皮肤目录不存在”。
		unool::android::AndroidAssetCatalog assetCatalog(assetManager);
		unool::core::DomainServices domainServices{
			configService, userRepository, randomService, clockService, assetCatalog };
		unool::core::installServices(domainServices);

		const std::size_t maxClients = std::max<std::size_t>(configService.maxClients(), 2);

		unool::shell::ServerNetwork serverNetwork;
		if (!serverNetwork.start(port, maxClients)) {
			std::println(stderr, "[AndroidServer] 监听端口 {} 失败", port);
			return false;
		}

		unool::core::AccountService accountService(serverNetwork, userRepository, maxClients);
		serverNetwork.installAccountService(accountService);
		unool::core::ShopService shopService(serverNetwork, userRepository);
		serverNetwork.installShopService(shopService);
		unool::shell::RoomManager roomManager(serverNetwork, domainServices);
		serverNetwork.installRoomManager(roomManager);

		std::println("[AndroidServer] 等待客户端连接，端口 {}，槽位上限 {}", port, maxClients);
		try {
			while (!stopRequested.load(std::memory_order_relaxed)) {
				serverNetwork.update();
				std::this_thread::sleep_for(std::chrono::milliseconds(kIdleSleepMs));
			}
		}
		catch (const std::exception& e) {
			std::println(stderr, "[AndroidServer] 主循环异常退出：{}", e.what());
		}
		serverNetwork.disconnect();
		std::println("[AndroidServer] 服务端已停止");
		return true;
	}
}

// 把 C++ 的 std::print/println 输出（服务端全部日志）同时送进 logcat 与 Kotlin 界面。
// 不改任何业务源码：仅在 JNI 启动时把 stdout/stderr 重定向到管道。
namespace {
	void emitLog(const std::string& text, const bool isError) {
		if (text.empty()) return;
		__android_log_print(isError ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO,
							"UNOOL", "%s", text.c_str());
		if (javaVm == nullptr || bridgeClass == nullptr || logMethod == nullptr) return;

		JNIEnv* env = nullptr;
		bool attached = false;
		if (javaVm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
			if (javaVm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
			attached = true;
		}
		jstring message = env->NewStringUTF(text.c_str());
		if (message != nullptr) {
			env->CallStaticVoidMethod(bridgeClass, logMethod, message,
									  static_cast<jboolean>(isError ? JNI_TRUE : JNI_FALSE));
			env->DeleteLocalRef(message);
		}
		if (env->ExceptionCheck()) env->ExceptionClear();
		if (attached) javaVm->DetachCurrentThread();
	}

	void logReaderLoop() {
		char buffer[4096]{};
		std::string pending;
		for (;;) {
			const ssize_t count = ::read(logPipe[0], buffer, sizeof(buffer));
			if (count > 0) {
				pending.append(buffer, static_cast<std::size_t>(count));
				std::size_t newline = pending.find('\n');
				while (newline != std::string::npos) {
					emitLog(pending.substr(0, newline), false);
					pending.erase(0, newline + 1);
					newline = pending.find('\n');
				}
				continue;
			}
			if (count < 0 && errno == EINTR) continue;
			break;
		}
		if (!pending.empty()) emitLog(pending, false);
	}

	void setupLogCapture() {
		if (logsReady || ::pipe(logPipe) != 0) return;
		::setvbuf(stdout, nullptr, _IONBF, 0);
		::setvbuf(stderr, nullptr, _IONBF, 0);
		(void)::dup2(logPipe[1], STDOUT_FILENO);
		(void)::dup2(logPipe[1], STDERR_FILENO);
		::close(logPipe[1]);
		logReader = std::thread(logReaderLoop);
		logsReady = true;
	}
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_unool_android_server_ServerBridge_nativeRun(
	JNIEnv* env, jclass, jint port, jstring dataDir, jobject assetManager) {

	if (running.exchange(true)) return JNI_FALSE;
	stopRequested.store(false);

	env->GetJavaVM(&javaVm);
	jclass localClass = env->FindClass("com/unool/android/server/ServerBridge");
	if (localClass != nullptr) {
		bridgeClass = static_cast<jclass>(env->NewGlobalRef(localClass));
		logMethod = env->GetStaticMethodID(bridgeClass, "onNativeLog", "(Ljava/lang/String;Z)V");
		env->DeleteLocalRef(localClass);
	}
	setupLogCapture();

	const char* path = env->GetStringUTFChars(dataDir, nullptr);
	const std::string dataDirString = path == nullptr ? std::string{} : std::string(path);
	if (path != nullptr) env->ReleaseStringUTFChars(dataDir, path);
	unool::platform::setAndroidDataDir(dataDirString);

	AAssetManager* manager = AAssetManager_fromJava(env, assetManager);
	if (manager != nullptr) {
		extractAsset(manager, "server_config.json", dataDirString);
	}
	else {
		std::println(stderr, "[AndroidServer] 无法获取 AssetManager，使用已有配置文件");
	}

	const auto selectedPort = port > 0 ? static_cast<unsigned short>(port) : kDefaultPort;
	const bool ok = runServer(selectedPort, manager);

	if (bridgeClass != nullptr) env->DeleteGlobalRef(bridgeClass);
	bridgeClass = nullptr;
	logMethod = nullptr;
	javaVm = nullptr;
	running.store(false);
	return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_unool_android_server_ServerBridge_nativeStop(JNIEnv*, jclass) {
	stopRequested.store(true);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_unool_android_server_ServerBridge_nativeIsRunning(JNIEnv*, jclass) {
	return running.load() ? JNI_TRUE : JNI_FALSE;
}
