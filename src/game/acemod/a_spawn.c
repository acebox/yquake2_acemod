/* =======================================================================
 *
 * Additional Spawn Code
 *
 * =======================================================================
 */

#include "../header/local.h"

// ace - for rogue spawning code
#define LEG_WAIT_TIME 1
#define MAX_LEGSFRAME 23

#define SPAWNGROW_LIFESPAN 0.3
#define STEPSIZE 18


/* =======================================================================
 *
 * Monster Scaling Code 
 *
 * - scales enemy health up when player connects
 * - scales enemy helath down when playe drops/disconnects.
 *
 * =======================================================================
 */

void ace_EnemyScaleUp(void)
{
}

void ace_EnemyScaleDn(void)
{
}

void ED_CallSpawn (edict_t *ent);

// ace - expanded to try other potential spawn spots.
static edict_t *FindZSpawn(int i)
{
	edict_t *oldSpot = NULL;
	edict_t *spot = NULL;

	while(i)
	{
		spot = G_Find (oldSpot, FOFS(classname), "info_player_deathmatch");
		if (spot != NULL)
		{
			i--;
		}
		else if (oldSpot == NULL)
		{
			return NULL;
		}

		oldSpot = spot;
	}

	// make 1 last ditch effor
	if (!spot)
		spot = G_Find(NULL, FOFS(classname), "info_player_deathmatch");

	// try misc_teleporter_dest
	while(i)
	{
		spot = G_Find (oldSpot, FOFS(classname), "misc_teleporter_dest");
		if (spot != NULL)
		{
			i--;
		}
		else if (oldSpot == NULL)
		{
			return NULL;
		}

		oldSpot = spot;
	}

	// make 1 last ditch effor
	if (!spot)
		spot = G_Find(NULL, FOFS(classname), "misc_teleporter_dest");

	// try misc_teleporter
	while(i)
	{
		spot = G_Find (oldSpot, FOFS(classname), "misc_teleporter");
		if (spot != NULL)
		{
			i--;
		}
		else if (oldSpot == NULL)
		{
			return NULL;
		}

		oldSpot = spot;
	}

	// make 1 last ditch effor
	if (!spot)
		spot = G_Find(NULL, FOFS(classname), "misc_teleporter");

	// try info_player_coop
	while(i)
	{
		spot = G_Find (oldSpot, FOFS(classname), "info_player_coop");
		if (spot != NULL)
		{
			i--;
		}
		else if (oldSpot == NULL)
		{
			return NULL;
		}

		oldSpot = spot;
	}

	// make 1 last ditch effor
	if (!spot)
		spot = G_Find(NULL, FOFS(classname), "info_player_coop");

	// try info_player_intermission
	while(i)
	{
		spot = G_Find (oldSpot, FOFS(classname), "info_player_intermission");
		if (spot != NULL)
		{
			i--;
		}
		else if (oldSpot == NULL)
		{
			return NULL;
		}

		oldSpot = spot;
	}

	// make 1 last ditch effor
	if (!spot)
		spot = G_Find(NULL, FOFS(classname), "info_player_intermission");

	return spot;
}

static qboolean SpawnZ(gitem_t *item, edict_t *spot)
{
	edict_t	*ent;
	vec3_t	forward;
	vec3_t  angles;
	vec3_t start;
	vec3_t end;
	trace_t tr;
	int ang = 0;
	int startAng = 0;

	ent = G_Spawn();

	ent->classname = item->classname;
	VectorSet (ent->mins, -15, -15, -15);
	VectorSet (ent->maxs, 15, 15, 15);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_BOUNCE;
	ED_CallSpawn(ent);

	startAng = rand() % 360;
	VectorCopy(spot->s.origin, start);
	start[2] += 16;

	for (ang = startAng; ang < startAng + 360; ang += 15)
	{
		angles[0] = 0;
		angles[1] = ang;
		angles[2] = 0;

		AngleVectors (angles, forward, NULL, NULL);
		VectorMA(start, 128, forward, end);

		tr = gi.trace(start, ent->mins, ent->maxs, end, NULL, MASK_SHOT);
		if (tr.fraction < 1.0)
			continue;

		VectorCopy(end, ent->s.origin);
		gi.linkentity(ent);
		return true;
	}
	G_FreeEdict(ent);
	return false;
}

static char *items[] = 
{
	"weapon_supershotgun",
	"weapon_chaingun",
	"weapon_railgun",
	"weapon_soniccannon",
	"weapon_sniperrifle",
	"ammo_grenades",
	"ammo_ired",
	"ammo_plasmashield",
	"item_quad",
	"item_bandolier",
	NULL
};

// ace - spawns extra ents
void ace_AddItems(void)
{
	char **ptr = NULL;
	int added = 0;
	int count = 1;
	
	// scan thru all the items looking for our items
	ptr = items;
	while (*ptr != NULL)
	{
		edict_t *e = G_Find(NULL, FOFS(classname), *ptr);
		if (e != NULL)
			return;

		ptr++;
	}
	
	// try to spawn 1 of each item near a deathmatch spot
	ptr = &items[0];
	while(*ptr != NULL)
	{
		int j = 0;
		gitem_t *i = NULL;
		edict_t *spot = NULL;

		i = FindItemByClassname(*ptr);
		ptr++;
		if (i == NULL)
			continue;

		for (j = 0; j < 4; j++)
		{
			spot = FindZSpawn(count++);
			if (spot == NULL)
			{
				gi.dprintf ("ace_AddItems: no spots available, bailing!\n");
				break;
			}

			if (SpawnZ(i, spot))
			{
				added++;
				break;
			}
		}
	}
	gi.dprintf ("ace_AddItems: %i entities added.\n", added);
}

// ace - TEST - randomizer
// FIXME: throw a cvar to control this
// this gives the monsters items, if they don't already
void ace_MonsterItemRandomizer(edict_t *ent)
{
	float ace_rand;

	ace_rand = ACE_RANDOM;
	
	// MONSTERS (modeled after coopordie's randomizer)
	if (!Q_strncasecmp(ent->classname, "monster_", 8))
	{
		gi.dprintf("ace_MonsterItemRandomizer: checking item on %s @ %s\n", ent->classname, vtos(ent->s.origin));

		if (strcmp(ent->classname, "monster_soldier") == 0)
		{
			if (ace_rand < 0.25) // 0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_shells");
			}
		}
		else if (strcmp(ent->classname, "monster_soldier_light") == 0)
		{
			if (ace_rand < 0.25) //0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_bullets");
			}
		}
		else if (strcmp(ent->classname, "monster_soldier_ss") == 0)
		{
			if (ace_rand < 0.33) // 0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_bullets");
			}
		}
		else if (strcmp(ent->classname, "monster_infantry") == 0)
		{
			if (ace_rand < 0.25) // 0.05)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_shells");
			}
			else if (ace_rand < 0.5)// 0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_bullets");
			}
		}
		else if (strcmp(ent->classname, "monster_flyer") == 0)
		{
			if (ace_rand < 0.25)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_cells");
			}
		}
		else if (strcmp(ent->classname, "monster_floater") == 0)
		{
			if (ace_rand < 0.25)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_cells");
			}
		}
		else if (strcmp(ent->classname, "monster_hover") == 0)
		{
			if (ace_rand < 0.25)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_cells");
			}
		}
		else if (strcmp(ent->classname, "monster_gunner") == 0)
		{
			if (ace_rand < 0.1)//0.05)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_grenades");
			}
			else if (ace_rand < 0.5)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_bullets");
			}
		}
		else if (strcmp(ent->classname, "monster_berserk") == 0)
		{
			if (ace_rand < 0.1)//0.05)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("item_adrenaline");
			}
			else if (ace_rand < 0.25)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("item_armor_jacket");
			}
		}
		else if (strcmp(ent->classname, "monster_mutant") == 0)
		{
			if (ace_rand < 0.05)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("item_adrenaline");
			}
		}
		else if (strcmp(ent->classname, "monster_chick") == 0)
		{
			if (ace_rand < 0.33)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_rockets");
			}
		}
		else if (strcmp(ent->classname, "monster_gladiator") == 0)
		{
			if (ace_rand < 0.5)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_slugs");
			}
		}
		else if (strcmp(ent->classname, "monster_tank") == 0)
		{
			if (ace_rand < 0.25)//0.05)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_rockets");
			}
			else if (ace_rand < 0.5)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_bullets");
			}
		}
		else if (strcmp(ent->classname, "monster_tank_commander") == 0)
		{
			if (ace_rand < 0.5)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_rockets");
			}
			else if (ace_rand < 0.75)//0.2)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_bullets");
			}
		}
		else if (strcmp(ent->classname, "monster_supertank") == 0)
		{
			if (ace_rand < 0.5)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_rockets");
			}
			else if (ace_rand < 0.75)//0.2)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_bullets");
			}
		}
		else if (strcmp(ent->classname, "monster_boss2") == 0)
		{
			if (ace_rand < 0.5)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_rockets");
			}
			else if (ace_rand < 0.75)//0.2)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_bullets");
			}
		}
		else if (strcmp(ent->classname, "monster_jorg") == 0)
		{
			if (ace_rand < 0.5)//0.1)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_slugs");
			}
			else if (ace_rand < 0.75)//0.2)
			{
				gi.dprintf("ace_MonsterItemRandomizer: HIT!\n");
				ent->item = FindItemByClassname("ammo_cells");
			}
		}
	}
}

// this changes ent->classname randomly
void ace_SpawnRandomizer(edict_t *ent)
{
	float ace_rand;

	ace_rand = ACE_RANDOM;

	// MONSTERS (modeled after coopordie's randomizer)
	if (!Q_strncasecmp(ent->classname, "monster_", 8))
	{
		gi.dprintf("ace_SpawnRandomizer: flipping coin on %s @ %s\n", ent->classname, vtos(ent->s.origin));
		if (strcmp(ent->classname, "monster_soldier") == 0)
		{
			if (ace_rand < 0.05)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_chick";
			}
			else if (ace_rand < 0.1)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_infantry";
			}
			else if (ace_rand < 0.2)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_soldier_ss";
			}
		}
		else if (strcmp(ent->classname, "monster_soldier_light") == 0)
		{
			if (ace_rand < 0.05)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_chick";
			}
			else if (ace_rand < 0.1)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_infantry";
			}
			else if (ace_rand < 0.2)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_soldier_ss";
			}
		}
		else if (strcmp(ent->classname, "monster_soldier_ss") == 0)
		{
			if (ace_rand < 0.1)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_chick";
			}
			else if (ace_rand < 0.15)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_infantry";
			}
		}
		else if (strcmp(ent->classname, "monster_infantry") == 0)
		{
			if (ace_rand < 0.05)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_chick";
			}
			else if (ace_rand < 0.1)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_gunner";
			}
			else if (ace_rand < 0.2)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_berserk";
			}
		}
		else if (strcmp(ent->classname, "monster_berserk") == 0)
		{
			if (ace_rand < 0.1)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_chick";
			}
			else if (ace_rand < 0.15)
			{
				gi.dprintf("ace_SpawnRandomizer: BOOM!\n");
				ent->classname = "monster_gunner";
			}
		}
	}
	// WEAPONS
	else if (Q_strncasecmp(ent->classname, "weapon_", 6))
	{
		gi.dprintf("ace_SpawnRandomizer: flipping coin on %s @ %s\n", ent->classname, vtos(ent->s.origin));
		if (strcmp(ent->classname, "weapon_shotgun") == 0)
		{
			if (ace_rand < 0.25)
			{
				gi.dprintf("ace_SpawnRandomizer: BANG!\n");
				ent->classname = "weapon_shotgun";
				return;
			}
		}
		else if (strcmp(ent->classname, "weapon_railgun") == 0)
		{
			if (ace_rand < 0.25)
			{
				gi.dprintf("ace_SpawnRandomizer: BANG!\n");
				ent->classname = "weapon_sniperrifle";
				return;
			}
		}
		else if (strcmp(ent->classname, "weapon_grenadelauncher") == 0)
		{
			if (ace_rand < 0.05)
			{
				gi.dprintf("ace_SpawnRandomizer: BANG!\n");
				ent->classname = "weapon_sniperrifle";
				return;
			}
			else if (ace_rand < 0.1)
			{
				gi.dprintf("ace_SpawnRandomizer: BANG!\n");
				ent->classname = "weapon_railgun";
				return;
			}
			else if (ace_rand < 0.25)
			{
				gi.dprintf("ace_SpawnRandomizer: BANG!\n");
				ent->classname = "weapon_rocketlauncher";
				return;
			}
		}
		else if (strcmp(ent->classname, "weapon_rocketlauncher") == 0)
		{
			if (ace_rand < 0.05)
			{
				gi.dprintf("ace_SpawnRandomizer: BANG!\n");
				ent->classname = "weapon_sniperrifle";
				return;
			}
			else if (ace_rand < 0.1)
			{
				gi.dprintf("ace_SpawnRandomizer: BANG!\n");
				ent->classname = "weapon_railgun";
				return;
			}
			else if (ace_rand < 0.25)
			{
				gi.dprintf("ace_SpawnRandomizer: BANG!\n");
				ent->classname = "weapon_grenadelauncher";
				return;
			}
		}
		else if (strcmp(ent->classname, "weapon_bfg") == 0)
		{
			if (((int)dmflags->value & DF_INFINITE_AMMO) && (deathmatch->value) )
			{
				gi.dprintf("(ace)SpawnItem: DeathMatch BFG Substitution!\n");
				if (ace_rand < 0.1)
				{
					gi.dprintf("ace_SpawnRandomizer: BANG!\n");
					ent->classname = "weapon_soniccannon";
					return;
				}
				else if (ace_rand < 0.25)
				{
					gi.dprintf("ace_SpawnRandomizer: BANG!\n");
					ent->classname = "weapon_sniperrifle";
					return;
				}
				else
				{
					gi.dprintf("ace_SpawnRandomizer: BANG!\n");
					ent->classname = "weapon_hyperblaster";
					return;
				}
			}
			else
			{
				if (ace_rand < 0.1)
				{
					gi.dprintf("ace_SpawnRandomizer: BANG!\n");
					ent->classname = "weapon_sniperrifle";
					return;
				}
				else if (ace_rand < 0.25)
				{
					gi.dprintf("ace_SpawnRandomizer: BANG!\n");
					ent->classname = "weapon_hyperblaster";
					return;
				}
				else if (ace_rand < 0.33)
				{
					gi.dprintf("ace_SpawnRandomizer: BANG!\n");
					ent->classname = "weapon_soniccannon";
					return;
				}
			}
		}
	}
};

/* ================================================================== */

/*
 *
 *	"acemod" version 3x monster spawn randomizer.
 *	- the system is revised to use arrays
 *	- record stats from original monster spawn (spawnflags, origin, angles, item, target, combattarget, etc..)
 *	- frees all original entities after stats are recorded (speed improvement?)
 *
 */

// prototypes for spawn functions
// misc
void SP_misc_insane(edict_t *self);

// type0 enemies
void SP_monster_soldier_light(edict_t *self);
void SP_monster_soldier(edict_t *self);
void SP_monster_soldier_ss(edict_t *self);
void SP_monster_soldier_hypergun(edict_t *self);
void SP_monster_soldier_lasergun(edict_t *self);
void SP_monster_soldier_ripper(edict_t *self);
void SP_monster_stalker(edict_t *self);
void SP_monster_hound (edict_t *self);
void SP_monster_parasite(edict_t *self);
// swimming type0 (aka, this spawns itself)
void SP_monster_flipper(edict_t *self);
// type1 enemies
void SP_monster_berserk(edict_t *self);
void SP_monster_gunner(edict_t *self);
void SP_monster_infantry(edict_t *self);
void SP_monster_gekk(edict_t *self); // can also swim...
void SP_monster_fixbot(edict_t *self);
void SP_monster_handler (edict_t *self);
// flying type1
void SP_monster_flyer(edict_t *self);
void SP_monster_brain(edict_t *self);
void SP_monster_floater(edict_t *self);
void SP_monster_hover(edict_t *self);
void SP_monster_kamikaze(edict_t *self);
// type2 enemies (mini-bosses)
void SP_monster_chick(edict_t *self);
void SP_monster_gladiator(edict_t *self);
void SP_monster_medic(edict_t *self);
void SP_monster_mutant(edict_t *self);
void SP_monster_tank(edict_t *self);
// type3 enemies (bosses)
void SP_monster_supertank(edict_t *self);
void SP_monster_boss2(edict_t *self);
void SP_monster_jorg(edict_t *self);
void SP_monster_carrier(edict_t *self);
void SP_monster_widow(edict_t *self);
void SP_monster_widow2(edict_t *self);
void SP_monster_boss5(edict_t *self);
void SP_monster_zboss(edict_t *self);

/* 
 *
 *	ace_TallyMonster
 *	- this adds another "slot" to the randomizer for the following: (general, by class, by tier)
 *
 */
static int ace_GetRandomMTYPE(); // for fallback.

static void ace_TallyMonster (edict_t *ent)
{

/*
	int max_mspawns[MAX_MONSTERTYPES];
	int num_mspawns[MAX_MONSTERTYPES];
	
	ace_mspawninfo_t	mspawnspot[MAX_ENTITIES];
	int					num_mspawnspot;	// number of spawn spots
*/
		// ace - general monster bump.
		level.rmonsterinfo.max_monsters++;
		
		if (ACE_RANDOM < ACE_CHANCE_INSANE)
		{
			level.rmonsterinfo.max_monsters++;
			level.rmonsterinfo.max_mspawns[MONSTER_INSANE]++;
		}

		if (strcmp(ent->classname, "monster_soldier") == 0)
		{
			ent->monstertype = MONSTER_SOLDIER;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_soldier_ss") == 0)
		{
			ent->monstertype = MONSTER_SOLDIER_SS;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_soldier_light") == 0)
		{
			ent->monstertype = MONSTER_SOLDIER_LIGHT;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_soldier_lasergun") == 0)
		{
			ent->monstertype = MONSTER_SOLDIER_LASERGUN;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_soldier_hypergun") == 0)
		{
			ent->monstertype = MONSTER_SOLDIER_HYPERGUN;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_soldier_ripper") == 0)
		{
			ent->monstertype = MONSTER_SOLDIER_RIPPER;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_stalker") == 0)
		{
			ent->monstertype = MONSTER_STALKER;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_infantry") == 0)
		{
			ent->monstertype = MONSTER_INFANTRY;
			ent->monstertier = 1;
		}
		else if (strcmp(ent->classname, "monster_handler") == 0)
		{
			ent->monstertype = MONSTER_HANDLER;
			ent->monstertier = 1;
			
			if(!(ent->spawnflags & 16)) // ace - (zaero) add one for the hound which is created later :)
			{
				level.rmonsterinfo.max_monsters++;
			}
		}
		else if (strcmp(ent->classname, "monster_gekk") == 0)
		{
			ent->monstertype = MONSTER_GEKK;
			ent->monstertier = 1;
		}
		else if (strcmp(ent->classname, "monster_flipper") == 0)
		{
			ent->monstertype = MONSTER_FLIPPER;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_parasite") == 0)
		{
			ent->monstertype = MONSTER_PARASITE;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_hound") == 0)
		{
			ent->monstertype = MONSTER_HOUND;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_flyer") == 0)
		{
			ent->monstertype = MONSTER_FLYER;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_floater") == 0)
		{
			ent->monstertype = MONSTER_FLOATER;
			ent->monstertier = 0;
		}
		else if (strcmp(ent->classname, "monster_hover") == 0)
		{
			ent->monstertype = MONSTER_HOVER;
			ent->monstertier = 1;
		}
		else if (strcmp(ent->classname, "monster_kamikaze") == 0)
		{
			ent->monstertype = MONSTER_KAMIKAZE;
			ent->monstertier = 1;
		}
		else if (strcmp(ent->classname, "monster_daedalus") == 0)
		{
			ent->monstertype = MONSTER_DAEDALUS;
			ent->monstertier = 1;
		}
		else if (strcmp(ent->classname, "monster_berserk") == 0)
		{
			ent->monstertype = MONSTER_BERSERK;
			ent->monstertier = 1;
		}
		else if (strcmp(ent->classname, "monster_gunner") == 0)
		{
			ent->monstertype = MONSTER_GUNNER;
			ent->monstertier = 1;
		}
		else if (strcmp(ent->classname, "monster_chick") == 0)
		{
			ent->monstertype = MONSTER_CHICK;
			ent->monstertier = 2;
		}
		else if (strcmp(ent->classname, "monster_brain") == 0)
		{
			ent->monstertype = MONSTER_BRAIN;
			ent->monstertier = 1;
		}
		else if (strcmp(ent->classname, "monster_fixbot") == 0)
		{
			ent->monstertype = MONSTER_FIXBOT;
			ent->monstertier = 1;
		}
		else if (strcmp(ent->classname, "monster_chick_heat") == 0)
		{
			ent->monstertype = MONSTER_CHICK_HEAT;
			ent->monstertier = 2;
		}
		else if (strcmp(ent->classname, "monster_gladiator") == 0)
		{
			ent->monstertype = MONSTER_GLADIATOR;
			ent->monstertier = 2;
		}
		else if (strcmp(ent->classname, "monster_gladb") == 0)
		{
			ent->monstertype = MONSTER_GLADB;
			ent->monstertier = 2;
		}
		else if (strcmp(ent->classname, "monster_medic") == 0)
		{
			ent->monstertype = MONSTER_MEDIC;
			ent->monstertier = 2;
		}
		else if (strcmp(ent->classname, "monster_medic_commander") == 0)
		{
			ent->monstertype = MONSTER_MEDIC_COMMANDER;
			ent->monstertier = 2;
		}
		else if (strcmp(ent->classname, "monster_mutant") == 0)
		{
			ent->monstertype = MONSTER_MUTANT;
			ent->monstertier = 2;
		}
		else if (strcmp(ent->classname, "monster_tank") == 0)
		{
			ent->monstertype = MONSTER_TANK;
			ent->monstertier = 2;
		}
		else if (strcmp(ent->classname, "monster_tank_commander") == 0)
		{
			ent->monstertype = MONSTER_TANK_COMMANDER;
			ent->monstertier = 3;
		}
		else if (strcmp(ent->classname, "monster_supertank") == 0)
		{
			ent->monstertype = MONSTER_SUPERTANK;
			ent->monstertier = 3;
		}
		else if (strcmp(ent->classname, "monster_boss2") == 0)
		{
			ent->monstertype = MONSTER_BOSS2;
			ent->monstertier = 3;
		}
		else if (strcmp(ent->classname, "monster_carrier") == 0)
		{
			ent->monstertype = MONSTER_CARRIER;
			ent->monstertier = 3;
		}
		else if (strcmp(ent->classname, "monster_boss5") == 0)
		{
			ent->monstertype = MONSTER_BOSS5;
			ent->monstertier = 3;
		}
		else if (strcmp(ent->classname, "monster_widow") == 0)
		{
			ent->monstertype = MONSTER_WIDOW;
			ent->monstertier = 3;
		}
		else if (strcmp(ent->classname, "monster_widow2") == 0)
		{
			ent->monstertype = MONSTER_WIDOW2;
			ent->monstertier = 3;
		}
		else if (strcmp(ent->classname, "monster_zboss") == 0)
		{
			ent->monstertype = MONSTER_ZBOSS;
			ent->monstertier = 3;
		}
		else if (strcmp(ent->classname, "monster_jorg") == 0)
		{
			ent->monstertype = MONSTER_JORG;
			ent->monstertier = 3;
		}
		else if (strcmp(ent->classname, "monster_makron") == 0)
		{
			ent->monstertype = MONSTER_MAKRON;
			ent->monstertier = 3;
		}
		else // fallback.. this covers all extra things converted
		{
			gi.dprintf("%s is not currently supported by the randomizer, randomizing.\n", ent->classname);
			ent->monstertype = ace_GetRandomMTYPE();
			ent->monstertier = 0; // spawn something random..
			if ((ent->monstertype == MONSTER_HANDLER) &&
				!(ent->spawnflags & 16)) // ace - (zaero-handler) add one for the hound which is created later :)
			{
				level.rmonsterinfo.max_monsters++;
			}
		}

		// save some code
		level.rmonsterinfo.max_mspawns[ent->monstertype]++;
}

/*
 *	ace_InitSpawnSpot
 *	- this "converts" (and frees) the original monster entity into the rmonsterinfo.mspawnspot array
 */
static void ace_InitSpawnSpot (edict_t *ent)
{
	// set item (if applicable)
	if (st.item)
	{
		ent->item = FindItemByClassname(st.item);

		if (!ent->item)
		{
			gi.dprintf("%s at %s has bad item: %s\n", ent->classname,
					vtos(ent->s.origin), st.item);
		}
	}
	else
	{
		// ace - randomize item if nothing defined
		ace_MonsterItemRandomizer(ent);
	}
	
	// transfer variables here
	int	rmon = level.rmonsterinfo.num_mspawnspot;

	level.rmonsterinfo.mspawnspot[rmon].spotnum = rmon;

	level.rmonsterinfo.mspawnspot[rmon].item = ent->item;
	VectorCopy (ent->s.origin, level.rmonsterinfo.mspawnspot[rmon].origin);
	VectorCopy (ent->s.angles, level.rmonsterinfo.mspawnspot[rmon].angles);
	level.rmonsterinfo.mspawnspot[rmon].target = ent->target;
	level.rmonsterinfo.mspawnspot[rmon].targetname = ent->targetname;
	level.rmonsterinfo.mspawnspot[rmon].killtarget = ent->killtarget;
	level.rmonsterinfo.mspawnspot[rmon].team = ent->team;
	level.rmonsterinfo.mspawnspot[rmon].pathtarget = ent->pathtarget;
	level.rmonsterinfo.mspawnspot[rmon].deathtarget = ent->deathtarget;
	level.rmonsterinfo.mspawnspot[rmon].combattarget = ent->combattarget;

	level.rmonsterinfo.mspawnspot[rmon].monstertype = ent->monstertype;
	level.rmonsterinfo.mspawnspot[rmon].monstertier = ent->monstertier;

	// bump counter
	level.rmonsterinfo.num_mspawnspot++;

	G_FreeEdict (ent); // once done, free edict_t;
}

void SP_monster_rspawnspot (edict_t *ent)
{
//	float r = random();
	
	// tally up!
	ace_TallyMonster (ent);

	// init spawnspot settings
	ace_InitSpawnSpot (ent);
}

// ace - slightly based on client random spawning, but with a twist...
/*
 * Returns the distance to the
 * nearest player from the given vec3_t - where
 */
static float PlayersRangeFromSpot2(vec3_t where)
{
	edict_t *player;
	float bestplayerdistance;
	vec3_t v;
	int n;
	float playerdistance;

	bestplayerdistance = 99999;

	for (n = 1; n <= maxclients->value; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
		{
			continue;
		}

		if (player->health <= 0) // #dead
		{
			continue;
		}

		VectorSubtract(where, player->s.origin, v);
		playerdistance = VectorLength(v);

		if (playerdistance < bestplayerdistance)
		{
			bestplayerdistance = playerdistance;
		}
	}

	return bestplayerdistance;
}

static int ace_FindMSpawn(void)
{
	int n, c, bestspot;
	float range, bestrange;
	qboolean	firstrun = true, ynot = false, seentit;
	trace_t	tr;
	vec3_t	testarea;
	edict_t	*cl;
	
	bestspot = -1; // -1 on array means nothing found
	bestrange = 99999;

	if (level.rmonsterinfo.num_mspawnspot < 1)
	{
		gi.dprintf("ace_FindMSpawn: no monster spawn points!\n");
		return -1;
	}
	
	for (n = 0; n < level.rmonsterinfo.num_mspawnspot; n++)
	{
		seentit = false; // forces recheck for each
//		checkspot = level.rmonsterinfo.mspawnspot[n];
		range = PlayersRangeFromSpot2(level.rmonsterinfo.mspawnspot[n].origin);

		if (level.rmonsterinfo.mspawnspot[n].timeout > level.time)
		{
//			gi.dprintf("ace_MonsterRandomizerGo: selected spawnspot timeout still in effect, delaying.\n");
			// rare chance of, well why not, running things back (incase of an expired timeout, etc..)
			if (ACE_RANDOM == 1 && ynot == false)
			{
				gi.dprintf("ace_FindMSpawn: ynot!\n");
				ynot = true;
				n = 0; // hopefully, I'm not kicking myself later for this.
			}
			// 25% chance random delay added (from 0.01 to 1)
			if (ACE_RANDOM < 0.25)
			{
				gi.dprintf("ace_FindMSpawn: selected spot delayed, delaying spawning overall.\n");
				level.rmonsterinfo.t1_spawned += (ACE_RANDOM * ACE_RANDOM); // random delay when spawn spot occupied
			}
			continue;
		}

		// basic check
		if (gi.pointcontents(level.rmonsterinfo.mspawnspot[n].origin) & (MASK_MONSTERSOLID | CONTENTS_PLAYERCLIP))
		{
//			gi.dprintf("ace_MonsterRandomizerGo: selected spawnspot monster occupied, delaying - hoping they move.\n");
//			level.rmonsterinfo.t1_spawned += 1; // delay when monster occupied spawn
			gi.dprintf("ace_FindMSpawn: spawn blocked, delaying spot.\n");
			// 5% chance random delay added (from 0.01 to 1)
			if (ACE_RANDOM < 0.05)
			{
				level.rmonsterinfo.t1_spawned += (ACE_RANDOM * ACE_RANDOM)/2; // random delay when spawn spot occupied
				gi.dprintf("ace_FindMSpawn: delaying spawning overall.\n");
			}
			// add random timeout to spot, hopefully clears up.
			level.rmonsterinfo.mspawnspot[n].timeout = level.time + (ACE_RANDOM * ACE_RANDOM); 
			continue;
		}

		// thorough check - hopefully this isn't taxing, then again it's 2015..
		VectorCopy (level.rmonsterinfo.mspawnspot[n].origin, testarea);
//		testarea[2] += 1; // check off the ground
		
		tr = gi.trace(testarea, vec3_origin, vec3_origin, testarea, NULL, (MASK_MONSTERSOLID | CONTENTS_PLAYERCLIP));

		if (tr.startsolid || tr.allsolid)
		{
			gi.dprintf("ace_FindMSpawn: trace solid.\n");
			// add random timeout to spot, hopefully clears up.
			level.rmonsterinfo.mspawnspot[n].timeout = level.time + (ACE_RANDOM * ACE_RANDOM); 
			continue; // in some solid
		}

		if (tr.ent != world &&
			(tr.ent->client || (tr.ent->svflags & SVF_MONSTER)) &&
			(tr.ent->health > 0) && (tr.ent->deadflag == DEAD_NO))
		{
			gi.dprintf("ace_FindMSpawn: traced into entity.\n");
			// add random timeout to spot, hopefully clears up.
			level.rmonsterinfo.mspawnspot[n].timeout = level.time + (ACE_RANDOM * ACE_RANDOM); 
			continue; // in some ent
		}

		// ace - reworked to check if any players can see this spot.
		for (c = 0; c < game.maxclients; c++)
		{
			cl = &g_edicts[c];

			if (!cl)
			{	// null entity check
				continue;
			}

			if (!cl->inuse)
			{	// invalid entity check
				continue;
			}
			
			if (!cl->client)
			{	// invalid client check
				continue;
			}

			if (!cl->client->pers.connected)
			{	// client not connected
				continue;
			}
			
			if (cl->client->pers.spectator)
			{	// don't worry about spectators
				continue;
			}

			if ((v_visible (cl->s.origin, level.rmonsterinfo.mspawnspot[n].origin)) &&
				(v_infront (cl->s.origin, cl->s.angles, level.rmonsterinfo.mspawnspot[n].origin)))
			{
				seentit = true; // this spawn is infront of a player and visible.
			}
		}
		
		if (seentit == true)
		{
			gi.dprintf("ace_FindMSpawn: SEEN'T IT!\n");
			// add random timeout to spot, hopefully clears up.
			level.rmonsterinfo.mspawnspot[n].timeout = level.time + (ACE_RANDOM * ACE_RANDOM); 
			continue; // in some ent
		}
		// this skips if theres an active level.sight_client
/*		if (level.sight_client != NULL && 
			level.sight_client->client &&
			level.sight_client->health > 0 &&
			game.maxclients >= 1)
		{
			if (v_visible (level.sight_client->s.origin, level.rmonsterinfo.mspawnspot[n].origin) &&
			v_infront (level.sight_client->s.origin, level.sight_client->s.angles, level.rmonsterinfo.mspawnspot[bestspot].origin))
			{
				continue; // don't wanna spawn anything right infront of the player (FIXME: still happens oddly, lol)
			}
		} */
						
		if (range < bestrange || firstrun) // set spot1, this is the closest to a player
		{
			if (firstrun) // this should hopefully lock something in
				firstrun = false;
			bestrange = range;
			bestspot = n; // reference array, no need to compilicate things.
		}
	}

	return bestspot; // returns -1 if no spawns found
}

/************************************************************************************

	ace_MonsterQuickPopulate
	- this runs throgh all the monster spawn spots and fills them in when needed.
	- checks for target, targetname, killtarget, combattarget, etc.. and starts with those filled.

*************************************************************************************/
void ace_MonsterQuickPopulate (void)
{
}

/************************************************************************************

	ace_MonsterRandomizerGo
	- thins every game frame, handles spawning monsters of all types
	- checks if any enemy (by tier) needs to be added, and does.
	- picks a random free spawn spot near the player, out of sight, within range.
	- if successfull, tags the spot with a delay.
	
	TODO:
	- fix flaw, 

*************************************************************************************/
static int last_mspawn;
static float last_mroll;

// ace - for now, a bit primitive but it works.  *should* cycle downwards if an enemy is trying to spawn
static int ace_GetRandomMTYPE()
{
	int mpick = MONSTER_SOLDIER_LIGHT; // default
	float r = ACE_RANDOM;

// ace - test this, make sure lightning doesn't strike twice. start
	while (r == last_mroll)
	{
		gi.dprintf("ace_GetRandomMTYPE: lightning will not strike twice!\n");
		r = ACE_RANDOM;
	}
	
	last_mroll = r;
// ace - test this, make sure lightning doesn't strike twice. end

	// ace - see acemod.h for definitions
	if (r < ACE_CHANCE_MEDIC_COMMANDER && level.num_wave > 20)
	{
		mpick = MONSTER_MEDIC_COMMANDER;
	}
	else if (r < ACE_CHANCE_MEDIC && level.num_wave > 13)
	{
		mpick = MONSTER_MEDIC;
	}
	else if (r < ACE_CHANCE_MUTANT && level.num_wave > 12)
	{
		mpick = MONSTER_MUTANT;
	}
	else if (r < ACE_CHANCE_GEKK && level.num_wave > 11)
	{
		mpick = MONSTER_GEKK;
	}
	else if (r < ACE_CHANCE_STALKER && level.num_wave > 10)
	{
		mpick = MONSTER_STALKER;
	}
	else if (r < ACE_CHANCE_GUNNER && level.num_wave > 9)
	{
		mpick = MONSTER_GUNNER;
	}
	else if (r < ACE_CHANCE_SOLDIER_LASER && level.num_wave > 8)
	{
		mpick = MONSTER_SOLDIER_LASERGUN;
	}
	else if (r < ACE_CHANCE_SOLDIER_RIPPER && level.num_wave > 7)
	{
		mpick = MONSTER_SOLDIER_RIPPER;
	}
	else if (r < ACE_CHANCE_SOLDIER_HYPER && level.num_wave > 6)
	{
		mpick = MONSTER_SOLDIER_HYPERGUN;
	}
	else if (r < ACE_CHANCE_BERSERK && level.num_wave > 5)
	{
		mpick = MONSTER_BERSERK;
	}
	else if (r < ACE_CHANCE_HANDLER && level.num_wave > 4)
	{
		mpick = MONSTER_HANDLER; // ace - try some zenemies
	}
	else if (r < ACE_CHANCE_SOLDIER_SS && level.num_wave > 2)
	{
		mpick = MONSTER_SOLDIER_SS;
	}
	else if (r < ACE_CHANCE_HOUND)
	{
		mpick = MONSTER_HOUND; // ace - try some zenemies
	}
	else if (r < ACE_CHANCE_INFANTRY)
	{
		mpick = MONSTER_INFANTRY;
	}
	else if (r < ACE_CHANCE_SOLDIER)
	{
		mpick = MONSTER_SOLDIER;
	}
/*	else // default ACE_CHANCE_SOLDIER_LIGHT
	{
		mpick = MONSTER_SOLDIER_LIGHT;
		// ace - incase none on this level (for whatever reason, just add em)
//		if (level.rmonsterinfo.max_mspawns[mpick] < 1)
//		{
//			level.rmonsterinfo.max_mspawns[mpick]++;
//		}
	} */

	gi.dprintf("ace_GetRandomMTYPE: rolled %f picked %i wave # %i\n", r, mpick, level.num_wave);
	return mpick;
}

void ace_MonsterRandomizerGo (void)
{
	float	sd;
	int i, count, maxcount, bestspot;
	edict_t	*ent;

	// valid reasons to bail - start
	if (deathmatch->value)
	{
		return;
	}

/*	if (level.rmonsterinfo.init_mspawn == false)
	{
		return;
	} */

	if (level.rmonsterinfo.wave_set == true)
	{
		return;
	}

	if (level.time < ACE_MSPAWN_DELAY)
	{
		return;
	}
	// valid reasons to bail - end

	// set max_wave_monsters here
	level.rmonsterinfo.max_wave_monsters = 25;

	// there are currently enough monsters for this "wave"
	if (level.rmonsterinfo.num_monsters >= level.rmonsterinfo.max_wave_monsters &&
		level.rmonsterinfo.wave_set == false)
	{
		level.rmonsterinfo.wave_set = true;
		gi.dprintf("ace_MonsterRandomizerGo: Wave # %i started, max enemies is around %i - KILL ALL THE STROGG!\n", level.num_wave, level.rmonsterinfo.max_wave_monsters);
		return;
	}
	
	// randomly spawn on timer
	if ((level.rmonsterinfo.t1_spawned < level.time) || (level.rmonsterinfo.t1_init != true))
	{
		int		mpick, cap;

		cap = level.rmonsterinfo.max_wave_monsters; // + ((level.skill_level * 5) * ACE_RANDOM);
//		if (level.rmonsterinfo.num_monsters > (100+(level.skill_level*5)))
		if (level.rmonsterinfo.num_monsters > cap)
		{	// try to "somewhat" regulate things, cap active monsters
			sd = (float)level.rmonsterinfo.num_monsters + (float)level.num_wave + ((float)level.skill_level * ACE_RANDOM);
			level.rmonsterinfo.t1_spawned = level.time + sd;
			level.rmonsterinfo.t0_spawned = level.time + sd;
			gi.dprintf("ace_MonsterRandomizerGo: the # of monsters is too damn high @ %i, capping @ %i. delaying all spawning for %f seconds.\n", level.rmonsterinfo.num_monsters, cap, sd);
			return;
		}

		// set delay timer
		if (level.rmonsterinfo.t1_init != true)
		{
			sd = 0;
		}
		else
		{
			float sd1, sd2;

			sd1 = ACE_SPAWNTIME_T1 * MAX_SKILL_LEVEL;
			sd2 = ACE_SPAWNTIME_T1 * (skill->value+1);
//			sd = (sd1 - sd2) + (random() * ACE_SPAWNTIME_T1);
			sd = sd1 + (ACE_RANDOM * sd2);

			if (sd < 15)
				sd = 15;
			if (sd > 45)
				sd = 45;
		}
		
		level.rmonsterinfo.t1_spawned = level.time + sd;

		// set max available
		maxcount = level.rmonsterinfo.max_monsters - level.rmonsterinfo.num_monsters;

		if (maxcount) // redundant, but for now...
		{
			// random factor between 50-100% of basecount + random "bonus" for skill-level
			// example(s): on skill 0 (easy) it should spawn between 5-10 enemies, 
			// skills normal+ adds random up to +5 *per* skill level.
			// on skill 7 (uber-insanity), it could try to spawn from 5-45 enemies nearby.  fun times.
//			count = (ACE_MBASE_T1/2 + ((ACE_MBASE_T1/2) * (int)skill->value)) + (randk() % ACE_MBASE_T1);
//			count = ACE_MBASE_T1;
//			count = ((ACE_MBASE_T1/2) + (randk() & (ACE_MBASE_T1/2))) + (randk() & ((ACE_MBASE_T1/2) * (int)skill->value));
			count = level.rmonsterinfo.max_wave_monsters + (randk() & level.rmonsterinfo.max_wave_monsters) + (randk() & level.skill_level);

			// this should catch initial level load
			if (level.rmonsterinfo.t1_init != true)
			{
				level.rmonsterinfo.t1_init = true;
				count = level.rmonsterinfo.max_monsters;

/*				if (count < 25)
				{
					count = 25;
				} */

				gi.dprintf("ace_MonsterRandomizerGo: this should be initial level load for %i enemies.\n", count);
			}
			else
			{
				if (count < 5)
				{
					count = 5;
				}
				if (count > maxcount) // cap it at something
				{
					count = maxcount;
				}
			}

			gi.dprintf("ace_MonsterRandomizerGo: spawning %i out of %i max enemies.\n", count, maxcount);
					
			for (i = 0; i < count; i++)
			{
				bestspot = ace_FindMSpawn(); // see if anything returns

				if (bestspot == -1)
				{
					// ace - just bail if there are no spawn spots (for whatever reason)
					gi.dprintf("ace_MonsterRandomizerGo: bailing, better luck next time!\n");
//					level.rmonsterinfo.t1_spawned = level.time + 9999;
					continue;
				}

				if (level.rmonsterinfo.t1_init)
				{
					// don't use this spot for random # seconds
					level.rmonsterinfo.mspawnspot[bestspot].timeout = level.time + ACE_RANDOM; 
					
					if (ACE_RANDOM < 0.33)
						level.rmonsterinfo.t1_spawned += (ACE_RANDOM*ACE_RANDOM); // increase delay randomly
				}
				
				ent = G_Spawn();
				if (!ent)
				{
					return;
				}

				ent->target = level.rmonsterinfo.mspawnspot[bestspot].target;
				ent->team = level.rmonsterinfo.mspawnspot[bestspot].team;
				ent->targetname = level.rmonsterinfo.mspawnspot[bestspot].targetname;
				ent->killtarget = level.rmonsterinfo.mspawnspot[bestspot].killtarget;
				ent->pathtarget = level.rmonsterinfo.mspawnspot[bestspot].pathtarget;
				ent->combattarget = level.rmonsterinfo.mspawnspot[bestspot].combattarget;
				ent->item = level.rmonsterinfo.mspawnspot[bestspot].item;							// set item
				ent->spawnflags = level.rmonsterinfo.mspawnspot[bestspot].spawnflags;				// set spawnflags
				VectorCopy (level.rmonsterinfo.mspawnspot[bestspot].origin, ent->s.origin);		// set origin
				ent->s.origin[2] += 1; // ace - test to see if this "unstucks" some things (for lack of better word)
				VectorCopy (level.rmonsterinfo.mspawnspot[bestspot].angles, ent->s.angles);		// set angles

//				ent->monstertype = level.rmonsterinfo.mspawnspot[bestspot].monstertype;

				// ace - this random 25% chance check will drop the ambush flag on 
				// an ambush enemy, and save that ambush spawn for later/another enemy.
				// this should mix things up moreso.  otherwise, drop the ambush flag 
				// once used.
				if (ACE_RANDOM < 0.25 && (ent->spawnflags & 1))
				{
					ent->spawnflags &= ~(1);
				}
				else
				{
					level.rmonsterinfo.mspawnspot[bestspot].spawnflags &= ~(1);	// this should  drop spawnflag sight
				}

				// monsters spawned after the initial load, technically won't count for kills.
				if (level.rmonsterinfo.t1_init == true)
				{
					ent->monsterinfo.aiflags |= AI_DO_NOT_COUNT;
				}

				// some spawnflags and target values get cleared after first use.
				level.rmonsterinfo.mspawnspot[bestspot].target = NULL;
				level.rmonsterinfo.mspawnspot[bestspot].targetname = NULL;
				level.rmonsterinfo.mspawnspot[bestspot].killtarget = NULL;
				level.rmonsterinfo.mspawnspot[bestspot].item = NULL;
				level.rmonsterinfo.mspawnspot[bestspot].team = NULL;

				// special cases for flying monsters, and other special guys
				if ((level.rmonsterinfo.mspawnspot[bestspot].monstertype == MONSTER_GEKK) &&
					(ACE_RANDOM < (0.5 + (skill->value * 0.05))))
				{
					mpick = MONSTER_GEKK; // ace - lets see more gekk
				}
				else if (level.rmonsterinfo.mspawnspot[bestspot].monstertype == MONSTER_FLYER)
				{
					mpick = MONSTER_FLYER;
					if (ACE_RANDOM < (0.1+(skill->value*0.1)))
					{
						mpick = MONSTER_KAMIKAZE;
					}
				}
				else if (level.rmonsterinfo.mspawnspot[bestspot].monstertype == MONSTER_KAMIKAZE)
				{
					mpick = MONSTER_FLYER;
					if (ACE_RANDOM < (0.5+(skill->value*0.05)))
					{
						mpick = MONSTER_KAMIKAZE;
					}
				}
				else if (level.rmonsterinfo.mspawnspot[bestspot].monstertype == MONSTER_FLOATER)
				{
					mpick = MONSTER_HOVER;
					if (ACE_RANDOM < (0.1+(skill->value*0.1)))
					{
						mpick = MONSTER_DAEDALUS;
					}
				}
				else if (level.rmonsterinfo.mspawnspot[bestspot].monstertype == MONSTER_HOVER)
				{
					mpick = MONSTER_HOVER;
					if (ACE_RANDOM < (0.25+(skill->value*0.1)))
					{
						mpick = MONSTER_DAEDALUS;
					}
				}
				else if (level.rmonsterinfo.mspawnspot[bestspot].monstertype == MONSTER_DAEDALUS)
				{
					mpick = MONSTER_HOVER;
					if (ACE_RANDOM < (0.5+(skill->value*0.05)))
					{
						mpick = MONSTER_DAEDALUS;
					}
				}
				// ace - if the spot is in water/liquid, spawn flipper (or chance of gekk)
				else if (gi.pointcontents(ent->s.origin) & MASK_WATER)
				{
					float chance = 0.25 + (skill->value * FRAMETIME);

					mpick = MONSTER_FLIPPER; // if water? start with ole flipper
					// setup chances for gekk water spawning
					if ((level.rmonsterinfo.mspawnspot[bestspot].monstertype == MONSTER_FLIPPER) && (ACE_RANDOM < chance))
					{
						mpick = MONSTER_GEKK;
					}
					else if ((level.rmonsterinfo.mspawnspot[bestspot].monstertype == MONSTER_GEKK) && (ACE_RANDOM < (chance/2)))
					{
						mpick = MONSTER_GEKK;
					}
					// ace - this is for "just incase" of a map where an enemy spawns underwater and its not a flipper/gekk
					if (level.rmonsterinfo.max_mspawns[mpick] < 1)
						level.rmonsterinfo.max_mspawns[mpick]++;
				}
				else
				{
					// ace - now the spawn is locked in, keep rerolling if its something we just spawned
					reroll:

					mpick = ace_GetRandomMTYPE();

					// ace - reroll if it just spawnd the same class, 'cept light soldiers
					if (last_mspawn == mpick && mpick != MONSTER_SOLDIER_LIGHT /* ||
					(level.rmonsterinfo.num_mspawns[mpick] > (level.rmonsterinfo.max_mspawns[mpick]+(int)skill->value))*/ )
					{
						gi.dprintf("ace_MonsterRandomizerGo: re-rolling class, for great justice!\n");
						goto reroll;
					}
				}

				gi.dprintf("ace_MonsterRandomizerGo: num_mspawns @ %i / %i \n", level.rmonsterinfo.num_mspawns[mpick],	level.rmonsterinfo.max_mspawns[mpick]);

				last_mspawn = mpick;	// keep track of last spawned to avoid dupes

				level.rmonsterinfo.num_mspawns[mpick]++;	// add to the queue
				level.rmonsterinfo.num_monsters++;
//				level.total_monsters++;
					
				ent->monstertype = mpick;
				ent->monstertier = 0;

				/* kill anything at the destination */
				KillBox(ent);

				// case switch: soldier, light, ss, lasergun, hypergun, ripper, infantry, gunner
				switch (mpick)
				{
				case MONSTER_SUPERTANK:
					ent->classname = "monster_supertank";				// set classname
					SP_monster_supertank (ent);
					break;
				case MONSTER_TANK_COMMANDER:
					ent->classname = "monster_tank_commander";				// set classname
					SP_monster_tank (ent);
					break;
				case MONSTER_TANK:
					ent->classname = "monster_tank";				// set classname
					SP_monster_tank (ent);
					break;
				case MONSTER_MEDIC_COMMANDER:
					ent->classname = "monster_medic_commander";				// set classname
					SP_monster_medic (ent);
					break;
				case MONSTER_MEDIC:
					ent->classname = "monster_medic";				// set classname
					SP_monster_medic (ent);
					break;
				case MONSTER_DAEDALUS:
					ent->classname = "monster_daedalus";				// set classname
					SP_monster_hover (ent);
					break;
				case MONSTER_HOVER:
					ent->classname = "monster_hover";				// set classname
					SP_monster_hover (ent);
					break;
				case MONSTER_KAMIKAZE:
					ent->classname = "monster_kamikaze";				// set classname
					SP_monster_kamikaze (ent);
					break;
				case MONSTER_FLYER:
					ent->classname = "monster_flyer";				// set classname
					SP_monster_flyer (ent);
					break;
				case MONSTER_GLADB:
					ent->classname = "monster_gladb";				// set classname
					SP_monster_gladiator (ent);
					break;
				case MONSTER_GLADIATOR:
					ent->classname = "monster_gladiator";				// set classname
					SP_monster_gladiator (ent);
					break;
				case MONSTER_MUTANT:
					ent->classname = "monster_mutant";				// set classname
					SP_monster_mutant (ent);
					break;
				case MONSTER_GEKK:
					ent->classname = "monster_gekk";				// set classname
					SP_monster_gekk (ent);
					break;
				case MONSTER_BERSERK:
					ent->classname = "monster_berserk";				// set classname
					SP_monster_berserk (ent);
					break;
				case MONSTER_SOLDIER_RIPPER:
					ent->classname = "monster_soldier_ripper";				// set classname
					SP_monster_soldier_ripper (ent);
					break;
				case MONSTER_SOLDIER_LASERGUN:
					ent->classname = "monster_soldier_lasergun";				// set classname
					SP_monster_soldier_lasergun (ent);
					break;
				case MONSTER_SOLDIER_HYPERGUN:
					ent->classname = "monster_soldier_hypergun";				// set classname
					SP_monster_soldier_hypergun (ent);
					break;
				case MONSTER_GUNNER:
					ent->classname = "monster_gunner";				// set classname
					SP_monster_gunner (ent);
					break;
				case MONSTER_SOLDIER_SS:
					ent->classname = "monster_soldier_ss";				// set classname
					SP_monster_soldier_ss (ent);
					break;
				case MONSTER_INFANTRY:
					ent->classname = "monster_infantry";				// set classname
					SP_monster_infantry (ent);
					break;
				case MONSTER_HANDLER:
					ent->classname = "monster_handler";				// set classname
					SP_monster_handler(ent);
					break;
				case MONSTER_HOUND:
					ent->classname = "monster_hound";				// set classname
					SP_monster_hound (ent);
					break;
				case MONSTER_SOLDIER:
					ent->classname = "monster_soldier";				// set classname
					SP_monster_soldier (ent);
					break;
				case MONSTER_STALKER:
					ent->classname = "monster_stalker";				// set classname
					SP_monster_stalker (ent);
					break;
				case MONSTER_SOLDIER_LIGHT:
				default:
					ent->classname = "monster_soldier_light";				// set classname
					SP_monster_soldier_light (ent);
					break;
				}

				gi.dprintf("ace_MonsterRandomizerGo: %s successfully spawned @ %s.\n", ent->classname, vtos(ent->s.origin));

				ent->s.event = /*EV_OTHER_TELEPORT*/ EV_PLAYER_TELEPORT; // ace - test spawn effect

/*				if (level.time > 5)
				{
					// show they "logged in" to the server
					gi.WriteByte (svc_muzzleflash);
					gi.WriteShort (ent-g_edicts);
					gi.WriteByte (MZ_LOGIN);
					gi.multicast (ent->s.origin, MULTICAST_PVS);
//					FindTarget(ent);
				} */
			}
		}
	}

	// randomly spawn insane guys around the map using "leftover" spawns
	if (level.rmonsterinfo.t0_spawned < level.time)
	{
		sd = level.num_wave + (ACE_RANDOM * level.skill_level) + (ACE_RANDOM * level.rmonsterinfo.max_wave_monsters);
		if (sd < 30)
		{
			sd = 30;
		}
		level.rmonsterinfo.t0_spawned = level.time + sd;

		count = (randk() & 3);

		if (count > level.rmonsterinfo.max_mspawns[MONSTER_INSANE])
			count = level.rmonsterinfo.max_mspawns[MONSTER_INSANE];

		if (count && 
			level.rmonsterinfo.num_mspawns[MONSTER_INSANE] < level.rmonsterinfo.max_mspawns[MONSTER_INSANE])
		{
			for (i = 0; i < count; i++)
			{
				bestspot = ace_FindMSpawn(); // see if anything returns, random pick

				if (bestspot == -1)
				{
					// ace - just bail if there are no spawn spots (for whatever reason)
					continue;
				}

				level.rmonsterinfo.mspawnspot[bestspot].timeout = level.time + ACE_RANDOM; // don't use this spot for # seconds

				level.rmonsterinfo.t0_spawned += ACE_RANDOM; // increase delay randomly
				
				ent = G_Spawn();
				if (!ent)
				{
					return;
				}

//				ent->target = level.rmonsterinfo.mspawnspot[bestspot].target;
//				ent->team = level.rmonsterinfo.mspawnspot[bestspot].team;
//				ent->targetname = level.rmonsterinfo.mspawnspot[bestspot].targetname;
//				ent->killtarget = level.rmonsterinfo.mspawnspot[bestspot].killtarget;
				ent->pathtarget = level.rmonsterinfo.mspawnspot[bestspot].pathtarget;
				ent->combattarget = level.rmonsterinfo.mspawnspot[bestspot].combattarget;
				ent->item = level.rmonsterinfo.mspawnspot[bestspot].item;							// set item
				ent->spawnflags = level.rmonsterinfo.mspawnspot[bestspot].spawnflags;				// set spawnflags
				VectorCopy (level.rmonsterinfo.mspawnspot[bestspot].origin, ent->s.origin);		// set origin
				VectorCopy (level.rmonsterinfo.mspawnspot[bestspot].angles, ent->s.angles);		// set angles

				/* kill anything at the destination */
				KillBox(ent);

				if (ACE_RANDOM < 0.6)
				{
					ent->spawnflags &= ~(1);
					ent->monsterinfo.aiflags |= AI_DO_NOT_COUNT|AI_GOOD_GUY;
					level.rmonsterinfo.num_mspawns[MONSTER_INSANE]++;	// add to the queue
					ent->monstertype = MONSTER_INSANE;
					ent->monstertier = 0;
					ent->classname = "misc_insane";				// set classname
					SP_misc_insane (ent);
				}
				else // or... just help blow stuff up!
				{
					ent->classname = "misc_explobox";
					SP_misc_explobox (ent);
				}

				// ace - if any monsters spawn near the "level.sight_entity" let em at it
/*				if (level.time > 5)
				{
					// show they "logged in" to the server
					gi.WriteByte (svc_muzzleflash);
					gi.WriteShort (ent-g_edicts);
					gi.WriteByte (MZ_LOGIN);
					gi.multicast (ent->s.origin, MULTICAST_PVS);

//					gi.dprintf("ace_MonsterRandomizerGo: %s successfully spawned @ %s.\n", ent->classname, vtos(ent->s.origin));
				} */
			}
		}
	}
}

/*
 * (ROGUE) Monster spawning code:
 * Used by the carrier, the medic_commander, and the black widow (ace - and something else)
 *
 * The sequence to create a flying monster is:
 *  FindSpawnPoint - tries to find suitable spot to spawn the monster in
 *  CreateFlyMonster  - this verifies the point as good and creates the monster
 *
 * To create a ground walking monster:
 *  FindSpawnPoint - same thing
 *  CreateGroundMonster - this checks the volume and makes sure the floor under the volume is suitable
 */

extern char *ED_NewString(const char *string);

edict_t *
CreateMonster(vec3_t origin, vec3_t angles, char *classname)
{
	edict_t *newEnt;

	if (!classname)
	{
		return NULL;
	}

	newEnt = G_Spawn();

	VectorCopy(origin, newEnt->s.origin);
	VectorCopy(angles, newEnt->s.angles);
	newEnt->classname = ED_NewString(classname);
	newEnt->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

	VectorSet(newEnt->gravityVector, 0, 0, -1);
	ED_CallSpawn(newEnt);
	newEnt->s.renderfx |= RF_IR_VISIBLE;

	return newEnt;
}

edict_t *
CreateFlyMonster(vec3_t origin, vec3_t angles, vec3_t mins,
		vec3_t maxs, char *classname)
{
	if (!classname)
	{
		return NULL;
	}

	if (!mins || !maxs ||
		VectorCompare(mins, vec3_origin) || VectorCompare(maxs, vec3_origin))
	{
		DetermineBBox(classname, mins, maxs);
	}

	if (!CheckSpawnPoint(origin, mins, maxs))
	{
		return NULL;
	}

	return CreateMonster(origin, angles, classname);
}

edict_t *
CreateGroundMonster(vec3_t origin, vec3_t angles, vec3_t entMins,
		vec3_t entMaxs, char *classname, int height)
{
	edict_t *newEnt;
	vec3_t mins, maxs;

	if (!classname)
	{
		return NULL;
	}

	/* if they don't provide us a bounding box, figure it out */
	if (!entMins || !entMaxs || VectorCompare(entMins,
				vec3_origin) || VectorCompare(entMaxs, vec3_origin))
	{
		DetermineBBox(classname, mins, maxs);
	}
	else
	{
		VectorCopy(entMins, mins);
		VectorCopy(entMaxs, maxs);
	}

	/* check the ground to make sure it's there, it's relatively flat, and it's not toxic */
	if (!CheckGroundSpawnPoint(origin, mins, maxs, height, -1))
	{
		return NULL;
	}

	newEnt = CreateMonster(origin, angles, classname);

	if (!newEnt)
	{
		return NULL;
	}

	return newEnt;
}

qboolean
FindSpawnPoint(vec3_t startpoint, vec3_t mins, vec3_t maxs,
		vec3_t spawnpoint, float maxMoveUp)
{
	trace_t tr;
	vec3_t top;

	tr = gi.trace(startpoint, mins, maxs, startpoint,
			NULL, MASK_MONSTERSOLID | CONTENTS_PLAYERCLIP);

	if ((tr.startsolid || tr.allsolid) || (tr.ent != world))
	{
		VectorCopy(startpoint, top);
		top[2] += maxMoveUp;

		tr = gi.trace(top, mins, maxs, startpoint, NULL, MASK_MONSTERSOLID);

		if (tr.startsolid || tr.allsolid)
		{
			return false;
		}
		else
		{
			VectorCopy(tr.endpos, spawnpoint);
			return true;
		}
	}
	else
	{
		VectorCopy(startpoint, spawnpoint);
		return true;
	}
}

qboolean
CheckSpawnPoint(vec3_t origin, vec3_t mins, vec3_t maxs)
{
	trace_t tr;

	if (!mins || !maxs ||
		VectorCompare(mins, vec3_origin) || VectorCompare(maxs, vec3_origin))
	{
		return false;
	}

	tr = gi.trace(origin, mins, maxs, origin, NULL, MASK_MONSTERSOLID);

	if (tr.startsolid || tr.allsolid)
	{
		return false;
	}

	if (tr.ent != world)
	{
		return false;
	}

	return true;
}

qboolean
CheckGroundSpawnPoint(vec3_t origin, vec3_t entMins, vec3_t entMaxs,
		float height, float gravity)
{
	trace_t tr;
	vec3_t start, stop;
	vec3_t mins, maxs;
	int x, y;
	float mid, bottom;

	if (!CheckSpawnPoint(origin, entMins, entMaxs))
	{
		return false;
	}


	VectorCopy(origin, stop);
	stop[2] = origin[2] + entMins[2] - height;

	tr = gi.trace(origin, entMins, entMaxs, stop,
			NULL, MASK_MONSTERSOLID | MASK_WATER);

	if ((tr.fraction < 1) && (tr.contents & MASK_MONSTERSOLID))
	{
		/* first, do the midpoint trace */
		VectorAdd(tr.endpos, entMins, mins);
		VectorAdd(tr.endpos, entMaxs, maxs);

		/* first, do the easy flat check */
		if (gravity > 0)
		{
			start[2] = maxs[2] + 1;
		}
		else
		{
			start[2] = mins[2] - 1;
		}

		for (x = 0; x <= 1; x++)
		{
			for (y = 0; y <= 1; y++)
			{
				start[0] = x ? maxs[0] : mins[0];
				start[1] = y ? maxs[1] : mins[1];

				if (gi.pointcontents(start) != CONTENTS_SOLID)
				{
					goto realcheck;
				}
			}
		}

		/* if it passed all four above checks, we're done */
		return true;

	realcheck:

		/* check it for real */
		start[0] = stop[0] = (mins[0] + maxs[0]) * 0.5;
		start[1] = stop[1] = (mins[1] + maxs[1]) * 0.5;
		start[2] = mins[2];

		tr = gi.trace(start, vec3_origin, vec3_origin,
				stop, NULL, MASK_MONSTERSOLID);

		if (tr.fraction == 1.0)
		{
			return false;
		}

		if (gravity < 0)
		{
			start[2] = mins[2];
			stop[2] = start[2] - STEPSIZE - STEPSIZE;
			mid = bottom = tr.endpos[2] + entMins[2];
		}
		else
		{
			start[2] = maxs[2];
			stop[2] = start[2] + STEPSIZE + STEPSIZE;
			mid = bottom = tr.endpos[2] - entMaxs[2];
		}

		for (x = 0; x <= 1; x++)
		{
			for (y = 0; y <= 1; y++)
			{
				start[0] = stop[0] = x ? maxs[0] : mins[0];
				start[1] = stop[1] = y ? maxs[1] : mins[1];

				tr = gi.trace(start, vec3_origin, vec3_origin,
						stop, NULL, MASK_MONSTERSOLID);

				if (gravity > 0)
				{
					if ((tr.fraction != 1.0) && (tr.endpos[2] < bottom))
					{
						bottom = tr.endpos[2];
					}

					if ((tr.fraction == 1.0) || (tr.endpos[2] - mid > STEPSIZE))
					{
						return false;
					}
				}
				else
				{
					if ((tr.fraction != 1.0) && (tr.endpos[2] > bottom))
					{
						bottom = tr.endpos[2];
					}

					if ((tr.fraction == 1.0) || (mid - tr.endpos[2] > STEPSIZE))
					{
						return false;
					}
				}
			}
		}

		return true; /* we can land on it, it's ok */
	}

	/* otherwise, it's either water (bad) or not
	 * there (too far) if we're here, it's bad below */
	return false;
}

void
DetermineBBox(char *classname, vec3_t mins, vec3_t maxs)
{
	edict_t *newEnt;

	if (!classname)
	{
		return;
	}

	newEnt = G_Spawn();

	VectorCopy(vec3_origin, newEnt->s.origin);
	VectorCopy(vec3_origin, newEnt->s.angles);
	newEnt->classname = ED_NewString(classname);
	newEnt->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

	ED_CallSpawn(newEnt);

	VectorCopy(newEnt->mins, mins);
	VectorCopy(newEnt->maxs, maxs);

	G_FreeEdict(newEnt);
}


void
spawngrow_think(edict_t *self)
{
	int i;

	if (!self)
	{
		return;
	}

	for (i = 0; i < 2; i++)
	{
		self->s.angles[0] = rand() % 360;
		self->s.angles[1] = rand() % 360;
		self->s.angles[2] = rand() % 360;
	}

	if ((level.time < self->wait) && (self->s.frame < 2))
	{
		self->s.frame++;
	}

	if (level.time >= self->wait)
	{
		if (self->s.effects & EF_SPHERETRANS)
		{
			G_FreeEdict(self);
			return;
		}
		else if (self->s.frame > 0)
		{
			self->s.frame--;
		}
		else
		{
			G_FreeEdict(self);
			return;
		}
	}

	self->nextthink += FRAMETIME;
}

void
SpawnGrow_Spawn(vec3_t startpos, int size)
{
	edict_t *ent;
	int i;
	float lifespan;

	ent = G_Spawn();
	VectorCopy(startpos, ent->s.origin);

	for (i = 0; i < 2; i++)
	{
		ent->s.angles[0] = rand() % 360;
		ent->s.angles[1] = rand() % 360;
		ent->s.angles[2] = rand() % 360;
	}

	ent->solid = SOLID_NOT;
	ent->s.renderfx = RF_IR_VISIBLE;
	ent->movetype = MOVETYPE_NONE;
	ent->classname = "spawngro";

	if (size <= 1)
	{
		lifespan = SPAWNGROW_LIFESPAN;
		ent->s.modelindex = gi.modelindex("models/items/spawngro2/tris.md2");
	}
	else if (size == 2)
	{
		ent->s.modelindex = gi.modelindex("models/items/spawngro3/tris.md2");
		lifespan = 2;
	}
	else
	{
		ent->s.modelindex = gi.modelindex("models/items/spawngro/tris.md2");
		lifespan = SPAWNGROW_LIFESPAN;
	}

	ent->think = spawngrow_think;

	ent->wait = level.time + lifespan;
	ent->nextthink = level.time + FRAMETIME;

	if (size != 2)
	{
		ent->s.effects |= EF_SPHERETRANS;
	}

	gi.linkentity(ent);
}

void
widowlegs_think(edict_t *self)
{
	vec3_t offset;
	vec3_t point;
	vec3_t f, r, u;

	if (!self)
	{
		return;
	}

	if (self->s.frame == 17)
	{
		VectorSet(offset, 11.77, -7.24, 23.31);
		AngleVectors(self->s.angles, f, r, u);
		G_ProjectSource2(self->s.origin, offset, f, r, u, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL);
		ThrowSmallStuff(self, point);
	}

	if (self->s.frame < MAX_LEGSFRAME)
	{
		self->s.frame++;
		self->nextthink = level.time + FRAMETIME;
		return;
	}
	else if (self->wait == 0)
	{
		self->wait = level.time + LEG_WAIT_TIME;
	}

	if (level.time > self->wait)
	{
		AngleVectors(self->s.angles, f, r, u);

		VectorSet(offset, -65.6, -8.44, 28.59);
		G_ProjectSource2(self->s.origin, offset, f, r, u, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL);
		ThrowSmallStuff(self, point);

		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib1/tris.md2",
				80 + (int)(random() * 20.0), GIB_METALLIC, point, 0, true);
		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib2/tris.md2",
				80 + (int)(random() * 20.0), GIB_METALLIC, point, 0, true);

		VectorSet(offset, -1.04, -51.18, 7.04);
		G_ProjectSource2(self->s.origin, offset, f, r, u, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL);
		ThrowSmallStuff(self, point);

		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib1/tris.md2",
				80 + (int)(random() * 20.0), GIB_METALLIC, point, 0, true);
		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib2/tris.md2",
				80 + (int)(random() * 20.0), GIB_METALLIC, point, 0, true);
		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib3/tris.md2",
				80 + (int)(random() * 20.0), GIB_METALLIC, point, 0, true);

		G_FreeEdict(self);
		return;
	}

	if ((level.time > (self->wait - 0.5)) && (self->count == 0))
	{
		self->count = 1;
		AngleVectors(self->s.angles, f, r, u);

		VectorSet(offset, 31, -88.7, 10.96);
		G_ProjectSource2(self->s.origin, offset, f, r, u, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL);

		VectorSet(offset, -12.67, -4.39, 15.68);
		G_ProjectSource2(self->s.origin, offset, f, r, u, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL);

		self->nextthink = level.time + FRAMETIME;
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

void
Widowlegs_Spawn(vec3_t startpos, vec3_t angles)
{
	edict_t *ent;

	ent = G_Spawn();
	VectorCopy(startpos, ent->s.origin);
	VectorCopy(angles, ent->s.angles);
	ent->solid = SOLID_NOT;
	ent->s.renderfx = RF_IR_VISIBLE;
	ent->movetype = MOVETYPE_NONE;
	ent->classname = "widowlegs";

	ent->s.modelindex = gi.modelindex("models/monsters/legs/tris.md2");
	ent->think = widowlegs_think;

	ent->nextthink = level.time + FRAMETIME;
	gi.linkentity(ent);
}