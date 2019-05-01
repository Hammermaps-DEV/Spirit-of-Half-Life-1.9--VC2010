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

enum glock_e {
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_HOLSTER2,
	GLOCK_ADD_SILENCER,
	GLOCK_DEL_SILENCER
};

class CGlock : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer(CBasePlayer *pPlayer);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy(void);
	void Holster();
	void Reload(void);
	void WeaponIdle(void);
private:
	unsigned short m_usFireGlock;
};
LINK_ENTITY_TO_CLASS(weapon_glock, CGlock);
LINK_ENTITY_TO_CLASS(weapon_9mmhandgun, CGlock);


void CGlock::Spawn()
{
	pev->classname = MAKE_STRING("weapon_9mmhandgun"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_GLOCK;
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");
	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;
	FallInit();// get ready to fall down.
}

void CGlock::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.8;
	SendWeaponAnim(GLOCK_HOLSTER);
}

void CGlock::Precache(void)
{
	PRECACHE_MODEL("models/v_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	int m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/pl_gun1.wav");//silenced handgun
	PRECACHE_SOUND("weapons/pl_gun2.wav");//silenced handgun
	PRECACHE_SOUND("weapons/pl_gun3.wav");//handgun

	m_usFireGlock = PRECACHE_EVENT(1, "events/glock1.sc");
}

int CGlock::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

int CGlock::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CGlock::Deploy()
{
	return DefaultDeploy("models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW, "onehanded", 0.8);
}

void CGlock::SecondaryAttack(void)
{
	if (m_iBody == 0)
	{
		SendWeaponAnim(GLOCK_HOLSTER2);
		m_iBody = 1;
		m_iOverloadLevel = 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
	else
	{
		SendWeaponAnim(GLOCK_DEL_SILENCER);
		m_iBody = 0;
		m_iOverloadLevel = 2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;
	}
	m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 4.0;
}

void CGlock::PrimaryAttack(void)
{
	if (m_iClip && m_pPlayer->pev->waterlevel != 3)//don't fire underwater
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		// silenced
		if (m_iBody)
		{
			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
			m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
		}
		else
		{
			// non-silenced
			m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
			m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
		}

		m_iClip--;

		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming;

		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

		Vector vecDir;
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(0.02, 0.02, 0.02), 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

		PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usFireGlock, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, pev->body, 0, (m_iClip == 0) ? 1 : 0, m_iBody);

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.35;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10, 15);
	}
	else
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7;
	}
}

void CGlock::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0) return;

	if (m_iClip == 0) DefaultReload(17, GLOCK_RELOAD, 1.5);
	else	DefaultReload(17, GLOCK_RELOAD_NOT_EMPTY, 1.5);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10, 15);
}



void CGlock::WeaponIdle(void)
{
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase()) return;
	if (m_iOverloadLevel == 1)
	{
		SendWeaponAnim(GLOCK_ADD_SILENCER);
		m_iOverloadLevel = 0; //silencer added
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;
		return;
	}
	if (m_iOverloadLevel == 2)
	{
		SendWeaponAnim(GLOCK_DRAW);
		m_iOverloadLevel = 0; //silencer removed
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.2;
		return;
	}
	// only idle if the slid isn't back
	if (m_iClip)
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1.2);
		if (flRand < 0.2)
		{
			iAnim = GLOCK_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.0;
		}
		else if (0.4 > flRand && flRand > 0.2)
		{
			iAnim = GLOCK_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.8;
		}
		else if (0.8 > flRand && flRand > 0.5)
		{
			iAnim = GLOCK_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.7;
		}
		else	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_LONG(10, 30);
		SendWeaponAnim(iAnim);
	}
}




class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_glockclip, CGlockAmmo);
LINK_ENTITY_TO_CLASS(ammo_9mmclip, CGlockAmmo);