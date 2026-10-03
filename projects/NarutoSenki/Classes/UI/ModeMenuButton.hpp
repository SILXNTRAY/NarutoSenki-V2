#pragma once
#include "UI/GameModeLayer.h"
#include "Constants/UiFlowKeys.hpp"

class ModeMenuButton : public Sprite, public CCTouchDelegate
{
private:
	Sprite *lockMask = nullptr;

public:
	PROP(GameModeLayer *, _gameModeLayer, Delegate);

	GameMode mode;
	bool useMask2 = false;

	bool init(const string &szImage)
	{
		RETURN_FALSE_IF(!Sprite::initWithFile(szImage.c_str()));
		// initWithSpriteFrameName(szImage);
		setAnchorPoint(Vec2(0.5, 0.5));

		return true;
	}

	void onEnter()
	{
		Sprite::onEnter();
		Director::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 10, true);
	}

	void onExit()
	{
		Sprite::onExit();
		Director::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
	}

	CCRect getRect()
	{
		CCSize size = getContentSize();
		return CCRect(0, 0, size.width, size.height);
	}

	bool ccTouchBegan(Touch *touch, Event *event)
	{
		return getRect().containsPoint(convertTouchToNodeSpace(touch));
	}

	void ccTouchEnded(Touch *touch, Event *event)
	{
		SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.ogg");
		_gameModeLayer->selectMode(mode);
	}

	bool isLocked()
	{
		return lockMask != nullptr;
	}

	// Logical lock state stays here; the chain mask visual is built by Lua
	// (GameModeFlowKeys::kDecorateLock) and handed back through setLockMask.
	void lock()
	{
		if (lockMask == nullptr)
			lua_call_func_self(GameModeFlowKeys::kDecorateLock, this, "ModeMenuButton", useMask2);
	}

	void setLockMask(Sprite *mask) { lockMask = mask; }

	void unlock()
	{
		if (lockMask)
		{
			lockMask->removeFromParentAndCleanup(true);
			lockMask = nullptr;
		}
	}

	static ModeMenuButton *create(const string &szImage)
	{
		ModeMenuButton *mb = new ModeMenuButton();
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
};
