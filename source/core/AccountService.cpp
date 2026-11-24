#include "AccountService.h"
#include "IAccountSession.h"
#include "IUserRepository.h"
#include "AccountProtocol.h"
#include "utils.h"

#include <chrono>
#include <cstdint>
#include <print>

namespace unool::core {

	namespace {
		// 口令派生（PBKDF2 12 万次）是登录链上唯一的 CPU 热点，
		// 把它的实际耗时打出来，「登录要等几秒」才能区分是服务端还是网络 / 客户端。
		std::int64_t msSince(std::chrono::steady_clock::time_point start) {
			return std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start).count();
		}
	}

	AccountService::AccountService(IAccountSession& session, IUserRepository& users,
								   std::size_t capacity)
		: session_(session), users_(users), capacity_(capacity) {}

	bool AccountService::spendPoints(std::size_t clientIdx, int amount) {
		if (clientIdx >= capacity_) return false;
		std::string username = session_.usernameOf(clientIdx);
		if (!users_.trySpendPoints(username, amount)) return false;
		session_.onPointsSpent(clientIdx, amount);
		return true;
	}

	void AccountService::handleAccountRequest(std::size_t clientIdx, MessageType type, const unool::protocol::AccountRequest& req) {
		if (!session_.isConnected(clientIdx)) return;

		if (type == MessageType::RegisterRequest) {
			std::string errMsg;
			const auto started = std::chrono::steady_clock::now();
			bool ok = users_.registerUser(req.username, req.password, errMsg);
			std::println("[Server] 注册：口令派生 + 落盘 {} ms", msSince(started));
			std::string msg = ok ? "注册成功" : errMsg;
			session_.sendAccountResponse(clientIdx, MessageType::RegisterResponse, ok, msg, 0, 0, 0);
		}
		else if (type == MessageType::LoginRequest) {
			std::string errMsg;
			const auto started = std::chrono::steady_clock::now();
			auto userInfo = users_.login(req.username, req.password, errMsg);
			std::println("[Server] 登录：口令派生 {} ms", msSince(started));

			bool ok = userInfo.has_value();

			// 检查该槽位是否是掉线玩家的：若有残留 username 且不匹配当前登录账号，拒绝占用
			if (ok && session_.isDisconnected(clientIdx) && !session_.usernameOf(clientIdx).empty()
				&& session_.usernameOf(clientIdx) != req.username) {
				ok = false;
				errMsg = "该座位已被其他玩家占用，请等待";
				//清理该 socket，避免占住槽位但未登录成功导致后续重连连锁失败
				session_.dropConnection(clientIdx);
			}

			// 检查是否有另一个槽位已用同一账号在线。
			// 房间功能上线后槽位数不再等于 2，这里按「除自己以外是否还有同名登录」判定；
			// 同账号重连顶替已由 ServerNetwork 在解析 LoginRequest 时先行处理。
			if (ok && session_.isUsernameLoggedInElsewhere(clientIdx, req.username)) {
				ok = false;
				errMsg = "该账号已在另一端登录";
			}

			int pts = ok ? userInfo->points : 0;
			int w = ok ? userInfo->wins : 0;
			int l = ok ? userInfo->losses : 0;
			std::string msg = ok ? "登录成功" : errMsg;

			session_.sendAccountResponse(clientIdx, MessageType::LoginResponse, ok, msg, pts, w, l);

			if (ok) {
				session_.onLoginSucceeded(clientIdx, req.username, pts, w, l);
			}
		}
		else if (type == MessageType::CheckUsernameRequest) {
			bool exists = users_.exists(req.username);
			session_.sendCheckUsernameResponse(clientIdx, exists);
		}
	}
}
