/* =======================================================================
 *
 * Additional Item Code
 *
 * =======================================================================
 */

#include "../header/local.h"

extern qboolean is_quad;
extern byte is_silenced;

void playQuadSound(edict_t *ent);
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

/*
=================
Plasma Shield
=================
*/

void PlasmaShield_die (edict_t *self)
{
	if (deathmatch->value)
	{
	  gi.sound(self, CHAN_VOICE, gi.soundindex("items/plasmashield/psdie.wav"), 1, ATTN_NORM, 0);
	}
  G_FreeEdict(self);
}


void PlasmaShield_killed (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
  PlasmaShield_die(self);
}


void Use_PlasmaShield (edict_t *ent, gitem_t *item)
{
  int ammoIdx = ITEM_INDEX(item);
	edict_t	*PlasmaShield;
  vec3_t forward, right, up, frontbottomleft, backtopright;

  if(!ent->client->pers.inventory[ammoIdx])
  {
    return;
  }

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ammoIdx]--;

	if (deathmatch->value)
	{
	  gi.sound(ent, CHAN_VOICE, gi.soundindex("items/plasmashield/psfire.wav"), 1, ATTN_NORM, 0);
	}

	PlasmaShield = G_Spawn();
	PlasmaShield->classname = "PlasmaShield";
	PlasmaShield->movetype = MOVETYPE_PUSH;
	PlasmaShield->solid = SOLID_BBOX;
	PlasmaShield->s.modelindex = gi.modelindex("sprites/plasmashield.sp2");
  PlasmaShield->s.effects |= EF_POWERSCREEN;
  PlasmaShield->s.sound = gi.soundindex ("items/plasmashield/psactive.wav");

	AngleVectors (ent->client->v_angle, forward, right, up);
	vectoangles (forward, PlasmaShield->s.angles);

	VectorMA (ent->s.origin, 50, forward, PlasmaShield->s.origin);

  VectorScale(forward, 10, frontbottomleft);
  VectorMA(frontbottomleft, -30, right, frontbottomleft);
  VectorMA(frontbottomleft, -30, up, frontbottomleft);

  VectorScale(forward, 5, backtopright);
  VectorMA(backtopright, 30, right, backtopright);
  VectorMA(backtopright, 50, up, backtopright);

  ClearBounds (PlasmaShield->mins, PlasmaShield->maxs);

  AddPointToBounds (frontbottomleft, PlasmaShield->mins, PlasmaShield->maxs);
  AddPointToBounds (backtopright, PlasmaShield->mins, PlasmaShield->maxs);

  PlasmaShield->health = PlasmaShield->max_health = 4000;
	PlasmaShield->die = PlasmaShield_killed;
	PlasmaShield->takedamage = DAMAGE_YES;

  PlasmaShield->think = PlasmaShield_die;
  PlasmaShield->nextthink = level.time + 10;

	gi.linkentity (PlasmaShield);
}

/*------------------------------------------------------------------------*/
/* TECH																	  */
/*------------------------------------------------------------------------*/
// "CTF" Techs Names
static char *tnames[] = {
	"item_tech1", "item_tech2", "item_tech3", "item_tech4",
	NULL
};

void
HasTech(edict_t *who)
{
	if (level.time - who->client->lasttechmsg > 2)
	{
		gi.centerprintf(who, "You already have a TECH powerup.");
		who->client->lasttechmsg = level.time;
	}
}

gitem_t *
What_Tech(edict_t *ent)
{
	gitem_t *tech;
	int i;

	i = 0;

	while (tnames[i])
	{
		if (((tech = FindItemByClassname(tnames[i])) != NULL) &&
			ent->client->pers.inventory[ITEM_INDEX(tech)])
		{
			return tech;
		}

		i++;
	}

	return NULL;
}

qboolean
Pickup_Tech(edict_t *ent, edict_t *other)
{
	gitem_t *tech;
	int i;

	i = 0;

	while (tnames[i])
	{
		if (((tech = FindItemByClassname(tnames[i])) != NULL) &&
			other->client->pers.inventory[ITEM_INDEX(tech)])
		{
			HasTech(other);
			return false; /* has this one */
		}

		i++;
	}

	/* client only gets one tech */
	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
	other->client->regentime = level.time;
	return true;
}

static void SpawnTech(gitem_t *item, edict_t *spot);

static edict_t *
FindTechSpawn(void)
{
	edict_t *spot = NULL;
	int i = rand() % 16;

	while (i--)
	{
		spot = G_Find(spot, FOFS(classname), "info_player_deathmatch");
	}

	if (!spot)
	{
		spot = G_Find(spot, FOFS(classname), "info_player_deathmatch");
	}

	return spot;
}

static void
TechThink(edict_t *tech)
{
	edict_t *spot;

	if ((spot = FindTechSpawn()) != NULL)
	{
		SpawnTech(tech->item, spot);
		G_FreeEdict(tech);
	}
	else
	{
		tech->nextthink = level.time + TECH_TIMEOUT;
		tech->think = TechThink;
	}
}

void
Drop_Tech(edict_t *ent, gitem_t *item)
{
	edict_t *tech;

	tech = Drop_Item(ent, item);
	tech->nextthink = level.time + TECH_TIMEOUT;
	tech->think = TechThink;
	ent->client->pers.inventory[ITEM_INDEX(item)] = 0;
}

void
DeadDropTech(edict_t *ent)
{
	gitem_t *tech;
	edict_t *dropped;
	int i;

	i = 0;

	while (tnames[i])
	{
		if (((tech = FindItemByClassname(tnames[i])) != NULL) &&
			ent->client->pers.inventory[ITEM_INDEX(tech)])
		{
			dropped = Drop_Item(ent, tech);

			/* hack the velocity to make it bounce random */
			dropped->velocity[0] = (rand() % 600) - 300;
			dropped->velocity[1] = (rand() % 600) - 300;
			dropped->nextthink = level.time + TECH_TIMEOUT;
			dropped->think = TechThink;
			dropped->owner = NULL;
			ent->client->pers.inventory[ITEM_INDEX(tech)] = 0;
		}

		i++;
	}
}

static void
SpawnTech(gitem_t *item, edict_t *spot)
{
	edict_t *ent;
	vec3_t forward, right;
	vec3_t angles;

	ent = G_Spawn();

	ent->classname = item->classname;
	ent->item = item;
	ent->spawnflags = DROPPED_ITEM;
	ent->s.effects = item->world_model_flags;
	ent->s.renderfx = RF_GLOW;
	VectorSet(ent->mins, -15, -15, -15);
	VectorSet(ent->maxs, 15, 15, 15);
	gi.setmodel(ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;
	ent->owner = ent;

	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;

	AngleVectors(angles, forward, right, NULL);
	VectorCopy(spot->s.origin, ent->s.origin);
	ent->s.origin[2] += 16;
	VectorScale(forward, 100, ent->velocity);
	ent->velocity[2] = 300;

	ent->nextthink = level.time + TECH_TIMEOUT;
	ent->think = TechThink;

	gi.linkentity(ent);
}

static void
SpawnTechs(edict_t *ent)
{
	gitem_t *tech;
	edict_t *spot;
	int i;

	i = 0;

	while (tnames[i])
	{
		if (((tech = FindItemByClassname(tnames[i])) != NULL) &&
			((spot = FindTechSpawn()) != NULL))
		{
			SpawnTech(tech, spot);
		}

		i++;
	}

	if (ent)
	{
		G_FreeEdict(ent);
	}
}

/* 
 * frees the passed edict! 
 */
void
RespawnTech(edict_t *ent)
{
	edict_t *spot;

	if ((spot = FindTechSpawn()) != NULL)
	{
		SpawnTech(ent->item, spot);
	}

	G_FreeEdict(ent);
}

void
SetupTechSpawn(void)
{
	edict_t *ent;

	if (((int)dmflags->value & DF_NO_TECH))
	{
		return;
	}

	ent = G_Spawn();
	ent->nextthink = level.time + 2;
	ent->think = SpawnTechs;
}

void
ResetTech(void)
{
	edict_t *ent;
	int i;

	for (ent = g_edicts + 1, i = 1; i < globals.num_edicts; i++, ent++)
	{
		if (ent->inuse)
		{
			if (ent->item && (ent->item->flags & IT_TECH))
			{
				G_FreeEdict(ent);
			}
		}
	}

	SpawnTechs(NULL);
}

int
ApplyResistance(edict_t *ent, int dmg)
{
	static gitem_t *tech = NULL;
	float volume = 1.0;

	if (ent->client && ent->client->silencer_shots)
	{
		volume = 0.2;
	}

	if (!tech)
	{
		tech = FindItemByClassname("item_tech1");
	}

	if (dmg && tech && ent->client &&
		ent->client->pers.inventory[ITEM_INDEX(tech)])
	{
		/* make noise */
		gi.sound(ent, CHAN_VOICE, gi.soundindex(
						"tech/tech1.wav"), volume, ATTN_NORM, 0);
		return dmg / 2;
	}

	return dmg;
}

qboolean
HasStrength(edict_t *ent)
{
	static gitem_t *tech;

	tech = FindItemByClassname("item_tech2");

	if (tech && ent->client &&
		ent->client->pers.inventory[ITEM_INDEX(tech)])
	{
		return true;
	}

	return false;
}

int
ApplyStrength(edict_t *ent, int dmg)
{
	static gitem_t *tech = NULL;

	if (!tech)
	{
		tech = FindItemByClassname("item_tech2");
	}

	if (dmg && tech && ent->client &&
		ent->client->pers.inventory[ITEM_INDEX(tech)])
	{
		return dmg * 2;
	}

	return dmg;
}

qboolean
ApplyStrengthSound(edict_t *ent)
{
	static gitem_t *tech = NULL;
	float volume = 1.0;

	if (ent->client && ent->client->silencer_shots)
	{
		volume = 0.2;
	}

	if (!tech)
	{
		tech = FindItemByClassname("item_tech2");
	}

	if (tech && ent->client &&
		ent->client->pers.inventory[ITEM_INDEX(tech)])
	{
		if (ent->client->techsndtime < level.time)
		{
			ent->client->techsndtime = level.time + 1;

			if (ent->client->quad_framenum > level.framenum)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex(
								"tech/tech2x.wav"), volume, ATTN_NORM, 0);
			}
			else
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex(
								"tech/tech2.wav"), volume, ATTN_NORM, 0);
			}
		}

		return true;
	}

	return false;
}

qboolean
ApplyHaste(edict_t *ent)
{
	static gitem_t *tech = NULL;

	if (!tech)
	{
		tech = FindItemByClassname("item_tech3");
	}

	if (tech && ent->client &&
		ent->client->pers.inventory[ITEM_INDEX(tech)])
	{
		return true;
	}

	return false;
}

void
ApplyHasteSound(edict_t *ent)
{
	static gitem_t *tech = NULL;
	float volume = 1.0;

	if (ent->client && ent->client->silencer_shots)
	{
		volume = 0.2;
	}

	if (!tech)
	{
		tech = FindItemByClassname("item_tech3");
	}

	if (tech && ent->client &&
		ent->client->pers.inventory[ITEM_INDEX(tech)] &&
		(ent->client->techsndtime < level.time))
	{
		ent->client->techsndtime = level.time + 1;
		gi.sound(ent, CHAN_VOICE, gi.soundindex(
						"tech/tech3.wav"), volume, ATTN_NORM, 0);
	}
}

void
ApplyRegeneration(edict_t *ent)
{
	static gitem_t *tech = NULL;
	qboolean noise = false;
	gclient_t *client;
	int index;
	float volume = 1.0;

	client = ent->client;

	if (!client)
	{
		return;
	}

	if (ent->client->silencer_shots)
	{
		volume = 0.2;
	}

	if (!tech)
	{
		tech = FindItemByClassname("item_tech4");
	}

	if (tech && client->pers.inventory[ITEM_INDEX(tech)])
	{
		if (client->regentime < level.time)
		{
			client->regentime = level.time;

			if (ent->health < 150)
			{
				ent->health += 5;

				if (ent->health > 150)
				{
					ent->health = 150;
				}

				client->regentime += 0.5;
				noise = true;
			}

			index = ArmorIndex(ent);

			if (index && (client->pers.inventory[index] < 150))
			{
				client->pers.inventory[index] += 5;

				if (client->pers.inventory[index] > 150)
				{
					client->pers.inventory[index] = 150;
				}

				client->regentime += 0.5;
				noise = true;
			}
		}

		if (noise && (ent->client->techsndtime < level.time))
		{
			ent->client->techsndtime = level.time + 1;
			gi.sound(ent, CHAN_VOICE, gi.soundindex(
							"tech/tech4.wav"), volume, ATTN_NORM, 0);
		}
	}
}

qboolean
HasRegeneration(edict_t *ent)
{
	static gitem_t *tech = NULL;

	if (!tech)
	{
		tech = FindItemByClassname("item_tech4");
	}

	if (tech && ent->client &&
		ent->client->pers.inventory[ITEM_INDEX(tech)])
	{
		return true;
	}

	return false;
}

// ace - Rogue item spawning
void
SP_xatrix_item(edict_t *self)
{
	gitem_t *item;
	int i;
	char *spawnClass = NULL;

	if (!self)
	{
		return;
	}

	if (!self->classname)
	{
		return;
	}

	if (!strcmp(self->classname, "ammo_magslug"))
	{
		spawnClass = "ammo_flechettes";
	}
	else if (!strcmp(self->classname, "ammo_trap"))
	{
		spawnClass = "weapon_proxlauncher";
	}
	else if (!strcmp(self->classname, "item_quadfire"))
	{
		float chance;

		chance = random();

		if (chance < 0.2)
		{
			spawnClass = "item_sphere_hunter";
		}
		else if (chance < 0.6)
		{
			spawnClass = "item_sphere_vengeance";
		}
		else
		{
			spawnClass = "item_sphere_defender";
		}
	}
	else if (!strcmp(self->classname, "weapon_boomer"))
	{
		spawnClass = "weapon_etf_rifle";
	}
	else if (!strcmp(self->classname, "weapon_phalanx"))
	{
		spawnClass = "weapon_plasmabeam";
	}

	/* check item spawn functions */
	for (i = 0, item = itemlist; i < game.num_items; i++, item++)
	{
		if (!item->classname)
		{
			continue;
		}

		if (!strcmp(item->classname, spawnClass))
		{
			/* found it */
			SpawnItem(self, item);
			return;
		}
	}
 }