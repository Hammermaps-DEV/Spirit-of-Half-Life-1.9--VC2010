#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"movewith.h"
#include	"saverestore.h"
#include	"player.h"

CWorld *g_pWorld = NULL; //LRC

BOOL g_doingDesired = FALSE; //LRC - marks whether the Desired functions are currently
BOOL NeedUpdate( CBaseEntity *pEnt ); //synchronization may be lost (e.g. after changelevel) merge it.									// being processed.

void UTIL_AddToAssistList( CBaseEntity *pEnt )
{
//	ALERT(at_console, "Add %s \"%s\" to AssistList\n", STRING(pEnt->pev->classname), STRING(pEnt->pev->targetname));

	if (pEnt->m_pAssistLink)
	{
//		ALERT(at_console, "Ignored AddToAssistList for %s \"%s\"\n", STRING(pEnt->pev->classname), STRING(pEnt->pev->targetname));
		return; // is pEnt already in the body of the list?
	}

	if ( !g_pWorld )
	{
		ALERT(at_debug, "AddToAssistList %s \"%s\" has no AssistList!\n", STRING(pEnt->pev->classname), STRING(pEnt->pev->targetname));
		return;
	}

	CBaseEntity *pListMember = g_pWorld;

	// find the last entry in the list...
	while (pListMember->m_pAssistLink != NULL)
		pListMember = pListMember->m_pAssistLink;

	if (pListMember == pEnt)
	{
//			ALERT(at_console, "(end)Ignored AddToAssistList for %s \"%s\"\n", STRING(pEnt->pev->classname), STRING(pEnt->pev->targetname));
		return; // pEnt is the last entry in the list.
	}

	pListMember->m_pAssistLink = pEnt; // it's not in the list already, so add pEnt to the list.
	
//	int count = 0;
//	for (pListMember = g_pWorld->m_pAssistLink; pListMember; pListMember = pListMember->m_pAssistLink)
//	{
//		count++;
//	}
//	ALERT(at_console, "Added %s \"%s\" to AssistList, length now %d\n", STRING(pEnt->pev->classname), STRING(pEnt->pev->targetname), count);
}

void HandlePostAssist( CBaseEntity *pEnt )
{
	if (pEnt->m_iLFlags & LF_POSTASSISTVEL)
	{
//		ALERT(at_console, "RestoreVel %s: orign %f %f %f, velocity was %f %f %f, back to %f %f %f\n",
//			STRING(pEnt->pev->targetname),
//			pEnt->pev->origin.x, pEnt->pev->origin.y, pEnt->pev->origin.z,
//			pEnt->pev->velocity.x, pEnt->pev->velocity.y, pEnt->pev->velocity.z,
//			pEnt->m_vecPostAssistVel.x, pEnt->m_vecPostAssistVel.y, pEnt->m_vecPostAssistVel.z
//		);
		pEnt->pev->velocity = pEnt->m_vecPostAssistVel;
		pEnt->m_vecPostAssistVel = g_vecZero;
		pEnt->m_iLFlags &= ~LF_POSTASSISTVEL;
	}
	if (pEnt->m_iLFlags & LF_POSTASSISTAVEL)
	{
//		ALERT(at_console, "RestoreVel %s: orign %f %f %f, velocity was %f %f %f, back to %f %f %f\n",
//			STRING(pEnt->pev->targetname),
//			pEnt->pev->origin.x, pEnt->pev->origin.y, pEnt->pev->origin.z,
//			pEnt->pev->velocity.x, pEnt->pev->velocity.y, pEnt->pev->velocity.z,
//			pEnt->m_vecPostAssistVel.x, pEnt->m_vecPostAssistVel.y, pEnt->m_vecPostAssistVel.z
//		);
		pEnt->pev->avelocity = pEnt->m_vecPostAssistAVel;
		pEnt->m_vecPostAssistAVel = g_vecZero;
		pEnt->m_iLFlags &= ~LF_POSTASSISTAVEL;
	}
	CBaseEntity *pChild;
	for (pChild = pEnt->m_pChildMoveWith; pChild != NULL; pChild = pChild->m_pSiblingMoveWith)
		HandlePostAssist( pChild );
}

// returns 0 if no desired settings needed, else returns 1
int ApplyDesiredSettings( CBaseEntity *pListMember )
{
	if (pListMember->m_iLFlags & LF_DODESIRED)
	{
		//ALERT(at_console, "Apply for %s\n", STRING(pListMember->pev->classname));
		pListMember->m_iLFlags &= ~LF_DODESIRED;
	}
	else
	{
		// don't need to apply any desired settings for this entity.
		//ALERT(at_console, "Not apply for %s\n", STRING(pListMember->pev->classname));
		return 0;
	}
//	ALERT(at_console, "ApplyDesiredSettings for %s \"%s\", pevnt %f, ltime %f, mfnt %f, mpevnt %f, %f\n", STRING(pListMember->pev->classname), STRING(pListMember->pev->targetname), pListMember->pev->nextthink, pListMember->pev->ltime, pListMember->m_fNextThink, pListMember->m_fPevNextThink, pListMember->pev->origin.x);

	if (pListMember->m_iLFlags & LF_DESIRED_ACTION)
	{
		pListMember->m_iLFlags &= ~LF_DESIRED_ACTION;
		pListMember->DesiredAction();
		if(NeedUpdate(pListMember)) SetBits(pListMember->m_pChildMoveWith->m_iLFlags, LF_MERGEPOS);
	}
	if (pListMember->m_iLFlags & (LF_POSTORG|LF_MERGEPOS))
	{
		UTIL_MergePos( pListMember );
	}
	if (pListMember->m_iLFlags & LF_DESIRED_INFO)
	{
		pListMember->m_iLFlags &= ~LF_DESIRED_INFO;
		ALERT(at_debug, "DesiredInfo: pos %f %f %f, vel %f %f %f. Child pos %f %f %f, vel %f %f %f\n\n", pListMember->pev->origin.x, pListMember->pev->origin.y, pListMember->pev->origin.z, pListMember->pev->velocity.x, pListMember->pev->velocity.y, pListMember->pev->velocity.z, pListMember->m_pChildMoveWith->pev->origin.x, pListMember->m_pChildMoveWith->pev->origin.y, pListMember->m_pChildMoveWith->pev->origin.z, pListMember->m_pChildMoveWith->pev->velocity.x, pListMember->m_pChildMoveWith->pev->velocity.y, pListMember->m_pChildMoveWith->pev->velocity.z);
	}
	if (pListMember->m_iLFlags & LF_DESIRED_POSTASSIST)
	{
		pListMember->m_iLFlags &= ~LF_DESIRED_POSTASSIST;
		HandlePostAssist( pListMember );
	}
	if (pListMember->m_iLFlags & LF_DESIRED_THINK)
	{
		pListMember->m_iLFlags &= ~LF_DESIRED_THINK;
		//ALERT(at_console, "DesiredThink %s\n", STRING(pListMember->pev->targetname));
		pListMember->Think();
	}

	return 1;
}

void AssistChildren( CBaseEntity *pEnt, Vector vecAdjustVel, Vector vecAdjustAVel )
{
	CBaseEntity *pChild;
	for(pChild = pEnt->m_pChildMoveWith; pChild != NULL; pChild = pChild->m_pSiblingMoveWith)
	{
		if (!(pChild->m_iLFlags & LF_POSTASSISTVEL))
		{
			pChild->m_vecPostAssistVel = pChild->pev->velocity;
			pChild->m_iLFlags |= LF_POSTASSISTVEL;
		}
		if (!(pChild->m_iLFlags & LF_POSTASSISTAVEL))
		{
			pChild->m_vecPostAssistAVel = pChild->pev->avelocity;
			pChild->m_iLFlags |= LF_POSTASSISTAVEL;
		}
		pChild->pev->velocity = pChild->pev->velocity - vecAdjustVel;// (pChild->pev->velocity - pEnt->m_vecPostAssistVel) + pEnt->m_vecPostAssistVel*fFraction;
		pChild->pev->avelocity = pChild->pev->avelocity - vecAdjustAVel;// (pChild->pev->avelocity - pEnt->m_vecPostAssistAVel) + pEnt->m_vecPostAssistAVel*fFraction;

		//ALERT(at_console, "AssistChild %s: origin %f %f %f, old vel %f %f %f. fraction %f, new vel %f %f %f, dest %f %f %f\n", STRING(pChild->pev->targetname), pChild->pev->origin.x, pChild->pev->origin.y, pChild->pev->origin.z, pChild->m_vecPostAssistVel.x, pChild->m_vecPostAssistVel.y, pChild->m_vecPostAssistVel.z, fFraction, pChild->pev->velocity.x, pChild->pev->velocity.y, pChild->pev->velocity.z, pChild->pev->origin.x + pChild->pev->velocity.x*gpGlobals->frametime, pChild->pev->origin.y + pChild->pev->velocity.y*gpGlobals->frametime, pChild->pev->origin.z + pChild->pev->velocity.z*gpGlobals->frametime );
		//ALERT(at_console, "AssistChild %s: origin %f %f %f. velocity was %f %f %f, now %f %f %f\n", STRING(pChild->pev->targetname), pChild->pev->origin.x, pChild->pev->origin.y, pChild->pev->origin.z, pChild->m_vecPostAssistVel.x, pChild->m_vecPostAssistVel.y, pChild->m_vecPostAssistVel.z, pChild->pev->velocity.x, pChild->pev->velocity.y, pChild->pev->velocity.z);

		AssistChildren( pChild, vecAdjustVel, vecAdjustAVel );
	}
}

// pEnt is a PUSH entity. Before physics is applied in a frame, we need to assist:
// 1) this entity, if it will stop moving this turn but its parent will continue.
// 2) this entity's PUSH children, if 1) was applied.
// 3) this entity's non-PUSH children, if this entity will stop moving this turn.
//
// returns 0 if the entity wasn't flagged for DoAssist.
int TryAssistEntity( CBaseEntity *pEnt )
{
	if (gpGlobals->frametime == 0)
	{
//		ALERT(at_console, "frametime 0, don't assist\n");
		return 0;
	}

	// not flagged as needing assistance?
	//if (!(pEnt->m_iLFlags & LF_DOASSIST)) return 0;

//	ALERT(at_console, "AssistList: %s\n", STRING(pEnt->pev->classname));

	if (pEnt->m_fNextThink <= 0)
	{
//		ALERT(at_console, "Cancelling assist for %s, %f\n", STRING(pEnt->pev->targetname), pEnt->pev->origin.x);
		pEnt->m_iLFlags &= ~LF_DOASSIST;
		return 0; // the think has been cancelled. Oh well...
	}

//	ALERT(at_console, "Assist, mfNT %f, pevNT %f\n", pEnt->m_fNextThink, pEnt->pev->nextthink);
//	pEnt->ThinkCorrection();

	// a fraction of the current velocity. (the part of it that the engine should see.)
	float fFraction = 0;

	// is this the frame when the entity will stop?
	if (pEnt->pev->movetype == MOVETYPE_PUSH)
	{
		if (pEnt->m_fNextThink <= pEnt->pev->ltime + gpGlobals->frametime)
			fFraction = (pEnt->m_fNextThink - pEnt->pev->ltime)/gpGlobals->frametime;
	}
	else if (pEnt->m_fNextThink <= gpGlobals->time + gpGlobals->frametime)
	{
		fFraction = (pEnt->m_fNextThink - gpGlobals->time)/gpGlobals->frametime;
//		ALERT(at_console, "Setting fFraction\n");
	}

	if (fFraction)
	{
//		ALERT(at_console, "Assisting %s \"%s\", %f <= %f + %f\n", STRING(pEnt->pev->classname), STRING(pEnt->pev->targetname), pEnt->m_fNextThink, pEnt->pev->ltime, gpGlobals->frametime);

		if (pEnt->m_iLFlags & LF_CORRECTSPEED)
		{
			if (!(pEnt->m_iLFlags & LF_POSTASSISTVEL))
			{
				pEnt->m_vecPostAssistVel = pEnt->pev->velocity;
				pEnt->m_iLFlags |= LF_POSTASSISTVEL;
			}

			if (!(pEnt->m_iLFlags & LF_POSTASSISTAVEL))
			{
				pEnt->m_vecPostAssistAVel = pEnt->pev->avelocity;
				pEnt->m_iLFlags |= LF_POSTASSISTAVEL;
			}

			Vector vecVelTemp = pEnt->pev->velocity;
			Vector vecAVelTemp = pEnt->pev->avelocity;

			if (pEnt->m_pMoveWith)
			{
				pEnt->pev->velocity = (pEnt->pev->velocity - pEnt->m_pMoveWith->pev->velocity)*fFraction + pEnt->m_pMoveWith->pev->velocity;
				pEnt->pev->avelocity = (pEnt->pev->avelocity - pEnt->m_pMoveWith->pev->avelocity)*fFraction + pEnt->m_pMoveWith->pev->avelocity;
			}
			else
			{
				pEnt->pev->velocity = pEnt->pev->velocity*fFraction;
				pEnt->pev->avelocity = pEnt->pev->avelocity*fFraction;
			}

			//ALERT(at_console, "Assist %s: origin %f %f %f, old vel %f %f %f. fraction %f, new vel %f %f %f, dest %f %f %f\n", STRING(pEnt->pev->targetname), pEnt->pev->origin.x, pEnt->pev->origin.y, pEnt->pev->origin.z, pEnt->m_vecPostAssistVel.x, pEnt->m_vecPostAssistVel.y, pEnt->m_vecPostAssistVel.z, fFraction, pEnt->pev->velocity.x, pEnt->pev->velocity.y, pEnt->pev->velocity.z, pEnt->pev->origin.x + pEnt->pev->velocity.x*gpGlobals->frametime, pEnt->pev->origin.y + pEnt->pev->velocity.y*gpGlobals->frametime, pEnt->pev->origin.z + pEnt->pev->velocity.z*gpGlobals->frametime);

			AssistChildren( pEnt, vecVelTemp - pEnt->pev->velocity, vecAVelTemp - pEnt->pev->avelocity );
			UTIL_DesiredPostAssist( pEnt );
		}

		UTIL_DesiredThink( pEnt );
//		ALERT(at_console, "Assist sets DesiredThink for %s\n", STRING(pEnt->pev->classname));

		pEnt->m_iLFlags &= ~LF_DOASSIST;
	}
	return 1;
}

// called every frame, by StartFrame
void CheckAssistList( void )
{
	CBaseEntity *pListMember;

	if ( !g_pWorld )
	{
		ALERT(at_error, "CheckAssistList has no AssistList!\n");
		return;
	}

	int count = 0;

	for (pListMember = g_pWorld; pListMember; pListMember = pListMember->m_pAssistLink)
	{
		if (pListMember->m_iLFlags & LF_DOASSIST)
			count++;
	}

	count = 0;
	pListMember = g_pWorld;

	while ( pListMember->m_pAssistLink ) // handle the remaining entries in the list
	{
		TryAssistEntity(pListMember->m_pAssistLink);
		if (!(pListMember->m_pAssistLink->m_iLFlags & LF_ASSISTLIST))
		{
			CBaseEntity *pTemp = pListMember->m_pAssistLink;
			pListMember->m_pAssistLink = pListMember->m_pAssistLink->m_pAssistLink;
			pTemp->m_pAssistLink = NULL;
		}
		else pListMember = pListMember->m_pAssistLink;
	}
}

// called every frame, by PostThink
void CheckDesiredList( void )
{
	CBaseEntity *pListMember;
	int loopbreaker = 1024; //max edicts

	if (g_doingDesired) ALERT(at_debug, "CheckDesiredList: doingDesired is already set!?\n");
	g_doingDesired = TRUE;

	if (!g_pWorld)
	{
		ALERT(at_console, "CheckDesiredList has no AssistList!\n");
		return;
	}

	pListMember = g_pWorld;
	CBaseEntity *pNext;
	pListMember = g_pWorld->m_pAssistLink;

	while (pListMember)
	{
		// cache this, in case ApplyDesiredSettings does a SUB_Remove.
		pNext = pListMember->m_pAssistLink;
		ApplyDesiredSettings( pListMember );
		pListMember = pNext;
		loopbreaker--;
		if (loopbreaker <= 0)
		{
			ALERT(at_error, "Infinite(?) loop in DesiredList!");
			break;
		}
	}

	g_doingDesired = FALSE;
}



void UTIL_MarkForAssist( CBaseEntity *pEnt, BOOL correctSpeed )
{
	pEnt->m_iLFlags |= LF_DOASSIST;

	if (correctSpeed)
		pEnt->m_iLFlags |= LF_CORRECTSPEED;
	else
		pEnt->m_iLFlags &= ~LF_CORRECTSPEED;

	UTIL_AddToAssistList( pEnt );
}

void UTIL_MarkForDesired ( CBaseEntity *pEnt )
{
	pEnt->m_iLFlags |= LF_DODESIRED;

	if (g_doingDesired)
	{
		//ALERT(at_console, "doingDesired is true, start immediately\n");
		ApplyDesiredSettings( pEnt );
		return;
	}

	UTIL_AddToAssistList( pEnt );
}

void UTIL_DesiredAction ( CBaseEntity *pEnt )
{
	pEnt->m_iLFlags |= LF_DESIRED_ACTION;
	UTIL_MarkForDesired( pEnt );
}

void UTIL_DesiredInfo ( CBaseEntity *pEnt )
{
	pEnt->m_iLFlags |= LF_DESIRED_INFO;
	UTIL_MarkForDesired( pEnt );
}

void UTIL_DesiredPostAssist ( CBaseEntity *pEnt )
{
	pEnt->m_iLFlags |= LF_DESIRED_POSTASSIST;
	UTIL_MarkForDesired( pEnt );
}

void UTIL_DesiredThink ( CBaseEntity *pEnt )
{
//	ALERT(at_console, "Setting DesiredThink %s\n", STRING(pEnt->pev->targetname));
	pEnt->m_iLFlags |= LF_DESIRED_THINK;
	pEnt->DontThink();
	UTIL_MarkForDesired( pEnt );
}



// LRC- change the origin to the given position, and bring any movewiths along too.
void UTIL_AssignOrigin( CBaseEntity *pEntity, const Vector vecOrigin )
{
	UTIL_AssignOrigin( pEntity, vecOrigin, TRUE);
}

// LRC- bInitiator is true if this is being called directly, rather than because pEntity is moving with something else.
void UTIL_AssignOrigin( CBaseEntity *pEntity, const Vector vecOrigin, BOOL bInitiator)
{
	Vector vecDiff = vecOrigin - pEntity->pev->origin;

	UTIL_SetOrigin(pEntity, vecOrigin );

	if (bInitiator && pEntity->m_pMoveWith)
	{
		pEntity->m_vecOffsetOrigin = pEntity->pev->origin - pEntity->m_pMoveWith->pev->origin;
	}
	if (pEntity->m_pChildMoveWith) // now I've moved pEntity, does anything else have to move with it?
	{
		CBaseEntity* pChild = pEntity->m_pChildMoveWith;

		Vector vecTemp;
		while (pChild)
		{
			if (pChild->pev->movetype != MOVETYPE_PUSH || pChild->pev->velocity == pEntity->pev->velocity) // if the child isn't moving under its own power
			{
				UTIL_AssignOrigin( pChild, vecOrigin + pChild->m_vecOffsetOrigin, FALSE );
			}
			else
			{
				vecTemp = vecDiff + pChild->pev->origin;
				UTIL_AssignOrigin( pChild, vecTemp, FALSE );
			}
			pChild = pChild->m_pSiblingMoveWith;
		}
	}
}

void UTIL_AssignAngles( CBaseEntity *pEntity, const Vector vecAngles )
{
	UTIL_AssignAngles( pEntity, vecAngles, TRUE );
}

void UTIL_AssignAngles( CBaseEntity *pEntity, const Vector vecAngles, BOOL bInitiator)
{
	Vector vecDiff = vecAngles - pEntity->pev->angles;
	if (vecDiff.Length() > 0.01 && CVAR_GET_FLOAT("sohl_mwdebug"))
		ALERT(at_debug,"SetAngles %s %s: (%f %f %f) goes to (%f %f %f)\n",STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname), pEntity->pev->angles.x, pEntity->pev->angles.y, pEntity->pev->angles.z, vecAngles.x, vecAngles.y, vecAngles.z);

//	UTIL_SetDesiredAngles(pEntity, vecAngles);
	UTIL_SetAngles(pEntity, vecAngles);

	if (bInitiator && pEntity->m_pMoveWith)
	{
		pEntity->m_vecOffsetAngles = vecAngles - pEntity->m_pMoveWith->pev->angles;
	}
	if (pEntity->m_pChildMoveWith) // now I've moved pEntity, does anything else have to move with it?
	{
		CBaseEntity* pChild = pEntity->m_pChildMoveWith;
		Vector vecTemp;
		while (pChild)
		{
			if (pChild->pev->avelocity == pEntity->pev->avelocity) // if the child isn't turning under its own power
			{
				UTIL_AssignAngles( pChild, vecAngles + pChild->m_vecOffsetAngles, FALSE );
			}
			else
			{
				vecTemp = vecDiff + pChild->pev->angles;
				UTIL_AssignAngles( pChild, vecTemp, FALSE );
			}
			//ALERT(at_console,"  child origin becomes (%f %f %f)\n",pChild->pev->origin.x,pChild->pev->origin.y,pChild->pev->origin.z);
			//ALERT(at_console,"ent %p has sibling %p\n",pChild,pChild->m_pSiblingMoveWith);
			pChild = pChild->m_pSiblingMoveWith;
		}
	}
}

//LRC- an arbitrary limit. If this number is exceeded we assume there's an infinite loop, and abort.
#define MAX_MOVEWITH_DEPTH 100

//LRC- for use in supporting movewith. Tell the entity that whatever it's moving with is about to change velocity.
// loopbreaker is there to prevent the game from hanging...
void UTIL_SetMoveWithVelocity ( CBaseEntity *pEnt, const Vector vecSet, int loopbreaker )
{
	if (loopbreaker <= 0)
	{
		ALERT(at_error, "Infinite child list for MoveWith!");
		return;
	}

	if (!pEnt->m_pMoveWith)
	{
		ALERT(at_error,"SetMoveWithVelocity: no MoveWith entity!?\n");
		return;
	}

	Vector vecNew;
//	if (pEnt->pev->movetype == MOVETYPE_PUSH)
//	{
//		float fFraction = 
		vecNew = (pEnt->pev->velocity - pEnt->m_pMoveWith->pev->velocity) + vecSet;
//	}
//	else
//		vecNew = vecSet; // lazy assumption: non-push entities don't move on their own.

//	ALERT(at_console,"SetMoveWithVelocity: %s changes to (%f %f %f) (based on %f %f %f)\n",
//		STRING(pEnt->pev->classname), vecNew.x, vecNew.y, vecNew.z,
//		pEnt->m_pMoveWith->pev->velocity.x, pEnt->m_pMoveWith->pev->velocity.y, pEnt->m_pMoveWith->pev->velocity.z
//	);

//	ALERT(at_console,"SMWV: %s is sent (%f,%f,%f) - goes from (%f,%f,%f) to (%f,%f,%f)\n",
//	    STRING(pEnt->pev->targetname), vecSet.x, vecSet.y, vecSet.z,
//		pEnt->pev->velocity.x, pEnt->pev->velocity.y, pEnt->pev->velocity.z,
//		vecNew.x, vecNew.y, vecNew.z
//	);

	//LRC- velocity is ignored on entities that aren't thinking! (Aargh...)
//	if (fThinkTime)
//	{
//		UTIL_DesiredNextThink( pEnt, fThinkTime );
//	}
//	else if (pEnt->pev->nextthink < 1)
//	{
//		UTIL_DesiredNextThink( pEnt, 1E6 );
//		//pEnt->pev->nextthink = gpGlobals->time + 1E6;
//	}

	if ( pEnt->m_pChildMoveWith )
	{
		CBaseEntity *pMoving = pEnt->m_pChildMoveWith;
		int sloopbreaker = MAX_MOVEWITH_DEPTH; // to prevent the game hanging...
		while (pMoving)
		{
			UTIL_SetMoveWithVelocity(pMoving, vecNew, loopbreaker - 1 );
			pMoving = pMoving->m_pSiblingMoveWith;
			sloopbreaker--;
			if (sloopbreaker <= 0)
			{
				ALERT(at_error, "SetMoveWithVelocity: Infinite sibling list for MoveWith!");
				break;
			}
		}
	}

//	if (bImmediate)
		pEnt->pev->velocity = vecNew;
//	else
//		UTIL_SetDesiredVel(pEnt, vecNew);
}

//LRC
void UTIL_SetVelocity ( CBaseEntity *pEnt, const Vector vecSet )
{
	Vector vecNew;
	if (pEnt->m_pMoveWith)
		vecNew = vecSet + pEnt->m_pMoveWith->pev->velocity;
	else
		vecNew = vecSet;

//	ALERT(at_console,"SetV: %s is sent (%f,%f,%f) - goes from (%f,%f,%f) to (%f,%f,%f)\n",
//	    STRING(pEnt->pev->targetname), vecSet.x, vecSet.y, vecSet.z,
//		pEnt->pev->velocity.x, pEnt->pev->velocity.y, pEnt->pev->velocity.z,
//		vecNew.x, vecNew.y, vecNew.z
//	);

	if ( pEnt->m_pChildMoveWith )
	{
		CBaseEntity *pMoving = pEnt->m_pChildMoveWith;
		int sloopbreaker = MAX_MOVEWITH_DEPTH; // LRC - to save us from infinite loops
		while (pMoving)
		{
			UTIL_SetMoveWithVelocity(pMoving, vecNew, MAX_MOVEWITH_DEPTH );
			if(vecSet != g_vecZero)SetBits(pMoving->m_iLFlags, LF_PARENTMOVE);
                              else ClearBits(pMoving->m_iLFlags, LF_PARENTMOVE);
			pMoving = pMoving->m_pSiblingMoveWith;
			sloopbreaker--;
			if (sloopbreaker <= 0)
			{
				ALERT(at_error, "SetVelocity: Infinite sibling list for MoveWith!\n");
				break;
			}
		}
	}

	pEnt->pev->velocity = vecNew;
}

//LRC - one more MoveWith utility. This one's for the simple version of RotWith.
void UTIL_SetMoveWithAvelocity ( CBaseEntity *pEnt, const Vector vecSet, int loopbreaker )
{
	if (loopbreaker <= 0)
	{
		ALERT(at_error, "Infinite child list for MoveWith!");
		return;
	}

	if (!pEnt->m_pMoveWith)
	{
		ALERT(at_error,"SetMoveWithAvelocity: no MoveWith entity!?\n");
		return;
	}

	Vector vecNew = (pEnt->pev->avelocity - pEnt->m_pMoveWith->pev->avelocity) + vecSet;

//	ALERT(at_console, "Setting Child AVelocity %f %f %f, was %f %f %f mw %f %f %f\n", vecNew.x, vecNew.y, vecNew.z, pEnt->pev->avelocity.x, pEnt->pev->avelocity.y, pEnt->pev->avelocity.z, pEnt->m_pMoveWith->pev->avelocity.x, pEnt->m_pMoveWith->pev->avelocity.y, pEnt->m_pMoveWith->pev->avelocity.z);

	if ( pEnt->m_pChildMoveWith )
	{
		CBaseEntity *pMoving = pEnt->m_pChildMoveWith;
		int sloopbreaker = MAX_MOVEWITH_DEPTH; // to prevent the game hanging...
		while (pMoving)
		{
			UTIL_SetMoveWithAvelocity(pMoving, vecNew, loopbreaker - 1 );
			pMoving = pMoving->m_pSiblingMoveWith;
			sloopbreaker--;
			if (sloopbreaker <= 0)
			{
				ALERT(at_error, "SetMoveWithVelocity: Infinite sibling list for MoveWith!");
				break;
			}
		}
	}

	//UTIL_SetDesiredAvelocity(pEnt, vecNew);
	pEnt->pev->avelocity = vecNew;
}

void UTIL_SetAvelocity ( CBaseEntity *pEnt, const Vector vecSet )
{
	Vector vecNew;
	if (pEnt->m_pMoveWith)
		vecNew = vecSet + pEnt->m_pMoveWith->pev->avelocity;
	else
		vecNew = vecSet;

//	ALERT(at_console, "Setting AVelocity %f %f %f\n", vecNew.x, vecNew.y, vecNew.z);

	if ( pEnt->m_pChildMoveWith )
	{
		CBaseEntity *pMoving = pEnt->m_pChildMoveWith;
		int sloopbreaker = MAX_MOVEWITH_DEPTH; // LRC - to save us from infinite loops
		while (pMoving)
		{
			UTIL_SetMoveWithAvelocity(pMoving, vecNew, MAX_MOVEWITH_DEPTH );
			UTIL_MergePos( pMoving ); //force to update
			if(vecSet != g_vecZero)SetBits(pMoving->m_iLFlags, LF_PARENTMOVE);
                              else ClearBits(pMoving->m_iLFlags, LF_PARENTMOVE);
			pMoving = pMoving->m_pSiblingMoveWith;
			sloopbreaker--;
			if (sloopbreaker <= 0)
			{
				ALERT(at_error, "SetAvelocity: Infinite sibling list for MoveWith!\n");
				break;
			}
		}
	}
	pEnt->pev->avelocity = vecNew;
}

void UTIL_MergePos ( CBaseEntity *pEnt, int loopbreaker )
{
	if (loopbreaker <= 0)return;
	if (!pEnt->m_pMoveWith)return;
          
	Vector forward, right, up, vecOrg, vecAngles;
          
	UTIL_MakeVectorsPrivate( pEnt->m_pMoveWith->pev->angles, forward, right, up );

	if(pEnt->m_pMoveWith->pev->flags & FL_MONSTER)
		vecOrg = pEnt->m_vecPostAssistOrg = pEnt->m_pMoveWith->pev->origin + (forward * pEnt->m_vecOffsetOrigin.x) + ( right * pEnt->m_vecOffsetOrigin.y) + (up * pEnt->m_vecOffsetOrigin.z);
	else	vecOrg = pEnt->m_vecPostAssistOrg = pEnt->m_pMoveWith->pev->origin + (forward * pEnt->m_vecOffsetOrigin.x) + (-right * pEnt->m_vecOffsetOrigin.y) + (up * pEnt->m_vecOffsetOrigin.z);

	vecAngles = pEnt->m_vecPostAssistAng = pEnt->m_pMoveWith->pev->angles + pEnt->m_vecOffsetAngles;
          
	if ( pEnt->m_pChildMoveWith )
	{
		CBaseEntity *pMoving = pEnt->m_pChildMoveWith;
		int sloopbreaker = MAX_MOVEWITH_DEPTH;
		while (pMoving)
		{
			UTIL_MergePos(pMoving, loopbreaker - 1 );
			pMoving = pMoving->m_pSiblingMoveWith;
			sloopbreaker--;
			if (sloopbreaker <= 0)break;
		}
	}
          
	if(pEnt->m_iLFlags & LF_MERGEPOS)
	{
		UTIL_AssignOrigin( pEnt, vecOrg );
		UTIL_AssignAngles( pEnt, vecAngles );
		ClearBits(pEnt->m_iLFlags, LF_MERGEPOS);
	}
	if(pEnt->m_iLFlags & LF_POSTORG)
	{
		pEnt->pev->origin = vecOrg;
		pEnt->pev->angles = vecAngles;
          	SetBits(pEnt->m_iLFlags, LF_DODESIRED ); //refresh position every frame
          	if(!pEnt->m_pAssistLink) UTIL_AddToAssistList(pEnt);
          }
}

BOOL NeedUpdate( CBaseEntity *pEnt )
{
	if( pEnt->m_pChildMoveWith && pEnt->m_pChildMoveWith->m_vecOffsetOrigin == g_vecZero)//potentially loser
	{
		if(pEnt->pev->origin != pEnt->m_pChildMoveWith->pev->origin)
		{
			ALERT( at_console, "Warning %s lose synch with child\n", STRING(pEnt->pev->classname));
			return TRUE;
		}
	}
	return FALSE;
}