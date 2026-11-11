#pragma once
#include <string>

class ClientNetwork;
class GameRenderer;

namespace unool::shell {

	// 阶段 4 · S4 输入适配：把 gamePhase 事件循环里的键盘 / 鼠标映射
	// （Scancode → 指针移动 / sendClientInput / 点击）搬到这里，
	// GameRenderer 仍保留其指针 / 点击相关方法，本适配器只做「翻译 + 分发」。
	// 函数体、case 顺序、键位判定与线上 ClientInput.key 字节均未改动；
	// 仅把事件循环从 ClientApp::gamePhase 平移到本类，构造参数（net / renderer）不变。
	class InputAdapter {
	public:
		InputAdapter(ClientNetwork& net, GameRenderer& renderer);

		// 拉取并分发所有待处理输入事件。
		// 返回 false 表示窗口已关闭（调用方应退出 gamePhase）。
		bool pollAndDispatch();

	private:
		ClientNetwork& net_;
		GameRenderer& renderer_;
	};

}
