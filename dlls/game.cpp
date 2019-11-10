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
#include "eiface.h"
#include "util.h"
#include "game.h"

//Macros to make skill cvars easier to define
#define DECLARE_SKILL_CVARS( name )					\
cvar_t	sk_##name##1 = { "sk_" #name "1", "0" };	\
cvar_t	sk_##name##2 = { "sk_" #name "2", "0" };	\
cvar_t	sk_##name##3 = { "sk_" #name "3", "0" }

#define REGISTER_SKILL_CVARS( name )	\
CVAR_REGISTER( &sk_##name##1 );			\
CVAR_REGISTER( &sk_##name##2 );			\
CVAR_REGISTER( &sk_##name##3 )

cvar_t	displaysoundlist = { "displaysoundlist","0" };

// Spectator settings
cvar_t	allow_spectators = { "allow_spectators","1", FCVAR_SERVER };
cvar_t	spectator_cmd_delay = { "spectator_cmd_delay","5" };

// multiplayer server rules
cvar_t	fragsleft = { "mp_fragsleft","0", FCVAR_SERVER | FCVAR_UNLOGGED };	  // Don't spam console/log files/users with this changing
cvar_t	timeleft = { "mp_timeleft","0" , FCVAR_SERVER | FCVAR_UNLOGGED };	  // "      "

// multiplayer server rules
cvar_t	teamplay = { "mp_teamplay","0", FCVAR_SERVER };
cvar_t	fraglimit = { "mp_fraglimit","0", FCVAR_SERVER };
cvar_t	timelimit = { "mp_timelimit","0", FCVAR_SERVER };
cvar_t	friendlyfire = { "mp_friendlyfire","0", FCVAR_SERVER };
cvar_t	falldamage = { "mp_falldamage","0", FCVAR_SERVER };
cvar_t	weaponstay = { "mp_weaponstay","0", FCVAR_SERVER };
cvar_t	forcerespawn = { "mp_forcerespawn","1", FCVAR_SERVER };
cvar_t	flashlight = { "mp_flashlight","0", FCVAR_SERVER };
cvar_t	aimcrosshair = { "mp_autocrosshair","1", FCVAR_SERVER };
cvar_t	decalfrequency = { "decalfrequency","30", FCVAR_SERVER };
cvar_t	teamlist = { "mp_teamlist","hgrunt;scientist", FCVAR_SERVER };
cvar_t	teamoverride = { "mp_teamoverride","1" };
cvar_t	defaultteam = { "mp_defaultteam","0" };
cvar_t	allowmonsters = { "mp_allowmonsters","0", FCVAR_SERVER };
cvar_t	bhopcap = { "mp_bhopcap", "1", FCVAR_SERVER };

cvar_t	mp_dmg_crowbar = { "mp_dmg_crowbar", "25", FCVAR_SERVER };
cvar_t	mp_dmg_glock = { "mp_dmg_glock", "12", FCVAR_SERVER };
cvar_t	mp_dmg_357 = { "mp_dmg_357", "40", FCVAR_SERVER };
cvar_t	mp_dmg_mp5 = { "mp_dmg_mp5", "12", FCVAR_SERVER };
cvar_t	mp_dmg_shotgun = { "mp_dmg_shotgun", "20", FCVAR_SERVER };
cvar_t	mp_dmg_xbow_scope = { "mp_dmg_xbow_scope", "120", FCVAR_SERVER };
cvar_t	mp_dmg_xbow_noscope = { "mp_dmg_xbow_noscope", "40", FCVAR_SERVER };
cvar_t	mp_dmg_rpg = { "mp_dmg_rpg", "120", FCVAR_SERVER };
cvar_t	mp_dmg_gauss_primary = { "mp_dmg_gauss_primary", "20", FCVAR_SERVER };
cvar_t	mp_dmg_gauss_secondary = { "mp_dmg_gauss_secondary", "200", FCVAR_SERVER };
cvar_t	mp_dmg_egon = { "mp_dmg_egon", "20", FCVAR_SERVER };
cvar_t	mp_dmg_hornet = { "mp_dmg_hornet", "10", FCVAR_SERVER };
cvar_t	mp_dmg_hgrenade = { "mp_dmg_hgrenade", "100", FCVAR_SERVER };
cvar_t	mp_dmg_satchel = { "mp_dmg_satchel", "120", FCVAR_SERVER };
cvar_t	mp_dmg_tripmine = { "mp_dmg_tripmine", "150", FCVAR_SERVER };
cvar_t	mp_dmg_m203 = { "mp_dmg_m203", "100", FCVAR_SERVER };

cvar_t	impulsetarget = { "sohl_impulsetarget","0", FCVAR_SERVER }; //LRC - trigger ents manually
cvar_t	mw_debug = { "sohl_mwdebug","0", FCVAR_SERVER }; //LRC - debug info. for MoveWith. (probably not useful for most people.)

cvar_t  mp_chattime = { "mp_chattime","10", FCVAR_SERVER };
cvar_t  mp_notify_player_status = { "mp_notify_player_status","7" };	// Notifications about join/leave/spectate

cvar_t	mp_welcomecam = { "mp_welcomecam", "1", FCVAR_SERVER };

cvar_t  motdfile_unicode = { "motdfile_unicode", "motd_unicode.txt", FCVAR_SERVER };
cvar_t  motdfile_html = { "motdfile_html", "motd.html", FCVAR_SERVER };

// Engine Cvars
cvar_t 	*g_psv_gravity = NULL;
cvar_t	*g_psv_aim = NULL;
cvar_t	*g_footsteps = NULL;

//CVARS FOR SKILL LEVEL SETTINGS

// Agrunt
DECLARE_SKILL_CVARS(agrunt_health);
DECLARE_SKILL_CVARS(agrunt_dmg_punch);

// Apache
DECLARE_SKILL_CVARS(apache_health);

// Barney
DECLARE_SKILL_CVARS(barney_health);

// Bullsquid
DECLARE_SKILL_CVARS(bullsquid_health);
DECLARE_SKILL_CVARS(bullsquid_dmg_bite);
DECLARE_SKILL_CVARS(bullsquid_dmg_whip);
DECLARE_SKILL_CVARS(bullsquid_dmg_spit);

// Big Momma
DECLARE_SKILL_CVARS(bigmomma_health_factor);
DECLARE_SKILL_CVARS(bigmomma_dmg_slash);
DECLARE_SKILL_CVARS(bigmomma_dmg_blast);
DECLARE_SKILL_CVARS(bigmomma_radius_blast);

// Gargantua
DECLARE_SKILL_CVARS(gargantua_health);
DECLARE_SKILL_CVARS(gargantua_dmg_slash);
DECLARE_SKILL_CVARS(gargantua_dmg_fire);
DECLARE_SKILL_CVARS(gargantua_dmg_stomp);

// Hassassin
DECLARE_SKILL_CVARS(hassassin_health);

// Headcrab
DECLARE_SKILL_CVARS(headcrab_health);
DECLARE_SKILL_CVARS(headcrab_dmg_bite);

// Hgrunt 
DECLARE_SKILL_CVARS(hgrunt_health);
DECLARE_SKILL_CVARS(hgrunt_kick);
DECLARE_SKILL_CVARS(hgrunt_pellets);
DECLARE_SKILL_CVARS(hgrunt_gspeed);

// Houndeye
DECLARE_SKILL_CVARS(houndeye_health);
DECLARE_SKILL_CVARS(houndeye_dmg_blast);

// ISlave
DECLARE_SKILL_CVARS(islave_health);
DECLARE_SKILL_CVARS(islave_dmg_claw);
DECLARE_SKILL_CVARS(islave_dmg_clawrake);
DECLARE_SKILL_CVARS(islave_dmg_zap);

// Icthyosaur
DECLARE_SKILL_CVARS(ichthyosaur_health);
DECLARE_SKILL_CVARS(ichthyosaur_shake);

// Leech
DECLARE_SKILL_CVARS(leech_health);
DECLARE_SKILL_CVARS(leech_dmg_bite);

// Controller
DECLARE_SKILL_CVARS(controller_health);
DECLARE_SKILL_CVARS(controller_dmgzap);
DECLARE_SKILL_CVARS(controller_speedball);
DECLARE_SKILL_CVARS(controller_dmgball);

// Nihilanth
DECLARE_SKILL_CVARS(nihilanth_health);
DECLARE_SKILL_CVARS(nihilanth_zap);

// Scientist
DECLARE_SKILL_CVARS(scientist_health);

// Snark
DECLARE_SKILL_CVARS(snark_health);
DECLARE_SKILL_CVARS(snark_dmg_bite);
DECLARE_SKILL_CVARS(snark_dmg_pop);

// Zombie Scientist
DECLARE_SKILL_CVARS(zombie_health);
DECLARE_SKILL_CVARS(zombie_dmg_one_slash);
DECLARE_SKILL_CVARS(zombie_dmg_both_slash);
DECLARE_SKILL_CVARS(zombie_head);
DECLARE_SKILL_CVARS(zombie_chest);
DECLARE_SKILL_CVARS(zombie_stomach);
DECLARE_SKILL_CVARS(zombie_leg);
DECLARE_SKILL_CVARS(zombie_arm);

//Turret
DECLARE_SKILL_CVARS(turret_health);

// MiniTurret
DECLARE_SKILL_CVARS(miniturret_health);

// Sentry Turret
DECLARE_SKILL_CVARS(sentry_health);

// PLAYER WEAPONS

// Crowbar whack
DECLARE_SKILL_CVARS(plr_crowbar);

// Glock Round
DECLARE_SKILL_CVARS(plr_9mm_bullet);

// 357 Round
DECLARE_SKILL_CVARS(plr_357_bullet);

// MP5 Round
DECLARE_SKILL_CVARS(plr_9mmAR_bullet);

// M203 grenade
DECLARE_SKILL_CVARS(plr_9mmAR_grenade);

// Shotgun buckshot
DECLARE_SKILL_CVARS(plr_buckshot);

// Crossbow
DECLARE_SKILL_CVARS(plr_xbow_bolt_client);
DECLARE_SKILL_CVARS(plr_xbow_bolt_monster);

// RPG
DECLARE_SKILL_CVARS(plr_rpg);

// Zero Point Generator
DECLARE_SKILL_CVARS(plr_gauss);

// Tau Cannon
DECLARE_SKILL_CVARS(plr_egon_narrow);
DECLARE_SKILL_CVARS(plr_egon_wide);

// Hand Grendade
DECLARE_SKILL_CVARS(plr_hand_grenade);

// Satchel Charge
DECLARE_SKILL_CVARS(plr_satchel);

// Tripmine
DECLARE_SKILL_CVARS(plr_tripmine);

// WORLD WEAPONS
DECLARE_SKILL_CVARS(12mm_bullet);
DECLARE_SKILL_CVARS(9mmAR_bullet);
DECLARE_SKILL_CVARS(9mm_bullet);

// HORNET
DECLARE_SKILL_CVARS(hornet_dmg);

// HEALTH/CHARGE
DECLARE_SKILL_CVARS(suitcharger);
DECLARE_SKILL_CVARS(battery);
DECLARE_SKILL_CVARS(healthcharger);
DECLARE_SKILL_CVARS(healthkit);
DECLARE_SKILL_CVARS(scientist_heal);

// FLASHLIGHT CHARGE VALUE
DECLARE_SKILL_CVARS(flashcharge);

// monster damage adjusters
DECLARE_SKILL_CVARS(monster_head);
DECLARE_SKILL_CVARS(monster_chest);
DECLARE_SKILL_CVARS(monster_stomach);
DECLARE_SKILL_CVARS(monster_arm);
DECLARE_SKILL_CVARS(monster_leg);

// player damage adjusters
DECLARE_SKILL_CVARS(player_head);
DECLARE_SKILL_CVARS(player_chest);
DECLARE_SKILL_CVARS(player_stomach);
DECLARE_SKILL_CVARS(player_arm);
DECLARE_SKILL_CVARS(player_leg);

// END Cvars for Skill Level settings

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit(void)
{
	// Register cvars here:

	g_psv_gravity = CVAR_GET_POINTER("sv_gravity");
	g_psv_aim = CVAR_GET_POINTER("sv_aim");
	g_footsteps = CVAR_GET_POINTER("mp_footsteps");

	CVAR_REGISTER(&displaysoundlist);

	CVAR_REGISTER(&teamplay);
	CVAR_REGISTER(&fraglimit);
	CVAR_REGISTER(&timelimit);

	CVAR_REGISTER(&fragsleft);
	CVAR_REGISTER(&timeleft);

	CVAR_REGISTER(&friendlyfire);
	CVAR_REGISTER(&falldamage);
	CVAR_REGISTER(&weaponstay);
	CVAR_REGISTER(&forcerespawn);
	CVAR_REGISTER(&flashlight);
	CVAR_REGISTER(&aimcrosshair);
	CVAR_REGISTER(&decalfrequency);
	CVAR_REGISTER(&teamlist);
	CVAR_REGISTER(&teamoverride);
	CVAR_REGISTER(&defaultteam);
	CVAR_REGISTER(&allowmonsters);
	CVAR_REGISTER(&bhopcap);
	CVAR_REGISTER(&impulsetarget); //LRC
	CVAR_REGISTER(&mw_debug); //LRC
	CVAR_REGISTER(&mp_chattime);
	CVAR_REGISTER(&mp_notify_player_status);
	CVAR_REGISTER(&mp_welcomecam);
	CVAR_REGISTER(&motdfile_unicode);
	CVAR_REGISTER(&motdfile_html);

	CVAR_REGISTER(&mp_dmg_crowbar);
	CVAR_REGISTER(&mp_dmg_glock);
	CVAR_REGISTER(&mp_dmg_357);
	CVAR_REGISTER(&mp_dmg_mp5);
	CVAR_REGISTER(&mp_dmg_shotgun);
	CVAR_REGISTER(&mp_dmg_xbow_scope);
	CVAR_REGISTER(&mp_dmg_xbow_noscope);
	CVAR_REGISTER(&mp_dmg_rpg);
	CVAR_REGISTER(&mp_dmg_gauss_primary);
	CVAR_REGISTER(&mp_dmg_gauss_secondary);
	CVAR_REGISTER(&mp_dmg_egon);
	CVAR_REGISTER(&mp_dmg_hornet);
	CVAR_REGISTER(&mp_dmg_hgrenade);
	CVAR_REGISTER(&mp_dmg_satchel);
	CVAR_REGISTER(&mp_dmg_tripmine);
	CVAR_REGISTER(&mp_dmg_m203);

	// REGISTER CVARS FOR SKILL LEVEL STUFF

	// Agrunt
	REGISTER_SKILL_CVARS(agrunt_health);
	REGISTER_SKILL_CVARS(agrunt_dmg_punch);

	// Apache
	REGISTER_SKILL_CVARS(apache_health);

	// Barney
	REGISTER_SKILL_CVARS(barney_health);

	// Bullsquid
	REGISTER_SKILL_CVARS(bullsquid_health);
	REGISTER_SKILL_CVARS(bullsquid_dmg_bite);
	REGISTER_SKILL_CVARS(bullsquid_dmg_whip);
	REGISTER_SKILL_CVARS(bullsquid_dmg_spit);

	// Bigmomma
	REGISTER_SKILL_CVARS(bigmomma_health_factor);
	REGISTER_SKILL_CVARS(bigmomma_dmg_slash);
	REGISTER_SKILL_CVARS(bigmomma_dmg_blast);
	REGISTER_SKILL_CVARS(bigmomma_radius_blast);

	// Gargantua
	REGISTER_SKILL_CVARS(gargantua_health);
	REGISTER_SKILL_CVARS(gargantua_dmg_slash);
	REGISTER_SKILL_CVARS(gargantua_dmg_fire);
	REGISTER_SKILL_CVARS(gargantua_dmg_stomp);

	// Hassassin
	REGISTER_SKILL_CVARS(hassassin_health);

	// Headcrab
	REGISTER_SKILL_CVARS(headcrab_health);
	REGISTER_SKILL_CVARS(headcrab_dmg_bite);

	// Hgrunt
	REGISTER_SKILL_CVARS(hgrunt_health);
	REGISTER_SKILL_CVARS(hgrunt_kick);
	REGISTER_SKILL_CVARS(hgrunt_pellets);
	REGISTER_SKILL_CVARS(hgrunt_gspeed);

	// Houndeye
	REGISTER_SKILL_CVARS(houndeye_health);
	REGISTER_SKILL_CVARS(houndeye_dmg_blast);

	// ISlave
	REGISTER_SKILL_CVARS(islave_health);
	REGISTER_SKILL_CVARS(islave_dmg_claw);
	REGISTER_SKILL_CVARS(islave_dmg_clawrake);
	REGISTER_SKILL_CVARS(islave_dmg_zap);

	// Icthyosaur
	REGISTER_SKILL_CVARS(ichthyosaur_health);
	REGISTER_SKILL_CVARS(ichthyosaur_shake);

	// Leech
	REGISTER_SKILL_CVARS(leech_health);
	REGISTER_SKILL_CVARS(leech_dmg_bite);

	// Controller
	REGISTER_SKILL_CVARS(controller_health);
	REGISTER_SKILL_CVARS(controller_dmgzap);
	REGISTER_SKILL_CVARS(controller_speedball);
	REGISTER_SKILL_CVARS(controller_dmgball);

	// Nihilanth
	REGISTER_SKILL_CVARS(nihilanth_health);
	REGISTER_SKILL_CVARS(nihilanth_zap);

	// Scientist
	REGISTER_SKILL_CVARS(scientist_health);

	// Snark
	REGISTER_SKILL_CVARS(snark_health);
	REGISTER_SKILL_CVARS(snark_dmg_bite);
	REGISTER_SKILL_CVARS(snark_dmg_pop);

	// Zombie Scientist
	REGISTER_SKILL_CVARS(zombie_health);
	REGISTER_SKILL_CVARS(zombie_dmg_one_slash);
	REGISTER_SKILL_CVARS(zombie_dmg_both_slash);
	REGISTER_SKILL_CVARS(zombie_head);
	REGISTER_SKILL_CVARS(zombie_chest);
	REGISTER_SKILL_CVARS(zombie_stomach);
	REGISTER_SKILL_CVARS(zombie_leg);
	REGISTER_SKILL_CVARS(zombie_arm);

	//Turret
	REGISTER_SKILL_CVARS(turret_health);

	// MiniTurret
	REGISTER_SKILL_CVARS(miniturret_health);

	// Sentry Turret
	REGISTER_SKILL_CVARS(sentry_health);

	// PLAYER WEAPONS

	// Crowbar whack
	REGISTER_SKILL_CVARS(plr_crowbar);

	// Glock Round
	REGISTER_SKILL_CVARS(plr_9mm_bullet);

	// 357 Round
	REGISTER_SKILL_CVARS(plr_357_bullet);

	// MP5 Round
	REGISTER_SKILL_CVARS(plr_9mmAR_bullet);

	// M203 grenade
	REGISTER_SKILL_CVARS(plr_9mmAR_grenade);

	// Shotgun buckshot
	REGISTER_SKILL_CVARS(plr_buckshot);

	// Crossbow
	REGISTER_SKILL_CVARS(plr_xbow_bolt_monster);
	REGISTER_SKILL_CVARS(plr_xbow_bolt_client);

	// RPG
	REGISTER_SKILL_CVARS(plr_rpg);

	// Gauss Gun
	REGISTER_SKILL_CVARS(plr_gauss);

	// Egon Gun
	REGISTER_SKILL_CVARS(plr_egon_narrow);
	REGISTER_SKILL_CVARS(plr_egon_wide);

	// Hand Grendade
	REGISTER_SKILL_CVARS(plr_hand_grenade);

	// Satchel Charge
	REGISTER_SKILL_CVARS(plr_satchel);

	// Tripmine
	REGISTER_SKILL_CVARS(plr_tripmine);

	// WORLD WEAPONS
	REGISTER_SKILL_CVARS(12mm_bullet);
	REGISTER_SKILL_CVARS(9mmAR_bullet);
	REGISTER_SKILL_CVARS(9mm_bullet);

	// HORNET
	REGISTER_SKILL_CVARS(hornet_dmg);

	// HEALTH/SUIT CHARGE DISTRIBUTION
	REGISTER_SKILL_CVARS(suitcharger);
	REGISTER_SKILL_CVARS(battery);
	REGISTER_SKILL_CVARS(healthcharger);
	REGISTER_SKILL_CVARS(healthkit);
	REGISTER_SKILL_CVARS(scientist_heal);
	REGISTER_SKILL_CVARS(flashcharge);

	// monster damage adjusters
	REGISTER_SKILL_CVARS(monster_head);
	REGISTER_SKILL_CVARS(monster_chest);
	REGISTER_SKILL_CVARS(monster_stomach);
	REGISTER_SKILL_CVARS(monster_arm);
	REGISTER_SKILL_CVARS(monster_leg);

	// player damage adjusters
	REGISTER_SKILL_CVARS(player_head);
	REGISTER_SKILL_CVARS(player_chest);
	REGISTER_SKILL_CVARS(player_stomach);
	REGISTER_SKILL_CVARS(player_arm);
	REGISTER_SKILL_CVARS(player_leg);

	// END REGISTER CVARS FOR SKILL LEVEL STUFF
	SERVER_COMMAND("exec skill.cfg\n");
	SERVER_COMMAND("exec skill_opfor.cfg\n"); // Opposing-Force
	SERVER_COMMAND("exec skill_hitgroups.cfg\n"); // Hitgroups
}