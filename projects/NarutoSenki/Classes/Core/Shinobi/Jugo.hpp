#pragma once
#include "Hero.hpp"
#include "BuffMechanics.hpp"

class Jugo : public Hero
{
	void perform() override
	{
		_mainTarget = nullptr;
		findHeroHalf();

		tryUseGear6();
		tryBuyGear(GearType::Gear06, GearType::Gear01, GearType::Gear02);

		if (needBackToTowerToRestoreHP() ||
			needBackToDefendTower())
			return;

		if (_mainTarget && _mainTarget->isNotFlog())
		{
			Vec2 moveDirection;
			Vec2 sp = getDistanceToTarget();

			if (isFreeState())
			{
				if (_isCanSkill3 && _mainTarget->getDEF() < 5000 && !_skillChangeBuffValue)
				{
					changeSide(sp);
					attack(SKILL3);
					return;
				}
				else if (enemyCombatPoint > friendCombatPoint && abs(enemyCombatPoint - friendCombatPoint) > 3000 && !_isHealing && !_isControlled)
				{
					if (abs(sp.x) < 160)
						stepBack2();
					else
						idle();
					return;
				}
				else if (abs(sp.x) < 128)
				{
					if (abs(sp.x) > 96 || abs(sp.y) > 32)
					{
						moveDirection = sp.getNormalized();
						walk(moveDirection);
						return;
					}

					if (_isCanOugis2 && !_isControlled && getGameLayer()->_isOugis2Game && !_skillChangeBuffValue && _mainTarget->getDEF() < 5000 && !_mainTarget->_isArmored && _mainTarget->getState() != State::KNOCKDOWN && !_mainTarget->_isSticking)
					{
						if (abs(sp.x) > 32 || abs(sp.y) > 32)
						{
							moveDirection = sp.getNormalized();
							walk(moveDirection);
							return;
						}
						changeSide(sp);
						attack(OUGIS2);
					}
					else if (_isCanOugis1 && !_isControlled && _mainTarget->getDEF() < 5000 && !_skillChangeBuffValue)
					{
						if (abs(sp.x) > 32 || abs(sp.y) > 32)
						{
							moveDirection = sp.getNormalized();
							walk(moveDirection);
							return;
						}

						changeSide(sp);
						attack(OUGIS1);
					}
					else if (_isCanSkill2 && _mainTarget->getDEF() < 5000 && !_skillChangeBuffValue)
					{
						changeSide(sp);
						attack(SKILL2);
						return;
					}
					else if (_isCanSkill1 && _mainTarget->getDEF() < 5000 && !_skillChangeBuffValue)
					{
						changeSide(sp);
						attack(SKILL1);
					}
					else
					{
						if (abs(sp.x) > 48 || abs(sp.y) > 32)
						{
							moveDirection = sp.getNormalized();
							walk(moveDirection);
							return;
						}
						else
						{
							changeSide(sp);
							attack(NAttack);
						}
					}

					return;
				}
			}
		}
		_mainTarget = nullptr;
		if (notFindFlogHalf())
			findTowerHalf();

		if (_mainTarget)
		{
			Vec2 moveDirection;
			Vec2 sp = getDistanceToTarget();

			if (abs(sp.x) > 32 || abs(sp.y) > 32)
			{
				moveDirection = sp.getNormalized();
				walk(moveDirection);
				return;
			}

			if (isFreeState())
			{
				if (_mainTarget->isFlog() && _isCanSkill1 && !_skillChangeBuffValue)
				{
					changeSide(sp);
					attack(SKILL1);
				}
				else
				{
					changeSide(sp);
					attack(NAttack);
				}
			}

			return;
		}

		checkHealingState();
	}

	void changeAction() override
	{
		BuffMechanics::jugoChange(*this);
	}

	void setActionResume() override
	{
		BuffMechanics::jugoActionResume(*this);
	}
};
