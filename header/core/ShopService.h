#pragma once
#include <cstddef>
#include <string>

namespace unool::core {

	class IAccountSession;
	class IUserRepository;

	// 商城展示 / 技能描述 / 签到用例服务（阶段 3 第十批 / W5 第二段分段第一段）。
	//
	// 承接原 ServerNetwork::sendShopData / handleSkillInfoRequest / handleCheckin /
	// handleShopPurchase 的全部业务判断，只依赖 IAccountSession / IUserRepository
	// 两个出站端口与领域 Character 静态数据；不依赖 ServerNetwork、UserDB 或传输细节。
	// 线上包内容（消息号 + JSON 字符串）与日志文本逐字保留。
	// W5 第二段第二段：购买决策已平移，W5 第二段至此收官。
	class ShopService {
	public:
		ShopService(IAccountSession& session, IUserRepository& users);

		// 组装并发送 ShopData（返回发送是否成功，保持原 bool 返回值）
		bool sendShopData(std::size_t clientIdx, const std::string& username);
		// 按角色名生成技能描述文本并回送 SkillInfoResponse
		void handleSkillInfoRequest(std::size_t clientIdx, const std::string& requestData);
		// 每日签到并回送 CheckinResponse，同步会话槽位积分
		void handleCheckin(std::size_t clientIdx);
		// 购买决策：角色 / 皮肤 / 道具三类购买、S 档门槛、集齐档位解锁、
		// 结果发包、积分同步与成功后自动刷新；type=Request 直接回送 ShopData
		void handleShopPurchase(std::size_t clientIdx, const std::string& purchaseData);

	private:
		IAccountSession& session_;
		IUserRepository& users_;
	};
}
