#include "GameOver.h"
#include "GameLayer.h"
#include "Core/Hero.hpp"
#include "GameMode/GameModeImpl.h"
#include "Core/CustomRegistry.hpp"
#include "Constants/UiFlowKeys.hpp"

GameOver::GameOver()
{
	_isWin = false;
	finnalScore = 0;
}

GameOver::~GameOver()
{
	AnimationCache::purgeSharedAnimationCache();
	SpriteFrameCache::sharedSpriteFrameCache()->removeUnusedSpriteFrames();
	TextureCache::sharedTextureCache()->removeUnusedTextures();
}

bool GameOver::init(RenderTexture* snapshoot)
{
	RETURN_FALSE_IF(!Layer::init());

	SimpleAudioEngine::sharedEngine()->stopAllEffects();
	SimpleAudioEngine::sharedEngine()->stopBackgroundMusic(true);

	// frozen screenshot of the game behind the result screen
	auto bgTexture = snapshoot->getSprite()->getTexture();
	_snapshotBg = Sprite::createWithTexture(bgTexture);
	_snapshotBg->setFlipY(true);
	addChild(_snapshotBg, 0);

	result_bg = Sprite::createWithSpriteFrameName("gameover_bg.png");
	addChild(result_bg, 4);

	// Lua builds dimmer, bars and title and places / scales the panel (lua/ui/GameOver.lua)
	lua_call_func_self(GameOverFlowKeys::kInit, this, "GameOver");

	auto su = ScaleTo::create(0.2f, 1.0);
	auto call = CallFunc::create(std::bind(&GameOver::listResult, this));
	auto seq = newSequence(su, call);
	result_bg->runAction(seq);

	return true;
}

void GameOver::listResult()
{
	if (getGameLayer()->_isHardCoreGame)
		SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/battle_over1.ogg");

	else
		SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/battle_over.ogg");

	auto currPlayer = getGameLayer()->currentPlayer;
	int _hour = getGameLayer()->_minute / 60;
	int _minute = getGameLayer()->_minute % 60;

	auto tempTime = format("{:02d}:{:02d}:{:02d}", _hour, _minute, getGameLayer()->_second);

	// Lua draws the portrait, list background and clock
	lua_call_func_self(GameOverFlowKeys::kShowFrame, this, "GameOver", currPlayer->getName().c_str(), tempTime.c_str());

	uint32_t _totalSecond = getGameLayer()->_minute * 60 + getGameLayer()->_second;
	float resultScore = 0;
	uint32_t killDead = currPlayer->getKillNum() - currPlayer->_deadNum;

	// Verify that the game time is valid
	if (_totalSecond != getGameLayer()->getTotalTime())
	{
		CCLOG("[GameOver] EXIT #1 clock mismatch: shown=%u internal=%u", _totalSecond, getGameLayer()->getTotalTime());
		SimpleAudioEngine::sharedEngine()->stopBackgroundMusic(true);
		Director::sharedDirector()->end();
		return;
	}

	if (getGameLayer()->_isHardCoreGame)
	{
		if (_totalSecond > 900)
			resultScore = ((killDead / (_totalSecond / 60.0f)) / 3) * 100;
		else
			resultScore = ((killDead - ((_totalSecond / 60.0f - 15) * 3)) / 45) * 100;
	}
	else
	{
		if (_totalSecond > 600)
			resultScore = ((killDead / (_totalSecond / 60.0f)) / 4) * 100;
		else
			resultScore = ((killDead - ((_totalSecond / 60.0f - 10) * 4)) / 40) * 100;
	}

	if (_totalSecond < 1 * 60 + 5 && _isWin)
	{
		CCLOG("[GameOver] EXIT #2 won in under 1:05 (%u s)", _totalSecond);
		SimpleAudioEngine::sharedEngine()->stopBackgroundMusic(true);
		Director::sharedDirector()->end();
		return;
	}

	int i = 0;
	uint32_t konohaKill = 0;
	uint32_t akatsukiKill = 0;

	for (auto hero : getGameLayer()->_CharacterArray)
	{
		if (hero->isClone() ||
			hero->isSummon() ||
			hero->isKugutsu() ||
			hero->isGuardian())
		{
			continue;
		}

		// 4v4
		if (Cheats >= kMaxCheats)
		{
			if (hero->isNotPlayer())
			{
				continue;
			}
		}

		if (hero->_isControlled)
		{
			hero->_isControlled = false;
			hero->changeGroup();
		}

		uint32_t realKillNum = hero->getKillNum();

		if (hero->isKonohaGroup())
		{
			konohaKill += realKillNum;
		}
		else
		{
			akatsukiKill += realKillNum;
		}

		// owned gears as "0,4,7" for Lua
		string gearsCsv;
		for (auto gear : hero->getGearArray())
		{
			if (!gearsCsv.empty())
				gearsCsv += ",";
			gearsCsv += std::to_string((int)gear);
		}

		// Lua draws the row (avatar, kill / dead / flog numbers, gear icons)
		lua_call_func_self(GameOverFlowKeys::kAddHeroRow, this, "GameOver", hero->getName().c_str(), (bool)hero->isKonohaGroup(), i,
						   realKillNum, (unsigned int)hero->_deadNum, (unsigned int)hero->_flogNum, gearsCsv.c_str());

		i++;
	}

	if (_totalSecond > 900 && getGameLayer()->_isSurrender)
	{
		if (currPlayer->getGroup() == Group::Konoha)
		{
			if (konohaKill > akatsukiKill)
				_isWin = true;
		}
		else
		{
			if (akatsukiKill > konohaKill)
				_isWin = true;
		}
	}

	if (Cheats < kMaxCheats)
	{
		// Verify that the game total kills is valid
		CCLOG("[GameOver] kill check: heroKills=%u totalKills=%u", akatsukiKill + konohaKill, getGameLayer()->getTotalKills());
		for (auto h : getGameLayer()->_CharacterArray)
		{
			if (h->getKillNum() > 0)
				CCLOG("[GameOver]   %s kills=%u clone=%d summon=%d kugutsu=%d guardian=%d counted=%d",
					h->getName().c_str(), h->getKillNum(), (int)h->isClone(), (int)h->isSummon(), (int)h->isKugutsu(), (int)h->isGuardian(),
					(int)!(h->isClone() || h->isSummon() || h->isKugutsu() || h->isGuardian()));
		}

		if ((akatsukiKill + konohaKill) != getGameLayer()->getTotalKills())
		{
			CCLOG("[GameOver] EXIT #3 kill total mismatch");
			SimpleAudioEngine::sharedEngine()->stopBackgroundMusic(true);
			Director::sharedDirector()->end();
			return;
		}

		uint32_t realKillNum = currPlayer->getKillNum();

		string tempReward = getGameLayer()->_isHardCoreGame ? "FDDD\24" : "ONNN\24";
		KTools::decode(tempReward);

		uint32_t coins = KTools::readCoinFromSQL();
		uint32_t rewardNum;
		if (_isWin)
			rewardNum = realKillNum * 75 + to_int(tempReward.c_str());
		else
			rewardNum = realKillNum * 50;

		const char* extraCoin;
		uint32_t tempCoin;
		if (_isWin)
		{
			string tempEtra = "idd4";
			KTools::decode(tempEtra);
			tempCoin = rewardNum + coins + to_int(tempEtra.c_str());
			extraCoin = "+500";
		}
		else
		{
			tempCoin = rewardNum + coins;
			extraCoin = "+0";
		}

		KTools::saveToSQLite("GameRecord", "coin", std::to_string(tempCoin).c_str(), false);

		// Lua draws the reward plate and its labels
		auto rewardCoin = std::to_string(rewardNum);
		lua_call_func_self(GameOverFlowKeys::kShowReward, this, "GameOver", rewardCoin.c_str(), extraCoin);
	}

	const char* imgSrc = nullptr;
	bool isEnableSROrBetter = getGameLayer()->_isHardCoreGame && getGameLayer()->_isRandomChar && !getGameLayer()->_enableGear;

	if (_isWin)
	{
		if (resultScore >= 140 && isEnableSROrBetter)
			imgSrc = "result_SSSR.png";
		else if (resultScore >= 120 && isEnableSROrBetter)
			imgSrc = "result_SSR.png";
		else if (resultScore >= 100 && isEnableSROrBetter)
			imgSrc = "result_SR.png";
		else if (resultScore >= 140)
			imgSrc = "result_SSS.png";
		else if (resultScore >= 120)
			imgSrc = "result_SS.png";
		else if (resultScore >= 100)
			imgSrc = "result_S.png";
		else if (resultScore >= 80)
			imgSrc = "result_A.png";
		else if (resultScore >= 60)
			imgSrc = "result_B.png";
		else
			imgSrc = "result_C.png";
	}
	else if (!_isWin)
	{
		imgSrc = "result_Defeat.png";
	}

	if (imgSrc)
	{
		// Lua draws the grade picture
		lua_call_func_self(GameOverFlowKeys::kShowGrade, this, "GameOver", imgSrc);

		if (_isWin && getGameLayer()->_isHardCoreGame)
		{
			// finnalScore = resultScore + float(currPlayer->_flogNum) / 100;
			// auto recordString = format("{0.2f}Pts", finnalScore);
			// auto recordScore = CCLabelBMFont::create(recordString.c_str(), Fonts::Default);
			// recordScore->setAnchorPoint(Vec2(1, 0.5f));
			// recordScore->setPosition(Vec2(recordSprite->getPositionX() + recordSprite->getContentSize().width, recordSprite->getPositionY() - 7));
			// recordScore->setScale(0.35f);
			// addChild(recordScore, 10);

			// upload_btn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("upload_btn.png"), Sprite::createWithSpriteFrameName("upload_btn.png"), nullptr, this, menu_selector(GameOver::onUPloadBtn));
			// Menu *upMenu = Menu::create(upload_btn, nullptr);
			// upload_btn->setAnchorPoint(Vec2(1.0, 0));
			// upMenu->setPosition(Vec2(winSize.width / 2 + result_bg->getContentSize().width / 2 - 14, winSize.height / 2 + result_bg->getContentSize().height / 2 - 62));
			// addChild(upMenu, 7);

			// detailRecord = format("{:02d}:{:02d},{},{},{}", _minute, getGameLayer()->_second, currPlayer->getKillNum(), currPlayer->_deadNum, currPlayer->_flogNum);
		}
		CCLOG("[GameOver] records: player='%s' win=%d Cheats=%d (records are written when Cheats < %d)", currPlayer->getName().c_str(), (int)_isWin, (int)Cheats, (int)kMaxCheats);
		if (Cheats < kMaxCheats)
		{
			resultChar = currPlayer->getName();
			if (currPlayer->getName() == HeroEnum::SageNaruto)
				resultChar = HeroEnum::Naruto;
			else if (currPlayer->getName() == HeroEnum::RikudoNaruto)
				resultChar = HeroEnum::Naruto;
			else if (currPlayer->getName() == HeroEnum::SageJiraiya)
				resultChar = HeroEnum::Jiraiya;
			else if (currPlayer->getName() == HeroEnum::ImmortalSasuke)
				resultChar = HeroEnum::Sasuke;
			else if (currPlayer->getName() == HeroEnum::RockLee)
				resultChar = HeroEnum::Lee;
			else if (currPlayer->getName() == HeroEnum::Nagato)
				resultChar = HeroEnum::Pain;

			// Custom forms (ns.Transform) are recorded under their base form, like the stock ones above
			string customBaseName = Custom::Registry::get().baseOf(currPlayer->getName());
			if (customBaseName != currPlayer->getName())
				resultChar = customBaseName;

			if (_isWin)
			{
				CCLOG("[GameOver] win block start, resultChar='%s'", resultChar.c_str());
				KTools::ensureCharRecord(resultChar.c_str());
				int winNum = KTools::readWinNumFromSQL(resultChar.c_str());
				if (resultScore >= 140)
					winNum += 3;
				else if (resultScore >= 120)
					winNum += 2;
				else
					winNum += 1;

				auto realWin = std::to_string(winNum);
				KTools::saveSQLite("CharRecord", "name", resultChar.c_str(), "column1", realWin, false);

				if (getGameLayer()->_isRandomChar && resultScore >= 120)
				{
					if (currPlayer->_isControlled)
					{
						currPlayer->_isControlled = false;
						currPlayer->changeGroup();
					}

					for (auto hero : getGameLayer()->_CharacterArray)
					{
						if (hero->isClone() ||
							hero->isPlayer() ||
							hero->isSummon() ||
							hero->isKugutsu() ||
							hero->isGuardian())
						{
							continue;
						}

						if (hero->_isControlled)
						{
							hero->_isControlled = false;
							hero->changeGroup();
						}

						if (hero->getGroup() == currPlayer->getGroup())
						{
							string allyRecordName = Custom::Registry::get().baseOf(hero->getName());
							KTools::ensureCharRecord(allyRecordName.c_str());
							int winNum2 = KTools::readWinNumFromSQL(allyRecordName.c_str());
							if (resultScore >= 140)
								winNum2 += 2;
							else
								winNum2 += 1;

							auto realWin2 = std::to_string(winNum2);
							KTools::saveSQLite("CharRecord", "name", allyRecordName.c_str(), "column1", realWin2, false);
						}
					}
				}

				CCLOG("[GameOver] win count saved, reading best time");
				auto bestTime = KTools::readSQLite("CharRecord", "name", resultChar.c_str(), "column3");
				CCLOG("[GameOver] bestTime='%s'", bestTime.c_str());
				if (bestTime.empty())
				{
					KTools::saveSQLite("CharRecord", "name", resultChar.c_str(), "column3", tempTime, false);
				}
				else
				{
					// Guard: substr() throws (and kills the game) on a time string shorter than "HH:MM:SS"
					if (bestTime.length() < 8)
					{
						CCLOG("[GameOver] bestTime '%s' too short, skipping record compare", bestTime.c_str());
					}
					else
					{
						int recordHour = to_int(bestTime.substr(0, 2).c_str());
						int recordMinute = to_int(bestTime.substr(3, 2).c_str());
						int recordSecond = to_int(bestTime.substr(6, 2).c_str());

						int recordTime = recordHour * 60 * 60 + recordMinute * 60 + recordSecond;
						int currentTime = getGameLayer()->_minute * 60 + getGameLayer()->_second;
						bool isNewRecord = currentTime < recordTime;

						if (isNewRecord)
						{
							KTools::saveSQLite("CharRecord", "name", resultChar.c_str(), "column3", tempTime, false);
						}
					}
				}
			}
		}
	}

	_closeBtn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("close_btn1.png"), Sprite::createWithSpriteFrameName("close_btn2.png"), nullptr, this, menu_selector(GameOver::onBackToMenu));
	_overMenu = Menu::create(_closeBtn, nullptr);
	addChild(_overMenu, 7);

	// Lua draws the version label and places the close menu
	lua_call_func_self(GameOverFlowKeys::kShowFooter, this, "GameOver", GAMEOVER_VER);

	getGameLayer()->_isSurrender = false;

	getGameModeHandler()->onGameOver();
}

void GameOver::onUPloadBtn(Ref* sender)
{
	auto tip = CCTips::create("ServerMainten");
	addChild(tip, 5000);
}

void GameOver::onBackToMenu(Ref* sender)
{
	if (!exitLayer)
	{
		SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/select.ogg");
		exitLayer = Layer::create();

		_yesBtn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("yes_btn1.png"), Sprite::createWithSpriteFrameName("yes_btn2.png"), this, menu_selector(GameOver::onLeft));
		_noBtn = MenuItemSprite::create(Sprite::createWithSpriteFrameName("no_btn1.png"), Sprite::createWithSpriteFrameName("no_btn2.png"), this, menu_selector(GameOver::onCancel));

		_confirmMenu = Menu::create(_yesBtn, _noBtn, nullptr);
		exitLayer->addChild(_confirmMenu, 2);
		addChild(exitLayer, 500);

		// Lua builds the dialog (background, title, text) and places the menu
		lua_call_func_self(GameOverFlowKeys::kBuildConfirm, this, "GameOver");
	}
}

void GameOver::onLeft(Ref* sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/confirm.ogg");

	getGameLayer()->_isExiting = true;
	Director::sharedDirector()->popScene();
}

void GameOver::onCancel(Ref* sender)
{
	SimpleAudioEngine::sharedEngine()->playEffect("Audio/Menu/cancel.ogg");
	exitLayer->removeFromParent();
	exitLayer = nullptr;
}

GameOver* GameOver::create(RenderTexture* snapshoot)
{
	GameOver* pl = new GameOver();
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