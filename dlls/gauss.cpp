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
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"


#define PRIMARY_CHARGE_VOLUME	256// how loud gauss is while charging
#define PRIMARY_FIRE_VOLUME	450// how loud gauss is when discharged

enum gauss_e {
	GAUSS_IDLE = 0,
	GAUSS_IDLE2,
	GAUSS_FIDGET,
	GAUSS_SPINUP,
	GAUSS_SPIN,
	GAUSS_FIRE,
	GAUSS_FIRE2,
	GAUSS_HOLSTER,
	GAUSS_DRAW
};

class CGauss : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);

	bool Deploy( void );
	void Holster( );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );
	
	void StartFire( void );
	void Fire( Vector vecOrigSrc, Vector vecDirShooting, float flDamage );
	int m_iSoundState;
private:
	unsigned short m_usGaussFire;
	unsigned short m_usGaussSpin;
};
LINK_ENTITY_TO_CLASS( weapon_gauss, CGauss );


void CGauss::Spawn( )
{
	Precache( );
	m_iId = WEAPON_GAUSS;
	SET_MODEL(ENT(pev), "models/w_gauss.mdl");
	m_iDefaultAmmo = GAUSS_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CGauss::Precache( void )
{
	PRECACHE_MODEL("models/w_gauss.mdl");
	PRECACHE_MODEL("models/v_gauss.mdl");
	PRECACHE_MODEL("models/p_gauss.mdl");

	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("weapons/electro5.wav");
	PRECACHE_SOUND("weapons/electro6.wav");
	
	//precached client resourses
	int m_iGlow = PRECACHE_MODEL( "sprites/hotglow.spr" );
	int m_iBalls = PRECACHE_MODEL( "sprites/hotglow.spr" );
	int m_iBeam = PRECACHE_MODEL( "sprites/smoke.spr" );

	m_usGaussFire = PRECACHE_EVENT( 1, "events/gauss.sc" );
	m_usGaussSpin = PRECACHE_EVENT( 1, "events/gaussspin.sc" );
	m_flChargeTime = UTIL_WeaponTimeBase() - 10;//instant zap after save\load
}

int CGauss::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_GAUSS;
	p->iFlags = 0;
	p->iWeight = GAUSS_WEIGHT;

	return 1;
}

bool CGauss::Deploy( )
{
	AnimRestore = true;
	m_flShockTime = 0;
	return DefaultDeploy( "models/v_gauss.mdl", "models/p_gauss.mdl", GAUSS_DRAW, "gauss", 0.6 );
}

void CGauss::Holster( )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( GAUSS_HOLSTER );
	m_fInAttack = 0;
	m_iChargeLevel = 0;
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}


void CGauss::PrimaryAttack()
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel != 3 && m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] > 2)
	{
		m_pPlayer->m_iWeaponVolume = PRIMARY_FIRE_VOLUME;
		pev->frags = true;//set primary fire

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 2;

		StartFire();
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2;
	}
	else
	{
		PlayEmptySound( 1 );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}
}

void CGauss::SecondaryAttack()
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		if ( m_fInAttack != 0 )
		{
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 80 + RANDOM_LONG(0,0x3f));
			SendWeaponAnim( GAUSS_IDLE );
			m_fInAttack = 0;
			m_iChargeLevel = 0;
		}
		else	PlayEmptySound( 1 );

		m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	if ( m_fInAttack == 0 )
	{
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		{
			PlayEmptySound( 1 );
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
			return;
		}

		pev->frags = FALSE;//set secondary attack
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;// take one ammo just to start the spin
		m_iChargeLevel++;

		// spin up
		m_pPlayer->m_iWeaponVolume = PRIMARY_CHARGE_VOLUME;
		
		SendWeaponAnim( GAUSS_SPINUP );
		m_fInAttack = 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		m_flChargeTime = UTIL_WeaponTimeBase();
		PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usGaussSpin, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 110, 0, 0, 0 );
		m_iSoundState = SND_CHANGE_PITCH;
	}
	else if (m_fInAttack == 1)
	{
		if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
		{
			SendWeaponAnim( GAUSS_SPIN );
			m_fInAttack = 2;
		}
	}
	else if (m_fInAttack == 2)
	{
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		{
			// out of ammo! force the gun to fire
			StartFire();
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
			return;
		}

		if ( m_flTimeUpdate < UTIL_WeaponTimeBase())
		{
			if(m_iChargeLevel != 16)
			{
				m_iChargeLevel++;
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
				if ( IsMultiplayer() ) m_flTimeUpdate = UTIL_WeaponTimeBase() + 0.05;
				else		       m_flTimeUpdate = UTIL_WeaponTimeBase() + 0.15;
				int pitch = 95 + 10 * m_iChargeLevel;
				PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usGaussSpin, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, pitch, 0, ( m_iSoundState == SND_CHANGE_PITCH ) ? 1 : 0, 0 );
			}
			if(m_iChargeLevel == 16)//end of charge
			{	
				m_fInAttack = 3;//unused value
			}			
		}
	}
	if(!AnimRestore)
	{
		if(m_fInAttack == 1) SendWeaponAnim( GAUSS_SPINUP );
		if(m_fInAttack >= 2) SendWeaponAnim( GAUSS_SPIN );
		AnimRestore = true;
	}

	if ( m_flChargeTime < UTIL_WeaponTimeBase() - 10 )
	{
		// Player charged up too long. Zap him.
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 80 + RANDOM_LONG(0,0x3f));
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM,   "weapons/electro6.wav", 1.0, ATTN_NORM, 0, 75 + RANDOM_LONG(0,0x3f));
			
		m_fInAttack = 0;
		m_iChargeLevel = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
				
		m_pPlayer->TakeDamage( VARS(eoNullEntity), VARS(eoNullEntity), 50, DMG_SHOCK );
		UTIL_ScreenFade( m_pPlayer, Vector(255,128,0), 2, 0.5, 128, FFADE_IN );

		SendWeaponAnim( GAUSS_IDLE );
		return;
	}

	m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	m_pPlayer->m_iWeaponVolume = PRIMARY_CHARGE_VOLUME;
}

//=========================================================
// StartFire- since all of this code has to run and then 
// call Fire(), it was easier at this point to rip it out 
// of weaponidle() and make its own function then to try to
// merge this into Fire(), which has some identical variable names 
//=========================================================
void CGauss::StartFire( void )
{
	float flDamage;
	
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	
	if ( pev->frags ) flDamage = gSkillData.plrDmgGauss;
	if ( m_iChargeLevel >= 1) flDamage = 12.5 * m_iChargeLevel;
	
	if (m_fInAttack >= 1)
	{
		float flZVel = m_pPlayer->pev->velocity.z;

		if ( !pev->frags ) m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * flDamage * 10;
		if ( !IsMultiplayer() ) m_pPlayer->pev->velocity.z = flZVel;
	}
	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// time until aftershock 'static discharge' sound
	m_flShockTime = UTIL_WeaponTimeBase() + RANDOM_FLOAT(0.3, 0.8);

	Fire( vecSrc, vecAiming, flDamage );
	m_fInAttack = 0;
	m_iChargeLevel = 0;
}

void CGauss::Fire( Vector vecOrigSrc, Vector vecDir, float flDamage )
{
	m_pPlayer->m_iWeaponVolume = PRIMARY_FIRE_VOLUME;

	Vector vecSrc = vecOrigSrc;
	Vector vecDest = vecSrc + vecDir * 8192;
	edict_t		*pentIgnore;
	TraceResult tr, beam_tr;
	float flMaxFrac = 1.0;
	int	nTotal = 0;
	int fHasPunched = 0;
	int fFirstBeam = 1;
	int	nMaxHits = 10;

	pentIgnore = ENT( m_pPlayer->pev );
	
	// The main firing event is sent unreliably so it won't be delayed.
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usGaussFire, 0.0, (float *)&m_pPlayer->pev->origin, (float *)&m_pPlayer->pev->angles, flDamage, 0.0, pev->body, 0, pev->frags, 0 );

	// This reliable event is used to stop the spinning sound
	// It's delayed by a fraction of second to make sure it is delayed by 1 frame on the client
	// It's sent reliably anyway, which could lead to other delays

	PLAYBACK_EVENT_FULL( NULL | FEV_RELIABLE, m_pPlayer->edict(), m_usGaussFire, 0.01, (float *)&m_pPlayer->pev->origin, (float *)&m_pPlayer->pev->angles, 0.0, 0.0, pev->body, 0, 0, 1 );
	
//	ALERT( at_console, "%f %f %f\n%f %f %f\n", vecSrc.x, vecSrc.y, vecSrc.z, vecDest.x, vecDest.y, vecDest.z );
//	ALERT( at_console, "%f %f\n", tr.flFraction, flMaxFrac );

	while (flDamage > 10 && nMaxHits > 0)
	{
		nMaxHits--;

		UTIL_TraceLine(vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr);

		if (tr.fAllSolid)
			break;

		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		if (pEntity == NULL) break;

		if ( fFirstBeam )
		{
			m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
			fFirstBeam = 0;
	
			nTotal += 26;
		}
		
		if (pEntity->pev->takedamage)
		{
			ClearMultiDamage();
			pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET );
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
		}

		if ( pEntity->ReflectGauss() )
		{
			float n;

			pentIgnore = NULL;

			n = -DotProduct(tr.vecPlaneNormal, vecDir);

			if (n < 0.5) // 60 degrees
			{
				// ALERT( at_console, "reflect %f\n", n );
				Vector r;
			
				r = 2.0 * tr.vecPlaneNormal * n + vecDir;
				flMaxFrac = flMaxFrac - tr.flFraction;
				vecDir = r;
				vecSrc = tr.vecEndPos + vecDir * 8;
				vecDest = vecSrc + vecDir * 8192;

				// explode a bit
				m_pPlayer->RadiusDamage( tr.vecEndPos, pev, m_pPlayer->pev, flDamage * n, CLASS_NONE, DMG_BLAST );

				nTotal += 34;
				
				// lose energy
				if (n == 0) n = 0.1;
				flDamage = flDamage * (1 - n);
			}
			else
			{
				nTotal += 13;

				// limit it to one hole punch
				if (fHasPunched)
					break;
				fHasPunched = 1;

				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if ( !pev->frags )
				{
					UTIL_TraceLine( tr.vecEndPos + vecDir * 8, vecDest, dont_ignore_monsters, pentIgnore, &beam_tr);
					if (!beam_tr.fAllSolid)
					{
						// trace backwards to find exit point
						UTIL_TraceLine( beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, pentIgnore, &beam_tr);

						n = (beam_tr.vecEndPos - tr.vecEndPos).Length( );

						if (n < flDamage)
						{
							if (n == 0) n = 1;
							flDamage -= n;

							// ALERT( at_console, "punch %f\n", n );
							nTotal += 21;

							// exit blast damage
							//m_pPlayer->RadiusDamage( beam_tr.vecEndPos + vecDir * 8, pev, m_pPlayer->pev, flDamage, CLASS_NONE, DMG_BLAST );
							float damage_radius;
							

							if ( IsMultiplayer() )
							{
								damage_radius = flDamage * 1.75;  // Old code == 2.5
							}
							else
							{
								damage_radius = flDamage * 2.5;
							}

							::RadiusDamage( beam_tr.vecEndPos + vecDir * 8, pev, m_pPlayer->pev, flDamage, damage_radius, CLASS_NONE, DMG_BLAST );

							CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

							nTotal += 53;

							vecSrc = beam_tr.vecEndPos + vecDir;
						}
					}
					else flDamage = 0;
				}
				else flDamage = 0;
			}
		}
		else
		{
			vecSrc = tr.vecEndPos + vecDir;
			pentIgnore = ENT( pEntity->pev );
		}
	}
}




void CGauss::WeaponIdle( void )
{
	// play aftershock static discharge
	if ( m_flShockTime && m_flShockTime < gpGlobals->time )
	{
		switch (RANDOM_LONG(0,3))
		{
			case 0:	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/electro4.wav", RANDOM_FLOAT(0.7, 0.8), ATTN_NORM); break;
			case 1:	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/electro5.wav", RANDOM_FLOAT(0.7, 0.8), ATTN_NORM); break;
			case 2:	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/electro6.wav", RANDOM_FLOAT(0.7, 0.8), ATTN_NORM); break;
			case 3:	break; // no sound
		}
		m_flShockTime = 0;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase()) return;

	if (m_fInAttack != 0)
	{
		StartFire();
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0,1);
		if ( flRand <= 0.3 )
		{
			iAnim = GAUSS_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 61 / 15;
		}
		if ( flRand >= 0.5 )
		{
			iAnim = GAUSS_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 91 / 30;
		}
		else 
		{
			iAnim = GAUSS_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 61 / 15;
		}
		SendWeaponAnim( iAnim );
	}
}






class CGaussAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_gaussammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_gaussammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_URANIUMBOX_GIVE, "uranium", URANIUM_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_gaussclip, CGaussAmmo );
