/* =======================================================================
 *
 * Additional Weapon Code
 *
 * =======================================================================
 */

#include "../header/local.h"
#include "../monster/misc/player.h"

// Prototypes and defines

extern qboolean is_quad;
extern byte is_silenced;

void Weapon_Generic (edict_t *ent, 
					 int FRAME_ACTIVATE_LAST, 
					 int FRAME_FIRE_LAST, 
					 int FRAME_IDLE_LAST, 
					 int FRAME_DEACTIVATE_LAST, 
					 int *pause_frames, 
					 int *fire_frames, 
					 void (*fire)(edict_t *ent));
void NoAmmoWeaponChange (edict_t *ent);
void check_dodge (edict_t *self, vec3_t start, vec3_t dir, int speed);

void Grenade_Explode(edict_t *ent);
void P_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);

void fire_sconnan (edict_t *self);
void fire_sconnanEffects (edict_t *self);

// void SpawnDamage (int type, vec3_t origin, vec3_t normal, int damage);

// used for rogue weapons
extern byte P_DamageModifier(edict_t *ent);
extern void check_dodge(edict_t *self, vec3_t start, vec3_t dir, int speed);
extern void hurt_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
extern void droptofloor(edict_t *ent);
// extern void Grenade_Explode(edict_t *ent);
extern void drawbbox(edict_t *ent);

/*
 * =======================================================================
 *
 *	Laser Trip Bombs
 *
 * =======================================================================
 */

void shrapnel_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// do damage if we can
	if (!other->takedamage)
		return;

	if (VectorCompare(ent->velocity, vec3_origin))
		return;

	T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin, 
		plane->normal, TBOMB_SHRAPNEL_DMG, 8, 0, MOD_TRIPBOMB);
	G_FreeEdict(ent);
}

void TripBomb_Explode (edict_t *ent)
{
	vec3_t origin;
	int i = 0;

	T_RadiusDamage(ent, ent->owner ? ent->owner : ent, ent->dmg, ent->enemy, ent->dmg_radius, MOD_TRIPBOMB);

	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);

	gi.WriteByte (svc_temp_entity);
	if (ent->waterlevel)
	{
		if (ent->groundentity)
			gi.WriteByte (TE_GRENADE_EXPLOSION_WATER);
		else
			gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
	}
	else
	{
		if (ent->groundentity)
			gi.WriteByte (TE_GRENADE_EXPLOSION);
		else
			gi.WriteByte (TE_ROCKET_EXPLOSION);
	}
	gi.WritePosition (origin);
	gi.multicast (ent->s.origin, MULTICAST_PHS);

	// throw off some debris
	for (i = 0; i < TBOMB_SHRAPNEL; i++)
	{
		edict_t *sh = G_Spawn();
		vec3_t forward, right, up;
		sh->classname = "shrapnel";
		sh->movetype = MOVETYPE_BOUNCE;
		sh->solid = SOLID_BBOX;
		sh->s.effects |= EF_GRENADE;
		sh->s.modelindex = gi.modelindex("models/objects/shrapnel/tris.md2");
		sh->owner = ent->owner;
		VectorSet (sh->avelocity, 300, 300, 300);
		VectorCopy(ent->s.origin, sh->s.origin);
		AngleVectors (ent->s.angles, forward, right, up);
		VectorScale(forward, 500, forward);
		VectorMA(forward, crandom()*500, right, forward);
		VectorMA(forward, crandom()*500, up, forward);
		VectorCopy(forward, sh->velocity);
		sh->touch = shrapnel_touch;
		sh->think = G_FreeEdict;
		sh->nextthink = level.time + 3.0 + crandom() * 1.5;
	}

	G_FreeEdict(ent);
}

void tripbomb_laser_think (edict_t *self)
{
	vec3_t start;
	vec3_t end;
	vec3_t delta;
	trace_t	tr;
	int		count = 8;

	self->nextthink = level.time + FRAMETIME;

	if (level.time > self->timeout)
	{
		// blow up
		self->chain->think = TripBomb_Explode;
		self->chain->nextthink = level.time + FRAMETIME;
		G_FreeEdict(self);
		return;
	}

	// randomly phase out
	if (random() < 0.1)
	{
		return;
	}

	self->svflags &= ~SVF_NOCLIENT;
	VectorCopy (self->s.origin, start);
	VectorMA (start, 2048, self->movedir, end);
	tr = gi.trace (start, NULL, NULL, end, self, MASK_SHOT);

	if (!tr.ent)
		return;

	VectorSubtract(tr.endpos, self->move_origin, delta);
	if (VectorCompare(self->s.origin, self->move_origin))
	{
		// we haven't done anything yet
		VectorCopy(tr.endpos, self->move_origin);
		if (self->spawnflags & 0x80000000)
		{
			self->spawnflags &= ~0x80000000;
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_LASER_SPARKS);
			gi.WriteByte (count);
			gi.WritePosition (tr.endpos);
			gi.WriteDir (tr.plane.normal);
			gi.WriteByte (self->s.skinnum);
			gi.multicast (tr.endpos, MULTICAST_PVS);
		}
	}
	else if (VectorLength(delta) > 1.0)
	{
		// blow up
		self->chain->think = TripBomb_Explode;
		self->chain->nextthink = level.time + FRAMETIME;
		G_FreeEdict(self);
		return;
	}
	VectorCopy(self->move_origin, self->s.old_origin);
}

void tripbomb_laser_on (edict_t *self)
{
	self->svflags &= ~SVF_NOCLIENT;
	self->think = tripbomb_laser_think;

	// play a sound
	gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/ired/las_arm.wav"), 1, ATTN_NORM, 0);
	tripbomb_laser_think(self);
}

void create_tripbomb_laser(edict_t *bomb)
{
	// create the laser
	edict_t *laser = G_Spawn();
	bomb->chain = laser;
	laser->classname = "laser trip bomb laser";
	VectorCopy(bomb->s.origin, laser->s.origin);
	VectorCopy(bomb->s.origin, laser->move_origin);
	VectorCopy(bomb->s.angles, laser->s.angles);
	G_SetMovedir (laser->s.angles, laser->movedir);
	laser->owner = bomb;
	laser->s.skinnum = 0xb0b1b2b3; // <- faint purple  0xf3f3f1f1 <-blue  red-> 0xf2f2f0f0;
	laser->s.frame = 2;
	laser->movetype = MOVETYPE_NONE;
	laser->solid = SOLID_NOT;
	laser->s.renderfx |= RF_BEAM|RF_TRANSLUCENT;
	laser->s.modelindex = 1;
	laser->chain = bomb;
	laser->spawnflags |= 0x80000001;
	laser->think = tripbomb_laser_on;
	laser->nextthink = level.time + FRAMETIME;
	laser->svflags |= SVF_NOCLIENT;
	laser->timeout = level.time + TBOMB_TIMEOUT;
	gi.linkentity (laser);
}

void use_tripbomb(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->chain)
	{
		// we already have a laser, remove it
		G_FreeEdict(self->chain);
		self->chain = NULL;
	}
	else
		// create the laser
		create_tripbomb_laser(self);
}

void turnOffGlow(edict_t *self)
{
	self->s.effects &= ~EF_COLOR_SHELL;
	self->s.renderfx &= ~RF_SHELL_GREEN;
	self->think = NULL;
	self->nextthink = 0;
}

void tripbomb_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	// turn on the glow
	self->damage_debounce_time = level.time + 0.2;

	// if we don't have a think function, then turn this thing on
	if (self->think == NULL)
	{
		self->s.effects |= EF_COLOR_SHELL;
		self->s.renderfx |= RF_SHELL_GREEN;
		self->nextthink = self->damage_debounce_time;
		self->think = turnOffGlow;
	}
}

void tripbomb_think(edict_t *self)
{
	if (self->chain == NULL)
	{
		// check whether we need to create the laser
		if (self->timeout < level.time)
		{
			create_tripbomb_laser(self);
		}
	}

	// do we need to show damage?
	if (self->damage_debounce_time > level.time)
	{
		self->s.effects |= EF_COLOR_SHELL;
		self->s.renderfx |= RF_SHELL_GREEN;
	}
	else
	{
		self->s.effects &= ~EF_COLOR_SHELL;
		self->s.renderfx &= ~RF_SHELL_GREEN;
	}

	self->nextthink = level.time + FRAMETIME;
}

void setupBomb(edict_t *bomb, char *classname, float damage, float damage_radius)
{
	bomb->classname = classname;
	VectorSet(bomb->mins, -8, -8, -8);
	VectorSet(bomb->maxs, 8, 8, 8);
	bomb->solid = SOLID_BBOX;
	bomb->movetype = MOVETYPE_NONE;
	bomb->s.modelindex = gi.modelindex("models/objects/ired/tris.md2");
	bomb->radius_dmg = damage;
	bomb->dmg = damage;
	bomb->dmg_radius = damage_radius;
	bomb->health = 1;
	bomb->takedamage = DAMAGE_YES; // DAMAGE_IMMORTAL;
	bomb->flags |= FL_GODMODE; // health will not be deducted
	bomb->pain = tripbomb_pain;
}

void removeOldest()
{
	edict_t *oldestEnt = NULL;
	edict_t *e = NULL;
	int count = 0;

	while(1)
	{
		e = G_Find(e, FOFS(classname), "ired");
		if (e == NULL) // no more
			break;

		count++;

		if (oldestEnt == NULL ||
			e->timestamp < oldestEnt->timestamp)
		{
			oldestEnt = e;
		}
	}

	// do we have too many?
	if (count > TBOMB_MAX_EXIST && oldestEnt != NULL)
	{
		// get this tbomb to explode
		oldestEnt->think = TripBomb_Explode;
		oldestEnt->nextthink = level.time + FRAMETIME;
		G_FreeEdict(oldestEnt->chain);
	}
}

qboolean fire_lasertripbomb(edict_t *self, vec3_t start, vec3_t dir, float timer, float damage, float damage_radius, qboolean quad)
{
	// trace a line
	trace_t tr;
	vec3_t endPos;
	vec3_t _dir;
	edict_t *bomb = NULL;

	VectorScale(dir, 64, _dir);
	VectorAdd(start, _dir, endPos);

	// trace ahead, looking for a wall
	tr = gi.trace(start, NULL, NULL, endPos, self, MASK_SHOT);
	if (tr.fraction == 1.0)
	{
		// not close enough
		return false;
	}

	// only place tripmines on the "world" itself
	if (Q_stricmp(tr.ent->classname, "worldspawn") != 0)
	{
		return false;
	}

	// create the bomb
	bomb = G_Spawn();
	VectorMA(tr.endpos, 3, tr.plane.normal, bomb->s.origin);
	vectoangles(tr.plane.normal, bomb->s.angles);
	bomb->owner = self;
	setupBomb(bomb, "ired", damage, damage_radius);
	gi.linkentity(bomb);

	bomb->timestamp = level.time;
	bomb->timeout = level.time + timer;
	bomb->nextthink = level.time + FRAMETIME;
	bomb->think = tripbomb_think;

	// remove the oldest trip bomb
	removeOldest();

	// play a sound
	gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/ired/las_set.wav"), 1, ATTN_NORM, 0);
	return true;
}

void weapon_lasertripbomb_fire (edict_t *ent)
{
	if (ent->client->ps.gunframe == 10)
	{
		vec3_t	offset;
		vec3_t	forward;
		vec3_t	start;
		int damage = TBOMB_DAMAGE;
		float radius = TBOMB_RADIUS_DAMAGE;
		if (is_quad)
			damage *= 4;

		// place the trip bomb
		VectorSet(offset, 0, 0, ent->viewheight * 0.75);
		AngleVectors (ent->client->v_angle, forward, NULL, NULL);
		VectorAdd(ent->s.origin, offset, start);

		if (fire_lasertripbomb(ent, start, forward, TBOMB_DELAY, damage, radius, is_quad))
		{
			if (!((int)dmflags->value & DF_INFINITE_AMMO))
			{
				ent->client->pers.inventory[ent->client->ammo_index] -= 1;
			}
			
			// switch models
			ent->client->ps.gunindex = gi.modelindex("models/weapons/v_ired/hand.md2");

		}
	}
	else if (ent->client->ps.gunframe == 15)
	{
		// switch models back
		int mi = gi.modelindex("models/weapons/v_ired/tris.md2");
		if (ent->client->ps.gunindex != mi)
		{
			ent->client->ps.gunindex = mi;
			// go back to get another trip bomb
			ent->client->ps.gunframe = 0;
			return;
		}
	}
	else if (ent->client->ps.gunframe == 6)
	{
		ent->client->ps.gunframe = 16;
		return;
	}

	ent->client->ps.gunframe++;
}

void Weapon_LaserTripBomb(edict_t *ent)
{
	static int	pause_frames[]	= {24, 33, 43, 0};
	static int	fire_frames[]	= {6, 10, 15, 0};

	const int deactivateFirst = 44;
	const int deactivateLast = 48;
	const int idleFirst = 16;
	const int idleLast = 43;
	const int fireFirst = 7;
	const int activateLast = 6;
	
	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->ps.gunframe == deactivateLast)
		{
			ChangeWeapon (ent);
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent->client->ps.gunframe == activateLast)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = idleFirst;
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING))
	{
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = deactivateFirst;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if(ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->ps.gunframe = fireFirst;
				ent->client->weaponstate = WEAPON_FIRING;

				// start the animation
				ent->client->anim_priority = ANIM_ATTACK;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crattak1-1;
					ent->client->anim_end = FRAME_crattak9;
				}
				else
				{
					ent->s.frame = FRAME_attack1-1;
					ent->client->anim_end = FRAME_attack8;
				}
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
		}
		else
		{
			if (ent->client->ps.gunframe == idleLast)
			{
				ent->client->ps.gunframe = idleFirst;
				return;
			}

			int n = 0;
			for (n = 0; pause_frames[n]; n++)
			{
				if (ent->client->ps.gunframe == pause_frames[n])
				{
					if (rand()&15)
						return;
				}
			}

			ent->client->ps.gunframe++;
			return;
		}
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		int n = 0;
		for (n = 0; fire_frames[n]; n++)
		{
			if (ent->client->ps.gunframe == fire_frames[n])
			{
				weapon_lasertripbomb_fire(ent);
				break;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == idleFirst+1)
			ent->client->weaponstate = WEAPON_READY;
	}
}

void SP_misc_lasertripbomb(edict_t *bomb)
{
	// precache
	gi.soundindex("weapons/ired/las_set.wav");
	gi.soundindex("weapons/ired/las_arm.wav");
	gi.modelindex("models/objects/shrapnel/tris.md2");
	gi.modelindex("models/objects/ired/tris.md2");

	if (bomb->spawnflags & CHECK_BACK_WALL)
	{
		vec3_t forward, endPos;
		trace_t tr;
		// look backwards toward a wall
		AngleVectors (bomb->s.angles, forward, NULL, NULL);
		VectorMA(bomb->s.origin, -64.0, forward, endPos);
		tr = gi.trace(bomb->s.origin, NULL, NULL, endPos, bomb, MASK_SOLID);
		VectorCopy(tr.endpos, bomb->s.origin);
		vectoangles(tr.plane.normal, bomb->s.angles);
	}

	// set up ourself
	setupBomb(bomb, "misc_ired", TBOMB_DAMAGE, TBOMB_RADIUS_DAMAGE);
	
	if (bomb->targetname)
	{
		bomb->use = use_tripbomb;
	}
	else
	{
		bomb->think = create_tripbomb_laser;
		bomb->nextthink = level.time + TBOMB_DELAY;
	}
	gi.linkentity(bomb);
}

/*
 * =======================================================================
 *
 *	Sonic Cannon
 *
 * =======================================================================
 */

void weapon_sc_fire (edict_t *ent)
{
	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe++;

		if(ent->client->weapon_sound && ent->client->ps.gunframe < 18)
		{
			ent->client->ps.gunframe = 18;
		}
	}
	else
	{
		if(!ent->client->startFireTime)
		{
			ent->client->startFireTime = level.time;
		}
		else if(level.time - ent->client->startFireTime >= SC_MAXFIRETIME)
		{
			ent->client->ps.gunframe = 17;
		}
	else
    {
		int old_cells = (int)ent->dmg_radius;

		ent->dmg_radius = ((level.time - ent->client->startFireTime) /  SC_MAXFIRETIME) * SC_MAXCELLS;
		if(old_cells < (int)ent->dmg_radius)
		{
			old_cells = (int)ent->dmg_radius - old_cells;
			if(ent->client->pers.inventory[ent->client->ammo_index] < old_cells)
			{
				ent->dmg_radius -= (old_cells - ent->client->pers.inventory[ent->client->ammo_index]);
				if (!((int)dmflags->value & DF_INFINITE_AMMO))
				{
					ent->client->pers.inventory[ent->client->ammo_index] = 0;
				}
			}
			else
			{
				if (!((int)dmflags->value & DF_INFINITE_AMMO))
				{
					ent->client->pers.inventory[ent->client->ammo_index] -= old_cells;
				}
			}
		  }
		}

		if(!ent->client->pers.inventory[ent->client->ammo_index])
		{
		  ent->client->ps.gunframe = 17;

				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
			else
			{
				if(ent->weaponsound_time < level.time)
				{
					ent->client->weapon_sound = gi.soundindex("weapons/sonic/sc_fire.wav");
				}
			}

			fire_sconnanEffects (ent);

			ent->client->ps.gunframe++;
			if (ent->client->ps.gunframe == 18 && 
			(level.time - ent->client->startFireTime) < SC_MAXFIRETIME &&
			ent->client->pers.inventory[ent->client->ammo_index])
				ent->client->ps.gunframe = 12;
		}

	if (ent->client->ps.gunframe == 18)
	{
		ent->client->weapon_sound = 0;
		ent->weaponsound_time = 0;

		if (!is_silenced)
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/sonic/sc_cool.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/sonic/sc_cool.wav"), 0.4, ATTN_NORM, 0);

		if(ent->dmg_radius)
		{
			fire_sconnan (ent);
		}

		ent->dmg_radius = 0;
		ent->client->startFireTime = 0;
	}
}

void Weapon_SonicCannon (edict_t *ent)
{
	static int	pause_frames[] = {32, 42, 52, 0};
	static int	fire_frames[]	= {12, 13, 14, 15, 16, 17, 0};

	if (ent->client->ps.gunframe == 0)
	{
		if (deathmatch->value)
		{
			if (!is_silenced)
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/sonic/sc_act.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/sonic/sc_act.wav"), 0.4, ATTN_NORM, 0);
		}
    ent->weaponsound_time = 0;
    ent->client->startFireTime = 0;
    ent->dmg_radius = 0;
  }
  else if (ent->client->ps.gunframe == 53)
  {
		if (deathmatch->value)
		{
			if (!is_silenced)
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/sonic/sc_dact.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/sonic/sc_dact.wav"), 0.4, ATTN_NORM, 0);
		}
  }
  else if((ent->client->buttons & BUTTON_ATTACK) && ent->weaponsound_time == 0)
  {
    ent->weaponsound_time = level.time + 0.4;

		if (!is_silenced)
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/sonic/sc_warm.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/sonic/sc_warm.wav"), 0.4, ATTN_NORM, 0);
  }

  Weapon_Generic (ent, 6, 22, 52, 57, pause_frames, fire_frames, weapon_sc_fire);
}

void fire_sconnanEffects (edict_t *self)
{
	vec3_t		start, end;
	vec3_t		forward, right;
	vec3_t		offset, v;
	trace_t		tr;
  
	AngleVectors (self->client->v_angle, forward, right, NULL);

	VectorScale (forward, -3, self->client->kick_origin);
	self->client->kick_angles[0] = -3;

	VectorSet(offset, 0, 7,  self->viewheight-8);
	P_ProjectSource (self->client, self->s.origin, offset, forward, right, start);

	VectorMA (start, 8192, forward, end);

  tr = gi.trace (start, NULL, NULL, end, self, MASK_SHOT|CONTENTS_SLIME|CONTENTS_LAVA);

	VectorMA (tr.endpos, -5, forward, end);

  VectorSet(v, crandom() * 10 - 20, crandom() * 10 - 20, crandom() * 10 - 20);
  SpawnDamage(TE_SHIELD_SPARKS, end, v); // , 0);
}

void scexplode_think(edict_t *self)
{
  gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_ROCKET_EXPLOSION);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	G_FreeEdict (self);
}

void fire_sconnan (edict_t *self)
{
	vec3_t		start, end, explodepos;
	vec3_t		forward, right, up;
	vec3_t		offset;
	trace_t		tr;
	float damage;
	float radius;

	damage = self->dmg_radius / SC_MAXCELLS;
	radius = damage * SC_MAXRADIUS;
	damage = SC_BASEDAMAGE + (damage * SC_DAMGERANGE);

	AngleVectors (self->client->v_angle, forward, right, up);

	VectorScale (forward, -3, self->client->kick_origin);
	self->client->kick_angles[0] = -3;

	VectorSet(offset, 0, 7,  self->viewheight-8);
	P_ProjectSource (self->client, self->s.origin, offset, forward, right, start);

	VectorMA (start, 8192, forward, end);

	tr = gi.trace (start, NULL, NULL, end, self, MASK_SHOT|CONTENTS_SLIME|CONTENTS_LAVA);

	if ((tr.ent != self) && (tr.ent->takedamage))
	{
		T_Damage (tr.ent, self, self, forward, tr.endpos, tr.plane.normal, damage, 0, 0, MOD_SONICCANNON);
	}

	T_RadiusDamagePosition (tr.endpos, self, self, damage, tr.ent, radius, MOD_SONICCANNON);

	VectorMA (tr.endpos, -5, forward, end);

  gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_ROCKET_EXPLOSION);
	gi.WritePosition (end);
	gi.multicast (self->s.origin, MULTICAST_PHS);

  damage -= 100;
  radius = 0.1;

  while(damage > 0)
  {
	  edict_t	*explode;
	  
		// ace - make this proportionate to the damage (vs 50)
    VectorMA (end, ((damage/2) * crandom()) - 5, forward, explodepos);
  	VectorMA (explodepos, ((damage/2) * crandom()) - 5, right, explodepos);
  	VectorMA (explodepos, ((damage/2) * crandom()) - 5, up, explodepos);

    explode = G_Spawn();
  	VectorCopy (explodepos, explode->s.origin);

  	explode->classname = "sconnanExplode";
  	explode->nextthink = level.time + radius;
  	explode->think = scexplode_think;

    radius += 0.1;
    damage -= 100;
  }

}

/*
 * =======================================================================
 *
 *	Sniper Rifle
 *
 * =======================================================================
 */
void fire_sniper_bullet (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick)
{
	trace_t tr;
	vec3_t end;
	vec3_t s;
	edict_t *ignore = self;
	VectorMA (start, 8192, aimdir, end);
	VectorCopy(start, s);
	while(1)
	{
		tr = gi.trace (s, NULL, NULL, end, ignore, MASK_SHOT /*MASK_SHOT_NO_WINDOW*/);
		if (tr.fraction >= 1.0)
			return;

		// if we hit a plasmashield, then pass thru it
		if (Q_stricmp(tr.ent->classname, "PlasmaShield") == 0)
		{
			ignore = tr.ent;
			VectorCopy(tr.endpos, s);
		}
		else
			break;
	}

	gi.WriteByte (svc_temp_entity);
	if (gi.pointcontents(tr.endpos) & MASK_WATER)
	{
		if (tr.plane.normal[2] > 0.7)
			gi.WriteByte (TE_GRENADE_EXPLOSION_WATER);
		else
			gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
	}
	else
	{
		if (tr.plane.normal[2] > 0.7)
			gi.WriteByte (TE_GRENADE_EXPLOSION);
		else
			gi.WriteByte (TE_ROCKET_EXPLOSION);
	}
	gi.WritePosition (tr.endpos);
	gi.multicast (tr.endpos, MULTICAST_PHS);

	if (tr.ent->takedamage)
		T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_NO_ARMOR, MOD_SNIPERRIFLE);
}

void weapon_sniperrifle_fire (edict_t *ent)
{
	vec3_t forward, right;
	vec3_t offset, start;
	int damage;
	int kick;

	if (deathmatch->value)
	{	// normal damage is too extreme in dm
		damage = 150;
		kick = 300;
	}
	else
	{
		damage = 250;
		kick = 400;
	}

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);
	// centre the shot
	VectorSet(offset, 0, 0, ent->viewheight);
	VectorAdd(ent->s.origin, offset, start);
	fire_sniper_bullet(ent, start, forward, damage, kick);

	if (!is_silenced)
	{
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/sniper/fire.wav"), 1, ATTN_NONE /*ATTN_NORM*/, 0);
	  	PlayerNoise(ent, start, PNOISE_WEAPON); // ace - moved this here
	}
	else
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/sniper/fire.wav"), 0.4, ATTN_NORM, 0);

	VectorScale (forward, -20, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;
	if (!((int)dmflags->value & DF_INFINITE_AMMO))
	{
		ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;
	}
}

void Weapon_SniperRifle(edict_t *ent)
{
	/*
		Activate/Deactivate
		0 - 8	: Activate
		9 - 18	: Fire
		19 - 27 : Idle 1
		28 - 36	: Idle 2
		37 - 41	: Deactivate

		Zoom
		0 - 1 Zoom
		Hold 1 while zoomed
	*/
	const static int activateStart = 0;
	const static int activateEnd = 8;
	const static int deactivateStart = 37;
	const static int deactivateEnd = 41;
	const static int spFov = 25; // 15;
	const static int dmFov = 30;
	
	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		ent->client->sniperFramenum = 0;
		if (ent->client->ps.gunframe == deactivateStart)
		{
			// back to old fov
			ent->client->ps.fov = 90;	
			if (deathmatch->value)
				gi.sound(ent, CHAN_BODY/*CHAN_WEAPON2*/, gi.soundindex("weapons/sniper/snip_bye.wav"), 1, ATTN_NORM, 0);
		}
		else if (ent->client->ps.gunframe == deactivateEnd)
		{
			ChangeWeapon(ent);
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent->client->ps.gunframe == activateStart)
		{
			// play the activation sound
			if (deathmatch->value)
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/sniper/snip_act.wav"), 1, ATTN_NORM, 0);
		}
		else if (ent->client->ps.gunframe == activateEnd)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunindex = (deathmatch->value ? 
				gi.modelindex("models/weapons/v_sniper/dmscope/tris.md2") :
				gi.modelindex("models/weapons/v_sniper/scope/tris.md2") );
			ent->client->ps.gunframe = 0;
			ent->client->ps.fov = (deathmatch->value ? dmFov : spFov);
			ent->client->sniperFramenum = level.framenum + SNIPER_CHARGE_TIME;
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING))
	{
		// back to other gun model
		ent->client->ps.gunindex = gi.modelindex("models/weapons/v_sniper/tris.md2");
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = deactivateStart;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		ent->client->ps.gunindex = (deathmatch->value ? 
			gi.modelindex("models/weapons/v_sniper/dmscope/tris.md2") :
			gi.modelindex("models/weapons/v_sniper/scope/tris.md2") );
		
		ent->client->ps.fov = (deathmatch->value ? dmFov : spFov);

		// beep if the sniper frame num is a multiple of 10
		if (ent->client->sniperFramenum >= level.framenum)
		{
			if ((ent->client->sniperFramenum - level.framenum) % 10 == 1)
				gi.sound(ent, CHAN_BODY/*CHAN_WEAPON2*/, gi.soundindex("weapons/sniper/beep.wav"), 1, ATTN_NORM, 0);
		}

		if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
		{
			if (level.framenum >= ent->client->sniperFramenum)
			{
				ent->client->latched_buttons &= ~BUTTON_ATTACK;
				if ((!ent->client->ammo_index) || 
					( ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity))
				{
					ent->client->weaponstate = WEAPON_FIRING;

					// start the animation
					ent->client->anim_priority = ANIM_ATTACK;
					if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
					{
						ent->s.frame = FRAME_crattak1-1;
						ent->client->anim_end = FRAME_crattak9;
					}
					else
					{
						ent->s.frame = FRAME_attack1-1;
						ent->client->anim_end = FRAME_attack8;
					}
				}
				else
				{
					if (level.time >= ent->pain_debounce_time)
					{
						gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
						ent->pain_debounce_time = level.time + 1;
					}
					NoAmmoWeaponChange (ent);
				}
			}
		}
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		ent->client->ps.gunindex = (deathmatch->value ? 
				gi.modelindex("models/weapons/v_sniper/dmscope/tris.md2") :
				gi.modelindex("models/weapons/v_sniper/scope/tris.md2") );
			
		ent->client->ps.fov = (deathmatch->value ? dmFov : spFov);

		// fire
		weapon_sniperrifle_fire(ent);
		
		// start recharge
		ent->client->weaponstate = WEAPON_READY;
		ent->client->sniperFramenum = level.framenum + SNIPER_CHARGE_TIME;
	}
}

/*
 * =======================================================================
 *
 *	Push (offhand push/shove)
 *
 *  Frame 0 - Start push
 *  Frame 4 - Contact
 *  Frame 8 - End push
 *
 * =======================================================================
 */

qboolean push_hit (edict_t *self, vec3_t start, vec3_t aim, int damage, int kick)
{
	trace_t tr;
	vec3_t end;
	vec3_t v;

	//see if enemy is in range
//	VectorMA(start, 64, aim, end);
	VectorMA(start, 72, aim, end);

	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);
	if (tr.fraction >= 1)
		return false;

	// play sound
	gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/push/contact.wav"), 1, ATTN_NORM, 0);

	if (tr.ent->svflags & SVF_MONSTER ||
		tr.ent->client)
	{
		// do our special form of knockback here
		VectorMA (tr.ent->absmin, 0.75, tr.ent->size, v);
		VectorSubtract (v, start, v);
		VectorNormalize (v);
		v[2] += 0.4; // push up
		VectorMA (tr.ent->velocity, kick, v, tr.ent->velocity);
		if (tr.ent->velocity[2] > 0)
			tr.ent->groundentity = NULL;
	}
/*	else if (tr.ent->movetype == MOVETYPE_FALLFLOAT)
	{
		if (tr.ent->touch)
		{
			float mass = tr.ent->mass;
			tr.ent->mass *= 0.25;
			tr.ent->touch(tr.ent, self, NULL, NULL);
			tr.ent->mass = mass;
		}
	} */

	// ok, we hit something, damage it
	if (!tr.ent->takedamage)
		return false;

	// do the damage
	T_Damage (tr.ent, self, self, aim, tr.endpos, vec3_origin, damage, kick/2, DAMAGE_NO_KNOCKBACK, MOD_HIT);

	return true;
}

void Action_Push (edict_t *ent)
{
	if (ent->client->ps.gunframe == 0)
	{
		ent->client->ps.gunframe++;
	}
	else if (ent->client->ps.gunframe == 4)
	{
		vec3_t forward;
		vec3_t offset;
		vec3_t start;
		
		int dmg, kick;
		
		dmg = 5 + (rand() & 10);
		kick = 512;

		if (is_quad) // yes, you can quad-push/kick
		{
			dmg *= 4;
			kick *= 2;
		}
		
		// contact
		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight * 0.5);
		VectorAdd(ent->s.origin, offset, start);
		push_hit(ent, start, forward, dmg, kick);
		ent->client->ps.gunframe++;
	}
	else if (ent->client->ps.gunframe == 8)
	{
		// go back to old weapon
		ent->client->newweapon = ent->client->pers.lastweapon;
		ChangeWeapon(ent);
	}
	else
		ent->client->ps.gunframe++;
}

/*
 * =======================================================================
 *
 *	Grappling Hook (From CTF)
 *
 * =======================================================================
 */

/* ent is player */
void
PlayerResetGrapple(edict_t *ent)
{
	if (ent->client && ent->client->grapple)
	{
		ResetGrapple(ent->client->grapple);
	}
}

/* self is grapple, not player */
void
ResetGrapple(edict_t *self)
{
	if (self->owner->client->grapple)
	{
		float volume = 1.0;
		gclient_t *cl;

		if (self->owner->client->silencer_shots)
		{
			volume = 0.2;
		}

		gi.sound(self->owner, CHAN_RELIABLE + CHAN_WEAPON,
				gi.soundindex( "weapons/grapple/grreset.wav"), volume, ATTN_NORM, 0);
		cl = self->owner->client;
		cl->grapple = NULL;
		cl->grapplereleasetime = level.time;
		cl->grapplestate = GRAPPLE_STATE_FLY; /* we're firing, not on hook */
		cl->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
		G_FreeEdict(self);
	}
}

void
GrappleTouch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	float volume = 1.0;

	if (other == self->owner)
	{
		return;
	}

	if (self->owner->client->grapplestate != GRAPPLE_STATE_FLY)
	{
		return;
	}

	if (surf && (surf->flags & SURF_SKY))
	{
		ResetGrapple(self);
		return;
	}

	// ace - gravity "fix"
	self->gravity = 0;

	VectorCopy(vec3_origin, self->velocity);

	PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		T_Damage(other, self, self->owner, self->velocity, self->s.origin,
				plane->normal, self->dmg, 1, 0, MOD_GRAPPLE);
//		ResetGrapple(self);
//		return;
	}

	self->owner->client->grapplestate = GRAPPLE_STATE_PULL; /* we're on hook */
	self->enemy = other;

	self->solid = SOLID_NOT;

	if (self->owner->client->silencer_shots)
	{
		volume = 0.2;
	}

	gi.sound(self->owner, CHAN_RELIABLE + CHAN_WEAPON,
			gi.soundindex("weapons/grapple/grpull.wav"), volume, ATTN_NORM, 0);
	gi.sound(self, CHAN_WEAPON, gi.soundindex(
					"weapons/grapple/grhit.wav"), volume, ATTN_NORM, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPARKS);
	gi.WritePosition(self->s.origin);

	if (!plane)
	{
		gi.WriteDir(vec3_origin);
	}
	else
	{
		gi.WriteDir(plane->normal);
	}

	gi.multicast(self->s.origin, MULTICAST_PVS);
}

/* 
 * Draw beam between grapple and self 
 */
void
GrappleDrawCable(edict_t *self)
{
	vec3_t offset, start, end, f, r;
	vec3_t dir;
	float distance;

	AngleVectors(self->owner->client->v_angle, f, r, NULL);
	VectorSet(offset, 16, 16, self->owner->viewheight - 8);
	P_ProjectSource(self->owner->client, self->owner->s.origin,
			offset, f, r, start);

	VectorSubtract(start, self->owner->s.origin, offset);

	VectorSubtract(start, self->s.origin, dir);
	distance = VectorLength(dir);

	/* don't draw cable if close */
	if (distance < 64)
	{
		return;
	}

	/* adjust start for beam origin being in middle of a segment */
	VectorCopy(self->s.origin, end);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_GRAPPLE_CABLE);
	gi.WriteShort(self->owner - g_edicts);
	gi.WritePosition(self->owner->s.origin);
	gi.WritePosition(end);
	gi.WritePosition(offset);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

void SV_AddGravity(edict_t *ent);

/* 
 * pull the player toward the grapple 
 */
void
GrapplePull(edict_t *self)
{
	vec3_t hookdir, v;
	float vlen;
//	trace_t	tr;

	// ace - this grapple is not "offhand" optional
	if (self->owner->client->pers.weapon != FindItem("grapple"))
	{
		ResetGrapple(self);
		return;
	}

	// ace - check for teleporters
	if (self->owner->s.event == EV_PLAYER_TELEPORT)
	{
		ResetGrapple(self);
		return;
	}

	if ((strcmp(self->owner->client->pers.weapon->classname,
				 "weapon_grapple") == 0) &&
		!self->owner->client->newweapon &&
		(self->owner->client->weaponstate != WEAPON_FIRING) &&
		(self->owner->client->weaponstate != WEAPON_ACTIVATING))
	{
		ResetGrapple(self);
		return;
	}

	if (self->enemy)
	{
		if (self->enemy->solid == SOLID_NOT || self->enemy->deadflag)
		{
			ResetGrapple(self);
			return;
		}

		if (self->enemy->solid == SOLID_BBOX)
		{
			VectorScale(self->enemy->size, 0.5, v);
			VectorAdd(v, self->enemy->s.origin, v);
			VectorAdd(v, self->enemy->mins, self->s.origin);
			gi.linkentity(self);
		}
		else
		{
			VectorCopy(self->enemy->velocity, self->velocity);
		}

		if (self->enemy->takedamage /*&&
			!CheckTeamDamage(self->enemy, self->owner)*/ && (level.time > self->timeout))
		{
			float volume = 1.0;

			self->timeout = level.time + FRAMETIME + random();

			if (self->owner->client->silencer_shots)
			{
				volume = 0.2;
			}

			T_Damage(self->enemy, self, self->owner, self->velocity,
					self->s.origin, vec3_origin, 1, 1, 0, MOD_GRAPPLE);
			// ace - no grapple/
			gi.sound(self, CHAN_WEAPON, gi.soundindex("misc/fhit3.wav"), volume, ATTN_NORM, 0);
		}

		if (self->enemy->deadflag) /* he died */
		{
			ResetGrapple(self);
			return;
		}
		
		// ace - pull towards enemy?
		if (self->enemy->client || (self->enemy->svflags & SVF_MONSTER))
		{
			VectorCopy (self->enemy->s.origin, self->s.origin);
			gi.linkentity(self);
		}
	}

	// ace - check for "breaks" in the line
	if (!visible (self, self->owner))
	{
		ResetGrapple(self);
		return;
	}
	
/*	tr = gi.trace(self->s.origin, NULL, NULL, self->owner->s.origin, self, MASK_SHOT);

	if ((tr.ent != NULL) && 
		(tr.ent != self->enemy) && 
		(tr.ent != self) && 
		(tr.ent != self->owner))
	{	// incase we have an "enemy" set, broken ignore
		ResetGrapple(self);
		return;
	}*/

//	if (tr.fraction < 1.0)
//	{	// should be a straight shot
//		ResetGrapple(self);
//		return;
//	}


	GrappleDrawCable(self);

	if (self->owner->client->grapplestate > GRAPPLE_STATE_FLY)
	{
		/* pull player toward grapple 
		   this causes icky stuff with prediction, we need to extend/
		   the prediction layer to include two new fields in the player/
		   move stuff: a point and a velocity. The client should add 
		   that velociy in the direction of the point */
		vec3_t forward, up;

		AngleVectors(self->owner->client->v_angle, forward, NULL, up);
		VectorCopy(self->owner->s.origin, v);
		v[2] += self->owner->viewheight;
		VectorSubtract(self->s.origin, v, hookdir);

		vlen = VectorLength(hookdir);

		if ((self->owner->client->grapplestate ==
			 GRAPPLE_STATE_PULL) &&
			(vlen < GRAPPLE_HANG_DIST)) // ace - grapple hand distance set here, stock is 64
		{
			float volume = 1.0;

			if (self->owner->client->silencer_shots)
			{
				volume = 0.2;
			}

			self->owner->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
			gi.sound(self->owner, CHAN_RELIABLE + CHAN_WEAPON,
					gi.soundindex("weapons/grapple/grhang.wav"), volume, ATTN_NORM, 0);
			self->owner->client->grapplestate = GRAPPLE_STATE_HANG;
		}

		// ace - test "swinging" physics
		if (self->owner->client->grapplestate == GRAPPLE_STATE_HANG)
		{
			vec3_t	offset, start, forward, right;
			vec3_t velpart;			// player's velocity component moving to or away from hook
			float force;			// restrainment force
//			vec3_t	chainvec;		// chain's vector
//			float chainlen;			// length of extended chain

			// derive start point of chain
			AngleVectors (self->owner->client->v_angle, forward, right, NULL);
			VectorSet(offset, 8, 8, self->owner->viewheight-8);
			P_ProjectSource_Reverse (self->owner->client, self->owner->s.origin, offset, forward, right, start);

			// get info about chain
			_VectorSubtract (self->s.origin, start, hookdir);
			vlen = VectorLength (hookdir);

			// if player's location is beyond the chain's reach
			if (vlen > GRAPPLE_HANG_DIST)	
			{	 
				// determine player's velocity component of chain vector
				VectorScale (hookdir, _DotProduct (self->owner->velocity, hookdir) / _DotProduct (hookdir, hookdir), velpart);
		
				// restrainment default force 
				force = (vlen - GRAPPLE_HANG_DIST) * 5;

				// if player's velocity heading is away from the hook
				if (_DotProduct (self->owner->velocity, hookdir) < 0)
				{
					// if chain has streched for 25 units
					if (vlen > GRAPPLE_HANG_DIST + 25)
						// remove player's velocity component moving away from hook
						_VectorSubtract(self->owner->velocity, velpart, self->owner->velocity);
				}
				else  // if player's velocity heading is towards the hook
				{
					if (VectorLength (velpart) < force)
						force -= VectorLength (velpart);
					else		
						force = 0;
				}
			}
			else
				force = 0;

			// disable prediction while suspended in air by hook
			// if server console variable hook_no_pred is set 
			if (!(self->owner->client->ps.pmove.pm_flags & PMF_ON_GROUND))
			{
//				if (hook_no_pred->value)
					self->owner->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
			}	
			else
				self->owner->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;

			// applys chain restrainment 
			VectorNormalize (hookdir);
			VectorMA (self->owner->velocity, force, hookdir, self->owner->velocity);
		}
		else
		{
			VectorNormalize(hookdir);
			VectorScale(hookdir, GRAPPLE_PULL_SPEED, hookdir);
			VectorCopy(hookdir, self->owner->velocity);
			SV_AddGravity(self->owner);
		}
	}
}

void
FireGrapple(edict_t *self, vec3_t start, vec3_t dir,
		int damage, int speed, int effect)
{
	edict_t *grapple;
	trace_t tr;

	VectorNormalize(dir);

	grapple = G_Spawn();
	VectorCopy(start, grapple->s.origin);
	VectorCopy(start, grapple->s.old_origin);
	vectoangles(dir, grapple->s.angles);
	VectorScale(dir, speed, grapple->velocity);
	grapple->movetype = MOVETYPE_TOSS; // MOVETYPE_FLYMISSILE;
	grapple->gravity = 0.25; // ace - "floaty" shot 
	grapple->clipmask = MASK_SHOT;
	grapple->solid = SOLID_BBOX;
	grapple->s.effects |= effect;
	VectorClear(grapple->mins);
	VectorClear(grapple->maxs);
	grapple->s.modelindex = gi.modelindex("models/weapons/grapple/hook/tris.md2");
	grapple->owner = self;
	grapple->touch = GrappleTouch;
	grapple->dmg = damage;
	self->client->grapple = grapple;
	self->client->grapplestate = GRAPPLE_STATE_FLY; /* we're firing, not on hook */
	gi.linkentity(grapple);

	tr = gi.trace(self->s.origin, NULL, NULL, grapple->s.origin, grapple, MASK_SHOT);

	if (tr.fraction < 1.0)
	{
		VectorMA(grapple->s.origin, -10, dir, grapple->s.origin);
		grapple->touch(grapple, tr.ent, NULL, NULL);
	}
}

void
GrappleFire(edict_t *ent, vec3_t g_offset, int damage, int effect)
{
	vec3_t forward, right;
	vec3_t start;
	vec3_t offset;
	float volume = 1.0;

	if (ent->client->grapplestate > GRAPPLE_STATE_FLY)
	{
		return; /* it's already out */
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight - 8 + 2);
	VectorAdd(offset, g_offset, offset);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	if (ent->client->silencer_shots)
	{
		volume = 0.2;
	}

	gi.sound(ent, CHAN_RELIABLE + CHAN_WEAPON,
			gi.soundindex("weapons/grapple/grfire.wav"), volume, ATTN_NORM, 0);
	FireGrapple(ent, start, forward, damage, GRAPPLE_SPEED * 2, effect);
	PlayerNoise(ent, start, PNOISE_WEAPON);
}

void
Weapon_Grapple_Fire(edict_t *ent)
{
	int damage;

	damage = 10;
	GrappleFire(ent, vec3_origin, damage, EF_GRENADE/*0*/);
	ent->client->ps.gunframe++;
}

void
Weapon_Grapple(edict_t *ent)
{
	static int pause_frames[] = {10, 18, 27, 0};
	static int fire_frames[] = {6, 0};
	int prevstate;

	/* if the the attack button is still down, stay in the firing frame */
	if ((ent->client->buttons & BUTTON_ATTACK) &&
		(ent->client->weaponstate == WEAPON_FIRING) &&
		ent->client->grapple)
	{
		ent->client->ps.gunframe = 9;
	}

	if (!(ent->client->buttons & BUTTON_ATTACK) &&
		ent->client->grapple)
	{
		ResetGrapple(ent->client->grapple);

		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
		}
	}

	if (ent->client->newweapon &&
		(ent->client->grapplestate > GRAPPLE_STATE_FLY) &&
		(ent->client->weaponstate == WEAPON_FIRING))
	{
		/* he wants to change weapons while grappled */
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = 32;
	}

	prevstate = ent->client->weaponstate;
	Weapon_Generic(ent, 5, 9, 31, 36, pause_frames, fire_frames,
			Weapon_Grapple_Fire);

	/* if we just switched back to grapple, immediately go to fire frame */
	if ((prevstate == WEAPON_ACTIVATING) &&
		(ent->client->weaponstate == WEAPON_READY) &&
		(ent->client->grapplestate > GRAPPLE_STATE_FLY))
	{
		if (!(ent->client->buttons & BUTTON_ATTACK))
		{
			ent->client->ps.gunframe = 9;
		}
		else
		{
			ent->client->ps.gunframe = 5;
		}

		ent->client->weaponstate = WEAPON_FIRING;
	}
}

/*
 * =======================================================================
 *
 *	Ion Ripper
 *
 * =======================================================================
 */
void
ionripper_sparks(edict_t *self)
{
	if (!self)
	{
		return;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WELDING_SPARKS);
	gi.WriteByte(0);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(vec3_origin);
	gi.WriteByte(0xe4 + (rand() & 3));
	gi.multicast(self->s.origin, MULTICAST_PVS);

	G_FreeEdict(self);
}

void
ionripper_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!self || !other || !plane || !surf)
	{
		return;
	}

	if (other == self->owner)
	{
		return;
	}

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->owner->client)
	{
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);
	}

	if (other->takedamage)
	{
		T_Damage(other, self, self->owner, self->velocity, self->s.origin,
				plane->normal, self->dmg, 1, DAMAGE_ENERGY, MOD_RIPPER);
	}
	else
	{
		return;
	}

	G_FreeEdict(self);
}

void
fire_ionripper(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int effect)
{
	edict_t *ion;
	trace_t tr;

	if (!self)
	{
		return;
	}

	VectorNormalize(dir);

	ion = G_Spawn();
	VectorCopy(start, ion->s.origin);
	VectorCopy(start, ion->s.old_origin);
	vectoangles(dir, ion->s.angles);
	VectorScale(dir, speed, ion->velocity);

	ion->movetype = MOVETYPE_WALLBOUNCE;
	ion->clipmask = MASK_SHOT;
	ion->solid = SOLID_BBOX;
	ion->s.effects |= effect;

	ion->s.renderfx |= RF_FULLBRIGHT;

	VectorClear(ion->mins);
	VectorClear(ion->maxs);
	ion->s.modelindex = gi.modelindex("models/objects/boomrang/tris.md2");
	ion->s.sound = gi.soundindex("misc/lasfly.wav");
	ion->owner = self;
	ion->touch = ionripper_touch;
	ion->nextthink = level.time + 3;
	ion->think = ionripper_sparks;
	ion->dmg = damage;
	ion->dmg_radius = 100;
	gi.linkentity(ion);

	if (self->client)
	{
		check_dodge(self, ion->s.origin, dir, speed);
	}

	tr = gi.trace(self->s.origin, NULL, NULL, ion->s.origin, ion, MASK_SHOT);

	if (tr.fraction < 1.0)
	{
		VectorMA(ion->s.origin, -10, dir, ion->s.origin);
		ion->touch(ion, tr.ent, NULL, NULL);
	}
}

/* RipperGun client code */
void
weapon_ionripper_fire(edict_t *ent)
{
	vec3_t start;
	vec3_t forward, right;
	vec3_t offset;
	vec3_t tempang;
	int damage;

  	if (!ent)
	{
		return;
	}

	if (deathmatch->value)
	{
		/* tone down for deathmatch */
		damage = 30;
	}
	else
	{
		damage = 50;
	}

	if (is_quad)
	{
		damage *= 4;
	}

	VectorCopy(ent->client->v_angle, tempang);
	tempang[YAW] += crandom();

	AngleVectors(tempang, forward, right, NULL);

	VectorScale(forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 16, 7, ent->viewheight - 8);

	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_ionripper(ent, start, forward, damage, 500, EF_IONRIPPER);

	/* send muzzle flash */
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_IONRIPPER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
	{
		ent->client->pers.inventory[ent->client->ammo_index] -=
			ent->client->pers.weapon->quantity;
	}

	if (ent->client->pers.inventory[ent->client->ammo_index] < 0)
	{
		ent->client->pers.inventory[ent->client->ammo_index] = 0;
	}
}

void
Weapon_Ionripper(edict_t *ent)
{
	static int pause_frames[] = {36, 0};
	static int fire_frames[] = {5, 0};

  	if (!ent)
	{
		return;
	}

	Weapon_Generic(ent, 4, 6, 36, 39, pause_frames,
			fire_frames, weapon_ionripper_fire);

/*	if (is_quadfire)
	{
		Weapon_Generic(ent, 4, 6, 36, 39, pause_frames,
				fire_frames, weapon_ionripper_fire);
	} */
}

/*
 * =======================================================================
 *
 *	Heat Laser
 *
 * =======================================================================
 */
void
fire_beams(edict_t *self, vec3_t start, vec3_t aimdir, vec3_t offset,
		int damage, int kick, int te_beam, int te_impact, int mod)
{
	trace_t tr;
	vec3_t dir;
	vec3_t forward, right, up;
	vec3_t end;
	vec3_t water_start, endpoint;
	qboolean water = false, underwater = false;
	int content_mask = MASK_SHOT | MASK_WATER;
	vec3_t beam_endpt;

	if (!self)
	{
		return;
	}

	vectoangles2(aimdir, dir);
	AngleVectors(dir, forward, right, up);

	VectorMA(start, 8192, forward, end);

	if (gi.pointcontents(start) & MASK_WATER)
	{
		underwater = true;
		VectorCopy(start, water_start);
		content_mask &= ~MASK_WATER;
	}

	tr = gi.trace(start, NULL, NULL, end, self, content_mask);

	/* see if we hit water */
	if (tr.contents & MASK_WATER)
	{
		water = true;
		VectorCopy(tr.endpos, water_start);

		if (!VectorCompare(start, tr.endpos))
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_HEATBEAM_SPARKS);
			gi.WritePosition(water_start);
			gi.WriteDir(tr.plane.normal);
			gi.multicast(tr.endpos, MULTICAST_PVS);
		}

		/* re-trace ignoring water this time */
		tr = gi.trace(water_start, NULL, NULL, end, self, MASK_SHOT);
	}

	VectorCopy(tr.endpos, endpoint);

	/* halve the damage if target underwater */
	if (water)
	{
		damage = damage / 2;
	}

	/* send gun puff / flash */
	if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0)
		{
			if (tr.ent->takedamage)
			{
				T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal,
						damage, kick, DAMAGE_ENERGY, mod);
			}
			else
			{
				if ((!water) && (strncmp(tr.surface->name, "sky", 3)))
				{
					/* This is the truncated steam entry - uses 1+1+2 extra bytes of data */
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(TE_HEATBEAM_STEAM);
					gi.WritePosition(tr.endpos);
					gi.WriteDir(tr.plane.normal);
					gi.multicast(tr.endpos, MULTICAST_PVS);

					if (self->client)
					{
						PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
					}
				}
			}
		}
	}

	/* if went through water, determine where the end and make a bubble trail */
	if ((water) || (underwater))
	{
		vec3_t pos;

		VectorSubtract(tr.endpos, water_start, dir);
		VectorNormalize(dir);
		VectorMA(tr.endpos, -2, dir, pos);

		if (gi.pointcontents(pos) & MASK_WATER)
		{
			VectorCopy(pos, tr.endpos);
		}
		else
		{
			tr = gi.trace(pos, NULL, NULL, water_start, tr.ent, MASK_WATER);
		}

		VectorAdd(water_start, tr.endpos, pos);
		VectorScale(pos, 0.5, pos);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BUBBLETRAIL2);
		gi.WritePosition(water_start);
		gi.WritePosition(tr.endpos);
		gi.multicast(pos, MULTICAST_PVS);
	}

	// ace - add "prediction" for beams? (FIXME: is move to engine-side possible?)
	VectorMA (start, FRAMETIME*FRAMETIME, self->velocity, start);

	if ((!underwater) && (!water))
	{
		VectorCopy(tr.endpos, beam_endpt);
	}
	else
	{
		VectorCopy(endpoint, beam_endpt);
	}
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(te_beam);
	if (te_beam == TE_LIGHTNING)
	{
		gi.WriteShort(tr.ent - g_edicts);
		gi.WriteShort(self - g_edicts);
		gi.WritePosition(beam_endpt);
		gi.WritePosition(start);
		gi.multicast(start, MULTICAST_PVS);
	}
	else
	{
		gi.WriteShort(self - g_edicts);
		gi.WritePosition(start);
		gi.WritePosition(beam_endpt);
		gi.multicast(self->s.origin, MULTICAST_ALL);
	}
}

void
fire_heat(edict_t *self, vec3_t start, vec3_t aimdir, vec3_t offset,
		int damage, int kick, qboolean monster)
{
	if (!self)
	{
		return;
	}

	if (monster)
	{
		fire_beams(self, start, aimdir, offset, damage, kick,
				TE_MONSTER_HEATBEAM, TE_HEATBEAM_SPARKS, MOD_HEATBEAM);
	}
	else
	{
		// ace - for the lulz, throwback lightning gun when damage is over normal
		int effect, effect2;

		effect = TE_HEATBEAM;
		effect2 = TE_HEATBEAM_SPARKS;

//		if (damage > HEATBEAM_SP_DMG)
		if (is_quad || HasStrength(self))
		{
			effect = TE_LIGHTNING;
			effect2 = TE_ELECTRIC_SPARKS;
		}

		fire_beams(self, start, aimdir, offset, damage,
				kick, effect/*TE_HEATBEAM*/, effect2 /*TE_HEATBEAM_SPARKS*/, MOD_HEATBEAM);
	}
}


void
Heatbeam_Fire(edict_t *ent)
{
	vec3_t start;
	vec3_t forward, right, up;
	vec3_t offset;
	int damage;
	int kick;

	if (!ent)
	{
		return;
	}

	if (deathmatch->value)
	{
		damage = HEATBEAM_DM_DMG;
	}
	else
	{
		damage = HEATBEAM_SP_DMG;
	}

	if (deathmatch->value)  /* really knock 'em around in deathmatch */
	{
		kick = 75;
	}
	else
	{
		kick = 30;
	}

	ent->client->ps.gunframe++;
	ent->client->ps.gunindex = gi.modelindex("models/weapons/v_beamer2/tris.md2");

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	VectorClear(ent->client->kick_origin);
	VectorClear(ent->client->kick_angles);

	/* get start / end positions */
	AngleVectors(ent->client->v_angle, forward, right, up);

	/* This offset is the "view" offset for the beam start (used by trace) */
	VectorSet(offset, 7, 2, ent->viewheight - 3);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	/* This offset is the entity offset */
	VectorSet(offset, 2, 7, -3);

	fire_heat(ent, start, forward, offset, damage, kick, false);

	/* send muzzle flash */
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_HEATBEAM | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
	{
		ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;
	}

	ent->client->anim_priority = ANIM_ATTACK;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - 1;
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - 1;
		ent->client->anim_end = FRAME_attack8;
	}
}

void
Weapon_Heatbeam(edict_t *ent)
{
	static int pause_frames[] = {35, 0};
	static int fire_frames[] = {9, 10, 11, 12, 0};

	if (!ent)
	{
		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		ent->client->weapon_sound = gi.soundindex("weapons/bfg__l1a.wav");

		if ((ent->client->pers.inventory[ent->client->ammo_index] >= 2) &&
			((ent->client->buttons) & BUTTON_ATTACK))
		{
			if (ent->client->ps.gunframe >= 13)
			{
				ent->client->ps.gunframe = 9;
				ent->client->ps.gunindex = gi.modelindex("models/weapons/v_beamer2/tris.md2");
			}
			else
			{
				ent->client->ps.gunindex = gi.modelindex("models/weapons/v_beamer2/tris.md2");
			}
		}
		else
		{
			ent->client->ps.gunframe = 13;
			ent->client->ps.gunindex = gi.modelindex("models/weapons/v_beamer/tris.md2");
		}
	}
	else
	{
		ent->client->ps.gunindex = gi.modelindex("models/weapons/v_beamer/tris.md2");
		ent->client->weapon_sound = 0;
	}

	Weapon_Generic(ent, 8, 12, 39, 44, pause_frames, fire_frames, Heatbeam_Fire);
}

/*
 * =======================================================================
 *
 *	Heat Seeker
 *
 * =======================================================================
 */
void
seeker_think(edict_t *self)
{
	edict_t *target = NULL;
	edict_t *aquire = NULL;
	vec3_t vec;
	int len;
	int oldlen = 0;

	if (!self)
	{
		return;
	}

	// ace - added a "timeout"
	if (level.time > self->timeout)
	{
		G_FreeEdict(self);
		return;
	}

	VectorClear(vec);

	/* aquire new target */
//	target = findradius(target, self->s.origin, 1024);
//	while (target)
	while ((target = findradius(target, self->s.origin, 1024)) != NULL)
	{
		if (self->owner == target)
		{
			continue;
		}

		if (!(target->svflags & SVF_MONSTER) && !(target->client))
		{
			continue;
		}

		if (target->health <= 0 || target->deadflag != DEAD_NO)
		{
			continue;
		}

		if (!visible(self, target))
		{
			continue;
		}

		if (!infront(self, target))
		{
			continue;
		}

		VectorSubtract(self->s.origin, target->s.origin, vec);
		len = VectorLength(vec);

		if ((aquire == NULL) || (len < oldlen))
		{
			aquire = target;
			self->target_ent = aquire;
			oldlen = len;
		}
	}

	if (aquire != NULL)
	{
		VectorSubtract(aquire->s.origin, self->s.origin, vec);
		vectoangles(vec, self->s.angles);
		VectorNormalize(vec);
		VectorCopy(vec, self->movedir);
		VectorScale(vec, self->speed/2, self->velocity); // set self->speed instead of defaulting to 500
	}

	self->nextthink = level.time + FRAMETIME + random();
}

void rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

void
fire_seeker(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed,
		float damage_radius, int radius_damage)
{
	edict_t *heat;

	if (!self)
	{
		return;
	}

	heat = G_Spawn();
	VectorCopy(start, heat->s.origin);
	VectorCopy(dir, heat->movedir);
	vectoangles(dir, heat->s.angles);
	VectorScale(dir, speed, heat->velocity);
	heat->movetype = MOVETYPE_FLYMISSILE;
	heat->clipmask = MASK_SHOT;
	heat->solid = SOLID_BBOX;
	heat->s.effects |= EF_ROCKET;
	VectorClear(heat->mins);
	VectorClear(heat->maxs);
	heat->s.modelindex = gi.modelindex("models/objects/rocket/tris.md2");
	heat->owner = self;
	heat->touch = rocket_touch;
	
	heat->speed = speed; // ace - keep track of base speed

	heat->timeout = level.time + 8000 / speed; // put this variable to use

	heat->nextthink = level.time + 0.1;
	heat->think = seeker_think;

	heat->dmg = damage;
	heat->radius_dmg = radius_damage;
	heat->dmg_radius = damage_radius;
	heat->s.sound = gi.soundindex("weapons/rockfly.wav");

	if (self->client)
	{
		check_dodge(self, heat->s.origin, dir, speed);
	}

	gi.linkentity(heat);
}

/*
 * =======================================================================
 *
 *	Phalanx
 *
 * =======================================================================
 */
void
plasma_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t origin;

	if (!ent || !other || !plane || !surf)
	{
		return;
	}
	
	if (other == ent->owner)
	{
		return;
	}

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	if (ent->owner->client)
	{
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);
	}

	/* calculate position for the explosion entity */
	VectorMA(ent->s.origin, -0.02, ent->velocity, origin);

	if (other->takedamage)
	{
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin,
				plane->normal, ent->dmg, 0, 0, MOD_PHALANX);
	}

	T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other,
			ent->dmg_radius, MOD_PHALANX);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_PLASMA_EXPLOSION);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	G_FreeEdict(ent);
}

// ace - modified to use the heatseaker code from above
void
fire_plasma(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, float damage_radius, int radius_damage)
{
	edict_t *plasma;

	if (!self)
	{
		return;
	}

	plasma = G_Spawn();
	VectorCopy(start, plasma->s.origin);
	VectorCopy(dir, plasma->movedir);
	vectoangles(dir, plasma->s.angles);
	VectorScale(dir, speed, plasma->velocity);
	plasma->movetype = MOVETYPE_FLYMISSILE;
	plasma->clipmask = MASK_SHOT;
	plasma->solid = SOLID_BBOX;

	VectorClear(plasma->mins);
	VectorClear(plasma->maxs);

	plasma->owner = self;
	plasma->touch = plasma_touch;
	plasma->dmg = damage;
	plasma->radius_dmg = radius_damage;
	plasma->dmg_radius = damage_radius;
	plasma->s.sound = gi.soundindex("weapons/rockfly.wav");

	// ace - modded to use heatseeker code from above
	plasma->timeout = level.time + 8000 / speed; // put this variable to use
	plasma->nextthink = level.time + 0.1 + random();
	plasma->think = seeker_think;
	plasma->speed = speed; // ace - keep track of base speed

	plasma->s.modelindex = gi.modelindex("sprites/s_photon.sp2");
	plasma->s.effects |= EF_PLASMA | EF_ANIM_ALLFAST;

	if (self->client)
	{
		check_dodge(self, plasma->s.origin, dir, speed);
	}

	gi.linkentity(plasma);
}

/*	Phalanx client code */

void
weapon_phalanx_fire(edict_t *ent)
{
	vec3_t start;
	vec3_t forward, right, up;
	vec3_t offset;
	vec3_t v;
	int damage;
	float damage_radius;
	int radius_damage;

  	if (!ent)
	{
		return;
	}

	damage = 70 + (int)(random() * 10.0);
	radius_damage = 120;
	damage_radius = 120;

	if (is_quad)
	{
		damage *= 4;
		radius_damage *= 4;
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	if (ent->client->ps.gunframe == 8)
	{
		v[PITCH] = ent->client->v_angle[PITCH];
		v[YAW] = ent->client->v_angle[YAW] - 1.5;
		v[ROLL] = ent->client->v_angle[ROLL];
		AngleVectors(v, forward, right, up);

		radius_damage = 30;
		damage_radius = 120;

		fire_plasma(ent, start, forward, damage, 725,
				damage_radius, radius_damage);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
		{
			ent->client->pers.inventory[ent->client->ammo_index]--;
		}
	}
	else
	{
		v[PITCH] = ent->client->v_angle[PITCH];
		v[YAW] = ent->client->v_angle[YAW] + 1.5;
		v[ROLL] = ent->client->v_angle[ROLL];
		AngleVectors(v, forward, right, up);
		fire_plasma(ent, start, forward, damage, 725,
				damage_radius, radius_damage);

		/* send muzzle flash */
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_PHALANX | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, start, PNOISE_WEAPON);
	}

	ent->client->ps.gunframe++;
}

void
Weapon_Phalanx(edict_t *ent)
{
	static int pause_frames[] = {29, 42, 55, 0};
	static int fire_frames[] = {7, 8, 0};

  	if (!ent)
	{
		return;
	}

	Weapon_Generic(ent, 5, 20, 58, 63, pause_frames,
			fire_frames, weapon_phalanx_fire);

/*	if (is_quadfire)
	{
		Weapon_Generic(ent, 5, 20, 58, 63, pause_frames,
				fire_frames, weapon_phalanx_fire);
	} */
}

/*
 * =======================================================================
 *
 *	Energy Trap
 *
 * =======================================================================
 */
void SP_item_foodcube(edict_t *self);

void
Trap_Think(edict_t *ent)
{
	edict_t *target = NULL;
	edict_t *best = NULL;
	vec3_t vec;
	int len, i;
	int oldlen = 8000;
	vec3_t forward, right, up;

	if (!ent)
	{
		return;
	}

	if (ent->timestamp < level.time)
	{
		BecomeExplosion1(ent);
		return;
	}

	ent->nextthink = level.time + 0.1;

	if (!ent->groundentity)
	{
		return;
	}

	/* ok lets do the blood effect */
	if (ent->s.frame > 4)
	{
		if (ent->s.frame == 5)
		{
			if (ent->wait == 64)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/trap/trapdown.wav"),
					   	1, ATTN_IDLE, 0);
			}

			ent->wait -= 2;
			ent->delay += level.time;

			for (i = 0; i < 3; i++)
			{
				best = G_Spawn();

				if (ent->mass > 200)
				{
					best->s.modelindex = gi.modelindex("models/objects/gibs/chest/tris.md2");
					best->s.effects |= TE_BLOOD;
				}
				else
				{
					best->s.modelindex = gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
					best->s.effects |= TE_BLOOD;
				}

				AngleVectors(ent->s.angles, forward, right, up);

				RotatePointAroundVector(vec, up, right, ((360.0 / 3) * i) + ent->delay);
				VectorMA(vec, ent->wait / 2, vec, vec);
				VectorAdd(vec, ent->s.origin, vec);
				VectorAdd(vec, forward, best->s.origin);

				best->s.origin[2] = ent->s.origin[2] + ent->wait;

				VectorCopy(ent->s.angles, best->s.angles);

				best->solid = SOLID_NOT;
				best->s.effects |= EF_GIB;
				best->takedamage = DAMAGE_YES;

				best->movetype = MOVETYPE_TOSS;
				best->svflags |= SVF_MONSTER;
				best->deadflag = DEAD_DEAD;

				VectorClear(best->mins);
				VectorClear(best->maxs);

				best->watertype = gi.pointcontents(best->s.origin);

				if (best->watertype & MASK_WATER)
				{
					best->waterlevel = 1;
				}

				best->nextthink = level.time + 0.1;
				best->think = G_FreeEdict;
				gi.linkentity(best);
			}

			if (ent->wait < 19)
			{
				ent->s.frame++;
			}

			return;
		}

		ent->s.frame++;

		if (ent->s.frame == 8)
		{
			ent->nextthink = level.time + 1.0;
			ent->think = G_FreeEdict;

			best = G_Spawn();
			SP_item_foodcube(best);
			VectorCopy(ent->s.origin, best->s.origin);
			best->s.origin[2] += 16;
			best->velocity[2] = 400;
			best->count = ent->mass;
			gi.linkentity(best);
			return;
		}

		return;
	}

	ent->s.effects &= ~EF_TRAP;

	if (ent->s.frame >= 4)
	{
		ent->s.effects |= EF_TRAP;
		VectorClear(ent->mins);
		VectorClear(ent->maxs);
	}

	if (ent->s.frame < 4)
	{
		ent->s.frame++;
	}

	while ((target = findradius(target, ent->s.origin, 256)) != NULL)
	{
		if (target == ent)
		{
			continue;
		}

		if (!(target->svflags & SVF_MONSTER) && !target->client)
		{
			continue;
		}

		if (target->health <= 0)
		{
			continue;
		}

		if (!visible(ent, target))
		{
			continue;
		}

		if (!best)
		{
			best = target;
			continue;
		}

		VectorSubtract(ent->s.origin, target->s.origin, vec);
		len = VectorLength(vec);

		if (len < oldlen)
		{
			oldlen = len;
			best = target;
		}
	}

	/* pull the enemy in */
	if (best)
	{
		vec3_t forward;

		if (best->groundentity)
		{
			best->s.origin[2] += 1;
			best->groundentity = NULL;
		}

		VectorSubtract(ent->s.origin, best->s.origin, vec);
		len = VectorLength(vec);

		if (best->client)
		{
			VectorNormalize(vec);
			VectorMA(best->velocity, 250, vec, best->velocity);
		}
		else
		{
			best->ideal_yaw = vectoyaw(vec);
			M_ChangeYaw(best);
			AngleVectors(best->s.angles, forward, NULL, NULL);
			VectorScale(forward, 256, best->velocity);
		}

		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/trap/trapsuck.wav"), 1, ATTN_IDLE, 0);

		if (len < 32)
		{
			if (best->mass < 400)
			{
				T_Damage(best, ent, ent->owner, vec3_origin, best->s.origin,
						vec3_origin, 100000, 1, 0, MOD_TRAP);
				ent->enemy = best;
				ent->wait = 64;
				VectorCopy(ent->s.origin, ent->s.old_origin);
				ent->timestamp = level.time + 30;

				if (deathmatch->value)
				{
					ent->mass = best->mass / 4;
				}
				else
				{
					ent->mass = best->mass / 10;
				}

				/* ok spawn the food cube */
				ent->s.frame = 5;
			}
			else
			{
				BecomeExplosion1(ent);
				return;
			}
		}
	}
}

void
fire_trap(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed, float timer, float damage_radius, qboolean held)
{
	edict_t *trap;
	vec3_t dir;
	vec3_t forward, right, up;

	if (!self)
	{
		return;
	}

	vectoangles(aimdir, dir);
	AngleVectors(dir, forward, right, up);

	trap = G_Spawn();
	VectorCopy(start, trap->s.origin);
	VectorScale(aimdir, speed, trap->velocity);
	VectorMA(trap->velocity, 200 + crandom() * 10.0, up, trap->velocity);
	VectorMA(trap->velocity, crandom() * 10.0, right, trap->velocity);
	VectorSet(trap->avelocity, 0, 300, 0);
	trap->movetype = MOVETYPE_BOUNCE;
	trap->clipmask = MASK_SHOT;
	trap->solid = SOLID_BBOX;
	VectorSet(trap->mins, -4, -4, 0);
	VectorSet(trap->maxs, 4, 4, 8);
	trap->s.modelindex = gi.modelindex("models/weapons/z_trap/tris.md2");
	trap->owner = self;
	trap->nextthink = level.time + 1.0;
	trap->think = Trap_Think;
	trap->dmg = damage;
	trap->dmg_radius = damage_radius;
	trap->classname = "htrap";
	trap->s.sound = gi.soundindex("weapons/trap/traploop.wav");

	if (held)
	{
		trap->spawnflags = 3;
	}
	else
	{
		trap->spawnflags = 1;
	}

	if (timer <= 0.0)
	{
		Grenade_Explode(trap);
	}
	else
	{
		gi.linkentity(trap);
	}

	trap->timestamp = level.time + 30;
}

/* TRAP client code */

void
weapon_trap_fire(edict_t *ent, qboolean held)
{
	vec3_t offset;
	vec3_t forward, right;
	vec3_t start;
	int damage = 125;
	float timer;
	int speed;
	float radius;

  	if (!ent)
	{
		return;
	}

	radius = damage + 40;

	if (is_quad)
	{
		damage *= 4;
	}

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	timer = ent->client->grenade_time - level.time;
	speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer) *
	   	((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER);
	fire_trap(ent, start, forward, damage, speed, timer, radius, held);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
	{
		ent->client->pers.inventory[ent->client->ammo_index]--;
	}
	ent->client->grenade_time = level.time + 1.0;
}

void
Weapon_Trap(edict_t *ent)
{
  	if (!ent)
	{
		return;
	}

	if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY))
	{
		ChangeWeapon(ent);
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		ent->client->weaponstate = WEAPON_READY;
		ent->client->ps.gunframe = 16;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons |
			  ent->client->buttons) & BUTTON_ATTACK))
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;

			if (ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->ps.gunframe = 1;
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0;
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"),
						   	1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}

				NoAmmoWeaponChange(ent);
			}

			return;
		}

		if ((ent->client->ps.gunframe == 29) ||
			(ent->client->ps.gunframe == 34) ||
			(ent->client->ps.gunframe == 39) ||
			(ent->client->ps.gunframe == 48))
		{
			if (rand() & 15)
			{
				return;
			}
		}

		if (++ent->client->ps.gunframe > 48)
		{
			ent->client->ps.gunframe = 16;
		}

		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		if (ent->client->ps.gunframe == 5)
		{
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/trap/trapcock.wav"),
				   	1, ATTN_NORM, 0);
		}

		if (ent->client->ps.gunframe == 11)
		{
			if (!ent->client->grenade_time)
			{
				ent->client->grenade_time = level.time + GRENADE_TIMER + 0.2;
				ent->client->weapon_sound = gi.soundindex("weapons/trap/traploop.wav");
			}

			/* they waited too long, detonate it in their hand */
			if (!ent->client->grenade_blew_up &&
				(level.time >= ent->client->grenade_time))
			{
				ent->client->weapon_sound = 0;
				weapon_trap_fire(ent, true);
				ent->client->grenade_blew_up = true;
			}

			if (ent->client->buttons & BUTTON_ATTACK)
			{
				return;
			}

			if (ent->client->grenade_blew_up)
			{
				if (level.time >= ent->client->grenade_time)
				{
					ent->client->ps.gunframe = 15;
					ent->client->grenade_blew_up = false;
				}
				else
				{
					return;
				}
			}
		}

		if (ent->client->ps.gunframe == 12)
		{
			ent->client->weapon_sound = 0;
			weapon_trap_fire(ent, false);

			if (ent->client->pers.inventory[ent->client->ammo_index] == 0)
			{
				NoAmmoWeaponChange(ent);
			}
		}

		if ((ent->client->ps.gunframe == 15) &&
			(level.time < ent->client->grenade_time))
		{
			return;
		}

		ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == 16)
		{
			ent->client->grenade_time = 0;
			ent->client->weaponstate = WEAPON_READY;
		}
	}
}

/*
 * =======================================================================
 *
 *	ETF Rifle
 *
 * =======================================================================
 */
void
flechette_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t dir;

	if (!self || !other || !plane || !surf)
	{
		return;
	}

	if (other == self->owner)
	{
		return;
	}

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->client)
	{
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);
	}

	if (other->takedamage)
	{
		T_Damage(other, self, self->owner, self->velocity, self->s.origin,
				plane->normal, self->dmg, self->dmg_radius, DAMAGE_NO_REG_ARMOR,
				MOD_ETF_RIFLE);
	}
	else
	{
		if (!plane)
		{
			VectorClear(dir);
		}
		else
		{
			VectorScale(plane->normal, 256, dir);
		}

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_FLECHETTE);
		gi.WritePosition(self->s.origin);
		gi.WriteDir(dir);
		gi.multicast(self->s.origin, MULTICAST_PVS);
	}

	G_FreeEdict(self);
}

void
fire_flechette(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int kick)
{
	edict_t *flechette;

	if (!self)
	{
		return;
	}

	VectorNormalize(dir);

	flechette = G_Spawn();
	VectorCopy(start, flechette->s.origin);
	VectorCopy(start, flechette->s.old_origin);
	vectoangles2(dir, flechette->s.angles);

	VectorScale(dir, speed, flechette->velocity);
	flechette->movetype = MOVETYPE_FLYMISSILE;
	flechette->clipmask = MASK_SHOT;
	flechette->solid = SOLID_BBOX;
	flechette->s.renderfx = RF_FULLBRIGHT;
	VectorClear(flechette->mins);
	VectorClear(flechette->maxs);

	flechette->s.modelindex = gi.modelindex("models/proj/flechette/tris.md2");

	flechette->owner = self;
	flechette->touch = flechette_touch;
	flechette->nextthink = level.time + 8000 / speed;
	flechette->think = G_FreeEdict;
	flechette->dmg = damage;
	flechette->dmg_radius = kick;

	gi.linkentity(flechette);

	if (self->client)
	{
		check_dodge(self, flechette->s.origin, dir, speed);
	}
}

/*
 * ======================================================================
 *
 * ETF RIFLE
 *
 * ======================================================================
 */
void
weapon_etf_rifle_fire(edict_t *ent)
{
	vec3_t forward, right, up;
	vec3_t start, tempPt;
	int damage;
	int kick = 3;
	int i;
	vec3_t offset;

	if (!ent)
	{
		return;
	}

	if (deathmatch->value)
	{
		damage = 10;
	}
	else
	{
		damage = 10;
	}

	if (ent->client->pers.inventory[ent->client->ammo_index] < ent->client->pers.weapon->quantity)
	{
		VectorClear(ent->client->kick_origin);
		VectorClear(ent->client->kick_angles);
		ent->client->ps.gunframe = 8;

		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}

		NoAmmoWeaponChange(ent);
		return;
	}

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	for (i = 0; i < 3; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.85;
		ent->client->kick_angles[i] = crandom() * 0.85;
	}

	/* get start / end positions */
	AngleVectors(ent->client->v_angle, forward, right, up);

	if (ent->client->ps.gunframe == 6) /* right barrel */
	{
		VectorSet(offset, 15, 8, -8);
	}
	else /* left barrel */
	{
		VectorSet(offset, 15, 6, -8);
	}

	VectorCopy(ent->s.origin, tempPt);
	tempPt[2] += ent->viewheight;
	P_ProjectSource2(ent->client, tempPt, offset, forward, right, up, start);
	fire_flechette(ent, start, forward, damage, 750, kick);

	/* send muzzle flash */
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_ETF_RIFLE);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->ps.gunframe++;

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
	{
		ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;
	}

	ent->client->anim_priority = ANIM_ATTACK;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - 1;
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - 1;
		ent->client->anim_end = FRAME_attack8;
	}
}

void
Weapon_ETF_Rifle(edict_t *ent)
{
	static int pause_frames[] = {18, 28, 0};
	static int fire_frames[] = {6, 7, 0};

	if (!ent)
	{
		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		if (ent->client->pers.inventory[ent->client->ammo_index] <= 0)
		{
			ent->client->ps.gunframe = 8;
		}
	}

	Weapon_Generic(ent, 4, 7, 37, 41, pause_frames,
			fire_frames, weapon_etf_rifle_fire);

	if ((ent->client->ps.gunframe == 8) &&
		(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe = 6;
	}
}

/*
 * ======================================================================
 *
 * GENERIC GRENADE (Rogue)
 *
 * ======================================================================
 */

void
weapon_grenade_fire2 (edict_t *ent, qboolean held)
{
	vec3_t offset;
	vec3_t forward, right, up;
	vec3_t start;
	int damage = 125;
	float timer;
	int speed;
	float radius;
	int damage_multiplier;

	if (!ent)
	{
		return;
	}

	radius = damage + 40;

	if (is_quad)
	{
		damage *= 4;
		damage_multiplier = 4;
	}
	else
		damage_multiplier = 1;

	AngleVectors(ent->client->v_angle, forward, right, up);

	if (ent->client->pers.weapon->tag == AMMO_TESLA)
	{
		VectorSet(offset, 0, -4, ent->viewheight - 22);
	}
	else
	{
		VectorSet(offset, 2, 6, ent->viewheight - 14);
	}

	P_ProjectSource2(ent->client, ent->s.origin, offset,
			forward, right, up, start);

	timer = ent->client->grenade_time - level.time;
	speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER);

	if (speed > GRENADE_MAXSPEED)
	{
		speed = GRENADE_MAXSPEED;
	}

	switch (ent->client->pers.weapon->tag)
	{
		case AMMO_GRENADES:
			fire_grenade2(ent, start, forward, damage, speed,
				timer, radius, held);
			break;
		case AMMO_TESLA:
			fire_tesla(ent, start, forward, damage_multiplier, speed);
			break;
		default:
			fire_prox(ent, start, forward, damage_multiplier, speed);
			break;
	}

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
	{
		ent->client->pers.inventory[ent->client->ammo_index]--;
	}

	ent->client->grenade_time = level.time + 1.0;

	if (ent->deadflag || (ent->s.modelindex != 255)) /* VWep animations screw up corpses */
	{
		return;
	}

	if (ent->health <= 0)
	{
		return;
	}

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = FRAME_crattak1 - 1;
		ent->client->anim_end = FRAME_crattak3;
	}
	else
	{
		ent->client->anim_priority = ANIM_REVERSE;
		ent->s.frame = FRAME_wave08;
		ent->client->anim_end = FRAME_wave01;
	}
}


void
Throw_Generic(edict_t *ent, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_THROW_SOUND,
		int FRAME_THROW_HOLD, int FRAME_THROW_FIRE, int *pause_frames, int EXPLODE,
		void (*fire)(edict_t *ent, qboolean held))
{
	int n;

	if (!ent || !pause_frames || !fire)
	{
		return;
	}

	if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY))
	{
		ChangeWeapon(ent);
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		ent->client->weaponstate = WEAPON_READY;
		ent->client->ps.gunframe = FRAME_IDLE_FIRST;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK))
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;

			if (ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->ps.gunframe = 1;
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0;
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}

				NoAmmoWeaponChange(ent);
			}

			return;
		}

		if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
		{
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		if (pause_frames)
		{
			for (n = 0; pause_frames[n]; n++)
			{
				if (ent->client->ps.gunframe == pause_frames[n])
				{
					if (rand() & 15)
					{
						return;
					}
				}
			}
		}

		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		if (ent->client->ps.gunframe == FRAME_THROW_SOUND)
		{
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);
		}

		if (ent->client->ps.gunframe == FRAME_THROW_HOLD)
		{
			if (!ent->client->grenade_time)
			{
				ent->client->grenade_time = level.time + GRENADE_TIMER + 0.2;

				switch (ent->client->pers.weapon->tag)
				{
					case AMMO_GRENADES:
						ent->client->weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
						break;
				}
			}

			/* they waited too long, detonate it in their hand */
			if (EXPLODE && !ent->client->grenade_blew_up &&
				(level.time >= ent->client->grenade_time))
			{
				ent->client->weapon_sound = 0;
				fire(ent, true);
				ent->client->grenade_blew_up = true;
			}

			if (ent->client->buttons & BUTTON_ATTACK)
			{
				return;
			}

			if (ent->client->grenade_blew_up)
			{
				if (level.time >= ent->client->grenade_time)
				{
					ent->client->ps.gunframe = FRAME_FIRE_LAST;
					ent->client->grenade_blew_up = false;
				}
				else
				{
					return;
				}
			}
		}

		if (ent->client->ps.gunframe == FRAME_THROW_FIRE)
		{
			ent->client->weapon_sound = 0;
			fire(ent, true);
		}

		if ((ent->client->ps.gunframe == FRAME_FIRE_LAST) &&
			(level.time < ent->client->grenade_time))
		{
			return;
		}

		ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST)
		{
			ent->client->grenade_time = 0;
			ent->client->weaponstate = WEAPON_READY;
		}
	}
}

/*
 * =======================================================================
 *
 *	Proximity Mines
 *
 * =======================================================================
 */
void
Prox_Explode(edict_t *ent)
{
	vec3_t origin;
	edict_t *owner;

	if (!ent)
	{
		return;
	}

	/* free the trigger field */
	if (ent->teamchain && (ent->teamchain->owner == ent))
	{
		G_FreeEdict(ent->teamchain);
	}

	owner = ent;

	if (ent->teammaster)
	{
		owner = ent->teammaster;
		PlayerNoise(owner, ent->s.origin, PNOISE_IMPACT);
	}

	/* play quad sound if appopriate */
	if (ent->dmg > PROX_DAMAGE)
	{
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
	}

	ent->takedamage = DAMAGE_NO;
	T_RadiusDamage(ent, owner, ent->dmg, ent, PROX_DAMAGE_RADIUS, MOD_PROX);

	VectorMA(ent->s.origin, -0.02, ent->velocity, origin);
	gi.WriteByte(svc_temp_entity);

	if (ent->groundentity)
	{
		gi.WriteByte(TE_GRENADE_EXPLOSION);
	}
	else
	{
		gi.WriteByte(TE_ROCKET_EXPLOSION);
	}

	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	G_FreeEdict(ent);
}

void
prox_die(edict_t *self, edict_t *inflictor, edict_t *attacker /* unused */,
		int damage, vec3_t point)
{
	if (!self || !inflictor)
	{
		return;
	}

	if (strcmp(inflictor->classname, "prox"))
	{
		self->takedamage = DAMAGE_NO;
		Prox_Explode(self);
	}
	else
	{
		self->takedamage = DAMAGE_NO;
		self->think = Prox_Explode;
		self->nextthink = level.time + FRAMETIME;
	}
}

void
Prox_Field_Touch(edict_t *ent, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	edict_t *prox;

	if (!ent || !other)
	{
		return;
	}

	if (!(other->svflags & SVF_MONSTER) && !other->client)
	{
		return;
	}

	/* trigger the prox mine if it's still there, and still mine. */
	prox = ent->owner;

	if (other == prox) /* don't set self off */
	{
		return;
	}

	if (prox->think == Prox_Explode) /* we're set to blow! */
	{
		return;
	}

	if (prox->teamchain == ent)
	{
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/prox/proxwarn.wav"), 1, ATTN_NORM, 0);
		prox->think = Prox_Explode;
		prox->nextthink = level.time + PROX_TIME_DELAY;
		return;
	}

	ent->solid = SOLID_NOT;
	G_FreeEdict(ent);
}

void
prox_seek(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (level.time > ent->wait)
	{
		Prox_Explode(ent);
	}
	else
	{
		ent->s.frame++;

		if (ent->s.frame > 13)
		{
			ent->s.frame = 9;
		}

		ent->think = prox_seek;
		ent->nextthink = level.time + 0.1;
	}
}

void
prox_open(edict_t *ent)
{
	edict_t *search;

	if (!ent)
	{
		return;
	}

	search = NULL;

	if (ent->s.frame == 9) /* end of opening animation */
	{
		/* set the owner to NULL so the owner can shoot it, etc.
		   needs to be done here so the owner doesn't get stuck on
		   it while it's opening if fired at point blank wall */
		ent->s.sound = 0;
		ent->owner = NULL;

		if (ent->teamchain)
		{
			ent->teamchain->touch = Prox_Field_Touch;
		}

		while ((search = findradius(search, ent->s.origin, PROX_DAMAGE_RADIUS + 10)) != NULL)
		{
			if (!search->classname) /* tag token and other weird shit */
			{
				continue;
			}

			/* if it's a monster or player with health > 0
			   or it's a player start point and we can see it
			   blow up */
			if (((((search->svflags & SVF_MONSTER) ||
				   (search->client)) && (search->health > 0)) ||
				   ((deathmatch->value) &&((!strcmp(search->classname, "info_player_deathmatch")) ||
				   (!strcmp(search->classname, "info_player_start")) ||
				   (!strcmp(search->classname, "info_player_coop")) ||
				   (!strcmp(search->classname, "misc_teleporter_dest"))))) &&
				   (visible(search, ent)))
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/prox/proxwarn.wav"), 1, ATTN_NORM, 0);
				Prox_Explode(ent);
				return;
			}
		}

/*		if (strong_mines && (strong_mines->value))
		{
			ent->wait = level.time + PROX_TIME_TO_LIVE;
		}
		else */
		{
			switch (ent->dmg / PROX_DAMAGE)
			{
				case 1:
					ent->wait = level.time + PROX_TIME_TO_LIVE;
					break;
				case 2:
					ent->wait = level.time + 30;
					break;
				case 4:
					ent->wait = level.time + 15;
					break;
				case 8:
					ent->wait = level.time + 10;
					break;
				default:
					ent->wait = level.time + PROX_TIME_TO_LIVE;
					break;
			}
		}

		ent->think = prox_seek;
		ent->nextthink = level.time + 0.2;
	}
	else
	{
		if (ent->s.frame == 0)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/prox/proxopen.wav"), 1, ATTN_NORM, 0);
		}

		ent->s.frame++;
		ent->think = prox_open;
		ent->nextthink = level.time + 0.05;
	}
}

void
prox_land(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t *field;
	vec3_t dir;
	vec3_t forward, right, up;
	int movetype = MOVETYPE_NONE;
	int stick_ok = 0;
	vec3_t land_point;

	if (!ent || !other || !plane || !surf)
	{
		return;
	}

	/* must turn off owner so owner can shoot it and set it off
	   moved to prox_open so owner can get away from it if fired
	   at pointblank range into wall */
	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	if (plane == NULL)
	{
		return;
	}

	VectorMA(ent->s.origin, -10.0, plane->normal, land_point);

	if (gi.pointcontents(land_point) & (CONTENTS_SLIME | CONTENTS_LAVA))
	{
		Prox_Explode(ent);
		return;
	}

	if ((other->svflags & SVF_MONSTER) || other->client ||
		(other->svflags & SVF_DAMAGEABLE))
	{
		if (other != ent->teammaster)
		{
			Prox_Explode(ent);
		}

		return;
	}

 #define STOP_EPSILON 0.1

	else if (other != world)
	{
		/* Here we need to check to see if we can stop on this entity. */
		vec3_t out;
		float backoff, change;
		int i;

		if (!plane->normal) /* this happens if you hit a point object, maybe other cases */
		{
			Prox_Explode(ent);
			return;
		}

		if ((other->movetype == MOVETYPE_PUSH) && (plane->normal[2] > 0.7))
		{
			stick_ok = 1;
		}
		else
		{
			stick_ok = 0;
		}

		backoff = DotProduct(ent->velocity, plane->normal) * 1.5;

		for (i = 0; i < 3; i++)
		{
			change = plane->normal[i] * backoff;
			out[i] = ent->velocity[i] - change;

			if ((out[i] > -STOP_EPSILON) && (out[i] < STOP_EPSILON))
			{
				out[i] = 0;
			}
		}

		if (out[2] > 60)
		{
			return;
		}

		movetype = MOVETYPE_BOUNCE;

		/* if we're here, we're going to stop on an entity */
		if (stick_ok)
		{
			/* it's a happy entity */
			VectorCopy(vec3_origin, ent->velocity);
			VectorCopy(vec3_origin, ent->avelocity);
		}
		else /* no-stick.  teflon time */
		{
			if (plane->normal[2] > 0.7)
			{
				Prox_Explode(ent);
				return;
			}

			return;
		}
	}
	else if (other->s.modelindex != 1)
	{
		return;
	}

	vectoangles2(plane->normal, dir);
	AngleVectors(dir, forward, right, up);

	if (gi.pointcontents(ent->s.origin) & (CONTENTS_LAVA | CONTENTS_SLIME))
	{
		Prox_Explode(ent);
		return;
	}

	field = G_Spawn();

	VectorCopy(ent->s.origin, field->s.origin);
	VectorClear(field->velocity);
	VectorClear(field->avelocity);
	VectorSet(field->mins, -PROX_BOUND_SIZE, -PROX_BOUND_SIZE, -PROX_BOUND_SIZE);
	VectorSet(field->maxs, PROX_BOUND_SIZE, PROX_BOUND_SIZE, PROX_BOUND_SIZE);
	field->movetype = MOVETYPE_NONE;
	field->solid = SOLID_TRIGGER;
	field->owner = ent;
	field->classname = "prox_field";
	field->teammaster = ent;
	gi.linkentity(field);

	VectorClear(ent->velocity);
	VectorClear(ent->avelocity);

	/* rotate to vertical */
	dir[PITCH] = dir[PITCH] + 90;
	VectorCopy(dir, ent->s.angles);
	ent->takedamage = DAMAGE_AIM;
	ent->movetype = movetype; /* either bounce or none, depending on whether we stuck to something */
	ent->die = prox_die;
	ent->teamchain = field;
	ent->health = PROX_HEALTH;
	ent->nextthink = level.time + 0.05;
	ent->think = prox_open;
	ent->touch = NULL;
	ent->solid = SOLID_BBOX;

	/* record who we're attached to */
	gi.linkentity(ent);
}

void
fire_prox(edict_t *self, vec3_t start, vec3_t aimdir, int damage_multiplier, int speed)
{
	edict_t *prox;
	vec3_t dir;
	vec3_t forward, right, up;

	if (!self)
	{
		return;
	}

	vectoangles2(aimdir, dir);
	AngleVectors(dir, forward, right, up);

	prox = G_Spawn();
	VectorCopy(start, prox->s.origin);
	VectorScale(aimdir, speed, prox->velocity);
	VectorMA(prox->velocity, 200 + crandom() * 10.0, up, prox->velocity);
	VectorMA(prox->velocity, crandom() * 10.0, right, prox->velocity);
	VectorCopy(dir, prox->s.angles);
	prox->s.angles[PITCH] -= 90;
	prox->movetype = MOVETYPE_BOUNCE;
	prox->solid = SOLID_BBOX;
	prox->s.effects |= EF_GRENADE;
	prox->clipmask = MASK_SHOT | CONTENTS_LAVA | CONTENTS_SLIME;
	prox->s.renderfx |= RF_IR_VISIBLE;
	VectorSet(prox->mins, -6, -6, -6);
	VectorSet(prox->maxs, 6, 6, 6);
	prox->s.modelindex = gi.modelindex("models/weapons/g_prox/tris.md2");
	prox->owner = self;
	prox->teammaster = self;
	prox->touch = prox_land;
	prox->think = Prox_Explode;
//	prox->dmg = PROX_DAMAGE * 4; 
	prox->dmg = PROX_DAMAGE * damage_multiplier;
	prox->classname = "prox";
	prox->svflags |= SVF_DAMAGEABLE;
	prox->flags |= FL_MECHANICAL;

	switch (damage_multiplier)
	{
		case 2:
			prox->nextthink = level.time + 30;
			break;
		case 4:
			prox->nextthink = level.time + 15;
			break;
		case 8:
			prox->nextthink = level.time + 10;
			break;
		default:
			prox->nextthink = level.time + PROX_TIME_TO_LIVE;
			break;
	}

	gi.linkentity(prox);
}

void
Weapon_Prox(edict_t *ent)
{
//	static int pause_frames[] = {22, 29, 0};
	static int pause_frames[] = {29, 34, 39, 48, 0};

	if (!ent)
	{
		return;
	}

	Throw_Generic(ent, 15, 48, 5, 11, 12, pause_frames, 0, weapon_grenade_fire2);
//	Throw_Generic(ent, 7, 27, 99, 2, 4, pause_frames, 0, weapon_grenade_fire2);
}

/*
 * ======================================================================
 *
 * PROX LAUNCHER
 *
 * ======================================================================
 */

void
weapon_proxlauncher_fire(edict_t *ent)
{
	vec3_t offset;
	vec3_t forward, right;
	vec3_t start;
	int damage_multiplier;
//	int damage;
//	float radius;

	if (!ent)
	{
		return;
	}

//	damage = 90;
//	radius = damage + 40;

	damage_multiplier = 1;
	if (is_quad)
	{
		damage_multiplier = 4;
//		damage *= damage_multiplier;
	}

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	fire_prox(ent, start, forward, damage_multiplier, 600);

	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte(MZ_GRENADE | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
	{
		ent->client->pers.inventory[ent->client->ammo_index]--;
	}
}

void
Weapon_ProxLauncher(edict_t *ent)
{
	static int pause_frames[] = {34, 51, 59, 0};
	static int fire_frames[] = {6, 0};

	if (!ent)
	{
		return;
	}

	Weapon_Generic(ent, 5, 16, 59, 64, pause_frames,
			fire_frames, weapon_proxlauncher_fire);
}

/*
 * =======================================================================
 *
 *	Chainsaw/Melee
 *
 * =======================================================================
 */
void
fire_player_melee(edict_t *self, vec3_t start, vec3_t aim, int reach,
		int damage, int kick, int quiet, int mod)
{
	vec3_t forward, right, up;
	vec3_t v;
	vec3_t point;
	trace_t tr;

	if (!self)
	{
		return;
	}

	vectoangles2(aim, v);
	AngleVectors(v, forward, right, up);
	VectorNormalize(forward);
	VectorMA(start, reach, forward, point);

	/* see if the hit connects */
	tr = gi.trace(start, NULL, NULL, point, self, MASK_SHOT);

	if (tr.fraction == 1.0)
	{
		if (!quiet)
		{
			gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/swish.wav"), 1, ATTN_NORM, 0);
		}

		return;
	}

	if ((tr.ent->takedamage == DAMAGE_YES) ||
		(tr.ent->takedamage == DAMAGE_AIM))
	{
		/* pull the player forward if you do damage */
		VectorMA(self->velocity, 75, forward, self->velocity);
		VectorMA(self->velocity, 75, up, self->velocity);

		/* do the damage */
		if (mod == MOD_CHAINFIST)
		{
			T_Damage(tr.ent, self, self, vec3_origin, tr.ent->s.origin, vec3_origin,
					damage, kick / 2, DAMAGE_DESTROY_ARMOR | DAMAGE_NO_KNOCKBACK, mod);
		}
		else
		{
			T_Damage(tr.ent, self, self, vec3_origin, tr.ent->s.origin, vec3_origin,
					damage, kick / 2, DAMAGE_NO_KNOCKBACK, mod);
		}

		if (!quiet)
		{
			gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/meatht.wav"), 1, ATTN_NORM, 0);
		}
	}
	else
	{
		if (!quiet)
		{
			gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/tink1.wav"), 1, ATTN_NORM, 0);
		}

		VectorScale(tr.plane.normal, 256, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_GUNSHOT);
		gi.WritePosition(tr.endpos);
		gi.WriteDir(point);
		gi.multicast(tr.endpos, MULTICAST_PVS);
	}
}

/* CHAINFIST */

void
weapon_chainfist_fire(edict_t *ent)
{
	vec3_t offset;
	vec3_t forward, right, up;
	vec3_t start;
	int damage;

	if (!ent)
	{
		return;
	}

	damage = 15;

	if (deathmatch->value)
	{
		damage = 30;
	}

	if (is_quad)
	{
		damage *= 4;
	}

	AngleVectors(ent->client->v_angle, forward, right, up);

	/* kick back */
	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	/* set start point */
	VectorSet(offset, 0, 8, ent->viewheight - 4);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_player_melee(ent, start, forward, CHAINFIST_REACH, damage,
			100, 1, MOD_CHAINFIST);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->ps.gunframe++;
	ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;
}

/*
 * this spits out some smoke from the motor. it's a two-stroke, you know.
 */
void
chainfist_smoke(edict_t *ent)
{
	vec3_t tempVec, forward, right, up;
	vec3_t offset;

	if (!ent)
	{
		return;
	}

	AngleVectors(ent->client->v_angle, forward, right, up);
	VectorSet(offset, 8, 8, ent->viewheight - 4);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, tempVec);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_CHAINFIST_SMOKE);
	gi.WritePosition(tempVec);
	gi.unicast(ent, 0);
}

void
Weapon_ChainFist(edict_t *ent)
{
	static int pause_frames[] = {0};
	static int fire_frames[] = {8, 9, 16, 17, 18, 30, 31, 0};

	/* these are caches for the sound index. there's probably a better way to do this. */
	float chance;
	int last_sequence;

	last_sequence = 0;

	if ((ent->client->ps.gunframe == 13) ||
		(ent->client->ps.gunframe == 23)) /* end of attack, go idle */
	{
		ent->client->ps.gunframe = 32;
	}

	/* holds for idle sequence */
	else if ((ent->client->ps.gunframe == 42) && (rand() & 7))
	{
		if ((ent->client->pers.hand != CENTER_HANDED) && (random() < 0.4))
		{
			chainfist_smoke(ent);
		}
	}
	else if ((ent->client->ps.gunframe == 51) && (rand() & 7))
	{
		if ((ent->client->pers.hand != CENTER_HANDED) && (random() < 0.4))
		{
			chainfist_smoke(ent);
		}
	}

	/* set the appropriate weapon sound. */
	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		ent->client->weapon_sound = gi.soundindex("weapons/saw/sawhit.wav");
	}
	else if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		ent->client->weapon_sound = 0;
	}
	else
	{
		ent->client->weapon_sound = gi.soundindex("weapons/saw/sawidle.wav");
	}

	Weapon_Generic(ent, 4, 32, 57, 60, pause_frames,
			fire_frames, weapon_chainfist_fire);

	if ((ent->client->buttons) & BUTTON_ATTACK)
	{
		if ((ent->client->ps.gunframe == 13) ||
			(ent->client->ps.gunframe == 23) ||
			(ent->client->ps.gunframe == 32))
		{
			last_sequence = ent->client->ps.gunframe;
			ent->client->ps.gunframe = 6;
		}
	}

	if (ent->client->ps.gunframe == 6)
	{
		chance = random();

		if (last_sequence == 13) /* if we just did sequence 1, do 2 or 3. */
		{
			chance -= 0.34;
		}
		else if (last_sequence == 23) /* if we just did sequence 2, do 1 or 3 */
		{
			chance += 0.33;
		}
		else if (last_sequence == 32) /* if we just did sequence 3, do 1 or 2 */
		{
			if (chance >= 0.33)
			{
				chance += 0.34;
			}
		}

		if (chance < 0.33)
		{
			ent->client->ps.gunframe = 14;
		}
		else if (chance < 0.66)
		{
			ent->client->ps.gunframe = 24;
		}
	}
}

/*
 * =======================================================================
 *
 *	Tesla Mines
 *
 * =======================================================================
 */
void
tesla_remove(edict_t *self)
{
	edict_t *cur, *next;

	if (!self)
	{
		return;
	}

	self->takedamage = DAMAGE_NO;

	if (self->teamchain)
	{
		cur = self->teamchain;

		while (cur)
		{
			next = cur->teamchain;
			G_FreeEdict(cur);
			cur = next;
		}
	}
	else if (self->air_finished)
	{
		gi.dprintf("tesla without a field!\n");
	}

	self->owner = self->teammaster; /* Going away, set the owner correctly. */
	self->enemy = NULL;

	/* play quad sound if quadded and an underwater explosion */
	if ((self->dmg_radius) && (self->dmg > (TESLA_DAMAGE * TESLA_EXPLOSION_DAMAGE_MULT)))
	{
		gi.sound(self, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
	}

	Grenade_Explode(self);
}

void
tesla_die(edict_t *self, edict_t *inflictor /* unused */, edict_t *attacker /* unused */,
		int damage /* unused */, vec3_t point /* unused */)
{
	if (!self)
	{
		return;
	}

	tesla_remove(self);
}

void
tesla_blow(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->dmg = self->dmg * TESLA_EXPLOSION_DAMAGE_MULT;
	self->dmg_radius = TESLA_EXPLOSION_RADIUS;
	tesla_remove(self);
}

void
tesla_zap(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
}

void
tesla_think_active(edict_t *self)
{
	int i, num;
	edict_t *touch[MAX_EDICTS], *hit;
	vec3_t dir, start;
	trace_t tr;

	if (!self)
	{
		return;
	}

	if (level.time > self->air_finished)
	{
		tesla_remove(self);
		return;
	}

	VectorCopy(self->s.origin, start);
	start[2] += 16;

	num = gi.BoxEdicts(self->teamchain->absmin, self->teamchain->absmax,
			touch, MAX_EDICTS, AREA_SOLID);

	for (i = 0; i < num; i++)
	{
		/* if the tesla died while zapping things, stop zapping. */
		if (!(self->inuse))
		{
			break;
		}

		hit = touch[i];

		if (!hit->inuse)
		{
			continue;
		}

		if (hit == self)
		{
			continue;
		}

//	ace - this should be a fun tweak
//		if (hit->health < 1)
//		if (((hit->gib_health) && (hit->health < hit->gib_health)) || (hit->health < 1))
		if (hit->health < hit->gib_health)
		{
			continue;
		}

		/* don't hit clients in single-player or coop */
		if (hit->client)
		{
			if (coop->value || !deathmatch->value)
			{
				continue;
			}
		}

		if (!(hit->svflags & (SVF_MONSTER | SVF_DAMAGEABLE)) && !hit->client)
		{
			continue;
		}

		tr = gi.trace(start, vec3_origin, vec3_origin, hit->s.origin,
				self, MASK_SHOT);

		if ((tr.fraction == 1) || (tr.ent == hit))
		{
			VectorSubtract(hit->s.origin, start, dir);

			/* play quad sound if it's above the "normal" damage */
			if (self->dmg > TESLA_DAMAGE)
			{
				gi.sound(self, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
			}

			/*  don't do knockback to walking monsters */
			if ((hit->svflags & SVF_MONSTER) &&
				!(hit->flags & (FL_FLY | FL_SWIM)))
			{
				T_Damage(hit, self, self->teammaster, dir, tr.endpos,
						tr.plane.normal, self->dmg, 0, 0, MOD_TESLA);
			}
			else
			{
				T_Damage(hit, self, self->teammaster, dir, tr.endpos, tr.plane.normal,
						self->dmg, TESLA_KNOCKBACK, 0, MOD_TESLA);
			}

			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_LIGHTNING);
			gi.WriteShort(hit - g_edicts); /* destination entity */
			gi.WriteShort(self - g_edicts); /* source entity */
			gi.WritePosition(tr.endpos);
			gi.WritePosition(start);
			gi.multicast(start, MULTICAST_PVS);
		}
	}

	if (self->inuse)
	{
		self->think = tesla_think_active;
		self->nextthink = level.time + FRAMETIME;
	}
}

void
tesla_activate(edict_t *self)
{
	edict_t *trigger;
	edict_t *search;

	if (!self)
	{
		return;
	}

	if (gi.pointcontents(self->s.origin) & (CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WATER))
	{
		tesla_blow(self);
		return;
	}

	/* only check for spawn points in deathmatch */
	if (deathmatch->value)
	{
		search = NULL;

		while ((search = findradius(search, self->s.origin, 1.5 * TESLA_DAMAGE_RADIUS)) != NULL)
		{
			if (search->classname)
			{
				if (((!strcmp(search->classname, "info_player_deathmatch")) ||
					 (!strcmp(search->classname, "info_player_start")) ||
					 (!strcmp(search->classname, "info_player_coop")) ||
					 (!strcmp(search->classname, "misc_teleporter_dest"))) &&
						(visible(search, self)))
				{
					tesla_remove(self);
					return;
				}
			}
		}
	}

	trigger = G_Spawn();
	VectorCopy(self->s.origin, trigger->s.origin);
	VectorSet(trigger->mins, -TESLA_DAMAGE_RADIUS, -TESLA_DAMAGE_RADIUS, self->mins[2]);
	VectorSet(trigger->maxs, TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS);
	trigger->movetype = MOVETYPE_NONE;
	trigger->solid = SOLID_TRIGGER;
	trigger->owner = self;
	trigger->touch = tesla_zap;
	trigger->classname = "tesla trigger";

	/* doesn't need to be marked as a teamslave since the move code for bounce looks for teamchains */
	gi.linkentity(trigger);

	VectorClear(self->s.angles);

	/* clear the owner if in deathmatch */
	if (deathmatch->value)
	{
		self->owner = NULL;
	}

	self->teamchain = trigger;
	self->think = tesla_think_active;
	self->nextthink = level.time + FRAMETIME;
	self->air_finished = level.time + TESLA_TIME_TO_LIVE;
}

void
tesla_think(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (gi.pointcontents(ent->s.origin) & (CONTENTS_SLIME | CONTENTS_LAVA))
	{
		tesla_remove(ent);
		return;
	}

	VectorClear(ent->s.angles);

	if (!(ent->s.frame))
	{
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/tesla/teslaopen.wav"), 1, ATTN_NORM, 0);
	}

	ent->s.frame++;

	if (ent->s.frame > 14)
	{
		ent->s.frame = 14;
		ent->think = tesla_activate;
		ent->nextthink = level.time + 0.1;
	}
	else
	{
		if (ent->s.frame > 9)
		{
			if (ent->s.frame == 10)
			{
				if (ent->owner && ent->owner->client)
				{
					PlayerNoise(ent->owner, ent->s.origin, PNOISE_WEAPON);      /* PGM */
				}

				ent->s.skinnum = 1;
			}
			else if (ent->s.frame == 12)
			{
				ent->s.skinnum = 2;
			}
			else if (ent->s.frame == 14)
			{
				ent->s.skinnum = 3;
			}
		}

		ent->think = tesla_think;
		ent->nextthink = level.time + 0.1;
	}
}

void
tesla_lava(edict_t *ent, edict_t *other /* unused */, cplane_t *plane, csurface_t *surf /* unused */)
{
	vec3_t land_point;

	if (!ent || !plane)
	{
		return;
	}

	VectorMA(ent->s.origin, -20.0, plane->normal, land_point);

	if (gi.pointcontents(land_point) & (CONTENTS_SLIME | CONTENTS_LAVA))
	{
		tesla_blow(ent);
		return;
	}

	if (random() > 0.5)
	{
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
	}
	else
	{
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
	}
}

void
fire_tesla(edict_t *self, vec3_t start, vec3_t aimdir,
		int damage_multiplier, int speed)
{
	edict_t *tesla;
	vec3_t dir;
	vec3_t forward, right, up;

	if (!self)
	{
		return;
	}

	vectoangles2(aimdir, dir);
	AngleVectors(dir, forward, right, up);

	tesla = G_Spawn();
	VectorCopy(start, tesla->s.origin);
	VectorScale(aimdir, speed, tesla->velocity);
	VectorMA(tesla->velocity, 200 + crandom() * 10.0, up, tesla->velocity);
	VectorMA(tesla->velocity, crandom() * 10.0, right, tesla->velocity);
	VectorClear(tesla->s.angles);
	tesla->movetype = MOVETYPE_BOUNCE;
	tesla->solid = SOLID_BBOX;
	tesla->s.effects |= EF_GRENADE;
	tesla->s.renderfx |= RF_IR_VISIBLE;
	VectorSet(tesla->mins, -12, -12, 0);
	VectorSet(tesla->maxs, 12, 12, 20);
	tesla->s.modelindex = gi.modelindex("models/weapons/g_tesla/tris.md2");

	tesla->owner = self;
	tesla->teammaster = self;

	tesla->wait = level.time + TESLA_TIME_TO_LIVE;
	tesla->think = tesla_think;
	tesla->nextthink = level.time + TESLA_ACTIVATE_TIME;

	/* blow up on contact with lava & slime code */
	tesla->touch = tesla_lava;

	if (deathmatch->value)
	{
		tesla->health = 20;
	}
	else
	{
		tesla->health = 30;
	}

	tesla->takedamage = DAMAGE_YES;
	tesla->die = tesla_die;
	tesla->dmg = TESLA_DAMAGE * damage_multiplier;
	tesla->classname = "tesla";
	tesla->svflags |= SVF_DAMAGEABLE;
	tesla->clipmask = MASK_SHOT | CONTENTS_SLIME | CONTENTS_LAVA;
	tesla->flags |= FL_MECHANICAL;

	gi.linkentity(tesla);
}

void
Weapon_Tesla(edict_t *ent)
{
	static int pause_frames[] = {21, 0};

	if (!ent)
	{
		return;
	}

	if ((ent->client->ps.gunframe > 1) && (ent->client->ps.gunframe < 9))
	{
		ent->client->ps.gunindex = gi.modelindex("models/weapons/v_tesla2/tris.md2");
	}
	else
	{
		ent->client->ps.gunindex = gi.modelindex("models/weapons/v_tesla/tris.md2");
	}

	Throw_Generic(ent, 8, 32, 99, 1, 2, pause_frames, 0, weapon_grenade_fire2);
}

/*
 * =======================================================================
 *
 *	Disintegrator/Tracker (rogue)
 *
 * =======================================================================
 */
/*
 * This is called to clean up the pain daemons that
 * the disruptor attaches to clients to damage them.
 */
void
RemoveAttackingPainDaemons(edict_t *self)
{
	edict_t *tracker;

	if (!self)
	{
		return;
	}

	tracker = G_Find(NULL, FOFS(classname), "pain daemon");

	while (tracker)
	{
		if (tracker->enemy == self)
		{
			G_FreeEdict(tracker);
		}

		tracker = G_Find(tracker, FOFS(classname), "pain daemon");
	}

	if (self->client)
	{
		self->client->tracker_pain_framenum = 0;
	}
}

void
tracker_pain_daemon_think(edict_t *self)
{
	static vec3_t pain_normal = {0, 0, 1};
	int hurt;

	if (!self)
	{
		return;
	}

	if (!self->inuse)
	{
		return;
	}

	if ((level.time - self->timestamp) > TRACKER_DAMAGE_TIME)
	{
		if (!self->enemy->client)
		{
			self->enemy->s.effects &= ~EF_TRACKERTRAIL;
		}

		G_FreeEdict(self);
	}
	else
	{
		if (self->enemy->health > 0)
		{
			T_Damage(self->enemy, self, self->owner, vec3_origin, self->enemy->s.origin,
					pain_normal, self->dmg, 0, TRACKER_DAMAGE_FLAGS, MOD_TRACKER);

			/* if we kill the player, we'll be removed. */
			if (self->inuse)
			{
				/* if we killed a monster, gib them. */
				if (self->enemy->health < 1)
				{
					if (self->enemy->gib_health)
					{
						hurt = 999 + (rand() & 999);
					}
					else
					{
						hurt = 500 + (rand() & 500);
					}

					T_Damage(self->enemy, self, self->owner, vec3_origin, self->enemy->s.origin,
							pain_normal, hurt, 0, TRACKER_DAMAGE_FLAGS, MOD_TRACKER);
				}

				if (self->enemy->client)
				{
					self->enemy->client->tracker_pain_framenum = level.framenum + 1;
				}
				else
				{
					self->enemy->s.effects |= EF_TRACKERTRAIL;
				}

				self->nextthink = level.time + FRAMETIME;
			}
		}
		else
		{
			if (!self->enemy->client)
			{
				self->enemy->s.effects &= ~EF_TRACKERTRAIL;
			}

			G_FreeEdict(self);
		}
	}
}

void
tracker_pain_daemon_spawn(edict_t *owner, edict_t *enemy, int damage)
{
	edict_t *daemon;

	if (!owner || !enemy)
	{
		return;
	}

	daemon = G_Spawn();
	daemon->classname = "pain daemon";
	daemon->think = tracker_pain_daemon_think;
	daemon->nextthink = level.time + FRAMETIME;
	daemon->timestamp = level.time;
	daemon->owner = owner;
	daemon->enemy = enemy;
	daemon->dmg = damage;
}

void
tracker_explode(edict_t *self, cplane_t *plane)
{
	vec3_t dir;

	if (!self)
	{
		return;
	}

	if (!plane)
	{
		VectorClear(dir);
	}
	else
	{
		VectorScale(plane->normal, 256, dir);
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TRACKER_EXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	G_FreeEdict(self);
}

void
tracker_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	float damagetime;

	if (!self || !other || !surf || !plane)
	{
		return;
	}

	if (other == self->owner)
	{
		return;
	}

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->client)
	{
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);
	}

	if (other->takedamage)
	{
		if ((other->svflags & SVF_MONSTER) || other->client)
		{
			if (other->health > 0) /* knockback only for living creatures */
			{
				T_Damage(other, self, self->owner, self->velocity, self->s.origin,
						plane->normal, 0, (self->dmg * 3), TRACKER_IMPACT_FLAGS,
						MOD_TRACKER);

				if (!(other->flags & (FL_FLY | FL_SWIM)))
				{
					other->velocity[2] += 140;
				}

				damagetime = ((float)self->dmg) * FRAMETIME;
				damagetime = damagetime / TRACKER_DAMAGE_TIME;

				tracker_pain_daemon_spawn(self->owner, other, (int)damagetime);
			}
			else /* lots of damage (almost autogib) for dead bodies */
			{
				T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal,
						self->dmg * 8, (self->dmg * 6), TRACKER_IMPACT_FLAGS, MOD_TRACKER);
			}
		}
		else /* full damage in one shot for inanimate objects */
		{
			T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal,
					self->dmg, (self->dmg * 3), TRACKER_IMPACT_FLAGS, MOD_TRACKER);
		}
	}

	tracker_explode(self, plane);
	return;
}

void
tracker_fly(edict_t *self)
{
	vec3_t dest;
	vec3_t dir;
	vec3_t center;

	if (!self)
	{
		return;
	}

	if ((!self->enemy) || (!self->enemy->inuse) || (self->enemy->health < 1))
	{
		tracker_explode(self, NULL);
		return;
	}

	/* try to hunt for center of enemy, if possible and not client */
	if (self->enemy->client)
	{
		VectorCopy(self->enemy->s.origin, dest);
		dest[2] += self->enemy->viewheight;
	}
	else if (VectorCompare(self->enemy->absmin, vec3_origin) ||
			 VectorCompare(self->enemy->absmax, vec3_origin))
	{
		VectorCopy(self->enemy->s.origin, dest);
	}
	else
	{
		VectorMA(vec3_origin, 0.5, self->enemy->absmin, center);
		VectorMA(center, 0.5, self->enemy->absmax, center);
		VectorCopy(center, dest);
	}

	VectorSubtract(dest, self->s.origin, dir);
	VectorNormalize(dir);
	vectoangles2(dir, self->s.angles);
	VectorScale(dir, self->speed, self->velocity);
	VectorCopy(dest, self->monsterinfo.saved_goal);

	self->nextthink = level.time + 0.1;
}

void
fire_tracker(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, edict_t *enemy)
{
	edict_t *bolt;
	trace_t tr;

	if (!self /* || !enemy */ )
	{
		return;
	}

	VectorNormalize(dir);

	bolt = G_Spawn();
	VectorCopy(start, bolt->s.origin);
	VectorCopy(start, bolt->s.old_origin);
	vectoangles2(dir, bolt->s.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->speed = speed;
	bolt->s.effects = EF_TRACKER;
	bolt->s.sound = gi.soundindex("weapons/disrupt.wav");
	VectorClear(bolt->mins);
	VectorClear(bolt->maxs);

	bolt->s.modelindex = gi.modelindex("models/proj/disintegrator/tris.md2");
	bolt->touch = tracker_touch;
	bolt->enemy = enemy;
	bolt->owner = self;
	bolt->dmg = damage;
	bolt->classname = "tracker";
	gi.linkentity(bolt);

	if (enemy)
	{
		bolt->nextthink = level.time + 0.1;
		bolt->think = tracker_fly;
	}
	else
	{
		bolt->nextthink = level.time + 2 + random(); // 10;
		bolt->think = tracker_fly;
//		bolt->think = G_FreeEdict;
	}

	if (self->client)
	{
		check_dodge(self, bolt->s.origin, dir, speed);
	}

	tr = gi.trace(self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);

	if (tr.fraction < 1.0)
	{
		VectorMA(bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}
}

void
weapon_tracker_fire(edict_t *self)
{
	vec3_t forward, right;
	vec3_t start;
	vec3_t end;
	vec3_t offset;
	edict_t *enemy;
	trace_t tr;
	int damage;
	vec3_t mins, maxs;

	if (!self)
	{
		return;
	}

	if (deathmatch->value)
	{
		damage = 30;
	}
	else
	{
		damage = 45;
	}

	if (is_quad)
	{
		damage *= 4;
	}

	VectorSet(mins, -16, -16, -16);
	VectorSet(maxs, 16, 16, 16);
	AngleVectors(self->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, self->viewheight - 8);
	P_ProjectSource(self->client, self->s.origin, offset, forward, right, start);

	VectorMA(start, 8192, forward, end);
	enemy = NULL;
	tr = gi.trace(start, vec3_origin, vec3_origin, end, self, MASK_SHOT);

	if (tr.ent != world)
	{
		if (tr.ent->svflags & SVF_MONSTER || tr.ent->client || tr.ent->svflags & SVF_DAMAGEABLE)
		{
			if (tr.ent->health > 0)
			{
				enemy = tr.ent;
			}
		}
	}
	else
	{
		tr = gi.trace(start, mins, maxs, end, self, MASK_SHOT);

		if (tr.ent != world)
		{
			if (tr.ent->svflags & SVF_MONSTER || tr.ent->client ||
				tr.ent->svflags & SVF_DAMAGEABLE)
			{
				if (tr.ent->health > 0)
				{
					enemy = tr.ent;
				}
			}
		}
	}

	VectorScale(forward, -2, self->client->kick_origin);
	self->client->kick_angles[0] = -1;

	fire_tracker(self, start, forward, damage, 1000, enemy);

	/* send muzzle flash */
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(MZ_TRACKER);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	PlayerNoise(self, start, PNOISE_WEAPON);

	self->client->ps.gunframe++;

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
	{
		self->client->pers.inventory[self->client->ammo_index] -= self->client->pers.weapon->quantity;
	}
}

void
Weapon_Disintegrator(edict_t *ent)
{
	static int pause_frames[] = {14, 19, 23, 0};
	static int fire_frames[] = {5, 0};

	Weapon_Generic(ent, 4, 9, 29, 34, pause_frames,
			fire_frames, weapon_tracker_fire);
}

// ace - (rogue) blaster2
/*
 * Fires a single green blaster bolt.  Used by monsters, generally.
 */
void
blaster2_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int mod;
	int damagestat;

	if (!self || !other || !plane || !surf)
	{
		return;
	}

	if (other == self->owner)
	{
		return;
	}

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->owner && self->owner->client)
	{
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);
	}

	if (other->takedamage)
	{
		/* the only time players will be firing blaster2
		   bolts will be from the defender sphere. */
/*		if (self->owner->client)
		{
			mod = MOD_DEFENDER_SPHERE;
		}
		else */
		{
			mod = MOD_BLASTER;
		}

		if (self->owner)
		{
			damagestat = self->owner->takedamage;
			self->owner->takedamage = DAMAGE_NO;

			if (self->dmg >= 5)
			{
				T_RadiusDamage(self, self->owner, self->dmg * 3, other,
						self->dmg_radius, 0);
			}

			T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal,
					self->dmg, 1, DAMAGE_ENERGY, mod);
			self->owner->takedamage = damagestat;
		}
		else
		{
			if (self->dmg >= 5)
			{
				T_RadiusDamage(self, self->owner, self->dmg * 3, other,
						self->dmg_radius, 0);
			}

			T_Damage(other, self, self->owner, self->velocity, self->s.origin,
					plane->normal, self->dmg, 1, DAMAGE_ENERGY, mod);
		}
	}
	else
	{
		/* yeowch this will get expensive */
		if (self->dmg >= 5)
		{
			T_RadiusDamage(self, self->owner, self->dmg * 3, self->owner,
					self->dmg_radius, 0);
		}

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BLASTER2);
		gi.WritePosition(self->s.origin);

		if (!plane)
		{
			gi.WriteDir(vec3_origin);
		}
		else
		{
			gi.WriteDir(plane->normal);
		}

		gi.multicast(self->s.origin, MULTICAST_PVS);
	}

	G_FreeEdict(self);
}

void
fire_blaster2(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int effect, qboolean hyper)
{
	edict_t *bolt;
	trace_t tr;

	if (!self)
	{
		return;
	}

	VectorNormalize(dir);

	bolt = G_Spawn();
	VectorCopy(start, bolt->s.origin);
	VectorCopy(start, bolt->s.old_origin);
	vectoangles2(dir, bolt->s.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= effect;
	VectorClear(bolt->mins);
	VectorClear(bolt->maxs);

	if (effect)
	{
		bolt->s.effects |= EF_TRACKER;
	}

	bolt->dmg_radius = 128;
	bolt->s.modelindex = gi.modelindex("models/proj/laser2/tris.md2");
	bolt->touch = blaster2_touch;

	bolt->owner = self;
	bolt->nextthink = level.time + 2;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->classname = "bolt";
	gi.linkentity(bolt);

	if (self->client)
	{
		check_dodge(self, bolt->s.origin, dir, speed);
	}

	tr = gi.trace(self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);

	if (tr.fraction < 1.0)
	{
		VectorMA(bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}
}

// xatrix blueblaster code
void blaster_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);

void
fire_blueblaster(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int effect)
{
	edict_t *bolt;
	trace_t tr;

	if (!self)
	{
		return;
	}

	VectorNormalize(dir);

	bolt = G_Spawn();
	VectorCopy(start, bolt->s.origin);
	VectorCopy(start, bolt->s.old_origin);
	vectoangles(dir, bolt->s.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= effect;
	VectorClear(bolt->mins);
	VectorClear(bolt->maxs);

	bolt->s.modelindex = gi.modelindex("models/objects/blaser/tris.md2");
	bolt->s.sound = gi.soundindex("misc/lasfly.wav");
	bolt->owner = self;
	bolt->touch = blaster_touch;
	bolt->nextthink = level.time + 2;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->classname = "bolt";
	gi.linkentity(bolt);

	if (self->client)
	{
		check_dodge(self, bolt->s.origin, dir, speed);
	}

	tr = gi.trace(self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);

	if (tr.fraction < 1.0)
	{
		VectorMA(bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}
}
