/***
*
*   SPIRIT OF HALF-LIFE 1.9: OPPOSING-FORCE EDITION
*
*   Spirit of Half-Life and their logos are the property of their respective owners.
*   Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*   This product contains software technology licensed from Id
*   Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
*   All Rights Reserved.
*
*   Modifications by Hammermaps.de DEV Team (support@hammermaps.de).
*
***/
//=========================================================
// NPC: Scientist Zombie * http://half-life.wikia.com/wiki/Standard_Zombie
// For Spirit of Half-Life v1.9: Opposing-Force Edition
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"monster_zombie.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT	0x01
#define	ZOMBIE_AE_ATTACK_LEFT	0x02
#define	ZOMBIE_AE_ATTACK_BOTH	0x03

#define ZOMBIE_FLINCH_DELAY		2	// at most one flinch every n secs

//=========================================================
// Monster's link to Class & Saverestore Begins
//=========================================================
LINK_ENTITY_TO_CLASS(monster_zombie, CZombie);

TYPEDESCRIPTION	CZombie::m_SaveData[] = {
	DEFINE_FIELD(CZombie, m_flNextFlinch, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CZombie, CBaseMonster);

//=========================================================
// Monster Sounds
//=========================================================
const char *CZombie::pAttackHitSounds[] = {
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav"
};

const char *CZombie::pAttackMissSounds[] = {
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav"
};

const char *CZombie::pAttackSounds[] = {
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav"
};

const char *CZombie::pIdleSounds[] = {
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav"
};

const char *CZombie::pAlertSounds[] = {
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav"
};

const char *CZombie::pPainSounds[] = {
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav"
};

const char *CZombie::pDeathSounds[] = {
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav"
};

//=========================================================
// Spawn Scientist Zombie
//=========================================================
void CZombie::Spawn() {
	Precache();

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/zombie.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;

	if (pev->health == 0)
		pev->health = gSkillData.zombieHealth;

	pev->effects = 0;
	pev->view_ofs = VEC_VIEW;
	pev->yaw_speed = 120;

	m_flFieldOfView = VIEW_FIELD_NORMAL;
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP | bits_CAP_HEAR;
	m_flBulletDR = 0.3; //damage from bullets
	m_flDebug = false; //Debug Massages

	m_flHitgroupHead = gSkillData.zombieHead;
	m_flHitgroupChest = gSkillData.zombieChest;
	m_flHitgroupStomach = gSkillData.zombieStomach;
	m_flHitgroupArm = gSkillData.zombieArm;
	m_flHitgroupLeg = gSkillData.zombieLeg;
	m_flDmgOneSlash = gSkillData.zombieDmgOneSlash;
	m_flDmgBothSlash = gSkillData.zombieDmgBothSlash;

	MonsterInit();
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CZombie::Classify(void) {
	return m_iClass ? m_iClass : CLASS_ALIEN_MONSTER;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie::Precache() {
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/zombie.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
}

//=========================================================
// TakeDamage - overridden for zombie, take XX% damage from bullets
//=========================================================
int CZombie::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType) {
	if (pev->spawnflags & SF_MONSTER_SPAWNFLAG_64) {
		if (m_flDebug)
			ALERT(at_console, "%s:TakeDamage:SF_MONSTER_SPAWNFLAG_64\n", STRING(pev->classname));

		CBaseEntity *pEnt = Instance(pevAttacker);
		if (pEnt->IsPlayer()) {
			pev->health = pev->max_health / 2;
			if (flDamage > 0) //Override all damage
				SetConditions(bits_COND_LIGHT_DAMAGE);

			if (flDamage >= 20) //Override all damage
				SetConditions(bits_COND_HEAVY_DAMAGE);

			return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
		}

		if (pevAttacker->owner) {
			pEnt = Instance(pevAttacker->owner);
			if (pEnt->IsPlayer()) {
				pev->health = pev->max_health / 2;
				if (flDamage > 0) //Override all damage
					SetConditions(bits_COND_LIGHT_DAMAGE);

				if (flDamage >= 20) //Override all damage
					SetConditions(bits_COND_HEAVY_DAMAGE);

				return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
			}
		}
	}

	// Take xx% damage from bullets
	if (bitsDamageType == DMG_BULLET && m_flBulletDR != 0) {
		if (m_flDebug)
			ALERT(at_console, "%s:TakeDamage:DMG_BULLET:Reduce Damage\n", STRING(pev->classname));

		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce(flDamage);
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= m_flBulletDR;
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// TraceAttack - Damage based on Hitgroups
//=========================================================
void CZombie::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType) {
	if (!IsAlive()) {
		CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
		return;
	}

	if (pev->takedamage) {
		if (IsAlive() && RANDOM_LONG(0, 4) <= 2) { PainSound(); }
		if (pev->spawnflags & SF_MONSTER_SPAWNFLAG_64) {
			CBaseEntity *pEnt = Instance(pevAttacker);
			if (pEnt->IsPlayer()) { return; }
			if (pevAttacker->owner) {
				pEnt = Instance(pevAttacker->owner);
				if (pEnt->IsPlayer()) { return; }
			}
		}

		switch (ptr->iHitgroup) {
		case HITGROUP_HEAD:
			if (m_flDebug)
				ALERT(at_console, "%s:TraceAttack:HITGROUP_HEAD\n", STRING(pev->classname));
			flDamage = m_flHitgroupHead * flDamage;
			break;
		case HITGROUP_CHEST:
			if (m_flDebug)
				ALERT(at_console, "%s:TraceAttack:HITGROUP_CHEST\n", STRING(pev->classname));
			flDamage = m_flHitgroupChest * flDamage;
			break;
		case HITGROUP_STOMACH:
			if (m_flDebug)
				ALERT(at_console, "%s:TraceAttack:HITGROUP_STOMACH\n", STRING(pev->classname));
			flDamage = m_flHitgroupStomach * flDamage;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			if (m_flDebug)
				ALERT(at_console, "%s:TraceAttack:HITGROUP_ARM\n", STRING(pev->classname));
			flDamage = m_flHitgroupArm * flDamage;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			if (m_flDebug)
				ALERT(at_console, "%s:TraceAttack:HITGROUP_LEG\n", STRING(pev->classname));
			flDamage = m_flHitgroupLeg * flDamage;
			break;
		}
	}

	SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);// a little surface blood.
	TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================================
// IdleSound
//=========================================================
void CZombie::IdleSound(void) {
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pIdleSounds);
}

//=========================================================
// AlertSound 
//=========================================================
void CZombie::AlertSound(void) {
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAlertSounds);
}

//=========================================================
// PainSound 
//=========================================================
void CZombie::PainSound(void) {
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);
}

//=========================================================
// DeathSound 
//=========================================================
void CZombie::DeathSound(void) {
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);
}

//=========================================================
// AttackSound 
//=========================================================
void CZombie::AttackSound(void) {
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAttackSounds);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombie::HandleAnimEvent(MonsterEvent_t *pEvent) {
	switch (pEvent->event) {
	case ZOMBIE_AE_ATTACK_LEFT: {
		if (m_flDebug)
			ALERT(at_console, "%s:HandleAnimEvent:Slash left!\n", STRING(pev->classname));

		CBaseEntity *pHurt = CheckTraceHullAttack(70, m_flDmgOneSlash, DMG_SLASH);
		if (pHurt) {
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) {
				pHurt->pev->punchangle.z = -18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
			}

			EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSounds);
		}
		else
			EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSounds);

		if (RANDOM_LONG(0, 1)) { AttackSound(); }
	}
								break;
	case ZOMBIE_AE_ATTACK_RIGHT: {
		if (m_flDebug)
			ALERT(at_console, "%s:HandleAnimEvent:Slash right!\n", STRING(pev->classname));

		CBaseEntity *pHurt = CheckTraceHullAttack(70, m_flDmgOneSlash, DMG_SLASH);
		if (pHurt) {
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) {
				pHurt->pev->punchangle.z = 18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
			}

			EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSounds);
		}
		else
			EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSounds);
	}
								 break;
	case ZOMBIE_AE_ATTACK_BOTH: {
		if (m_flDebug)
			ALERT(at_console, "%s:HandleAnimEvent:Slash both!\n", STRING(pev->classname));

		CBaseEntity *pHurt = CheckTraceHullAttack(70, m_flDmgBothSlash, DMG_CLUB);
		if (pHurt) {
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) {
				pHurt->pev->punchangle.x = 20;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
			}

			EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSounds);
		}
		else
			EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSounds);

		if (RANDOM_LONG(0, 1)) { AttackSound(); }
	}
								break;
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// IgnoreConditions
//=========================================================
int CZombie::IgnoreConditions(void) {
	int iIgnore = CBaseMonster::IgnoreConditions();
	if ((m_Activity == ACT_MELEE_ATTACK1)) {
		if (m_flNextFlinch >= UTIL_GlobalTimeBase())
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH)) {
		if (m_flNextFlinch < UTIL_GlobalTimeBase())
			m_flNextFlinch = UTIL_GlobalTimeBase() + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
}