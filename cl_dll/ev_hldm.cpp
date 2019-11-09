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
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "pm_shared.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"
#include "com_weapons.h"

extern engine_studio_api_t IEngineStudio;

static int tracerCount[ 32 ];

extern char PM_FindTextureType( char *name );

void V_PunchAxis( int axis, float punch );
void VectorAngles( const float *forward, float *angles );
vec3_t previousorigin; //egon use this

extern cvar_t *cl_lw;

extern "C"
{
	// HLDM
	void EV_FireNull(event_args_t *args);
	void EV_FireCrowbar(struct event_args_s *args);
	void EV_FireGlock1( struct event_args_s *args  );
	void EV_FireMP5( struct event_args_s *args  );
	void EV_FirePython( struct event_args_s *args  );
	void EV_FireGauss( struct event_args_s *args  );
	void EV_SpinGauss( struct event_args_s *args  );
	void EV_EgonFire( struct event_args_s *args  );
	void EV_EgonStop( struct event_args_s *args  );
	void EV_FireShotGunSingle( struct event_args_s *args  );
	void EV_FireShotGunDouble( struct event_args_s *args  );
	void EV_SnarkFire( struct event_args_s *args  );
	void EV_TrainPitchAdjust( struct event_args_s *args );
	void EV_PlayEmptySound( struct event_args_s *args );
	void EV_Decals( struct event_args_s *args );
	void EV_Explode( struct event_args_s *args );
}

#define VECTOR_CONE_1DEGREES Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES Vector( 0.06105, 0.06105, 0.06105 )	
#define VECTOR_CONE_8DEGREES Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES Vector( 0.17365, 0.17365, 0.17365 )

//hulls defines
#define VEC_HULL_MIN	Vector(-16, -16, -36)
#define VEC_HULL_MAX	Vector( 16,  16,  36)
#define VEC_HUMAN_HULL_MIN	Vector( -16, -16, 0 )
#define VEC_HUMAN_HULL_MAX	Vector( 16, 16, 72 )
#define VEC_HUMAN_HULL_DUCK	Vector( 16, 16, 36 )

#define VEC_DUCK_HULL_MIN	Vector(-16, -16, -18 )
#define VEC_DUCK_HULL_MAX	Vector( 16,  16,  18)

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
float EV_HLDM_PlayTextureSound( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, int iBulletType )
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	float fvol;
	float fvolbar;
	char *rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	char *pTextureName;
	char texname[ 64 ];
	char szbuffer[ 64 ];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	// FIXME check if playtexture sounds movevar is set
	chTextureType = 0;

	// Player
	if ( entity >= 1 && entity <= gEngfuncs.GetMaxClients() )
	{
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	}
	else if ( entity == 0 )
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );
		
		if ( pTextureName )
		{
			strcpy( texname, pTextureName );
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}
			
			strcpy( szbuffer, pTextureName );
			szbuffer[ CBTEXTURENAMEMAX - 1 ] = 0;
				
			// get texture type
			chTextureType = PM_FindTextureType( szbuffer );	
		}
	}
	
	switch (chTextureType)
	{
		default:
		case CHAR_TEX_CONCRETE: fvol = 0.9;	fvolbar = 0.6;
			rgsz[0] = "player/pl_step1.wav";
			rgsz[1] = "player/pl_step2.wav";
			cnt = 2;
		break;
		case CHAR_TEX_METAL: fvol = 0.9; fvolbar = 0.3;
			rgsz[0] = "player/pl_metal1.wav";
			rgsz[1] = "player/pl_metal2.wav";
			cnt = 2;
		break;
		case CHAR_TEX_DIRT:	fvol = 0.9; fvolbar = 0.1;
			rgsz[0] = "player/pl_dirt1.wav";
			rgsz[1] = "player/pl_dirt2.wav";
			rgsz[2] = "player/pl_dirt3.wav";
			cnt = 3;
		break;
		case CHAR_TEX_VENT:	fvol = 0.5; fvolbar = 0.3;
			rgsz[0] = "player/pl_duct1.wav";
			rgsz[1] = "player/pl_duct1.wav";
			cnt = 2;
		break;
		case CHAR_TEX_GRATE: fvol = 0.9; fvolbar = 0.5;
			rgsz[0] = "player/pl_grate1.wav";
			rgsz[1] = "player/pl_grate4.wav";
			cnt = 2;
		break;
		case CHAR_TEX_TILE:	fvol = 0.8; fvolbar = 0.2;
			rgsz[0] = "player/pl_tile1.wav";
			rgsz[1] = "player/pl_tile3.wav";
			rgsz[2] = "player/pl_tile2.wav";
			rgsz[3] = "player/pl_tile4.wav";
			cnt = 4;
		break;
		case CHAR_TEX_SLOSH: fvol = 0.9; fvolbar = 0.0;
			rgsz[0] = "player/pl_slosh1.wav";
			rgsz[1] = "player/pl_slosh3.wav";
			rgsz[2] = "player/pl_slosh2.wav";
			rgsz[3] = "player/pl_slosh4.wav";
			cnt = 4;
		break;
		// SOHL 1.9.1
		case CHAR_TEX_SNOW: fvol = 0.9; fvolbar = 0.2;
			rgsz[0] = "player/pl_snow1.wav";
			rgsz[1] = "player/pl_snow3.wav";
			rgsz[2] = "player/pl_snow2.wav";
			rgsz[3] = "player/pl_snow4.wav";
			cnt = 4;
		break;
		case CHAR_TEX_WOOD: fvol = 0.9; fvolbar = 0.2;
			rgsz[0] = "debris/wood1.wav";
			rgsz[1] = "debris/wood2.wav";
			rgsz[2] = "debris/wood3.wav";
			cnt = 3;
		break;
		case CHAR_TEX_GLASS:
		case CHAR_TEX_COMPUTER:
			fvol = 0.8; fvolbar = 0.2;
			rgsz[0] = "debris/glass1.wav";
			rgsz[1] = "debris/glass2.wav";
			rgsz[2] = "debris/glass3.wav";
			cnt = 3;
		break;
		case CHAR_TEX_FLESH:
			if (iBulletType == BULLET_PLAYER_CROWBAR) return 0.0; // crowbar already makes this sound
			fvol = 1.0;	fvolbar = 0.2;
			rgsz[0] = "weapons/bullet_hit1.wav";
			rgsz[1] = "weapons/bullet_hit2.wav";
			fattn = 1.0;
			cnt = 2;
		break;
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong(0,cnt-1)], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong(0,0xf) );
	return fvolbar;
}

//======================
//	   MIRROR UTILS
//======================
vec3_t EV_GetMirrorOrigin(int mirror_index, vec3_t pos)
{
	vec3_t result;
	VectorCopy(pos, result);

	switch (gHUD.Mirrors[mirror_index].type)
	{
		case 0:
			result[0] = gHUD.Mirrors[mirror_index].origin[0]*2 - pos[0];
		break;
		case 1:
			result[1] = gHUD.Mirrors[mirror_index].origin[1]*2 - pos[1];
		break;
		default:
		case 2:
			result[2] = gHUD.Mirrors[mirror_index].origin[2]*2 - pos[2];
		break;
	}

	return result;
}

vec3_t EV_GetMirrorAngles (int mirror_index, vec3_t angles )
{
	vec3_t result;
	VectorCopy(angles, result);

	switch (gHUD.Mirrors[mirror_index].type)
	{
		case 0:
			result[0] = -result[0]; 
		break;
		case 1:
			result[1] = -result[1]; 
		break;
		default:
		case 2:
			result[2] = -result[2]; 
		break;
	}

	return result;
}

vec3_t EV_MirrorVector( vec3_t angles )
{
	vec3_t result;
	VectorCopy(angles, result);

	if (gHUD.numMirrors)
	{
		for (int imc=0; imc < gHUD.numMirrors; imc++)
		{
			if (!gHUD.Mirrors[imc].enabled) continue;
			VectorCopy(EV_GetMirrorAngles(imc, angles), result);
		}
	}

	return result;
}

vec3_t EV_MirrorPos( vec3_t endpos )
{
	vec3_t mirpos(0, 0, 0);
	if (gHUD.numMirrors)
	{
		for (int imc=0; imc < gHUD.numMirrors; imc++)
		{
			vec3_t delta; float dist;
			if (!gHUD.Mirrors[imc].enabled) continue;
			VectorSubtract(gHUD.Mirrors[imc].origin, endpos, delta);
			dist = Length(delta);

			if (gHUD.Mirrors[imc].radius < dist) continue;
			VectorCopy(EV_GetMirrorOrigin(imc, endpos), mirpos);
		}
	}

	return mirpos;
}

//======================
// EV_HLDM_MuzzleFlash
//     SOHL 1.9.1
//======================
void EV_HLDM_MuzzleFlash(vec3_t pos, int radius, int red, int green, int blue)
{
	//Flag weapon/view model for muzzle flash
    dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 999 );
    dl->origin = pos;
    dl->color.r = red; // red
    dl->color.g = green; // green
    dl->color.b = blue; // blue
	dl->radius = radius;
}

//======================
//	 BulletFlyBySound
//     SOHL 1.9.1
//======================
void EV_HLDM_BulletFlyBySound ( int idx, vec3_t start, vec3_t end )
{
	char *zngs[12]; int cnt;
	cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();
	int iRand = gEngfuncs.pfnRandomLong(1, 10);
	vec3_t soundPoint = Vector(0, 0, 0);

	if ( !EV_IsLocal(idx) )
	{
		if( EV_PointLineIntersect(start, end, pthisplayer->origin, 150, soundPoint ) )
		{
			if (iRand < 5)
			{
				zngs[0] = "misc/whizz1.wav";
				zngs[1] = "misc/whizz2.wav";
				zngs[2] = "misc/whizz3.wav";
				zngs[3] = "misc/whizz4.wav";
				zngs[4] = "misc/whizz5.wav";

				cnt = (sizeof(zngs) / sizeof(int));
				gEngfuncs.pEventAPI->EV_PlaySound( pthisplayer->index, 
					soundPoint, 
					CHAN_STATIC, 
					zngs[gEngfuncs.pfnRandomLong(0,cnt-1)], 
					gEngfuncs.pfnRandomFloat(0.92, 1.0), 
					ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );
			}
		}
	}
}

//======================
//	START DECALS
//======================
void EV_HLDM_FindHullIntersection( int idx, vec3_t vecSrc, pmtrace_t pTrace, float *mins, float *maxs )
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	pmtrace_t tmpTrace;
	vec3_t		vecHullEnd = pTrace.endpos;
	vec3_t		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();
	
	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecHullEnd, PM_STUDIO_BOX, -1, &tmpTrace );
	gEngfuncs.pEventAPI->EV_PopPMStates();
	
	if ( tmpTrace.fraction < 1.0 )
	{
		pTrace = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];
                                        
                gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true ); // Store off the old count
				gEngfuncs.pEventAPI->EV_PushPMStates();
				gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 ); // Now add in all of the players.

				gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
				gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tmpTrace );
				gEngfuncs.pEventAPI->EV_PopPMStates();
				
				if ( tmpTrace.fraction < 1.0 )
				{
					float thisDistance = Vector(tmpTrace.endpos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						pTrace = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

char *EV_HLDM_DamageDecal( physent_t *pe )
{
	static char decalname[ 32 ];
	int idx;

	if ( pe->classnumber == 1 )
	{
		idx = gEngfuncs.pfnRandomLong( 0, 2 );
		sprintf( decalname, "{break%i", idx + 1 );
	}
	else if ( pe->rendermode != kRenderNormal )
	{
		sprintf( decalname, "{bproof1" );
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong( 0, 4 );
		sprintf( decalname, "{shot%i", idx + 1 );
	}

	return decalname;
}

//======================
// EV_HLDM_CrowbarDecalTrace
//======================
void EV_HLDM_CrowbarDecalTrace(pmtrace_t *pTrace, char *decalName )
{
	physent_t *pe;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );
	if(!pe) return; //g-cont. fix old bug that crashes Spirit
          
	struct model_s *mdl = pe->model;

	// Only decal brush models such as the world etc.
	if (  decalName && decalName[0] && pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			int decal_tex = gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) );
			gEngfuncs.pEfxAPI->R_DecalShoot( decal_tex, gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0 );
			
			vec3_t mirpos = EV_MirrorPos(pTrace->endpos); 
			if(mirpos != vec3_t(0,0,0))
				gEngfuncs.pEfxAPI->R_DecalShoot(decal_tex,0, 0, mirpos, 0 );
		}
	}
}

//======================
// EV_HLDM_GunshotDecalTrace
//======================
void EV_HLDM_GunshotDecalTrace(pmtrace_t *pTrace, char *decalName )
{
	int iRand;
	physent_t *pe;

	iRand = gEngfuncs.pfnRandomLong(0,0x7FFF);
	if ( iRand < (0x7fff/2) )// not every bullet makes a sound.
	{
		switch( iRand % 5)
		{
			case 0:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 1:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 2:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 3:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 4:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		}
	}

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	struct model_s *mdl = pe->model;

	// Only decal brush models such as the world etc.
	if (  decalName && decalName[0] && pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			int decal_tex = gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) );
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				decal_tex, 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0 );
                              gEngfuncs.pEfxAPI->R_SparkStreaks(pTrace->endpos, gEngfuncs.pfnRandomLong(10,25),-120,120);
			
			vec3_t mirpos = EV_MirrorPos(pTrace->endpos); 
			if(mirpos != vec3_t(0,0,0))
			{
				//Mirror decal!
				gEngfuncs.pEfxAPI->R_DecalShoot(decal_tex,0, 0, mirpos, 0 );
				gEngfuncs.pEfxAPI->R_SparkStreaks(mirpos,gEngfuncs.pfnRandomLong(10,25),-120,120);
			}
		}
	}
}

//======================
// EV_HLDM_DecalGunshot
//======================
void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType )
{
	physent_t *pe;
	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if ( pe && pe->solid == SOLID_BSP )
	{
		switch( iBulletType )
		{
			case BULLET_PLAYER_CROWBAR:
				EV_HLDM_CrowbarDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ) );
			break;
			case BULLET_PLAYER_9MM:
			case BULLET_MONSTER_9MM:
			case BULLET_PLAYER_MP5:
			case BULLET_MONSTER_MP5:
			case BULLET_PLAYER_BUCKSHOT:
			case BULLET_PLAYER_357:
			default:
				// smoke and decal
				EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ) );
			break;
		}
	}
}

//======================
// EV_HLDM_CheckTracer
//======================
int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount )
{
	int tracer = 0;
	int i;
	qboolean player = idx >= 1 && idx <= gEngfuncs.GetMaxClients() ? true : false;

	if ( iTracerFreq != 0 && ( (*tracerCount)++ % iTracerFreq) == 0 )
	{
		vec3_t vecTracerSrc;

		if ( player )
		{
			vec3_t offset( 0, 0, -4 );

			// adjust tracer position for player
			for ( i = 0; i < 3; i++ )
			{
				vecTracerSrc[ i ] = vecSrc[ i ] + offset[ i ] + right[ i ] * 2 + forward[ i ] * 16;
			}
		}
		else
			VectorCopy( vecSrc, vecTracerSrc );
		
		if ( iTracerFreq != 1 )		// guns that always trace also always decal
			tracer = 1;

		switch( iBulletType )
		{
			case BULLET_PLAYER_MP5:
			case BULLET_MONSTER_MP5:
			case BULLET_MONSTER_9MM:
			case BULLET_MONSTER_12MM:
			default:
				EV_CreateTracer( vecTracerSrc, end );
			break;
		}
	}

	return tracer;
}

//======================
//	Play Empty Sound
//======================
void EV_PlayEmptySound( event_args_s *args )
{
	//play custom empty sound
	//added pitch tuning for final spirit release
	if (args->iparam2 == 0) 
		args->iparam2 = PITCH_NORM;

	if (args->iparam1 == 0) gEngfuncs.pEventAPI->EV_PlaySound(args->entindex, args->origin, CHAN_AUTO, "weapons/cock1.wav",    0.8, ATTN_NORM, 0, args->iparam2);
	if (args->iparam1 == 1) gEngfuncs.pEventAPI->EV_PlaySound(args->entindex, args->origin, CHAN_AUTO, "buttons/button10.wav",     0.8, ATTN_NORM, 0, args->iparam2);
	if (args->iparam1 == 2) gEngfuncs.pEventAPI->EV_PlaySound(args->entindex, args->origin, CHAN_AUTO, "buttons/button11.wav",     0.8, ATTN_NORM, 0, args->iparam2);
	if (args->iparam1 == 3) gEngfuncs.pEventAPI->EV_PlaySound(args->entindex, args->origin, CHAN_AUTO, "buttons/lightswitch2.wav", 0.8, ATTN_NORM, 0, args->iparam2);
	if (args->iparam1 == 4) gEngfuncs.pEventAPI->EV_PlaySound(args->entindex, args->origin, CHAN_AUTO, "weapons/shotgun_empty.wav", 0.8, ATTN_NORM, 0, args->iparam2);
}

//======================
//	   FireBullets
// Go to the trouble of combining multiple pellets into a single damage call.
//======================
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY )
{
	int i;
	pmtrace_t tr;
	int iShot;
	int tracer;
	
	for ( iShot = 1; iShot <= cShots; iShot++ )	
	{
		vec3_t vecDir, vecEnd;
			
		float x, y, z;
		//We randomize for the Shotgun.
		if ( iBulletType == BULLET_PLAYER_BUCKSHOT )
		{
			do {
				x = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				y = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				z = x*x+y*y;
			} while (z > 1);

			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + x * flSpreadX * right[ i ] + y * flSpreadY * up [ i ];
				vecEnd[i] = vecSrc[ i ] + flDistance * vecDir[ i ];
			}
		}//But other guns already have their spread randomized in the synched spread.
		else
		{

			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + flSpreadX * right[ i ] + flSpreadY * up [ i ];
				vecEnd[i] = vecSrc[ i ] + flDistance * vecDir[ i ];
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	
		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();
	
		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );

		//SOHL 1.9.1
		EV_HLDM_BulletFlyBySound( idx, vecSrc, vecEnd );

		tracer = EV_HLDM_CheckTracer( idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount );

		// do damage, paint decals
		if ( tr.fraction != 1.0 )
		{
			switch(iBulletType)
			{
				default:
				case BULLET_PLAYER_9MM:		
					EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
					EV_HLDM_DecalGunshot( &tr, iBulletType );
				break;
				case BULLET_PLAYER_MP5:		
					if ( !tracer )
					{
						EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
						EV_HLDM_DecalGunshot( &tr, iBulletType );
					}
				break;
				case BULLET_PLAYER_BUCKSHOT:
					EV_HLDM_DecalGunshot( &tr, iBulletType );
				break;
				case BULLET_PLAYER_357:
					EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
					EV_HLDM_DecalGunshot( &tr, iBulletType );
				break;
			}
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

//======================
//	   CROWBAR START
//======================
enum crowbar_e {
	CROWBAR_IDLE = 0,
	CROWBAR_DRAW,
	CROWBAR_HOLSTER,
	CROWBAR_ATTACK1HIT,
	CROWBAR_ATTACK1MISS,
	CROWBAR_ATTACK2MISS,
	CROWBAR_ATTACK2HIT,
	CROWBAR_ATTACK3MISS,
	CROWBAR_ATTACK3HIT,
	CROWBAR_IDLE2,
	CROWBAR_IDLE3
};

int g_iSwing;

void EV_FireCrowbar(event_args_t *args)
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
          vec3_t up, right, forward;
	physent_t *pHit;
	pmtrace_t tr;
	
	idx = args->entindex;
	VectorCopy( args->angles, angles );
	VectorCopy( args->origin, origin );
	vec3_t vecSrc, vecEnd;
          
          AngleVectors( angles, forward, right, up );
	
	EV_GetGunPosition( args, vecSrc, origin );
	vecEnd = vecSrc + forward * 32;	
          
	//make trace 
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );
	gEngfuncs.pEventAPI->EV_PopPMStates();

	if ( tr.fraction >= 1.0 )
	{
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );
		gEngfuncs.pEventAPI->EV_PopPMStates();

		if ( tr.fraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			pHit = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

			if ( !pHit || pHit->solid == SOLID_BSP )
				EV_HLDM_FindHullIntersection( idx, vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
				vecEnd = tr.endpos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
	if ( tr.fraction >= 1.0 )
	{
		if(args->iparam2) //fFirst
		{
			if ( EV_IsLocal( idx ) )
			{
				// miss
				switch( (g_iSwing++) % 3 )
				{
					case 0: gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK1MISS, args->iparam1 ); break;
					case 1: gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK2MISS, args->iparam1 ); break;
					case 2: gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK3MISS, args->iparam1 ); break;
				}
			}

			// play wiff or swish sound
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xF ));
		}
	}
	else
	{
		if ( EV_IsLocal( idx ) )
		{
			switch( ((g_iSwing++) % 2) + 1 )
			{
				case 0: gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK1HIT, args->iparam1 ); break;
				case 1: gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK2HIT, args->iparam1 ); break;
				case 2: gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK3HIT, args->iparam1 ); break;
			}
		}

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;
  		pHit = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );
  		
		if (pHit)
		{
			if ( args->bparam1 )
			{	// play thwack or smack sound
				switch( gEngfuncs.pfnRandomLong(0,2) )
				{
					case 0: gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM, 0, PITCH_NORM); break;
					case 1: gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/cbar_hitbod2.wav", 1, ATTN_NORM, 0, PITCH_NORM); break;
					case 2: gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM, 0, PITCH_NORM); break;
				}

				fHitWorld = FALSE;
				args->bparam1 = FALSE; //hit monster
			}
		}

		if (fHitWorld)
		{
			float fvolbar = EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR );

			// also play crowbar strike
			switch( gEngfuncs.pfnRandomLong(0,1) )
			{
				case 0: gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0,3)); break;
				case 1: gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0,3)); break;
			}

			// delay the decal a bit
			EV_HLDM_DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );
		}
	}
}

//======================
//	    GLOCK START
//======================
void EV_FireGlock1( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	empty = args->bparam1;
	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		cl_entity_s *view = gEngfuncs.GetViewModel();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? GLOCK_SHOOT_EMPTY : GLOCK_SHOOT, args->iparam1 );
		EV_MuzzleFlash();

		if(args->bparam2)//silence mode
			gEngfuncs.pEfxAPI->R_MuzzleFlash( (float *)&view->attachment[1], 17 );
		else//normal flash
			gEngfuncs.pEfxAPI->R_MuzzleFlash( (float *)&view->attachment[0], 11 );

		V_PunchAxis( 0, -2.0 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );
	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	if(args->bparam2)
	{
		switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
		{
			case 0:
				gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pl_gun1.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 2 ) );
				break;
			case 1:
				gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pl_gun2.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 2 ) );
				break;
		}
	}
	else gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_MuzzleFlash( vecSrc, gEngfuncs.pfnRandomLong( 260, 300 ), 255, 255, 128 ); //SOHL 1.9.1
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, 0, args->fparam1, args->fparam2 );
}

//======================
//	    MP5 START
//======================
void EV_FireMP5( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int body;
	
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	body = args->iparam1;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5_FIRE1 + gEngfuncs.pfnRandomLong(0,2), body );

		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -2, 2 ) );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );
	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	}

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_MuzzleFlash( vecSrc, gEngfuncs.pfnRandomLong( 260, 300 ), 255, 255, 128 ); //SOHL 1.9.1

	if ( gEngfuncs.GetMaxClients() > 1 )
		EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5, 2, &tracerCount[idx-1], args->fparam1, args->fparam2 );
	else 	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5, 2, &tracerCount[idx-1], args->fparam1, args->fparam2 );
}

//======================
//	  SHOTGUN START
//======================
void EV_FireShotGunDouble( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	int j;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shotgunshell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SHOTGUN_FIRE2, args->iparam1 );
		V_PunchAxis( 0, -10.0 );
	}

	for ( j = 0; j < 2; j++ )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6 );
		EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL ); 
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/dbarrel1.wav", gEngfuncs.pfnRandomFloat(0.98, 1.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_MuzzleFlash( vecSrc, gEngfuncs.pfnRandomLong( 260, 300 ), 255, 255, 128 ); //SOHL 1.9.1

	if ( gEngfuncs.GetMaxClients() > 1 )
		EV_HLDM_FireBullets( idx, forward, right, up, 8, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1], 0.17365, 0.04362 );
	else
		EV_HLDM_FireBullets( idx, forward, right, up, 12, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1], 0.08716, 0.08716 );
}

void EV_FireShotGunSingle( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shotgunshell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SHOTGUN_FIRE, args->iparam1 );

		V_PunchAxis( 0, -5.0 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6 );
	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/sbarrel1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_MuzzleFlash( vecSrc, gEngfuncs.pfnRandomLong( 260, 300 ), 255, 255, 128 ); //SOHL 1.9.1

	if ( gEngfuncs.GetMaxClients() > 1 )
		EV_HLDM_FireBullets( idx, forward, right, up, 4, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1], 0.08716, 0.04362 );
	else
		EV_HLDM_FireBullets( idx, forward, right, up, 6, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1], 0.08716, 0.08716 );
}

//======================
//	   PHYTON START 
//	     ( .357 )
//======================
void EV_FirePython( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		// Python uses different body in multiplayer versus single player
		int multiplayer = gEngfuncs.GetMaxClients() == 1 ? 0 : 1;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( PYTHON_FIRE1, args->iparam1 );

		V_PunchAxis( 0, -10.0 );
	}

	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
		case 0:
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/357_shot1.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );
		break;
		case 1:
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/357_shot2.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );
		break;
	}

	EV_GetGunPosition( args, vecSrc, origin );

	VectorCopy( forward, vecAiming );

	EV_HLDM_MuzzleFlash( vecSrc, gEngfuncs.pfnRandomLong( 260, 300 ), 255, 255, 128 ); //SOHL 1.9.1
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_357, 0, 0, args->fparam1, args->fparam2 );
}

//======================
//	   GAUSS START 
//======================
#define SND_CHANGE_PITCH	(1<<7)		// duplicated in protocol.h change sound pitch

void EV_SpinGauss( event_args_t *args )
{
	int pitch = args->iparam1;
	int iSoundState = args->bparam1 ? SND_CHANGE_PITCH : 0;

	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, args->origin, CHAN_WEAPON, "ambience/pulsemachine.wav", 1.0, ATTN_NORM, iSoundState, pitch );
}

/*
==============================
    EV_StopPreviousGauss
==============================
*/
void EV_StopPreviousGauss( int idx )
{
	// Make sure we don't have a gauss spin event in the queue for this guy
	gEngfuncs.pEventAPI->EV_KillEvents( idx, "events/gaussspin.sc" );
	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_WEAPON, "ambience/pulsemachine.wav" );
}

void EV_FireGauss( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles, mangles;
	vec3_t velocity;
	float flDamage = args->fparam1;

	int m_fPrimaryFire = args->bparam1;
	int m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	vec3_t vecSrc, vecDest;
	vec3_t vecMirrorSrc, vecMirrorDest; //env_mirror use this
  
	edict_t		*pentIgnore;
	pmtrace_t tr, mtr, beam_tr;
	float flMaxFrac = 1.0;
	int	nTotal = 0;                             
	int fHasPunched = 0;
	int fFirstBeam = 1;
	int	nMaxHits = 10;
	physent_t *pEntity, *pMEntity;
	int m_iBeam, m_iGlow, m_iBalls;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	if ( args->bparam2 )
	{
		EV_StopPreviousGauss( idx );
		return;
	}

//	Con_Printf( "Firing gauss with %f\n", flDamage );
	EV_GetGunPosition( args, vecSrc, origin );

	m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/smoke.spr" );
	m_iBalls = m_iGlow = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/hotglow.spr" );
	
	AngleVectors( angles, forward, NULL, NULL );
          
	mangles = EV_MirrorVector(angles);
	AngleVectors( mangles, right, NULL, NULL );
	
	vecMirrorSrc = EV_MirrorPos( vecSrc );
	
	VectorMA( vecSrc, 8192, forward, vecDest );
    VectorMA( vecMirrorSrc, 8192, right, vecMirrorDest );
	
	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -2.0 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( GAUSS_FIRE2, args->iparam1 );
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/gauss2.wav", 0.5 + flDamage * (1.0 / 400.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	while (flDamage > 10 && nMaxHits > 0)
	{
		nMaxHits--;

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
		
		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();
	
		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecDest, PM_STUDIO_BOX, -1, &tr );
                    
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecMirrorSrc, vecMirrorDest, PM_STUDIO_BOX, -1, &mtr );

		gEngfuncs.pEventAPI->EV_PopPMStates();
                    
		if ( tr.allsolid ) break;

		if (fFirstBeam)
		{
			if ( EV_IsLocal( idx ) )
			{
				// Add muzzle flash to current weapon model
				EV_MuzzleFlash();
			}
			fFirstBeam = 0;

			gEngfuncs.pEfxAPI->R_BeamEntPoint( idx | 0x1000,tr.endpos,m_iBeam,0.1,m_fPrimaryFire ? 1.0 : 2.5,0.0,m_fPrimaryFire ? 128.0 : flDamage, 0,0,0,m_fPrimaryFire ? 255 : 255, m_fPrimaryFire ? 128 : 255, m_fPrimaryFire ? 0 : 255 );
			if((vecMirrorDest != vec3_t(0,0,0)) && (vecMirrorSrc != vec3_t(0,0,0)))
				gEngfuncs.pEfxAPI->R_BeamPoints( vecMirrorSrc,mtr.endpos,m_iBeam,0.1,m_fPrimaryFire ? 1.0 : 2.5,0.0,m_fPrimaryFire ? 128.0 : flDamage,0,0,0,m_fPrimaryFire ? 255 : 255,m_fPrimaryFire ? 128 : 255,m_fPrimaryFire ? 0 : 255);
		}
		else
		{
			gEngfuncs.pEfxAPI->R_BeamPoints( vecSrc, tr.endpos, m_iBeam, 0.1, m_fPrimaryFire ? 1.0 : 2.5,0.0,m_fPrimaryFire ? 128.0 : flDamage,0,0,0,m_fPrimaryFire ? 255 : 255,m_fPrimaryFire ? 128 : 255,m_fPrimaryFire ? 0 : 255);
		}

		pEntity = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );
		pMEntity = gEngfuncs.pEventAPI->EV_GetPhysent( mtr.ent );

		if ( pEntity == NULL )
			break;
                    
		if ( pEntity->solid == SOLID_BSP )
		{
			float n;

			pentIgnore = NULL;

			n = -DotProduct( tr.plane.normal, forward );

			if (n < 0.5) // 60 degrees	
			{
				// ALERT( at_console, "reflect %f\n", n );
				// reflect
				vec3_t r;
			
				VectorMA( forward, 2.0 * n, tr.plane.normal, r );

				flMaxFrac = flMaxFrac - tr.fraction;
				
				VectorCopy( r, forward );

				VectorMA( tr.endpos, 8.0, forward, vecSrc );
				VectorMA( vecSrc, 8192.0, forward, vecDest );

				gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage * n / 255.0, flDamage * n * 0.5 * 0.1, FTENT_FADEOUT );
                                        if((vecMirrorDest != vec3_t(0,0,0)) && (vecMirrorSrc != vec3_t(0,0,0))) 
                                                 gEngfuncs.pEfxAPI->R_TempSprite( mtr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage * n / 255.0, flDamage * n * 0.5 * 0.1, FTENT_FADEOUT );
                                        
				vec3_t fwd;
				VectorAdd( tr.endpos, tr.plane.normal, fwd );

				gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 100, 255, 100 );
                                        
				// lose energy
				if ( n == 0 )
				{
					n = 0.1;
				}
				
				flDamage = flDamage * (1 - n);

			}
			else
			{
				// tunnel
				EV_HLDM_DecalGunshot( &tr, BULLET_MONSTER_12MM );

				gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 1.0, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT );
                                        if((vecMirrorDest != vec3_t(0,0,0)) && (vecMirrorSrc != vec3_t(0,0,0)))
                                        	gEngfuncs.pEfxAPI->R_TempSprite( mtr.endpos, vec3_origin, 1.0, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT );

				// limit it to one hole punch
				if (fHasPunched) break;
				fHasPunched = 1;
				
				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if ( !m_fPrimaryFire )
				{
					vec3_t start;
					
					VectorMA( tr.endpos, 8.0, forward, start );
					
					// Store off the old count
					gEngfuncs.pEventAPI->EV_PushPMStates();
						
					// Now add in all of the players.
					gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );

					gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
					gEngfuncs.pEventAPI->EV_PlayerTrace( start, vecDest, PM_STUDIO_BOX, -1, &beam_tr );

					if ( !beam_tr.allsolid )
					{
						vec3_t delta;

						// trace backwards to find exit point

						gEngfuncs.pEventAPI->EV_PlayerTrace( beam_tr.endpos, tr.endpos, PM_STUDIO_BOX, -1, &beam_tr );

						VectorSubtract( beam_tr.endpos, tr.endpos, delta );
						
						n = Length( delta );

						if (n < flDamage)
						{
							if (n == 0)
								n = 1;
							flDamage -= n;

							// absorption balls
							{
								vec3_t fwd;
								VectorSubtract( tr.endpos, forward, fwd );
								gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 100, 255, 100 );
							}

							EV_HLDM_DecalGunshot( &beam_tr, BULLET_MONSTER_12MM );
							
							gEngfuncs.pEfxAPI->R_TempSprite( beam_tr.endpos, vec3_origin, 0.1, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT );
			
							// balls
							{
								vec3_t fwd;
								VectorSubtract( beam_tr.endpos, forward, fwd );
								gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, beam_tr.endpos, fwd, m_iBalls, (int)(flDamage * 0.3), 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 200, 255, 40 );
							}
							
							VectorAdd( beam_tr.endpos, forward, vecSrc );
						}
					}
					else
					{
						flDamage = 0;
					}

					gEngfuncs.pEventAPI->EV_PopPMStates();
				}
				else
				{
					if ( m_fPrimaryFire )
					{
						// slug doesn't punch through ever with primary 
						// fire, so leave a little glowy bit and make some balls
						gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, 200.0 / 255.0, 0.3, FTENT_FADEOUT );
			                              if((vecMirrorDest != vec3_t(0,0,0)) && (vecMirrorSrc != vec3_t(0,0,0))) 
			                              	gEngfuncs.pEfxAPI->R_TempSprite( mtr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, 200.0 / 255.0, 0.3, FTENT_FADEOUT );
						
						// balls
						{
							vec3_t fwd;
							int num_balls = gEngfuncs.pfnRandomFloat( 10, 20 );
							VectorAdd( tr.endpos, tr.plane.normal, fwd );
							gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 8, 0.6, num_balls / 100.0, 100, 255, 200 );
							VectorAdd( mtr.endpos, mtr.plane.normal, fwd );
							if((vecMirrorDest != vec3_t(0,0,0)) && (vecMirrorSrc != vec3_t(0,0,0)))
								gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, mtr.endpos, fwd, m_iBalls, 8, 0.6, num_balls / 100.0, 100, 255, 200 );
						}
					}

					flDamage = 0;
				}
			}
		}
		else
		{
			VectorAdd( tr.endpos, forward, vecSrc );
		}
	}
}

//======================
//	    EGON START 
//======================
enum egon_e {
	EGON_IDLE1 = 0,
	EGON_FIDGET1,
	EGON_ALTFIREON,
	EGON_ALTFIREOFF,
	EGON_FIRESTOP,
	EGON_FIRECYCLE,
	EGON_DRAW,
	EGON_HOLSTER
};

enum EGON_FIRESTATE { FIRE_OFF = 0, FIRE_CHARGE };
enum EGON_FIREMODE { FIRE_NARROW = 0, FIRE_WIDE };

#define EGON_PRIMARY_VOLUME		450
#define EGON_BEAM_SPRITE		"sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.spr"
#define EGON_SOUND_OFF			"weapons/egon_off1.wav"
#define EGON_SOUND_RUN			"weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP		"weapons/egon_windup2.wav"

#define ARRAYSIZE(p)		(sizeof(p)/sizeof(p[0]))

BEAM *pBeam;
BEAM *pBeam2;

BEAM *pMirBeam;
BEAM *pMirBeam2;
TEMPENTITY *EndFlare;
TEMPENTITY *EndMirFlare;

//UNDONE : mirror beams don't update, end sprite don't drawing
bool b_mir = 0;

void EV_EgonFire( event_args_t *args )
{
	int idx, iFireState, iFireMode;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	iFireMode = args->iparam2;
	int iStartup = args->bparam1;
	iFireState = args->bparam2;
	int m_iFlare = gEngfuncs.pEventAPI->EV_FindModelIndex( EGON_FLARE_SPRITE );

	if ( iStartup )
	{
		if ( iFireMode == FIRE_WIDE )
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.98, ATTN_NORM, 0, 125 );
		else	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.9, ATTN_NORM, 0, 100 );
	}
	else
	{
		if ( iFireMode == FIRE_WIDE )
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.98, ATTN_NORM, 0, 125 );
		else	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.9, ATTN_NORM, 0, 100 );
	}

	//Only play the weapon anims if I shot it.
	//if ( EV_IsLocal( idx ) ) gEngfuncs.pEventAPI->EV_WeaponAnimation ( EGON_FIRECYCLE, args->iparam1 );

	if ( iStartup == 1 && EV_IsLocal( idx ) && !pBeam && !pBeam2 && cl_lw->value )
	{
		vec3_t vecSrc, vecEnd, angles, forward, right, mangles;
		vec3_t vecMirrorSrc, vecMirrorDest; //env_mirror use this
		pmtrace_t tr, mtr;

		cl_entity_t *pl = gEngfuncs.GetEntityByIndex( idx );

		if ( pl )
		{
			VectorCopy( gHUD.m_vecAngles, angles );
			AngleVectors( angles, forward, NULL, NULL );
                              EV_GetGunPosition( args, vecSrc, pl->origin );
                              
			mangles = EV_MirrorVector(angles);
			AngleVectors( mangles, right, NULL, NULL );
			vecMirrorSrc = EV_MirrorPos( vecSrc );
			
			VectorMA( vecSrc, 2048, forward, vecEnd );
			VectorMA( vecMirrorSrc, 2048, right, vecMirrorDest );
			gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );	
				
			// Store off the old count
			gEngfuncs.pEventAPI->EV_PushPMStates();
			// Now add in all of the players.
			gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );
                              //mirror trace
			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecMirrorSrc, vecMirrorDest, PM_STUDIO_BOX, -1, &mtr );

			gEngfuncs.pEventAPI->EV_PopPMStates();

			int iBeamModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( EGON_BEAM_SPRITE );

			float r = 50.0f;
			float g = 50.0f;
			float b = 125.0f;

			if ( IEngineStudio.IsHardware() )
			{
				r /= 100.0f;
				g /= 100.0f;
			}
				
		
			if ( iFireMode == FIRE_WIDE )
			{
				pBeam = gEngfuncs.pEfxAPI->R_BeamEntPoint ( idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 3.5, 0.2, 0.7, 55, 0, 0, r, g, b );
				if ( pBeam )pBeam->flags |= ( FBEAM_SINENOISE );
				pBeam2 = gEngfuncs.pEfxAPI->R_BeamEntPoint ( idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 5.0, 0.08, 0.7, 25, 0, 0, r, g, b );
				if((vecMirrorDest != vec3_t(0,0,0)) && (vecMirrorSrc != vec3_t(0,0,0)) && b_mir)
				{
					pMirBeam = gEngfuncs.pEfxAPI->R_BeamPoints ( vecMirrorSrc, mtr.endpos, iBeamModelIndex, 99999, 3.5, 0.2, 0.7, 55, 0, 0, r, g, b );
					if ( pMirBeam )pBeam->flags |= ( FBEAM_SINENOISE );
					pMirBeam2 = gEngfuncs.pEfxAPI->R_BeamPoints ( vecMirrorSrc, mtr.endpos, iBeamModelIndex, 99999, 5.0, 0.08, 0.7, 25, 0, 0, r, g, b );
				}
			}
			else
			{
				pBeam = gEngfuncs.pEfxAPI->R_BeamEntPoint ( idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 1.5, 0.1, 0.7, 55, 0, 0, r, g, b );
				if ( pBeam )pBeam->flags |= ( FBEAM_SINENOISE );
				pBeam2 = gEngfuncs.pEfxAPI->R_BeamEntPoint ( idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 5.0, 0.08, 0.7, 25, 0, 0, r, g, b );
				if((vecMirrorDest != vec3_t(0,0,0)) && (vecMirrorSrc != vec3_t(0,0,0)) && b_mir)
				{
					pMirBeam = gEngfuncs.pEfxAPI->R_BeamPoints ( vecMirrorSrc, mtr.endpos, iBeamModelIndex, 99999, 1.5, 0.1, 0.7, 55, 0, 0, r, g, b );
					if ( pMirBeam )pBeam->flags |= ( FBEAM_SINENOISE );
					pMirBeam2 = gEngfuncs.pEfxAPI->R_BeamPoints ( vecMirrorSrc, mtr.endpos, iBeamModelIndex, 99999, 5.0, 0.08, 0.7, 25, 0, 0, r, g, b );
				}
			}

                     	EndFlare = gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 1.0, m_iFlare, kRenderGlow, kRenderFxNoDissipation, 255, 9999, FTENT_SPRANIMATE );
			if((vecMirrorDest != vec3_t(0,0,0)) && (vecMirrorSrc != vec3_t(0,0,0)) && b_mir)
                     		EndMirFlare = gEngfuncs.pEfxAPI->R_TempSprite( mtr.endpos, vec3_origin, 1.0, m_iFlare, kRenderGlow, kRenderFxNoDissipation, 255, 9999, FTENT_SPRANIMATE );
		}
	}
}

void EV_EgonStop( event_args_t *args )
{
	int idx;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy ( args->origin, origin );

	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, EGON_SOUND_RUN );
	
	if ( args->iparam1 )
		 gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_OFF, 0.98, ATTN_NORM, 0, 100 );

	if ( EV_IsLocal( idx ) ) 
	{
		if ( pBeam )
		{
			pBeam->die = 0.0;
			pBeam = NULL;
		}
		if ( pBeam2 )
		{
			pBeam2->die = 0.0;
			pBeam2 = NULL;
		}
		if ( pMirBeam )
		{
			pMirBeam->die = 0.0;
			pMirBeam = NULL;
		}
		if ( pMirBeam2 )
		{
			pMirBeam2->die = 0.0;
			pMirBeam2 = NULL;
		}
		if( EndFlare )
		{
			EndFlare->die = 0.0;
			EndFlare = NULL;
		}
		if( EndMirFlare )
		{
			EndMirFlare->die = 0.0;
			EndMirFlare = NULL;
		}
		gEngfuncs.pEventAPI->EV_WeaponAnimation( EGON_FIRESTOP, args->iparam2 );
	}
}

void EV_UpdateBeams ( void )
{
	if ( !pBeam && !pBeam2 ) return;
	
	vec3_t forward, vecSrc, vecEnd, origin, angles, right, mangles;
	vec3_t vecMirrorSrc, vecMirrorDest; //env_mirror use this
	pmtrace_t tr, mtr;

	cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();
	int idx = pthisplayer->index;
	
	// Get our exact viewangles from engine
	gEngfuncs.GetViewAngles( (float *)angles );

	VectorCopy(previousorigin, origin);

	AngleVectors( angles, forward, NULL, NULL );
	VectorCopy( origin, vecSrc );
	VectorMA( vecSrc, 2048, forward, vecEnd );

	mangles = EV_MirrorVector(angles);
	AngleVectors( mangles, right, NULL, NULL );
	vecMirrorSrc = EV_MirrorPos( vecSrc );
          VectorMA( vecMirrorSrc, 2048, right, vecMirrorDest );
	
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );	
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );

 	//mirror trace
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecMirrorSrc, vecMirrorDest, PM_STUDIO_BOX, -1, &mtr );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	if ( pBeam )
	{
		pBeam->target = tr.endpos;
		pBeam->die = gEngfuncs.GetClientTime() + 0.1; // We keep it alive just a little bit forward in the future, just in case.
	}
	if ( pBeam2 )
	{
		pBeam2->target = tr.endpos;
		pBeam2->die = gEngfuncs.GetClientTime() + 0.1; // We keep it alive just a little bit forward in the future, just in case.
	}
	if ( pMirBeam )
	{
		pMirBeam->target = mtr.endpos;
		pMirBeam->die = gEngfuncs.GetClientTime() + 0.1; // We keep it alive just a little bit forward in the future, just in case.
	}
	if ( pMirBeam2 )
	{
		pMirBeam2->target = mtr.endpos;
		pMirBeam2->die = gEngfuncs.GetClientTime() + 0.1; // We keep it alive just a little bit forward in the future, just in case.
	}
	if( EndFlare )
	{
		EndFlare->tentOffset = tr.endpos;
		EndFlare->die = gEngfuncs.GetClientTime() + 0.1;
	}
	if( EndMirFlare )
	{
		EndMirFlare->tentOffset = mtr.endpos;
		EndMirFlare->die = gEngfuncs.GetClientTime() + 0.1;
	}
}

//======================
//	   DECALS START
//======================
void EV_Decals( struct event_args_s *args )
{
	int idx;
	pmtrace_t tr;
	pmtrace_t *pTrace = &tr;
          physent_t *pe;
          
	idx = args->entindex;
	VectorCopy( args->origin, pTrace->endpos );
	pTrace->ent = args->iparam1; 
	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );
	
	if(args->iparam2 == 0)//explode decals
	{
		switch( gEngfuncs.pfnRandomLong( 0, 2 ) )
		{
			case 0: EV_HLDM_CrowbarDecalTrace( pTrace, "{scorch1" ); break;
			case 1: EV_HLDM_CrowbarDecalTrace( pTrace, "{scorch2" ); break;
			case 2: EV_HLDM_CrowbarDecalTrace( pTrace, "{scorch3" ); break;
		}
	}

	if(args->iparam2 == 1)//red blood decals
	{
		switch( gEngfuncs.pfnRandomLong( 0, 7 ) )
		{
			case 0: EV_HLDM_CrowbarDecalTrace( pTrace, "{blood1" ); break;
			case 1: EV_HLDM_CrowbarDecalTrace( pTrace, "{blood2" ); break;
			case 2: EV_HLDM_CrowbarDecalTrace( pTrace, "{blood3" ); break;
			case 3: EV_HLDM_CrowbarDecalTrace( pTrace, "{blood4" ); break;
			case 4: EV_HLDM_CrowbarDecalTrace( pTrace, "{blood5" ); break;
			case 5: EV_HLDM_CrowbarDecalTrace( pTrace, "{blood6" ); break;
			case 6: EV_HLDM_CrowbarDecalTrace( pTrace, "{blood7" ); break;
			case 7: EV_HLDM_CrowbarDecalTrace( pTrace, "{blood8" ); break;
		}
	}

	if(args->iparam2 == 2)//yellow blood decals
	{
		switch( gEngfuncs.pfnRandomLong( 0, 5 ) )
		{
			case 0: EV_HLDM_CrowbarDecalTrace( pTrace, "{yblood1" ); break;
			case 1: EV_HLDM_CrowbarDecalTrace( pTrace, "{yblood2" ); break;
			case 2: EV_HLDM_CrowbarDecalTrace( pTrace, "{yblood3" ); break;
			case 3: EV_HLDM_CrowbarDecalTrace( pTrace, "{yblood4" ); break;
			case 4: EV_HLDM_CrowbarDecalTrace( pTrace, "{yblood5" ); break;
			case 5: EV_HLDM_CrowbarDecalTrace( pTrace, "{yblood6" ); break;
		}
	}

	if(args->iparam2 == 4)//bulsquid decals
	{
		switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
		{
			case 0: EV_HLDM_CrowbarDecalTrace( pTrace, "{spit1" ); break;
			case 1: EV_HLDM_CrowbarDecalTrace( pTrace, "{spit2" ); break;
		}
	}

	if(args->iparam2 == 5)//garg flames
	{
		switch( gEngfuncs.pfnRandomLong( 0, 2 ) )
		{
			case 0: EV_HLDM_CrowbarDecalTrace( pTrace, "{smscorch1" ); break;
			case 1: EV_HLDM_CrowbarDecalTrace( pTrace, "{smscorch2" ); break;
			case 2: EV_HLDM_CrowbarDecalTrace( pTrace, "{smscorch3" ); break;
		}
	}

	if(args->iparam2 == 6)//monsters shoot
	{
           	if ( pe && pe->solid == SOLID_BSP )
			EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ) );
	}
}

//======================
//     EFX START
//======================
void EV_Explode( struct event_args_s *args )
{
	int m_iExplodeSprite;
	vec3_t mirpos = EV_MirrorPos(args->origin); 	

	if(args->bparam1)//water explosion
		m_iExplodeSprite = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/WXplo1.spr" );
    else	
		m_iExplodeSprite = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/zerogxplode.spr" ); 

	gEngfuncs.pEfxAPI->R_Explosion( args->origin, m_iExplodeSprite, (args->fparam1 - 50) * 0.06, 15, TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOPARTICLES );
	EV_HLDM_MuzzleFlash( mirpos, gEngfuncs.pfnRandomLong( 260, 300 ), 255, 255, 128 ); //SOHL 1.9.1

	if(mirpos != vec3_t(0,0,0))
		gEngfuncs.pEfxAPI->R_Explosion( mirpos, m_iExplodeSprite, (args->fparam1 - 50) * 0.06, 15, TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOSOUND | TE_EXPLFLAG_NOPARTICLES );
}

//======================
//	   SQUEAK START
//======================
enum squeak_e {
	SQUEAK_IDLE1 = 0,
	SQUEAK_FIDGETFIT,
	SQUEAK_FIDGETNIP,
	SQUEAK_DOWN,
	SQUEAK_UP,
	SQUEAK_THROW
};

void EV_SnarkFire( event_args_t *args )
{
	int idx;
	vec3_t vecSrc, angles, view_ofs, forward;
	pmtrace_t tr;

	idx = args->entindex;
	VectorCopy( args->origin, vecSrc );
	VectorCopy( args->angles, angles );

	AngleVectors ( angles, forward, NULL, NULL );
		
	if ( !EV_IsLocal ( idx ) )
		return;
	
	if ( args->ducking )
		vecSrc = vecSrc - ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
	
	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc + forward * 20, vecSrc + forward * 64, PM_NORMAL, -1, &tr );

	//Find space to drop the thing.
	if ( tr.allsolid == 0 && tr.startsolid == 0 && tr.fraction > 0.25 )
		 gEngfuncs.pEventAPI->EV_WeaponAnimation ( SQUEAK_THROW, args->iparam1 );
	
	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//======================
//	   SQUEAK END
//======================

//======================
//	   NULL START
//======================
void EV_FireNull(event_args_t *args)
{
	CONPRINT("Fire Null!\n");
}

//======================
//	   NULL END
//======================
void EV_TrainPitchAdjust( event_args_t *args )
{
	int idx;
	vec3_t origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;
	
	char sz[ 256 ];

	idx = args->entindex;
	
	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop	  = args->bparam1;

	m_flVolume	= (float)(us_params & 0x003f)/40.0;
	noise		= (int)(((us_params) >> 12 ) & 0x0007);
	pitch		= (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch ( noise )
	{
		case 1: strcpy( sz, "plats/ttrain1.wav"); break;
		case 2: strcpy( sz, "plats/ttrain2.wav"); break;
		case 3: strcpy( sz, "plats/ttrain3.wav"); break; 
		case 4: strcpy( sz, "plats/ttrain4.wav"); break;
		case 5: strcpy( sz, "plats/ttrain6.wav"); break;
		case 6: strcpy( sz, "plats/ttrain7.wav"); break;
		default:
			// no sound
			strcpy( sz, "" );
			return;
	}

	if ( stop )
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, sz );
	else
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
}

int EV_TFC_IsAllyTeam( int iTeam1, int iTeam2 )
{
	return 0;
}

/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
void _DLLEXPORT HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed )
{
	// Offset final origin by view_offset
	if(cl_lw->value)
	{
		for (int i = 0; i < 3; i++ ) previousorigin[i] = to->playerstate.origin[i] + to->client.view_ofs[ i ];
		to = NULL;
	}
}