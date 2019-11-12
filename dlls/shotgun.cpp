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

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00  )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN	Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum shotgun_e {
	SHOTGUN_IDLE = 0,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_START_RELOAD,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP
};

class CShotgun : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy();
	void Holster();
	void Reload(void);
	void WeaponIdle(void);
private:
	int m_iShell;
	unsigned short m_usDoubleFire;
	unsigned short m_usSingleFire;
};
LINK_ENTITY_TO_CLASS(weapon_shotgun, CShotgun);

void CShotgun::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/w_shotgun.mdl");
	m_iDefaultAmmo = SHOTGUN_DEFAULT_GIVE;
	FallInit();// get ready to fall
}


void CShotgun::Precache(void)
{
	PRECACHE_MODEL("models/v_shotgun.mdl");
	PRECACHE_MODEL("models/w_shotgun.mdl");
	PRECACHE_MODEL("models/p_shotgun.mdl");

	m_iShell = PRECACHE_MODEL("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND("weapons/dbarrel1.wav");//shotgun
	PRECACHE_SOUND("weapons/sbarrel1.wav");//shotgun

	PRECACHE_SOUND("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND("weapons/reload3.wav");	// shotgun reload

	m_usSingleFire = PRECACHE_EVENT(1, "events/shotgun1.sc");
	m_usDoubleFire = PRECACHE_EVENT(1, "events/shotgun2.sc");
}

int CShotgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SHOTGUN_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = SHOTGUN_WEIGHT;

	return 1;
}

BOOL CShotgun::Deploy()
{
	return DefaultDeploy("models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW, "shotgun");
}

void CShotgun::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_GlobalTimeBase() + 0.5;
	SendWeaponAnim(SHOTGUN_HOLSTER);
}

void CShotgun::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound(4);
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip == 0)
	{
		PlayEmptySound(4);
		Reload();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	Vector vecDir;

	if (IsMultiplayer())
		vecDir = m_pPlayer->FireBulletsPlayer(4, vecSrc, vecAiming, VECTOR_CONE_DM_SHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	else	vecDir = m_pPlayer->FireBulletsPlayer(6, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usSingleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, pev->body, 0, 0, 0);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
	m_flNextSecondaryAttack = UTIL_GlobalTimeBase() + 1.0;

	if (m_iClip != 0) m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 5.0;
	else 		  m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 1.9;
	m_iChargeLevel = 0;
}


void CShotgun::SecondaryAttack(void)
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound(4);
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip == 1)
	{
		PrimaryAttack();
		return;
	}

	if (m_iClip == 0)
	{
		PlayEmptySound(4);
		Reload();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip -= 2;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	Vector vecDir;

	if (IsMultiplayer())
		vecDir = m_pPlayer->FireBulletsPlayer(8, vecSrc, vecAiming, VECTOR_CONE_DM_DOUBLESHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	else	vecDir = m_pPlayer->FireBulletsPlayer(12, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);


	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usDoubleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, pev->body, 0, 0, 0);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);


	m_flNextPrimaryAttack = GetNextAttackDelay(1.5);
	m_flNextSecondaryAttack = UTIL_GlobalTimeBase() + 1.5;

	if (m_iClip != 0) m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 6.0;
	else	        m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 1.9;
	m_iChargeLevel = 0;
	m_pPlayer->pev->punchangle.x -= 5;
}


void CShotgun::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP) return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_GlobalTimeBase()) return;

	// check to see if we're ready to reload
	if (m_iChargeLevel == 0)
	{
		SendWeaponAnim(SHOTGUN_START_RELOAD);
		m_iChargeLevel = 1;
		m_pPlayer->m_flNextAttack = UTIL_GlobalTimeBase() + 0.6;
		m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 0.6;
		m_flNextPrimaryAttack = UTIL_GlobalTimeBase() + 1.0;
		m_flNextSecondaryAttack = UTIL_GlobalTimeBase() + 1.0;
		return;
	}
	else if (m_iChargeLevel == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_GlobalTimeBase()) return;
		// was waiting for gun to move to side
		m_iChargeLevel = 2;

		if (RANDOM_LONG(0, 1)) EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));
		else		  EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));

		SendWeaponAnim(SHOTGUN_RELOAD);

		m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 0.6;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_iChargeLevel = 1;
		if (m_iClip == SHOTGUN_MAX_CLIP) m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 0.1;
	}
}



void CShotgun::WeaponIdle(void)
{
	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle < UTIL_GlobalTimeBase())
	{
		if (m_iClip == 0 && m_iChargeLevel == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]) Reload();
		else if (m_iChargeLevel != 0)
		{
			if (m_iClip != SHOTGUN_MAX_CLIP && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]) Reload();
			else
			{
				// reload debounce has timed out
				SendWeaponAnim(SHOTGUN_PUMP);
				m_iChargeLevel = 0;
				m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 1.5;
			}
		}
		else
		{

			float flRand = RANDOM_FLOAT(0, 1);
			if (flRand <= 0.5)
			{
				SendWeaponAnim(SHOTGUN_IDLE);
				m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + RANDOM_FLOAT(10, 15);
			}
		}
	}
}


class CShotgunAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_shotbox.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_shotbox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_BUCKSHOTBOX_GIVE, "buckshot", BUCKSHOT_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_buckshot, CShotgunAmmo);


