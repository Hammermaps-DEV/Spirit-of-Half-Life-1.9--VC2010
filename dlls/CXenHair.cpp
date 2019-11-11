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
#include "CActAnimating.h"

class CXenHair : public CActAnimating
{
public:
	void		Spawn(void);
	void		Precache(void);
	void		Think(void);
};

LINK_ENTITY_TO_CLASS(xen_hair, CXenHair);

#define SF_HAIR_SYNC 0x0001

void CXenHair::Spawn(void)
{
	Precache();
	SET_MODEL(edict(), "models/hair.mdl");
	UTIL_SetSize(this, Vector(-4, -4, 0), Vector(4, 4, 32));
	pev->sequence = 0;

	if (!(pev->spawnflags & SF_HAIR_SYNC))
	{
		pev->frame = RANDOM_FLOAT(0, 255);
		pev->framerate = RANDOM_FLOAT(0.7, 1.4);
	}
	ResetSequenceInfo();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetNextThink(RANDOM_FLOAT(0.1, 0.4));	// Load balance these a bit
}

void CXenHair::Think(void)
{
	StudioFrameAdvance();
	SetNextThink(0.5);
}

void CXenHair::Precache(void)
{
	PRECACHE_MODEL("models/hair.mdl");
}