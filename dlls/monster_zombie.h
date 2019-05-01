/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// NPC: Scientist Zombie * http://half-life.wikia.com/wiki/Standard_Zombie
// For Spirit of Half-Life v1.9: Opposing-Force Edition
//=========================================================
#ifndef MONSTER_ZOMBIE_H
#define MONSTER_ZOMBIE_H

// class definition
class CZombie : public CBaseMonster {
	public:
		virtual void Spawn(void);
		virtual void Precache(void);
		virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
		virtual void PainSound(void);
		virtual void DeathSound(void);
		virtual void AlertSound(void);
		virtual void IdleSound(void);
		virtual void AttackSound(void);
		virtual void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

		virtual BOOL CheckRangeAttack1(float flDot, float flDist) { return FALSE; }
		virtual BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }

		virtual int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
		virtual int IgnoreConditions(void);
		virtual int Classify(void);

		virtual int	Save(CSave &save);
		virtual int	Restore(CRestore &restore);

		static TYPEDESCRIPTION m_SaveData[];

		static const char *pAttackSounds[];
		static const char *pIdleSounds[];
		static const char *pAlertSounds[];
		static const char *pPainSounds[];
		static const char *pDeathSounds[];
		static const char *pAttackHitSounds[];
		static const char *pAttackMissSounds[];

	protected:
		BOOL m_flDebug = false;
		float m_flBulletDR = 0.0;
		float m_flNextFlinch;
		float m_flHitgroupHead;
		float m_flHitgroupChest;
		float m_flHitgroupStomach;
		float m_flHitgroupArm;
		float m_flHitgroupLeg;
		float m_flDmgOneSlash;
		float m_flDmgBothSlash;
};

#endif // MONSTER_ZOMBIE_H