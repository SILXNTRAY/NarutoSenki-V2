#pragma once
#include "SelectLayer.h"
#include "CreditsLayer.h"
#include "UI/GameModeLayer.h"
#include "MyUtils/KTools.h"
#include "MyUtils/CCStrokeLabel.h"
#include "MyUtils/CCScrewLayer.h"

// declare menuButton
enum class MenuButtonType
{
	Custom,
	Training,
	Exit,
	Credits,
	HardCore
};

// Logical position of a carousel button. Lua (lua/ui/StartMenu.lua) maps each
// slot to a screen position / scale / z-order and animates the transitions, so
// keep the numeric values in sync with SLOT in that file.
// Scrolling "forward" moves every button one slot down the list (wrapping
// Hidden -> Upper), scrolling "backward" one slot up.
enum class MenuSlot
{
	Upper = 0, // small, above the active button
	Top = 1,   // active button: full size, the one that reacts to taps
	Lower = 2, // small, below the active button
	Hidden = 3 // faded out behind the active button
};

class StartMenu;

class MenuButton : public Sprite, public CCTouchDelegate
{
public:
	MenuSlot _slot = MenuSlot::Upper;
	float prePosY;
	MenuButtonType _type;
	PROP(StartMenu *, _startMenu, Delegate);

	bool init(const char *szImage);
	CCRect getRect();
	void setBtnType(MenuButtonType type);
	MenuButtonType getBtnType();
	void playSound();

	// Lua accessor (see lua/ui/StartMenu.lua)
	int getSlotIndex() { return (int)_slot; }

	static MenuButton *create(const char *szImage);

protected:
	void onEnter();
	void onExit();
	bool ccTouchBegan(Touch *touch, Event *event);
	void ccTouchMoved(Touch *touch, Event *event);
	void ccTouchEnded(Touch *touch, Event *event);

	inline bool containsTouchLocation(Touch *touch);
};

class StartMenu : public Layer
{
public:
	StartMenu();

	bool init();

	void onTrainingCallBack();
	void onHardCoreOn(Ref *sender);
	void onHardCoreOff(Ref *sender);
	void onHardLayerCallBack();

	void enterCustomMode();
	void enterTrainingMode();
	void enterSelectLayer();

	void onCreditsCallBack();
	void onExitCallBack();

	void onNewsBtn(Ref *sender);
	void onLoginBtn(Ref *sender);

	// Decides the new slot of every button (forward/backward) and hands the
	// movement to Lua. `touched` is the button the user tapped / dragged.
	void scrollMenu(MenuButton *touched);
	Sprite *menuText;

	Layer *hardCoreLayer;

	bool isClockwise;
	bool isDrag;

	MenuItem *news_btn;
	MenuItem *login_btn;
	void setNotice();

	Layer *notice_layer;
	CCLabelTTF *noticeLabel;

	void keyBackClicked();

	void setCheats(int cheats);

	// ---- Lua accessors (see lua/ui/StartMenu.lua) ----
	// Lua owns the layout of these C++-driven controls; behaviour stays here.
	int getMenuButtonCount() { return (int)_menuArray.size(); }
	MenuButton *getMenuButton(int index)
	{
		return (index >= 0 && index < (int)_menuArray.size()) ? _menuArray[index] : nullptr;
	}
	Sprite *getMenuText() { return menuText; }
	MenuItem *getNewsButton() { return news_btn; }
	MenuItem *getLoginButton() { return login_btn; }
	Node *getNoticeBg() { return noticeBg; }
	Node *getNoticeClipper() { return noticeClipper; }
	Node *getNoticeLabel() { return noticeLabel; }

	CREATE_FUNC(StartMenu);

private:
	void onEnter();
	void onExit();

	Node *noticeBg;
	Node *noticeClipper;

	vector<MenuButton *> _menuArray;
};
