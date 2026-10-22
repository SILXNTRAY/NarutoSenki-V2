#pragma once
#include "Defines.h"

// Pause screen. Audio pausing, the sound / preload toggles, resuming and
// surrendering stay in C++. lua/ui/PauseLayer.lua owns the look: dimmer, menu
// bars, title, hint text, the position of every button and the confirm dialog.
// See Classes/Constants/UiFlowKeys.hpp (PauseFlowKeys) for the Lua entry points.
class PauseLayer : public Layer
{
public:
	bool init(RenderTexture *snapshoot);

	Menu *pauseMenu = nullptr;
	Menu *soundMenu = nullptr;
	Menu *preMenu = nullptr;
	Layer *exitLayer = nullptr;
	MenuItem *bgm_btn = nullptr;
	MenuItem *voice_btn = nullptr;
	MenuItem *pre_btn = nullptr;

	// ---- Lua accessors (see lua/ui/PauseLayer.lua) ----
	Sprite *getSnapshotBg() { return _snapshotBg; }
	Menu *getPauseMenu() { return pauseMenu; }
	Menu *getSoundMenu() { return soundMenu; }
	Menu *getPreMenu() { return preMenu; }
	Layer *getExitLayer() { return exitLayer; }
	Menu *getConfirmMenu() { return _confirmMenu; }
	// every button, so Lua can place each one on its own
	MenuItem *getResumeButton() { return _resumeBtn; }
	MenuItem *getBackButton() { return _backBtn; }
	MenuItem *getBgmButton() { return bgm_btn; }
	MenuItem *getVoiceButton() { return voice_btn; }
	MenuItem *getPreloadButton() { return pre_btn; }
	MenuItem *getYesButton() { return _yesBtn; }
	MenuItem *getNoButton() { return _noBtn; }

	static PauseLayer *create(RenderTexture *snapshoot);

private:
	Sprite *_snapshotBg = nullptr;
	Menu *_confirmMenu = nullptr;
	MenuItem *_resumeBtn = nullptr;
	MenuItem *_backBtn = nullptr;
	MenuItem *_yesBtn = nullptr;
	MenuItem *_noBtn = nullptr;

	void onResume(Ref *sender);
	void onBackToMenu(Ref *sender);
	void onLeft(Ref *sender);
	void onCancel(Ref *sender);
	void onBGM(Ref *sender);
	void onPreload(Ref *sender);
	void onVoice(Ref *sender);
};
