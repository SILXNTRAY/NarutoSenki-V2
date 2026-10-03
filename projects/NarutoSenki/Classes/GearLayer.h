#pragma once
#include "Defines.h"

class GameLayer;
class ScrewLayer;

// Keep the numeric values in sync with BUTTON in lua/ui/GearLayer.lua.
enum class GearButtonType : uint8_t
{
	Buy,
	Sell
};

// In-game gear shop. Purchases, selection state, touch handling and the scroll
// list stay in C++. lua/ui/GearLayer.lua owns the look: the dimmer and shop
// panel, the position / anchor of every control created here, the icons on the
// gear buttons and the detail card. See Classes/Constants/UiFlowKeys.hpp
// (GearFlowKeys) for the Lua entry points.
class GearLayer : public Layer
{
public:
	GearLayer();
	~GearLayer();

	bool init(RenderTexture *snapshoot);

	Layer *currentGear_layer = nullptr;
	CCLabelBMFont *coinLabel = nullptr;

	Sprite *gearDetail = nullptr;
	Sprite *gearBigIcon = nullptr; // desktop builds only

	ScrewLayer *_screwLayer = nullptr;
	GearType currentGear = GearType::None;
	void updatePlayerGear();
	void updateGearList();
	void confirmPurchase();

	// Shows the detail card of `type` (and the big icon when `updateBigIcon`).
	// Lua does the actual frame switching.
	void showGearDetail(GearType type, bool updateBigIcon);

	// ---- Lua accessors (see lua/ui/GearLayer.lua) ----
	// Lua owns the layout of these C++-driven controls; behaviour stays here.
	Sprite *getSnapshotBg() { return _snapshotBg; }
	CCLabelBMFont *getCoinLabel() { return coinLabel; }
	Sprite *getGearDetail() { return gearDetail; }
	Sprite *getGearBigIcon() { return gearBigIcon; }
	Menu *getBuyMenu() { return _buyMenu; }
	Menu *getCloseMenu() { return _closeMenu; }
	Node *getClipper() { return _clipper; }
	ScrewLayer *getScrewLayer() { return _screwLayer; }
	Layer *getCurrentGearLayer() { return currentGear_layer; }
	Sprite *getScrewBar();

	static GearLayer *create(RenderTexture *snapshoot);

private:
	void onResume(Ref *sender);
	void onGearBuy(Ref *sender);

	Sprite *_snapshotBg = nullptr;
	Menu *_buyMenu = nullptr;
	Menu *_closeMenu = nullptr;
	Node *_clipper = nullptr;
};

class GearButton : public Sprite, public CCTouchDelegate
{
public:
	bool init(const char *szImage);

	bool _isBuyed = false;
	GearType _gearType = GearType::None;
	GearButtonType _btnType = GearButtonType::Buy;
	PROP(GearLayer *, _delegate, Delegate);

	CCRect getRect();
	// `slot` is the grid index for Buy buttons and the position in the owned
	// row for Sell buttons; Lua turns it into a screen position.
	void setBtnType(GearType type, GearButtonType btnType, bool isBuyed, int slot);
	GearType getBtnType();
	void playSound();
	void click();

	static GearButton *create(const char *szImage);

protected:
	void onEnter();
	void onExit();
	bool ccTouchBegan(Touch *touch, Event *event);
	void ccTouchMoved(Touch *touch, Event *event);
	void ccTouchEnded(Touch *touch, Event *event);

	inline bool containsTouchLocation(Touch *touch);
};

class ScrewLayer : public Layer
{
public:
	bool init();

	float prePosY = 0;
	int totalRow = 0;
	int gearNum = 0;
	Sprite *screwBar = nullptr;
	PROP_Vector(vector<GearButton *>, _gearBtnArray, GearBtnArray);
	PROP(GearLayer *, _delegate, Delegate);

	// Scroll limits, set by Lua together with the layout: the scroll bar stays
	// between barMinY and barMaxY, the list never scrolls below listMinY.
	void setScrollLimits(float barMinY, float barMaxY, float listMinY)
	{
		_barMinY = barMinY;
		_barMaxY = barMaxY;
		_listMinY = listMinY;
	}

	CREATE_FUNC(ScrewLayer);

protected:
	bool ccTouchBegan(Touch *touch, Event *event);
	void ccTouchMoved(Touch *touch, Event *event);
	void ccTouchEnded(Touch *touch, Event *event);

	void onEnter();
	void onExit();

private:
	float _barMinY = 90;
	float _barMaxY = 122;
	float _listMinY = 76;
};
