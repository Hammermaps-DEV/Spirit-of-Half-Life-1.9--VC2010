/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

#define BOLT_AIR_VELOCITY	2000
#define BOLT_WATER_VELOCITY	1000

class CCrossbowBolt : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT ExplodeThink(void);

	int m_iTrail;

public:
	static CCrossbowBolt *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(crossbow_bolt, CCrossbowBolt);

CCrossbowBolt *CCrossbowBolt::BoltCreate(void)
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = GetClassPtr((CCrossbowBolt *)NULL);
	pBolt->pev->classname = MAKE_STRING("bolt");
	pBolt->Spawn();

	return pBolt;
}

void CCrossbowBolt::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	UTIL_SetOrigin(this, pev->origin);
	UTIL_SetSize(this, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch(&CCrossbowBolt::BoltTouch);
	SetThink(&CCrossbowBolt::BubbleThink);
	SetNextThink(0.2);
}


void CCrossbowBolt::Precache()
{
	PRECACHE_MODEL("models/crossbow_bolt.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("fvox/beep.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CCrossbowBolt::Classify(void)
{
	return	CLASS_NONE;
}

void CCrossbowBolt::BoltTouch(CBaseEntity *pOther)
{
	SetTouch(NULL);
	SetThink(NULL);

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t	*pevOwner;

		pevOwner = VARS(pev->owner);

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		if (pOther->IsPlayer())
		{
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowClient, pev->velocity.Normalize(), &tr, DMG_NEVERGIB);
		}
		else
		{
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowMonster, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
		}

		ApplyMultiDamage(pev, pevOwner);

		SetVelocityZero();
		
		// play body "thwack" sound
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM); break;
		}

		if (!IsMultiplayer())
		{
			Killed(pev, GIB_NEVER);
		}
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/xbow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));

		SetThink(&CCrossbowBolt::SUB_Remove);
		SetNextThink(10);// this will get changed below if the bolt is allowed to stick in what it hit.

		// if what we hit is static architecture, can stay around for a while.
		Vector vecDir = pev->velocity.Normalize();
		UTIL_SetOrigin(this, pev->origin - vecDir * 12);
		pev->angles = UTIL_VecToAngles(vecDir);
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_FLY;
		SetVelocityZero();
		pev->avelocity.z = 0;
		pev->angles.z = RANDOM_LONG(0, 360);

		if (pOther->IsBSPModel())
		{
			SetParent(pOther);//glue bolt with parent system
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(pev->origin);
		}
	}

	if (IsMultiplayer())
	{
		SetThink(&CCrossbowBolt::ExplodeThink);
		SetNextThink(0.1);
	}
}

void CCrossbowBolt::BubbleThink(void)
{
	SetNextThink(0.1);

	if (pev->waterlevel == 0 || pev->watertype <= CONTENT_FLYFIELD)
		return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}

void CCrossbowBolt::ExplodeThink(void)
{
	int iContents = UTIL_PointContents(pev->origin);
	int iScale;

	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	if (iContents != CONTENTS_WATER)
	{
		WRITE_SHORT(g_sModelIndexFireball);
	}
	else
	{
		WRITE_SHORT(g_sModelIndexWExplosion);
	}
	WRITE_BYTE(iScale); // scale * 10
	WRITE_BYTE(15); // framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	entvars_t *pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage(pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB);

	UTIL_Remove(this);
}

enum crossbow_e {
	CROSSBOW_IDLE1 = 0,	// full
	CROSSBOW_IDLE2,	// empty
	CROSSBOW_FIDGET1,	// full
	CROSSBOW_FIRE,	// full
	CROSSBOW_FIRE_LAST,	// to empty
	CROSSBOW_RELOAD,	// from empty
	CROSSBOW_DRAW1,	// full
	CROSSBOW_DRAW2,	// empty
	CROSSBOW_HOLSTER1,	// full
	CROSSBOW_HOLSTER2,	// empty
};
class CCrossbow : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);

	void FireBolt(void);
	void FireSniperBolt(void);
	void PrimaryAttack(void);
	void SecondaryAttack(void) { m_flNextSecondaryAttack = UTIL_GlobalTimeBase() + 1E6; };//just stub
	BOOL Deploy();
	void Holster();
	BOOL ShouldWeaponIdle(void) { return TRUE; };
	void Reload(void);
	void WeaponIdle(void);
	void ZoomUpdate(void);
	void ZoomReset(void);
	BOOL b_setup;
};
LINK_ENTITY_TO_CLASS(weapon_crossbow, CCrossbow);

void CCrossbow::Spawn()
{
	Precache();
	m_iId = WEAPON_CROSSBOW;
	SET_MODEL(ENT(pev), "models/w_crossbow.mdl");

	m_iDefaultAmmo = CROSSBOW_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CCrossbow::Precache(void)
{
	PRECACHE_MODEL("models/w_crossbow.mdl");
	PRECACHE_MODEL("models/v_crossbow.mdl");
	PRECACHE_MODEL("models/p_crossbow.mdl");

	PRECACHE_SOUND("weapons/xbow_fire1.wav");
	PRECACHE_SOUND("weapons/xbow_reload1.wav");

	UTIL_PrecacheOther("crossbow_bolt");
}


int CCrossbow::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "bolts";
	p->iMaxAmmo1 = BOLT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CROSSBOW_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 2;
	p->iId = WEAPON_CROSSBOW;
	p->iFlags = 0;
	p->iWeight = CROSSBOW_WEIGHT;
	return 1;
}


BOOL CCrossbow::Deploy()
{
	if (m_iClip)
		return DefaultDeploy("models/v_crossbow.mdl", "models/p_crossbow.mdl", CROSSBOW_DRAW1, "bow");
	return DefaultDeploy("models/v_crossbow.mdl", "models/p_crossbow.mdl", CROSSBOW_DRAW2, "bow");
}

void CCrossbow::Holster()
{
	m_fInReload = FALSE;// cancel any reload in progress.
	m_pPlayer->m_flNextAttack = UTIL_GlobalTimeBase() + 0.5;
	ZoomReset();

	if (m_iClip) SendWeaponAnim(CROSSBOW_HOLSTER1);
	else SendWeaponAnim(CROSSBOW_HOLSTER2);
}

void CCrossbow::PrimaryAttack(void)
{
	if (m_iChargeLevel && IsMultiplayer())
	{
		FireSniperBolt();
		return;
	}

	FireBolt();
}

// this function only gets called in multiplayer
void CCrossbow::FireSniperBolt()
{
	m_flNextPrimaryAttack = UTIL_GlobalTimeBase() + 0.75;

	if (m_iClip == 0)
	{
		PlayEmptySound();
		return;
	}

	TraceResult tr;

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_iClip--;

	// make twang sound
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/xbow_fire1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));

	if (m_iClip)
	{
		SendWeaponAnim(CROSSBOW_FIRE);
		m_iBody++;
	}
	else SendWeaponAnim(CROSSBOW_FIRE_LAST);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir = gpGlobals->v_forward;

	UTIL_TraceLine(vecSrc, vecSrc + vecDir * 8192, dont_ignore_monsters, m_pPlayer->edict(), &tr);

	if (tr.pHit->v.takedamage)
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0: EMIT_SOUND(tr.pHit, CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM); break;
		case 1: EMIT_SOUND(tr.pHit, CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM); break;
		}

		ClearMultiDamage();
		CBaseEntity::Instance(tr.pHit)->TraceAttack(m_pPlayer->pev, 120, vecDir, &tr, DMG_BULLET | DMG_NEVERGIB);
		ApplyMultiDamage(pev, m_pPlayer->pev);
	}
	else
	{
		// create a bolt
		CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate();
		pBolt->pev->origin = tr.vecEndPos - vecDir * 10;
		pBolt->pev->angles = UTIL_VecToAngles(vecDir);
		pBolt->pev->solid = SOLID_NOT;
		pBolt->SetTouch(NULL);
		pBolt->SetThink(&CCrossbowBolt::SUB_Remove);

		EMIT_SOUND(pBolt->edict(), CHAN_WEAPON, "weapons/xbow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM);

		if (UTIL_PointContents(tr.vecEndPos) != CONTENTS_WATER)
		{
			UTIL_Sparks(tr.vecEndPos);
		}

		if (FClassnameIs(tr.pHit, "worldspawn"))
		{
			// let the bolt sit around for a while if it hit static architecture
			pBolt->pev->nextthink = gpGlobals->time + 5.0;
		}
		else
		{
			pBolt->pev->nextthink = gpGlobals->time;
		}
	}
}

void CCrossbow::FireBolt()
{
	TraceResult tr;

	if (m_iClip == 0)
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	m_iClip--;

	// make twang sound
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/xbow_fire1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));

	if (m_iClip)
	{
		SendWeaponAnim(CROSSBOW_FIRE);
		m_iBody++;
	}
	else SendWeaponAnim(CROSSBOW_FIRE_LAST);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);

	anglesAim.x = -anglesAim.x;
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir = gpGlobals->v_forward;

	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();

	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		pBolt->SetVelocity(vecDir * BOLT_WATER_VELOCITY);
		pBolt->pev->speed = BOLT_WATER_VELOCITY;
	}
	else
	{
		pBolt->SetVelocity(vecDir * BOLT_AIR_VELOCITY);
		pBolt->pev->speed = BOLT_AIR_VELOCITY;
	}
	pBolt->pev->avelocity.z = 10;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_pPlayer->m_flNextAttack = UTIL_GlobalTimeBase() + 0.75;

	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 5.0;
	else	m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 0.75;
}


void CCrossbow::Reload(void)
{
	if (m_iClip) return;
	if (m_iChargeLevel) ZoomReset();
	m_iBody = 0;//show full
	if (DefaultReload(5, CROSSBOW_RELOAD, 4.7))
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/xbow_reload1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));
	}
}

void CCrossbow::ZoomUpdate(void)
{
	if (m_pPlayer->pev->button & IN_ATTACK2)
	{
		if (m_iChargeLevel == 0)
		{
			if (m_flShockTime > UTIL_GlobalTimeBase()) return;
			m_iChargeLevel = 1;
			m_flTimeUpdate = UTIL_GlobalTimeBase() + 0.5;
		}
		if (m_iChargeLevel == 1)
		{
			m_pPlayer->m_iFOV = 50;
			m_iChargeLevel = 2;//ready to zooming, wait for 0.5 secs
		}

		if (m_flTimeUpdate > UTIL_GlobalTimeBase()) return;
		if (m_iChargeLevel == 2 && m_pPlayer->m_iFOV > 20)
		{
			m_pPlayer->m_iFOV--;
			m_flTimeUpdate = UTIL_GlobalTimeBase() + 0.02;
		}
		if (m_iChargeLevel == 3) ZoomReset();
	}
	else if (m_iChargeLevel > 1) m_iChargeLevel = 3;
}

void CCrossbow::ZoomReset(void)
{
	m_flShockTime = UTIL_GlobalTimeBase() + 0.5;
	m_pPlayer->m_iFOV = 90;
	m_iChargeLevel = 0;//clear zoom
}

void CCrossbow::WeaponIdle(void)
{
	m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES);  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM
	ZoomUpdate();

	if (m_flTimeWeaponIdle < UTIL_GlobalTimeBase())
	{
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.75)
		{
			if (m_iClip)
			{
				SendWeaponAnim(CROSSBOW_IDLE1);
			}
			else
			{
				SendWeaponAnim(CROSSBOW_IDLE2);
			}
			m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + RANDOM_FLOAT(10, 15);
		}
		else if (m_iClip)
		{
			SendWeaponAnim(CROSSBOW_FIDGET1);
			m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 90.0 / 30.0;
		}
	}
}



class CCrossbowAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_crossbow_clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_crossbow_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_CROSSBOWCLIP_GIVE, "bolts", BOLT_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_crossbow, CCrossbowAmmo);
