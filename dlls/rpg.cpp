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


enum rpg_e {
	RPG_IDLE = 0,
	RPG_IDLE2,
	RPG_DRAW,
	RPG_HOLSTER,
	RPG_FIRE,
	RPG_RELOAD
};

class CRpg : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	void Reload(void);
	int GetItemInfo(ItemInfo *p);

	BOOL Deploy(void);
	void Holster();
	void UpdateScreen(void);
	void ShutdownScreen(void);
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void WeaponIdle(void);
	void UpdateSpot(void);
	int AddDuplicate(CBasePlayerItem *pOriginal) { return FALSE; };//don't give second launcher!
	BOOL ShouldWeaponIdle(void) { return TRUE; };

	CLaserSpot *m_pSpot;
	CLaserSpot *m_pMirSpot;
};

class CRpgRocket : public CGrenade
{
public:
	int	Save(CSave &save);
	int	Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
	void Spawn(void);
	void Precache(void);
	void EXPORT FollowThink(void);
	void EXPORT IgniteThink(void);
	void EXPORT RocketTouch(CBaseEntity *pOther);
	void Detonate(void);
	void CreateTrail(void);
	static CRpgRocket *Create(Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher);

	int m_iTrail;
	float m_flIgniteTime;
	CRpg *m_pLauncher;// pointer back to the launcher that fired me. 
	BOOL b_setup;
};

LINK_ENTITY_TO_CLASS(weapon_rpg, CRpg);
LINK_ENTITY_TO_CLASS(rpg_rocket, CRpgRocket);

//===========================
//
//	Rocket code
//
//===========================

CRpgRocket *CRpgRocket::Create(Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher)
{
	CRpgRocket *pRocket = GetClassPtr((CRpgRocket *)NULL);

	UTIL_SetOrigin(pRocket, vecOrigin);
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch(&CRpgRocket::RocketTouch);
	pRocket->m_pLauncher = pLauncher;// remember what RPG fired me. 
	pRocket->m_pLauncher->m_iChargeLevel++;// register this missile as active for the launcher
	pRocket->pev->owner = pOwner->edict();

	return pRocket;
}

TYPEDESCRIPTION	CRpgRocket::m_SaveData[] =
{
	DEFINE_FIELD(CRpgRocket, m_flIgniteTime, FIELD_TIME),
	DEFINE_FIELD(CRpgRocket, m_pLauncher, FIELD_CLASSPTR),
};
IMPLEMENT_SAVERESTORE(CRpgRocket, CGrenade);

void CRpgRocket::Spawn(void)
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->renderfx = kRenderFxEntInPVS;//for telemetric rocket
	SET_MODEL(ENT(pev), "models/rpgrocket.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin(this, pev->origin);

	pev->classname = MAKE_STRING("rpg_rocket");

	SetThink(&CRpgRocket::IgniteThink);
	SetTouch(&CRpgRocket::ExplodeTouch);

	pev->angles.x -= 30;
	UTIL_MakeVectors(pev->angles);
	pev->angles.x = -(pev->angles.x + 30);
	pev->velocity = gpGlobals->v_forward * 250;
	pev->gravity = 0.5;

	SetNextThink(0.4);

	pev->dmg = gSkillData.plrDmgRPG;
}

void CRpgRocket::RocketTouch(CBaseEntity *pOther)
{
	CBaseEntity *pPlayer = CBaseEntity::Instance(pev->owner);
	if (m_pLauncher)
	{
		// my launcher is still around, tell it I'm dead.
		m_pLauncher->m_iChargeLevel--;
		if (m_pLauncher->m_pSpot)//make sound only if laser spot created
			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "weapons/beep2.wav", 1, ATTN_NORM);//play sound at player
	}
	STOP_SOUND(edict(), CHAN_VOICE, "weapons/rocket1.wav");
	ExplodeTouch(pOther);
}

void CRpgRocket::Detonate(void)
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!
	CBaseEntity *pPlayer = CBaseEntity::Instance(pev->owner);
	if (m_pLauncher)
	{
		// my launcher is still around, tell it I'm dead.
		m_pLauncher->m_iChargeLevel--;
		if (m_pLauncher->m_pSpot)//make sound only if laser spot created
			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "weapons/beep2.wav", 1, ATTN_NORM);//play sound at player
	}
	STOP_SOUND(edict(), CHAN_VOICE, "weapons/rocket1.wav");
	vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST);
}

void CRpgRocket::Precache(void)
{
	PRECACHE_MODEL("models/rpgrocket.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND("weapons/rocket1.wav");
	PRECACHE_SOUND("weapons/beep2.wav");
}


void CRpgRocket::IgniteThink(void)
{
	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	CreateTrail();
	m_flIgniteTime = gpGlobals->time;

	// set to follow laser spot
	SetThink(&CRpgRocket::FollowThink);
	SetNextThink(0.1);
}

void CRpgRocket::CreateTrail(void)
{
	if (!b_setup)
	{
		// make rocket sound after save\load
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5);
		// restore rocket trail
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_BEAMFOLLOW);
		WRITE_SHORT(entindex());	// entity
		WRITE_SHORT(m_iTrail);	// model
		WRITE_BYTE(30); // life
		WRITE_BYTE(5);  // width
		WRITE_BYTE(200);   // r, g, b
		WRITE_BYTE(200);   // r, g, b
		WRITE_BYTE(200);   // r, g, b
		WRITE_BYTE(200);	// brightness
		MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
		b_setup = TRUE;
	}
}


void CRpgRocket::FollowThink(void)
{
	CBaseEntity *pOther = NULL;
	Vector vecTarget;
	Vector vecDir;
	float flDist, flMax, flDot;
	TraceResult tr;
	UTIL_MakeAimVectors(pev->angles);

	CreateTrail();

	vecTarget = gpGlobals->v_forward;
	flMax = 4096;
	// Examine all entities within a reasonable radius
	while ((pOther = UTIL_FindEntityByClassname(pOther, "laser_spot")) != NULL)
	{
		UTIL_TraceLine(pev->origin, pOther->pev->origin, dont_ignore_monsters, ENT(pev), &tr);
		// ALERT( at_console, "%f\n", tr.flFraction );
		if (tr.flFraction >= 0.90)
		{
			vecDir = pOther->pev->origin - pev->origin;
			flDist = vecDir.Length();
			vecDir = vecDir.Normalize();
			flDot = DotProduct(gpGlobals->v_forward, vecDir);
			if ((flDot > 0) && (flDist * (1 - flDot) < flMax))
			{
				flMax = flDist * (1 - flDot);
				vecTarget = vecDir;
			}
		}
	}

	pev->angles = UTIL_VecToAngles(vecTarget);

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();
	if (gpGlobals->time - m_flIgniteTime < 1.0)
	{
		pev->velocity = pev->velocity * 0.2 + vecTarget * (flSpeed * 0.8 + 400);
		if (pev->waterlevel == 3 && pev->watertype > CONTENT_FLYFIELD)
		{
			// go slow underwater
			if (pev->velocity.Length() > 300)
			{
				pev->velocity = pev->velocity.Normalize() * 300;
			}
			UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 4);
		}
		else
		{
			if (pev->velocity.Length() > 2000)
			{
				pev->velocity = pev->velocity.Normalize() * 2000;
			}
		}
	}
	else
	{
		if (pev->effects & EF_LIGHT)
		{
			pev->effects = 0;
			STOP_SOUND(ENT(pev), CHAN_VOICE, "weapons/rocket1.wav");
		}
		pev->velocity = pev->velocity * 0.2 + vecTarget * flSpeed * 0.798;
		if ((pev->waterlevel == 0 || pev->watertype == CONTENT_FOG) && pev->velocity.Length() < 1500)
		{
			Detonate();
		}
	}
	//ALERT( at_console, "%.0f\n", flSpeed );

	SetNextThink(0.05);
}

//===========================
//
//	Rocket launcher code
//
//===========================

//m_iOverloadLevel is LTD trigger like m_fSpotActive in original HL
//m_iChargeLevel is indicator what indicates how many missiles in air
//fired from this laubcher

void CRpg::Spawn()
{
	Precache();
	m_iId = WEAPON_RPG;

	SET_MODEL(ENT(pev), "models/w_rpg.mdl");
	m_iOverloadLevel = TRUE;//turn on LTD

	if (IsMultiplayer())m_iDefaultAmmo = RPG_DEFAULT_GIVE * 2;
	else	m_iDefaultAmmo = RPG_DEFAULT_GIVE;
	FallInit();// get ready to fall down.
}

void CRpg::Precache(void)
{
	PRECACHE_MODEL("models/w_rpg.mdl");
	PRECACHE_MODEL("models/v_rpg.mdl");
	PRECACHE_MODEL("models/p_rpg.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	UTIL_PrecacheOther("laser_spot");
	UTIL_PrecacheOther("rpg_rocket");

	PRECACHE_SOUND("weapons/rocketfire1.wav");
	PRECACHE_SOUND("weapons/glauncher.wav"); // alternative fire sound
	PRECACHE_SOUND("weapons/beep.wav");
	PRECACHE_SOUND("weapons/spot_on.wav");
	PRECACHE_SOUND("weapons/spot_off.wav");
}

int CRpg::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "rockets";
	p->iMaxAmmo1 = ROCKET_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = RPG_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_RPG;
	p->iFlags = 0;
	p->iWeight = RPG_WEIGHT;

	return 1;
}

BOOL CRpg::Deploy()
{
	return DefaultDeploy("models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW, "rpg");
}

void CRpg::Holster()
{
	ShutdownScreen();//set skin to 0 manually
	m_fInReload = FALSE;// cancel any reload in progress.
	m_pPlayer->m_flNextAttack = UTIL_GlobalTimeBase() + 0.5;
	SendWeaponAnim(RPG_HOLSTER);
}

void CRpg::PrimaryAttack()
{
	if (m_iClip)
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

		m_iBody = 1; //hide rocket in laubcher
		SendWeaponAnim(RPG_FIRE);
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

		CRpgRocket *pRocket = CRpgRocket::Create(vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this);
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);// RpgRocket::Create stomps on globals, so remake.
		pRocket->pev->velocity = pRocket->pev->velocity + gpGlobals->v_forward * DotProduct(m_pPlayer->pev->velocity, gpGlobals->v_forward);

		// firing RPG no longer turns on the designator. ALT fire is a toggle switch for the LTD.
		// Ken signed up for this as a global change (sjb)

		m_iClip--;
		m_flNextPrimaryAttack = UTIL_GlobalTimeBase() + 1.5;
		m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 1.5;
	}
	else
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_GlobalTimeBase() + 0.7;//no longer indicate fps :)
	}
	UpdateSpot();
}

void CRpg::SecondaryAttack()
{
	m_iOverloadLevel = !m_iOverloadLevel;
	if (!m_iOverloadLevel && m_pSpot) ShutdownScreen();//simply call shutdown function

	m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_GlobalTimeBase() + 0.3;
}

void CRpg::Reload(void)
{
	if ((m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0) || (m_iClip == 1)) return;

	m_flNextPrimaryAttack = UTIL_GlobalTimeBase() + 0.5;

	if (m_iChargeLevel && m_iOverloadLevel) return;

	if (m_iClip == 0)
	{
		if (m_pSpot && m_iOverloadLevel) m_pSpot->Suspend(2.1);
		if (m_pMirSpot && m_iOverloadLevel) m_pMirSpot->Suspend(2.1);
		m_iBody = 0;//show rocket
		DefaultReload(RPG_MAX_CLIP, RPG_RELOAD, 2.1);
		m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + RANDOM_FLOAT(10, 15);
	}
}

void CRpg::UpdateSpot(void)
{
	if (m_iOverloadLevel)
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

		//disable scale feature
		//m_pSpot->pev->scale = flLength / 650;
		int m_iSpotBright = (1 / log(flLength / 0.3)) * 1700;
		if (m_iSpotBright > 255) m_iSpotBright = 255;

		m_iSpotBright = m_iSpotBright + RANDOM_LONG(1, flLength / 200);
		m_pSpot->pev->renderamt = m_iSpotBright;

		UTIL_SetOrigin(m_pSpot, tr.vecEndPos + tr.vecPlaneNormal * 0.1);

		Vector mirpos = UTIL_MirrorPos(tr.vecEndPos + tr.vecPlaneNormal * 0.1);
		if (mirpos != Vector(0, 0, 0))
		{
			if (!m_pMirSpot)m_pMirSpot = CLaserSpot::CreateSpot();
			UTIL_SetOrigin(m_pMirSpot, mirpos);
			m_pMirSpot->pev->renderamt = m_iSpotBright;
		}
	}

	UpdateScreen();// update rocket screen
}

void CRpg::UpdateScreen(void)
{
	if (m_flTimeUpdate > UTIL_GlobalTimeBase()) return;

	if (m_pSpot)
	{
		if (m_iChargeLevel)
		{
			if (pev->skin >= 4) pev->skin = 1;
			pev->skin++;
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/beep.wav", 1, ATTN_NORM);
		}
		else pev->skin = 5;
	}
	else pev->skin = 0;

	m_flTimeUpdate = UTIL_GlobalTimeBase() + 0.3;
}

void CRpg::ShutdownScreen(void)
{
	pev->skin = 0;
	if (m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NEVER);
		m_pSpot = NULL;
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/spot_off.wav", 1, ATTN_NORM);
	}
	if (m_pMirSpot)
	{
		m_pMirSpot->Killed(NULL, GIB_NEVER);
		m_pMirSpot = NULL;
	}
}

void CRpg::WeaponIdle(void)
{
	UpdateSpot();

	if (m_flTimeWeaponIdle > UTIL_GlobalTimeBase()) return;

	if (m_iClip && !m_iOverloadLevel)
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.75)
		{
			iAnim = RPG_IDLE;
			m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 6.3;
		}
		else
		{
			iAnim = RPG_IDLE2;
			m_flTimeWeaponIdle = UTIL_GlobalTimeBase() + 6.3;
		}
		SendWeaponAnim(iAnim);
	}
}



class CRpgAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_rpgammo.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_rpgammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int iGive;

		if (IsMultiplayer())
		{
			// hand out more ammo per rocket in multiplayer.
			iGive = AMMO_RPGCLIP_GIVE * 2;
		}
		else
		{
			iGive = AMMO_RPGCLIP_GIVE;
		}
		if (pOther->GiveAmmo(iGive, "rockets", ROCKET_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_rpgclip, CRpgAmmo);