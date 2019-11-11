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

class CXenSpore : public CActAnimating
{
public:
	void		Spawn(void);
	void		Precache(void);
	void		Touch(CBaseEntity* pOther);
	void		Think(void);
	int			TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) { Attack(); return 0; }
	//	void		HandleAnimEvent( MonsterEvent_t *pEvent );
	void		Attack(void) {}

	static const char* pModelNames[];
};

class CXenSporeSmall : public CXenSpore
{
	void		Spawn(void);
};

class CXenSporeMed : public CXenSpore
{
	void		Spawn(void);
};

class CXenSporeLarge : public CXenSpore
{
	void		Spawn(void);

	static const Vector m_hullSizes[];
};

// Fake collision box for big spores
class CXenHull : public CPointEntity
{
public:
	static CXenHull* CreateHull(CBaseEntity* source, const Vector& mins, const Vector& maxs, const Vector& offset);
	int			Classify(void) { return CLASS_BARNACLE; }
};

CXenHull* CXenHull::CreateHull(CBaseEntity* source, const Vector& mins, const Vector& maxs, const Vector& offset)
{
	CXenHull* pHull = GetClassPtr((CXenHull*)NULL);

	UTIL_SetOrigin(pHull, source->pev->origin + offset);
	SET_MODEL(pHull->edict(), STRING(source->pev->model));
	pHull->pev->solid = SOLID_BBOX;
	pHull->pev->classname = MAKE_STRING("xen_hull");
	pHull->pev->movetype = MOVETYPE_NONE;
	pHull->pev->owner = source->edict();
	UTIL_SetSize(pHull, mins, maxs);
	pHull->pev->renderamt = 0;
	pHull->pev->rendermode = kRenderTransTexture;
	//	pHull->pev->effects = EF_NODRAW;

	return pHull;
}

LINK_ENTITY_TO_CLASS(xen_spore_small, CXenSporeSmall);
LINK_ENTITY_TO_CLASS(xen_spore_medium, CXenSporeMed);
LINK_ENTITY_TO_CLASS(xen_spore_large, CXenSporeLarge);
LINK_ENTITY_TO_CLASS(xen_hull, CXenHull);

void CXenSporeSmall::Spawn(void)
{
	pev->skin = 0;
	CXenSpore::Spawn();
	UTIL_SetSize(this, Vector(-16, -16, 0), Vector(16, 16, 64));
}
void CXenSporeMed::Spawn(void)
{
	pev->skin = 1;
	CXenSpore::Spawn();
	UTIL_SetSize(this, Vector(-40, -40, 0), Vector(40, 40, 120));
}


// I just eyeballed these -- fill in hulls for the legs
const Vector CXenSporeLarge::m_hullSizes[] =
{
	Vector(90, -25, 0),
	Vector(25, 75, 0),
	Vector(-15, -100, 0),
	Vector(-90, -35, 0),
	Vector(-90, 60, 0),
};

void CXenSporeLarge::Spawn(void)
{
	pev->skin = 2;
	CXenSpore::Spawn();
	UTIL_SetSize(this, Vector(-48, -48, 110), Vector(48, 48, 240));

	Vector forward, right;

	UTIL_MakeVectorsPrivate(pev->angles, forward, right, NULL);

	// Rotate the leg hulls into position
	for (int i = 0; i < HL_ARRAYSIZE(m_hullSizes); i++)
		CXenHull::CreateHull(this, Vector(-12, -12, 0), Vector(12, 12, 120), (m_hullSizes[i].x * forward) + (m_hullSizes[i].y * right));
}

void CXenSpore::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), pModelNames[pev->skin]);
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;

	//	SetActivity( ACT_IDLE );
	pev->sequence = 0;
	pev->frame = RANDOM_FLOAT(0, 255);
	pev->framerate = RANDOM_FLOAT(0.7, 1.4);
	ResetSequenceInfo();
	SetNextThink(RANDOM_FLOAT(0.1, 0.4));	// Load balance these a bit
}

const char* CXenSpore::pModelNames[] =
{
	"models/fungus(small).mdl",
	"models/fungus.mdl",
	"models/fungus(large).mdl",
};


void CXenSpore::Precache(void)
{
	PRECACHE_MODEL((char*)pModelNames[pev->skin]);
}


void CXenSpore::Touch(CBaseEntity* pOther)
{
}


void CXenSpore::Think(void)
{
	float flInterval = StudioFrameAdvance();
	SetNextThink(0.1);
}