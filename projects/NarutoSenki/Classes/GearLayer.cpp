#include "GearLayer.h"
#include "GameLayer.h"
#include "HudLayer.h"
#include "Core/Hero.hpp"
#include "Constants/UiFlowKeys.hpp"

/*----------------------
init GearButton ;
----------------------*/

bool GearButton::init(const char *szImage)
{
	RETURN_FALSE_IF(!Sprite::init());

	if (!is_same(szImage, ""))
		initWithSpriteFrameName(szImage);

	setAnchorPoint(Vec2(0, 0));

	return true;
}

void GearButton::onEnter()
{
	Sprite::onEnter();
	Director::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -1, true);
}

void GearButton::onExit()
{
	Sprite::onExit();
	Director::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
}

CCRect GearButton::getRect()
{
	CCSize size = getContentSize();
	return CCRect(0, 0, size.width, size.height);
}

bool GearButton::containsTouchLocation(Touch *touch)
{
	return getRect().containsPoint(convertTouchToNodeSpace(touch));
}

void GearButton::setBtnType(GearType type, GearButtonType btnType, bool isBuyed, int slot)
{
	_gearType = type;
	_btnType = btnType;
	_isBuyed = isBuyed;

	// Lua builds the gear icon and the "sold out" overlay and places the
	// button (lua/ui/GearLayer.lua).
	lua_call_func_self(GearFlowKeys::kDecorateButton, this, "GearButton", (int)_gearType, (int)_btnType, isBuyed, slot);
}

GearType GearButton::getBtnType()
{
	return _gearType;
}

void GearButton::click()
{
	if (_delegate->currentGear != _gearType && UserDefault::sharedUserDefault()->getBoolForKey("isVoice"))
	{
		SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/select.ogg");
	}

	if (!_isBuyed)
	{
		_delegate->currentGear = _gearType;
	}

	_delegate->showGearDetail(_gearType, true);
}

bool GearButton::ccTouchBegan(Touch *touch, Event *event)
{
	// touch area
	if (!containsTouchLocation(touch) || _isBuyed)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void GearButton::ccTouchMoved(Touch *touch, Event *event)
{
	// touch area
}

void GearButton::ccTouchEnded(Touch *touch, Event *event)
{
	click();
}

void GearButton::playSound()
{
}

GearButton *GearButton::create(const char *szImage)
{
	GearButton *mb = new GearButton();
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

bool ScrewLayer::init()
{
	RETURN_FALSE_IF(!Layer::init());

	return true;
}

void ScrewLayer::onEnter()
{
	Layer::onEnter();
	Director::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 0, true);
}

void ScrewLayer::onExit()
{
	Layer::onExit();
	Director::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
}

bool ScrewLayer::ccTouchBegan(Touch *touch, Event *event)
{
	prePosY = 0;
	return true;
}

void ScrewLayer::ccTouchMoved(Touch *touch, Event *event)
{
	// touch area
	Vec2 curPoint = touch->getLocation();
	if (prePosY == 0)
	{
		prePosY = curPoint.y;
	}
	else
	{
		float distanceY = curPoint.y - prePosY;
		if (getPositionY() < totalRow * 74 || distanceY < 0)
		{
			setPositionY(getPositionY() + distanceY);
		}

		if ((screwBar->getPositionY() > _barMinY || distanceY < 0) && screwBar->getPositionY() <= _barMaxY)
		{
			screwBar->setPositionY(screwBar->getPositionY() - distanceY);
		}

		if (screwBar->getPositionY() > _barMaxY)
		{
			screwBar->setPositionY(_barMaxY);
		}

		if (screwBar->getPositionY() < _barMinY)
		{
			screwBar->setPositionY(_barMinY);
		}

		prePosY = curPoint.y;
	}
};

void ScrewLayer::ccTouchEnded(Touch *touch, Event *event)
{
	prePosY = 0;
	// CCLOG("%f",getPositionY());
	// CCLOG("Height:%f",getContentSize().height);

	if (getPositionY() > totalRow * 74 - 74)
	{
		setPositionY(totalRow * 65);
	}

	if (getPositionY() < _listMinY)
	{
		setPositionY(_listMinY);
	}

	if (screwBar->getPositionY() > _barMaxY)
	{
		screwBar->setPositionY(_barMaxY);
	}

	if (screwBar->getPositionY() < _barMinY)
	{
		screwBar->setPositionY(_barMinY);
	}
}

GearLayer::GearLayer()
{
}

GearLayer::~GearLayer()
{
	getGameLayer()->_isGear = false;
}

bool GearLayer::init(RenderTexture *snapshoot)
{
	RETURN_FALSE_IF(!Layer::init());

	SimpleAudioEngine::sharedEngine()->stopAllEffects();

	// Everything below keeps its behaviour in C++ (purchase, selection, scrolling).
	// Lua builds the dimmer and the shop panel and decides where each control
	// goes (see GearLayer_LayoutControls in lua/ui/GearLayer.lua).

	// frozen screenshot of the game behind the shop
	Texture2D *bgTexture = snapshoot->getSprite()->getTexture();
	_snapshotBg = Sprite::createWithTexture(bgTexture);
	_snapshotBg->setFlipY(true);
	addChild(_snapshotBg, 0);

	coinLabel = CCLabelBMFont::create("0", Fonts::Arial);
	addChild(coinLabel, 12);

	ClippingNode *clipper = ClippingNode::create();
	Node *stencil = Sprite::createWithSpriteFrameName("screwMask.png");
	stencil->setAnchorPoint(Vec2(0, 0));
	clipper->setStencil(stencil);
	_clipper = clipper;

	_screwLayer = ScrewLayer::create();
	_screwLayer->gearNum = 9;

	_screwLayer->screwBar = Sprite::createWithSpriteFrameName("screwBar.png");
	addChild(_screwLayer->screwBar, 600);

	gearDetail = Sprite::createWithSpriteFrameName("gearDetail_00.png");
	addChild(gearDetail, 600);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX) || (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
	gearBigIcon = Sprite::createWithSpriteFrameName("gear_00.png");
	addChild(gearBigIcon, 600);
#endif

	MenuItem *buy_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("gearBuy_btn.png"),
											   Sprite::createWithSpriteFrameName("gearBuy_btn2.png"), this, menu_selector(GearLayer::onGearBuy));
	_buyMenu = Menu::create(buy_btn, nullptr);
	addChild(_buyMenu, 600);

	clipper->addChild(_screwLayer);
	addChild(clipper, 600);

	MenuItem *btm_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("close_btn1.png"),
											   Sprite::createWithSpriteFrameName("close_btn2.png"), this, menu_selector(GearLayer::onResume));
	_closeMenu = Menu::create(btm_btn, nullptr);
	addChild(_closeMenu, 600);

	lua_call_func_self(GearFlowKeys::kLayoutControls, this, "GearLayer");

	return true;
}

Sprite *GearLayer::getScrewBar()
{
	return _screwLayer ? _screwLayer->screwBar : nullptr;
}

void GearLayer::showGearDetail(GearType type, bool updateBigIcon)
{
	lua_call_func_self(GearFlowKeys::kShowDetail, this, "GearLayer", (int)type, updateBigIcon);
}

void GearLayer::confirmPurchase()
{
	// this function for keyboard buy event
	onGearBuy(nullptr);
	// refresh HUB
	getGameLayer()->getHudLayer()->updateGears();
}

void GearLayer::onResume(Ref *sender)
{
	getGameLayer()->getHudLayer()->updateGears();
	Director::sharedDirector()->popScene();

	getGameLayer()->_isGear = false;
}

void GearLayer::onGearBuy(Ref *sender)
{
	if (UserDefault::sharedUserDefault()->getBoolForKey("isVoice"))
	{
		SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.ogg");
	}

	if (getGameLayer()->currentPlayer->setGear(currentGear))
	{
		updatePlayerGear();
	}
}

void GearLayer::updatePlayerGear()
{
	if (getGameLayer()->currentPlayer->getGearArray().size() > 0)
	{
		if (currentGear_layer != nullptr)
			currentGear_layer->removeFromParent();
		currentGear_layer = Layer::create();
		int i = 0;
		for (auto gear : getGameLayer()->currentPlayer->getGearArray())
		{
			GearButton *btn = GearButton::create("");
			btn->setBtnType(gear, GearButtonType::Sell, false, i);
			btn->setDelegate(this);
			currentGear_layer->addChild(btn);
			i++;
		}
		addChild(currentGear_layer, 800);
		lua_call_func_self(GearFlowKeys::kLayoutCurrentGears, this, "GearLayer");
	}

	coinLabel->setString(to_cstr(getGameLayer()->currentPlayer->getCoin()));
	updateGearList();
}

void GearLayer::updateGearList()
{
	auto &gearBtns = _screwLayer->getGearBtnArray();
	if (!gearBtns.empty())
	{
		for (auto btn : gearBtns)
			btn->removeFromParent();
		gearBtns.clear();
	}

	currentGear = GearType::None;
	_screwLayer->totalRow = 3;
	for (uint8_t i = 0; i < _screwLayer->gearNum; i++)
	{
		int row = floor(i / 3.0f);
		int column = abs(3 * row - i);

		GearButton *btn;
		if (column >= 1)
		{
			btn = GearButton::create("value_1000.png");
		}
		else
		{
			btn = GearButton::create("value_500.png");
		}

		bool isBuyed = false;
		for (auto gear : getGameLayer()->currentPlayer->getGearArray())
		{
			if (static_cast<uint32_t>(gear) == i)
				isBuyed = true;
		}

		if (currentGear == GearType::None && !isBuyed)
		{
			currentGear = GearType(i);
			showGearDetail(currentGear, false);
		}

		btn->setBtnType(GearType(i), GearButtonType::Buy, isBuyed, i);
		btn->setDelegate(this);

		gearBtns.push_back(btn);
		_screwLayer->addChild(btn);
	}
}

GearLayer *GearLayer::create(RenderTexture *snapshoot)
{
	GearLayer *grl = new GearLayer();
	if (grl && grl->init(snapshoot))
	{
		grl->autorelease();
		return grl;
	}
	else
	{
		delete grl;
		return nullptr;
	}
}
