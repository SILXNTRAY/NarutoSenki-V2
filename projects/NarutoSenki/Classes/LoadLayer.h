#pragma once
#include "BGLayer.h"
#include "GameLayer.h"
#include "HudLayer.h"

class CharacterBase;
class Hero;

// Loading screen shown before a match. Asset preloading, the timers (loading
// music, starting the match) and building the game scene stay in C++.
// lua/ui/LoadLayer.lua owns the look: bars, title, clouds, background, the tip
// picture, the blinking "loading" text and the walking hero avatars.
// See Classes/Constants/UiFlowKeys.hpp (LoadFlowKeys) for the Lua entry points.
class LoadLayer : public Layer
{
public:
	LoadLayer();

	bool init();

	int loadNum;

	PROP_PTR(GameLayer, _gameLayer, GameLayer);
	PROP_PTR(BGLayer, _bgLayer, BGLayer);
	PROP_PTR(HudLayer, _hudLayer, HudLayer);

	bool _isHardCoreMode;
	bool _enableGear;

	void preloadAudio();
	void preloadIMG();
	void playBGM(float dt);
	void onLoadFinish(float dt);

	CREATE_FUNC(LoadLayer);
	static void perloadCharIMG(const string &name);
	static void unloadCharIMG(CharacterBase *player);
	static void unloadAllCharsIMG(const vector<Hero *> &players);

private:
	void setLoadingAnimation(const char *player, int index);

	vector<string> loadVector;
};
