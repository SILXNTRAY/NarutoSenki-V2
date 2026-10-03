#include "StartMenu.h"
#include "Constants/UiFlowKeys.hpp"

GameMode s_GameMode = GameMode::Classic;
std::array<std::unique_ptr<IGameModeHandler>, GameMode::__Internal_Max_Length> s_ModeHandlers = {
	std::make_unique<Mode1v1>(),
	std::make_unique<Mode3v3>(),
	std::make_unique<Mode4v4>(),
	std::make_unique<ModeHardCore>(),
	std::make_unique<ModeBoss>(),
	std::make_unique<ModeClone>(false),
	std::make_unique<ModeDeathmatch>(),
	std::make_unique<ModeRandomDeathmatch>(),
};
int Cheats = 0;
bool enableCustomSelect = false;

/*----------------------
init MenuButton ;
----------------------*/

bool MenuButton::init(const char *szImage)
{
	RETURN_FALSE_IF(!Sprite::init());

	initWithSpriteFrameName(szImage);
	setAnchorPoint(Vec2(0.5, 0));

	return true;
}

void MenuButton::onEnter()
{
	Sprite::onEnter();
	Director::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 10, true);

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

	// JniMethodInfo minfo;

	// bool isHave = JniHelper::getStaticMethodInfo(minfo, "net/zakume/game/NarutoSenki", "showBanner", "()V");
	// if (isHave)
	// {
	// 	minfo.env->CallStaticVoidMethod(minfo.classID, minfo.methodID);
	// }

#endif
}

void MenuButton::onExit()
{
	Sprite::onExit();
	Director::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
}

CCRect MenuButton::getRect()
{
	CCSize size = getContentSize();
	return CCRect(0, 0, size.width, size.height);
}

bool MenuButton::containsTouchLocation(Touch *touch)
{
	return getRect().containsPoint(convertTouchToNodeSpace(touch));
}

void MenuButton::setBtnType(MenuButtonType type)
{
	_type = type;
}

MenuButtonType MenuButton::getBtnType()
{
	return _type;
}

bool MenuButton::ccTouchBegan(Touch *touch, Event *event)
{
	// touch area
	if (!containsTouchLocation(touch))
		return false;

	// click();

	prePosY = 0;

	return true;
}

void MenuButton::ccTouchMoved(Touch *touch, Event *event)
{
	// touch area
	if (prePosY == 0)
	{
		prePosY = touch->getLocation().y;
	}
	else
	{
		if (getBtnType() != MenuButtonType::HardCore)
		{
			if (abs(touch->getLocation().y - prePosY) > 16)
			{
				if (touch->getLocation().y < prePosY)
				{
					_startMenu->isClockwise = true;
				}
				else
				{
					_startMenu->isClockwise = false;
				}
				_startMenu->isDrag = true;
			}
		}
	}
}

void MenuButton::ccTouchEnded(Touch *touch, Event *event)
{
	if (_slot == MenuSlot::Top && !_startMenu->isDrag)
	{
		switch (_type)
		{
		case MenuButtonType::Training:
			SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.ogg");
			_startMenu->onTrainingCallBack();
			break;
		case MenuButtonType::Credits:
			SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.ogg");
			_startMenu->onCreditsCallBack();
			break;
		case MenuButtonType::Exit:
			SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.ogg");
			_startMenu->onExitCallBack();
			break;
		case MenuButtonType::Custom:
			SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.ogg");
			// TODO
			break;
		case MenuButtonType::HardCore:
			SimpleAudioEngine::sharedEngine()->playEffect(SELECT_SOUND);
			auto frame = getSpriteFrame("menu05_text.png");
			_startMenu->menuText->setDisplayFrame(frame);
			_startMenu->onHardLayerCallBack();
			break;
		}
	}
	else
	{
		SimpleAudioEngine::sharedEngine()->playEffect(SELECT_SOUND);
		prePosY = 0;
		_startMenu->scrollMenu(this);
		_startMenu->isDrag = false;
	}
}

void MenuButton::playSound()
{
	SimpleAudioEngine::sharedEngine()->stopAllEffects();

	switch (_type)
	{
	case MenuButtonType::Training:
		SimpleAudioEngine::sharedEngine()->playEffect(TRAINING_SOUND);
		break;
	case MenuButtonType::Custom:
		SimpleAudioEngine::sharedEngine()->playEffect(NETWORK_SOUND);
		break;
	case MenuButtonType::Credits:
		SimpleAudioEngine::sharedEngine()->playEffect(CREDITS_SOUND);
		break;
	case MenuButtonType::Exit:
		SimpleAudioEngine::sharedEngine()->playEffect(EXIT_SOUND);
		break;
	default:
		break;
	}
}

MenuButton *MenuButton::create(const char *szImage)
{
	auto mb = new MenuButton();
	if (mb && mb->init(szImage))
	{
		mb->autorelease();
		return mb;
	}
	else
	{
		delete mb;
		return nullptr;
	}
}

/*----------------------
init StartMenu layer;
----------------------*/

StartMenu::StartMenu()
{
	isClockwise = false;
	isDrag = false;
	menuText = nullptr;
	hardCoreLayer = nullptr;
	notice_layer = nullptr;
	noticeLabel = nullptr;
	news_btn = nullptr;
	login_btn = nullptr;
	noticeBg = nullptr;
	noticeClipper = nullptr;
}

bool StartMenu::init()
{
	RETURN_FALSE_IF(!Layer::init());

	addSprites("Menu.plist");
	addSprites("Result.plist");
	addSprites("NamePlate.plist");

	// Static decoration (ground, clouds, menu bars, title, version label and the
	// avatar fade) is built and animated by lua/ui/StartMenu.lua.
	lua_call_func_self(StartMenuFlowKeys::kInitDecor, this, "StartMenu", VERSION_CODE);

	// The carousel buttons keep their touch handling here. Their slot is the
	// logical state; Lua turns a slot into position / scale / visibility and
	// animates the moves between slots (see scrollMenu).
	auto gamemode_btn = MenuButton::create("menu01.png");
	gamemode_btn->setDelegate(this);
	gamemode_btn->setBtnType(MenuButtonType::Custom);
	gamemode_btn->_slot = MenuSlot::Upper;
	_menuArray.push_back(gamemode_btn);

	auto credits_btn = MenuButton::create("menu04.png");
	credits_btn->setDelegate(this);
	credits_btn->setBtnType(MenuButtonType::Credits);
	credits_btn->_slot = MenuSlot::Hidden;
	_menuArray.push_back(credits_btn);

	auto training_btn = MenuButton::create("menu02.png");
	training_btn->setDelegate(this);
	training_btn->setBtnType(MenuButtonType::Training);
	training_btn->_slot = MenuSlot::Top;
	_menuArray.push_back(training_btn);

	auto exit_btn = MenuButton::create("menu03.png");
	exit_btn->setDelegate(this);
	exit_btn->setBtnType(MenuButtonType::Exit);
	exit_btn->_slot = MenuSlot::Lower;
	_menuArray.push_back(exit_btn);

	menuText = Sprite::createWithSpriteFrameName("menu02_text.png");
	addChild(menuText, 5);

	for (auto menu : _menuArray)
	{
		addChild(menu, 2);
	}

	news_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("news_btn.png"), nullptr, this, menu_selector(StartMenu::onNewsBtn));
	Menu *menu = Menu::create(news_btn, nullptr);
	addChild(menu, 5);

	setNotice();

	login_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("login_btn.png"), nullptr, this, menu_selector(StartMenu::onLoginBtn));
	Menu *menu2 = Menu::create(login_btn, nullptr);
	addChild(menu2, 5);

	// Lua positions everything above (buttons per slot, menu text, news / login
	// buttons, notice bar) and starts the notice marquee.
	lua_call_func_self(StartMenuFlowKeys::kLayoutControls, this, "StartMenu");

	return true;
}

void StartMenu::onEnter()
{
	Layer::onEnter();
	// SimpleAudioEngine::sharedEngine()->end();
	SimpleAudioEngine::sharedEngine()->stopBackgroundMusic(true);

	if (UserDefault::sharedUserDefault()->getBoolForKey("isBGM"))
	{
		SimpleAudioEngine::sharedEngine()->playBackgroundMusic(MENU_MUSIC, true);
	}
}

void StartMenu::onExit()
{
	Layer::onExit();
	// Keep audio engine alive across scene transitions (e.g. Credits),
	// otherwise newly started BGM can be cut off when StartMenu exits.
}

void StartMenu::onLoginBtn(Ref *sender)
{
	auto tip = CCTips::create("ServerMainten");
	addChild(tip, 5000);
	return;
}

void StartMenu::setNotice()
{
	// Builds the notice bar. Positions and the marquee scroll are handled by
	// lua/ui/StartMenu.lua (StartMenu_LayoutControls).
	if (!notice_layer)
	{
		notice_layer = Layer::create();
		noticeBg = Sprite::createWithSpriteFrameName("notice_bg.png");
		notice_layer->addChild(noticeBg);

		ClippingNode *clipper = ClippingNode::create();
		noticeClipper = clipper;
		Node *stencil = Sprite::createWithSpriteFrameName("notice_mask.png");
		stencil->setAnchorPoint(Vec2(0, 0));
		clipper->setStencil(stencil);

		auto strings = CCDictionary::createWithContentsOfFile("Config/strings.xml");
		auto reply = ((CCString *)strings->objectForKey("Notice"))->m_sString.c_str();

		noticeLabel = CCLabelTTF::create(reply, FONT_NAME, 12);
		clipper->addChild(noticeLabel);

		notice_layer->addChild(clipper);

		addChild(notice_layer);
	}
}

void StartMenu::onNewsBtn(Ref *sender)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

	// JniMethodInfo minfo;

	// bool isHave = JniHelper::getStaticMethodInfo(minfo, "net/zakume/game/NarutoSenki", "getInstance", "()Lnet/zakume/game/NarutoSenki;");
	// jobject jobj;
	// if (isHave)
	// {
	// 	jobj = minfo.env->CallStaticObjectMethod(minfo.classID, minfo.methodID);

	// 	isHave = JniHelper::getMethodInfo(minfo, "net/zakume/game/NarutoSenki", "openWebview", "()V");
	// 	if (isHave)
	// 	{
	// 		minfo.env->CallVoidMethod(jobj, minfo.methodID);
	// 	}
	// }

#endif
}

void StartMenu::onHardCoreOn(Ref *sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.ogg");
	if (hardCoreLayer)
	{
		hardCoreLayer->removeAllChildren();
		hardCoreLayer->removeFromParent();
		hardCoreLayer = nullptr;
	}
}

void StartMenu::onHardCoreOff(Ref *sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/cancel.ogg");
	if (hardCoreLayer)
	{
		hardCoreLayer->removeAllChildren();
		hardCoreLayer->removeFromParent();
		hardCoreLayer = nullptr;
	}
}

void StartMenu::onHardLayerCallBack()
{
	if (UserDefault::sharedUserDefault()->getBoolForKey("isHardCore") == false)
	{
		if (!hardCoreLayer)
		{
			hardCoreLayer = Layer::create();

			Sprite *confirm_bg = Sprite::createWithSpriteFrameName("confirm_bg.png");
			confirm_bg->setPosition(Vec2(winSize.width / 2, winSize.height / 2));

			Sprite *hardcore_title = Sprite::createWithSpriteFrameName("hardcore_title.png");
			hardcore_title->setPosition(Vec2(winSize.width / 2, winSize.height / 2 + 38));

			Sprite *hardcore_text = Sprite::createWithSpriteFrameName("hardcore_text.png");
			hardcore_text->setPosition(Vec2(winSize.width / 2, winSize.height / 2 + 8));

			MenuItem *yes_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("yes_btn1.png"), Sprite::createWithSpriteFrameName("yes_btn2.png"), this, menu_selector(StartMenu::onHardCoreOn));
			MenuItem *no_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("no_btn1.png"), Sprite::createWithSpriteFrameName("no_btn2.png"), this, menu_selector(StartMenu::onHardCoreOff));

			Menu *confirm_menu = Menu::create(yes_btn, no_btn, nullptr);
			confirm_menu->alignItemsHorizontallyWithPadding(24);
			confirm_menu->setPosition(Vec2(winSize.width / 2, winSize.height / 2 - 30));

			hardCoreLayer->addChild(confirm_bg, 600);
			hardCoreLayer->addChild(confirm_menu, 650);
			hardCoreLayer->addChild(hardcore_title, 650);
			hardCoreLayer->addChild(hardcore_text, 650);
			addChild(hardCoreLayer, 700);
		}
	}
}

void StartMenu::onTrainingCallBack()
{
	// Enter game mode scene
	auto modeScene = Scene::create();
	auto gameModeLayer = GameModeLayer::create();
	modeScene->addChild(gameModeLayer);

	Director::sharedDirector()->replaceScene(TransitionFade::create(1.25f, modeScene));
}

void StartMenu::onCreditsCallBack()
{
	SimpleAudioEngine::sharedEngine()->stopBackgroundMusic();
	Scene *creditsScene = Scene::create();
	CreditsLayer *creditsLayer = CreditsLayer::create();
	creditsScene->addChild(creditsLayer);
	Director::sharedDirector()->replaceScene(TransitionFade::create(1.25f, creditsScene));
}

void StartMenu::scrollMenu(MenuButton *touched)
{
	// Tapping the button above the active one (or dragging downwards) scrolls
	// forward, everything else scrolls backward.
	const bool forward = touched->_slot == MenuSlot::Upper || (isDrag && isClockwise);

	// Slots are ordered Upper, Top, Lower, Hidden: forward moves each button one
	// slot down that list, backward one slot up (both wrap around).
	const int slotCount = 4;
	const int step = forward ? 1 : slotCount - 1;

	for (auto menu : _menuArray)
	{
		const MenuSlot from = menu->_slot;
		const MenuSlot to = (MenuSlot)(((int)from + step) % slotCount);
		menu->_slot = to;

		// Lua moves / scales / fades the button and re-orders it.
		lua_call_func_self(StartMenuFlowKeys::kMoveButton, menu, "MenuButton", (int)from, (int)to);
	}

	string src;
	for (auto menu : _menuArray)
	{
		if (menu->_slot == MenuSlot::Top)
		{
			switch (menu->getBtnType())
			{
			case MenuButtonType::Training:
				src = "menu02_text.png";
				break;
			case MenuButtonType::Custom:
				src = "menu01_text.png";
				break;
			case MenuButtonType::Credits:
				src = "menu04_text.png";
				break;
			case MenuButtonType::Exit:
				src = "menu03_text.png";
				break;
			default:
				break;
			}
		}
	}

	auto frame = getSpriteFrame(src);
	menuText->setDisplayFrame(frame);
}

void StartMenu::keyBackClicked()
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

	// JniMethodInfo minfo;
	// bool isHave = JniHelper::getStaticMethodInfo(minfo, "net/zakume/game/DialogUtils", "showTipDialog", "(Ljava/lang/String;Ljava/lang/String;)V");

	// if (isHave)
	// {
	// 	jstring jTitle = minfo.env->NewStringUTF("Exit Game");
	// 	jstring jMsg = minfo.env->NewStringUTF("Do you really want to exit?");
	// 	minfo.env->CallStaticVoidMethod(minfo.classID, minfo.methodID, jTitle, jMsg);
	// 	minfo.env->DeleteLocalRef(jTitle);
	// 	minfo.env->DeleteLocalRef(jMsg);
	// }
#else
	Director::sharedDirector()->end();
	exit(0);
#endif
}

void StartMenu::setCheats(int cheats)
{
	Cheats = cheats;
}

void StartMenu::onExitCallBack()
{
	keyBackClicked();
}
