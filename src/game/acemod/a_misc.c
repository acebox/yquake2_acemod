/* =======================================================================
 *
 * Additional Misc Functions
 *
 * =======================================================================
 */

#include "../header/local.h"
// #include "../monster/misc/player.h"

void gib_die(edict_t *self, edict_t *inflictor /* unused */, edict_t *attacker /* unused */, int damage /* unused */, vec3_t point /* unused */);
void VelocityForDamage(int damage, vec3_t v);
void ClipGibVelocity(edict_t *ent);

vec_t VectorLengthSquared(vec3_t v)
{
	int		i;
	float	length;
	
	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];

	return length;
}

void angleToward(edict_t *self, vec3_t point, float speed)
{
	vec3_t forward;
	float yaw = 0.0;
	float vel = 0.0;
	vec3_t delta;
	vec3_t destAngles;
	VectorSubtract(point, self->s.origin, delta);
	vectoangles(delta, destAngles);
	self->ideal_yaw = destAngles[YAW];
	self->yaw_speed = speed;
	M_ChangeYaw(self);
	yaw = self->s.angles[YAW];
	self->ideal_yaw = destAngles[PITCH];
	self->s.angles[YAW] = self->s.angles[PITCH];
	M_ChangeYaw(self);
	self->s.angles[PITCH] = self->s.angles[YAW];
	self->s.angles[YAW] = yaw;
	AngleVectors (self->s.angles, forward, NULL, NULL);
	vel = VectorLength(self->velocity);
	VectorScale(forward, vel, self->velocity);
}

// Misc Utilities (rouge)

void
G_ProjectSource2(vec3_t point, vec3_t distance, vec3_t forward,
		vec3_t right, vec3_t up, vec3_t result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1] +
				up[0] * distance[2];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1] +
				up[1] * distance[2];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] +
				up[2] * distance[2];
}

void
P_ProjectSource2(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward,
		vec3_t right, vec3_t up, vec3_t result)
{
	vec3_t _distance;

	if (!client)
	{
		return;
	}

	VectorCopy(distance, _distance);

	if (client->pers.hand == LEFT_HANDED)
	{
		_distance[1] *= -1;
	}
	else if (client->pers.hand == CENTER_HANDED)
	{
		_distance[1] = 0;
	}

	G_ProjectSource2(point, _distance, forward, right, up, result);
}

// this is the same as function P_ProjectSource in p_weapons.c except it projects
// the offset distance in reverse since hook is launched with player's free hand
void P_ProjectSource_Reverse (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t	_distance;

	VectorCopy (distance, _distance);
	if (client->pers.hand == RIGHT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource (point, _distance, forward, right, result);
}

/*
 * Returns entities that have origins within a spherical area
 */
edict_t *
findradius2(edict_t *from, vec3_t org, float rad)
{
	/* rad must be positive */
	vec3_t eorg;
	int j;

	if (!from)
	{
		from = g_edicts;
	}
	else
	{
		from++;
	}

	for ( ; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
		{
			continue;
		}

		if (from->solid == SOLID_NOT)
		{
			continue;
		}

		if (!from->takedamage)
		{
			continue;
		}

		if (!(from->svflags & SVF_DAMAGEABLE))
		{
			continue;
		}

		for (j = 0; j < 3; j++)
		{
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j]) * 0.5);
		}

		if (VectorLength(eorg) > rad)
		{
			continue;
		}

		return from;
	}

	return NULL;
}

float
vectoyaw2(vec3_t vec)
{
	float yaw;

	if (vec[PITCH] == 0)
	{
		if (vec[YAW] == 0)
		{
			yaw = 0;
		}
		else if (vec[YAW] > 0)
		{
			yaw = 90;
		}
		else
		{
			yaw = 270;
		}
	}
	else
	{
		yaw = (atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);

		if (yaw < 0)
		{
			yaw += 360;
		}
	}

	return yaw;
}

void
vectoangles2(vec3_t value1, vec3_t angles)
{
	float forward;
	float yaw, pitch;

	if ((value1[1] == 0) && (value1[0] == 0))
	{
		yaw = 0;

		if (value1[2] > 0)
		{
			pitch = 90;
		}
		else
		{
			pitch = 270;
		}
	}
	else
	{
		if (value1[0])
		{
			yaw = (atan2(value1[1], value1[0]) * 180 / M_PI);
		}
		else if (value1[1] > 0)
		{
			yaw = 90;
		}
		else
		{
			yaw = 270;
		}

		if (yaw < 0)
		{
			yaw += 360;
		}

		forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch = (atan2(value1[2], forward) * 180 / M_PI);

		if (pitch < 0)
		{
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

// some ctf/zoid helpers
void
stuffcmd(edict_t *ent, char *s)
{
	gi.WriteByte(11);
	gi.WriteString(s);
	gi.unicast(ent, true);
}

/*
 * Returns entities that have 
 * origins within a spherical area
 */
#define LOC_FINDRADIUS_USE_ROGUE_CODE

edict_t *
loc_findradius(edict_t *from, vec3_t org, float rad)
{
#ifndef LOC_FINDRADIUS_USE_ROGUE_CODE // ace - use ctf code
	vec3_t eorg;
	int j;

	if (!from)
	{
		from = g_edicts;
	}
	else
	{
		from++;
	}

	for ( ; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
		{
			continue;
		}

		for (j = 0; j < 3; j++)
		{
			eorg[j] = org[j] - (from->s.origin[j] +
					   (from->mins[j] + from->maxs[j]) * 0.5);
		}

		if (VectorLength(eorg) > rad)
		{
			continue;
		}

		return from;
	}

	return NULL;
#else // ace - use rogue code here if LOC_FINDRADIUS_USE_ROGUE_CODE defined
	return findradius2(from, org, rad);
#endif // LOC_FINDRADIUS_USE_ROGUE_CODE
}

static void
loc_buildboxpoints(vec3_t p[8], vec3_t org, vec3_t mins, vec3_t maxs)
{
	VectorAdd(org, mins, p[0]);
	VectorCopy(p[0], p[1]);
	p[1][0] -= mins[0];
	VectorCopy(p[0], p[2]);
	p[2][1] -= mins[1];
	VectorCopy(p[0], p[3]);
	p[3][0] -= mins[0];
	p[3][1] -= mins[1];
	VectorAdd(org, maxs, p[4]);
	VectorCopy(p[4], p[5]);
	p[5][0] -= maxs[0];
	VectorCopy(p[0], p[6]);
	p[6][1] -= maxs[1];
	VectorCopy(p[0], p[7]);
	p[7][0] -= maxs[0];
	p[7][1] -= maxs[1];
}

qboolean
loc_CanSee(edict_t *targ, edict_t *inflictor)
{
	trace_t trace;
	vec3_t targpoints[8];
	int i;
	vec3_t viewpoint;

	/* bmodels need special checking because their origin is 0,0,0 */
	if (targ->movetype == MOVETYPE_PUSH)
	{
		return false; /* bmodels not supported */
	}

	loc_buildboxpoints(targpoints, targ->s.origin, targ->mins, targ->maxs);

	VectorCopy(inflictor->s.origin, viewpoint);
	viewpoint[2] += inflictor->viewheight;

	for (i = 0; i < 8; i++)
	{
		trace = gi.trace(viewpoint, vec3_origin, vec3_origin,
				targpoints[i], inflictor, MASK_SOLID);

		if (trace.fraction == 1.0)
		{
			return true;
		}
	}

	return false;
}

void Z_RadiusDamageVisible(edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;
		if (!visible(inflictor, ent))
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);
		points = damage - 0.5 * VectorLength (v);
		if (ent == attacker)
			points = points * 0.5;
		if (points > 0)
		{
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);
				T_Damage (ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
			}
		}
	}
}

/*
============
T_RadiusDamagePosition
============
*/
void T_RadiusDamagePosition (vec3_t origin, edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;

	while ((ent = findradius(ent, origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (origin, v, v);
		points = damage - 0.5 * VectorLength (v);
		if (ent == attacker)
			points = points * 0.5;
		if (points > 0)
		{
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, origin, dir);
				T_Damage (ent, inflictor, attacker, dir, origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
			}
		}
	}
}

/*
 *	ace - some misc ai functions, but set to use vectors and no entities whatsoever.
 *
 */

/*
 * returns 1 if the entity is visible
 * to self, even if not infront
 */
qboolean
v_visible(vec3_t start, /* float vh1, */ vec3_t where /*, float vh2 */)
{
	vec3_t spot1;
	vec3_t spot2;
	trace_t trace;

	VectorCopy(start, spot1);
//	spot1[2] += vh1;
	VectorCopy(where, spot2);
//	spot2[2] += vh2;
	trace = gi.trace(spot1, vec3_origin, vec3_origin, spot2, NULL, MASK_OPAQUE);

	if (trace.fraction == 1.0)
	{
		return true;
	}

	return false;
}

/*
 * returns 1 if the other vector is in
 * front (in sight) of self vector using angles for vector
 */
qboolean
v_infront(vec3_t start, vec3_t angles, vec3_t where)
{
	vec3_t vec;
	float dot;
	vec3_t forward;

	AngleVectors(angles, forward, NULL, NULL);

	VectorSubtract(where, start, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.3)
	{
		return true;
	}

	return false;
}

/*
 * =======================================================================
 *
 * Rogue specific level functions.
 *
 * =======================================================================
 */

void fd_secret_move1(edict_t *self);
void fd_secret_move2(edict_t *self);
void fd_secret_move3(edict_t *self);
void fd_secret_move4(edict_t *self);
void fd_secret_move5(edict_t *self);
void fd_secret_move6(edict_t *self);
void fd_secret_done(edict_t *self);
void Move_Calc(edict_t *ent, vec3_t dest, void (*func)(edict_t *));

/*
 * =============================================================================
 *
 * SECRET DOORS
 *
 * =============================================================================
 */

#define SEC_OPEN_ONCE 1     /* stays open */
#define SEC_1ST_LEFT 2      /* 1st move is left of arrow */
#define SEC_1ST_DOWN 4      /* 1st move is down from arrow */
#define SEC_NO_SHOOT 8      /* only opened by trigger */
#define SEC_YES_SHOOT 16    /* shootable even if targeted */
#define SEC_MOVE_RIGHT 32
#define SEC_MOVE_FORWARD 64

void
fd_secret_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	edict_t *ent;

	if (!self)
	{
		return;
	}

	if (self->flags & FL_TEAMSLAVE)
	{
		return;
	}

	/* trigger all paired doors */
	for (ent = self; ent; ent = ent->teamchain)
	{
		Move_Calc(ent, ent->moveinfo.start_origin, fd_secret_move1);
	}
}

void
fd_secret_killed(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, vec3_t point)
{
	if (!self || !inflictor || !attacker)
	{
		return;
	}

	self->health = self->max_health;
	self->takedamage = DAMAGE_NO;

	if (self->flags & FL_TEAMSLAVE && self->teammaster &&
		(self->teammaster->takedamage != DAMAGE_NO))
	{
		fd_secret_killed(self->teammaster, inflictor, attacker, damage, point);
	}
	else
	{
		fd_secret_use(self, inflictor, attacker);
	}
}

void
fd_secret_move1(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->nextthink = level.time + 1.0;
	self->think = fd_secret_move2;
}

/*
 * Start moving sideways w/sound...
 */
void
fd_secret_move2(edict_t *self)
{
	if (!self)
	{
		return;
	}

	Move_Calc(self, self->moveinfo.end_origin, fd_secret_move3);
}

/*
 * Wait here until time to go back...
 */
void
fd_secret_move3(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!(self->spawnflags & SEC_OPEN_ONCE))
	{
		self->nextthink = level.time + self->wait;
		self->think = fd_secret_move4;
	}
}

/*
 * Move backward...
 */
void
fd_secret_move4(edict_t *self)
{
	if (!self)
	{
		return;
	}

	Move_Calc(self, self->moveinfo.start_origin, fd_secret_move5);
}

/*
 * Wait 1 second...
 */
void
fd_secret_move5(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->nextthink = level.time + 1.0;
	self->think = fd_secret_move6;
}

void
fd_secret_move6(edict_t *self)
{
	if (!self)
	{
		return;
	}

	Move_Calc(self, self->move_origin, fd_secret_done);
}

void
fd_secret_done(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->targetname || self->spawnflags & SEC_YES_SHOOT)
	{
		self->health = 1;
		self->takedamage = DAMAGE_YES;
		self->die = fd_secret_killed;
	}
}

void
secret_blocked(edict_t *self, edict_t *other)
{
	if (!self || !other)
	{
		return;
	}

	if (!(self->flags & FL_TEAMSLAVE))
	{
		T_Damage(other, self, self, vec3_origin, other->s.origin,
				vec3_origin, self->dmg, 0, 0, MOD_CRUSH);
	}
}

/*
 * Prints messages
 */
void
secret_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */, csurface_t *surf /* unused */)
{
	if (!self || !other)
	{
		return;
	}

	if (other->health <= 0)
	{
		return;
	}

	if (!(other->client))
	{
		return;
	}

	if (self->monsterinfo.attack_finished > level.time)
	{
		return;
	}

	self->monsterinfo.attack_finished = level.time + 2;

	if (self->message)
	{
		gi.centerprintf(other, self->message);
	}
}

/*
 * QUAKED func_door_secret2 (0 .5 .8) ? open_once 1st_left 1st_down no_shoot always_shoot slide_right slide_forward
 *
 * Basic secret door. Slides back, then to the left. Angle determines direction.
 *
 * FLAGS:
 *  open_once = not implemented yet
 *  1st_left = 1st move is left/right of arrow
 *  1st_down = 1st move is forwards/backwards
 *  no_shoot = not implemented yet
 *  always_shoot = even if targeted, keep shootable
 *  reverse_left = the sideways move will be to right of arrow
 *  reverse_back = the to/fro move will be forward
 *
 * VALUES:
 *  wait = # of seconds before coming back (5 default)
 *  dmg  = damage to inflict when blocked (2 default)
 */
void
SP_func_door_secret2(edict_t *ent)
{
	vec3_t forward, right, up;
	float lrSize = 0, fbSize = 0;

	if (!ent)
	{
		return;
	}

	ent->moveinfo.sound_start = gi.soundindex("doors/dr1_strt.wav");
	ent->moveinfo.sound_middle = gi.soundindex("doors/dr1_mid.wav");
	ent->moveinfo.sound_end = gi.soundindex("doors/dr1_end.wav");

	if (!ent->dmg)
	{
		ent->dmg = 2;
	}

	AngleVectors(ent->s.angles, forward, right, up);
	VectorCopy(ent->s.origin, ent->move_origin);
	VectorCopy(ent->s.angles, ent->move_angles);

	G_SetMovedir(ent->s.angles, ent->movedir);
	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	gi.setmodel(ent, ent->model);

	if ((ent->move_angles[1] == 0) || (ent->move_angles[1] == 180))
	{
		lrSize = ent->size[1];
		fbSize = ent->size[0];
	}
	else if ((ent->move_angles[1] == 90) || (ent->move_angles[1] == 270))
	{
		lrSize = ent->size[0];
		fbSize = ent->size[1];
	}
	else
	{
		gi.dprintf("Secret door not at 0,90,180,270!\n");
	}

	if (ent->spawnflags & SEC_MOVE_FORWARD)
	{
		VectorScale(forward, fbSize, forward);
	}
	else
	{
		VectorScale(forward, fbSize * -1, forward);
	}

	if (ent->spawnflags & SEC_MOVE_RIGHT)
	{
		VectorScale(right, lrSize, right);
	}
	else
	{
		VectorScale(right, lrSize * -1, right);
	}

	if (ent->spawnflags & SEC_1ST_DOWN)
	{
		VectorAdd(ent->s.origin, forward, ent->moveinfo.start_origin);
		VectorAdd(ent->moveinfo.start_origin, right, ent->moveinfo.end_origin);
	}
	else
	{
		VectorAdd(ent->s.origin, right, ent->moveinfo.start_origin);
		VectorAdd(ent->moveinfo.start_origin, forward, ent->moveinfo.end_origin);
	}

	ent->touch = secret_touch;
	ent->blocked = secret_blocked;
	ent->use = fd_secret_use;
	ent->moveinfo.speed = 50;
	ent->moveinfo.accel = 50;
	ent->moveinfo.decel = 50;

	if (!ent->targetname || ent->spawnflags & SEC_YES_SHOOT)
	{
		ent->health = 1;
		ent->max_health = ent->health;
		ent->takedamage = DAMAGE_YES;
		ent->die = fd_secret_killed;
	}

	if (!ent->wait)
	{
		ent->wait = 5; /* 5 seconds before closing */
	}

	gi.linkentity(ent);
}

/* ================================================== */

#define FWALL_START_ON 1

void
force_wall_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->wait)
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_FORCEWALL);
		gi.WritePosition(self->pos1);
		gi.WritePosition(self->pos2);
		gi.WriteByte(self->style);
		gi.multicast(self->offset, MULTICAST_PVS);
	}

	self->think = force_wall_think;
	self->nextthink = level.time + 0.1;
}

void
force_wall_use(edict_t *self, edict_t *other /* activator */,
	   	edict_t *activator /* activator */)
{
	if (!self)
	{
		return;
	}

	if (!self->wait)
	{
		self->wait = 1;
		self->think = NULL;
		self->nextthink = 0;
		self->solid = SOLID_NOT;
		gi.linkentity(self);
	}
	else
	{
		self->wait = 0;
		self->think = force_wall_think;
		self->nextthink = level.time + 0.1;
		self->solid = SOLID_BSP;
		KillBox(self); /* Is this appropriate? */
		gi.linkentity(self);
	}
}

/*
 * QUAKED func_force_wall (1 0 1) ? start_on
 *
 * A vertical particle force wall. Turns on and solid when triggered.
 * If someone is in the force wall when it turns on, they're telefragged.
 *
 *  start_on - forcewall begins activated. triggering will turn it off.
 *  style - color of particles to use.
 *   208: green, 240: red, 241: blue, 224: orange
 */
void
SP_func_force_wall(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	gi.setmodel(ent, ent->model);

	ent->offset[0] = (ent->absmax[0] + ent->absmin[0]) / 2;
	ent->offset[1] = (ent->absmax[1] + ent->absmin[1]) / 2;
	ent->offset[2] = (ent->absmax[2] + ent->absmin[2]) / 2;

	ent->pos1[2] = ent->absmax[2];
	ent->pos2[2] = ent->absmax[2];

	if (ent->size[0] > ent->size[1])
	{
		ent->pos1[0] = ent->absmin[0];
		ent->pos2[0] = ent->absmax[0];
		ent->pos1[1] = ent->offset[1];
		ent->pos2[1] = ent->offset[1];
	}
	else
	{
		ent->pos1[0] = ent->offset[0];
		ent->pos2[0] = ent->offset[0];
		ent->pos1[1] = ent->absmin[1];
		ent->pos2[1] = ent->absmax[1];
	}

	if (!ent->style)
	{
		ent->style = 208;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->wait = 1;

	if (ent->spawnflags & FWALL_START_ON)
	{
		ent->solid = SOLID_BSP;
		ent->think = force_wall_think;
		ent->nextthink = level.time + 0.1;
	}
	else
	{
		ent->solid = SOLID_NOT;
	}

	ent->use = force_wall_use;

	ent->svflags = SVF_NOCLIENT;

	gi.linkentity(ent);
}

/*
 * =======================================================================
 *
 * Rogue specific triggers.
 *
 * =======================================================================
 */

#define TELEPORT_PLAYER_ONLY 1
#define TELEPORT_SILENT 2
#define TELEPORT_CTF_ONLY 4
#define TELEPORT_START_ON 8

extern void TeleportEffect(vec3_t origin);

/*
 * QUAKED info_teleport_destination (.5 .5 .5) (-16 -16 -24) (16 16 32)
 *
 * Destination marker for a teleporter.
 */
void
SP_info_teleport_destination(edict_t *self)
{
}

/*
 * QUAKED trigger_teleport (.5 .5 .5) ? player_only silent ctf_only start_on
 *
 * Any object touching this will be transported to the corresponding
 * info_teleport_destination entity. You must set the "target" field,
 * and create an object with a "targetname" field that matches.
 *
 * If the trigger_teleport has a targetname, it will only teleport
 * entities when it has been fired.
 *
 *  player_only: only players are teleported
 *  silent: <not used right now>
 *  ctf_only: <not used right now>
 *  start_on: when trigger has targetname, start active, deactivate when used.
 */
void
trigger_teleport_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	edict_t *dest;
	int i;

	if (!self || !other)
	{
		return;
	}

	if (!(other->client))
	{
		return;
	}

	if (self->delay)
	{
		return;
	}

	dest = G_Find(NULL, FOFS(targetname), self->target);

	if (!dest)
	{
		gi.dprintf("Teleport Destination not found!\n");
		return;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(other->s.origin);
	gi.multicast(other->s.origin, MULTICAST_PVS);

	/* unlink to make sure it can't possibly interfere with KillBox */
	gi.unlinkentity(other);

	VectorCopy(dest->s.origin, other->s.origin);
	VectorCopy(dest->s.origin, other->s.old_origin);
	other->s.origin[2] += 10;

	/* clear the velocity and hold them in place briefly */
	VectorClear(other->velocity);

	if (other->client)
	{
		other->client->ps.pmove.pm_time = 160 >> 3; /* hold time */
		other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

		/* draw the teleport splash at source and on the player */
		other->s.event = EV_PLAYER_TELEPORT;

		/* set angles */
		for (i = 0; i < 3; i++)
		{
			other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(
					dest->s.angles[i] - other->client->resp.cmd_angles[i]);
		}

		VectorClear(other->client->ps.viewangles);
		VectorClear(other->client->v_angle);
	}

	VectorClear(other->s.angles);

	/* kill anything at the destination */
	KillBox(other);

	gi.linkentity(other);
}

void
trigger_teleport_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	if (self->delay)
	{
		self->delay = 0;
	}
	else
	{
		self->delay = 1;
	}
}

void
SP_trigger_teleport(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->wait)
	{
		self->wait = 0.2;
	}

	self->delay = 0;

	if (self->targetname)
	{
		self->use = trigger_teleport_use;

		if (!(self->spawnflags & TELEPORT_START_ON))
		{
			self->delay = 1;
		}
	}

	self->touch = trigger_teleport_touch;

	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;

	if (!VectorCompare(self->s.angles, vec3_origin))
	{
		G_SetMovedir(self->s.angles, self->movedir);
	}

	gi.setmodel(self, self->model);
	gi.linkentity(self);
}

/*
 * QUAKED trigger_disguise (.5 .5 .5) ? TOGGLE START_ON REMOVE
 *
 * Anything passing through this trigger when it is active will
 * be marked as disguised.
 *
 *  TOGGLE - field is turned off and on when used.
 *  START_ON - field is active when spawned.
 *  REMOVE - field removes the disguise
 */

void
trigger_disguise_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	if (!self || !other)
	{
		return;
	}

	if (other->client)
	{
		if (self->spawnflags & 4)
		{
			other->flags &= ~FL_DISGUISED;
		}
		else
		{
			other->flags |= FL_DISGUISED;
		}
	}
}

void
trigger_disguise_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	if (self->solid == SOLID_NOT)
	{
		self->solid = SOLID_TRIGGER;
	}
	else
	{
		self->solid = SOLID_NOT;
	}

	gi.linkentity(self);
}

void
SP_trigger_disguise(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->spawnflags & 2)
	{
		self->solid = SOLID_TRIGGER;
	}
	else
	{
		self->solid = SOLID_NOT;
	}

	self->touch = trigger_disguise_touch;
	self->use = trigger_disguise_use;
	self->movetype = MOVETYPE_NONE;
	self->svflags = SVF_NOCLIENT;

	gi.setmodel(self, self->model);
	gi.linkentity(self);
}

/*
 * =======================================================================
 *
 * Rogue specific targets.
 *
 * =======================================================================
 */

/*
 * QUAKED target_steam (1 0 0) (-8 -8 -8) (8 8 8)
 * Creates a steam effect (particles w/ velocity in a line).
 *
 *  speed = velocity of particles (default 50)
 *  count = number of particles (default 32)
 *  sounds = color of particles (default 8 for steam)
 *			 the color range is from this color to this color + 6
 *  wait = seconds to run before stopping (overrides default
 *		   value derived from func_timer)
 *
 * best way to use this is to tie it to a func_timer that "pokes"
 * it every second (or however long you set the wait time, above)
 *
 * note that the width of the base is proportional to the speed
 * good colors to use:
 * 6-9 - varying whites (darker to brighter)
 * 224 - sparks
 * 176 - blue water
 * 80  - brown water
 * 208 - slime
 * 232 - blood
 */
void
use_target_steam(edict_t *self, edict_t *other, edict_t *activator /* unused */)
{
	static int nextid;
	vec3_t point;

	if (!self || !other)
	{
		return;
	}

	if (nextid > 20000)
	{
		nextid = nextid % 20000;
	}

	nextid++;

	/* automagically set wait from func_timer unless they set it
	   already, or  default to 1000 if not called by a func_timer */
	if (!self->wait)
	{
		if (other)
		{
			self->wait = other->wait * 1000;
		}
		else
		{
			self->wait = 1000;
		}
	}

	if (self->enemy)
	{
		VectorMA(self->enemy->absmin, 0.5, self->enemy->size, point);
		VectorSubtract(point, self->s.origin, self->movedir);
		VectorNormalize(self->movedir);
	}

	VectorMA(self->s.origin, self->plat2flags * 0.5, self->movedir, point);

	if (self->wait > 100)
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_STEAM);
		gi.WriteShort(nextid);
		gi.WriteByte(self->count);
		gi.WritePosition(self->s.origin);
		gi.WriteDir(self->movedir);
		gi.WriteByte(self->sounds & 0xff);
		gi.WriteShort((short int)(self->plat2flags));
		gi.WriteLong((int)(self->wait));
		gi.multicast(self->s.origin, MULTICAST_PVS);
	}
	else
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_STEAM);
		gi.WriteShort((short int)-1);
		gi.WriteByte(self->count);
		gi.WritePosition(self->s.origin);
		gi.WriteDir(self->movedir);
		gi.WriteByte(self->sounds & 0xff);
		gi.WriteShort((short int)(self->plat2flags));
		gi.multicast(self->s.origin, MULTICAST_PVS);
	}
}

void
target_steam_start(edict_t *self)
{
	edict_t *ent;

	if (!self)
	{
		return;
	}

	self->use = use_target_steam;

	if (self->target)
	{
		ent = G_Find(NULL, FOFS(targetname), self->target);

		if (!ent)
		{
			gi.dprintf("%s at %s: %s is a bad target\n", self->classname,
					vtos(self->s.origin), self->target);
		}

		self->enemy = ent;
	}
	else
	{
		G_SetMovedir(self->s.angles, self->movedir);
	}

	if (!self->count)
	{
		self->count = 32;
	}

	if (!self->plat2flags)
	{
		self->plat2flags = 75;
	}

	if (!self->sounds)
	{
		self->sounds = 8;
	}

	if (self->wait)
	{
		self->wait *= 1000; /* we want it in milliseconds, not seconds */
	}

	/* paranoia is good */
	self->sounds &= 0xff;
	self->count &= 0xff;

	self->svflags = SVF_NOCLIENT;

	gi.linkentity(self);
}

void
SP_target_steam(edict_t *self)
{
	self->plat2flags = self->speed;

	if (self->target)
	{
		self->think = target_steam_start;
		self->nextthink = level.time + 1;
	}
	else
	{
		target_steam_start(self);
	}
}

void
target_anger_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	edict_t *target;
	edict_t *t;

	if (!self)
	{
		return;
	}

	t = NULL;
	target = G_Find(t, FOFS(targetname), self->killtarget);

	if (target && self->target)
	{
		/* Make whatever a "good guy" so the monster will try to kill it! */
		target->monsterinfo.aiflags |= AI_GOOD_GUY;
		target->svflags |= SVF_MONSTER;
		target->health = 300;

		t = NULL;

		while ((t = G_Find(t, FOFS(targetname), self->target)))
		{
			if (t == self)
			{
				gi.dprintf("WARNING: entity used itself.\n");
			}
			else
			{
				if (t->use)
				{
					if (t->health < 0)
					{
						return;
					}

					t->enemy = target;
					t->monsterinfo.aiflags |= AI_TARGET_ANGER;
					FoundTarget(t);
				}
			}

			if (!self->inuse)
			{
				gi.dprintf("entity was removed while using targets\n");
				return;
			}
		}
	}
}

/*
 * QUAKED target_anger (1 0 0) (-8 -8 -8) (8 8 8)
 *
 * This trigger will cause an entity to be angry at another entity when a player touches it.
 * Target the entity you want to anger, and killtarget the entity you want it to be angry at.
 *
 *  target - entity to piss off
 *  killtarget - entity to be pissed off at
 */
void
SP_target_anger(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->target)
	{
		gi.dprintf("target_anger without target!\n");
		G_FreeEdict(self);
		return;
	}

	if (!self->killtarget)
	{
		gi.dprintf("target_anger without killtarget!\n");
		G_FreeEdict(self);
		return;
	}

	self->use = target_anger_use;
	self->svflags = SVF_NOCLIENT;
}

void
target_killplayers_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	int i;
	edict_t *ent, *player;

	if (!self)
	{
		return;
	}

	/* kill the players */
	for (i = 0; i < game.maxclients; i++)
	{
		player = &g_edicts[1 + i];

		if (!player->inuse)
		{
			continue;
		}

		/* nail it */
		T_Damage(player, self, self, vec3_origin, self->s.origin, vec3_origin,
				100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
	}

	/* kill any visible monsters */
	for (ent = g_edicts; ent < &g_edicts[globals.num_edicts]; ent++)
	{
		if (!ent->inuse)
		{
			continue;
		}

		if (ent->health < 1)
		{
			continue;
		}

		if (!ent->takedamage)
		{
			continue;
		}

		for (i = 0; i < game.maxclients; i++)
		{
			player = &g_edicts[1 + i];

			if (!player->inuse)
			{
				continue;
			}

			if (visible(player, ent))
			{
				T_Damage(ent, self, self, vec3_origin, ent->s.origin, vec3_origin,
						ent->health, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
				break;
			}
		}
	}
}

/*
 * QUAKED target_killplayers (1 0 0) (-8 -8 -8) (8 8 8)
 *
 * When triggered, this will kill all the players on the map.
 */
void
SP_target_killplayers(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->use = target_killplayers_use;
	self->svflags = SVF_NOCLIENT;
}

/*
 * QUAKED target_blacklight (1 0 1) (-16 -16 -24) (16 16 24)
 *
 * Pulsing black light with sphere in the center
 */
void
blacklight_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->s.angles[0] = rand() % 360;
	self->s.angles[1] = rand() % 360;
	self->s.angles[2] = rand() % 360;
	self->nextthink = level.time + 0.1;
}

void
SP_target_blacklight(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (deathmatch->value)
	{
		/* auto-remove for deathmatch */
		G_FreeEdict(ent);
		return;
	}

	VectorClear(ent->mins);
	VectorClear(ent->maxs);

	ent->s.effects |= (EF_TRACKERTRAIL | EF_TRACKER);
	ent->think = blacklight_think;
	ent->s.modelindex = gi.modelindex("models/items/spawngro2/tris.md2");
	ent->s.frame = 1;
	ent->nextthink = level.time + 0.1;
	gi.linkentity(ent);
}

/*
 * QUAKED target_orb (1 0 1) (-16 -16 -24) (16 16 24)
 *
 * Translucent pulsing orb with speckles
 */
void
orb_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->s.angles[0] = rand() % 360;
	self->s.angles[1] = rand() % 360;
	self->s.angles[2] = rand() % 360;
	self->nextthink = level.time + 0.1;
}

void
SP_target_orb(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (deathmatch->value)
	{
		/* auto-remove for deathmatch */
		G_FreeEdict(ent);
		return;
	}

	VectorClear(ent->mins);
	VectorClear(ent->maxs);

	ent->think = orb_think;
	ent->nextthink = level.time + 0.1;
	ent->s.modelindex = gi.modelindex("models/items/spawngro2/tris.md2");
	ent->s.frame = 2;
	ent->s.effects |= EF_SPHERETRANS;
	gi.linkentity(ent);
}

/*
	Extra Gib/Blood Code
	-Acid gibs (initially for Gekk)
	-AQ2-style splats (modded to fit Q2)
*/
// ace - external reference to standard q2 pointer
extern int	gibsthisframe;
extern int lastgibframe;
// ace - seperate counter for the AQ2-based splats
int	spraysthisframe;
int lastsprayframe;


// ace - modified "splats" from AQ2 (tweaked for Q2) - START
void
blood_spray_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	dir;

	if (!ent || !other) /* plane is unused, surf can be NULL */
	{
		G_FreeEdict(ent);
		return;
	}

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	if (other == ent->owner)
	{
		return;
	}

	// ace - impact splat
	if (random() < 0.25)
	{
		int		blood_effect;

		blood_effect = TE_BLOOD;
		if (random() < 0.15) // every once in a while...
			blood_effect = TE_MOREBLOOD;

		if (!plane)
			VectorCopy(vec3_origin, dir);
		else
			VectorCopy(plane->normal, dir);

		SpawnDamage(blood_effect, ent->s.origin, dir);
	}

	// ace - just kill it on impact
	VectorClear (ent->velocity); // ace - this should stop it from "double-backing"
//	ent->s.effects = 0;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + FRAMETIME; 
}

void
spray_blood(edict_t *self, vec3_t start, vec3_t dir, int mod)
{
	edict_t *blood;
	int speed;

	if (level.framenum > lastsprayframe)
	{
		spraysthisframe = 0;
		lastsprayframe = level.framenum;
	}

	spraysthisframe++;

	if (spraysthisframe > MAX_SPRAYS_PER_FRAME)
	{
		return;
	}

	switch(mod) 
	{ 
		case MOD_HYPERBLASTER:
		case MOD_BLASTER:
			speed = 2200;
			break;
		case MOD_MACHINEGUN:
			speed = 1500;
			break;  
		case MOD_CHAINGUN:
			speed = 2400;
			break;
		case MOD_BFG_LASER:
			speed = 3300;
			break;
		case MOD_RAILGUN:
			speed = 4000;
			break;
		default:
			speed = 1800;
	}

	blood = G_Spawn();
	VectorNormalize(dir);
	VectorCopy (start, blood->s.origin);
	VectorCopy (dir, blood->movedir);
	vectoangles (dir, blood->s.angles);
	VectorScale (dir, (speed * frandk()) , blood->velocity); // ace - test random vel
	blood->movetype = MOVETYPE_TOSS;
	blood->clipmask = MASK_SOLID; // MASK_SHOT;
	blood->solid = SOLID_NOT;
	blood->s.effects |= EF_GIB; 
	blood->svflags |= SVF_DEADMONSTER; // ace - just incase, don't try and predict against any clients.
	VectorClear (blood->mins);
	VectorClear (blood->maxs);
	// ace - set model to see where/how fast they are *actually* flying
//	blood->s.modelindex = gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
	blood->s.modelindex = gi.modelindex ("sprites/null.sp2");
//	blood->s.renderfx |= RF_SHELL_RED;
	blood->owner = self;
	blood->nextthink = level.time + speed/1000; //3.2;
	blood->touch = blood_spray_touch;
	blood->think = G_FreeEdict;
	blood->classname = "blood_spray";

	gi.linkentity (blood);
}

// zucc based on some code in Action Quake
void
spray_more_blood(edict_t *self, vec3_t start, vec3_t dir)
{
	vec3_t forward;
	int mod = MOD_RAILGUN;

	if (level.framenum > lastsprayframe)
	{
		spraysthisframe = 0;
		lastsprayframe = level.framenum;
	}

	spraysthisframe++;

	if (spraysthisframe > MAX_SPRAYS_PER_FRAME)
	{
		return;
	}


	VectorCopy (dir, forward);
	forward[2] += .03;
	spray_blood(self, start, forward, mod);
        
	VectorCopy (dir, forward);      
	forward[2] -= .03;
	spray_blood(self, start, forward, mod);

	VectorCopy (dir, forward);
	if ( (forward[0] > 0)  && (forward[1] > 0) )
	{
		forward[0] -= .03;
		forward[1] += .03;
	}
	if ( (forward[0] > 0)  && (forward[1] < 0) )
	{
		forward[0] += .03;
		forward[1] += .03;
	}
	if ( (forward[0] < 0)  && (forward[1] > 0) )
	{
		forward[0] -= .03;
		forward[1] -= .03;
	}
	if ( (forward[0] < 0)  && (forward[1] < 0) )
	{
		forward[0] += .03;
		forward[1] -= .03;
	}
	spray_blood(self, start, forward, mod);

	VectorCopy (dir, forward);
	if ( (forward[0] > 0)  && (forward[1] > 0) )
	{
		forward[0] += .03;
		forward[1] -= .03;
	}
	if ( (forward[0] > 0)  && (forward[1] < 0) )
	{
		forward[0] -= .03;
		forward[1] -= .03;
	}
	if ( (forward[0] < 0)  && (forward[1] > 0) )
	{
		forward[0] += .03;
		forward[1] += .03;
	}
	if ( (forward[0] < 0)  && (forward[1] < 0) )
	{
		forward[0] -= .03;
		forward[1] += .03;
	}
	spray_blood(self, start, forward, mod);

	VectorCopy (dir, forward);
	spray_blood(self, start, forward, mod);
}
// ace - modified "splats" from AQ2 (tweaked for Q2) - END

void
ThrowGibACID(edict_t *self, char *gibname, int damage, int type)
{
	edict_t *gib;
	vec3_t vd;
	vec3_t origin;
	vec3_t size;
	float vscale;

	if (!self || !gibname)
	{
		return;
	}

	if (level.framenum > lastgibframe)
	{
		gibsthisframe = 0;
		lastgibframe = level.framenum;
	}

	gibsthisframe++;

	if (gibsthisframe > 20)
	{
		return;
	}

	gib = G_Spawn();

	VectorScale(self->size, 0.5, size);
	VectorAdd(self->absmin, size, origin);
	gib->s.origin[0] = origin[0] + crandom() * size[0];
	gib->s.origin[1] = origin[1] + crandom() * size[1];
	gib->s.origin[2] = origin[2] + crandom() * size[2];

	/* gi.setmodel (gib, gibname); */
	gib->s.modelindex = gi.modelindex(gibname);

	gib->clipmask = MASK_SHOT;
	gib->solid = SOLID_BBOX;

	gib->s.effects |= EF_GREENGIB;
	/* note to self check this */
//	gib->s.renderfx |= RF_FULLBRIGHT;
	gib->flags |= FL_NO_KNOCKBACK;
	gib->takedamage = DAMAGE_YES;
	gib->die = gib_die;
	gib->dmg = 2;

	if (type == GIB_ORGANIC)
	{
		gib->movetype = MOVETYPE_TOSS;
		vscale = 3.0;
	}
	else
	{
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage(damage, vd);
	VectorMA(self->velocity, vscale, vd, gib->velocity);
	ClipGibVelocity(gib);
	gib->avelocity[0] = random() * 600;
	gib->avelocity[1] = random() * 600;
	gib->avelocity[2] = random() * 600;

	gib->think = G_FreeEdict;
	gib->nextthink = level.time + 10 + random() * 10;

	gi.linkentity(gib);
}

void
ThrowHeadACID(edict_t *self, char *gibname, int damage, int type)
{
	vec3_t vd;
	float vscale;

    if (!self || !gibname)
	{
		return;
	}
	
	self->s.skinnum = 0;
	self->s.frame = 0;
	VectorClear(self->mins);
	VectorClear(self->maxs);

	self->s.modelindex2 = 0;
	gi.setmodel(self, gibname);

	self->clipmask = MASK_SHOT;
	self->solid = SOLID_BBOX;

	self->s.effects |= EF_GREENGIB;
	self->s.effects &= ~EF_FLIES;
//	self->s.effects |= RF_FULLBRIGHT;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;
	self->svflags &= ~SVF_MONSTER;
	self->takedamage = DAMAGE_YES;
	self->die = gib_die;
	self->dmg = 2;

	if (type == GIB_ORGANIC)
	{
		self->movetype = MOVETYPE_TOSS;
		vscale = 0.5;
	}
	else
	{
		self->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage(damage, vd);
	VectorMA(self->velocity, vscale, vd, self->velocity);
	ClipGibVelocity(self);

	self->avelocity[YAW] = crandom() * 600;

	self->think = G_FreeEdict;
	self->nextthink = level.time + 10 + random() * 10;

	gi.linkentity(self);
}

// ace - rogue plat2
#define PLAT_LOW_TRIGGER 1
#define PLAT2_TOGGLE 2
#define PLAT2_TOP 4
#define PLAT2_TRIGGER_TOP 8
#define PLAT2_TRIGGER_BOTTOM 16
#define PLAT2_BOX_LIFT 32

#define STATE_TOP 0
#define STATE_BOTTOM 1
#define STATE_UP 2
#define STATE_DOWN 3

#define DOOR_START_OPEN 1
#define DOOR_REVERSE 2
#define DOOR_CRUSHER 4
#define DOOR_NOMONSTER 8
#define DOOR_TOGGLE 32
#define DOOR_X_AXIS 64
#define DOOR_Y_AXIS 128
#define DOOR_INACTIVE 8192

#define AccelerationDistance(target, rate) (target * ((target / rate) + 1) / 2)

#define PLAT2_CALLED 1
#define PLAT2_MOVING 2
#define PLAT2_WAITING 4

#define TRAIN_START_ON 1
#define TRAIN_TOGGLE 2
#define TRAIN_BLOCK_STOPS 4

#define SECRET_ALWAYS_SHOOT 1
#define SECRET_1ST_LEFT 2
#define SECRET_1ST_DOWN 4

void door_secret_move1(edict_t *self);
void door_secret_move2(edict_t *self);
void door_secret_move3(edict_t *self);
void door_secret_move4(edict_t *self);
void door_secret_move5(edict_t *self);
void door_secret_move6(edict_t *self);
void door_secret_done(edict_t *self);

void train_next(edict_t *self);
void door_go_down(edict_t *self);
void plat2_go_down(edict_t *ent);
void plat2_go_up(edict_t *ent);
void plat2_spawn_danger_area(edict_t *ent);
void plat2_kill_danger_area(edict_t *ent);
void Think_AccelMove(edict_t *ent);
void plat_go_down(edict_t *ent);

edict_t *plat_spawn_inside_trigger(edict_t *ent);

void
plat2_spawn_danger_area(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	vec3_t mins, maxs;

	VectorCopy(ent->mins, mins);
	VectorCopy(ent->maxs, maxs);
	maxs[2] = ent->mins[2] + 64;

	SpawnBadArea(mins, maxs, 0, ent);
}

void
plat2_kill_danger_area(edict_t *ent)
{
	edict_t *t;

	if (!ent)
	{
		return;
	}

	t = NULL;

	while ((t = G_Find(t, FOFS(classname), "bad_area")))
	{
		if (t->owner == ent)
		{
			G_FreeEdict(t);
		}
	}
}

void
plat2_hit_top(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_end)
		{
			gi.sound(ent, CHAN_NO_PHS_ADD + CHAN_VOICE, ent->moveinfo.sound_end,
					1, ATTN_STATIC, 0);
		}

		ent->s.sound = 0;
	}

	ent->moveinfo.state = STATE_TOP;

	if (ent->plat2flags & PLAT2_CALLED)
	{
		ent->plat2flags = PLAT2_WAITING;

		if (!(ent->spawnflags & PLAT2_TOGGLE))
		{
			ent->think = plat2_go_down;
			ent->nextthink = level.time + 5.0;
		}

		if (deathmatch->value)
		{
			ent->last_move_time = level.time - 1.0;
		}
		else
		{
			ent->last_move_time = level.time - 2.0;
		}
	}
	else if (!(ent->spawnflags & PLAT2_TOP) &&
			 !(ent->spawnflags & PLAT2_TOGGLE))
	{
		ent->plat2flags = 0;
		ent->think = plat2_go_down;
		ent->nextthink = level.time + 2.0;
		ent->last_move_time = level.time;
	}
	else
	{
		ent->plat2flags = 0;
		ent->last_move_time = level.time;
	}

	G_UseTargets(ent, ent);
}

void
plat2_hit_bottom(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_end)
		{
			gi.sound(ent, CHAN_NO_PHS_ADD + CHAN_VOICE,
					ent->moveinfo.sound_end, 1,
					ATTN_STATIC, 0);
		}

		ent->s.sound = 0;
	}

	ent->moveinfo.state = STATE_BOTTOM;

	if (ent->plat2flags & PLAT2_CALLED)
	{
		ent->plat2flags = PLAT2_WAITING;

		if (!(ent->spawnflags & PLAT2_TOGGLE))
		{
			ent->think = plat2_go_up;
			ent->nextthink = level.time + 5.0;
		}

		if (deathmatch->value)
		{
			ent->last_move_time = level.time - 1.0;
		}
		else
		{
			ent->last_move_time = level.time - 2.0;
		}
	}
	else if ((ent->spawnflags & PLAT2_TOP) && !(ent->spawnflags & PLAT2_TOGGLE))
	{
		ent->plat2flags = 0;
		ent->think = plat2_go_up;
		ent->nextthink = level.time + 2.0;
		ent->last_move_time = level.time;
	}
	else
	{
		ent->plat2flags = 0;
		ent->last_move_time = level.time;
	}

	plat2_kill_danger_area(ent);
	G_UseTargets(ent, ent);
}

void
plat2_go_down(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_start)
		{
			gi.sound(ent, CHAN_NO_PHS_ADD + CHAN_VOICE,
					ent->moveinfo.sound_start, 1,
					ATTN_STATIC, 0);
		}

		ent->s.sound = ent->moveinfo.sound_middle;
	}

	ent->moveinfo.state = STATE_DOWN;
	ent->plat2flags |= PLAT2_MOVING;

	Move_Calc(ent, ent->moveinfo.end_origin, plat2_hit_bottom);
}

void
plat2_go_up(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_start)
		{
			gi.sound(ent, CHAN_NO_PHS_ADD + CHAN_VOICE,
					ent->moveinfo.sound_start, 1,
					ATTN_STATIC, 0);
		}

		ent->s.sound = ent->moveinfo.sound_middle;
	}

	ent->moveinfo.state = STATE_UP;
	ent->plat2flags |= PLAT2_MOVING;

	plat2_spawn_danger_area(ent);

	Move_Calc(ent, ent->moveinfo.start_origin, plat2_hit_top);
}

void
plat2_operate(edict_t *ent, edict_t *other)
{
	int otherState;
	float pauseTime;
	float platCenter;
	edict_t *trigger;

  	if (!ent || !other)
	{
		return;
	}

	trigger = ent;
	ent = ent->enemy; /* now point at the plat, not the trigger */

	if (ent->plat2flags & PLAT2_MOVING)
	{
		return;
	}

	if ((ent->last_move_time + 2) > level.time)
	{
		return;
	}

	platCenter = (trigger->absmin[2] + trigger->absmax[2]) / 2;

	if (ent->moveinfo.state == STATE_TOP)
	{
		otherState = STATE_TOP;

		if (ent->spawnflags & PLAT2_BOX_LIFT)
		{
			if (platCenter > other->s.origin[2])
			{
				otherState = STATE_BOTTOM;
			}
		}
		else
		{
			if (trigger->absmax[2] > other->s.origin[2])
			{
				otherState = STATE_BOTTOM;
			}
		}
	}
	else
	{
		otherState = STATE_BOTTOM;

		if (other->s.origin[2] > platCenter)
		{
			otherState = STATE_TOP;
		}
	}

	ent->plat2flags = PLAT2_MOVING;

	if (deathmatch->value)
	{
		pauseTime = 0.3;
	}
	else
	{
		pauseTime = 0.5;
	}

	if (ent->moveinfo.state != otherState)
	{
		ent->plat2flags |= PLAT2_CALLED;
		pauseTime = 0.1;
	}

	ent->last_move_time = level.time;

	if (ent->moveinfo.state == STATE_BOTTOM)
	{
		ent->think = plat2_go_up;
		ent->nextthink = level.time + pauseTime;
	}
	else
	{
		ent->think = plat2_go_down;
		ent->nextthink = level.time + pauseTime;
	}
}

void
Touch_Plat_Center2(edict_t *ent, edict_t *other,
		cplane_t *plane /* unused */, csurface_t *surf /* unused */)
{
	if (!ent || !other)
	{
		return;
	}

	/* this requires monsters to actively trigger plats, not just step on them. */
	if (other->health <= 0)
	{
		return;
	}

	/* don't let non-monsters activate plat2s */
	if ((!(other->svflags & SVF_MONSTER)) && (!other->client))
	{
		return;
	}

	plat2_operate(ent, other);
}

void
plat2_blocked(edict_t *self, edict_t *other)
{
	if (!self || !other)
	{
		return;
	}

	if (!(other->svflags & SVF_MONSTER) && (!other->client))
	{
		/* give it a chance to go away on it's own terms (like gibs) */
		T_Damage(other, self, self, vec3_origin, other->s.origin,
				vec3_origin, 100000, 1, 0, MOD_CRUSH);

		/* if it's still there, nuke it */
		if (other && other->inuse)
		{
			BecomeExplosion1(other);
		}

		return;
	}

	/* gib dead things */
	if (other->health < 1)
	{
		T_Damage(other, self, self, vec3_origin, other->s.origin,
				vec3_origin, 100, 1, 0, MOD_CRUSH);
	}

	T_Damage(other, self, self, vec3_origin, other->s.origin,
			vec3_origin, self->dmg, 1, 0, MOD_CRUSH);

	if (self->moveinfo.state == STATE_UP)
	{
		plat2_go_down(self);
	}
	else if (self->moveinfo.state == STATE_DOWN)
	{
		plat2_go_up(self);
	}
}

void
Use_Plat2(edict_t *ent, edict_t *other /* unused */,
	   	edict_t *activator)
{
	edict_t *trigger;
	int i;

	if (!ent || !activator)
	{
		return;
	}

	if (ent->moveinfo.state > STATE_BOTTOM)
	{
		return;
	}

	if ((ent->last_move_time + 2) > level.time)
	{
		return;
	}

	for (i = 1, trigger = g_edicts + 1; i < globals.num_edicts; i++, trigger++)
	{
		if (!trigger->inuse)
		{
			continue;
		}

		if (trigger->touch == Touch_Plat_Center2)
		{
			if (trigger->enemy == ent)
			{
				plat2_operate(trigger, activator);
				return;
			}
		}
	}
}

void
plat2_activate(edict_t *ent, edict_t *other /* unused */,
	   	edict_t *activator /* unused */)
{
	edict_t *trigger;

	if (!ent)
	{
		return;
	}

	ent->use = Use_Plat2;
	trigger = plat_spawn_inside_trigger(ent); /* the "start moving" trigger */

	trigger->maxs[0] += 10;
	trigger->maxs[1] += 10;
	trigger->mins[0] -= 10;
	trigger->mins[1] -= 10;

	gi.linkentity(trigger);

	trigger->touch = Touch_Plat_Center2; /* Override trigger touch function */

	plat2_go_down(ent);
}

/* QUAKED func_plat2 (0 .5 .8) ? PLAT_LOW_TRIGGER PLAT2_TOGGLE PLAT2_TOP PLAT2_TRIGGER_TOP PLAT2_TRIGGER_BOTTOM BOX_LIFT
 * speed default 150
 *
 * PLAT_LOW_TRIGGER - creates a short trigger field at the bottom
 * PLAT2_TOGGLE - plat will not return to default position.
 * PLAT2_TOP - plat's default position will the the top.
 * PLAT2_TRIGGER_TOP - plat will trigger it's targets each time it hits top
 * PLAT2_TRIGGER_BOTTOM - plat will trigger it's targets each time it hits bottom
 * BOX_LIFT - this indicates that the lift is a box, rather than just a platform
 *
 * Plats are always drawn in the extended position, so they will light correctly.
 *
 * If the plat is the target of another trigger or button, it will start out
 * disabled in the extended position until it is trigger, when it will lower
 * and become a normal plat.
 *
 * "speed"	overrides default 200.
 * "accel" overrides default 500
 * "lip"	no default
 *
 * If the "height" key is set, that will determine the amount the plat moves,
 *  instead of being implicitly determoveinfoned by the model's height.
 *
 */
void
SP_func_plat2(edict_t *ent)
{
	edict_t *trigger;

	if (!ent)
	{
		return;
	}

	VectorClear(ent->s.angles);
	ent->solid = SOLID_BSP;
	ent->movetype = MOVETYPE_PUSH;

	gi.setmodel(ent, ent->model);

	ent->blocked = plat2_blocked;

	if (!ent->speed)
	{
		ent->speed = 20;
	}
	else
	{
		ent->speed *= 0.1;
	}

	if (!ent->accel)
	{
		ent->accel = 5;
	}
	else
	{
		ent->accel *= 0.1;
	}

	if (!ent->decel)
	{
		ent->decel = 5;
	}
	else
	{
		ent->decel *= 0.1;
	}

	if (deathmatch->value)
	{
		ent->speed *= 2;
		ent->accel *= 2;
		ent->decel *= 2;
	}

	/* Added to kill things it's being blocked by */
	if (!ent->dmg)
	{
		ent->dmg = 2;
	}

	/* pos1 is the top position, pos2 is the bottom */
	VectorCopy(ent->s.origin, ent->pos1);
	VectorCopy(ent->s.origin, ent->pos2);

	if (st.height)
	{
		ent->pos2[2] -= (st.height - st.lip);
	}
	else
	{
		ent->pos2[2] -= (ent->maxs[2] - ent->mins[2]) - st.lip;
	}

	ent->moveinfo.state = STATE_TOP;

	if (ent->targetname)
	{
		ent->use = plat2_activate;
	}
	else
	{
		ent->use = Use_Plat2;

		trigger = plat_spawn_inside_trigger(ent); /* the "start moving" trigger */

		trigger->maxs[0] += 10;
		trigger->maxs[1] += 10;
		trigger->mins[0] -= 10;
		trigger->mins[1] -= 10;

		gi.linkentity(trigger);
		trigger->touch = Touch_Plat_Center2; /* Override trigger touch function */

		if (!(ent->spawnflags & PLAT2_TOP))
		{
			VectorCopy(ent->pos2, ent->s.origin);
			ent->moveinfo.state = STATE_BOTTOM;
		}
	}

	gi.linkentity(ent);

	ent->moveinfo.speed = ent->speed;
	ent->moveinfo.accel = ent->accel;
	ent->moveinfo.decel = ent->decel;
	ent->moveinfo.wait = ent->wait;
	VectorCopy(ent->pos1, ent->moveinfo.start_origin);
	VectorCopy(ent->s.angles, ent->moveinfo.start_angles);
	VectorCopy(ent->pos2, ent->moveinfo.end_origin);
	VectorCopy(ent->s.angles, ent->moveinfo.end_angles);

	ent->moveinfo.sound_start = gi.soundindex("plats/pt1_strt.wav");
	ent->moveinfo.sound_middle = gi.soundindex("plats/pt1_mid.wav");
	ent->moveinfo.sound_end = gi.soundindex("plats/pt1_end.wav");
}