#pragma once
#include "Defines.h"
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include "../../../cocos2dx/platform/android/jni/JniHelper.h"
#endif

// Result screen. Score / reward / record calculation, the anti-cheat exits,
// saving to the database and leaving the screen stay in C++.
// lua/ui/GameOver.lua owns the look: dimmer, menu bars, title, result panel,
// portrait, scoreboard rows, reward plate, grade picture, version label and
// the confirm dialog. See Classes/Constants/UiFlowKeys.hpp (GameOverFlowKeys)
// for the Lua entry points.
class GameOver : public Layer
{
public:
	GameOver();
	~GameOver();

	bool init(RenderTexture* snapshoot);

	PROP(bool, _isWin, Win);

	Layer* exitLayer = nullptr;
	Layer* cheatLayer = nullptr;
	Sprite* result_bg = nullptr;

	Sprite* refreshBtn = nullptr;
	MenuItem* upload_btn = nullptr;

	string detailRecord;
	float finnalScore;

	// ---- Lua accessors (see lua/ui/GameOver.lua) ----
	Sprite* getSnapshotBg() { return _snapshotBg; }
	Sprite* getResultBg() { return result_bg; }
	Menu* getOverMenu() { return _overMenu; }
	Layer* getExitLayer() { return exitLayer; }
	Menu* getConfirmMenu() { return _confirmMenu; }
	// every button, so Lua can place each one on its own
	MenuItem* getCloseButton() { return _closeBtn; }
	MenuItem* getYesButton() { return _yesBtn; }
	MenuItem* getNoButton() { return _noBtn; }

	static GameOver* create(RenderTexture* snapshoot);

private:
	std::string resultChar;

	Sprite* _snapshotBg = nullptr;
	Menu* _overMenu = nullptr;
	Menu* _confirmMenu = nullptr;
	MenuItem* _closeBtn = nullptr;
	MenuItem* _yesBtn = nullptr;
	MenuItem* _noBtn = nullptr;

	void onBackToMenu(Ref* sender);
	void listResult();
	void onCancel(Ref* sender);
	void onLeft(Ref* sender);
	void onUPloadBtn(Ref* sender);
};