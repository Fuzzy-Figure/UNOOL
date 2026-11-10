#include "AccountService.h"
#include "IAccountSession.h"
#include "IUserRepository.h"
#include "AccountProtocol.h"
#include "utils.h"

namespace unool::application {

	AccountService::AccountService(IAccountSession& session, IUserRepository& users)
		: session_(session), users_(users) {}

	bool AccountService::spendPoints(std::size_t clientIdx, int amount) {
		if (clientIdx >= unool::MAX_PLAYERS) return false;
		std::string username = session_.usernameOf(clientIdx);
		if (!users_.trySpendPoints(username, amount)) return false;
		session_.onPointsSpent(clientIdx, amount);
		return true;
	}

	void AccountService::handleAccountRequest(std::size_t clientIdx, MessageType type, const unool::protocol::AccountRequest& req) {
		if (!session_.isConnected(clientIdx)) return;

		if (type == MessageType::RegisterRequest) {
			std::string errMsg;
			bool ok = users_.registerUser(req.username, req.password, errMsg);
			std::string msg = ok ? "注册成功" : errMsg;
			session_.sendAccountResponse(clientIdx, MessageType::RegisterResponse, ok, msg, 0, 0, 0);
		}
		else if (type == MessageType::LoginRequest) {
			std::string errMsg;
			auto userInfo = users_.login(req.username, req.password, errMsg);

			bool ok = userInfo.has_value();

			// 检查该槽位是否是掉线玩家的：若有残留 username 且不匹配当前登录账号，拒绝占用
			if (ok && session_.isDisconnected(clientIdx) && !session_.usernameOf(clientIdx).empty()
				&& session_.usernameOf(clientIdx) != req.username) {
				ok = false;
				errMsg = "该座位已被其他玩家占用，请等待";
				//清理该 socket，避免占住槽位但未登录成功导致后续重连连锁失败
				session_.dropConnection(clientIdx);
			}

			// 检查另一端是否已用同一账号登录
			if (ok) {
				std::size_t other = 1 - clientIdx;
				if (other < unool::MAX_PLAYERS
					&& session_.isLoggedIn(other)
					&& session_.usernameOf(other) == req.username) {
					ok = false;
					errMsg = "该账号已在另一端登录";
				}
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
		else if (type == MessageType::ReadyToPlay) {
			if (!session_.isLoggedIn(clientIdx)) return;
			session_.onReadyToPlay(clientIdx);
		}
	}
}
