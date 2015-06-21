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
 * Combat code like damage, death and so on.
 *
 * =======================================================================
 */

#include "header/local.h"

/*
 * Returns true if the inflictor can
 * directly damage the target.  Used for
 * explosions and melee attacks.
 */
qboolean
CanDamage(edict_t *targ, edict_t *inflictor)
{
	vec3_t dest;
	trace_t trace;

	if (!targ || !inflictor)
	{
		return false;
	}

	/* bmodels need special checking because their origin is 0,0,0 */
	if (targ->movetype == MOVETYPE_PUSH)
	{
		VectorAdd(targ->absmin, targ->absmax, dest);
		VectorScale(dest, 0.5, dest);
		trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
				dest, inflictor, MASK_SOLID);

		if (trace.fraction == 1.0)
		{
			return true;
		}

		if (trace.ent == targ)
		{
			return true;
		}

		return false;
	}

	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
			targ->s.origin, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0)
	{
		return true;
	}

	VectorCopy(targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
			dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0)
	{
		return true;
	}

	VectorCopy(targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
			dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0)
	{
		return true;
	}

	VectorCopy(targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
			dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0)
	{
		return true;
	}

	VectorCopy(targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin,
			dest, inflictor, MASK_SOLID);

	if (trace.fraction == 1.0)
	{
		return true;
	}

	return false;
}

void
Killed(edict_t *targ, edict_t *inflictor, edict_t *attacker,
		int damage, vec3_t point)
{
	if (!targ || !inflictor || !attacker)
	{
		return;
	}
	// ace - no limit on damage
/*	if (targ->health < -999)
	{
		targ->health = -999;
	} */

	targ->enemy = attacker;

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
// ace - add spawned slot support from rogue
		/* free up slot for spawned monster if it's spawned */
		if (targ->monsterinfo.aiflags & AI_DO_NOT_COUNT)
		{
			if (targ->monsterinfo.commander &&
				targ->monsterinfo.commander->inuse &&
				(!strcmp(targ->monsterinfo.commander->classname, "monster_carrier") ||
				!strcmp(targ->monsterinfo.commander->classname, "monster_medic_commander")))
			{
				targ->monsterinfo.commander->monsterinfo.monster_slots++;
			}
			/* need to check this because we can
			   have variable numbers of coop players */
			if (targ->monsterinfo.commander &&
				targ->monsterinfo.commander->inuse &&
				!strncmp(targ->monsterinfo.commander->classname, "monster_widow", 13))
			{
				if (targ->monsterinfo.commander->monsterinfo.monster_used > 0)
				{
					targ->monsterinfo.commander->monsterinfo.monster_used--;
				}
			}
		}
// ace - end
// ace - randomizer stuff - start
//		if (targ->monstertype) // ace - this is for keeping count of enemies for the randomizer
		{
			int check1, check2;
			
			level.rmonsterinfo.num_mspawns[targ->monstertype]--;
			level.rmonsterinfo.num_monsters--;

			// how many monsters do we need to kill in otder to advance the "wave"
//			check = level.killed_monsters + 1 + (randk() & level.skill_level);
//			if ((check > level.rmonsterinfo.max_wave_monsters) &&
			check1 = level.killed_monsters + 1 + (randk()& level.skill_level);
			check2 = level.rmonsterinfo.max_wave_monsters*level.num_wave;
			if ((check1 & check2) &&
				level.rmonsterinfo.wave_set == true)
			{
				float delay;
				
				level.rmonsterinfo.wave_set = false; // resume spawning
				level.num_wave++;
				if (level.num_wave > 33) // cap waves
					level.num_wave = 33;

				// ace - set delay to a random of 3+ seconds, progressing w/ wave# and skill level
				delay = 30 + (float)level.num_wave - ((float)level.skill_level * ACE_RANDOM);
				if (delay > 60) // define this somewhere in acemod.h
				{
					delay = 60; // cap delay
				}

				level.rmonsterinfo.t1_spawned = level.time + delay;

				gi.dprintf("ace_MonsterRandomizerGo: WAVE %i TRIGGERED!  More strogg incomming...\n", level.num_wave);
			}

//			gi.dprintf("(ace)Killed: %s monstertype %i killed @ %i hp.\n", targ->classname, targ->monstertype, targ->health);
		}
// ace - randomizer stuff - end

		if (!(targ->monsterinfo.aiflags & AI_GOOD_GUY))
		{
			if (!(targ->monsterinfo.aiflags & AI_DO_NOT_COUNT))
			{
				level.killed_monsters++;
			}

			if (/*coop->value && */attacker->client)
			{ // ace - up the score anyways for the player that scored
				attacker->client->resp.score++;
			}

			// ace - reduce spawn delay for randomizer when monster dies
			level.rmonsterinfo.t1_spawned -= (damage * (FRAMETIME*FRAMETIME));

			/* medics won't heal monsters that they kill themselves */
			// ace - actually they do, and make them work for them.
			if (strcmp(attacker->classname, "monster_medic") == 0)
			{
				targ->enemy = NULL;
				//	targ->owner = attacker;
			}
		}
	}

	if ((targ->movetype == MOVETYPE_PUSH) ||
		(targ->movetype == MOVETYPE_STOP) ||
		(targ->movetype == MOVETYPE_NONE))
	{
		/* doors, triggers, etc */
		targ->die(targ, inflictor, attacker, damage, point);
		return;
	}

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
		targ->touch = NULL;
		monster_death_use(targ);
	}

	targ->die(targ, inflictor, attacker, damage, point);
}

void
SpawnDamage(int type, vec3_t origin, vec3_t normal)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(type);
	gi.WritePosition(origin);
	gi.WriteDir(normal);
	gi.multicast(origin, MULTICAST_PVS);
}

/*
 * targ			entity that is being damaged
 * inflictor	entity that is causing the damage
 * attacker		entity that caused the inflictor to damage targ
 *      example: targ=monster, inflictor=rocket, attacker=player
 *
 * dir			direction of the attack
 * point		point at which the damage is being inflicted
 * normal		normal vector from that point
 * damage		amount of damage being inflicted
 * knockback	force to be applied against targ as a result of the damage
 *
 * dflags -> these flags are used to control how T_Damage works
 *      DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
 *      DAMAGE_NO_ARMOR			armor does not protect from this damage
 *      DAMAGE_ENERGY			damage is from an energy based weapon
 *      DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
 *      DAMAGE_BULLET			damage is from a bullet (used for ricochets)
 *      DAMAGE_NO_PROTECTION	kills godmode, armor, everything
 */

int
CheckPowerArmor(edict_t *ent, vec3_t point, vec3_t normal, int damage,
		int dflags)
{
	gclient_t *client;
	int save;
	int power_armor_type;
	int index;
	int damagePerCell;
	int pa_te_type;
	int power = 0;
	int power_used;

	if (!ent)
	{
		return 0;
	}

	if (!damage)
	{
		return 0;
	}

	index = 0;

	client = ent->client;

	if (dflags & DAMAGE_NO_ARMOR)
	{
		return 0;
	}

	if (client)
	{
		power_armor_type = PowerArmorType(ent);

		if (power_armor_type != POWER_ARMOR_NONE)
		{
			index = ITEM_INDEX(FindItem("Cells"));
			power = client->pers.inventory[index];
		}
	}
	else if (ent->svflags & SVF_MONSTER)
	{
		power_armor_type = ent->monsterinfo.power_armor_type;
		power = ent->monsterinfo.power_armor_power;
		index = 0;
	}
	else
	{
		return 0;
	}

	if (power_armor_type == POWER_ARMOR_NONE)
	{
		return 0;
	}

	if (!power)
	{
		return 0;
	}

	if (power_armor_type == POWER_ARMOR_SCREEN)
	{
		vec3_t vec;
		float dot;
		vec3_t forward;

		/* only works if damage point is in front */
		AngleVectors(ent->s.angles, forward, NULL, NULL);
		VectorSubtract(point, ent->s.origin, vec);
		VectorNormalize(vec);
		dot = DotProduct(vec, forward);

		if (dot <= 0.3)
		{
			return 0;
		}

		damagePerCell = 1;
		pa_te_type = TE_SCREEN_SPARKS;
		damage = damage / 3;
	}
	else
	{
		damagePerCell = 2;
		pa_te_type = TE_SHIELD_SPARKS;
		damage = (2 * damage) / 3;
	}

	save = power * damagePerCell;

	/* etf rifle */
	if (dflags & DAMAGE_NO_REG_ARMOR)
	{
		save = (power * damagePerCell) / 2;
	}
	else
	{
		save = power * damagePerCell;
	}

	if (!save)
	{
		return 0;
	}

	if (save > damage)
	{
		save = damage;
	}

	SpawnDamage(pa_te_type, point, normal);
	ent->powerarmor_time = level.time + 0.2;

	if (dflags & DAMAGE_NO_REG_ARMOR)
	{
		power_used = (save / damagePerCell) * 2;
	}
	else
	{
		power_used = save / damagePerCell;
	}

	if (client)
	{
		client->pers.inventory[index] -= power_used;
	}
	else
	{
		ent->monsterinfo.power_armor_power -= power_used;
	}

	return save;
}

int
CheckArmor(edict_t *ent, vec3_t point, vec3_t normal, int damage,
		int te_sparks, int dflags)
{
	gclient_t *client;
	int save;
	int index;
	gitem_t *armor;

	if (!ent)
	{
		return 0;
	}

	if (!damage)
	{
		return 0;
	}

	client = ent->client;

	if (!client)
	{
		return 0;
	}

	if (dflags & (DAMAGE_NO_ARMOR | DAMAGE_NO_REG_ARMOR))
	{
		return 0;
	}

	index = ArmorIndex(ent);

	if (!index)
	{
		return 0;
	}

	armor = GetItemByIndex(index);

	if (dflags & DAMAGE_ENERGY)
	{
		save = ceil(((gitem_armor_t *)armor->info)->energy_protection * damage);
	}
	else
	{
		save = ceil(((gitem_armor_t *)armor->info)->normal_protection * damage);
	}

	if (save >= client->pers.inventory[index])
	{
		save = client->pers.inventory[index];
	}

	if (!save)
	{
		return 0;
	}

	client->pers.inventory[index] -= save;
	SpawnDamage(te_sparks, point, normal);

	return save;
}
// ace - modified monster reaction to damage
void
M_ReactToDamage(edict_t *targ, edict_t *attacker, edict_t *inflictor)
{
	qboolean new_tesla;

	if (!targ || !attacker)
	{
		return;
	}

	if (targ->health <= 0)
	{
		return;
	}

	if (!(attacker->client) && !(attacker->svflags & SVF_MONSTER))
	{
		return;
	}

	/* logic for tesla - if you are hit by a tesla,
	   and can't see who you should be mad at (attacker)
	   attack the tesla also, target the tesla if it's
	   a "new" tesla */
	if ((inflictor) && (!strcmp(inflictor->classname, "tesla")))
	{
		new_tesla = MarkTeslaArea(targ, inflictor);

		if (new_tesla)
		{
			TargetTesla(targ, inflictor);
		}

		return;
	}

	if (attacker == targ)
	{
		return;
	}

	if (attacker == targ->enemy)
	{
		if (strcmp(attacker->classname,targ->classname) == 0)
			return; // monsters ignore friendly fire from same classes
	}

	/* if we are a good guy monster and our attacker is a player
	   or another good guy, do not get mad at them */
	if (targ->monsterinfo.aiflags & AI_GOOD_GUY)
	{
		if (attacker->client || (attacker->monsterinfo.aiflags & AI_GOOD_GUY))
		{
			return;
		}
	}

	// ace - chance to make enemies more...BRUTAL! </end_corny_sfx>
	if ((ACE_RANDOM < (FRAMETIME + (skill->value*FRAMETIME))) &&
		!(targ->monsterinfo.aiflags & AI_BRUTAL))
	{
		targ->monsterinfo.aiflags |= AI_BRUTAL;
		targ->monsterinfo.brutal_framenum = level.framenum + (3+skill->value);
	}
	// ace - end BRUTAL

	/* if attacker is a client, get mad at
	   them because he's good and we're not */
	if (attacker->client)
	{
		targ->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

		/* this can only happen in coop (both new and old
		   enemies are clients)  only switch if can't see
		   the current enemy */
		if (targ->enemy /* && targ->enemy->client */)
		{
			if (visible(targ, targ->enemy))
			{
				targ->oldenemy = attacker;
				return;
			}

			targ->oldenemy = targ->enemy;
		}

		targ->enemy = attacker;

		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
		{
			FoundTarget(targ);
		}

		return;
	}

	/* it's the same base (walk/swim/fly) type and a
	   different classname and it's not a tank
	   (they spray too much), get mad at them */
	// ace - added tank commander, missionpack bosses and medics.  should just make a BOSS flag...
	if ((((targ->flags & (FL_FLY | FL_SWIM)) ==
		 (attacker->flags & (FL_FLY | FL_SWIM))) &&
		(strcmp(targ->classname, attacker->classname) != 0) &&
		(strcmp(attacker->classname, "monster_medic") != 0) &&
		(strcmp(attacker->classname, "monster_medic_commander") != 0) &&
		(strcmp(attacker->classname, "monster_tank") != 0) &&
		(strcmp(attacker->classname, "monster_tank_commander") != 0) &&
		(strcmp(attacker->classname, "monster_supertank") != 0) &&
		(strcmp(attacker->classname, "monster_boss2") != 0) &&
		(strcmp(attacker->classname, "monster_boss5") != 0) &&
		(strcmp(attacker->classname, "monster_zboss") != 0) &&
		(strcmp(attacker->classname, "monster_carrier") != 0) &&
		(strcmp(attacker->classname, "monster_widow") != 0) &&
		(strcmp(attacker->classname, "monster_widow2") != 0) &&
		(strcmp(attacker->classname, "monster_makron") != 0) &&
		(strcmp(attacker->classname, "monster_jorg") != 0)) && 
		(g_infighting->value)) // ace - AND... the infighting cvar is enabled
	{
		if (targ->enemy && targ->enemy->client)
		{
			targ->oldenemy = targ->enemy;
		}

		targ->enemy = attacker;

		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
		{
			FoundTarget(targ);
		}
	}
	/* if they *meant* to shoot us, then shoot back */
	else if (attacker->enemy == targ)
	{
		if (targ->enemy /* && targ->enemy->client */)
		{
			targ->oldenemy = targ->enemy;
		}

		targ->enemy = attacker;

		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
		{
			FoundTarget(targ);
		}
	}
	/* otherwise get mad at whoever they are mad
	   at (help our buddy) unless it is us! */
	else if (attacker->enemy && 
			(attacker->enemy != targ) && 
			(strcmp(attacker->classname,targ->classname) == 0))
	{
		if (targ->enemy /*&& targ->enemy->client*/)
		{
			targ->oldenemy = targ->enemy;
		}

		targ->enemy = attacker->enemy;

		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
		{
			FoundTarget(targ);
		}
	}
}

void
T_Damage(edict_t *targ, edict_t *inflictor, edict_t *attacker,
		vec3_t dir, vec3_t point, vec3_t normal, int damage,
		int knockback, int dflags, int mod)
{
	gclient_t *client;
	int take;
	int save;
	int asave;
	int psave;
	int te_sparks;

	if (!targ || !inflictor || !attacker)
	{
		return;
	}

	if (!targ->takedamage)
	{
		return;
	}

	/* friendly fire avoidance if enabled you
	   can't hurt teammates (but you can hurt
	   yourself) knockback still occurs */
	if ((targ != attacker) && ((deathmatch->value &&
		  ((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS))) ||
		 coop->value))
	{
		if (OnSameTeam(targ, attacker))
		{
			if ((int)(dmflags->value) & DF_NO_FRIENDLY_FIRE)
			{
				damage = 0;
			}
			else
			{
				mod |= MOD_FRIENDLY_FIRE;
			}
		}
	}

	meansOfDeath = mod;

	/* easy mode takes half damage */
	if ((skill->value == 0) && (deathmatch->value == 0) && targ->client)
	{
		damage *= 0.5;

		if (!damage)
		{
			damage = 1;
		}
	}

	client = targ->client;

	if (dflags & DAMAGE_BULLET)
	{
		te_sparks = TE_BULLET_SPARKS;
	}
	else
	{
		te_sparks = TE_SPARKS;
	}

	VectorNormalize(dir);

	/* bonus damage for suprising a monster */
	if (!(dflags & DAMAGE_RADIUS) && (targ->svflags & SVF_MONSTER) &&
		(attacker->client) && (!targ->enemy) && (targ->health > 0))
	{
//		damage *= 2;
		damage *= (1.25 + (ACE_RANDOM*0.75)); // ace - made bonus damage variable
	}

	/* strength tech */
	damage = ApplyStrength(attacker, damage);

	if (targ->flags & FL_NO_KNOCKBACK)
	{
		knockback = 0;
	}

	/* figure momentum add */
	if (!(dflags & DAMAGE_NO_KNOCKBACK))
	{
		// ace - changed this to do knockback on any players/monsters
//		if ((knockback) && (targ->movetype != MOVETYPE_NONE) &&
//			(targ->movetype != MOVETYPE_BOUNCE) &&
//			(targ->movetype != MOVETYPE_PUSH) &&
//			(targ->movetype != MOVETYPE_STOP))
		if ((knockback) && ((targ->svflags & SVF_MONSTER) || (targ->client)))
		{
			vec3_t kvel, flydir;
			float mass;

			// ace - flydir from AQ2 source (loved that knockback effect)
			if ( mod != MOD_FALLING )
			{
				VectorCopy(dir, flydir);
//				flydir[2] += 0.4;
				if (mod == MOD_RAILGUN ||
					mod == MOD_SNIPERRIFLE)
				{
					flydir[2] += 0.15 + (ACE_RANDOM * 0.2); // ace - random variance, railgun
				}
				else if ((dflags & DAMAGE_RADIUS) || 
						mod == MOD_GRENADE ||
						mod == MOD_G_SPLASH ||
						mod == MOD_ROCKET ||
						mod == MOD_R_SPLASH ||
						mod == MOD_BFG_BLAST ||
						mod == MOD_HANDGRENADE ||
						mod == MOD_HG_SPLASH ||
						mod == MOD_HELD_GRENADE ||
						mod == MOD_TRIPBOMB ||
						mod == MOD_RIPPER ||
						mod == MOD_PHALANX ||
						mod == MOD_DISINTEGRATOR ||
						mod == MOD_PROX ||
						mod == MOD_BARREL ||
						
						mod == MOD_BLASTER)
				{
					flydir[2] += 0.1 + (ACE_RANDOM * 0.15); // ace - random variance, all other knockback
				}
			}

			if (targ->mass < MIN_KNOCKBACK_MASS) 
			{
				mass = MIN_KNOCKBACK_MASS;
			}
			else
			{
				mass = targ->mass;
			}

			// ace - TEST: remove groundentity
//			if (targ->velocity[2] != 0) // if any Z velocity on targ (this should go below)
//			if (flydir[2] != 0) // if any Z velocity being added
			{ // comment above to remove groundentity on any knockback
				targ->groundentity = NULL;
			}

			if (targ->client && (attacker == targ))
			{
				/* This allows rocket jumps */
				VectorScale(flydir, 1600.0 * (float)knockback / mass, kvel);
			}
			else
			{
				VectorScale(flydir, 500.0 * (float)knockback / mass, kvel);
			}

			VectorAdd(targ->velocity, kvel, targ->velocity);
		}
	}

	take = damage;
	save = 0;
	// ace - godmode check *should* be disabled on nightmare mode
	/* check for godmode */
	if ((targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		// ace - Godmode will stop you from dying, but not from taking damage.
//		take = 0;
//		save = damage;
//		SpawnDamage(te_sparks, point, normal);
		if (targ->health == 1)
			take = 0;
		if (take >= targ->health)
			take = targ->health - 1;
	}

	/* check for invincibility */
	if ((client && (client->invincible_framenum > level.framenum)) &&
		!(dflags & DAMAGE_NO_PROTECTION))
	{
		if (targ->pain_debounce_time < level.time)
		{
			gi.sound(targ, CHAN_ITEM, gi.soundindex(
						"items/protect4.wav"), 1, ATTN_NORM, 0);
			targ->pain_debounce_time = level.time + 2;
		}

		take = 0;
		save = damage;
	}

	psave = CheckPowerArmor(targ, point, normal, take, dflags);
	take -= psave;

	asave = CheckArmor(targ, point, normal, take, te_sparks, dflags);
	take -= asave;

	/* treat cheat/powerup savings the same as armor */
	asave += save;

	/* team damage avoidance */
	if (!(dflags & DAMAGE_NO_PROTECTION) && false)
	{
		return;
	}

	/* do the damage */
	if (take)
	{
		if ((targ->svflags & SVF_MONSTER) || (client))
		{
			// ace - for randomly applying the unused "moreblood" effect
			int blood_effect = TE_BLOOD;

			if (strcmp(targ->classname, "monster_gekk") == 0)
			{
				blood_effect = TE_GREENBLOOD;
				SpawnDamage(blood_effect, point, normal);
			}
			else // ace - "extra" gore based on AQ2 splats (see a_misc.c) FIXME: show gekk some love (eventually)
			if (mod == MOD_CHAINFIST ||
				mod == MOD_BLASTER ||
				mod == MOD_SHOTGUN ||
				mod == MOD_SSHOTGUN ||
				mod == MOD_MACHINEGUN ||
				mod == MOD_CHAINGUN ||
//				mod == MOD_HYPERBLASTER ||
				mod == MOD_RAILGUN ||
				mod == MOD_SNIPERRIFLE ||
//				mod == MOD_BFG_LASER ||
//				mod == MOD_TARGET_LASER ||
				mod == MOD_TARGET_BLASTER)
	        {
				vec3_t splat;
				vec3_t splatorig;
				float chance = 0.5;
				//vec3_t forward;
				
				if (mod == MOD_CHAINGUN || mod == MOD_MACHINEGUN)
					chance = 0.1;
				else if (mod == MOD_BFG_LASER || mod == MOD_SSHOTGUN)
					chance = 0.15;
				else if (mod == MOD_SHOTGUN)
					chance = 0.2;
				else if (mod == MOD_HYPERBLASTER)
					chance = 0.25;
				else if (mod == MOD_RAILGUN || mod == MOD_SNIPERRIFLE)
					chance = 0.75 + ACE_RANDOM; // ace - almost definitely on sniper/rail shots
				
				if (ACE_RANDOM < chance)
				{
//					VectorMA (targ->s.origin, 50, dir, splat);
					//AngleVectors (attacker->client->v_angle, forward, NULL, NULL);
				
					VectorScale( dir, 20, splat);
					VectorAdd( point, splat, splatorig );

					if (mod == MOD_RAILGUN || mod == MOD_SNIPERRIFLE)
						spray_more_blood( targ, splatorig, dir );
					else
						spray_blood (targ, splatorig, dir, mod );
				}
				else
				{
					if (ACE_RANDOM < 0.5) // every once in a while...
						blood_effect = TE_MOREBLOOD;

					SpawnDamage(blood_effect, point, normal);
				}
	        }
			else
			{
				if (ACE_RANDOM < 0.25) // every once in a while...
					blood_effect = TE_MOREBLOOD;

				SpawnDamage(blood_effect, point, normal);
			}
		}
		else
		{
			SpawnDamage(te_sparks, point, normal);
		}

		// ace - make bfg "absorbe" health to do more damage (in singleplayer/coop)
		if (!deathmatch->value && 
			(mod == MOD_BFG_LASER || mod == MOD_BFG_BLAST || mod == MOD_BFG_EFFECT) )
		{
			inflictor->dmg += take;
			inflictor->dmg_radius += take;
		}

		targ->health = targ->health - take;

		if (targ->health <= 0)
		{
			if ((targ->svflags & SVF_MONSTER) || (client))
			{
				targ->flags |= FL_NO_KNOCKBACK;
			}

			Killed(targ, inflictor, attacker, take, point);
			return;
		}
	}

	if (targ->svflags & SVF_MONSTER)
	{
		M_ReactToDamage(targ, attacker, inflictor);

		if (!(targ->monsterinfo.aiflags & AI_DUCKED) && (take))
		{
			targ->pain(targ, attacker, knockback, take);

			/* nightmare mode monsters don't go into pain frames often */
			if (skill->value == 3)
			{
				targ->pain_debounce_time = level.time + 5;
			}
			if (skill->value > 3) // ace - skill level 4 and above, enemies should rarely stop from "pain"
			{
				float debouncer;
				debouncer = targ->health - take;
				if (debouncer < 5)
					debouncer = 5;
				targ->pain_debounce_time = level.time + debouncer;
			}
			// ace - special case for MOD_TESLA and MOD_HEATBEAM (give monsters teh jittorz)
			if (mod == MOD_TESLA || ((mod == MOD_HEATBEAM) && (targ->health < 200)))
				targ->pain_debounce_time = level.time + (random() * random());
		}
	}
	else if (client)
	{
		if (!(targ->flags & FL_GODMODE) && (take))
		{
			targ->pain(targ, attacker, knockback, take);
		}
	}
	else if (take)
	{
		if (targ->pain)
		{
			targ->pain(targ, attacker, knockback, take);
		}
	}

	/* add to the damage inflicted on a player this frame
	   the total will be turned into screen blends and view
	   angle kicks at the end of the frame */
	if (client)
	{
		client->damage_parmor += psave;
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		VectorCopy(point, client->damage_from);
	}
}

void
T_RadiusDamage(edict_t *inflictor, edict_t *attacker, float damage,
		edict_t *ignore, float radius, int mod)
{
	float points;
	edict_t *ent = NULL;
	vec3_t v;
	vec3_t dir;

	if (!inflictor || !attacker)
	{
		return;
	}

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
		{
			continue;
		}

		if (!ent->takedamage)
		{
			continue;
		}

		VectorAdd(ent->mins, ent->maxs, v);
		VectorMA(ent->s.origin, 0.5, v, v);
		VectorSubtract(inflictor->s.origin, v, v);
		points = damage - 0.5 * VectorLength(v);

		if (ent == attacker)
		{
			points = points * 0.5;
		}

		if (points > 0)
		{
			if (CanDamage(ent, inflictor))
			{
				VectorSubtract(ent->s.origin, inflictor->s.origin, dir);
				T_Damage(ent, inflictor, attacker, dir, inflictor->s.origin,
						vec3_origin, (int)points, (int)points, DAMAGE_RADIUS,
						mod);
			}
		}
	}
}
