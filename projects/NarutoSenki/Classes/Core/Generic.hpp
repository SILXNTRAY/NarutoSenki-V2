#pragma once
#include "CustomRegistry.hpp"
#include "Hero.hpp"

/**
 * Plain character template for custom units registered with gimmick 'Generic'
 * in lua/class/custom.lua.
 *
 * No buffs, no form changes, no per-character overrides: just the five
 * standard slots (Skill1-3 and Ougis1-2, plus the normal attack) driven by
 * the xml. The AI is a basic fighter: it buys gear, retreats to heal, goes
 * for enemy heroes, then flogs and towers, using each skill when it is ready.
 *
 * Optional: when the unit is spawned as a clone, summon or kugutsu (declared in
 * CustomClones / CustomKuchiyose and linked in ns.LinkSummon) it does not fight
 * like a hero but like the game's own summons: it walks to the nearest flog,
 * enemy hero or tower and hits it, using only the skills its xml defines. It
 * never buys gear or retreats to heal. Units spawned as heroes are unaffected.
 *
 * Also optional: transformation. A form change itself needs nothing from this
 * class (the xml's setTransform event and ns.Transform do it), but the AI only
 * knows to use the transforming skill if the chain says so:
 *   ns.Transform = { { 'A', 'B', via = 'ougis2', below = 0.5 } }
 * Then, in any form that still has a next form, the AI uses that skill as soon
 * as it is ready and HP is at or below `below` (a fraction, default 1 = any HP),
 * and keeps the skill out of its normal attack choices. Without `via` nothing
 * changes: the transforming skill is used like any other.
 */
class GenericHero : public Hero
{
	void perform() override
	{
		if (isClone() || isSummon() || isKugutsu())
		{
			performSummon();
			return;
		}

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
				if (tryTransform())
					return;

				if (_isCanOugis2 && !isTransformSlot(5) && !_isControlled && getGameLayer()->_isOugis2Game && !_isArmored &&
					!_mainTarget->_isArmored && _mainTarget->getState() != State::KNOCKDOWN && !_mainTarget->_isSticking)
				{
					if (approach(sp, 48, 16))
						return;

					changeSide(sp);
					attack(OUGIS2);
					return;
				}
				else if (_isCanOugis1 && !isTransformSlot(4) && !_isControlled && !_isArmored && !_mainTarget->_isArmored)
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
					if (_isCanSkill1 && !isTransformSlot(1))
						attack(SKILL1);
					else if (_isCanSkill2 && !isTransformSlot(2))
						attack(SKILL2);
					else if (_isCanSkill3 && !isTransformSlot(3))
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
				if (_mainTarget->isFlog() && _isCanSkill2 && !isTransformSlot(2))
					attack(SKILL2);
				else if (_mainTarget->isTower() && _isCanSkill3 && !isTransformSlot(3))
					attack(SKILL3);
				else
					attack(NAttack);
			}
			return;
		}

		checkHealingState();
	}

	/** Transform hint of the current form (ns.Transform via / below), or nullptr. */
	const Custom::Registry::TransformAi *transformAi()
	{
		return Custom::Registry::get().transformAiOf(getName());
	}

	/** True when `slot` (1-3 skills, 4-5 ougis) is kept for the transform. */
	bool isTransformSlot(int slot)
	{
		auto ai = transformAi();
		return ai && ai->slot == slot;
	}

	/** Use the transforming skill when the chain asks for it. Returns true if the unit attacked. */
	bool tryTransform()
	{
		auto ai = transformAi();
		if (!ai || !Custom::Registry::get().transformNext(getName()))
			return false; // no hint, or already in the last form

		if (getHpPercent() > ai->below || _isControlled || !isFreeState())
			return false;

		switch (ai->slot)
		{
		case 1:
			if (!_isCanSkill1 || !getSkill1Action())
				return false;
			attack(SKILL1);
			return true;
		case 2:
			if (!_isCanSkill2 || !getSkill2Action())
				return false;
			attack(SKILL2);
			return true;
		case 3:
			if (!_isCanSkill3 || !getSkill3Action())
				return false;
			attack(SKILL3);
			return true;
		case 4:
			if (!_isCanOugis1 || _isArmored)
				return false;
			attack(OUGIS1);
			return true;
		case 5:
			if (!_isCanOugis2 || !getGameLayer()->_isOugis2Game || _isArmored)
				return false;
			attack(OUGIS2);
			return true;
		}
		return false;
	}

	/** Clone / kuchiyose / kugutsu behaviour, same shape as NarutoClone and the other stock summons. */
	void performSummon()
	{
		if (notFindFlog(0))
		{
			if (notFindHero(0))
			{
				if (notFindTower(0))
				{
					_mainTarget = nullptr;
				}
			}
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

				// only skills the xml actually defines (a summon often has none)
				if (_isCanSkill1 && getSkill1Action())
					attack(SKILL1);
				else if (_isCanSkill2 && getSkill2Action())
					attack(SKILL2);
				else if (_isCanSkill3 && getSkill3Action())
					attack(SKILL3);
				else
					attack(NAttack);
			}
			return;
		}

		stepOn();
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
