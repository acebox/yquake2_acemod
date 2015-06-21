/*
 * =======================================================================
 *
 * Additional Monster Functions.
 *
 * =======================================================================
 */

#include "../header/local.h"


void
monster_fire_tracker(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, edict_t *enemy, int flashtype)
{
	if (!self || !enemy)
	{
		return;
	}

	fire_tracker(self, start, dir, damage, speed, enemy);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void
monster_fire_blaster2(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype, int effect)
{
	if (!self)
	{
		return;
	}

	fire_blaster2(self, start, dir, damage, speed, effect, false);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void
monster_fire_blueblaster(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype, int effect)
{
	if (!self)
	{
		return;
	}

	fire_blueblaster(self, start, dir, damage, speed, effect);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(MZ_BLUEHYPERBLASTER);
	gi.multicast(start, MULTICAST_PVS);
}

void
monster_fire_ionripper(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype, int effect)
{
 	if (!self)
	{
		return;
	}

	fire_ionripper(self, start, dir, damage, speed, effect);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void
monster_fire_heat(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype)
{
 	if (!self)
	{
		return;
	}

/*void
fire_heat(edict_t *self, vec3_t start, vec3_t aimdir, vec3_t offset,
		int damage, int kick, qboolean monster)
*/

	fire_heat(self, start, dir, vec3_origin, damage, speed, true);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void
dabeam_hit(edict_t *self)
{
	edict_t *ignore;
	vec3_t start;
	vec3_t end;
	trace_t tr;

  	if (!self)
	{
		return;
	}

	ignore = self;
	VectorCopy(self->s.origin, start);
	VectorMA(start, 2048, self->movedir, end);

	while (1)
	{
		tr = gi.trace(start, NULL, NULL, end, ignore,
				CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);

		if (!tr.ent)
		{
			break;
		}

		/* hurt it if we can */
		if ((tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER) &&
			(tr.ent != self->owner))
		{
			T_Damage(tr.ent, self, self->owner, self->movedir, tr.endpos,
					vec3_origin, self->dmg, skill->value, DAMAGE_ENERGY,
					MOD_TARGET_LASER);
		}

		if (self->dmg < 0) /* healer ray */
		{
			/* when player is at 100 health
			   just undo health fix */
			if (tr.ent->client && (tr.ent->health > 100))
			{
				tr.ent->health += self->dmg;
			}
		}

		/* if we hit something that's not a monster or
		   player or is immune to lasers, we're done */
		if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
		{
			if (self->spawnflags & 0x80000000)
			{
				self->spawnflags &= ~0x80000000;
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_LASER_SPARKS);
				gi.WriteByte(10);
				gi.WritePosition(tr.endpos);
				gi.WriteDir(tr.plane.normal);
				gi.WriteByte(self->s.skinnum);
				gi.multicast(tr.endpos, MULTICAST_PVS);
			}

			break;
		}

		ignore = tr.ent;
		VectorCopy(tr.endpos, start);
	}

	VectorCopy(tr.endpos, self->s.old_origin);
	self->nextthink = level.time + 0.1;
	self->think = G_FreeEdict;
}

void
monster_dabeam(edict_t *self)
{
	vec3_t last_movedir;
	vec3_t point;

  	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->s.renderfx |= RF_BEAM | RF_TRANSLUCENT;
	self->s.modelindex = 1;

	self->s.frame = 2;

	if (self->owner->monsterinfo.aiflags & AI_MEDIC)
	{
		self->s.skinnum = 0xf3f3f1f1;
	}
	else
	{
		self->s.skinnum = 0xf2f2f0f0;
	}

	if (self->enemy)
	{
		VectorCopy(self->movedir, last_movedir);
		VectorMA(self->enemy->absmin, 0.5, self->enemy->size, point);

		if (self->owner->monsterinfo.aiflags & AI_MEDIC)
		{
			point[0] += sin(level.time) * 8;
		}

		VectorSubtract(point, self->s.origin, self->movedir);
		VectorNormalize(self->movedir);

		if (!VectorCompare(self->movedir, last_movedir))
		{
			self->spawnflags |= 0x80000000;
		}
	}
	else
	{
		G_SetMovedir(self->s.angles, self->movedir);
	}

	self->think = dabeam_hit;
	self->nextthink = level.time + 0.1;
	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);
	gi.linkentity(self);

	self->spawnflags |= 0x80000001;
	self->svflags &= ~SVF_NOCLIENT;
}

void stationarymonster_start_go(edict_t *self);

void
stationarymonster_triggered_spawn(edict_t *self)
{
	if (!self)
	{
		return;
	}

	KillBox(self);

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_NONE;
	self->svflags &= ~SVF_NOCLIENT;
	self->air_finished = level.time + 12;
	gi.linkentity(self);

	self->spawnflags &= ~2;
	stationarymonster_start_go(self);

	if (self->enemy && !(self->spawnflags & 1) &&
		!(self->enemy->flags & FL_NOTARGET))
	{
		if (!(self->enemy->flags & FL_DISGUISED))
		{
			FoundTarget(self);
		}
		else
		{
			self->enemy = NULL;
		}
	}
	else
	{
		self->enemy = NULL;
	}
}

void
stationarymonster_triggered_spawn_use(edict_t *self, edict_t *other /* unused */, edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	/* we have a one frame delay here so we don't telefrag the guy who activated us */
	self->think = stationarymonster_triggered_spawn;
	self->nextthink = level.time + FRAMETIME;

	if (activator->client)
	{
		self->enemy = activator;
	}

	self->use = monster_use;
}

void
stationarymonster_triggered_start(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
	self->use = stationarymonster_triggered_spawn_use;
}

void
stationarymonster_start_go(edict_t *self)
{

	if (!self)
	{
		return;
	}

	if (!self->yaw_speed)
	{
		self->yaw_speed = 20;
	}

	monster_start_go(self);

	if (self->spawnflags & 2)
	{
		stationarymonster_triggered_start(self);
	}
}

void
stationarymonster_start(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->think = stationarymonster_start_go;
	monster_start(self);
}

void
monster_done_dodge(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.aiflags &= ~AI_DODGING;
}

/*
 * clean up heal targets for medic
 */
void
cleanupHealTarget(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->monsterinfo.healer = NULL;
	ent->takedamage = DAMAGE_YES;
	ent->monsterinfo.aiflags &= ~AI_RESURRECTING;
	M_SetEffects(ent);
}