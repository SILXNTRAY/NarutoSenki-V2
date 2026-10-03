#include "CreditsLayer.h"
#include "Constants/UiFlowKeys.hpp"

bool CreditsLayer::init()
{
	RETURN_FALSE_IF(!Layer::init());

	// Lua builds the whole screen (see lua/ui/CreditsLayer.lua).
	lua_call_func_self(CreditsFlowKeys::kInit, this, "CreditsLayer");

	return true;
}

void CreditsLayer::onEnterTransitionDidFinish()
{
	Layer::onEnterTransitionDidFinish();
	setKeypadEnabled(true);
}

void CreditsLayer::keyBackClicked()
{
	setKeypadEnabled(false);

	lua_call_func(UiFlowKeys::kCreditsBackToStartMenu);
}
