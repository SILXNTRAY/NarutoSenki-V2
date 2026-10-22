#pragma once
#include "CharacterBase.h"
#include "Hero.hpp"
#include "GameLayer.h"
#include "HudLayer.h"
#include "Enums/HeroEnum.h"
#include "HPBar.h"
#include "Utils/Cocos2dxHelper.hpp"

#include <string>
#include <unordered_map>

/**
 * cBuff mechanics (the "change form" buffs: <p type="attackType">cBuff</p> plus
 * <e type="setBuff">17</e>) that more than one unit can use.
 *
 * The original characters keep their mechanic here, and their class just calls
 * it (Suigetsu::changeAction() -> BuffMechanics::suigetsuChange(*this)), so there is
 * one copy of the code. A custom unit borrows one through lua/class/custom.lua:
 *
 *   ns.CustomBuffs = { Custom1 = { [17] = 'Suigetsu', [18] = 'Kakashi' } }
 *
 * When Custom1 fires a cBuff with that id, CharacterBase runs the donor's
 * `change` instead of its own changeAction(), and later `resume` (buff timer
 * ran out) and/or `actionResume` (the xml's setActionResume event), like the
 * donor's class does. The unit's own xml supplies the data: the SPC animations
 * (skill10-12 for the characters below), the skill's setBuff / setActionResume
 * events and the frames.
 *
 * Add a character by writing its change / resume / actionResume as static
 * functions below, registering them in find(), and calling them from its class.
 */
struct BuffMechanics
{
	struct Entry
	{
		void (*change)(CharacterBase &) = nullptr;		 // the cBuff starts (changeAction)
		void (*resume)(CharacterBase &) = nullptr;		 // the buff timer ended (resumeAction)
		void (*actionResume)(CharacterBase &) = nullptr; // xml setActionResume event (setActionResume)
	};

	static const Entry *find(const std::string &donor)
	{
		static const std::unordered_map<std::string, Entry> table = {
			{std::string(HeroEnum::Suigetsu), {suigetsuChange, nullptr, suigetsuActionResume}},
			{std::string(HeroEnum::Jugo), {jugoChange, nullptr, jugoActionResume}},
			{std::string(HeroEnum::Kisame), {kisameChange, nullptr, kisameActionResume}},
			{std::string(HeroEnum::Hiruzen), {hiruzenChange, nullptr, hiruzenActionResume}},
			{std::string(HeroEnum::Kakashi), {kakashiChange, kakashiResume, nullptr}},
		};
		auto it = table.find(donor);
		return it == table.end() ? nullptr : &it->second;
	}

	/** Entry points used by CharacterBase. Return false when `donor` has no such mechanic. */
	static bool change(const std::string &donor, CharacterBase &c) { return run(donor, &Entry::change, c, "change form"); }
	static bool resume(const std::string &donor, CharacterBase &c) { return run(donor, &Entry::resume, c, nullptr); }
	static bool actionResume(const std::string &donor, CharacterBase &c) { return run(donor, &Entry::actionResume, c, nullptr); }

	/** ---- Armored forms: Suigetsu, Jugo, Kisame, Hiruzen ---- */

	struct FormSpec
	{
		int nAttack;		// normal attack value while in the form
		int rangeX;			// normal attack range while in the form
		bool armored;		// armored while in the form
		int walkSpeed;		// 0 = unchanged
		bool guardResume;	// actionResume does nothing if no buff is active
	};

	static constexpr FormSpec kSuigetsu{1960, 16, true, 0, false};
	static constexpr FormSpec kJugo{1960, 16, false, 320, true};
	static constexpr FormSpec kKisame{660, 32, false, 112, true};
	static constexpr FormSpec kHiruzen{560, 16, true, 0, false};

	static void suigetsuChange(CharacterBase &c) { enterForm(c, kSuigetsu); }
	static void suigetsuActionResume(CharacterBase &c) { leaveForm(c, kSuigetsu); }
	static void jugoChange(CharacterBase &c) { enterForm(c, kJugo); }
	static void jugoActionResume(CharacterBase &c) { leaveForm(c, kJugo); }
	static void kisameChange(CharacterBase &c) { enterForm(c, kKisame); }
	static void kisameActionResume(CharacterBase &c) { leaveForm(c, kKisame); }
	static void hiruzenChange(CharacterBase &c) { enterForm(c, kHiruzen); }
	static void hiruzenActionResume(CharacterBase &c) { leaveForm(c, kHiruzen); }

	static void enterForm(CharacterBase &c, const FormSpec &f)
	{
		if (f.armored)
			c._isArmored = true;

		c._nAttackRangeX = f.rangeX;
		c._nAttackRangeY = 48;

		c.setTempAttackValue1(c.getNAttackValue());
		c.setNAttackValue(f.nAttack);

		c._isOnlySkillLocked = true;

		c.setIdleAction(c.createAnimation(c.skillSPC1Array, 5, true, false));
		c.setWalkAction(c.createAnimation(c.skillSPC2Array, 10, true, false));
		c.setNAttackAction(c.createAnimation(c.skillSPC3Array, 10, false, true));

		if (f.walkSpeed)
		{
			c.setWalkSpeed(f.walkSpeed);
			c._originSpeed = f.walkSpeed;
		}

		c.lockOugisButtons();
	}

	static void leaveForm(CharacterBase &c, const FormSpec &f)
	{
		if (f.guardResume && c._skillChangeBuffValue == 0)
			return;

		if (f.armored)
			c._isArmored = false;

		if (c.hasTempAttackValue1())
		{
			c.setNAttackValue(c.getTempAttackValue1());
			c.setTempAttackValue1(0);
		}
		c._isOnlySkillLocked = false;
		c.unlockOugisButtons();

		c._nAttackRangeX = 16;
		c._nAttackRangeY = 48;

		c.setIdleAction(c.createAnimation(c.idleArray, 5, true, false));
		c.setWalkAction(c.createAnimation(c.walkArray, 10, true, false));
		c.setNAttackAction(c.createAnimation(c.nattackArray, 10, false, true));

		c._skillChangeBuffValue = 0;

		if (f.walkSpeed)
		{
			c.setWalkSpeed(224);
			c._originSpeed = 224;
		}
	}

	/** ---- Kakashi: Sharingan (alternate skill 1 / 2, reveals hidden enemies) ---- */

	static void kakashiChange(CharacterBase &c)
	{
		c.setSkill1Action(c.createAnimation(c.skillSPC1Array, 10, false, true));
		c.setSkill2Action(c.createAnimation(c.skillSPC2Array, 10, false, true));
		c.setIdleAction(c.createAnimation(c.skillSPC3Array, 5, true, false));
		c.setTempAttackValue1(c.getSAttackValue1());
		c.setSAttackValue1(c.getSpcAttackValue1());

		for (auto hero : getGameLayer()->_CharacterArray)
		{
			if (c.getGroup() != hero->getGroup() &&
				hero->isPlayerOrCom() &&
				hero->getState() != State::HURT &&
				hero->getState() != State::DEAD)
			{
				float distanceX = (hero->getPosition() - c.getPosition()).x;
				if (distanceX < kAttackRange)
				{
					if (!hero->_isVisable)
					{
						if (hero->getGimmickName() == HeroEnum::Konan ||
							hero->getGimmickName() == HeroEnum::Deidara)
						{
							hero->unschedule(schedule_selector(CharacterBase::disableBuff));
						}

						hero->setOpacity(255);
						hero->setVisible(true);

						if (hero->_hpBar)
							hero->_hpBar->setVisible(true);
						if (hero->_shadow)
							hero->_shadow->setVisible(true);

						hero->_isVisable = true;
					}
				}
			}
		}

		// icons come from the unit's own plist: <Name>_skill1_1.png / <Name>_skill2_1.png
		if (c.isPlayer())
		{
			setSkillIcons(c, "_skill1_1.png", "_skill2_1.png");
		}
	}

	/** The buff timer ended. Does not reset _skillChangeBuffValue: the caller's CharacterBase::resumeAction does. */
	static void kakashiResume(CharacterBase &c)
	{
		c.setIdleAction(c.createAnimation(c.idleArray, 5, true, false));
		c.setSkill1Action(c.createAnimation(c.skill1Array, 10, false, true));
		c.setSkill2Action(c.createAnimation(c.skill2Array, 10, false, true));

		c.setSAttackValue1(c.getTempAttackValue1());

		if (c.isPlayer())
		{
			setSkillIcons(c, "_skill1.png", "_skill2.png");
		}
	}

private:
	template <class Member>
	static bool run(const std::string &donor, Member which, CharacterBase &c, const char *what)
	{
		auto entry = find(donor);
		if (!entry)
		{
			if (what)
				CCLOG("[Custom] %s borrows '%s' for a cBuff, but it has no portable mechanic yet", c.getName().c_str(), donor.c_str());
			return false;
		}
		auto fn = entry->*which;
		if (!fn)
			return false;
		fn(c);
		return true;
	}

	static void setSkillIcons(CharacterBase &c, const char *skill1Suffix, const char *skill2Suffix)
	{
		auto hud = getGameLayer()->getHudLayer();
		const std::string name = c.getName();
		if (auto f = getSpriteFrame(name + skill1Suffix))
			hud->skill1Button->setDisplayFrame(f);
		if (auto f = getSpriteFrame(name + skill2Suffix))
			hud->skill2Button->setDisplayFrame(f);
	}
};
