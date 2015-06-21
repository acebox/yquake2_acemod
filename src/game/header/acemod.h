/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * AceMod: additional global header, globally-referenced stuff should go here.
 *
 * =======================================================================
 */

#ifndef ACE_LOCAL_H
#define ACE_LOCAL_H

// general defines
#ifdef Z_MAX
#undef Z_MAX
#endif // Z_MAX
#define Z_MAX(a,b)	((a) > (b) ? (a) : (b))

#ifdef Z_MIN
#undef Z_MIN
#endif // Z_MIN
#define Z_MIN(a,b)	((a) < (b) ? (a) : (b))

#define Z_MALLOC(size)	gi.TagMalloc(size, TAG_GAME)
#define Z_FREE(block)	gi.TagFree(block)

/* ace - this snipped from the site below, is for helping generate "Better" random numbers (float)
 *	from http://stackoverflow.com/questions/13408990/how-to-generate-random-float-number-in-c 
 */
#define ACE_RANDOM ((float)rand()/(float)(RAND_MAX)) * 1.0

// rogue defines
// #define min(a, b) ((a) < (b) ? (a) : (b))
// #define max(a, b) ((a) > (b) ? (a) : (b))
#define _isnan(a) (isnan(a))

// entity flags
#define FL_MECHANICAL 0x00002000		/* entity is mechanical, use sparks not blood */
#define FL_SAM_RAIMI 0x00004000		/* entity is in sam raimi cam mode */
#define FL_DISGUISED 0x00008000		/* entity is in disguise, monsters will not recognize.	*/
#define FL_RMONSTER 0x00010000		/* ace - this is for the random monster spawn system,	*
										 * enemies spawned with this flag will get converted	*
										 * to a spawn spot, class tallied.						*/
#define FL_NOGIB 0x00010000		/* vaporized, drop no gibs */

// entity spawnflags
#define CHECK_BACK_WALL 1

// gameplay defines
// max ammo defines
#define MAX_BULLETS_NORM				200		// player start
#define MAX_BULLETS_BAND				300		// max bandolier will increase to
#define INC_BULLETS_BAND				50		// each bandolier will increase max to
#define MAX_BULLETS_PACK				400		// max backpack will increase to
#define INC_BULLETS_PACK				100		// each backpack will increase max by

#define MAX_SHELLS_NORM				50		// player start
#define MAX_SHELLS_BAND				100		// max bandolier will increase to
#define INC_SHELLS_BAND				25		// each bandolier will increase max to
#define MAX_SHELLS_PACK				150		// max backpack will increase to
#define INC_SHELLS_PACK				50		// each backpack will increase max by

#define MAX_ROCKETS_NORM				10		// player start
#define MAX_ROCKETS_PACK				50		// max backpack will increase to
#define INC_ROCKETS_PACK				10		// each backpack will increase max by

#define MAX_GRENADES_NORM				10		// player start
#define MAX_GRENADES_PACK				50		// max backpack will increase to
#define INC_GRENADES_PACK				10		// each backpack will increase max by

#define MAX_CELLS_NORM				200		// player start
#define MAX_CELLS_BAND				300		// max bandolier will increase to
#define INC_CELLS_BAND				25		// each bandolier will increase max to
#define MAX_CELLS_PACK				400		// max backpack will increase to
#define INC_CELLS_PACK				50		// each backpack will increase max by

#define MAX_SLUGS_NORM				50		// player start
#define MAX_SLUGS_BAND				100		// max bandolier will increase to
#define INC_SLUGS_BAND				25		// each bandolier will increase max to
#define MAX_SLUGS_PACK				150		// max backpack will increase to
#define INC_SLUGS_PACK				25		// each backpack will increase max by

#define MAX_MAGSLUG_NORM				25		// player start
#define MAX_MAGSLUG_BAND				50		// max bandolier will increase to
#define INC_MAGSLUG_BAND				5		// each bandolier will increase max to
#define MAX_MAGSLUG_PACK				75		// max backpack will increase to
#define INC_MAGSLUG_PACK				5		// each backpack will increase max by

#define MAX_TESLA_NORM				10		// player start
#define MAX_TESLA_PACK				50		// max backpack will increase to
#define INC_TESLA_PACK				10		// each backpack will increase max by

#define MAX_PROX_NORM				10		// player start
#define MAX_PROX_PACK				50		// max backpack will increase to
#define INC_PROX_PACK				10		// each backpack will increase max by

#define MAX_TRAP_NORM				5		// player start
#define MAX_TRAP_PACK				200		// max backpack will increase to
#define INC_TRAP_PACK				200		// each backpack will increase max by

#define MAX_FLECHETTES_NORM				200		// player start
#define MAX_FLECHETTES_BAND				300		// max bandolier will increase to
#define INC_FLECHETTES_BAND				25		// each bandolier will increase max to
#define MAX_FLECHETTES_PACK				400		// max backpack will increase to
#define INC_FLECHETTES_PACK				25		// each backpack will increase max by

// ace - max skill level, used some places
#define MAX_SKILL_LEVEL	7
/* ace - corpse decay delay, after this delay allow normal corpse gibbing. */
// #define CORPSE_DECAY	30

/* ace - maximum number of gibs/debris allowed per-frame - vanilla yq2 is 20, this can still rain */
#define MAX_GIBS_PER_FRAME	20		// shouldn't be too excessive
#define MAX_DEBRIS_PER_FRAME	40		// should be better incase a map has a large building/etc..
#define MAX_SPRAYS_PER_FRAME	3		// this should help with overflow/no free ents

// knockback minimum mass
#define MIN_KNOCKBACK_MASS	100

#define STATE_TOP 0
#define STATE_BOTTOM 1
#define STATE_UP 2
#define STATE_DOWN 3

#define HINT_ENDPOINT 0x0001
#define MAX_HINT_CHAINS 100

#define TESLA_DAMAGE_RADIUS 128

#define Z_RADUISLISTSIZE	2000

/* (rogue) this determines how long to wait after a duck to duck again.
   this needs to be longer than the time after the monster_duck_up
   in all of the animation sequences */
#define DUCK_INTERVAL 0.5

/* (rogue) this is for the count of monsters */
#define ENT_SLOTS_LEFT \
	(ent->monsterinfo.monster_slots - \
	 ent->monsterinfo.monster_used)
#define SELF_SLOTS_LEFT \
	(self->monsterinfo.monster_slots - \
	 self->monsterinfo.monster_used)

// monsterinfo aiflags AI_

/* ROGUE */
#define AI_WALK_WALLS 0x00008000
#define AI_MANUAL_STEERING 0x00010000
#define AI_TARGET_ANGER 0x00020000
#define AI_DODGING 0x00040000
#define AI_CHARGING 0x00080000
#define AI_HINT_PATH 0x00100000
#define AI_IGNORE_SHOTS 0x00200000
#define AI_DO_NOT_COUNT 0x00400000          /* set for healed monsters */
/*	ace - use AI_DO_NOT_COUNT to indicate:
	- monster obviously does not count towards kills.
	- resurrected by a  medic type
	- summoned in by a carrier, medic commander, black widow, or other means.
	- yeah, free up some aiflags.
*/
//#define AI_SPAWNED_CARRIER 0x00800000       /* both do_not_count and spawned are set for spawned monsters */
//#define AI_SPAWNED_MEDIC_C 0x01000000       /* both do_not_count and spawned are set for spawned monsters */
//#define AI_SPAWNED_WIDOW 0x02000000         /* both do_not_count and spawned are set for spawned monsters */
//#define AI_SPAWNED_MASK 0x03800000          /* mask to catch all three flavors of spawned */
#define AI_BLOCKED 0x04000000               /* used by blocked_checkattack: set to say I'm attacking while blocked */
	                                            /* (prevents run-attacks) */
/* ZAERO */
#define AI_SCHOOLING  			0x08000000 // 0x00008000
// #define AI_REDUCEDDAMAGE			0x10000000 // 0x00010000
#define AI_SCHOOLINGTURNING		0x20000000 // 0x00020000
#define AI_SCHOOLINGTURNINGFAST	0x40000000 // 0x00040000
#define AI_DODGETIMEOUT			0x80000000 // 0x00080000
#define AI_JUMPING					0x10000000 // 0x00100000
// #define AI_MONREDUCEDDAMAGE		0x08000000 // 0x00200000
// #define AI_ONESHOTTARGET			0x08000000 // 0x00400000

// monsterinfo attack states
#define AS_BLIND			5	// ace - rogue extra
#define AS_FLY_STRAFE		6	// ace - zaero extra, might come in handy with flying ai.

// item IT_ flags
#define IT_TECH 64 // for ctf tech item definition
#define IT_MELEE 0x00000040
#define IT_NOT_GIVEABLE 0x00000080      /* item can not be given */

// damage flags
#define DAMAGE_DESTROY_ARMOR 0x00000040     /* damage is done to armor and health. */
#define DAMAGE_NO_REG_ARMOR 0x00000080      /* damage skips regular armor */
#define DAMAGE_NO_POWER_ARMOR 0x00000100    /* damage skips power armor */

// means of death (34+ goes here)
#define MOD_SNIPERRIFLE		34
#define MOD_TRIPBOMB			35
#define MOD_GRAPPLE			36
#define MOD_SONICCANNON		37
#define MOD_RIPPER				38
#define MOD_PHALANX			39
#define MOD_TRAP				40
#define MOD_CHAINFIST			41
#define MOD_DISINTEGRATOR		42
#define MOD_ETF_RIFLE			43
#define MOD_HEATBEAM			44
#define MOD_TESLA				45
#define MOD_PROX				46
#define MOD_TRACKER			47
#define MOD_NUKE				48
#define MOD_BLASTER2			49

// for external weapon models (multiplayer/vweap)
#define WEAP_GRAPPLE			12
#define WEAP_PHALANX			13
#define WEAP_BOOMER			14
#define WEAP_DISRUPTOR		15
#define WEAP_ETFRIFLE			16
#define WEAP_PLASMA			17
#define WEAP_PROXLAUNCH		18
#define WEAP_CHAINFIST		19

// weapon defines
#define FRAME_FIRE_FIRST (FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST (FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST (FRAME_IDLE_LAST + 1)

#define GRENADE_TIMER			3.0
#define GRENADE_MINSPEED		400
#define GRENADE_MAXSPEED		800

#define CHAINGUN_WINDUP		10

// zaero weapons
#define SC_MAXFIRETIME	5         // in seconds...
#define SC_BASEDAMAGE		10        // minimum damage
#define SC_DAMGERANGE		990       // maximum damaged range (max damage possible is SC_BASEDAMAGE + SC_DAMGERANGE)
#define SC_MAXRADIUS		500       // maximum blast radius
#define SC_MAXCELLS		100       // maximum number of cells

#define MAXROTATION 20

// variables
#define TBOMB_DELAY	1.0
#define TBOMB_TIMEOUT	180
#define TBOMB_DAMAGE 150
#define TBOMB_RADIUS_DAMAGE 384
#define TBOMB_HEALTH 100
#define TBOMB_SHRAPNEL	15
#define TBOMB_SHRAPNEL_DMG	15
#define TBOMB_MAX_EXIST	50

#define SNIPER_CHARGE_TIME	30
// end zaero weapons

// rogue weapons
#define NUKE_DELAY 4
#define NUKE_TIME_TO_LIVE 6
#define NUKE_RADIUS 512
#define NUKE_DAMAGE 400
#define NUKE_QUAKE_TIME 3
#define NUKE_QUAKE_STRENGTH 100

#define PROX_TIME_TO_LIVE 45
#define PROX_TIME_DELAY 0.5
#define PROX_BOUND_SIZE 96
#define PROX_DAMAGE_RADIUS 192
#define PROX_HEALTH 20
#define PROX_DAMAGE 90

#define TESLA_TIME_TO_LIVE 30
#define TESLA_DAMAGE_RADIUS 128
#define TESLA_DAMAGE 3
#define TESLA_KNOCKBACK 8
#define TESLA_ACTIVATE_TIME 3
#define TESLA_EXPLOSION_DAMAGE_MULT 50
#define TESLA_EXPLOSION_RADIUS 200

#define TRACKER_DAMAGE_FLAGS (DAMAGE_NO_POWER_ARMOR | DAMAGE_ENERGY | DAMAGE_NO_KNOCKBACK)
#define TRACKER_IMPACT_FLAGS (DAMAGE_NO_POWER_ARMOR | DAMAGE_ENERGY)
#define TRACKER_DAMAGE_TIME 0.5
// end rogue weapons

// "ctf" grapple
#define GRAPPLE_SPEED			650		/* speed of grapple in flight */
#define GRAPPLE_PULL_SPEED	650		/* speed player is pulled at */
#define GRAPPLE_HANG_DIST		64

typedef enum
{
	GRAPPLE_STATE_FLY,
	GRAPPLE_STATE_PULL,
	GRAPPLE_STATE_HANG
} grapplestate_t;
// end grapple

// chainfist
#define CHAINFIST_REACH		64
#define CHAINFIST_SP_DMG		15
#define CHAINFIST_DM_DMG		30

// heatbeam/plasmabeam
#define HEATBEAM_DM_DMG		15
#define HEATBEAM_SP_DMG		15

// extra dmflags
#define DF_ARMOR_PROTECT 262144
#define DF_NO_TECH 524288

// "CTF" Techs
#define TECH_TIMEOUT 60		/* seconds before techs spawn again */

/* 
	ace - monster randomizer v3x, for per-level monster randomization.
		- v3x changes: awitch to array (and use less entities)
*/
#define ACE_MRANDOMIZER

#define ACE_MSPAWN_DELAY		0
#define ACE_CHANCE_INSANE		0.1

// spawn timers (seconds)
#define ACE_SPAWNTIME_T1		15
#define ACE_SPAWNTIME_T2		60
#define ACE_SPAWNTIME_T3		120
#define ACE_SPAWNTIME_T4		300

// base spawn counts
#define ACE_MBASE_T1			10
#define ACE_MBASE_T2			5
#define ACE_MBASE_T3			3
#define ACE_MBASE_T4			1

/*	quick way of setting spawn chances, untill I can do something better lol.
	basically, using random() 0.00 -> 1.00, starting with the lowest going up.
	MONSTER_SOLDIER_LIGHT being the last/default spawn choice. since it's the
	same random instance, this would divide things up.
	
	example:
	ACE_CHANCE_GUNNER				0.25
	ACE_CHANCE_INFANTRY				0.50
	ACE_CHANCE_CHANCE_SOLDIER_LIGHT 1.00

	here, this works out to first 25% chance for gunner, then from 25-50% 
	(basically the next 25%) for the infantry, then the remaining 50% for
	light soldiers.  or sumthin like that.
	
	UPDATES/NOTES:
	-re-ordered enemies and added in medic commander and stalker
	-flipper is included by default anywhere there is a underwater spawn 
		-based on origin, might need to add in a check (make sure its X units under water/etc)
	-currently bosses/tanks/etc.. are not included, they spawn normally.
	-this works on map spawns, but not implemented in triggered spawn.
	-might work something based on what the original spawn type is (for sizing new spawns)
*/
// #define ACE_CHANCE_			0.05
#define ACE_CHANCE_MEDIC_COMMANDER	0.01
#define ACE_CHANCE_MEDIC				0.03
#define ACE_CHANCE_MUTANT				0.05
#define ACE_CHANCE_GEKK				0.07
#define ACE_CHANCE_STALKER			0.1
#define ACE_CHANCE_GUNNER				0.15
#define ACE_CHANCE_SOLDIER_LASER		0.2
#define ACE_CHANCE_SOLDIER_RIPPER	0.25
#define ACE_CHANCE_SOLDIER_HYPER		0.3
#define ACE_CHANCE_BERSERK			0.35
#define ACE_CHANCE_HANDLER			0.4
#define ACE_CHANCE_SOLDIER_SS			0.45
#define ACE_CHANCE_HOUND				0.5
#define ACE_CHANCE_INFANTRY			0.65
#define ACE_CHANCE_SOLDIER			0.75
// #define ACE_CHANCE_SOLDIER_LIGHT		1.00

// monstertype_t, for monster array
typedef enum
{
	MONSTER_SOLDIER_LIGHT,
	MONSTER_SOLDIER,
	MONSTER_SOLDIER_SS,
	MONSTER_SOLDIER_HYPERGUN,
	MONSTER_SOLDIER_LASERGUN,
	MONSTER_SOLDIER_RIPPER,
	MONSTER_STALKER,
	MONSTER_INFANTRY,
	MONSTER_HANDLER,
	MONSTER_GEKK,
	MONSTER_FLIPPER,
	MONSTER_PARASITE,
	MONSTER_HOUND,
	MONSTER_FLYER,
	MONSTER_FLOATER,
	MONSTER_HOVER,
	MONSTER_KAMIKAZE,
	MONSTER_DAEDALUS,
	MONSTER_BERSERK,
	MONSTER_GUNNER,
	MONSTER_CHICK,
	MONSTER_CHICK_HEAT,
	MONSTER_BRAIN,
	MONSTER_FIXBOT,
	MONSTER_GLADIATOR,
	MONSTER_GLADB,
	MONSTER_MEDIC,
	MONSTER_MEDIC_COMMANDER,
	MONSTER_MUTANT,
	MONSTER_TANK,
	MONSTER_TANK_COMMANDER,
	MONSTER_SUPERTANK,
	MONSTER_CARRIER,
	MONSTER_BOSS2,
	MONSTER_WIDOW,
	MONSTER_WIDOW2,
	MONSTER_BOSS5,
	MONSTER_ZBOSS,
	MONSTER_JORG,
	MONSTER_MAKRON,
	MONSTER_INSANE,
	MONSTER_MISC
} monstertype_t;
// #define MAX_MONSTERTYPES		40 // moved to local.h for level_local array
// ace - typedef moved to headers/local.h before level_locals_t

void ace_MonsterRandomizerGo (void);
// ace - monster randomizer end

// ace - monster AI defines start
#define SOLDIER_JUMP_DN_BASE	128
// ace - monster AI defines end

// cvars
extern cvar_t *sv_allow_grapple;	// "ctf" modded grappling hook is enabled server-side
extern cvar_t *sv_allow_push;		// zaero "offhand" push is enabled server-side
extern cvar_t *sv_spawn_chainfist;	// this should be a cheat, but allows all clients to spawn with a chainfist.
extern cvar_t *g_infighting;		// toggles monster infighting

// prototypes

// ace - (misc.c) utility functions
void G_ProjectSource2(vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t up, vec3_t result);
void P_ProjectSource2(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t up, vec3_t result);
void P_ProjectSource_Reverse (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
float vectoyaw2(vec3_t vec);
void vectoangles2(vec3_t vec, vec3_t angles);
edict_t *findradius2(edict_t *from, vec3_t org, float rad);
void Z_RadiusDamageVisible(edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod);
void T_RadiusDamagePosition (vec3_t origin, edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod);
qboolean v_visible(vec3_t start, /* float vh1, */ vec3_t where /*, float vh2 */);
qboolean v_infront(vec3_t start, vec3_t angles, vec3_t where);

// ace - (weapons.c) weapons
// CTF Grapple base w/ "Hooked 1.6 Swinging Physics" Mod
void Weapon_Grapple(edict_t *ent);
// *** Zaero prototypes ***
void Weapon_SniperRifle(edict_t *ent);
void Weapon_LaserTripBomb(edict_t *ent);
void Weapon_SonicCannon (edict_t *ent);
void Action_Push(edict_t *ent);
void Use_PlasmaShield (edict_t *ent, gitem_t *item);
// *** Xatrix ***
void Weapon_Ionripper(edict_t *ent);
void Weapon_Phalanx(edict_t *ent);
void Weapon_Trap(edict_t *ent);
// *** Rogue ***
void Weapon_ChainFist(edict_t *ent);
void Weapon_Disintegrator(edict_t *ent);
void Weapon_ETF_Rifle(edict_t *ent);
void Weapon_Heatbeam(edict_t *ent);
void Weapon_Prox(edict_t *ent);
void Weapon_Tesla(edict_t *ent);
void Weapon_ProxLauncher(edict_t *ent);

void fire_tracker(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, edict_t *enemy);

void fire_heat(edict_t *self, vec3_t start, vec3_t aimdir, vec3_t offset, int damage, int kick, qboolean monster);

void fire_blaster2(edict_t *self, vec3_t start, vec3_t dir, int damage,	int speed, int effect, qboolean hyper);
void monster_fire_blaster2(edict_t *self, vec3_t start, vec3_t dir, int damage,	int speed, int flashtype, int effect);
void monster_fire_tracker(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, edict_t *enemy, int flashtype);

// grapple-specific funcs
void PlayerResetGrapple(edict_t *ent);
void GrapplePull(edict_t *self);
void ResetGrapple(edict_t *self);

// other weaps
void fire_ionripper(edict_t *self, vec3_t start, vec3_t aimdir, int damage,	int speed, int effect);
void fire_seeker(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage);
void fire_blueblaster(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int effect);
void fire_plasma(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage);
void fire_trap(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, qboolean held);
void fire_tesla(edict_t *self, vec3_t start, vec3_t aimdir,	int damage_multiplier, int speed);
void fire_prox(edict_t *self, vec3_t start, vec3_t aimdir, int damage_multiplier, int speed);


// ace - (items.c) items
void HasTech(edict_t *who);
gitem_t *What_Tech(edict_t *ent);
qboolean Pickup_Tech(edict_t *ent, edict_t *other);
void Drop_Tech(edict_t *ent, gitem_t *item);
void DeadDropTech(edict_t *ent);
void RespawnTech(edict_t *ent);
void SetupTechSpawn(void); // might not need this
void ResetTech(void);
int ApplyResistance(edict_t *ent, int dmg);
qboolean HasStrength(edict_t *ent);
int ApplyStrength(edict_t *ent, int dmg);
qboolean ApplyStrengthSound(edict_t *ent);
qboolean ApplyHaste(edict_t *ent);
void ApplyHasteSound(edict_t *ent);
void ApplyRegeneration(edict_t *ent);
qboolean HasRegeneration(edict_t *ent);

// ace - (a_spawn.c) - misc spawning funcs
void ace_AddItems(void);

void ace_SpawnRandomizer (edict_t *ent);		// randomizes spawns
void ace_MonsterItemRandomizer (edict_t *ent);	// gives monsters random drops

void SP_monster_rspawnspot (edict_t *ent);

// multiplayer enemy health scaling.
void ace_EnemyScaleUp();
void ace_EnemyScaleDn();

// ace - rogue spawn prototypes
edict_t *CreateMonster(vec3_t origin, vec3_t angles, char *classname);
edict_t *CreateFlyMonster(vec3_t origin, vec3_t angles, vec3_t mins,
		vec3_t maxs, char *classname);
edict_t *CreateGroundMonster(vec3_t origin, vec3_t angles, vec3_t mins,
		vec3_t maxs, char *classname, int height);
qboolean FindSpawnPoint(vec3_t startpoint, vec3_t mins, vec3_t maxs,
		vec3_t spawnpoint, float maxMoveUp);
qboolean CheckSpawnPoint(vec3_t origin, vec3_t mins, vec3_t maxs);
qboolean CheckGroundSpawnPoint(vec3_t origin, vec3_t entMins, vec3_t entMaxs,
		float height, float gravity);
void DetermineBBox(char *classname, vec3_t mins, vec3_t maxs);
void SpawnGrow_Spawn(vec3_t startpos, int size);
void Widowlegs_Spawn(vec3_t startpos, vec3_t angles);

// ace - rogue spawns
void SP_xatrix_item(edict_t *self);
void SP_func_plat2(edict_t *ent);
void SP_func_door_secret2(edict_t *ent);
void SP_func_force_wall(edict_t *ent);
void SP_info_player_coop_lava(edict_t *self);
void SP_info_teleport_destination(edict_t *self);
void SP_trigger_teleport(edict_t *self);
void SP_trigger_disguise(edict_t *self);
void SP_monster_stalker(edict_t *self);
void SP_monster_turret(edict_t *self);
void SP_target_steam(edict_t *self);
void SP_target_anger(edict_t *self);
void SP_target_killplayers(edict_t *self);

void SP_target_blacklight(edict_t *self);
void SP_target_orb(edict_t *self);

void SP_hint_path(edict_t *self);
void SP_monster_carrier(edict_t *self);
void SP_monster_widow(edict_t *self);
void SP_monster_widow2(edict_t *self);
// void SP_dm_tag_token(edict_t *self);
// void SP_dm_dball_goal(edict_t *self);
// void SP_dm_dball_ball(edict_t *self);
// void SP_dm_dball_team1_start(edict_t *self);
// void SP_dm_dball_team2_start(edict_t *self);
// void SP_dm_dball_ball_start(edict_t *self);
// void SP_dm_dball_speed_change(edict_t *self);
void SP_monster_kamikaze(edict_t *self);
void SP_turret_invisible_brain(edict_t *self);
void SP_xatrix_item(edict_t *self);
// void SP_misc_nuke_core(edict_t *self);

void SpawnDamage(int type, vec3_t origin, vec3_t normal); // from g_combat.c but used elsewhere

void ThrowMoreStuff(edict_t *self, vec3_t point);
void ThrowSmallStuff(edict_t *self, vec3_t point);
void ThrowWidowGibLoc(edict_t *self, char *gibname, int damage,
		int type, vec3_t startpos, qboolean fade);
void ThrowWidowGibSized(edict_t *self, char *gibname, int damage, int type,
		vec3_t startpos, int hitsound, qboolean fade);

// ace - (a_monsters.c) additional monster functions
void monster_fire_blueblaster(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, int effect);
void monster_fire_ionripper(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, int effect);
void monster_fire_heat(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype);
void dabeam_hit(edict_t *self);
void monster_dabeam(edict_t *self);
void stationarymonster_start(edict_t *self);
void cleanupHealTarget(edict_t *ent);
// ace - extra prototypes for spawning code
void monster_start_go(edict_t *self);
qboolean monster_start(edict_t *self);
void monster_use(edict_t *self, edict_t *other /* unused */, edict_t *activator);
void M_SetEffects(edict_t *ent);

void SP_misc_explobox(edict_t *self);

// ace - (a_ai.c) modified newai (rogue)
qboolean FindTarget(edict_t *self); // from g_ai.c

qboolean blocked_checkshot(edict_t *self, float shotChance);
qboolean blocked_checkplat(edict_t *self, float dist);
qboolean blocked_checkjump(edict_t *self, float dist, float maxDown, float maxUp);
qboolean blocked_checknewenemy(edict_t *self);
qboolean monsterlost_checkhint(edict_t *self);
qboolean inback(edict_t *self, edict_t *other);
float realrange(edict_t *self, edict_t *other);
edict_t *SpawnBadArea(vec3_t mins, vec3_t maxs, float lifespan, edict_t *owner);
edict_t *CheckForBadArea(edict_t *ent);
qboolean MarkTeslaArea(edict_t *self, edict_t *tesla);
void InitHintPaths(void);
void PredictAim(edict_t *target, vec3_t start, float bolt_speed, qboolean eye_height,
float offset, vec3_t aimdir, vec3_t aimpoint);
qboolean below(edict_t *self, edict_t *other);
void drawbbox(edict_t *self);
void M_MonsterDodge(edict_t *self, edict_t *attacker, float eta, trace_t *tr);
void monster_duck_down(edict_t *self);
void monster_duck_hold(edict_t *self);
void monster_duck_up(edict_t *self);
qboolean has_valid_enemy(edict_t *self);
void TargetTesla(edict_t *self, edict_t *tesla);
void hintpath_stop(edict_t *self);
edict_t *PickCoopTarget(edict_t *self);
int CountPlayers(void);
void monster_jump_start(edict_t *self);
qboolean monster_jump_finished(edict_t *self);
void monster_done_dodge(edict_t *self);

// zaero ai (schooling)
void ai_schoolStand (edict_t *self, float dist);
void ai_schoolRun (edict_t *self, float dist);
void ai_schoolWalk (edict_t *self, float dist);
void ai_schoolCharge (edict_t *self, float dist);
void ai_schoolBackWalk (edict_t *self, float dist);
void ai_schoolSideStepRight (edict_t *self, float dist);
void ai_schoolSideStepLeft (edict_t *self, float dist);


// ace - (a_misc.c) misc stuff
void spray_blood(edict_t *self, vec3_t start, vec3_t dir, int mod);		// ace - based on aq2 splats
void spray_more_blood(edict_t *self, vec3_t start, vec3_t dir);			// ace - based on aq2 splats
void ThrowGibACID(edict_t *self, char *gibname, int damage, int type);
void ThrowHeadACID(edict_t *self, char *gibname, int damage, int type);

#endif /* ACE_LOCAL_H */