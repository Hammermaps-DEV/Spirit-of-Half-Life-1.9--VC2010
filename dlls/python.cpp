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
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"


enum python_e {
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_HOLSTER,
	PYTHON_DRAW,
	PYTHON_IDLE2,
	PYTHON_IDLE3
};

class CPython : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy(void);
	void Holster();
	void Reload(void);
	void WeaponIdle(void);
	void UpdateSpot(void);
	float m_flSoundDelay;
	BOOL ShouldWeaponIdle(void) { return TRUE; };
	CLaserSpot *m_pSpot;
private:
	unsigned short m_usFirePython;
	int m_fSpotActive;//LTD allow only in multiplayer - no need save\restore
	int m_iShell;
};

LINK_ENTITY_TO_CLASS(weapon_python, CPython);
LINK_ENTITY_TO_CLASS(weapon_357, CPython);

int CPython::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = PYTHON_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_PYTHON;
	p->iWeight = PYTHON_WEIGHT;

	return 1;
}

void CPython::Spawn()
{
	pev->classname = MAKE_STRING("weapon_357"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_PYTHON;
	SET_MODEL(ENT(pev), "models/w_357.mdl");

	m_iDefaultAmmo = PYTHON_DEFAULT_GIVE;
	FallInit();// get ready to fall down.
}


void CPython::Precache(void)
{
	PRECACHE_MODEL("models/v_357.mdl");
	PRECACHE_MODEL("models/w_357.mdl");
	PRECACHE_MODEL("models/p_357.mdl");

	PRECACHE_SOUND("weapons/357_reload1.wav");
	m_usFirePython = PRECACHE_EVENT(1, "events/python.sc");
	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shellTE_MODEL
}

BOOL CPython::Deploy()
{
	if (IsMultiplayer()) m_iBody = 1;//enable laser sight geometry
	return DefaultDeploy("models/v_357.mdl", "models/p_357.mdl", PYTHON_DRAW, "python", 0.7);
}


void CPython::Holster()
{
	m_fInReload = FALSE;// cancel any reload in progress.
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6;

	SendWeaponAnim(PYTHON_HOLSTER);

	if (m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NEVER);
		m_pSpot = NULL;
	}
}

void CPython::SecondaryAttack(void)
{
	if (IsMultiplayer())
	{
		m_fSpotActive = !m_fSpotActive;

		if (!m_fSpotActive && m_pSpot)
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/spot_off.wav", 1, ATTN_NORM);
			m_pSpot->Killed(NULL, GIB_NORMAL);
			m_pSpot = NULL;
		}

	}
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CPython::PrimaryAttack()
{
	if (m_iClip && m_pPlayer->pev->waterlevel != 3)//don't fire underwater
	{
		if (m_pSpot && m_fSpotActive)	m_pSpot->Suspend(1.0);

		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

		m_iClip--;

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

		Vector vecDir;
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

		PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usFirePython, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, pev->body, 0, 0, 0);

		m_flNextPrimaryAttack = gpGlobals->time + 1.0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10, 15);
	}
	else
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7;
	}
}


void CPython::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0) return;
	if (m_pSpot && m_fSpotActive) m_pSpot->Suspend(2.0);

	if (DefaultReload(6, PYTHON_RELOAD, 2.0))
	{
		m_flSoundDelay = gpGlobals->time + 1.5;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10, 15);
}

void CPython::UpdateSpot(void)
{
	if (m_fSpotActive)
	{
		if (!m_pSpot)
		{
			m_pSpot = CLaserSpot::CreateSpot();
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/spot_on.wav", 1, ATTN_NORM);
		}

		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		Vector vecSrc = m_pPlayer->GetGunPosition();;
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ignore_glass, ENT(m_pPlayer->pev), &tr);
		float flLength = (tr.vecEndPos - vecSrc).Length();

		m_pSpot->pev->scale = flLength / 340;
		int m_iSpotBright = (1 / log(flLength / 0.3)) * 1700;
		if (m_iSpotBright > 255) m_iSpotBright = 255;

		//ALERT( at_console, "%f\n", flLength / 200);
		//ALERT( at_console, "%d\n", m_iSpotBright );

		m_iSpotBright = m_iSpotBright + RANDOM_LONG(1, flLength / 200);
		m_pSpot->pev->renderamt = m_iSpotBright;

		UTIL_SetOrigin(m_pSpot, tr.vecEndPos + tr.vecPlaneNormal * 0.1);

		//allow oriented LTD in multiplayer only, but python has LTD only in multiplayer - remove check
		Vector n = tr.vecPlaneNormal;
		n.x *= -1;
		n.y *= -1;
		m_pSpot->pev->angles = UTIL_VecToAngles(n);

	}
}

void CPython::WeaponIdle(void)
{
	UpdateSpot();

	if (m_flSoundDelay != 0 && m_flSoundDelay <= gpGlobals->time)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_reload1.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		m_flSoundDelay = 0;

		for (int i = 0; i < 6; i++)
		{
			EjectBrass(m_pPlayer->pev->origin, Vector(RANDOM_FLOAT(-10.0, 10.0), RANDOM_FLOAT(-10.0, 10.0), (float)0.0), m_pPlayer->pev->angles.y, m_iShell, TE_BOUNCE_SHELL);
		}
	}

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())return;

	// only idle if the slid isn't back
	if (m_iClip)
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.5)
		{
			iAnim = PYTHON_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (70.0 / 30.0);
		}
		else if (flRand <= 0.7)
		{
			iAnim = PYTHON_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (60.0 / 30.0);
		}
		else if (flRand <= 0.9)
		{
			iAnim = PYTHON_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (88.0 / 30.0);
		}
		else
		{
			if (!m_fSpotActive)
			{
				iAnim = PYTHON_FIDGET;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (170.0 / 30.0);
			}
		}
		SendWeaponAnim(iAnim);
	}
}



class CPythonAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_357ammobox.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_357ammobox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_357BOX_GIVE, "357", _357_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_357, CPythonAmmo);