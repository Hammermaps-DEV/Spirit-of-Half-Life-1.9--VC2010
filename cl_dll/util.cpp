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
//
// util.cpp
//
// implementation of class-less helper functions
//

#include "STDIO.H"
#include "STDLIB.H"
#include "MATH.H"

#include "hud.h"
#include "cl_util.h"
#include <string.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

vec3_t vec3_origin( 0, 0, 0 );

double sqrt(double x);

float Length(const float *v)
{
	int		i;
	float	length;
	
	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);		// FIXME

	return length;
}

void VectorAngles( const float *forward, float *angles )
{
	float	tmp, yaw, pitch;
	
	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		tmp = sqrt (forward[0]*forward[0] + forward[1]*forward[1]);
		pitch = (atan2(forward[2], tmp) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

float VectorNormalize (float *v)
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);		// FIXME

	if (length)
	{
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;

}

void VectorInverse ( float *v )
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale (const float *in, float scale, float *out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

void VectorMA (const float *veca, float scale, const float *vecb, float *vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}

HL_HSPRITE LoadSprite(const char *pszName)
{
	int i;
	char sz[256]; 

	if (ScreenWidth < 640)
		i = 320;
	else
		i = 640;

	sprintf(sz, pszName, i);

	return SPR_Load(sz);
}

//SOHL 1.9.1
vec_t VectorLengthSquared( const vec3_t v )
{
	return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

float TransformColor ( float color )
{
	float trns_clr;
	if(color >= 0 ) trns_clr = color / 255.0f;
	else trns_clr = 1.0;//default value
	return trns_clr;
}

/*
====================
Sys LoadGameDLL

====================
*/
bool Sys_LoadLibrary (const char* dllname, dllhandle_t* handle, const dllfunction_t *fcts)
{
	const dllfunction_t *gamefunc;
	char dllpath[128];
	dllhandle_t dllhandle = 0;

	if (handle == NULL) return false;

	// Initializations
	for (gamefunc = fcts; gamefunc && gamefunc->name != NULL; gamefunc++)
		*gamefunc->funcvariable = NULL;

	sprintf(dllpath, "%s/cl_dlls/%s", gEngfuncs.pfnGetGameDirectory(), dllname);
	dllhandle = LoadLibrary (dllpath);
        
	// No DLL found
	if (! dllhandle) return false;

	// Get the function adresses
	for( gamefunc = fcts; gamefunc && gamefunc->name != NULL; gamefunc++ )
	{
		if (!(*gamefunc->funcvariable = (void *) Sys_GetProcAddress (dllhandle, gamefunc->name)))
		{
			CONPRINT( "Error: failed to get proc address %s\n", gamefunc->name );
			Sys_UnloadLibrary (&dllhandle);
			return false;
		}
	}
          
	CONPRINT( "%s sucessfully loaded\n", dllname );
	*handle = dllhandle;
	return true;
}


void Sys_UnloadLibrary (dllhandle_t* handle)
{
	if (handle == NULL || *handle == NULL)
		return;

	FreeLibrary (*handle);
	*handle = NULL;
}

void* Sys_GetProcAddress (dllhandle_t handle, const char* name)
{
	return (void *)GetProcAddress (handle, name);
}

//============
// UTIL_FileExtension
// returns file extension
//============
const char *UTIL_FileExtension( const char *in )
{
	const char *separator, *backslash, *colon, *dot;

	separator = strrchr( in, '/' );
	backslash = strrchr( in, '\\' );
	if( !separator || separator < backslash )
		separator = backslash;
	colon = strrchr( in, ':' );
	if( !separator || separator < colon )
		separator = colon;
	dot = strrchr( in, '.' );
	if( dot == NULL || (separator && ( dot < separator )))
		return "";
	return dot + 1;
}