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
#include "player.h"

enum satchel_e {
	SATCHEL_IDLE1 = 0,
	SATCHEL_FIDGET1,
	SATCHEL_DRAW,
	SATCHEL_DROP
};

enum satchel_radio_e {
	SATCHEL_RADIO_IDLE1 = 0,
	SATCHEL_RADIO_FIDGET1,
	SATCHEL_RADIO_DRAW,
	SATCHEL_RADIO_FIRE,
	SATCHEL_RADIO_HOLSTER
};

enum satchel_state {
	SATCHEL_IDLE = 0,
	SATCHEL_READY,
	SATCHEL_RELOAD,
};

class CSatchel : public CBasePlayerWeapon
{
public:
	int	Save(CSave &save);
	int	Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	int AddDuplicate(CBasePlayerItem *pOriginal);
	int CSatchel::AddToPlayer(CBasePlayer *pPlayer);
	BOOL CanDeploy(void);
	BOOL Deploy(void);
	BOOL IsUseable(void);

	void Holster();
	void WeaponIdle(void);
	void Throw(void);
};

class CSatchelCharge : public CGrenade
{
	void Spawn(void);
	void Precache(void);
	void BounceSound(void);

	void EXPORT SatchelSlide(CBaseEntity *pOther);
	void EXPORT SatchelThink(void);

	Vector m_lastBounceOrigin;	// Used to fix a bug in engine: when object isn't moving, but its speed isn't 0 and on ground isn't set

public:
	void Deactivate(void);
};
LINK_ENTITY_TO_CLASS(monster_satchel, CSatchelCharge);

//=========================================================
// Deactivate - do whatever it is we do to an orphaned 
// satchel when we don't want it in the world anymore.
//=========================================================
void CSatchelCharge::Deactivate(void)
{
	pev->solid = SOLID_NOT;
	UTIL_Remove(this);
}

void CSatchelCharge::Spawn(void)
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_satchel.mdl");
	UTIL_SetSize(this, Vector(-4, -4, -4), Vector(4, 4, 4));	// Uses point-sized, and can be stepped over
	UTIL_SetOrigin(this, pev->origin);

	SetTouch(&CSatchelCharge::SatchelSlide);
	SetUse(&CSatchelCharge::DetonateUse);
	SetThink(&CSatchelCharge::SatchelThink);
	SetNextThink(0.1);

	pev->gravity = 0.5;
	pev->friction = 0.8;

	pev->dmg = gSkillData.plrDmgSatchel;
	pev->sequence = 1;
}

void CSatchelCharge::SatchelSlide(CBaseEntity *pOther)
{
	entvars_t	*pevOther = pOther->pev;

	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner) return;
	pev->gravity = 1;// normal gravity now

	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 10), ignore_monsters, edict(), &tr);

	if (tr.flFraction < 1.0)
	{
		// add a bit of static friction
		SetVelocity(pev->velocity * 0.95);
		pev->avelocity = pev->avelocity * 0.9;
		// play sliding sound, volume based on velocity
	}
	if (!(pev->flags & FL_ONGROUND) && pev->velocity.Length2D() > 10)
	{
		// Fix for a bug in engine: when object isn't moving, but its speed isn't 0 and on ground isn't set
		if (pev->origin != m_lastBounceOrigin)
		{
			BounceSound();
		}
	}
	
	m_lastBounceOrigin = pev->origin;
	// There is no model animation so commented this out to prevent net traffic
	//StudioFrameAdvance( );
}

void CSatchelCharge::SatchelThink(void)
{
	// There is no model animation so commented this out to prevent net traffic
	//StudioFrameAdvance( );
	SetNextThink(0.1);

	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	if (pev->waterlevel == 3 && pev->watertype != CONTENT_FOG)
	{
		pev->movetype = MOVETYPE_FLY;
		SetVelocity(pev->velocity * 0.8);
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 8;
	}
	else if (pev->waterlevel == 0 || pev->watertype == CONTENT_FOG)
		pev->movetype = MOVETYPE_BOUNCE;
	else
		pev->velocity.z -= 8;
}

void CSatchelCharge::Precache(void)
{
	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_SOUND("weapons/g_bounce1.wav");
	PRECACHE_SOUND("weapons/g_bounce2.wav");
	PRECACHE_SOUND("weapons/g_bounce3.wav");
}

void CSatchelCharge::BounceSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce1.wav", 1, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce2.wav", 1, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce3.wav", 1, ATTN_NORM);	break;
	}
}
LINK_ENTITY_TO_CLASS(weapon_satchel, CSatchel);


//=========================================================
// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
//=========================================================
int CSatchel::AddDuplicate(CBasePlayerItem *pOriginal)
{
	if (IsMultiplayer())
	{
		CSatchel* pSatchel = (CSatchel*)pOriginal;
		if (pSatchel->m_chargeReady != SATCHEL_IDLE)
			return FALSE;
	}
	return CBasePlayerWeapon::AddDuplicate(pOriginal);
}

TYPEDESCRIPTION	CSatchel::m_SaveData[] =
{
	DEFINE_FIELD(CSatchel, m_chargeReady, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CSatchel, CBasePlayerWeapon);

//=========================================================
//=========================================================
int CSatchel::AddToPlayer(CBasePlayer *pPlayer)
{
	int bResult = CBasePlayerItem::AddToPlayer(pPlayer);

	pPlayer->pev->weapons |= (1 << m_iId);
	m_chargeReady = SATCHEL_IDLE; // this satchel charge weapon now forgets that any satchels are deployed by it.

	if (bResult) return AddWeapon();
	return FALSE;
}

void CSatchel::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_satchel.mdl");

	m_iDefaultAmmo = SATCHEL_DEFAULT_GIVE;
	FallInit();// get ready to fall down.
}


void CSatchel::Precache(void)
{
	PRECACHE_MODEL("models/v_satchel.mdl");
	PRECACHE_MODEL("models/v_satchel_radio.mdl");
	PRECACHE_MODEL("models/w_satchel.mdl");
	PRECACHE_MODEL("models/p_satchel.mdl");
	PRECACHE_MODEL("models/p_satchel_radio.mdl");

	UTIL_PrecacheOther("monster_satchel");
}


int CSatchel::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Satchel Charge";
	p->iMaxAmmo1 = SATCHEL_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 1;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId = WEAPON_SATCHEL;
	p->iWeight = SATCHEL_WEIGHT;

	return 1;
}

//=========================================================
//=========================================================
BOOL CSatchel::IsUseable(void)
{
	return CanDeploy();
}

BOOL CSatchel::CanDeploy(void)
{
	if (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0) return TRUE;
	if (m_chargeReady != SATCHEL_IDLE) return TRUE;
	return FALSE;
}

BOOL CSatchel::Deploy()
{
	m_pPlayer->m_flNextAttack = UTIL_GlobalTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + RANDOM_FLOAT(10, 15);

	if (m_chargeReady)
		return DefaultDeploy("models/v_satchel_radio.mdl", "models/p_satchel_radio.mdl", SATCHEL_RADIO_DRAW, "hive", 0.6);

	return DefaultDeploy("models/v_satchel.mdl", "models/p_satchel.mdl", SATCHEL_DRAW, "trip", 1.5);

}

void CSatchel::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_GlobalTimeBase() + 0.6;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);

	if (m_chargeReady) 
		SendWeaponAnim(SATCHEL_RADIO_HOLSTER);
	else
		SendWeaponAnim(SATCHEL_DROP);
	
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 && m_chargeReady != SATCHEL_READY)
		DestroyItem();
}

void CSatchel::PrimaryAttack()
{
	switch (m_chargeReady)
	{
	case SATCHEL_IDLE:
	{
		Throw();
	}
	break;
	case SATCHEL_READY:
	{
		SendWeaponAnim(SATCHEL_RADIO_FIRE);
		m_chargeReady = 2;
		m_iChargeLevel = 1;
		m_flNextPrimaryAttack = UTIL_GlobalTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_GlobalTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 1.5;
		m_flTimeUpdate = UTIL_GlobalTimeBase() + RANDOM_FLOAT(0.3, 0.5);
		break;
	}
	case SATCHEL_RELOAD:
	{
	}
	break;
	}
}

void CSatchel::SecondaryAttack(void)
{
	if (m_chargeReady != SATCHEL_RELOAD) Throw();
}

void CSatchel::Throw(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		Vector vecSrc = m_pPlayer->pev->origin;
		Vector vecThrow = gpGlobals->v_forward * 274 + m_pPlayer->pev->velocity;
		CBaseEntity *pSatchel = Create("monster_satchel", vecSrc, Vector(0, 0, 0), m_pPlayer->edict());
		pSatchel->SetVelocity(vecThrow);
		pSatchel->pev->avelocity.y = 400;

		m_chargeReady = SATCHEL_READY;
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_satchel_radio.mdl");
		m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_satchel_radio.mdl");
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		SendWeaponAnim(SATCHEL_RADIO_DRAW);

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		m_iOverloadLevel = 0;//reset
		m_flNextPrimaryAttack = UTIL_GlobalTimeBase() + 1.0;
		m_flNextSecondaryAttack = UTIL_GlobalTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 1.5;
	}
}


void CSatchel::WeaponIdle(void)
{
	if (m_flTimeUpdate < UTIL_GlobalTimeBase() && m_iChargeLevel)
	{
		edict_t *pPlayer = m_pPlayer->edict();
		CBaseEntity *pSatchel = NULL;
		while ((pSatchel = UTIL_FindEntityInSphere(pSatchel, m_pPlayer->pev->origin, 4096)) != NULL)
		{
			if (FClassnameIs(pSatchel->pev, "monster_satchel"))
			{
				if (pSatchel->pev->owner == pPlayer)
				{
					pSatchel->Use(m_pPlayer, m_pPlayer, USE_ON, 0);
					m_chargeReady = 2;
				}
			}
		}
		SendWeaponAnim(SATCHEL_RADIO_HOLSTER);
		m_iChargeLevel = 0;
	}

	if (m_flTimeWeaponIdle > UTIL_GlobalTimeBase()) return;
	switch (m_chargeReady)
	{
	case SATCHEL_IDLE:
		SendWeaponAnim(SATCHEL_FIDGET1);
		// use tripmine animations
		strcpy(m_pPlayer->m_szAnimExtention, "trip");
		break;
	case SATCHEL_READY:
		SendWeaponAnim(SATCHEL_RADIO_FIDGET1);
		// use hivehand animations
		strcpy(m_pPlayer->m_szAnimExtention, "hive");
		break;
	case SATCHEL_RELOAD:
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		{
			m_chargeReady = 0;
			RetireWeapon();
			return;
		}

		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_satchel.mdl");
		m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_satchel.mdl");
		SendWeaponAnim(SATCHEL_DRAW);

		// use tripmine animations
		strcpy(m_pPlayer->m_szAnimExtention, "trip");

		m_flNextPrimaryAttack = UTIL_GlobalTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_GlobalTimeBase() + 0.5;
		m_chargeReady = SATCHEL_IDLE;
		break;
	}
	m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + RANDOM_LONG(10, 15);// how long till we do this again.
}

//=========================================================
// DeactivateSatchels - removes all satchels owned by
// the provided player. Should only be used upon death.
//
// Made this global on purpose.
//=========================================================
void DeactivateSatchels(CBasePlayer *pOwner)
{
	edict_t* pFind = FIND_ENTITY_BY_CLASSNAME(NULL, "monster_satchel");
	while (!FNullEnt(pFind))
	{
		CBaseEntity *pEnt = CBaseEntity::Instance(pFind);
		CSatchelCharge *pSatchel = (CSatchelCharge *)pEnt;

		if (pSatchel)
		{
			if (pSatchel->pev->owner == pOwner->edict())
				pSatchel->Deactivate();
		}

		pFind = FIND_ENTITY_BY_CLASSNAME(pFind, "monster_satchel");
	}
}