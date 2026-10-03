#pragma once
#include "Defines.h"

// Fully Lua screen: every visual (background, clouds, bars, title, credit
// sheets, return button) and the credits music are built by
// lua/ui/CreditsLayer.lua. This class is only the shell that owns the engine
// lifecycle: it hands init over to Lua and forwards the back key.
class CreditsLayer : public Layer
{
public:
	bool init();

	CREATE_FUNC(CreditsLayer);

private:
	void onEnterTransitionDidFinish();

	void keyBackClicked();
};
