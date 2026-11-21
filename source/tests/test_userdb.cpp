#include "test_userdb.h"

#include "Character.h"
#include "UserDB.h"
#include "test_fakes.h"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

using unool::tests::TestResult;

namespace {

	// 存档路径改到系统临时目录：UserDB::dataFile() 在「首次构造」时读一次
	// UNOOL_USERDATA，所以环境变量必须在第一个 UserDB 出现之前设好；
	// 这里用函数内 static 保证只设一次，且顺序天然满足（所有用例都先调本函数）。
	const std::string& storePath() {
		static const std::string path = [] {
			const std::filesystem::path p =
				std::filesystem::temp_directory_path() / "unool_userdb_test.json";
			_putenv_s("UNOOL_USERDATA", p.string().c_str());
			return p.string();
		}();
		return path;
	}

	// 每个用例组都从空库开始：删掉文件后构造的 UserDB 会自己写出一份空存档。
	void resetStore() {
		std::error_code ec;
		std::filesystem::remove(storePath(), ec);
	}

	std::string readStore() {
		std::ifstream file(storePath(), std::ios::binary);
		std::ostringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}

	void writeStore(const std::string& text) {
		std::ofstream file(storePath(), std::ios::binary | std::ios::trunc);
		file << text;
	}

	void testRegister(TestResult& r) {
		resetStore();
		UserDB db;
		std::string err;

		r.check(!db.registerUser("", "pw", err) && err == "用户名不能为空", "空用户名被拒");
		err.clear();
		r.check(!db.registerUser("alice", "", err) && err == "密码不能为空", "空口令被拒");
		err.clear();
		r.check(db.registerUser("alice", "pw123", err), "注册成功");
		r.check(db.exists("alice"), "账号已入库");
		r.check(db.getPoints("alice") == 0, "初始积分 0");

		err.clear();
		r.check(!db.registerUser("alice", "other", err), "重名注册被拒");
		r.check(err == "用户名已存在");

		// 落盘格式：只写盐与派生哈希，明文口令一个字节都不进去。
		const std::string saved = readStore();
		r.check(saved.find("passwordHash") != std::string::npos, "落盘含 passwordHash");
		r.check(saved.find("salt") != std::string::npos, "落盘含 salt");
		r.check(saved.find("pw123") == std::string::npos, "落盘无明文口令");
		r.check(saved.find("\"password\"") == std::string::npos, "落盘不再写 password 字段");
	}

	void testLogin(TestResult& r) {
		resetStore();
		UserDB db;
		std::string err;
		(void)db.registerUser("bob", "secret", err);

		const auto ok = db.login("bob", "secret", err);
		r.check(ok.has_value(), "正确口令登录成功");
		r.check(ok && ok->passwordHash.size() == 64, "返回完整 UserInfo（哈希 64 位十六进制）");
		r.check(ok && ok->salt.size() == 32, "返回完整 UserInfo（盐 32 位十六进制）");

		err.clear();
		r.check(!db.login("bob", "wrong", err).has_value(), "错误口令被拒");
		r.check(err == "密码错误");

		err.clear();
		r.check(!db.login("ghost", "secret", err).has_value(), "不存在的账号被拒");
		r.check(err == "用户名不存在");
	}

	void testLegacyPlaintext(TestResult& r) {
		// P2-1 破坏性换代：旧存档里的明文 password 字段不再放行，
		// 但账号本身仍能读出来（服务端会提示一次性迁移而不是静默丢数据）。
		resetStore();
		writeStore(R"({"bob": {"password": "legacy", "points": 42}})");
		UserDB db;

		r.check(db.exists("bob"), "旧存档仍能读出账号");
		r.check(db.getPoints("bob") == 42, "旧存档的积分字段仍被保留");
		std::string err;
		r.check(!db.login("bob", "legacy", err).has_value(), "旧明文口令不再放行");
		r.check(err.find("旧格式") != std::string::npos, "错误信息提示按旧格式重置");
	}

	void testPoints(TestResult& r) {
		resetStore();
		UserDB db;
		std::string err;
		(void)db.registerUser("pat", "pw", err);

		db.addPoints("pat", 100);
		r.check(db.getPoints("pat") == 100, "加分生效");
		r.check(!db.trySpendPoints("pat", 0), "0 积分不可扣");
		r.check(!db.trySpendPoints("pat", -5), "负积分不可扣");
		r.check(db.trySpendPoints("pat", 30), "足额扣分成功");
		r.check(db.getPoints("pat") == 70, "扣分后余额");
		r.check(!db.trySpendPoints("pat", 71), "余额不足被拒");
		r.check(db.getPoints("pat") == 70, "不足时不扣分");
		db.addPoints("pat", -20);
		r.check(db.getPoints("pat") == 50, "允许负增量");
		db.addPoints("nobody", 10);
		r.check(db.getPoints("nobody") == 0, "不存在的账号静默忽略");
	}

	void testPurchase(TestResult& r) {
		resetStore();
		UserDB db;
		std::string err;
		(void)db.registerUser("buyer", "pw", err);
		db.addPoints("buyer", 300);

		// 角色：扣分 + 附赠默认皮肤
		r.check(db.purchaseCharacter("buyer", "测试角色", 100), "购买角色成功");
		r.check(db.getPoints("buyer") == 200, "买角色即扣分");
		r.check(db.getOwnedSkins("buyer", "测试角色") == std::set<std::string>{"默认"},
		        "角色附赠默认皮肤");

		// 皮肤：必须先拥有该角色
		r.check(!db.purchaseSkin("buyer", "未拥有角色", "皮肤A", 10), "无角色不能买皮肤");
		r.check(db.purchaseSkin("buyer", "测试角色", "皮肤A", 50), "有角色可买皮肤");
		r.check(db.getOwnedSkins("buyer", "测试角色").contains("皮肤A"), "皮肤已入库");
		r.check(db.getPoints("buyer") == 150, "皮肤扣分");

		// 道具：可叠数量，用完即清条目
		r.check(db.purchaseItem("buyer", "炸弹卡", 30), "买道具成功");
		r.check(db.getItemCount("buyer", "炸弹卡") == 1);
		r.check(db.getPoints("buyer") == 120, "道具扣分");
		(void)db.purchaseItem("buyer", "炸弹卡", 30);
		r.check(db.getItemCount("buyer", "炸弹卡") == 2, "道具可叠加");
		db.useItem("buyer", "炸弹卡");
		r.check(db.getItemCount("buyer", "炸弹卡") == 1, "消耗一件");
		db.useItem("buyer", "炸弹卡");
		r.check(db.getItemCount("buyer", "炸弹卡") == 0, "用尽后数量归零");
		db.useItem("buyer", "炸弹卡");
		r.check(db.getItemCount("buyer", "炸弹卡") == 0, "再消耗不会变负");

		// 积分不足：一律失败且不写任何东西
		(void)db.registerUser("poor", "pw", err);
		r.check(!db.purchaseCharacter("poor", "贵角色", 100), "积分不足买不了角色");
		r.check(!db.purchaseItem("poor", "炸弹卡", 30), "积分不足买不了道具");
		r.check(!db.purchaseSkin("poor", "测试角色", "皮肤A", 10), "无角色时连皮肤判定都不过");
		r.check(db.getOwnedCharacters("poor").empty(), "失败不写入角色");
		r.check(db.getItemCount("poor", "炸弹卡") == 0, "失败不写入道具");
	}

	void testPersist(TestResult& r) {
		resetStore();
		{
			UserDB db;
			std::string err;
			(void)db.registerUser("keeper", "pw", err);
			db.addPoints("keeper", 200);
			(void)db.purchaseCharacter("keeper", "测试角色", 100);
			(void)db.purchaseItem("keeper", "增分卡", 50);
		}
		{
			// 重新构造 = 重新从磁盘读，等价于重启进程。
			UserDB db;
			r.check(db.getPoints("keeper") == 50, "积分经磁盘往返");
			r.check(db.getItemCount("keeper", "增分卡") == 1, "道具经磁盘往返");
			r.check(db.getOwnedCharacters("keeper") == std::set<std::string>{"测试角色"},
			        "角色经磁盘往返");
			r.check(db.getOwnedSkins("keeper", "测试角色").contains("默认"), "皮肤经磁盘往返");
			std::string err;
			r.check(db.login("keeper", "pw", err).has_value(), "哈希经磁盘往返仍可校验");
			r.check(!db.login("keeper", "bad", err).has_value(), "错误口令仍被拒");
		}
	}

	void testMatchResult(TestResult& r) {
		resetStore();
		UserDB db;
		std::string err;
		(void)db.registerUser("winner", "pw", err);
		(void)db.registerUser("loser", "pw", err);

		using Level = Character::Level;
		const int delta = Character::getScore(Level::S, Level::S);
		const int beforeWinner = db.getPoints("winner");
		const int beforeLoser = db.getPoints("loser");

		db.addMatchResult("winner", "loser", {Level::S}, {Level::S});
		r.check(db.getPoints("winner") == beforeWinner + delta, "胜者按等级差加分");
		r.check(db.getPoints("loser") == beforeLoser, "普通败者不加分");
		r.check(db.getUserInfo("winner").wins == 1, "胜场 +1");
		r.check(db.getUserInfo("loser").losses == 1, "负场 +1");

		// 满血翻倍：再加一份 delta
		db.addMatchResult("winner", "loser", {Level::S}, {Level::S}, true);
		r.check(db.getPoints("winner") == beforeWinner + delta * 3, "满血翻倍生效");

		// 至尊皮肤败者补偿 +5
		const int loserBeforeSupreme = db.getPoints("loser");
		db.addMatchResult("winner", "loser", {Level::S}, {Level::S}, false,
		                  Character::SkinQuality::normal, Character::SkinQuality::supreme);
		r.check(db.getPoints("loser") == loserBeforeSupreme + 5, "至尊皮肤败者 +5");

		// 饮料皮肤在品质加成里等同普通：胜者拿到裸 delta，败者无补偿
		const int winnerBeforeDrink = db.getPoints("winner");
		const int loserBeforeDrink = db.getPoints("loser");
		db.addMatchResult("winner", "loser", {Level::S}, {Level::S}, false,
		                  Character::SkinQuality::drink, Character::SkinQuality::drink);
		r.check(db.getPoints("winner") == winnerBeforeDrink + delta, "饮料皮肤等同普通");
		r.check(db.getPoints("loser") == loserBeforeDrink, "饮料皮肤败者无补偿");
	}

	void testCheckin(TestResult& r) {
		resetStore();
		UserDB db;
		std::string err;
		(void)db.registerUser("ck", "pw", err);

		const auto first = db.checkin("ck");
		r.check(first.ok, "首次签到成功");
		r.check(first.msg.starts_with("签到成功"), "签到文案");
		r.check(first.points == db.getPoints("ck"), "返回签到后积分");
		r.check(!db.getUserInfo("ck").lastCheckinDate.empty(), "记录签到日期");
		r.check(db.getUserInfo("ck").checkinStreak == 1, "首日连续天数为 1");

		const auto again = db.checkin("ck");
		r.check(!again.ok, "当日重复签到被拒");
		r.check(again.msg == "今天已签到！", "重复签到文案");
		r.check(again.points == db.getPoints("ck"), "重复签到不改变积分");

		const auto ghost = db.checkin("nobody");
		r.check(!ghost.ok && ghost.msg == "用户不存在", "不存在的账号签不了");
	}

	void testCollect(TestResult& r) {
		resetStore();
		UserDB db;
		std::string err;
		(void)db.registerUser("collector", "pw", err);
		db.addPoints("collector", 10'000'000);

		// 把 F 档全部买下来：这是 checkAndUnlockSRank 的触发条件。
		std::size_t fCount = 0;
		std::size_t sCount = 0;
		for (const auto& [name, info] : Character::infos) {
			if (info.level == Character::Level::S) {
				++sCount;
				continue;
			}
			if (info.level != Character::Level::F || name == "白板") continue;
			const auto price = Character::getCharacterPrice(name);
			r.check(price.has_value(), std::format("F 档角色{}有售价", name));
			if (!price.has_value()) continue;
			++fCount;
			r.check(db.purchaseCharacter("collector", name, static_cast<int>(*price)),
			        std::format("购入 F 档角色{}", name));
		}
		r.check(fCount > 0, "F 档角色表非空");
		r.check(db.hasAllCharactersOfLevel("collector", Character::Level::F), "集齐 F 档");
		r.check(!db.hasAllCharactersOfLevel("collector", Character::Level::D), "未集齐 D 档");
		r.check(!db.hasAllNonSRank("collector"), "未集齐 F~A 全档");

		if (sCount > 0) {
			const auto unlocked = db.checkAndUnlockSRank("collector");
			r.check(unlocked.has_value(), "集齐一档后随机解锁 S 档");
			if (unlocked) {
				r.check(db.getOwnedCharacters("collector").contains(*unlocked), "S 档已入库");
				r.check(db.getOwnedSkins("collector", *unlocked).contains("默认"),
				        "S 档附赠默认皮肤");
			}
		}
	}

} // namespace

TestResult runUserDbTests() {
	// checkin / checkAndUnlockSRank 经 utils 的随机门面调 core::services().random。
	// Tests 没有组合根（main() 里没 installServices），不装配就会撞到未安装的槽位。
	// 用函数内 static 的 FakeWorld：装配的引用必须活得比任何一次 services() 调用久，
	// 换成栈对象会在本函数返回后留下悬空引用。
	static unool::tests::FakeWorld world;
	unool::core::installServices(world.services);

	TestResult r;
	r.section("db.register", testRegister);
	r.section("db.login", testLogin);
	r.section("db.legacyPlaintext", testLegacyPlaintext);
	r.section("db.points", testPoints);
	r.section("db.purchase", testPurchase);
	r.section("db.persist", testPersist);
	r.section("db.matchResult", testMatchResult);
	r.section("db.checkin", testCheckin);
	r.section("db.collect", testCollect);

	std::error_code ec;
	std::filesystem::remove(storePath(), ec);
	return r;
}
