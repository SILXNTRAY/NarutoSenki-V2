#include "PauseLayer.h"
#include "GameLayer.h"
#include "Constants/UiFlowKeys.hpp"

bool PauseLayer::init(RenderTexture *snapshoot)
{
	if (!Layer::init())
		return false;

	SimpleAudioEngine::sharedEngine()->stopAllEffects();
	SimpleAudioEngine::sharedEngine()->pauseAllEffects();
	SimpleAudioEngine::sharedEngine()->pauseBackgroundMusic();

	// frozen screenshot of the game behind the pause screen
	Texture2D *bgTexture = snapshoot->getSprite()->getTexture();
	_snapshotBg = Sprite::createWithTexture(bgTexture);
	_snapshotBg->setFlipY(true);
	addChild(_snapshotBg, 0);

	// Lua builds the dimmer, bars, title and hint (lua/ui/PauseLayer.lua)
	lua_call_func_self(PauseFlowKeys::kInitDecor, this, "PauseLayer");

	_resumeBtn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("resume.png"), nullptr, nullptr, this, menu_selector(PauseLayer::onResume));
	_backBtn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("btm.png"), nullptr, nullptr, this, menu_selector(PauseLayer::onBackToMenu));

	pauseMenu = Menu::create(_resumeBtn, _backBtn, nullptr);
	addChild(pauseMenu, 3);

	bgm_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("bgm_on.png"), Sprite::createWithSpriteFrameName("bgm_off.png"), nullptr, this, menu_selector(PauseLayer::onBGM));
	voice_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("voice_on.png"), Sprite::createWithSpriteFrameName("voice_off.png"), nullptr, this, menu_selector(PauseLayer::onVoice));
	soundMenu = Menu::create(bgm_btn, voice_btn, nullptr);
	addChild(soundMenu, 4);

	pre_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("preload_on.png"), Sprite::createWithSpriteFrameName("preload_off.png"), nullptr, this, menu_selector(PauseLayer::onPreload));
	preMenu = Menu::create(pre_btn, nullptr);
	addChild(preMenu, 4);

	// Lua places every button
	lua_call_func_self(PauseFlowKeys::kLayoutControls, this, "PauseLayer");

	if (UserDefault::sharedUserDefault()->getBoolForKey("isBGM") == false)
	{
		bgm_btn->selected();
	}
	if (UserDefault::sharedUserDefault()->getBoolForKey("isVoice") == false)
	{
		voice_btn->selected();
	}

	if (UserDefault::sharedUserDefault()->getBoolForKey("isPreload") == false)
	{
		pre_btn->selected();
	}

	return true;
}

void PauseLayer::onBGM(Ref *sender)
{
	if (UserDefault::sharedUserDefault()->getBoolForKey("isBGM") == true)
	{
		UserDefault::sharedUserDefault()->setBoolForKey("isBGM", false);
		bgm_btn->selected();
	}
	else
	{
		UserDefault::sharedUserDefault()->setBoolForKey("isBGM", true);
		bgm_btn->unselected();
	}
}

void PauseLayer::onVoice(Ref *sender)
{
	if (UserDefault::sharedUserDefault()->getBoolForKey("isVoice") == true)
	{
		UserDefault::sharedUserDefault()->setBoolForKey("isVoice", false);
		voice_btn->selected();
	}
	else
	{
		UserDefault::sharedUserDefault()->setBoolForKey("isVoice", true);
		voice_btn->unselected();
	}
}

void PauseLayer::onPreload(Ref *sender)
{
	if (UserDefault::sharedUserDefault()->getBoolForKey("isPreload") == true)
	{
		UserDefault::sharedUserDefault()->setBoolForKey("isPreload", false);
		pre_btn->selected();
	}
	else
	{
		UserDefault::sharedUserDefault()->setBoolForKey("isPreload", true);
		pre_btn->unselected();
	}
}

void PauseLayer::onResume(Ref *sender)
{
	if (UserDefault::sharedUserDefault()->getBoolForKey("isBGM"))
	{
		SimpleAudioEngine::sharedEngine()->resumeBackgroundMusic();
	}
	if (UserDefault::sharedUserDefault()->getBoolForKey("isVoice"))
	{
		SimpleAudioEngine::sharedEngine()->resumeAllEffects();
	}

	Director::sharedDirector()->popScene();
	getGameLayer()->_isPause = false;
}

void PauseLayer::onBackToMenu(Ref *sender)
{
	pauseMenu->setVisible(false);
	soundMenu->setVisible(false);
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/select.ogg");
	exitLayer = Layer::create();

	_yesBtn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("yes_btn1.png"), Sprite::createWithSpriteFrameName("yes_btn2.png"), this, menu_selector(PauseLayer::onLeft));
	_noBtn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("no_btn1.png"), Sprite::createWithSpriteFrameName("no_btn2.png"), this, menu_selector(PauseLayer::onCancel));

	_confirmMenu = Menu::create(_yesBtn, _noBtn, nullptr);
	exitLayer->addChild(_confirmMenu, 2);
	addChild(exitLayer, 500);

	// Lua builds the dialog (background, title, text) and places the menu
	lua_call_func_self(PauseFlowKeys::kBuildConfirm, this, "PauseLayer");
}

void PauseLayer::onLeft(Ref *sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.ogg");
	getGameLayer()->_isSurrender = true;
	Director::sharedDirector()->popScene();

	getGameLayer()->_isPause = false;
}

void PauseLayer::onCancel(Ref *sender)
{
	pauseMenu->setVisible(true);
	soundMenu->setVisible(true);
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/cancel.ogg");
	exitLayer->removeFromParent();
}

PauseLayer *PauseLayer::create(RenderTexture *snapshoot)
{
	PauseLayer *pl = new PauseLayer();
	if (pl && pl->init(snapshoot))
	{
		pl->autorelease();
		return pl;
	}
	else
	{
		delete pl;
		return nullptr;
	}
}
