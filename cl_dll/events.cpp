//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "event_api.h"

extern "C"
{
	void EV_FireNull(event_args_t *args);
	void EV_FireCrowbar(event_args_t *args);
	void EV_PlayEmptySound( struct event_args_s *args );
	void EV_FireGlock1( struct event_args_s *args  );
	void EV_FireMP5( struct event_args_s *args  );
	void EV_FirePython( struct event_args_s *args  );
	void EV_FireGauss( struct event_args_s *args  );
	void EV_SpinGauss( struct event_args_s *args  );
	void EV_EgonFire( struct event_args_s *args );
	void EV_EgonStop( struct event_args_s *args );
	void EV_FireShotGunSingle( struct event_args_s *args  );
	void EV_FireShotGunDouble( struct event_args_s *args  );
	void EV_SnarkFire( struct event_args_s *args );
	void EV_TrainPitchAdjust( struct event_args_s *args );
	void EV_Decals( struct event_args_s *args );
	void EV_Explode( struct event_args_s *args );
}

/*
======================
Game_HookEvents

Associate script file name with callback functions.  Callback's must be extern "C" so
 the engine doesn't get confused about name mangling stuff.  Note that the format is
 always the same.  Of course, a clever mod team could actually embed parameters, behavior
 into the actual .sc files and create a .sc file parser and hook their functionality through
 that.. i.e., a scripting system.

That was what we were going to do, but we ran out of time...oh well.
======================
*/

void EV_HookEvents( void )
{
	gEngfuncs.pfnHookEvent( "events/null.sc",				EV_FireNull);
	gEngfuncs.pfnHookEvent( "events/crowbar.sc",				EV_FireCrowbar);
	gEngfuncs.pfnHookEvent( "events/tripfire.sc",				EV_PlayEmptySound );
	gEngfuncs.pfnHookEvent( "events/glock1.sc",				EV_FireGlock1 );
	gEngfuncs.pfnHookEvent( "events/shotgun1.sc",				EV_FireShotGunSingle );
	gEngfuncs.pfnHookEvent( "events/shotgun2.sc",				EV_FireShotGunDouble );
	gEngfuncs.pfnHookEvent( "events/mp5.sc",				EV_FireMP5 );
	gEngfuncs.pfnHookEvent( "events/mp52.sc",				EV_Explode );
	gEngfuncs.pfnHookEvent( "events/python.sc",				EV_FirePython );
	gEngfuncs.pfnHookEvent( "events/gauss.sc",				EV_FireGauss );
	gEngfuncs.pfnHookEvent( "events/gaussspin.sc",				EV_SpinGauss );
	gEngfuncs.pfnHookEvent( "events/egon_fire.sc",				EV_EgonFire );
	gEngfuncs.pfnHookEvent( "events/egon_stop.sc",				EV_EgonStop );
	gEngfuncs.pfnHookEvent( "events/train.sc",				EV_TrainPitchAdjust );
	gEngfuncs.pfnHookEvent( "events/snarkfire.sc",				EV_SnarkFire );
	gEngfuncs.pfnHookEvent( "events/glock2.sc",				EV_Decals );
}