#include "../bot_common.h"

// Face the entity and "use" it
// NOTE: This state assumes we are standing in range of the entity to be used, with no obstructions.

void UseEntityState::OnEnter(CHLBot *me)
{
	;
}

void UseEntityState::OnUpdate(CHLBot *me)
{
	// in the very rare situation where two or more bots "used" a hostage at the same time,
	// one bot will fail and needs to time out of this state
	const float useTimeout = 5.0f;
	if (gpGlobals->time - me->GetStateTimestamp() > useTimeout || m_entity == NULL)
	{
		me->Idle();
		return;
	}

	// look at the entity
	Vector pos = m_entity->pev->origin + Vector(0, 0, HumanHeight * 0.5f);
	me->SetLookAt("Use entity", &pos, PRIORITY_HIGH);

	const char *classname = STRING(m_entity->pev->classname);
	float elapsed = gpGlobals->time - me->GetStateTimestamp();

	if (FStrEq(classname, "func_button"))
	{
		if (!me->IsButtonRecentlyPressed(m_entity))
		{
			me->UseEnvironment();
			me->MarkButtonPressed(m_entity);
		}

		if (elapsed < 0.5f)
			return;

		me->Idle();
		return;
	}

	me->UseEnvironment();

	if ((FStrEq(classname, "func_recharge") && me->pev->armorvalue < 100) ||
		(FStrEq(classname, "func_healthcharger") && me->pev->health < 100))
	{
		return;
	}

	me->Idle();
}

void UseEntityState::OnExit(CHLBot *me)
{
	me->ClearLookAt();
	me->SetGoalEntity(NULL);
	me->ResetStuckMonitor();
}
