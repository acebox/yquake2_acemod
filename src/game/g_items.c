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
 * Item handling and item definitions.
 *
 * =======================================================================
 */

#include "header/local.h"

#define HEALTH_IGNORE_MAX 1
#define HEALTH_TIMED 2

qboolean Pickup_Weapon(edict_t *ent, edict_t *other);
void Use_Weapon(edict_t *ent, gitem_t *inv);
void Drop_Weapon(edict_t *ent, gitem_t *inv);

void Weapon_Blaster(edict_t *ent);
void Weapon_Shotgun(edict_t *ent);
void Weapon_SuperShotgun(edict_t *ent);
void Weapon_Machinegun(edict_t *ent);
void Weapon_Chaingun(edict_t *ent);
void Weapon_HyperBlaster(edict_t *ent);
void Weapon_RocketLauncher(edict_t *ent);
void Weapon_Grenade(edict_t *ent);
void Weapon_GrenadeLauncher(edict_t *ent);
void Weapon_Railgun(edict_t *ent);
void Weapon_BFG(edict_t *ent);

gitem_armor_t jacketarmor_info = {25, 50, .30, .00, ARMOR_JACKET};
gitem_armor_t combatarmor_info = {50, 100, .60, .30, ARMOR_COMBAT};
gitem_armor_t bodyarmor_info = {100, 200, .80, .60, ARMOR_BODY};

int jacket_armor_index;
int combat_armor_index;
int body_armor_index;
static int power_screen_index;
static int power_shield_index;

void Use_Quad(edict_t *ent, gitem_t *item);
static int quad_drop_timeout_hack;

/* ====================================================================== */

gitem_t *
GetItemByIndex(int index)
{
	if ((index == 0) || (index >= game.num_items))
	{
		return NULL;
	}

	return &itemlist[index];
}

gitem_t *
FindItemByClassname(char *classname)
{
	int i;
	gitem_t *it;

	if (!classname)
	{
		return NULL;
	}

	it = itemlist;

	for (i = 0; i < game.num_items; i++, it++)
	{
		if (!it->classname)
		{
			continue;
		}

		if (!Q_stricmp(it->classname, classname))
		{
			return it;
		}
	}

	return NULL;
}

gitem_t *
FindItem(char *pickup_name)
{
	int i;
	gitem_t *it;

	if (!pickup_name)
	{
		return NULL;
	}

	it = itemlist;

	for (i = 0; i < game.num_items; i++, it++)
	{
		if (!it->pickup_name)
		{
			continue;
		}

		if (!Q_stricmp(it->pickup_name, pickup_name))
		{
			return it;
		}
	}

	return NULL;
}

/* ====================================================================== */

void
DoRespawn(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (ent->team)
	{
		edict_t *master;
		int count;
		int choice;

		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->chain, count++)
		{
		}

		choice = count ? randk() % count : 0;

		for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
		{
		}
	}

	ent->svflags &= ~SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	gi.linkentity(ent);

	/* send an effect */
	ent->s.event = EV_ITEM_RESPAWN;
}

void
SetRespawn(edict_t *ent, float delay)
{
	if (!ent)
	{
		return;
	}

	ent->flags |= FL_RESPAWN;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->nextthink = level.time + delay;
	ent->think = DoRespawn;
	gi.linkentity(ent);
}

/* ====================================================================== */

qboolean
Pickup_Powerup(edict_t *ent, edict_t *other)
{
	int quantity;

	if (!ent || !other)
	{
		return false;
	}

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	if (((skill->value == 1) &&
		 (quantity >= 2)) || ((skill->value >= 2) && (quantity >= 1)))
	{
		return false;
	}

	if ((coop->value) && (ent->item->flags & IT_STAY_COOP) && (quantity > 0))
	{
		return false;
	}

	// ace - monster randomizer - on power pickup, reduce monster spawning timer.  
	// on higher skill level, chance of resetting to a random timer.
	if (level.skill_level > 1)
	{
		if (ACE_RANDOM < 0.9)
		{
			if (random() < 0.5)
				level.rmonsterinfo.t1_spawned = level.time + ACE_RANDOM;
			else
				level.rmonsterinfo.t1_spawned = level.time + level.skill_level + ACE_RANDOM;
		}
	}
	else if (ACE_RANDOM < 0.5)
		level.rmonsterinfo.t1_spawned -= ACE_RANDOM;
	// ace - monster randomizer - end

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	// ace - allow respawn and dmflags when coop (and wouldnt respawn), or non-coop sp/dm
//	if (deathmatch->value)
	if (((coop->value) && !(ent->item->flags & IT_STAY_COOP)) || (coop->value == 0))
	{
		if (!(ent->spawnflags & DROPPED_ITEM))
		{
			SetRespawn(ent, ent->item->quantity);
		}

		if (((int)dmflags->value & DF_INSTANT_ITEMS) ||
			((ent->item->use == Use_Quad) &&
			 (ent->spawnflags & DROPPED_PLAYER_ITEM)))
		{
			if ((ent->item->use == Use_Quad) &&
				(ent->spawnflags & DROPPED_PLAYER_ITEM))
			{
				quad_drop_timeout_hack =
					(ent->nextthink - level.time) / FRAMETIME;
			}

			ent->item->use(other, ent->item);
		}
	}

	return true;
}

void
Drop_General(edict_t *ent, gitem_t *item)
{
	Drop_Item(ent, item);
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);
}

/* ====================================================================== */
// ace - this should have some kind of "temporary" speed boost, if anything.
qboolean
Pickup_Adrenaline(edict_t *ent, edict_t *other)
{
	if (!ent || !other)
	{
		return false;
	}

	if (!deathmatch->value)
	{
		other->max_health += 1;
		// ace - monster randomizer - on power pickup, reduce monster spawning timer.  
		// on higher skill level, chance of resetting to a random timer.
		if ((ACE_RANDOM < 0.25) && (level.skill_level > 1))
		{
			if (ACE_RANDOM < 0.1)
				level.rmonsterinfo.t1_spawned = level.time + (ACE_RANDOM*5);
			else
				level.rmonsterinfo.t1_spawned = level.time + level.skill_level + (ACE_RANDOM*5);
		}
		// ace - monster randomizer - end
	}

	if (other->health < other->max_health)
	{
		other->health = other->max_health;
	}

	if (!(ent->spawnflags & DROPPED_ITEM)/* && (deathmatch->value)*/)
	{
		SetRespawn(ent, ent->item->quantity);
	}

	return true;
}

qboolean
Pickup_AncientHead(edict_t *ent, edict_t *other)
{
	if (!ent || !other)
	{
		return false;
	}

	// ace - monster randomizer - on power pickup, reduce monster spawning timer.  
	// on higher skill level, chance of resetting to a random timer.
	if ((ACE_RANDOM < 0.5) && (level.skill_level > 1))
	{
		if (ACE_RANDOM < 0.25)
			level.rmonsterinfo.t1_spawned = level.time + (ACE_RANDOM*2.5);
		else
			level.rmonsterinfo.t1_spawned = level.time + level.skill_level + (ACE_RANDOM*2.5);
	}
	// ace - monster randomizer - end


	other->max_health += 2;

	if (!(ent->spawnflags & DROPPED_ITEM)/* && (deathmatch->value)*/)
	{
		SetRespawn(ent, ent->item->quantity);
	}

	return true;
}

// ace - changed behavior, each bandolier (and backpack) will increase max ammo by # ammount, untill capping.
qboolean
Pickup_Bandolier(edict_t *ent, edict_t *other)
{
	gitem_t *item;
	int index;

	if (!ent || !other)
	{
		return false;
	}

	// ace - monster randomizer - on power pickup, reduce monster spawning timer.  
	// on higher skill level, chance of resetting to a random timer.
	if ((ACE_RANDOM < 0.1) && (level.skill_level > 1))
	{
		if (ACE_RANDOM < 0.1)
			level.rmonsterinfo.t1_spawned = level.time + (ACE_RANDOM*5);
		else
			level.rmonsterinfo.t1_spawned = level.time + level.skill_level + (ACE_RANDOM*5);
	}
	// ace - monster randomizer - end

	if (other->client->pers.max_bullets < MAX_BULLETS_BAND)
	{
		other->client->pers.max_bullets += INC_BULLETS_BAND;
	}

	if (other->client->pers.max_shells < MAX_SHELLS_BAND)
	{
		other->client->pers.max_shells += INC_SHELLS_BAND;
	}

	if (other->client->pers.max_cells < MAX_CELLS_BAND)
	{
		other->client->pers.max_cells += INC_CELLS_BAND;
	}

	if (other->client->pers.max_slugs < MAX_SLUGS_BAND)
	{
		other->client->pers.max_slugs += INC_SLUGS_BAND;
	}

// ace - additional ammo
	if (other->client->pers.max_magslug < MAX_MAGSLUG_BAND)
	{
		other->client->pers.max_magslug += INC_MAGSLUG_BAND;
	}
	if (other->client->pers.max_flechettes < INC_MAGSLUG_BAND)
	{
		other->client->pers.max_flechettes += INC_FLECHETTES_BAND;
	}
// ace - additional ammo


	item = FindItem("Bullets");

	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] >
			other->client->pers.max_bullets)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_bullets;
		}
	}

	item = FindItem("Shells");

	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] >
			other->client->pers.max_shells)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_shells;
		}
	}

	item = FindItem("Flechettes");

	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] >
			other->client->pers.max_flechettes)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_flechettes;
		}
	}

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
	{
		SetRespawn(ent, ent->item->quantity);
	}

	return true;
}

qboolean
Pickup_Pack(edict_t *ent, edict_t *other)
{
	gitem_t *item;
	int index;

	if (!ent || !other)
	{
		return false;
	}

	// ace - monster randomizer - on power pickup, reduce monster spawning timer.  
	// on higher skill level, chance of resetting to a random timer.
	if ((ACE_RANDOM < 0.25) && (level.skill_level > 1))
	{
		if (ACE_RANDOM < 0.1)
			level.rmonsterinfo.t1_spawned = level.time + (ACE_RANDOM*5);
		else
			level.rmonsterinfo.t1_spawned = level.time + level.skill_level + (ACE_RANDOM*5);
	}
	// ace - monster randomizer - end

	if (other->client->pers.max_bullets < MAX_BULLETS_PACK)
	{
		other->client->pers.max_bullets += INC_BULLETS_PACK;
	}

	if (other->client->pers.max_shells < MAX_SHELLS_PACK)
	{
		other->client->pers.max_shells = INC_SHELLS_PACK;
	}

	if (other->client->pers.max_rockets < MAX_ROCKETS_PACK)
	{
		other->client->pers.max_rockets += INC_ROCKETS_PACK;
	}

	if (other->client->pers.max_grenades < MAX_GRENADES_PACK)
	{
		other->client->pers.max_grenades += INC_GRENADES_PACK;
	}

	if (other->client->pers.max_cells < MAX_CELLS_PACK)
	{
		other->client->pers.max_cells += INC_CELLS_PACK;
	}

	if (other->client->pers.max_slugs < MAX_SLUGS_PACK)
	{
		other->client->pers.max_slugs += INC_SLUGS_PACK;
	}

// ace - additional ammo (magslug, flechettes, prox, tesla, trap)
	if (other->client->pers.max_magslug < MAX_MAGSLUG_PACK)
	{
		other->client->pers.max_magslug += INC_MAGSLUG_PACK;
	}
	if (other->client->pers.max_flechettes < MAX_FLECHETTES_PACK)
	{
		other->client->pers.max_flechettes += INC_FLECHETTES_PACK;
	}
	if (other->client->pers.max_prox < MAX_PROX_PACK)
	{
		other->client->pers.max_prox += INC_PROX_PACK;
	}
	if (other->client->pers.max_tesla < MAX_TESLA_PACK)
	{
		other->client->pers.max_tesla += INC_TESLA_PACK;
	}
	if (other->client->pers.max_trap < MAX_TRAP_PACK)
	{
		other->client->pers.max_trap += INC_TRAP_PACK;
	}
// ace - additional ammo

	item = FindItem("Bullets");

	if (item)
	{	// ace - double pickup
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += (item->quantity*2);

		if (other->client->pers.inventory[index] >
			other->client->pers.max_bullets)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_bullets;
		}
	}

	item = FindItem("Shells");

	if (item)
	{	// ace - double pickup
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += (item->quantity*2);

		if (other->client->pers.inventory[index] >
			other->client->pers.max_shells)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_shells;
		}
	}

	item = FindItem("Cells");

	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] >
			other->client->pers.max_cells)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_cells;
		}
	}

	item = FindItem("Grenades");

	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] >
			other->client->pers.max_grenades)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_grenades;
		}
	}

	item = FindItem("Rockets");

	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] >
			other->client->pers.max_rockets)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_rockets;
		}
	}

	item = FindItem("Slugs");

	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] >
			other->client->pers.max_slugs)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_slugs;
		}
	}

// ace - additional ammo
	item = FindItem("Flechettes");

	if (item)
	{	// ace - double pickup
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += (item->quantity*2);

		if (other->client->pers.inventory[index] >
			other->client->pers.max_flechettes)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_flechettes;
		}
	}

	item = FindItem("Mag Slug");

	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] >
			other->client->pers.max_magslug)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_magslug;
		}
	}

	item = FindItem("Prox");

	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] >
			other->client->pers.max_prox)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_prox;
		}
	}

	item = FindItem("Tesla");

	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] >
			other->client->pers.max_tesla)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_tesla;
		}
	}

	item = FindItem("Trap");

	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] >
			other->client->pers.max_trap)
		{
			other->client->pers.inventory[index] =
				other->client->pers.max_trap;
		}
	}
// ace - additional ammo


	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
	{
		SetRespawn(ent, ent->item->quantity);
	}

	return true;
}

/* ====================================================================== */

void
Use_Quad(edict_t *ent, gitem_t *item)
{
	int timeout;

	if (!ent || !item)
	{
		return;
	}

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (quad_drop_timeout_hack)
	{
		timeout = quad_drop_timeout_hack;
		quad_drop_timeout_hack = 0;
	}
	else
	{
		timeout = 300;
	}

	if (ent->client->quad_framenum > level.framenum)
	{
		ent->client->quad_framenum += timeout;
	}
	else
	{
		ent->client->quad_framenum = level.framenum + timeout;
	}

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

/* ====================================================================== */

void
Use_Breather(edict_t *ent, gitem_t *item)
{
	if (!ent || !item)
	{
		return;
	}

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (ent->client->breather_framenum > level.framenum)
	{
		ent->client->breather_framenum += 300;
	}
	else
	{
		ent->client->breather_framenum = level.framenum + 300;
	}
}

/* ====================================================================== */

void
Use_Envirosuit(edict_t *ent, gitem_t *item)
{
	if (!ent || !item)
	{
		return;
	}

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (ent->client->enviro_framenum > level.framenum)
	{
		ent->client->enviro_framenum += 300;
	}
	else
	{
		ent->client->enviro_framenum = level.framenum + 300;
	}
}

/* ====================================================================== */

void
Use_Invulnerability(edict_t *ent, gitem_t *item)
{
	if (!ent || !item)
	{
		return;
	}

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (ent->client->invincible_framenum > level.framenum)
	{
		ent->client->invincible_framenum += 300;
	}
	else
	{
		ent->client->invincible_framenum = level.framenum + 300;
	}

	gi.sound(ent, CHAN_ITEM, gi.soundindex( "items/protect.wav"), 1, ATTN_NORM, 0);
}

/* ====================================================================== */

void
Use_Silencer(edict_t *ent, gitem_t *item)
{
	if (!ent || !item)
	{
		return;
	}

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);
	ent->client->silencer_shots += 30;
}

/* ====================================================================== */

qboolean
Pickup_Key(edict_t *ent, edict_t *other)
{
	if (!ent || !other)
	{
		return false;
	}

	if (coop->value)
	{
		if (strcmp(ent->classname, "key_power_cube") == 0)
		{
			if (other->client->pers.power_cubes &
				((ent->spawnflags & 0x0000ff00) >> 8))
			{
				return false;
			}

			other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
			other->client->pers.power_cubes |=
				((ent->spawnflags & 0x0000ff00) >> 8);
		}
		else
		{
			if (other->client->pers.inventory[ITEM_INDEX(ent->item)])
			{
				return false;
			}

			other->client->pers.inventory[ITEM_INDEX(ent->item)] = 1;
		}

		return true;
	}

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
	return true;
}

/* ====================================================================== */

qboolean
Add_Ammo(edict_t *ent, gitem_t *item, int count)
{
	int index;
	int max;

	if (!ent || !item)
	{
		return false;
	}

	if (!ent->client)
	{
		return false;
	}

	if (item->tag == AMMO_BULLETS)
	{
		max = ent->client->pers.max_bullets;
	}
	else if (item->tag == AMMO_SHELLS)
	{
		max = ent->client->pers.max_shells;
	}
	else if (item->tag == AMMO_ROCKETS)
	{
		max = ent->client->pers.max_rockets;
	}
	else if (item->tag == AMMO_GRENADES)
	{
		max = ent->client->pers.max_grenades;
	}
	else if (item->tag == AMMO_CELLS)
	{
		max = ent->client->pers.max_cells;
	}
	else if (item->tag == AMMO_SLUGS)
	{
		max = ent->client->pers.max_slugs;
	}
// ace - additional ammo
	else if (item->tag == AMMO_MAGSLUG)
	{
		max = ent->client->pers.max_magslug;
	}
	else if (item->tag == AMMO_TRAP)
	{
		max = ent->client->pers.max_trap;
	}
	else if (item->tag == AMMO_FLECHETTES)
	{
		max = ent->client->pers.max_flechettes;
	}
	else if (item->tag == AMMO_PROX)
	{
		max = ent->client->pers.max_prox;
	}
	else if (item->tag == AMMO_TESLA)
	{
		max = ent->client->pers.max_tesla;
	}
// ace - additional ammo
	else
	{
		return false;
	}

	index = ITEM_INDEX(item);

	if (ent->client->pers.inventory[index] == max)
	{
		return false;
	}

	ent->client->pers.inventory[index] += count;

	if (ent->client->pers.inventory[index] > max)
	{
		ent->client->pers.inventory[index] = max;
	}

	return true;
}

qboolean
Pickup_Ammo(edict_t *ent, edict_t *other)
{
	int oldcount;
	int count;
	qboolean weapon;

	if (!ent || !other)
	{
		return false;
	}

	weapon = (ent->item->flags & IT_WEAPON);

	if ((weapon) && ((int)dmflags->value & DF_INFINITE_AMMO))
	{
		count = 1000;
	}
	else if (ent->count)
	{
		count = ent->count;
	}
	else
	{
		count = ent->item->quantity;
	}

	oldcount = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	if (!Add_Ammo(other, ent->item, count))
	{
		return false;
	}

	if (weapon && !oldcount)
	{
		/* don't switch to tesla */
		if ((other->client->pers.weapon != ent->item) &&
			(!deathmatch->value || (other->client->pers.weapon == FindItem("blaster"))) &&
			(strcmp(ent->classname, "ammo_tesla")))
		{
			other->client->newweapon = ent->item;
		}
	}

	if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) &&
		(deathmatch->value))
	{
		SetRespawn(ent, 30);
	}

	return true;
}

void
Drop_Ammo(edict_t *ent, gitem_t *item)
{
	edict_t *dropped;
	int index;

	if (!ent || !item)
	{
		return;
	}

	index = ITEM_INDEX(item);
	dropped = Drop_Item(ent, item);

	if (ent->client->pers.inventory[index] >= item->quantity)
	{
		dropped->count = item->quantity;
	}
	else
	{
		dropped->count = ent->client->pers.inventory[index];
	}

	if (ent->client->pers.weapon &&
		(ent->client->pers.weapon->tag == AMMO_GRENADES) &&
		(item->tag == AMMO_GRENADES) &&
		(ent->client->pers.inventory[index] - dropped->count <= 0))
	{
		gi.cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
		G_FreeEdict(dropped);
		return;
	}

	ent->client->pers.inventory[index] -= dropped->count;
	ValidateSelectedItem(ent);
}

/* ====================================================================== */

void
MegaHealth_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->owner->health > self->owner->max_health)
	{
		self->nextthink = level.time + 1;
		self->owner->health -= 1;
		return;
	}

	if (!(self->spawnflags & DROPPED_ITEM) && (deathmatch->value))
	{
		SetRespawn(self, 20);
	}
	else
	{
		G_FreeEdict(self);
	}
}

qboolean
Pickup_Health(edict_t *ent, edict_t *other)
{
	if (!ent || !other)
	{
		return false;
	}

	if (!(ent->style & HEALTH_IGNORE_MAX))
	{
		if (other->health >= other->max_health)
		{
			return false;
		}
	}

	other->health += ent->count;

	if (!(ent->style & HEALTH_IGNORE_MAX))
	{
		if (other->health > other->max_health)
		{
			other->health = other->max_health;
		}
	}

	if (ent->style & HEALTH_TIMED)
	{
		// ace - monster randomizer - on power pickup, reduce monster spawning timer.  
		// on higher skill level, chance of resetting to a random timer.
		if ((ACE_RANDOM < 0.5) && (level.skill_level > 1))
		{
			if (ACE_RANDOM < 0.25)
				level.rmonsterinfo.t1_spawned = level.time + ACE_RANDOM;
			else
				level.rmonsterinfo.t1_spawned = level.time + level.skill_level + ACE_RANDOM;
		}
		else if (ACE_RANDOM < 0.25)
			level.rmonsterinfo.t1_spawned -= ACE_RANDOM;
		// ace - monster randomizer - end

		ent->think = MegaHealth_think;
		ent->nextthink = level.time + 5;
		ent->owner = other;
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	else
	{
		if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		{
			SetRespawn(ent, 30);
		}
	}

	return true;
}

/* ====================================================================== */

int
ArmorIndex(edict_t *ent)
{
	if (!ent)
	{
		return 0;
	}

	if (!ent->client)
	{
		return 0;
	}

	if (ent->client->pers.inventory[jacket_armor_index] > 0)
	{
		return jacket_armor_index;
	}

	if (ent->client->pers.inventory[combat_armor_index] > 0)
	{
		return combat_armor_index;
	}

	if (ent->client->pers.inventory[body_armor_index] > 0)
	{
		return body_armor_index;
	}

	return 0;
}

qboolean
Pickup_Armor(edict_t *ent, edict_t *other)
{
	int old_armor_index;
	gitem_armor_t *oldinfo;
	gitem_armor_t *newinfo;
	int newcount;
	float salvage;
	int salvagecount;

	if (!ent || !other)
	{
		return false;
	}

	/* get info on new armor */
	newinfo = (gitem_armor_t *)ent->item->info;

	old_armor_index = ArmorIndex(other);

	/* handle armor shards specially */
	if (ent->item->tag == ARMOR_SHARD)
	{
		if (!old_armor_index)
		{
			other->client->pers.inventory[jacket_armor_index] = 2;
		}
		else
		{
			other->client->pers.inventory[old_armor_index] += 2;
		}
	}
	else if (!old_armor_index) /* if player has no armor, just use it */
	{
		other->client->pers.inventory[ITEM_INDEX(ent->item)] =
			newinfo->base_count;
	}
	else /* use the better armor */
	{
		/* get info on old armor */
		if (old_armor_index == jacket_armor_index)
		{
			oldinfo = &jacketarmor_info;
		}
		else if (old_armor_index == combat_armor_index)
		{
			oldinfo = &combatarmor_info;
		}
		else
		{
			oldinfo = &bodyarmor_info;
		}

		if (newinfo->normal_protection > oldinfo->normal_protection)
		{
			/* calc new armor values */
			salvage = oldinfo->normal_protection / newinfo->normal_protection;
			salvagecount = salvage *
						   other->client->pers.inventory[old_armor_index];
			newcount = newinfo->base_count + salvagecount;

			if (newcount > newinfo->max_count)
			{
				newcount = newinfo->max_count;
			}

			/* zero count of old armor so it goes away */
			other->client->pers.inventory[old_armor_index] = 0;

			/* change armor to new item with computed value */
			other->client->pers.inventory[ITEM_INDEX(ent->item)] = newcount;
		}
		else
		{
			/* calc new armor values */
			salvage = newinfo->normal_protection / oldinfo->normal_protection;
			salvagecount = salvage * newinfo->base_count;
			newcount = other->client->pers.inventory[old_armor_index] +
					   salvagecount;

			if (newcount > oldinfo->max_count)
			{
				newcount = oldinfo->max_count;
			}

			/* if we're already maxed out then we don't need the new armor */
			if (other->client->pers.inventory[old_armor_index] >= newcount)
			{
				return false;
			}

			/* update current armor value */
			other->client->pers.inventory[old_armor_index] = newcount;
		}
	}

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
	{
		SetRespawn(ent, 20);
	}

	return true;
}

/* ====================================================================== */

int
PowerArmorType(edict_t *ent)
{
	if (!ent)
	{
		return POWER_ARMOR_NONE;
	}

	if (!ent->client)
	{
		return POWER_ARMOR_NONE;
	}

	if (!(ent->flags & FL_POWER_ARMOR))
	{
		return POWER_ARMOR_NONE;
	}

	if (ent->client->pers.inventory[power_shield_index] > 0)
	{
		return POWER_ARMOR_SHIELD;
	}

	if (ent->client->pers.inventory[power_screen_index] > 0)
	{
		return POWER_ARMOR_SCREEN;
	}

	return POWER_ARMOR_NONE;
}

void
Use_PowerArmor(edict_t *ent, gitem_t *item)
{
	int index;

	if (!ent || !item)
	{
		return;
	}

	if (ent->flags & FL_POWER_ARMOR)
	{
		ent->flags &= ~FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex(
						"misc/power2.wav"), 1, ATTN_NORM, 0);
	}
	else
	{
		index = ITEM_INDEX(FindItem("cells"));

		if (!ent->client->pers.inventory[index])
		{
			gi.cprintf(ent, PRINT_HIGH, "No cells for power armor.\n");
			return;
		}

		ent->flags |= FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex(
						"misc/power1.wav"), 1, ATTN_NORM, 0);
	}
}

qboolean
Pickup_PowerArmor(edict_t *ent, edict_t *other)
{
	int quantity;

	if (!ent || !other)
	{
		return false;
	}

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	if (deathmatch->value)
	{
		if (!(ent->spawnflags & DROPPED_ITEM))
		{
			SetRespawn(ent, ent->item->quantity);
		}
	}

	/* ace - auto-use if we didn't already have one, not only in DM */
	if (!quantity)
	{
		ent->item->use(other, ent->item);
	}

	return true;
}

void
Drop_PowerArmor(edict_t *ent, gitem_t *item)
{
	if (!ent || !item)
	{
		return;
	}

	if ((ent->flags & FL_POWER_ARMOR) &&
		(ent->client->pers.inventory[ITEM_INDEX(item)] == 1))
	{
		Use_PowerArmor(ent, item);
	}

	Drop_General(ent, item);
}

/* ====================================================================== */

void
Touch_Item(edict_t *ent, edict_t *other, cplane_t *plane /* unused */, csurface_t *surf /* unused */)
{
	qboolean taken;

	if (!ent || !other)
	{
		return;
	}

	if (!other->client)
	{
		return;
	}

	if (other->health < 1)
	{
		return; /* dead people can't pickup */
	}

	if (!ent->item->pickup)
	{
		return; /* not a grabbable item? */
	}

	taken = ent->item->pickup(ent, other);

	if (taken)
	{
		/* flash the screen */
		other->client->bonus_alpha = 0.25;

		/* show icon and name on status bar */
		other->client->ps.stats[STAT_PICKUP_ICON] =
			gi.imageindex( ent->item->icon);
		other->client->ps.stats[STAT_PICKUP_STRING] =
		   	CS_ITEMS + ITEM_INDEX( ent->item);
		other->client->pickup_msg_time = level.time + 3.0;

		/* change selected item */
		if (ent->item->use)
		{
			other->client->pers.selected_item =
				other->client->ps.stats[STAT_SELECTED_ITEM] =
			   	ITEM_INDEX( ent->item);
		}

		if (ent->item->pickup == Pickup_Health)
		{
			if (ent->count == 2)
			{
				gi.sound(other, CHAN_ITEM, gi.soundindex(
								"items/s_health.wav"), 1, ATTN_NORM, 0);
			}
			else if (ent->count == 10)
			{
				gi.sound(other, CHAN_ITEM, gi.soundindex(
								"items/n_health.wav"), 1, ATTN_NORM, 0);
			}
			else if (ent->count == 25)
			{
				gi.sound(other, CHAN_ITEM, gi.soundindex(
								"items/l_health.wav"), 1, ATTN_NORM, 0);
			}
			else /* (ent->count == 100) */
			{
				gi.sound(other, CHAN_ITEM, gi.soundindex(
								"items/m_health.wav"), 1, ATTN_NORM, 0);
			}
		}
		else if (ent->item->pickup_sound)
		{
			gi.sound(other, CHAN_ITEM, gi.soundindex(
							ent->item->pickup_sound), 1, ATTN_NORM, 0);
		}
	}

	if (!(ent->spawnflags & ITEM_TARGETS_USED))
	{
		G_UseTargets(ent, other);
		ent->spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
	{
		return;
	}

	if (!((coop->value) &&
		  (ent->item->flags & IT_STAY_COOP)) ||
		(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
	{
		if (ent->flags & FL_RESPAWN)
		{
			ent->flags &= ~FL_RESPAWN;
		}
		else
		{
			G_FreeEdict(ent);
		}
	}
}

/* ====================================================================== */

void
drop_temp_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!ent || !other)
	{
		return;
	}

	if (other == ent->owner)
	{
		return;
	}

	/* plane and surf are unused in Touch_Item
	   but since the function is part of the
	   game <-> client interface dropping
	   them is too much pain. */
	Touch_Item(ent, other, plane, surf);
}

void
drop_make_touchable(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->touch = Touch_Item;

	if (deathmatch->value)
	{
		ent->nextthink = level.time + 29;
		ent->think = G_FreeEdict;
	}
}

edict_t *
Drop_Item(edict_t *ent, gitem_t *item)
{
	edict_t *dropped;
	vec3_t forward, right;
	vec3_t offset;

	if (!ent || !item)
	{
		return NULL;
	}

	dropped = G_Spawn();

	dropped->classname = item->classname;
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	dropped->s.effects = item->world_model_flags;
	dropped->s.renderfx = RF_GLOW;

	if (randk() > 0.5)
	{
		dropped->s.angles[1] += randk()*45;
	}
	else
	{
		dropped->s.angles[1] -= randk()*45;
	}

	VectorSet (dropped->mins, -16, -16, -16);
	VectorSet (dropped->maxs, 16, 16, 16);
	gi.setmodel(dropped, dropped->item->world_model);
	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;
	dropped->touch = drop_temp_touch;
	dropped->owner = ent;

	if (ent->client)
	{
		trace_t trace;

		AngleVectors(ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 0, -16);
		G_ProjectSource(ent->s.origin, offset, forward, right,
				dropped->s.origin);
		trace = gi.trace(ent->s.origin, dropped->mins, dropped->maxs,
				dropped->s.origin, ent, CONTENTS_SOLID);
		VectorCopy(trace.endpos, dropped->s.origin);
	}
	else
	{
		AngleVectors(ent->s.angles, forward, right, NULL);
		VectorCopy(ent->s.origin, dropped->s.origin);
	}

	VectorScale(forward, 100, dropped->velocity);
	dropped->velocity[2] = 300;

	dropped->think = drop_make_touchable;
	dropped->nextthink = level.time + 1;

	gi.linkentity(dropped);

	return dropped;
}

void
Use_Item(edict_t *ent, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!ent)
	{
		return;
	}

	ent->svflags &= ~SVF_NOCLIENT;
	ent->use = NULL;

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->touch = Touch_Item;
	}

	gi.linkentity(ent);
}

/* ====================================================================== */

void
droptofloor(edict_t *ent)
{
	trace_t tr;
	vec3_t dest;
	float *v;

	if (!ent)
	{
		return;
	}

	v = tv(-15, -15, -15);
	VectorCopy(v, ent->mins);
	v = tv(15, 15, 15);
	VectorCopy(v, ent->maxs);

	if (ent->model)
	{
		gi.setmodel(ent, ent->model);
	}
	else
	{
		gi.setmodel(ent, ent->item->world_model);
	}

	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;

	v = tv(0, 0, -128);
	VectorAdd(ent->s.origin, v, dest);

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);

	if (tr.startsolid)
	{
		gi.dprintf("droptofloor: %s startsolid at %s\n", ent->classname,
				vtos(ent->s.origin));
		G_FreeEdict(ent);
		return;
	}

	VectorCopy(tr.endpos, ent->s.origin);

	if (ent->team)
	{
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = NULL;

		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;

		if (ent == ent->teammaster)
		{
			ent->nextthink = level.time + FRAMETIME;
			ent->think = DoRespawn;
		}
	}

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
		ent->s.effects &= ~EF_ROTATE;
		ent->s.renderfx &= ~RF_GLOW;
	}

	if (ent->spawnflags & ITEM_TRIGGER_SPAWN)
	{
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		ent->use = Use_Item;
	}

	gi.linkentity(ent);
}

/*
 * Precaches all data needed for a given item.
 * This will be called for each item spawned in a level,
 * and for each item in each client's inventory.
 */
void
PrecacheItem(gitem_t *it)
{
	char *s, *start;
	char data[MAX_QPATH];
	int len;
	gitem_t *ammo;

	if (!it)
	{
		return;
	}

	if (it->pickup_sound)
	{
		gi.soundindex(it->pickup_sound);
	}

	if (it->world_model)
	{
		gi.modelindex(it->world_model);
	}

	if (it->view_model)
	{
		gi.modelindex(it->view_model);
	}

	if (it->icon)
	{
		gi.imageindex(it->icon);
	}

	/* parse everything for its ammo */
	if (it->ammo && it->ammo[0])
	{
		ammo = FindItem(it->ammo);

		if (ammo != it)
		{
			PrecacheItem(ammo);
		}
	}

	/* parse the space seperated precache string for other items */
	s = it->precaches;

	if (!s || !s[0])
	{
		return;
	}

	while (*s)
	{
		start = s;

		while (*s && *s != ' ')
		{
			s++;
		}

		len = s - start;

		if ((len >= MAX_QPATH) || (len < 5))
		{
			gi.error("PrecacheItem: %s has bad precache string", it->classname);
		}

		memcpy(data, start, len);
		data[len] = 0;

		if (*s)
		{
			s++;
		}

		/* determine type based on extension */
		if (!strcmp(data + len - 3, "md2"))
		{
			gi.modelindex(data);
		}
		else if (!strcmp(data + len - 3, "sp2"))
		{
			gi.modelindex(data);
		}
		else if (!strcmp(data + len - 3, "wav"))
		{
			gi.soundindex(data);
		}

		if (!strcmp(data + len - 3, "pcx"))
		{
			gi.imageindex(data);
		}
	}
}

/*
 * ============
 * Sets the clipping size and
 * plants the object on the floor.
 *
 * Items can't be immediately dropped
 * to floor, because they might be on
 * an entity that hasn't spawned yet.
 * ============
 */
void
SpawnItem(edict_t *ent, gitem_t *item)
{
	if (!ent || !item)
	{
		return;
	}

	PrecacheItem(item);

	if (ent->spawnflags)
	{
		if (strcmp(ent->classname, "key_power_cube") != 0)
		{
			ent->spawnflags = 0;
			gi.dprintf("%s at %s has invalid spawnflags set\n",
					ent->classname, vtos(ent->s.origin));
		}
	}

	/* ace - allow dmflags rules to set other modes also */
//	if (1) //deathmatch->value)
	{
		if ((int)dmflags->value & DF_NO_ARMOR)
		{
			if ((item->pickup == Pickup_Armor) ||
				(item->pickup == Pickup_PowerArmor))
			{
				G_FreeEdict(ent);
				return;
			}
		}

		if ((int)dmflags->value & DF_NO_ITEMS)
		{
			if (item->pickup == Pickup_Powerup)
			{
				G_FreeEdict(ent);
				return;
			}
		}

		if ((int)dmflags->value & DF_NO_HEALTH)
		{
			if ((item->pickup == Pickup_Health) ||
				(item->pickup == Pickup_Adrenaline) ||
				(item->pickup == Pickup_AncientHead))
			{
				G_FreeEdict(ent);
				return;
			}
		}

		// ace - allow bfg in sp/coop
		if (((int)dmflags->value & DF_INFINITE_AMMO) && (deathmatch->value) )
		{
			if ((item->flags == IT_AMMO) ||
				(strcmp(ent->classname, "weapon_bfg") == 0))
			{
//				G_FreeEdict(ent);
				// ace - change them to HyperBlasters (for now);
				ent->classname = "weapon_hyperblaster";
				ent->item = FindItem ("HyperBlaster");
				SpawnItem(ent, ent->item); // run it back
//				goto retry
				return;
			}
		}
	}

	if (coop->value && (strcmp(ent->classname, "key_power_cube") == 0))
	{
		ent->spawnflags |= (1 << (8 + level.power_cubes));
		level.power_cubes++;
	}

	/* don't let them drop items that stay in a coop game */
	if ((coop->value) && (item->flags & IT_STAY_COOP))
	{
		item->drop = NULL;
	}

	ent->item = item;
	ent->nextthink = level.time + 2 * FRAMETIME; /* items start after other solids */
	ent->think = droptofloor;
	ent->s.effects = item->world_model_flags;
	ent->s.renderfx = RF_GLOW;

	if (ent->model)
	{
		gi.modelindex(ent->model);
	}
}

/* ====================================================================== */

gitem_t itemlist[] = {
	{
		NULL
	}, /* leave index 0 alone */

	/* QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_armor_body",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/body/tris.md2", EF_ROTATE,
		NULL,
		"i_bodyarmor",
		"Body Armor",
		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&bodyarmor_info,
		ARMOR_BODY,
		""
	},

	/* QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_armor_combat",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/combat/tris.md2", EF_ROTATE,
		NULL,
		"i_combatarmor",
		"Combat Armor",
		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&combatarmor_info,
		ARMOR_COMBAT,
		""
	},

	/* QUAKED item_armor_jacket (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_armor_jacket",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/jacket/tris.md2", EF_ROTATE,
		NULL,
		"i_jacketarmor",
		"Jacket Armor",
		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&jacketarmor_info,
		ARMOR_JACKET,
		""
	},

	/* QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_armor_shard",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar2_pkup.wav",
		"models/items/armor/shard/tris.md2", EF_ROTATE,
		NULL,
		"i_jacketarmor",
		"Armor Shard",
		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		ARMOR_SHARD,
		""
	},

	/* QUAKED item_power_screen (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_power_screen",
		Pickup_PowerArmor,
		Use_PowerArmor,
		Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"models/items/armor/screen/tris.md2", EF_ROTATE,
		NULL,
		"i_powerscreen",
		"Power Screen",
		0,
		60,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED item_power_shield (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_power_shield",
		Pickup_PowerArmor,
		Use_PowerArmor,
		Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"models/items/armor/shield/tris.md2", EF_ROTATE,
		NULL,
		"i_powershield",
		"Power Shield",
		0,
		60,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		0,
		"misc/power2.wav misc/power1.wav"
	},

/*QUAKED weapon_hand (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_push",
		NULL,
		Use_Weapon,
		NULL,
		Action_Push,
/* pickup sound */	NULL,
		NULL, 0,
		"models/weapons/v_hand/tris.md2",
/* icon */		NULL,
/* pickup */	"Push",
		0,
		0,
		NULL,
		IT_WEAPON|IT_STAY_COOP,
		0,
		NULL,
		0,
/* precache */ "weapons/push/contact.wav", // weapons/push/grunt.wav"
/* hide flags HIDE_FROM_INVENTORY | HIDE_DONT_KEEP | HIDE_FROM_SELECTION */
	},

	/* 
	 * weapon_grapple (.3 .3 1) (-16 -16 -16) (16 16 16)
	 * always owned, never in the world
	 */
	{
		"weapon_grapple",
		NULL,
		Use_Weapon,
		NULL,
		Weapon_Grapple,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/grapple/tris.md2",
		"w_grapple",
		"Grapple",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_GRAPPLE,
		NULL,
		0,

		"weapons/grapple/grfire.wav weapons/grapple/grpull.wav weapons/grapple/grhang.wav weapons/grapple/grreset.wav weapons/grapple/grhit.wav"
	},

	/* QUAKED weapon_chainfist (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN */
	{
		"weapon_chainfist",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_ChainFist,
		"misc/w_pkup.wav",
		"models/weapons/g_chainf/tris.md2", EF_ROTATE,
		"models/weapons/v_chainf/tris.md2",
		"w_chainfist",
		"Chainfist",
		0,
		0,
		NULL,
		IT_WEAPON /*  | IT_MELEE */,
		WEAP_CHAINFIST,
		NULL,
		1,
		"weapons/sawidle.wav weapons/sawhit.wav",
	},

	/* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
	   always owned, never in the world */
	{
		"weapon_blaster",
		NULL,
		Use_Weapon,
		NULL,
		Weapon_Blaster,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/v_blast/tris.md2",
		"w_blaster",
		"Blaster",
		0,
		0,
		NULL,
		IT_WEAPON | IT_STAY_COOP,
		WEAP_BLASTER,
		NULL,
		0,
		"weapons/blastf1a.wav misc/lasfly.wav"
	},

	/* QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"weapon_shotgun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Shotgun,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg/tris.md2", EF_ROTATE,
		"models/weapons/v_shotg/tris.md2",
		"w_shotgun",
		"Shotgun",
		0,
		1,
		"Shells",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_SHOTGUN,
		NULL,
		0,
		"weapons/shotgf1b.wav weapons/shotgr1b.wav"
	},

	/* QUAKED weapon_supershotgun (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"weapon_supershotgun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_SuperShotgun,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg2/tris.md2", EF_ROTATE,
		"models/weapons/v_shotg2/tris.md2",
		"w_sshotgun",
		"Super Shotgun",
		0,
		2,
		"Shells",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_SUPERSHOTGUN,
		NULL,
		0,
		"weapons/sshotf1b.wav"
	},

	/* QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"weapon_machinegun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Machinegun,
		"misc/w_pkup.wav",
		"models/weapons/g_machn/tris.md2", EF_ROTATE,
		"models/weapons/v_machn/tris.md2",
		"w_machinegun",
		"Machinegun",
		0,
		1,
		"Bullets",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_MACHINEGUN,
		NULL,
		0,
		"weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf4b.wav weapons/machgf5b.wav"
	},

	/* QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"weapon_chaingun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Chaingun,
		"misc/w_pkup.wav",
		"models/weapons/g_chain/tris.md2", EF_ROTATE,
		"models/weapons/v_chain/tris.md2",
		"w_chaingun",
		"Chaingun",
		0,
		1,
		"Bullets",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_CHAINGUN,
		NULL,
		0,
		"weapons/chngnu1a.wav weapons/chngnl1a.wav weapons/machgf3b.wav` weapons/chngnd1a.wav daedalus/daedatck1.wav"
	},

	/* QUAKED weapon_etf_rifle (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN */
	{
		"weapon_etf_rifle",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_ETF_Rifle,
		"misc/w_pkup.wav",
		"models/weapons/g_etf_rifle/tris.md2", EF_ROTATE,
		"models/weapons/v_etf_rifle/tris.md2",
		"w_etf_rifle",
		"ETF Rifle",
		0,
		1,
		"Flechettes",
		IT_WEAPON,
		WEAP_ETFRIFLE,
		NULL,
		0,
		"weapons/nail1.wav models/proj/flechette/tris.md2",
	},

	/* QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"ammo_grenades",
		Pickup_Ammo,
		Use_Weapon,
		Drop_Ammo,
		Weapon_Grenade,
		"misc/am_pkup.wav",
		"models/items/ammo/grenades/medium/tris.md2", 0,
		"models/weapons/v_handgr/tris.md2",
		"a_grenades",
		"Grenades",
		3,
		5,
		"grenades",
		IT_AMMO | IT_WEAPON,
		WEAP_GRENADES,
		NULL,
		AMMO_GRENADES,
		"weapons/hgrent1a.wav weapons/hgrena1b.wav weapons/hgrenc1b.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav "
	},
	// ace - zaero
/*QUAKED ammo_ired (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_ired",
		Pickup_Ammo,
		Use_Weapon,
		Drop_Ammo,
		Weapon_LaserTripBomb,
		"misc/am_pkup.wav",
		"models/items/ammo/ireds/tris.md2", 0,
		"models/weapons/v_ired/tris.md2",
/* icon */		"w_ired",
/* pickup */	"IRED",
/* width */		3,
		3,
		"ired",
		IT_AMMO|IT_WEAPON,
		WEAP_GRENADES, // ace - atleast show something in hand
		NULL,
		AMMO_LASERTRIPBOMB,
/* precache */ "models/weapons/v_ired/hand.md2 models/objects/ired/tris.md2 modes/objects models/objects/shrapnel/tris.md2 "
	"weapons/ired/las_set.wav weapons/ired/las_arm.wav "
	/*"weapons/ired/las_tink.wav "weapons/ired/las_trig.wav "*/
	/*"weapons/ired/las_glow.wav"*/,
	},

	// ace - xatrix
	/*
	 * QUAKED ammo_trap (.3 .3 1) (-16 -16 -16) (16 16 16)
	 */
	{
		"ammo_trap",
		Pickup_Ammo,
		Use_Weapon,
		Drop_Ammo,
		Weapon_Trap,
		"misc/am_pkup.wav",
		"models/weapons/g_trap/tris.md2", EF_ROTATE,
		"models/weapons/v_trap/tris.md2",
		"a_trap",
		"Trap",
		3,
		1,
		"trap",
		IT_AMMO | IT_WEAPON,
		WEAP_GRENADES, // 0, ace - atleast show something in hand
		NULL,
		AMMO_TRAP,
		"weapons/trap/trapcock.wav weapons/trap/traploop.wav weapons/trap/trapsuck.wav weapons/trap/trapdown.wav"
	},

	/* QUAKED ammo_prox (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN */
	{
		"ammo_prox",
		Pickup_Ammo,
		Use_Weapon, // NULL,
		Drop_Ammo,
		Weapon_Prox, // NULL,
		"misc/am_pkup.wav",
		"models/ammo/am_prox/tris.md2", 0,
		"models/weapons/v_handgr/tris.md2",
		"a_prox",
		"Prox",
		3,
		5,
		"Prox",
		IT_AMMO | IT_WEAPON,
		WEAP_GRENADES, // 0, ace - atleast show something in hand
		NULL,
		AMMO_PROX,
		"models/weapons/v_handgr/tris.md2 models/weapons/g_prox/tris.md2 weapons/proxwarn.wav"
	},

	/* QUAKED ammo_tesla (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN */
	{
		"ammo_tesla",
		Pickup_Ammo,
		Use_Weapon,
		Drop_Ammo,
		Weapon_Tesla,
		"misc/am_pkup.wav",
		"models/ammo/am_tesl/tris.md2", 0,
		"models/weapons/v_tesla/tris.md2",
		"a_tesla",
		"Tesla",
		3,
		5,
		"Tesla",
		IT_AMMO | IT_WEAPON,
		WEAP_GRENADES, // 0, ace - atleast show something in hand
		NULL,
		AMMO_TESLA,
		"models/weapons/v_tesla2/tris.md2 weapons/teslaopen.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav models/weapons/g_tesla/tris.md2"
	},

	/* QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"weapon_grenadelauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_GrenadeLauncher,
		"misc/w_pkup.wav",
		"models/weapons/g_launch/tris.md2", EF_ROTATE,
		"models/weapons/v_launch/tris.md2",
		"w_glauncher",
		"Grenade Launcher",
		0,
		1,
		"Grenades",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_GRENADELAUNCHER,
		NULL,
		0,
		"models/objects/grenade/tris.md2 weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav"
	},

/* QUAKED weapon_proxlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN */
	{
		"weapon_proxlauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_ProxLauncher,
		"misc/w_pkup.wav",
		"models/weapons/g_plaunch/tris.md2", EF_ROTATE,
		"models/weapons/v_plaunch/tris.md2",
		"w_proxlaunch",
		"Prox Launcher",
		0,
		1,
		"Prox",
		IT_WEAPON,
		WEAP_PROXLAUNCH,
		NULL,
		AMMO_PROX,
		"weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav weapons/proxwarn.wav weapons/proxopen.wav",
	},

	/* QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"weapon_rocketlauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_RocketLauncher,
		"misc/w_pkup.wav",
		"models/weapons/g_rocket/tris.md2", EF_ROTATE,
		"models/weapons/v_rocket/tris.md2",
		"w_rlauncher",
		"Rocket Launcher",
		0,
		1,
		"Rockets",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_ROCKETLAUNCHER,
		NULL,
		0,
		"models/objects/rocket/tris.md2 weapons/rockfly.wav weapons/rocklf1a.wav weapons/rocklr1b.wav models/objects/debris2/tris.md2"
	},

	// ace - xatrix
	/*
	 * QUAKED weapon_phalanx (.3 .3 1) (-16 -16 -16) (16 16 16)
	 */

	{
		"weapon_phalanx",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Phalanx,
		"misc/w_pkup.wav",
		"models/weapons/g_shotx/tris.md2", EF_ROTATE,
		"models/weapons/v_shotx/tris.md2",
		"w_phallanx",
		"Phalanx",
		0,
		1,
		"Mag Slug",
		IT_WEAPON,
		WEAP_PHALANX,
		NULL,
		0,
		"weapons/plasma/plasshot.wav"
	},

	/* QUAKED weapon_hyperblaster (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"weapon_hyperblaster",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_HyperBlaster,
		"misc/w_pkup.wav",
		"models/weapons/g_hyperb/tris.md2", EF_ROTATE,
		"models/weapons/v_hyperb/tris.md2",
		"w_hyperblaster",
		"HyperBlaster",
		0,
		1,
		"Cells",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_HYPERBLASTER,
		NULL,
		0,
		"weapons/hyprbu1a.wav weapons/hyprbl1a.wav weapons/hyprbf1a.wav weapons/hyprbd1a.wav misc/lasfly.wav"
	},

	/* QUAKED weapon_plasmabeam (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN */
	{
		"weapon_plasmabeam",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Heatbeam,
		"misc/w_pkup.wav",
		"models/weapons/g_beamer/tris.md2", EF_ROTATE,
		"models/weapons/v_beamer/tris.md2",
		"w_heatbeam",
		"Plasma Beam",
		0,
		2,
		"Cells",
		IT_WEAPON,
		WEAP_PLASMA,
		NULL,
		0,
		"models/weapons/v_beamer2/tris.md2 weapons/bfg__l1a.wav",
	},

	// ace - xatrix
	/*
	 * QUAKED weapon_boomer (.3 .3 1) (-16 -16 -16) (16 16 16)
	 */
	{
		"weapon_boomer",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Ionripper,
		"misc/w_pkup.wav",
		"models/weapons/g_boom/tris.md2", EF_ROTATE,
		"models/weapons/v_boomer/tris.md2",
		"w_ripper",
		"Ionripper",
		0,
		2,
		"Cells",
		IT_WEAPON,
		WEAP_BOOMER,
		NULL,
		0,
		"weapons/rg_hum.wav weapons/rippfire.wav"
	},

	/* QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"weapon_railgun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Railgun,
		"misc/w_pkup.wav",
		"models/weapons/g_rail/tris.md2", EF_ROTATE,
		"models/weapons/v_rail/tris.md2",
		"w_railgun",
		"Railgun",
		0,
		1,
		"Slugs",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_RAILGUN,
		NULL,
		0,
		"weapons/rg_hum.wav"
	},

	// ace - zaero
/*QUAKED weapon_sniperrifle (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_sniperrifle", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_SniperRifle,
		"misc/w_pkup.wav",
		"models/weapons/g_sniper/tris.md2", EF_ROTATE,
		"models/weapons/v_sniper/tris.md2",
/* icon */		"w_sniper",
/* pickup */	"Sniper Rifle",
		0,
		2,
		"Slugs",
		IT_WEAPON|IT_STAY_COOP,
		0,
		NULL,
		0,
/* precache */ "models/weapons/v_sniper/scope/tris.md2 models/weapons/v_sniper/dmscope/tris.md2 weapons/sniper/beep.wav weapons/sniper/fire.wav "
		/*"weapons/sniper/snip_act.wav weapons/sniper/snip_bye.wav"*/
	},

	/* QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"weapon_bfg",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_BFG,
		"misc/w_pkup.wav",
		"models/weapons/g_bfg/tris.md2", EF_ROTATE,
		"models/weapons/v_bfg/tris.md2",
		"w_bfg",
		"BFG10K",
		0,
		50,
		"Cells",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_BFG,
		NULL,
		0,
		"sprites/s_bfg1.sp2 sprites/s_bfg2.sp2 sprites/s_bfg3.sp2 weapons/bfg__f1y.wav weapons/bfg__l1a.wav weapons/bfg__x1b.wav weapons/bfg_hum.wav"
	},

/* QUAKED weapon_disintegrator (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN */
	{
		"weapon_disintegrator",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Disintegrator,
		"misc/w_pkup.wav",
		"models/weapons/g_dist/tris.md2", EF_ROTATE,
		"models/weapons/v_dist/tris.md2",
		"w_disintegrator",
		"Disruptor",
		0,
		10,
		"Cells",
		IT_WEAPON | IT_STAY_COOP /*IT_NOT_GIVEABLE*/,
		WEAP_DISRUPTOR,
		NULL,
		0 /*1*/,
		"models/items/spawngro/tris.md2 models/proj/disintegrator/tris.md2 weapons/disrupt.wav weapons/disint2.wav weapons/disrupthit.wav",
	},

	// ace - zaero
/*QUAKED weapon_soniccannon  (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_soniccannon",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_SonicCannon,
		"misc/w_pkup.wav",
		"models/weapons/g_sonic/tris.md2", EF_ROTATE,
		"models/weapons/v_sonic/tris.md2",
/* icon */		"w_sonic",
/* pickup */	"Sonic Cannon",
		0,
		1,
		"Cells",
		IT_WEAPON|IT_STAY_COOP,
		0,
		NULL,
		0,
/* precache */ "weapons/sonic/sc_warm.wav weapons/sonic/sc_cool.wav weapons/sonic/sc_fire.wav" // weapons/sonic/sc_act.wav weapons/sonic/sc_dact.wav  
	},

	/* QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"ammo_shells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/shells/medium/tris.md2", 0,
		NULL,
		"a_shells",
		"Shells",
		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SHELLS,
		""
	},

	/* QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"ammo_bullets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/bullets/medium/tris.md2", 0,
		NULL,
		"a_bullets",
		"Bullets",
		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_BULLETS,
		""
	},

	/* QUAKED ammo_flechettes (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN */
	{
		"ammo_flechettes",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/ammo/am_flechette/tris.md2", 0,
		NULL,
		"a_flechettes",
		"Flechettes",
		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_FLECHETTES
	},

	/* QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"ammo_cells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/cells/medium/tris.md2", 0,
		NULL,
		"a_cells",
		"Cells",
		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_CELLS,
		""
	},

	/* QUAKED ammo_prox (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN */
	{
		"ammo_prox",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/ammo/am_prox/tris.md2", 0,
		NULL,
		"a_prox",
		"Prox",
		3,
		5,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_PROX,
		"models/weapons/g_prox/tris.md2 weapons/proxwarn.wav"
	},

	/* QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"ammo_rockets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/rockets/medium/tris.md2", 0,
		NULL,
		"a_rockets",
		"Rockets",
		3,
		5,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_ROCKETS,
		""
	},

	/* QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"ammo_slugs",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/slugs/medium/tris.md2", 0,
		NULL,
		"a_slugs",
		"Slugs",
		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SLUGS,
		""
	},

	// ace - xatrix
	/*
	 * QUAKED ammo_magslug (.3 .3 1) (-16 -16 -16) (16 16 16)
	 */
	{
		"ammo_magslug",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/objects/ammo/tris.md2", 0,
		NULL,
		"a_mslugs",
		"Mag Slug",
		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_MAGSLUG,
		""
	},

// ace - Evolve items
/*QUAKED ammo_plasmashield (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_plasmashield",
		Pickup_Ammo,
		Use_PlasmaShield,
		Drop_Ammo,
		NULL,
		"misc/ar3_pkup.wav",
		"models/items/plasma/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_plasma",
/* pickup */	"Plasma Shield",
/* width */		1,
		5,
		NULL, // ,""
		IT_AMMO,
		0,
		NULL,
		AMMO_PLASMASHIELD,
/* precache */ "items/plasmashield/psactive.wav sprites/plasmashield.sp2"
///* precache */ "items/plasmashield/psfire.wav items/plasmashield/psactive.wav items/plasmashield/psdie.wav sprites/plasmashield.sp2"
	},

	/* QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_quad",
		Pickup_Powerup,
		Use_Quad,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/quaddama/tris.md2", EF_ROTATE,
		NULL,
		"p_quad",
		"Quad Damage",
		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		"items/damage.wav items/damage2.wav items/damage3.wav"
	},

	/* QUAKED item_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_invulnerability",
		Pickup_Powerup,
		Use_Invulnerability,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/invulner/tris.md2", EF_ROTATE,
		NULL,
		"p_invulnerability",
		"Invulnerability",
		2,
		300,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		"items/protect.wav items/protect2.wav items/protect4.wav"
	},

	/* QUAKED item_silencer (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_silencer",
		Pickup_Powerup,
		Use_Silencer,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/silencer/tris.md2", EF_ROTATE,
		NULL,
		"p_silencer",
		"Silencer",
		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED item_breather (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_breather",
		Pickup_Powerup,
		Use_Breather,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/breather/tris.md2", EF_ROTATE,
		NULL,
		"p_rebreather",
		"Rebreather",
		2,
		60,
		NULL,
		IT_STAY_COOP | IT_POWERUP,
		0,
		NULL,
		0,
		"items/airout.wav"
	},

	/* QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_enviro",
		Pickup_Powerup,
		Use_Envirosuit,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/enviro/tris.md2", EF_ROTATE,
		NULL,
		"p_envirosuit",
		"Environment Suit",
		2,
		60,
		NULL,
		IT_STAY_COOP | IT_POWERUP,
		0,
		NULL,
		0,
		"items/airout.wav"
	},

	/* QUAKED item_ancient_head (.3 .3 1) (-16 -16 -16) (16 16 16)
	   Special item that gives +2 to maximum health */
	{
		"item_ancient_head",
		Pickup_AncientHead,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/c_head/tris.md2", EF_ROTATE,
		NULL,
		"i_fixme",
		"Ancient Head",
		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED item_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16)
	   gives +1 to maximum health */
	{
		"item_adrenaline",
		Pickup_Adrenaline,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/adrenal/tris.md2", EF_ROTATE,
		NULL,
		"p_adrenaline",
		"Adrenaline",
		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED item_bandolier (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_bandolier",
		Pickup_Bandolier,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/band/tris.md2", EF_ROTATE,
		NULL,
		"p_bandolier",
		"Bandolier",
		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED item_pack (.3 .3 1) (-16 -16 -16) (16 16 16) */
	{
		"item_pack",
		Pickup_Pack,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/pack/tris.md2", EF_ROTATE,
		NULL,
		"i_pack",
		"Ammo Pack",
		2,
		180,
		NULL,
		0,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED key_data_cd (0 .5 .8) (-16 -16 -16) (16 16 16)
	   key for computer centers */
	{
		"key_data_cd",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/data_cd/tris.md2", EF_ROTATE,
		NULL,
		"k_datacd",
		"Data CD",
		2,
		0,
		NULL,
		IT_STAY_COOP | IT_KEY,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED key_power_cube (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN NO_TOUCH
	   warehouse circuits */
	{
		"key_power_cube",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/power/tris.md2", EF_ROTATE,
		NULL,
		"k_powercube",
		"Power Cube",
		2,
		0,
		NULL,
		IT_STAY_COOP | IT_KEY,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED key_pyramid (0 .5 .8) (-16 -16 -16) (16 16 16)
	   key for the entrance of jail3 */
	{
		"key_pyramid",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/pyramid/tris.md2", EF_ROTATE,
		NULL,
		"k_pyramid",
		"Pyramid Key",
		2,
		0,
		NULL,
		IT_STAY_COOP | IT_KEY,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED key_data_spinner (0 .5 .8) (-16 -16 -16) (16 16 16)
	   key for the city computer */
	{
		"key_data_spinner",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/spinner/tris.md2", EF_ROTATE,
		NULL,
		"k_dataspin",
		"Data Spinner",
		2,
		0,
		NULL,
		IT_STAY_COOP | IT_KEY,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED key_pass (0 .5 .8) (-16 -16 -16) (16 16 16)
	   security pass for the security level */
	{
		"key_pass",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/pass/tris.md2", EF_ROTATE,
		NULL,
		"k_security",
		"Security Pass",
		2,
		0,
		NULL,
		IT_STAY_COOP | IT_KEY,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED key_blue_key (0 .5 .8) (-16 -16 -16) (16 16 16)
	   normal door key - blue */
	{
		"key_blue_key",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/key/tris.md2", EF_ROTATE,
		NULL,
		"k_bluekey",
		"Blue Key",
		2,
		0,
		NULL,
		IT_STAY_COOP | IT_KEY,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED key_red_key (0 .5 .8) (-16 -16 -16) (16 16 16)
	   normal door key - red */
	{
		"key_red_key",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/red_key/tris.md2", EF_ROTATE,
		NULL,
		"k_redkey",
		"Red Key",
		2,
		0,
		NULL,
		IT_STAY_COOP | IT_KEY,
		0,
		NULL,
		0,
		""
	},

	/*
	 * QUAKED key_green_key (0 .5 .8) (-16 -16 -16) (16 16 16)
	 * normal door key - green
	 */
	{
		"key_green_key",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/green_key/tris.md2", EF_ROTATE,
		NULL,
		"k_green",
		"Green Key",
		2,
		0,
		NULL,
		IT_STAY_COOP | IT_KEY,
		0,
		NULL,
		0,
		""
	},
	/* QUAKED key_commander_head (0 .5 .8) (-16 -16 -16) (16 16 16)
	   tank commander's head */
	{
		"key_commander_head",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/monsters/commandr/head/tris.md2", EF_GIB,
		NULL,
		"k_comhead",
		"Commander's Head",
		2,
		0,
		NULL,
		IT_STAY_COOP | IT_KEY,
		0,
		NULL,
		0,
		""
	},

	/* QUAKED key_airstrike_target (0 .5 .8) (-16 -16 -16) (16 16 16) */
	{
		"key_airstrike_target",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/target/tris.md2", EF_ROTATE,
		NULL,
		"i_airstrike",
		"Airstrike Marker",
		2,
		0,
		NULL,
		IT_STAY_COOP | IT_KEY,
		0,
		NULL,
		0,
		""
	},

	/* Resistance Tech */
	{
		"item_tech1",
		Pickup_Tech,
		NULL,
		Drop_Tech,
		NULL,
		"items/pkup.wav",
		"models/tech/resistance/tris.md2", EF_ROTATE,
		NULL,
		"tech1",
		"Disruptor Shield",
		2,
		0,
		NULL,
		IT_TECH,
		0,
		NULL,
		0,
		"tech/tech1.wav"
	},

	/* Strength Tech */
	{
		"item_tech2",
		Pickup_Tech,
		NULL,
		Drop_Tech,
		NULL,
		"items/pkup.wav",
		"models/tech/strength/tris.md2", EF_ROTATE,
		NULL,
		"tech2",
		"Power Amplifier",
		2,
		0,
		NULL,
		IT_TECH,
		0,
		NULL,
		0,
		"tech/tech2.wav tech/tech2x.wav"
	},

	/* Haste Tech */
	{
		"item_tech3",
		Pickup_Tech,
		NULL,
		Drop_Tech,
		NULL,
		"items/pkup.wav",
		"models/tech/haste/tris.md2", EF_ROTATE,
		NULL,
		"tech3",
		"Time Accel",
		2,
		0,
		NULL,
		IT_TECH,
		0,
		NULL,
		0,
		"tech/tech3.wav"
	},

	/* Regeneration Tech */
	{
		"item_tech4",
		Pickup_Tech,
		NULL,
		Drop_Tech,
		NULL,
		"items/pkup.wav",
		"models/tech/regeneration/tris.md2", EF_ROTATE,
		NULL,
		"tech4",
		"AutoDoc",
		2,
		0,
		NULL,
		IT_TECH,
		0,
		NULL,
		0,
		"tech/tech4.wav"
	},

	{
		NULL,
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		NULL, 0,
		NULL,
		"i_health",
		"Health",
		3,
		0,
		NULL,
		0,
		0,
		NULL,
		0,
		"items/s_health.wav items/n_health.wav items/l_health.wav items/m_health.wav"
	},

	/* end of list marker */
	{NULL}
};

/*
 * QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)
 */
void
SP_item_health(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH))
	{
		G_FreeEdict(self);
		return;
	}

	self->model = "models/items/healing/medium/tris.md2";
	self->count = 10;
	SpawnItem(self, FindItem("Health"));
	gi.soundindex("items/n_health.wav");
}

/*
 * QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16)
 */
void
SP_item_health_small(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH))
	{
		G_FreeEdict(self);
		return;
	}

	self->model = "models/items/healing/stimpack/tris.md2";
	self->count = 2;
	SpawnItem(self, FindItem("Health"));
	self->style = HEALTH_IGNORE_MAX;
	gi.soundindex("items/s_health.wav");
}

/*
 * QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)
 */
void
SP_item_health_large(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH))
	{
		G_FreeEdict(self);
		return;
	}

	self->model = "models/items/healing/large/tris.md2";
	self->count = 25;
	SpawnItem(self, FindItem("Health"));
	gi.soundindex("items/l_health.wav");
}

/*
 * QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16)
 */
void
SP_item_health_mega(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH))
	{
		G_FreeEdict(self);
		return;
	}

	self->model = "models/items/mega_h/tris.md2";
	self->count = 100;
	SpawnItem(self, FindItem("Health"));
	gi.soundindex("items/m_health.wav");
	self->style = HEALTH_IGNORE_MAX | HEALTH_TIMED;
}

// ace - for the trap
void
SP_item_foodcube(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH))
	{
		G_FreeEdict(self);
		return;
	}

	self->model = "models/objects/trapfx/tris.md2";
	SpawnItem(self, FindItem("Health"));
	self->spawnflags |= DROPPED_ITEM;
	self->style = HEALTH_IGNORE_MAX;
	gi.soundindex("items/s_health.wav");
	self->classname = "foodcube";
}

void
InitItems(void)
{
	game.num_items = sizeof(itemlist) / sizeof(itemlist[0]) - 1;
}

/*
 * Called by worldspawn
 */
void
SetItemNames(void)
{
	int i;
	gitem_t *it;

	for (i = 0; i < game.num_items; i++)
	{
		it = &itemlist[i];
		gi.configstring(CS_ITEMS + i, it->pickup_name);
	}

	jacket_armor_index = ITEM_INDEX(FindItem("Jacket Armor"));
	combat_armor_index = ITEM_INDEX(FindItem("Combat Armor"));
	body_armor_index = ITEM_INDEX(FindItem("Body Armor"));
	power_screen_index = ITEM_INDEX(FindItem("Power Screen"));
	power_shield_index = ITEM_INDEX(FindItem("Power Shield"));
}
