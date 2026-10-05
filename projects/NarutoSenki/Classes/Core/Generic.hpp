#pragma once
#include "Hero.hpp"

/**
 * Plain character template for custom units registered with gimmick 'Generic'
 * in lua/class/custom.lua.
 *
 * No buffs, no form changes, no per-character overrides: just the five
 * standard slots (Skill1-3 and Ougis1-2, plus the normal attack) driven by
 * the xml. The AI is a basic fighter: it buys gear, retreats to heal, goes
 * for enemy heroes, then flogs and towers, using each skill when it is ready.
 */
class GenericHero : public Hero
{
	void perform() override
	{
		_mainTarget = nullptr;
		findHeroHalf();

		tryUseGear6();
		tryBuyGear(GearType::Gear00, GearType::Gear01, GearType::Gear02);

		if (needBackToTowerToRestoreHP() ||
			needBackToDefendTower())
			return;

		if (_mainTarget && _mainTarget->isNotFlog() && (battleCondiction >= 0 || _isCanOugis1 || _isCanOugis2))
		{
			Vec2 sp = getDistanceToTarget();

			if (isFreeState())
			{
				if (_isCanOugis2 && !_isControlled && getGameLayer()->_isOugis2Game && !_isArmored &&
					!_mainTarget->_isArmored && _mainTarget->getState() != State::KNOCKDOWN && !_mainTarget->_isSticking)
				{
					if (approach(sp, 48, 16))
						return;

					changeSide(sp);
					attack(OUGIS2);
					return;
				}
				else if (_isCanOugis1 && !_isControlled && !_isArmored && !_mainTarget->_isArmored)
				{
					if (approach(sp, 48, 16))
						return;

					changeSide(sp);
					attack(OUGIS1);
					return;
				}
				else if (enemyCombatPoint > friendCombatPoint && abs(enemyCombatPoint - friendCombatPoint) > 3000 && !_isHealing && !_isControlled)
				{
					// Outmatched: keep distance instead of trading blows
					if (abs(sp.x) < 160)
						stepBack2();
					else
						idle();
					return;
				}
				else if (abs(sp.x) < 128)
				{
					if (approach(sp, 64, 32))
						return;

					changeSide(sp);
					if (_isCanSkill1)
						attack(SKILL1);
					else if (_isCanSkill2)
						attack(SKILL2);
					else if (_isCanSkill3)
						attack(SKILL3);
					else
						attack(NAttack);
					return;
				}
			}
		}

		// No hero to fight: flogs first, then towers
		_mainTarget = nullptr;
		if (battleCondiction >= 0)
		{
			if (notFindFlogHalf())
				findTowerHalf();
		}
		else
		{
			findTowerHalf();
		}

		if (_mainTarget)
		{
			Vec2 sp = getDistanceToTarget();

			if (abs(sp.x) > 32 || abs(sp.y) > 32)
			{
				walk(sp.getNormalized());
				return;
			}

			if (isFreeState())
			{
				changeSide(sp);
				if (_mainTarget->isFlog() && _isCanSkill2)
					attack(SKILL2);
				else if (_mainTarget->isTower() && _isCanSkill3)
					attack(SKILL3);
				else
					attack(NAttack);
			}
			return;
		}

		checkHealingState();
	}

	/** Walk toward the target until inside (rangeX, rangeY). Returns true while still walking. */
	bool approach(Vec2 sp, int rangeX, int rangeY)
	{
		if (abs(sp.x) > rangeX || abs(sp.y) > rangeY)
		{
			if (_isCanGear00)
				useGear(GearType::Gear00);

			walk(sp.getNormalized());
			return true;
		}
		return false;
	}
};
