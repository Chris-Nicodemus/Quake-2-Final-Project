/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "g_local.h"
#include "m_player.h"


char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		//gi.cprintf(ent, PRINT_HIGH, "Giving Weapons");
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = FindItem("Jacket Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
}


int PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void Cmd_PlayerList_f(edict_t *ent)
{
	int i;
	char st[80];
	char text[1400];
	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		sprintf(st, "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}

int partyIndex = 1;
int monsterIndex = 1;
qboolean shopOpen = false;

//AoE attacks
int goblinBomb = 1;		//triggers at 4
int drakeFire = 1;		//triggers at 3
int dragonFire = 1;		//triggers at 2

qboolean goblin = false;	//true if there is a goblin among the enemies
qboolean drake = false;		//true if there is a drake
qboolean dragon = false;	//true if there is a dragon

qboolean battleLost = false; //true if the monsters kill the hero
//if battle ends or you run from battle, enemy stats and values are cleaned
void Cmd_CleanValues_f(edict_t* ent)
{
	gclient_t* client;

	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	client->heroTempHealth = 0;
	client->rangerTempHealth = 0;
	client->wizardTempHealth = 0;
	client->warriorTempHealth = 0;
	client->heroBuffer = false;
	client->warriorTaunt = false;

	edict_t* enemy;
	enemy = client->enemy;

	enemy->enemy1Health = 0;
	enemy->enemy1Type = 0;
	enemy->enemy2Health = 0;
	enemy->enemy2Type = 0;
	enemy->enemy3Health = 0;
	enemy->enemy3Type = 0;
	enemy->demonShroud1 = false;
	enemy->demonShroud2 = false;
	enemy->demonShroud3 = false;
	enemy->enemy1Weak = false;
	enemy->enemy2Weak = false;
	enemy->enemy3Weak = false;
	//client->enemy = NULL;

	goblinBomb = 1;
	drakeFire = 1;
	dragonFire = 1;

	goblin = false;
	drake = false;
	dragon = false;

	partyIndex = 1;
	monsterIndex = 1;
}

void Cmd_Shop_f(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	if (!client->inCombat)
	{
		if (!shopOpen)
		{
			shopOpen = true;
			gi.cprintf(ent, 50, "Shop Opened");
		}
		else
		{
			shopOpen = false;
			gi.cprintf(ent, 50, "Shop Closed");
		}
	}
	else
	{
		gi.centerprintf(ent, "Cannot shop while in combat.");
	}
}

extern char* info;
extern qboolean infoSet;

extern char* guide;
extern qboolean guideSet;

extern qboolean firstCombat;
//hero stuff
qboolean smite;
qboolean buffer;

//warrior stuff
int shieldBash = 0;

extern int numEnemies;

//Finds next turn for player and updates enemy start of turn effects
void PartyNextTurn(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}

	edict_t* enemy = client->enemy;

	switch (partyIndex)
	{
	case CLASS_RANGER:
		if (client->rangerDead)
		{
			partyIndex++;
		}
		else
		{
			break;
		}
	case CLASS_WIZARD:
		if (client->wizardDead)
		{
			partyIndex++;
		}
		else
		{
			break;
		}
	case CLASS_WARRIOR:
		if (client->warriorDead)
		{
			client->turn = false;
			partyIndex = 1;

			//just to be safe
			monsterIndex = 1;

			//give the demons their shrouds back
			if (enemy->enemy1Type == MONSTER_DEMON)
			{
				enemy->demonShroud1 = true;
			}
			if (enemy->enemy2Type == MONSTER_DEMON)
			{
				enemy->demonShroud2 = true;
			}
			if (enemy->enemy3Type == MONSTER_DEMON)
			{
				enemy->demonShroud3 = true;
			}
		}
		break;
	case 5:
		client->turn = false;
		partyIndex = 1;
		if (enemy->enemy1Type == MONSTER_DEMON)
		{
			enemy->demonShroud1 = true;
		}
		if (enemy->enemy2Type == MONSTER_DEMON)
		{
			enemy->demonShroud2 = true;
		}
		if (enemy->enemy3Type == MONSTER_DEMON)
		{
			enemy->demonShroud3 = true;
		}
	}
}


//Call CombatScreen outside of this function to get things to show. This allows you to override the info and guide calls so you can put your own text 
//Party member attack damage calculation
void PartyAttack(edict_t* ent, int target, int damage, qboolean weapon, qboolean base)
{
	//gi.cprintf(ent, 1, "Party Attack is getting called\n");
	int deal = 0;

	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}

	//declare enemy
	edict_t* enemy;
	enemy = client->enemy;

	//base attack random damage
	if (base)
	{
		//range
		deal = (int)(crandom() * 6);

		//invert negatives
		if (deal < 0)
		{
			deal = deal * -1;
		}
		//increase min values
		deal += 5;
	}
	//increase damage by weapon value if weapon is true
	if(weapon)
	{
		switch (partyIndex)
		{
		case CLASS_HERO:
			deal += (client->heroWeapon * 3);
			break;
		case CLASS_RANGER:
			deal += (client->rangerWeapon * 3);
			break;
		case CLASS_WIZARD:
			deal += (client->wizardWeapon * 3);
			break;
		case CLASS_WARRIOR:
			deal += (client->warriorWeapon * 3);
			break;
		}
	}

	//adds damage bonus if skill
	deal += damage;

	switch (target)
	{
	case 1:
		if (enemy->demonShroud1 && !smite)
		{
			enemy->demonShroud1 = false;
			info = "The demons\'s shroud blocked the attack!";
			infoSet = true;

			guide = "The demon\'s shroud blocked your attack!";
			guideSet = true;
			return;
		}
		else if (enemy->demonShroud1 && smite)
		{
			//damage
			enemy->demonShroud1 = false;
			enemy->enemy1Health = enemy->enemy1Health - deal;

			//info card
			info = "The demons\'s shroud was destroyed by your smite!";
			infoSet = true;

			//guide card
			char* dealStr = malloc(2);
			sprintf(dealStr, "%i", deal);
			int dLen = strlen(dealStr);

			char* begin = "Your attack dealt ";
			int beginLen = strlen(begin);

			char* end = " damage!";
			int endLen = strlen(end);
			guide = malloc(beginLen + dLen + endLen);

			memcpy(guide, begin, beginLen);
			memcpy(guide + beginLen, dealStr ,dLen);
			memcpy(guide + beginLen + dLen, end ,endLen);
			guideSet = true;
		}

		//normal smite
		if (!enemy->demonShroud1 && smite)
		{
			enemy->enemy1Health = enemy->enemy1Health - deal;

			info = "You used smite on an enemy!";
			infoSet = true;

			//guide card
			char* dealStr = malloc(2);
			sprintf(dealStr, "%i", deal);
			int dLen = strlen(dealStr);

			char* begin = "Your attack dealt ";
			int beginLen = strlen(begin);

			char* end = " damage!";
			int endLen = strlen(end);
			guide = malloc(beginLen + dLen + endLen);

			memcpy(guide, begin, beginLen);
			memcpy(guide + beginLen, dealStr, dLen);
			memcpy(guide + beginLen + dLen, end, endLen);
			guideSet = true;
		}

		//normal attack
		if (!enemy->demonShroud1 && !smite)
		{
			enemy->enemy1Health = enemy->enemy1Health - deal;

			info = "You used attack on an enemy!";
			infoSet = true;

			//guide card
			char* dealStr = malloc(2);
			sprintf(dealStr, "%i", deal);
			int dLen = strlen(dealStr);

			char* begin = "Your attack dealt ";
			int beginLen = strlen(begin);

			char* end = " damage!";
			int endLen = strlen(end);
			guide = malloc(beginLen + dLen + endLen);

			memcpy(guide, begin, beginLen);
			memcpy(guide + beginLen, dealStr, dLen);
			memcpy(guide + beginLen + dLen, end, endLen);
			guideSet = true;
		}

		//check if enemy dead, and shift enemies down if so
		if (enemy->enemy1Health <= 0)
		{
			info = "You have defeated an enemy!";
			infoSet = true;

			numEnemies--;

			//shift enemies down
			enemy->enemy1Health = enemy->enemy2Health;
			enemy->enemy1Type = enemy->enemy2Type;

			enemy->enemy2Health = enemy->enemy3Health;
			enemy->enemy2Type = enemy->enemy3Type;
		}
		break;
	case 2:
		if (enemy->demonShroud2 && !smite)
		{
			enemy->demonShroud2 = false;
			info = "The demons\'s shroud blocked the attack!";
			infoSet = true;

			guide = "Your attack did no damage!";
			guideSet = true;
			return;
		}
		else if (enemy->demonShroud2 && smite)
		{
			//damage
			enemy->demonShroud2 = false;
			enemy->enemy2Health = enemy->enemy2Health - deal;

			//info card
			info = "The demons\'s shroud was destroyed by your smite!";
			infoSet = true;

			//guide card
			char* dealStr = malloc(2);
			sprintf(dealStr, "%i", deal);
			int dLen = strlen(dealStr);

			char* begin = "Your attack dealt ";
			int beginLen = strlen(begin);

			char* end = " damage!";
			int endLen = strlen(end);
			guide = malloc(beginLen + dLen + endLen);

			memcpy(guide, begin, beginLen);
			memcpy(guide + beginLen, dealStr, dLen);
			memcpy(guide + beginLen + dLen, end, endLen);
			guideSet = true;
		}

		//normal smite
		if (!enemy->demonShroud2 && smite)
		{
			enemy->enemy2Health = enemy->enemy2Health - deal;

			info = "You used smite on an enemy!";
			infoSet = true;

			//guide card
			char* dealStr = malloc(2);
			sprintf(dealStr, "%i", deal);
			int dLen = strlen(dealStr);

			char* begin = "Your attack dealt ";
			int beginLen = strlen(begin);

			char* end = " damage!";
			int endLen = strlen(end);
			guide = malloc(beginLen + dLen + endLen);

			memcpy(guide, begin, beginLen);
			memcpy(guide + beginLen, dealStr, dLen);
			memcpy(guide + beginLen + dLen, end, endLen);
			guideSet = true;
		}

		//normal attack
		if (!enemy->demonShroud2 && !smite)
		{
			enemy->enemy2Health = enemy->enemy2Health - deal;

			info = "You used attack on an enemy!";
			infoSet = true;

			//guide card
			char* dealStr = malloc(2);
			sprintf(dealStr, "%i", deal);
			int dLen = strlen(dealStr);

			char* begin = "Your attack dealt ";
			int beginLen = strlen(begin);

			char* end = " damage!";
			int endLen = strlen(end);
			guide = malloc(beginLen + dLen + endLen);

			memcpy(guide, begin, beginLen);
			memcpy(guide + beginLen, dealStr, dLen);
			memcpy(guide + beginLen + dLen, end, endLen);
			guideSet = true;
		}

		//check if enemy dead, and shift enemies down if so
		if (enemy->enemy2Health <= 0)
		{
			info = "You have defeated an enemy!";
			infoSet = true;

			numEnemies--;

			//shift enemies down
			enemy->enemy2Health = enemy->enemy3Health;
			enemy->enemy2Type = enemy->enemy3Type;

			enemy->enemy3Health = 0;
			enemy->enemy3Type = MONSTER_NONE;
		}
		break;
	case 3:
		if (enemy->demonShroud3 && !smite)
		{
			enemy->demonShroud3 = false;
			info = "The demons\'s shroud blocked the attack!";
			infoSet = true;

			guide = "Your attack did no damage!";
			guideSet = true;
			return;
		}
		else if (enemy->demonShroud3 && smite)
		{
			//damage
			enemy->demonShroud3 = false;
			enemy->enemy3Health = enemy->enemy3Health - deal;

			//info card
			info = "The demons\'s shroud was destroyed by your smite!";
			infoSet = true;

			//guide card
			char* dealStr = malloc(2);
			sprintf(dealStr, "%i", deal);
			int dLen = strlen(dealStr);

			char* begin = "Your attack dealt ";
			int beginLen = strlen(begin);

			char* end = " damage!";
			int endLen = strlen(end);
			guide = malloc(beginLen + dLen + endLen);

			memcpy(guide, begin, beginLen);
			memcpy(guide + beginLen, dealStr, dLen);
			memcpy(guide + beginLen + dLen, end, endLen);
			guideSet = true;
		}

		//normal smite
		if (!enemy->demonShroud3 && smite)
		{
			enemy->enemy3Health = enemy->enemy3Health - deal;

			info = "You used smite on an enemy!";
			infoSet = true;

			//guide card
			char* dealStr = malloc(2);
			sprintf(dealStr, "%i", deal);
			int dLen = strlen(dealStr);

			char* begin = "Your attack dealt ";
			int beginLen = strlen(begin);

			char* end = " damage!";
			int endLen = strlen(end);
			guide = malloc(beginLen + dLen + endLen);

			memcpy(guide, begin, beginLen);
			memcpy(guide + beginLen, dealStr, dLen);
			memcpy(guide + beginLen + dLen, end, endLen);
			guideSet = true;
		}

		//normal attack
		if (!enemy->demonShroud3 && !smite)
		{
			enemy->enemy3Health = enemy->enemy3Health - deal;

			info = "You used attack on an enemy!";
			infoSet = true;

			//guide card
			char* dealStr = malloc(2);
			sprintf(dealStr, "%i", deal);
			int dLen = strlen(dealStr);

			char* begin = "Your attack dealt ";
			int beginLen = strlen(begin);

			char* end = " damage!";
			int endLen = strlen(end);
			guide = malloc(beginLen + dLen + endLen);

			memcpy(guide, begin, beginLen);
			memcpy(guide + beginLen, dealStr, dLen);
			memcpy(guide + beginLen + dLen, end, endLen);
			guideSet = true;
		}

		//check if enemy dead, and clear enemy 3
		if (enemy->enemy3Health <= 0)
		{
			info = "You have defeated an enemy!";
			infoSet = true;

			numEnemies--;

			//shift enemies down
			enemy->enemy3Health = 0;
			enemy->enemy3Type = MONSTER_NONE;

		}
		break;
	}

	smite = false;
}



void EnemyCheck(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}

	if (!client->inCombat)
	{
		return;
	}

	edict_t* enemy = client->enemy;

	//checks for goblin
	if (enemy->enemy1Type == MONSTER_GOBLIN || enemy->enemy2Type == MONSTER_GOBLIN || enemy->enemy3Type == MONSTER_GOBLIN)
	{
		goblin = true;
	}
	else
	{
		goblin = false;
		goblinBomb = 1;
	}

	//checks for drake
	if (enemy->enemy1Type == MONSTER_DRAKE || enemy->enemy2Type == MONSTER_DRAKE)
	{
		drake = true;
		
	}
	else
	{
		drake = false;
		drakeFire = 1;
	}

	//checks for dragon
	if (enemy->enemy1Type == MONSTER_DRAGON)
	{
		dragon = true;
	}
	else
	{
		dragon = false;
		dragonFire = 1;
	}

}

//function for testing random
int Cmd_Roll_f(edict_t* ent)
{
	//int i;
	int random;
	//for (i = 0; i < 5; i++)
	//{
		//range
		random = (int)(crandom() * 4);

		//invert negatives
		if (random < 0)
		{
			random = random * -1;
		}


		//increase min values
		random += 1;

		//repeat unitl non zero answer
		/*while (random == 0)
		{
			random = (int)(crandom() * 6);
			if (random < 0)
			{
				random = random * -1;
			}
			random += 5;
		}*/
		gi.cprintf(ent, 1, "random int: %d\n", random);
		return random;
	//}
}
//randomizes damage of attacks for monsters and targets of attacks for monsters
int Generate(int min, int range)
{
	int deal = 0;

	deal = (int) (crandom() * range);
	//invert negatives
	if(deal < 0)
	{
		deal = deal * -1;
	}

	//increase min values
	deal += min;

	return deal;
}

//finds next turn if on monster team
void MonsterNextTurn(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}

	if (!client->inCombat)
	{
		return;
	}

	edict_t* enemy = client->enemy;


	if (monsterIndex > numEnemies)
	{
		//set turn to player's turn
		client->turn = true;

		monsterIndex = 1;
		partyIndex = 1;
		
		//end curse of frailty
		enemy->enemy1Weak = false;
		enemy->enemy2Weak = false;
		enemy->enemy3Weak = false;

		//end hero temp health
		client->heroBuffer = false;
		client->heroTempHealth = 0;
		client->rangerTempHealth = 0;
		client->warriorTempHealth = 0;
		client->wizardTempHealth = 0;

		//tick down shieldBash if it is active
		if (shieldBash > 0)
		{
			shieldBash--;
		}

		//turn off taunt
		client->warriorTaunt = false;

		if (goblinBomb == 4)
		{
			goblinBomb = 0;
		}
		if (drakeFire == 3)
		{
			drakeFire = 0;
		}
		if (dragonFire == 2)
		{
			dragonFire = 0;
		}

		//AoE tick up
		if (goblin)
		{
			goblinBomb++;
		}

		if (drake)
		{
			drakeFire++;
		}

		if (dragon)
		{
			dragonFire++;
		}
	}
}

//Monster attack damage calculation
void MonsterAttack(edict_t* ent, int target, int damage)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}

	//protecting against this being called after hero dies
	if (!client->inCombat)
	{
		return;
	}

	edict_t* enemy = client->enemy;
	char* begin;
	char* dealStr;
	char* end;

	int beginlen;
	int dealLen;
	int endlen;

	qboolean weak = false;

	switch (monsterIndex)
	{
	case 1:
		if (enemy->enemy1Weak)
		{
			weak = true;
		}
		break;
	case 2:
		if (enemy->enemy2Weak)
		{
			weak = true;
		}
		break;
	case 3:
		if (enemy->enemy3Weak)
		{
			weak = true;
		}
		break;
	}
	//curse of frailty debuff
	if (weak)
	{
		damage = damage / 2;
	}

	switch (target)
	{
	case CLASS_HERO:

		damage -= (client->heroArmor * 3);
		if (damage < 0)
		{
			damage = 0;

			info = "The enemy's attack was too weak to harm you!";
			infoSet = true;

			guide = "The hero took 0 damage!";
			guideSet = true;
			return;
		}
		if (client->heroBuffer)
		{
			if (client->heroTempHealth > damage)
			{
				//do damage
				client->heroTempHealth = client->heroTempHealth - damage;

				//set info
				info = "The enemy has attacked your holy shield!";
				infoSet = true;

				//set guide
				if (damage > 9)
				{
					dealStr = malloc(2);
				}
				if (damage < 10)
				{
					dealStr = malloc(1);
				}
				sprintf(dealStr, "%d", damage);
				dealLen = strlen(dealStr);
				begin = "The enemy has dealt ";
				beginlen = strlen(begin);
				end = " to the hero\'s shield!";
				endlen = strlen(end);

				guide = malloc(beginlen + dealLen + endlen);
				memcpy(guide, begin, beginlen);
				memcpy(guide + beginlen, dealStr, dealLen);
				memcpy(guide + beginlen + dealLen, end, endlen);
				guideSet = true;
			}

			if (client->heroTempHealth <= damage)
			{
				damage -= client->heroTempHealth;
				client->heroTempHealth = 0;
				//don't let the display try to show a negative number so skip to end
				if (client->heroHealth - damage <= 0)
				{
					goto loseState;
				}
				client->heroHealth = client->heroHealth - damage;

				info = "The enemy has broken your holy shield!";
				infoSet = true;

				//set guide
				if (damage > 9)
				{
					dealStr = malloc(2);
				}
				if (damage < 10)
				{
					dealStr = malloc(1);
				}
				sprintf(dealStr, "%d", damage);
				dealLen = strlen(dealStr);
				begin = "The enemy has dealt ";
				beginlen = strlen(begin);
				end = " to the hero!";
				endlen = strlen(end);

				guide = malloc(beginlen + dealLen + endlen);
				memcpy(guide, begin, beginlen);
				memcpy(guide + beginlen, dealStr, dealLen);
				memcpy(guide + beginlen + dealLen, end, endlen);
				guideSet = true;
			}
		}
		else
		{
			//don't let the display try to show a negative number so skip to end
			if (client->heroHealth - damage <= 0)
			{
				goto loseState;
			}
			client->heroHealth = client->heroHealth - damage;

			info = "The hero was attacked!";
			infoSet = true;

			//set guide
			if (damage > 9)
			{
				dealStr = malloc(2);
			}
			if (damage < 10)
			{
				dealStr = malloc(1);
			}
			sprintf(dealStr, "%d", damage);
			dealLen = strlen(dealStr);
			begin = "The enemy has dealt ";
			beginlen = strlen(begin);
			end = " to the hero!";
			endlen = strlen(end);

			guide = malloc(beginlen + dealLen + endlen);
			memcpy(guide, begin, beginlen);
			memcpy(guide + beginlen, dealStr, dealLen);
			memcpy(guide + beginlen + dealLen, end, endlen);
			guideSet = true;
		}

		//lose state only for hero. You can go on fighting so long as the hero is alive
		if (client->heroHealth <= 0)
		{
			loseState:
			client->heroHealth = 100;
			client->rangerHealth = 100;
			client->wizardHealth = 100;
			client->warriorHealth = 100;

			battleLost = true;

			firstCombat = false;
			client->gold = 0;
			client->gunpowder = 0;
			client->scales = 0;

			client->potions = 0;
			client->bombs = 0;
			client->magicPotions = 0;

			client->heroArmor = 0;
			client->heroWeapon = 0;
			client->rangerArmor = 0;
			client->rangerWeapon = 0;
			client->wizardArmor = 0;
			client->wizardWeapon = 0;
			client->warriorArmor = 0;
			client->warriorWeapon = 0;
			

			client->heroTempHealth = 0;
			client->rangerTempHealth = 0;
			client->wizardTempHealth = 0;
			client->warriorTempHealth = 0;
			client->heroBuffer = false;
			client->warriorTaunt = false;


			partyIndex = 1;
			monsterIndex = 1;


			shieldBash = 0;
			smite = false;
			client->warriorTaunt = false;
			client->heroBuffer = false;
			buffer = false;

			client->inCombat = false;
			
			enemy->enemy1Health = 0;
			enemy->enemy1Type = 0;
			enemy->enemy2Health = 0;
			enemy->enemy2Type = 0;
			enemy->enemy3Health = 0;
			enemy->enemy3Type = 0;
			enemy->demonShroud1 = false;
			enemy->demonShroud2 = false;
			enemy->demonShroud3 = false;
			enemy->enemy1Weak = false;
			enemy->enemy2Weak = false;
			enemy->enemy3Weak = false;
			//client->enemy = NULL;

			goblinBomb = 1;
			drakeFire = 1;
			dragonFire = 1;

			goblin = false;
			drake = false;
			dragon = false;

			if (ent->client->showhelp)
			{
				Cmd_Help_f(ent);
			}

			gi.centerprintf(ent, "You lost the battle! Your stats are reset!");

			/*vec3_t* pos;
			pos = ent->s.origin;*/

			/*if ((enemy->svflags & SVF_MONSTER) || (client))
				enemy->flags |= FL_NO_KNOCKBACK;
			Killed(client, enemy, enemy, damage, pos);*/
			//Cmd_Kill_f(ent);
			//player_die(ent, ent, ent, 1000, vec3_origin);
			return;
		}
		return;
	case CLASS_RANGER:
		damage -= (client->rangerArmor * 3);
		if (damage < 0)
		{
			damage = 0;

			info = "The enemy's attack was too weak to harm you!";
			infoSet = true;

			guide = "The ranger took 0 damage!";
			guideSet = true;
			return;
		}
		if (client->heroBuffer)
		{
			if (client->rangerTempHealth > damage)
			{
				//do damage
				client->rangerTempHealth = client->rangerTempHealth - damage;

				//set info
				info = "The enemy has attacked your holy shield!";
				infoSet = true;

				//set guide
				if (damage > 9)
				{
					dealStr = malloc(2);
				}
				if (damage < 10)
				{
					dealStr = malloc(1);
				}
				sprintf(dealStr, "%d", damage);
				dealLen = strlen(dealStr);
				begin = "The enemy has dealt ";
				beginlen = strlen(begin);
				end = " to the ranger\'s shield!";
				endlen = strlen(end);

				guide = malloc(beginlen + dealLen + endlen);
				memcpy(guide, begin, beginlen);
				memcpy(guide + beginlen, dealStr, dealLen);
				memcpy(guide + beginlen + dealLen, end, endlen);
				guideSet = true;
			}

			if (client->rangerTempHealth <= damage)
			{
				damage -= client->rangerTempHealth;
				client->rangerTempHealth = 0;
				client->rangerHealth = client->rangerHealth - damage;

				info = "The enemy has broken your holy shield!";
				infoSet = true;

				//set guide
				if (damage > 9)
				{
					dealStr = malloc(2);
				}
				if (damage < 10)
				{
					dealStr = malloc(1);
				}
				sprintf(dealStr, "%d", damage);
				dealLen = strlen(dealStr);
				begin = "The enemy has dealt ";
				beginlen = strlen(begin);
				end = " to the ranger!";
				endlen = strlen(end);

				guide = malloc(beginlen + dealLen + endlen);
				memcpy(guide, begin, beginlen);
				memcpy(guide + beginlen, dealStr, dealLen);
				memcpy(guide + beginlen + dealLen, end, endlen);
				guideSet = true;
			}
		}
		else
		{
			client->rangerHealth = client->rangerHealth - damage;

			info = "The ranger was attacked!";
			infoSet = true;

			//set guide
			if (damage > 9)
			{
				dealStr = malloc(2);
			}
			if (damage < 10)
			{
				dealStr = malloc(1);
			}
			sprintf(dealStr, "%d", damage);
			dealLen = strlen(dealStr);
			begin = "The enemy has dealt ";
			beginlen = strlen(begin);
			end = " to the ranger!";
			endlen = strlen(end);

			guide = malloc(beginlen + dealLen + endlen);
			memcpy(guide, begin, beginlen);
			memcpy(guide + beginlen, dealStr, dealLen);
			memcpy(guide + beginlen + dealLen, end, endlen);
			guideSet = true;
		}

		if (client->rangerHealth <= 0)
		{
			client->rangerHealth = 0;
			client->rangerDead = true;

			info = "The ranger has died!";
			infoSet = true;
		}
		return;
	case CLASS_WIZARD:
		damage -= (client->wizardArmor * 3);
		if (damage < 0)
		{
			damage = 0;

			info = "The enemy's attack was too weak to harm you!";
			infoSet = true;

			guide = "The wizard took 0 damage!";
			guideSet = true;
			return;
		}
		if (client->heroBuffer)
		{
			if (client->wizardTempHealth > damage)
			{
				//do damage
				client->wizardTempHealth = client->wizardTempHealth - damage;

				//set info
				info = "The enemy has attacked your holy shield!";
				infoSet = true;

				//set guide
				if (damage > 9)
				{
					dealStr = malloc(2);
				}
				if (damage < 10)
				{
					dealStr = malloc(1);
				}
				sprintf(dealStr, "%d", damage);
				dealLen = strlen(dealStr);
				begin = "The enemy has dealt ";
				beginlen = strlen(begin);
				end = " to the wizard\'s shield!";
				endlen = strlen(end);

				guide = malloc(beginlen + dealLen + endlen);
				memcpy(guide, begin, beginlen);
				memcpy(guide + beginlen, dealStr, dealLen);
				memcpy(guide + beginlen + dealLen, end, endlen);
				guideSet = true;
			}

			if (client->wizardTempHealth <= damage)
			{
				damage -= client->wizardTempHealth;
				client->wizardTempHealth = 0;
				client->wizardHealth = client->wizardHealth - damage;

				info = "The enemy has broken your holy shield!";
				infoSet = true;

				//set guide
				if (damage > 9)
				{
					dealStr = malloc(2);
				}
				if (damage < 10)
				{
					dealStr = malloc(1);
				}
				sprintf(dealStr, "%d", damage);
				dealLen = strlen(dealStr);
				begin = "The enemy has dealt ";
				beginlen = strlen(begin);
				end = " to the wizard!";
				endlen = strlen(end);

				guide = malloc(beginlen + dealLen + endlen);
				memcpy(guide, begin, beginlen);
				memcpy(guide + beginlen, dealStr, dealLen);
				memcpy(guide + beginlen + dealLen, end, endlen);
				guideSet = true;
			}
		}
		else
		{
			client->wizardHealth = client->wizardHealth - damage;

			info = "The wizard was attacked!";
			infoSet = true;

			//set guide
			if (damage > 9)
			{
				dealStr = malloc(2);
			}
			if (damage < 10)
			{
				dealStr = malloc(1);
			}
			sprintf(dealStr, "%d", damage);
			dealLen = strlen(dealStr);
			begin = "The enemy has dealt ";
			beginlen = strlen(begin);
			end = " to the wizard!";
			endlen = strlen(end);

			guide = malloc(beginlen + dealLen + endlen);
			memcpy(guide, begin, beginlen);
			memcpy(guide + beginlen, dealStr, dealLen);
			memcpy(guide + beginlen + dealLen, end, endlen);
			guideSet = true;
		}

		if (client->wizardHealth <= 0)
		{
			client->wizardHealth = 0;
			client->wizardDead = true;

			info = "The wizard has died!";
			infoSet = true;
		}
		return;
	case CLASS_WARRIOR:
		damage -= (client->warriorArmor * 3);
		if (shieldBash > 0)
		{
			damage -= 5;
		}

		if (damage < 0)
		{
			damage = 0;

			info = "The enemy's attack was too weak to harm you!";
			infoSet = true;

			guide = "The warrior took 0 damage!";
			guideSet = true;
			return;
		}

		if (client->heroBuffer)
		{
			if (client->warriorTempHealth > damage)
			{
				//do damage
				client->warriorTempHealth = client->warriorTempHealth - damage;

				//set info
				info = "The enemy has attacked your holy shield!";
				infoSet = true;

				//set guide
				if (damage > 9)
				{
					dealStr = malloc(2);
				}
				if (damage < 10)
				{
					dealStr = malloc(1);
				}
				sprintf(dealStr, "%d", damage);
				dealLen = strlen(dealStr);
				begin = "The enemy has dealt ";
				beginlen = strlen(begin);
				end = " to the warrior\'s shield!";
				endlen = strlen(end);

				guide = malloc(beginlen + dealLen + endlen);
				memcpy(guide, begin, beginlen);
				memcpy(guide + beginlen, dealStr, dealLen);
				memcpy(guide + beginlen + dealLen, end, endlen);
				guideSet = true;
			}

			if (client->warriorTempHealth <= damage)
			{
				damage -= client->warriorTempHealth;
				client->warriorTempHealth = 0;
				client->warriorHealth = client->warriorHealth - damage;

				info = "The enemy has broken your holy shield!";
				infoSet = true;

				//set guide
				if (damage > 9)
				{
					dealStr = malloc(2);
				}
				if (damage < 10)
				{
					dealStr = malloc(1);
				}
				sprintf(dealStr, "%d", damage);
				dealLen = strlen(dealStr);
				begin = "The enemy has dealt ";
				beginlen = strlen(begin);
				end = " to the warrior!";
				endlen = strlen(end);

				guide = malloc(beginlen + dealLen + endlen);
				memcpy(guide, begin, beginlen);
				memcpy(guide + beginlen, dealStr, dealLen);
				memcpy(guide + beginlen + dealLen, end, endlen);
				guideSet = true;
			}
		}
		else
		{
			client->warriorHealth = client->warriorHealth - damage;

			info = "The warrior was attacked!";
			infoSet = true;

			//set guide
			if (damage > 9)
			{
				dealStr = malloc(2);
			}
			if (damage < 10)
			{
				dealStr = malloc(1);
			}
			sprintf(dealStr, "%d", damage);
			dealLen = strlen(dealStr);
			begin = "The enemy has dealt ";
			beginlen = strlen(begin);
			end = " to the warrior!";
			endlen = strlen(end);

			guide = malloc(beginlen + dealLen + endlen);
			memcpy(guide, begin, beginlen);
			memcpy(guide + beginlen, dealStr, dealLen);
			memcpy(guide + beginlen + dealLen, end, endlen);
			guideSet = true;
		}

		if (client->warriorHealth <= 0)
		{
			client->warriorHealth = 0;
			client->warriorDead = true;

			info = "The warrior has died!";
			infoSet = true;
		}
		return;
	}
}

void MonsterTypeBehavior(edict_t* ent, int target, int monsterType)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	edict_t* enemy = client->enemy;

	int deal;
	switch (monsterType)
	{
	case MONSTER_NONE:
		//gi.cprintf(ent, 1, "A monster type 0 was called! You messed up man!\n");
		return;
		break;
	case MONSTER_GOBLIN:
		if (goblinBomb == 4)
		{
			MonsterAttack(ent, 1, 10);
			if (!client->rangerDead)
			{
				MonsterAttack(ent, 2, 10);
			}

			if (!client->wizardDead)
			{
				MonsterAttack(ent, 3, 10);
			}

			if (!client->warriorDead)
			{
				MonsterAttack(ent, 4, 10);
			}

			guide = "The goblin has thrown a bomb at your party!";
			guideSet = true;
		}
		else
		{
			deal = Generate(5, 6);
			MonsterAttack(ent, target, deal);
		}
		return; 
		break;
	case MONSTER_ORC:
		deal = Generate(10, 6);
		MonsterAttack(ent, target, deal);
		return;
		break;
	case MONSTER_DRAKE:
		if (drakeFire == 3)
		{
			MonsterAttack(ent, 1, 15);
			if (!client->rangerDead)
			{
				MonsterAttack(ent, 2, 15);
			}

			if (!client->wizardDead)
			{
				MonsterAttack(ent, 3, 15);
			}

			if (!client->warriorDead)
			{
				MonsterAttack(ent, 4, 15);
			}
			guide = "The drake used his fire breath on your party!";
			guideSet = true;
		}
		else
		{
			deal = Generate(15, 6);
			MonsterAttack(ent, target, deal);
		}
		return;
		break;
	case MONSTER_DEMON:
		deal = Generate(15, 6);
		MonsterAttack(ent, target, deal);
		return;
		break;
	case MONSTER_DRAGON:
		if (dragonFire == 2)
		{
			MonsterAttack(ent, 1, 20);
			if (!client->rangerDead)
			{
				MonsterAttack(ent, 2, 20);
			}

			if (!client->wizardDead)
			{
				MonsterAttack(ent, 3, 20);
			}

			if (!client->warriorDead)
			{
				MonsterAttack(ent, 4, 20);
			}
			guide = "The dragon used his fire breath on your party!";
			guideSet = true;
		}
		else
		{
			deal = Generate(15, 6);
			MonsterAttack(ent, target, deal);
		}
		return;
		break;
	}
}

void Cmd_MonsterBehave_f(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}

	//do nothing if not in combat
	if (!client->inCombat)
	{
		return;
	}

	//do nothing if not enemy turn
	if (client->turn)
	{
		return;
	}

	edict_t* enemy = client->enemy;

	qboolean validTarget = false;
	int target = 0;
	int damage = 0;

	//finds out if there are AoE attacks to consider
	EnemyCheck(ent);

	//while (!validTarget)
	//{
	getTarget:
	target = Generate(1, 4);

		//target = (int)(random() * 4);
		//target++;
		//always true if combat is still happening
		if (target == CLASS_HERO)
		{
			validTarget = true;
		}

		if (target == CLASS_RANGER)
		{
			if (client->rangerDead)
			{
				validTarget = false;
				goto getTarget;
			}
		}

		if (target == CLASS_WIZARD)
		{
			if (client->wizardDead)
			{
				validTarget = false;
				goto getTarget;
			}
		}

		if (target == CLASS_WARRIOR)
		{
			if (client->warriorDead)
			{
				validTarget = false;
				goto getTarget;
			}
		}
	//}
	
	//always attack warrior if the warrior is alive and taunting
		if (client->warriorTaunt && !client->warriorDead)
		{
			target = 4;
			validTarget = true;
		}

	//gi.cprintf(ent, 1, "%d\n", target);

	switch (monsterIndex)
	{
	case 1:
		MonsterTypeBehavior(ent, target, enemy->enemy1Type);
		monsterIndex++;
		MonsterNextTurn(ent);
		break;
	case 2:
		MonsterTypeBehavior(ent, target, enemy->enemy2Type);
		monsterIndex++;
		MonsterNextTurn(ent);
		break;
	case 3:
		MonsterTypeBehavior(ent, target, enemy->enemy3Type);
		monsterIndex++;
		MonsterNextTurn(ent);
		break;
	}

	CombatScreen(ent);
}

void Cmd_WeaponPrices_f(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	gi.cprintf(ent, 1, "Weapon Upgrades\nLight        10g\nDarksteel    30g\nDragon Scale 15 Scales\n");
}

void Cmd_ArmorPrices_f(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	gi.cprintf(ent, 1, "Armor Upgrades\nLight        10g\nDarksteel    30g\nDragon Scale 15 Scales\n");
}

void Cmd_ConsumablePrices_f(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	gi.cprintf(ent, 1, "Consumables\nHealth Potion  15g\nMagic Potion 20g\nBomb         5 Gunpowder\n");
}

void Cmd_Resources_f(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	if (!client->inCombat)
	{
		gi.cprintf(ent,1,"Current Gold:      %d\nCurrent Scales:    %d\nCurrent Gunpowder: %d", client->gold,client->scales,client->gunpowder);
	}
	else
	{
		gi.centerprintf(ent,"Cannot shop while in combat.");
	}
}

void Cmd_Items_f(edict_t* ent)
{
	gi.cprintf(ent, 1, "Potions:         %d\nMagic Potions:   %d\nBombs:           %d\n",ent->client->potions, ent->client->magicPotions, ent->client->bombs);
}

void Cmd_Stats_f(edict_t* ent)
{
	//to be finished
}

//Use a skill
void Cmd_UseSkill_f(edict_t* ent)
{
	int i;
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	//This check has to be done before declaration of enemy.
	if ((Q_stricmp(gi.argv(1), "hero") == 0))
	{
		if (gi.argc() == 3)
		{
			//hope skill
			if (Q_stricmp(gi.argv(2), "hope") == 0)
			{
				gi.cprintf(ent, 1, "Got to hope!\n");
					//cancel if in combat and it is not hero's turn. This has to be done so that the player can activate this skill while out of combat
					if (client->inCombat && !(partyIndex == 1) && !client->turn)
					{
						gi.cprintf(ent, 1, "It is not the hero\'s turn!\n");
						return;
					}

				//check for MP
				if (client->heroMP >= 40)
				{
					//do the skill
					if (client->rangerDead)
					{
						client->rangerDead = false;
						client->rangerHealth = 40;
					}
					if (client->wizardDead)
					{
						client->wizardDead = false;
						client->wizardHealth = 40;
					}
					if (client->warriorDead)
					{
						client->warriorDead = false;
						client->warriorHealth = 40;
					}

					//cost
					client->heroMP = client->heroMP - 40;

					//update screen
					gi.cprintf(ent, 1, "You revived your party!\n");
					CombatScreen(ent);

					//next teammate will always be alive so we don't have to check
					if(client->inCombat)
					{
					partyIndex++;
					}
					return;
				}
				else
				{
					gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
					return;
				}
			}
		}
	}

	//declare enemy
	edict_t* enemy;
	enemy = client->enemy;
	if (!enemy)
	{
		return;
	}

	//most skills cannot be used when not in combat, not your turn, and not on the character's initiative
	if (!client->inCombat)
	{
		gi.cprintf(ent, 1, "Skill can only be used in battle!\n");
		return;
	}
	if (!client->turn)
	{
		gi.cprintf(ent, 1, "It is not your turn yet!\n");
		return;
	}

	//hero skills
	if (Q_stricmp(gi.argv(1), "hero") == 0)
	{
		//most skills cannot be used when not in combat, not your turn, and not on the character's initiative
		if (!(partyIndex == 1))
		{
			gi.cprintf(ent, 1, "It is not the hero\'s turn!\n");
			return;
		}
		//contains all 2 argument skills
		if (gi.argc() == 3)
		{	
			if (Q_stricmp(gi.argv(2), "holyshield") == 0)
			{
				if (client->heroMP >= 30)
				{
					
					//do skill
					client->heroBuffer = true;
					buffer = true;
					client->heroTempHealth = 20;
					client->rangerTempHealth = 20;
					client->wizardTempHealth = 20;
					client->warriorTempHealth = 20;

					//take cost
					client->heroMP = client->heroMP - 30;

					//find next turn
					partyIndex++;
					PartyNextTurn(ent);

					//update screen
					CombatScreen(ent);
					return;
				}
				else
				{
					gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
					return;
				}
			}
			//if there is only one enemy
			if (Q_stricmp(gi.argv(2), "smite") == 0)
			{
				//check if call was correct
				if (numEnemies > 1)
				{
					gi.cprintf(ent, 1, "Specify your target!\n");
				}
				else if (numEnemies == 1)
				{
					//check for MP, if high enough, do the skill
					if (client->heroMP >= 20)
					{
						//do the skill
						smite = true;
						PartyAttack(ent, 1, 10, true, true);
						
						//take the MP
						client->heroMP = client->heroMP - 20;

						//find next turn
						partyIndex++;
						PartyNextTurn(ent);

						//update screen
						CombatScreen(ent);

					}
					else
					{
						gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
						return;
					}
				}
			}
		}
		if (gi.argc() == 4)
		{
			//gi.cprintf(ent, 1, "Got to argc 4\n");
			if (Q_stricmp(gi.argv(2), "smite") == 0)
			{

				//check for mana
				if (client->heroMP >= 20)
				{
					int target = 0;

					//find out who the target of the attack is
					switch (numEnemies)
					{
					case 1:
						target = 1;
						break;
					case 2:
						if (Q_stricmp(gi.argv(3), "left") == 0)
						{
							target = 1;
							break;
						}
						if (Q_stricmp(gi.argv(3), "right") == 0)
						{
							target = 2;
							break;
						}

						//invalid arg
						if (target == 0)
						{
							gi.cprintf(ent, 1, "You did not enter a valid target!\n");
							return;
						}
						break;
					case 3:
						if (Q_stricmp(gi.argv(3), "left") == 0)
						{
							target = 1;
							break;
						}
						if (Q_stricmp(gi.argv(3), "center") == 0)
						{
							target = 2;
							break;
						}
						if (Q_stricmp(gi.argv(3), "right") == 0)
						{
							target = 3;
							break;
						}

						//invalid arg
						if (target == 0)
						{
							gi.cprintf(ent, 1, "You did not enter a valid target!\n");
							return;
						}
						break;
					}

					//do the skill
					

					if (target != 0)
					{
						smite = true;
						PartyAttack(ent, target, 10, true, true);

						client->heroMP = client->heroMP - 20;

						partyIndex++;
						PartyNextTurn(ent);

						CombatScreen(ent);
					}
					else
					{
						gi.cprintf(ent, 1, "Something went wrong!\n");
						return;
					}
				}
				//don't have enough mana
				else
				{
					gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
					return;
				}
			}
			return;
		}
	}
	
	//ranger skills
	if (Q_stricmp(gi.argv(1), "ranger") == 0)
	{
		//turn and dead exclusions
		if (!(partyIndex == 2))
		{
			gi.cprintf(ent, 1, "It is not the ranger\'s turn!\n");
			return;
		}

		if (client->rangerDead)
		{
			gi.cprintf(ent, 1, "The ranger is dead!\n");
			return;
		}

		//now check for arrowrain skill or single target heartpiercer
		if (gi.argc() == 3)
		{
			//arrow rain
			if (Q_stricmp(gi.argv(2), "arrowrain") == 0)
			{
				//check for MP
				if (client->rangerMP >= 25)
				{
					//do skill: attack all living enemies in reverse order in case any die
					if (enemy->enemy3Type != MONSTER_NONE)
					{
						PartyAttack(ent, 3, 5, true, true);
					}

					if (enemy->enemy2Type != MONSTER_NONE)
					{
						PartyAttack(ent, 2, 5, true, true);
					}

					if (enemy->enemy1Type != MONSTER_NONE)
					{
						PartyAttack(ent, 1, 5, true, true);
					}

					//say what happened
					info = "You attacked all enemies with rain of arrows!";
					infoSet = true;

					//MP cost
					client->rangerMP = client->rangerMP - 25;

					//get next turn
					partyIndex++;
					PartyNextTurn(ent);

					//show stuff on screen
					CombatScreen(ent);
				}
			}

			//if there is only one enemy
			if (Q_stricmp(gi.argv(2), "heartpiercer") == 0)
			{
				//check if call was correct
				if (numEnemies > 1)
				{
					gi.cprintf(ent, 1, "Specify your target!\n");
				}
				else if (numEnemies == 1)
				{
					//check for MP, if high enough, do the skill
					if (client->rangerMP >= 30)
					{
						//bonus for skill
						int damage = 10;

						//increase bonus damage against drakes and dragons
						if (enemy->enemy1Type == MONSTER_DRAKE || enemy->enemy1Type == MONSTER_DRAGON)
						{
							damage += 15;
						}
						//do the skill
						PartyAttack(ent, 1, damage, true, true);

						//take the MP
						client->rangerMP = client->rangerMP - 30;

						//find next turn
						partyIndex++;
						PartyNextTurn(ent);

						//heartpiercer text is overrided if that kills the enemy
						if (!(Q_stricmp(info, "You have defeated an enemy!") == 0))
						{
							//gi.cprintf(ent, 1, "Got to info set\n");
							info = "You used heartpiercer on an enemy!";
							infoSet = true;
						}
						//update screen
						CombatScreen(ent);

					}
					else
					{
						gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
						return;
					}
				}
			}
			return;
		}
		//multiple possible targets for heatpiercer
		if (gi.argc() == 4)
		{
			//gi.cprintf(ent, 1, "Got to argc 4\n");
			if (Q_stricmp(gi.argv(2), "heartpiercer") == 0)
			{

				//check for mana
				if (client->rangerMP >= 30)
				{
					int target = 0;
					int damage = 10;
					//find out who the target of the attack is, then calculate bonus
					switch (numEnemies)
					{
					case 1:
						target = 1;
						break;
					case 2:
						if (Q_stricmp(gi.argv(3), "left") == 0)
						{
							target = 1;
							break;
						}
						if (Q_stricmp(gi.argv(3), "right") == 0)
						{
							target = 2;
							break;
						}

						//invalid arg
						if (target == 0)
						{
							gi.cprintf(ent, 1, "You did not enter a valid target!\n");
							return;
						}
						break;
					case 3:
						if (Q_stricmp(gi.argv(3), "left") == 0)
						{
							target = 1;
							break;
						}
						if (Q_stricmp(gi.argv(3), "center") == 0)
						{
							target = 2;
							break;
						}
						if (Q_stricmp(gi.argv(3), "right") == 0)
						{
							target = 3;
							break;
						}

						//invalid arg
						if (target == 0)
						{
							gi.cprintf(ent, 1, "You did not enter a valid target!\n");
							return;
						}
						break;
					}

					//do the skill if target is working
					if (target != 0)
					{
						switch (target)
						{
						case 1:
							if (enemy->enemy1Type == MONSTER_DRAKE || enemy->enemy1Type == MONSTER_DRAGON)
							{
								damage += 15;
							}
							break;
						case 2:
							if (enemy->enemy2Type == MONSTER_DRAKE || enemy->enemy2Type == MONSTER_DRAGON)
							{
								damage += 15;
							}
							break;
						case 3:
							if (enemy->enemy3Type == MONSTER_DRAKE || enemy->enemy3Type == MONSTER_DRAGON)
							{
								damage += 15;
							}
							break;
						}

						PartyAttack(ent, target, damage, true, true);
						client->rangerMP = client->rangerMP - 30;

						//heartpiercer text is overrided if that kills the enemy
						if (!(Q_stricmp(info, "You have defeated an enemy!") == 0))
						{
							//gi.cprintf(ent, 1, "Got to info set\n");
							info = "You used heartpiercer on an enemy!";
							infoSet = true;
						}

						partyIndex++;
						PartyNextTurn(ent);

						CombatScreen(ent);
					}
					else
					{
						gi.cprintf(ent, 1, "Something went wrong!\n");
						return;
					}
				}
				//don't have enough mana
				else
				{
					gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
					return;
				}
			}
			return;
		}
	}

	//wizard skills
	if (Q_stricmp(gi.argv(1), "wizard") == 0)
	{
		//turn and dead exclusions
		if (!(partyIndex == 3))
		{
			gi.cprintf(ent, 1, "It is not the wizard\'s turn!\n");
			return;
		}

		if (client->wizardDead)
		{
			gi.cprintf(ent, 1, "The wizard is dead!\n");
			return;
		}

		//fireball, frailty curse with one enemy, and lightning with one enemy
		if (gi.argc() == 3)
		{
			//fireball
			if (Q_stricmp(gi.argv(2), "fireball") == 0)
			{
				//check for MP
				if (client->wizardMP >= 35)
				{
					int damage = 15;
					//do skill: attack all living enemies in reverse order in case any die
					if (enemy->enemy3Type != MONSTER_NONE)
					{
						if (enemy->enemy3Type == MONSTER_GOBLIN || enemy->enemy3Type == MONSTER_ORC)
						{
							PartyAttack(ent, 3, damage + 10, true, false);
						}
						else
						{
							PartyAttack(ent, 3, damage, true, false);
						}
					}

					if (enemy->enemy2Type != MONSTER_NONE)
					{
							if (enemy->enemy2Type == MONSTER_GOBLIN || enemy->enemy2Type == MONSTER_ORC)
							{
								PartyAttack(ent, 2, damage + 10, true, false);
							}
							else
							{
								PartyAttack(ent, 2, damage, true, false);
							}
					}

					if (enemy->enemy1Type != MONSTER_NONE)
					{
							if (enemy->enemy1Type == MONSTER_GOBLIN || enemy->enemy1Type == MONSTER_ORC)
							{
								PartyAttack(ent, 1, damage + 10, true, false);
							}
							else
							{
								PartyAttack(ent, 1, damage, true, false);
							}
					}

					//say what happened
					info = "You attacked all enemies with a fireball!";
					infoSet = true;

					//MP cost
					client->wizardMP = client->wizardMP - 35;

					//get next turn
					partyIndex++;
					PartyNextTurn(ent);

					//show stuff on screen
					CombatScreen(ent);
				}
			}

			//if there is only one enemy
			if (Q_stricmp(gi.argv(2), "frailtycurse") == 0)
			{
				//check if call was correct
				if (numEnemies > 1)
				{
					gi.cprintf(ent, 1, "Specify your target!\n");
				}
				else if (numEnemies == 1)
				{
					//check for MP, if high enough, do the skill
					if (client->wizardMP >= 25)
					{
						//bonus for skill
						int damage = 5;

						//do the skill
						PartyAttack(ent, 1, damage, true, false);
						enemy->enemy1Weak = true;

						//take the MP
						client->wizardMP = client->wizardMP - 25;

						//find next turn
						partyIndex++;
						PartyNextTurn(ent);

						//gi.cprintf(ent, 1, "Got to info set\n");
						if (!(Q_stricmp(info, "You have defeated an enemy!") == 0))
						{
							info = "You used a curse of frailty on an enemy!";
							infoSet = true;
						}
						//update screen
						CombatScreen(ent);

					}
					else
					{
						gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
						return;
					}
				}
			}

			if (Q_stricmp(gi.argv(2), "lightning") == 0)
			{
				//check if call was correct
				if (numEnemies > 1)
				{
					gi.cprintf(ent, 1, "Specify your target!\n");
				}
				else if (numEnemies == 1)
				{
					//check for MP, if high enough, do the skill
					if (client->wizardMP >= 40)
					{
						//bonus for skill
						int damage = 60;

						//do the skill
						PartyAttack(ent, 1, damage, false, false);

						//take the MP
						client->wizardMP = client->wizardMP - 40;

						//find next turn
						partyIndex++;
						PartyNextTurn(ent);

						//gi.cprintf(ent, 1, "Got to info set\n");
						if (!(Q_stricmp(info, "You have defeated an enemy!") == 0))
						{
							info = "You struck an enemy with lightning!";
							infoSet = true;
						}

						//update screen
						CombatScreen(ent);

					}
					else
					{
						gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
						return;
					}
				}
			}
			return;
		}

		if (gi.argc() == 4)
		{
			if (Q_stricmp(gi.argv(2), "frailtycurse") == 0)
			{

				//check for mana
				if (client->wizardMP >= 25)
				{
					int target = 0;
					int damage = 5;
					//find out who the target of the attack is, then apply weakness
					switch (numEnemies)
					{
					case 1:
						target = 1;
						enemy->enemy1Weak = true;
						break;
					case 2:
						if (Q_stricmp(gi.argv(3), "left") == 0)
						{
							target = 1;
							enemy->enemy1Weak = true;
							break;
						}
						if (Q_stricmp(gi.argv(3), "right") == 0)
						{
							target = 2;
							enemy->enemy2Weak = true;
							break;
						}

						//invalid arg
						if (target == 0)
						{
							gi.cprintf(ent, 1, "You did not enter a valid target!\n");
							return;
						}
						break;
					case 3:
						if (Q_stricmp(gi.argv(3), "left") == 0)
						{
							target = 1;
							enemy->enemy1Weak = true;
							break;
						}
						if (Q_stricmp(gi.argv(3), "center") == 0)
						{
							target = 2;
							enemy->enemy2Weak = true;
							break;
						}
						if (Q_stricmp(gi.argv(3), "right") == 0)
						{
							target = 3;
							enemy->enemy3Weak = true;
							break;
						}

						//invalid arg
						if (target == 0)
						{
							gi.cprintf(ent, 1, "You did not enter a valid target!\n");
							return;
						}
						break;
					}

					//do the skill if target is working
					if (target != 0)
					{

						PartyAttack(ent, target, damage, true, false);

						client->wizardMP = client->wizardMP - 25;

						if (!(Q_stricmp(info, "You have defeated an enemy!") == 0))
						{
							info = "You used a curse of frailty on an enemy!";
							infoSet = true;
						}

						partyIndex++;
						PartyNextTurn(ent);

						CombatScreen(ent);
					}
					else
					{
						gi.cprintf(ent, 1, "Something went wrong!\n");
						return;
					}
				}
				//don't have enough mana
				else
				{
					gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
					return;
				}
			}

			if (Q_stricmp(gi.argv(2), "lightning") == 0)
			{

				//check for mana
				if (client->wizardMP >= 40)
				{
					int target = 0;
					int damage = 60;
					//find out who the target of the attack is, then apply weakness
					switch (numEnemies)
					{
					case 1:
						target = 1;
						break;
					case 2:
						if (Q_stricmp(gi.argv(3), "left") == 0)
						{
							target = 1;
							break;
						}
						if (Q_stricmp(gi.argv(3), "right") == 0)
						{
							target = 2;
							break;
						}

						//invalid arg
						if (target == 0)
						{
							gi.cprintf(ent, 1, "You did not enter a valid target!\n");
							return;
						}
						break;
					case 3:
						if (Q_stricmp(gi.argv(3), "left") == 0)
						{
							target = 1;
							break;
						}
						if (Q_stricmp(gi.argv(3), "center") == 0)
						{
							target = 2;
							break;
						}
						if (Q_stricmp(gi.argv(3), "right") == 0)
						{
							target = 3;
							break;
						}

						//invalid arg
						if (target == 0)
						{
							gi.cprintf(ent, 1, "You did not enter a valid target!\n");
							return;
						}
						break;
					}

					//do the skill if target is working
					if (target != 0)
					{

						PartyAttack(ent, target, damage, false, false);

						client->wizardMP = client->wizardMP - 40;

						if (!(Q_stricmp(info, "You have defeated an enemy!") == 0))
						{
							info = "You struck an enemy with lightning!";
							infoSet = true;
						}

						partyIndex++;
						PartyNextTurn(ent);

						CombatScreen(ent);
					}
					else
					{
						gi.cprintf(ent, 1, "Something went wrong!\n");
						return;
					}
				}
				//don't have enough mana
				else
				{
					gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
					return;
				}
			}
			return;
		}
	}

	//warrior skills
	if (Q_stricmp(gi.argv(1), "warrior") == 0)
	{
		//turn and dead exclusions
		if (!(partyIndex == 4))
		{
			gi.cprintf(ent, 1, "It is not the warrior\'s turn!\n");
			return;
		}

		if (client->warriorDead)
		{
			gi.cprintf(ent, 1, "The warrior is dead!\n");
			return;
		}

		//taunt and shield bash for one enemy
		if (gi.argc() == 3)
		{
			if(Q_stricmp(gi.argv(2), "taunt") == 0)
			{
				if (client->warriorMP >= 35)
				{
					client->warriorTaunt = true;

					client->warriorMP = client->warriorMP - 35;

					info = "You have activated taunt!";
					infoSet = true;

					guide = "All enemies will target the warrior on their next turn!";
					guideSet = true;

					partyIndex++;
					PartyNextTurn(ent);

					CombatScreen(ent);
				}
				else
				{
					gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
				}
			}

			if (Q_stricmp(gi.argv(2), "shieldbash") == 0)
			{
				//check if call was correct
				if (numEnemies > 1)
				{
					gi.cprintf(ent, 1, "Specify your target!\n");
				}
				else if (numEnemies == 1)
				{
					//check for MP, if high enough, do the skill
					if (client->warriorMP >= 25)
					{
						//bonus for skill
						shieldBash = 2;

						//do the skill
						PartyAttack(ent, 1, 10, true, true);

						//take the MP
						client->warriorMP = client->warriorMP - 25;

						//find next turn
						partyIndex++;
						PartyNextTurn(ent);

						//gi.cprintf(ent, 1, "Got to info set\n");
						if (!(Q_stricmp(info, "You have defeated an enemy!") == 0))
						{
							info = "You struck an enemy with your shield!";
							infoSet = true;
						}

						//update screen
						CombatScreen(ent);

					}
					else
					{
						gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
						return;
					}
				}
			}
			return;
		}

		if (gi.argc() == 4)
		{
			if (Q_stricmp(gi.argv(2), "shieldbash") == 0)
			{

				//check for mana
				if (client->warriorMP >= 25)
				{
					int target = 0;
					//find out who the target of the attack is, then apply weakness
					switch (numEnemies)
					{
					case 1:
						target = 1;
						break;
					case 2:
if (Q_stricmp(gi.argv(3), "left") == 0)
{
	target = 1;
	break;
}
if (Q_stricmp(gi.argv(3), "right") == 0)
{
	target = 2;
	break;
}

//invalid arg
if (target == 0)
{
	gi.cprintf(ent, 1, "You did not enter a valid target!\n");
	return;
}
break;
					case 3:
						if (Q_stricmp(gi.argv(3), "left") == 0)
						{
							target = 1;
							break;
						}
						if (Q_stricmp(gi.argv(3), "center") == 0)
						{
							target = 2;
							break;
						}
						if (Q_stricmp(gi.argv(3), "right") == 0)
						{
							target = 3;
							break;
						}

						//invalid arg
						if (target == 0)
						{
							gi.cprintf(ent, 1, "You did not enter a valid target!\n");
							return;
						}
						break;
					}

					//do the skill if target is working
					if (target != 0)
					{

						PartyAttack(ent, target, 10, true, true);

						client->warriorMP = client->warriorMP - 25;

						shieldBash = 2;

						if (!(Q_stricmp(info, "You have defeated an enemy!") == 0))
						{
							info = "You struck an enemy with your shield!";
							infoSet = true;
						}

						partyIndex++;
						PartyNextTurn(ent);

						CombatScreen(ent);
					}
					else
					{
						gi.cprintf(ent, 1, "Something went wrong!\n");
						return;
					}
				}
				//don't have enough mana
				else
				{
					gi.cprintf(ent, 1, "You don\'t have enough MP!\n");
					return;
				}
			}
		}
	}
}

//basic attack command
void Cmd_Attack_f(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	if (!client->inCombat)
	{
		gi.cprintf(ent, 1, "Can only attack in combat!\n");
		return;
	}
	if (!client->turn)
	{
		gi.cprintf(ent, 1, "Can only attack on your turn!\n");
		return;
	}
	edict_t* enemy;
	enemy = client->enemy;

	if (gi.argc() == 1)
	{
		if (numEnemies > 1)
		{
			gi.cprintf(ent, 1, "Please specify your target!\n");
			return;
		}
		
		if (numEnemies == 1)
		{
			PartyAttack(ent, 1, 0, true, true);


			if (!(Q_stricmp(info, "You have defeated an enemy!") == 0))
			{
				info = "You attacked an enemy!";
				infoSet = true;
			}

			partyIndex++;
			PartyNextTurn(ent);

			CombatScreen(ent);
			return;
		}
	}

	//multiple enemies
	if(gi.argc() == 2)
	{
		int target = 0;

		switch (numEnemies)
		{
		case 1:
			target = 1;
			break;
		case 2:
			if (Q_stricmp(gi.argv(1), "left") == 0)
			{
				target = 1;
				break;
			}
			if (Q_stricmp(gi.argv(1), "right") == 0)
			{
				target = 2;
				break;
			}

			//invalid arg
			if (target == 0)
			{
				gi.cprintf(ent, 1, "You did not enter a valid target!\n");
				return;
			}
			break;
		case 3:
			if (Q_stricmp(gi.argv(1), "left") == 0)
			{
				target = 1;
				break;
			}
			if (Q_stricmp(gi.argv(1), "center") == 0)
			{
				target = 2;
				break;
			}
			if (Q_stricmp(gi.argv(1), "right") == 0)
			{
				target = 3;
				break;
			}

			//invalid arg
			if (target == 0)
			{
				gi.cprintf(ent, 1, "You did not enter a valid target!\n");
				return;
			}
			break;
		}

		//do the attack if target is working
		if (target != 0)
		{

			PartyAttack(ent, target, 0, true, true);


			if (!(Q_stricmp(info, "You have defeated an enemy!") == 0))
			{
				info = "You attacked an enemy!";
				infoSet = true;
			}

			partyIndex++;
			PartyNextTurn(ent);

			CombatScreen(ent);
			return;
		}
		else
		{
			gi.cprintf(ent, 1, "Something went wrong!\n");
			return;
		}
	}
}

//use items in or out of combat
void Cmd_UseItem_f(edict_t* ent)
{
	gclient_t* client = ent->client;
	if (!client)
	{
		return;
	}

	if (!client->inCombat)
	{
		//in combat, you do not specify who gets the item because it will automatically go to whoever's turn it is
		if (gi.argc() == 2)
		{
			if (Q_stricmp(gi.argv(1), "bomb") == 0)
			{
				gi.cprintf(ent, 1, "Bommbs can only be used in combat!\n");
				return;
			}

			gi.cprintf(ent, 1, "Please specify which party member you'd like to use an item on!\n");
			return;
		}

		//out of combat, you must specify who gets the item
		if (gi.argc() == 3)
		{
			//health potion
			if (Q_stricmp(gi.argv(1), "potion") == 0)
			{
				int heal = 40;
				if (client->potions <= 0)
				{
					gi.cprintf(ent, 1, "You do not have any potions!\n");
					return;
				}

				if (Q_stricmp(gi.argv(2), "hero") == 0)
				{
					client->heroHealth += heal;
					if (client->heroHealth > 100)
					{
						client->heroHealth = 100;
					}

					gi.cprintf(ent, 1, "The hero now has %d health!\n", client->heroHealth);

					client->potions = client->potions - 1;
					return;
				}

				if (Q_stricmp(gi.argv(2), "ranger") == 0)
				{
					client->rangerHealth += heal;
					if (client->rangerHealth > 100)
					{
						client->rangerHealth = 100;
					}

					client->rangerDead = false;

					gi.cprintf(ent, 1, "The ranger now has %d health!\n", client->rangerHealth);

					client->potions = client->potions - 1;
					return;
				}

				if (Q_stricmp(gi.argv(2), "wizard") == 0)
				{
					client->wizardHealth += heal;
					if (client->wizardHealth > 100)
					{
						client->wizardHealth = 100;
					}

					client->wizardDead = false;

					gi.cprintf(ent, 1, "The wizard now has %d health!\n", client->wizardHealth);

					client->potions = client->potions - 1;
					return;
				}

				if (Q_stricmp(gi.argv(2), "warrior") == 0)
				{
					client->warriorHealth += heal;
					if (client->warriorHealth > 100)
					{
						client->warriorHealth = 100;
					}

					client->warriorDead = false;

					gi.cprintf(ent, 1, "The warrior now has %d health!\n", client->warriorHealth);

					client->potions = client->potions - 1;
					return;
				}
			}
		}

		//magic potion
		if (Q_stricmp(gi.argv(1), "magicpotion") == 0)
		{
			int mp = 40;
			if (client->magicPotions <= 0)
			{
				gi.cprintf(ent, 1, "You do not have any magic potions!\n");
				return;
			}

			if (Q_stricmp(gi.argv(2), "hero") == 0)
			{
				client->heroMP += mp;
				if (client->heroMP > 100)
				{
					client->heroMP = 100;
				}

				gi.cprintf(ent, 1, "The hero now has %d MP!\n", client->heroMP);

				client->magicPotions = client->magicPotions - 1;
				return;
			}

			if (Q_stricmp(gi.argv(2), "ranger") == 0)
			{
				client->rangerMP += mp;
				if (client->rangerMP > 100)
				{
					client->rangerMP = 100;
				}

				gi.cprintf(ent, 1, "The ranger now has %d MP!\n", client->rangerMP);

				client->magicPotions = client->magicPotions - 1;
				return;
			}

			if (Q_stricmp(gi.argv(2), "wizard") == 0)
			{
				client->wizardMP += mp;
				if (client->wizardMP > 100)
				{
					client->wizardMP = 100;
				}

				gi.cprintf(ent, 1, "The wizard now has %d MP!\n", client->wizardMP);

				client->magicPotions = client->magicPotions - 1;
				return;
			}

			if (Q_stricmp(gi.argv(2), "warrior") == 0)
			{
				client->warriorMP += mp;
				if (client->warriorMP > 100)
				{
					client->warriorMP = 100;
				}

				gi.cprintf(ent, 1, "The warrior now has %d MP!\n", client->warriorMP);

				client->magicPotions = client->magicPotions - 1;
				return;
			}
		}
	}
	//in combat
	else
	{
		edict_t* enemy = client->enemy;

		//turn exclusion
		if (!client->turn)
		{
			gi.cprintf(ent, 1, "You can only use items on your turn!\n");
			return;
		}

		if (gi.argc() == 2)
		{
			if (Q_stricmp(gi.argv(1), "potion") == 0)
			{
				if (client->potions <= 0)
				{
					gi.cprintf(ent, 1, "You do not have any potions!\n");
					return;
				}

				//apply effects
				int heal = 40;
				char* health;
				char* begin;
				char* end;

				int healthlen;
				int beginlen;
				int endlen;
				switch (partyIndex)
				{
				case CLASS_HERO:
					client->heroHealth += heal;
					if (client->heroHealth > 100)
					{
						client->heroHealth = 100;
					}

					//next turn
					partyIndex++;
					PartyNextTurn(ent);

					//get screen set up
					if (client->heroHealth == 100)
					{
						health = malloc(3);
					}
					if (client->heroHealth < 100 && client->heroHealth > 9)
					{
						health = malloc(2);
					}
					if (client->heroHealth < 10)
					{
						health = malloc(1);
					}
					sprintf(health, "%d", client->heroHealth);

					healthlen = strlen(health);
					begin = "The hero now has ";
					beginlen = strlen(begin);
					end = " health!";
					endlen = strlen(end);

					guide = malloc(healthlen + beginlen + endlen);
					memcpy(guide, begin, beginlen);
					memcpy(guide + beginlen, health, healthlen);
					memcpy(guide + beginlen + healthlen, end, endlen);
					guideSet = true;

					info = "The hero used a health potion!";
					infoSet = true;

					CombatScreen(ent);
					//gi.cprintf(ent, 1, "The hero now has %d health!\n", ent->health);

					client->potions = client->potions - 1;
					return;
					break;
				case CLASS_RANGER:
					client->rangerHealth += heal;
					if (client->rangerHealth > 100)
					{
						client->rangerHealth = 100;
					}

					client->rangerDead = false;

					//gi.cprintf(ent, 1, "The ranger now has %d health!\n", client->rangerHealth);
					//next turn
					partyIndex++;
					PartyNextTurn(ent);

					//get screen set up
					if (client->rangerHealth == 100)
					{
						health = malloc(3);
					}
					if (client->rangerHealth < 100 && client->rangerHealth > 9)
					{
						health = malloc(2);
					}
					if (client->rangerHealth < 10)
					{
						health = malloc(1);
					}
					sprintf(health, "%d", client->rangerHealth);

					healthlen = strlen(health);
					begin = "The ranger now has ";
					beginlen = strlen(begin);
					end = " health!";
					endlen = strlen(end);

					guide = malloc(healthlen + beginlen + endlen);
					memcpy(guide, begin, beginlen);
					memcpy(guide + beginlen, health, healthlen);
					memcpy(guide + beginlen + healthlen, end, endlen);
					guideSet = true;

					info = "The ranger used a health potion!";
					infoSet = true;

					CombatScreen(ent);

					client->potions = client->potions - 1;
					return;
					break;
				case CLASS_WIZARD:
					client->wizardHealth += heal;
					if (client->wizardHealth > 100)
					{
						client->wizardHealth = 100;
					}

					client->wizardDead = false;

					//gi.cprintf(ent, 1, "The wizard now has %d health!\n", client->wizardHealth);
					//next turn
					partyIndex++;
					PartyNextTurn(ent);

					//get screen set up

					if (client->wizardHealth == 100)
					{
						health = malloc(3);
					}
					if (client->wizardHealth < 100 && client->wizardHealth > 9)
					{
						health = malloc(2);
					}
					if (client->wizardHealth < 10)
					{
						health = malloc(1);
					}
					sprintf(health, "%d", client->wizardHealth);

					healthlen = strlen(health);
					begin = "The wizard now has ";
					beginlen = strlen(begin);
					end = " health!";
					endlen = strlen(end);

					guide = malloc(healthlen + beginlen + endlen);
					memcpy(guide, begin, beginlen);
					memcpy(guide + beginlen, health, healthlen);
					memcpy(guide + beginlen + healthlen, end, endlen);
					guideSet = true;

					info = "The wizard used a health potion!";
					infoSet = true;

					CombatScreen(ent);

					client->potions = client->potions - 1;
					return;
					break;
				case CLASS_WARRIOR:
					client->warriorHealth += heal;
					if (client->warriorHealth > 100)
					{
						client->warriorHealth = 100;
					}

					client->warriorDead = false;

					//gi.cprintf(ent, 1, "The warrior now has %d health!\n", client->warriorHealth);
					//next turn
					partyIndex++;
					PartyNextTurn(ent);

					//get screen set up

					if (client->warriorHealth == 100)
					{
						health = malloc(3);
					}
					if (client->warriorHealth < 100 && client->warriorHealth > 9)
					{
						health = malloc(2);
					}
					if (client->warriorHealth < 10)
					{
						health = malloc(1);
					}
					sprintf(health, "%d", client->warriorHealth);

					healthlen = strlen(health);
					begin = "The warrior now has ";
					beginlen = strlen(begin);
					end = " health!";
					endlen = strlen(end);

					guide = malloc(healthlen + beginlen + endlen);
					memcpy(guide, begin, beginlen);
					memcpy(guide + beginlen, health, healthlen);
					memcpy(guide + beginlen + healthlen, end, endlen);
					guideSet = true;

					info = "The warrior used a health potion!";
					infoSet = true;

					CombatScreen(ent);

					client->potions = client->potions - 1;
					return;
					break;
				}

			}

			if (Q_stricmp(gi.argv(1), "magicpotion") == 0)
			{
				if (client->magicPotions <= 0)
				{
					gi.cprintf(ent, 1, "You do not have any magic potions!\n");
					return;
				}

				//apply effects
				int mp = 40;
				char* magic;
				char* begin;
				char* end;

				int magiclen;
				int beginlen;
				int endlen;
				switch (partyIndex)
				{
				case CLASS_HERO:
					client->heroMP += mp;
					if (client->heroMP > 100)
					{
						client->heroMP = 100;
					}

					//next turn
					partyIndex++;
					PartyNextTurn(ent);

					//get screen set up
					if (client->heroMP == 100)
					{
						magic = malloc(3);
					}
					if (client->heroMP < 100 && client->heroMP > 9)
					{
						magic = malloc(2);
					}
					if (client->heroMP < 10)
					{
						magic = malloc(1);
					}
					sprintf(magic, "%d", client->heroMP);

					magiclen = strlen(magic);
					begin = "The hero now has ";
					beginlen = strlen(begin);
					end = " MP!";
					endlen = strlen(end);

					guide = malloc(magiclen + beginlen + endlen);
					memcpy(guide, begin, beginlen);
					memcpy(guide + beginlen, magic, magiclen);
					memcpy(guide + beginlen + magiclen, end, endlen);
					guideSet = true;

					info = "The hero used a magic potion!";
					infoSet = true;

					CombatScreen(ent);
					//gi.cprintf(ent, 1, "The hero now has %d health!\n", ent->health);

					client->magicPotions = client->magicPotions - 1;
					return;
					break;
				case CLASS_RANGER:
					client->rangerMP += mp;
					if (client->rangerMP > 100)
					{
						client->rangerMP = 100;
					}

					//gi.cprintf(ent, 1, "The ranger now has %d health!\n", client->rangerHealth);
					//next turn
					partyIndex++;
					PartyNextTurn(ent);

					//get screen set up
					if (client->rangerMP == 100)
					{
						magic = malloc(3);
					}
					if (client->rangerMP < 100 && client->rangerMP > 9)
					{
						magic = malloc(2);
					}
					if (client->rangerMP < 10)
					{
						magic = malloc(1);
					}
					sprintf(magic, "%d", client->rangerMP);

					magiclen = strlen(magic);
					begin = "The ranger now has ";
					beginlen = strlen(begin);
					end = " MP!";
					endlen = strlen(end);

					guide = malloc(magiclen + beginlen + endlen);
					memcpy(guide, begin, beginlen);
					memcpy(guide + beginlen, magic, magiclen);
					memcpy(guide + beginlen + magiclen, end, endlen);
					guideSet = true;

					info = "The ranger used a magic potion!";
					infoSet = true;

					CombatScreen(ent);

					client->magicPotions = client->magicPotions - 1;
					return;
					break;
				case CLASS_WIZARD:
					client->wizardMP += mp;
					if (client->wizardMP > 100)
					{
						client->wizardMP = 100;
					}

					//gi.cprintf(ent, 1, "The wizard now has %d health!\n", client->wizardHealth);
					//next turn
					partyIndex++;
					PartyNextTurn(ent);

					//get screen set up

					if (client->wizardMP == 100)
					{
						magic = malloc(3);
					}
					if (client->wizardMP < 100 && client->wizardMP > 9)
					{
						magic = malloc(2);
					}
					if (client->wizardMP < 10)
					{
						magic = malloc(1);
					}
					sprintf(magic, "%d", client->wizardMP);

					magiclen = strlen(magic);
					begin = "The wizard now has ";
					beginlen = strlen(begin);
					end = " MP!";
					endlen = strlen(end);

					guide = malloc(magiclen + beginlen + endlen);
					memcpy(guide, begin, beginlen);
					memcpy(guide + beginlen, magic, magiclen);
					memcpy(guide + beginlen + magiclen, end, endlen);
					guideSet = true;

					info = "The wizard used a magic potion!";
					infoSet = true;

					CombatScreen(ent);

					client->magicPotions = client->magicPotions - 1;
					return;
					break;
				case CLASS_WARRIOR:
					client->warriorMP += mp;
					if (client->warriorMP > 100)
					{
						client->warriorMP = 100;
					}

					//gi.cprintf(ent, 1, "The warrior now has %d health!\n", client->warriorHealth);
					//next turn
					partyIndex++;
					PartyNextTurn(ent);

					//get screen set up

					if (client->warriorMP == 100)
					{
						magic = malloc(3);
					}
					if (client->warriorMP < 100 && client->warriorMP > 9)
					{
						magic = malloc(2);
					}
					if (client->warriorMP < 10)
					{
						magic = malloc(1);
					}
					sprintf(magic, "%d", client->warriorMP);

					magiclen = strlen(magic);
					begin = "The warrior now has ";
					beginlen = strlen(begin);
					end = " MP!";
					endlen = strlen(end);

					guide = malloc(magiclen + beginlen + endlen);
					memcpy(guide, begin, beginlen);
					memcpy(guide + beginlen, magic, magiclen);
					memcpy(guide + beginlen + magiclen, end, endlen);
					guideSet = true;

					info = "The warrior used a magic potion!";
					infoSet = true;

					CombatScreen(ent);

					client->magicPotions = client->magicPotions - 1;
					return;
					break;
				}
			}

			if (Q_stricmp(gi.argv(1), "bomb") == 0)
			{
				if (client->bombs <= 0)
				{
					gi.cprintf(ent, 1, "You do not have any bombs!\n");
					return;
				}

				//damage all enemies in reverse order in case enemies die
				if(enemy->enemy3Type != MONSTER_NONE)
				{
					PartyAttack(ent, 3, 20, false, false);
				}
				if (enemy->enemy2Type != MONSTER_NONE)
				{
					PartyAttack(ent, 2, 20, false, false);
				}
				if (enemy->enemy1Type != MONSTER_NONE)
				{
					PartyAttack(ent, 1, 20, false, false);
				}

				client->bombs = client->bombs - 1;

				partyIndex++;
				PartyNextTurn(ent);

				info = "You used a bomb to damage all enemies!";
				infoSet = true;

				guide = "All enemies took 20 damage!";
				guideSet = true;

				CombatScreen(ent);
			}
		}
	}
}

//lists skills that characters can use
void Cmd_Skills_f(edict_t* ent)
{
	char* hero = "Hero Skills:\nHope --- Revives Fallen Allies \tCost:40\nHolyShield --- Temp Health for Allies \tCost:30\nSmite --- Powerful Attack, Ignores Shrouds \tCost:20\n";
	char* ranger = "Ranger Skills:\nArrowRain --- Strikes All \tCost:25\nHeartpiercer --- Powerful Attack, Extremely Effective Against Armor \tCost:30\n";
	char* wizard = "Wizard Skills:\nFireBall --- Strikes All, Powerful Against Weaker Enemies \tCost:35\nFrailtyCurse --- Halves Damage of an Enemy\'s Attack \tCost:25\nLightning --- Severely Damage An Enemy \tCost:40\n";
	char* warrior = "Warrior Skills:\nTaunt --- All Enemies Attack You Next Turn \tCost:35\nShieldBash --- Stronger Attack, Increased Defense for two Turns \tCost:25";

	//shows skills of current turn
	if (gi.argc() == 1 && ent->client->turn)
	{
		switch (partyIndex)
		{
		case CLASS_HERO:
			gi.cprintf(ent, 1, "%s", hero);
			return;
			break;
		case CLASS_RANGER:
			gi.cprintf(ent, 1, "%s", ranger);
			return;
			break;
		case CLASS_WIZARD:
			gi.cprintf(ent, 1, "%s", wizard);
			return;
			break;
		case CLASS_WARRIOR:
			gi.cprintf(ent, 1, "%s", warrior);
			return;
			break;
		}
	}
	else if (gi.argc() == 1 && !ent->client->turn)
	{
		gi.cprintf(ent, 1, "Please Specify Class\n");
		return;
	}

	if (Q_stricmp(gi.argv(1), "hero") == 0)
	{
		gi.cprintf(ent, 1, "%s",hero);
		return;
	}
	if (Q_stricmp(gi.argv(1), "ranger") == 0)
	{	
		gi.cprintf(ent, 1, "%s", ranger);
		return;
	}
	if (Q_stricmp(gi.argv(1), "wizard") == 0)
	{
		gi.cprintf(ent, 1, "%s", wizard);
		return;
	}
	if (Q_stricmp(gi.argv(1), "warrior") == 0)
	{
		gi.cprintf(ent, 1, "%s", warrior);
		return;
	}
}

void Cmd_Buy_f(edict_t* ent)
{
	char* name;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t* it_ent;
	int price;
	int quantity;

	//gi.cprintf(ent, 1, "%s\n", name);

	if (!shopOpen)
	{
		gi.centerprintf(ent, "Shop is closed!");
		return;
	}

	/*gi.cprintf(ent, 1, "Number of arguments: %d\n", gi.argc());
	gi.cprintf(ent, 1, "Args() gives: %s\n", gi.args());
	for (i = 0; i < gi.argc(); i++)
	{
		gi.cprintf(ent, 1, "new arg: %s\n",gi.argv(i));
		if (i == 2)
		{
			gi.cprintf(ent, 1, "Hopefully this is the right digit: %d\n", atoi(gi.argv(i)));
		}
	}*/
	//health potions
	if (Q_stricmp(gi.argv(1), "potion") == 0)
	{
		//gi.cprintf(ent, 1, "Got to potion. argc is: %d\n", gi.argc());

		if (gi.argc() == 3)
		{
			//gi.cprintf(ent, 1, "%d", atoi(gi.argv(2)));
			quantity = atoi(gi.argv(2));
			//gi.cprintf(ent, 1, "quantity: %d\n", quantity);
			price = 15 * quantity;
			if (price > ent->client->gold)
			{
				gi.centerprintf(ent, "You do not have enough gold!");
				return;
			}
			else
			{
				ent->client->gold = ent->client->gold - price;
				ent->client->potions = ent->client->potions + quantity;
				gi.centerprintf(ent, "Purchased");
				return;
			}
		}
		else
		{
			price = 15;
			if (price > ent->client->gold)
			{
				gi.centerprintf(ent, "You do not have enough gold!");
				return;
			}
			else
			{
				ent->client->gold = ent->client->gold - price;
				ent->client->potions = ent->client->potions + 1;
				gi.centerprintf(ent, "Purchased");
				return;
			}
		}
	}

	//MP potions
	if (Q_stricmp(gi.argv(1), "magicpotion") == 0)
	{
		//gi.cprintf(ent, 1, "Got to potion. argc is: %d\n", gi.argc());

		if (gi.argc() == 3)
		{
			//gi.cprintf(ent, 1, "%d", atoi(gi.argv(2)));
			quantity = atoi(gi.argv(2));
			//gi.cprintf(ent, 1, "quantity: %d\n", quantity);
			price = 20 * quantity;
			if (price > ent->client->gold)
			{
				gi.centerprintf(ent, "You do not have enough gold!");
				return;
			}
			else
			{
				ent->client->gold = ent->client->gold - price;
				ent->client->magicPotions = ent->client->magicPotions + quantity;
				gi.centerprintf(ent, "Purchased");
				return;
			}
		}
		else
		{
			price = 20;
			if (price > ent->client->gold)
			{
				gi.centerprintf(ent, "You do not have enough gold!");
				return;
			}
			else
			{
				ent->client->gold = ent->client->gold - price;
				ent->client->magicPotions = ent->client->magicPotions + 1;
				gi.centerprintf(ent, "Purchased");
				return;
			}
		}
	}

	if (Q_stricmp(gi.argv(1), "bomb") == 0)
	{
		//gi.cprintf(ent, 1, "Got to potion. argc is: %d\n", gi.argc());

		if (gi.argc() == 3)
		{
			//gi.cprintf(ent, 1, "%d", atoi(gi.argv(2)));
			quantity = atoi(gi.argv(2));
			//gi.cprintf(ent, 1, "quantity: %d\n", quantity);
			price = 5 * quantity;
			if (price > ent->client->gunpowder)
			{
				gi.centerprintf(ent, "You do not have enough gunpowder!");
				return;
			}
			else
			{
				ent->client->gunpowder = ent->client->gunpowder - price;
				ent->client->bombs = ent->client->bombs + quantity;
				gi.centerprintf(ent, "Purchased");
				return;
			}
		}
		else
		{
			price = 5;
			if (price > ent->client->gold)
			{
				gi.centerprintf(ent, "You do not have enough gunpowder!");
				return;
			}
			else
			{
				ent->client->gunpowder = ent->client->gunpowder - price;
				ent->client->bombs = ent->client->bombs + 1;
				gi.centerprintf(ent, "Purchased");
				return;
			}
		}
	}

	if (Q_stricmp(gi.argv(1), "hero") == 0)
	{
		if (gi.argc() == 3)
		{
			if (Q_stricmp(gi.argv(2), "armor") == 0)
			{
				switch (ent->client->heroArmor)
				{
					//no armor to light
				case ARMOR_NONE:
					price = 10;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->heroArmor = ARMOR_LIGHT;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
					//light armor to medium
				case ARMOR_LIGHT:
					price = 30;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->heroArmor = ARMOR_MEDIUM;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case ARMOR_MEDIUM:
					price = 15;
					if (price > ent->client->scales)
					{
						gi.centerprintf(ent, "You do not have enough dragon scales!");
						return;
					}
					else
					{
						ent->client->heroArmor = ARMOR_HEAVY;
						ent->client->scales = ent->client->scales - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case ARMOR_HEAVY:
					gi.centerprintf(ent, "You already have the best armor available for this party member!");
					break;
				default:
					gi.centerprintf(ent, "There was a problem");
				}
			}

			if (Q_stricmp(gi.argv(2), "weapon") == 0)
			{
				switch (ent->client->heroWeapon)
				{
					//basic weapon to iron
				case WEAPON_BASIC:
					price = 10;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->heroWeapon = WEAPON_IRON;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
					//iron weapon to darksteel
				case WEAPON_IRON:
					price = 30;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->heroWeapon = WEAPON_DARKSTEEL;
						ent->client->gold = ent->client->gold - price;
						return;
					}
					break;
				case WEAPON_DARKSTEEL:
					price = 15;
					if (price > ent->client->scales)
					{
						gi.centerprintf(ent, "You do not have enough dragon scales!");
						return;
					}
					else
					{
						ent->client->heroWeapon = WEAPON_DRAGONSCALE;
						ent->client->scales = ent->client->scales - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case WEAPON_DRAGONSCALE:
					gi.centerprintf(ent, "You already have the best weapon available for this party member!");
					break;
				default:
					gi.centerprintf(ent, "There was a problem");
				}
			}
		}
	}

	if (Q_stricmp(gi.argv(1), "ranger") == 0)
	{
		if (gi.argc() == 3)
		{
			if (Q_stricmp(gi.argv(2), "armor") == 0)
			{
				switch (ent->client->rangerArmor)
				{
					//no armor to light
				case ARMOR_NONE:
					price = 10;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->rangerArmor = ARMOR_LIGHT;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
					//light armor to medium
				case ARMOR_LIGHT:
					price = 30;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->rangerArmor = ARMOR_MEDIUM;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case ARMOR_MEDIUM:
					price = 15;
					if (price > ent->client->scales)
					{
						gi.centerprintf(ent, "You do not have enough dragon scales!");
						return;
					}
					else
					{
						ent->client->rangerArmor = ARMOR_HEAVY;
						ent->client->scales = ent->client->scales - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case ARMOR_HEAVY:
					gi.centerprintf(ent, "You already have the best armor available for this party member!");
					break;
				default:
					gi.centerprintf(ent, "There was a problem");
				}
			}

			if (Q_stricmp(gi.argv(2), "weapon") == 0)
			{
				switch (ent->client->rangerWeapon)
				{
					//basic weapon to iron
				case WEAPON_BASIC:
					price = 10;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->rangerWeapon = WEAPON_IRON;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
					//iron weapon to darksteel
				case WEAPON_IRON:
					price = 30;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->rangerWeapon = WEAPON_DARKSTEEL;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case WEAPON_DARKSTEEL:
					price = 15;
					if (price > ent->client->scales)
					{
						gi.centerprintf(ent, "You do not have enough dragon scales!");
						return;
					}
					else
					{
						ent->client->rangerWeapon = WEAPON_DRAGONSCALE;
						ent->client->scales = ent->client->scales - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case WEAPON_DRAGONSCALE:
					gi.centerprintf(ent, "You already have the best weapon available for this party member!");
					break;
				default:
					gi.centerprintf(ent, "There was a problem");
				}
			}
		}
	}

	if (Q_stricmp(gi.argv(1), "wizard") == 0)
	{
		if (gi.argc() == 3)
		{
			if (Q_stricmp(gi.argv(2), "armor") == 0)
			{
				switch (ent->client->wizardArmor)
				{
					//no armor to light
				case ARMOR_NONE:
					price = 10;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->wizardArmor = ARMOR_LIGHT;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
					//light armor to medium
				case ARMOR_LIGHT:
					price = 30;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->wizardArmor = ARMOR_MEDIUM;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case ARMOR_MEDIUM:
					price = 15;
					if (price > ent->client->scales)
					{
						gi.centerprintf(ent, "You do not have enough dragon scales!");
						return;
					}
					else
					{
						ent->client->wizardArmor = ARMOR_HEAVY;
						ent->client->scales = ent->client->scales - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case ARMOR_HEAVY:
					gi.centerprintf(ent, "You already have the best armor available for this party member!");
					break;
				default:
					gi.centerprintf(ent, "There was a problem");
				}
			}

			if (Q_stricmp(gi.argv(2), "weapon") == 0)
			{
				switch (ent->client->wizardWeapon)
				{
					//basic weapon to iron
				case WEAPON_BASIC:
					price = 10;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->wizardWeapon = WEAPON_IRON;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
					//iron weapon to darksteel
				case WEAPON_IRON:
					price = 30;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->wizardWeapon = WEAPON_DARKSTEEL;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case WEAPON_DARKSTEEL:
					price = 15;
					if (price > ent->client->scales)
					{
						gi.centerprintf(ent, "You do not have enough dragon scales!");
						return;
					}
					else
					{
						ent->client->wizardWeapon = WEAPON_DRAGONSCALE;
						ent->client->scales = ent->client->scales - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case WEAPON_DRAGONSCALE:
					gi.centerprintf(ent, "You already have the best weapon available for this party member!");
					break;
				default:
					gi.centerprintf(ent, "There was a problem");
				}
			}
		}
	}

	if (Q_stricmp(gi.argv(1), "warrior") == 0)
	{
		if (gi.argc() == 3)
		{
			if (Q_stricmp(gi.argv(2), "armor") == 0)
			{
				switch (ent->client->warriorArmor)
				{
					//no armor to light
				case ARMOR_NONE:
					price = 10;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->warriorArmor = ARMOR_LIGHT;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
					//light armor to medium
				case ARMOR_LIGHT:
					price = 30;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->warriorArmor = ARMOR_MEDIUM;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case ARMOR_MEDIUM:
					price = 15;
					if (price > ent->client->scales)
					{
						gi.centerprintf(ent, "You do not have enough dragon scales!");
						return;
					}
					else
					{
						ent->client->warriorArmor = ARMOR_HEAVY;
						ent->client->scales = ent->client->scales - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case ARMOR_HEAVY:
					gi.centerprintf(ent, "You already have the best armor available for this party member!");
					break;
				default:
					gi.centerprintf(ent, "There was a problem");
				}
			}

			if (Q_stricmp(gi.argv(2), "weapon") == 0)
			{
				switch (ent->client->warriorWeapon)
				{
					//basic weapon to iron
				case WEAPON_BASIC:
					price = 10;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->warriorWeapon = WEAPON_IRON;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
					//iron weapon to darksteel
				case WEAPON_IRON:
					price = 30;
					if (price > ent->client->gold)
					{
						gi.centerprintf(ent, "You do not have enough gold!");
						return;
					}
					else
					{
						ent->client->warriorWeapon = WEAPON_DARKSTEEL;
						ent->client->gold = ent->client->gold - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case WEAPON_DARKSTEEL:
					price = 15;
					if (price > ent->client->scales)
					{
						gi.centerprintf(ent, "You do not have enough dragon scales!");
						return;
					}
					else
					{
						ent->client->warriorWeapon = WEAPON_DRAGONSCALE;
						ent->client->scales = ent->client->scales - price;
						gi.centerprintf(ent, "Purchased");
						return;
					}
					break;
				case WEAPON_DRAGONSCALE:
					gi.centerprintf(ent, "You already have the best weapon available for this party member!");
					break;
				default:
					gi.centerprintf(ent, "There was a problem");
				}
			}
		}
	}
}

void Cmd_Gold_f(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	client->gold = client->gold + 500;
	gi.cprintf(ent, 1, "You now have %d gold\n", ent->client->gold);
}

void Cmd_Scales_f(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}

	client->scales = client->scales + 100;
	gi.cprintf(ent, 1, "You now have %d scales\n", ent->client->scales);
}

void Cmd_Gunpowder_f(edict_t* ent)
{
	ent->client->gunpowder = ent->client->gunpowder + 50;
	gi.cprintf(ent, 1, "You now have %d gunpowder\n", ent->client->gunpowder);
}

//Handles win state
void Cmd_CombatWon_f(edict_t* ent)
{
	gclient_t* client;

	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}

	edict_t* enemy;
	enemy = client->enemy;

	client->gold = client->gold + enemy->goldValue;
	client->scales = client->scales + enemy->scaleValue;
	client->gunpowder = client->gunpowder + enemy->gunpowderValue;

	if (client->inCombat)
	{
		client->inCombat = false;
	}

	if (ent->client->showhelp)
	{
		Cmd_Help_f(ent);
	}

	gi.centerprintf(ent, "You Won!");
}



//main combat function that wasn't used at all pay no attention to this lol
void Cmd_Combat_f(edict_t* ent)
{
	gclient_t* client;

	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}

	edict_t* enemy;
	enemy = client->enemy;

	if (client->turn)
	{

	}
}

void Cmd_Run_f(edict_t* ent)
{
	gclient_t* client;
	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	if (client->inCombat && client->turn)
	{
		client->inCombat = false;
		Cmd_CleanValues_f(ent);
		client->enemy = NULL;

		if(ent->client->showhelp)
		{ 
			Cmd_Help_f(ent);
		}

		gi.centerprintf(ent, "Fled from combat!");
	}
	else if (!client->inCombat)
	{
		gi.centerprintf(ent, "You are not in combat!");
	}
	else if (!client->turn)
	{
		gi.cprintf(ent, 1, "You can only run on your turn!\n");
	}

}


//assigns loot and stat values based on enemy type
void Cmd_LootStatVals_f(edict_t* ent, int enemyType, int numEnemy)
{
	gclient_t* client;

	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}

	edict_t* enemy;
	enemy = client->enemy;
	switch (numEnemy)
	{
	//enemy number 1
	case 1:
		switch (enemyType)
		{
		case MONSTER_GOBLIN:
			//setting enemy type and health
			enemy->enemy1Type = MONSTER_GOBLIN;
			enemy->enemy1Health = 40;

			//loot values
			enemy->gunpowderValue = enemy->gunpowderValue + 3;
			enemy->goldValue = enemy->goldValue + 5;
			break;

		case MONSTER_ORC:
			//setting enemy type and health
			enemy->enemy1Type = MONSTER_ORC;
			enemy->enemy1Health = 60;

			//loot values
			enemy->goldValue = enemy->goldValue + 15;
			break;

		case MONSTER_DRAKE:
			//setting enemy type and health
			enemy->enemy1Type = MONSTER_DRAKE;
			enemy->enemy1Health = 100;

			//loot values
			enemy->scaleValue = enemy->scaleValue + 10;
			enemy->goldValue = enemy->goldValue + 30;
			break;

		case MONSTER_DEMON:
			//setting enemy type and health
			enemy->enemy1Type = MONSTER_DEMON;
			enemy->enemy1Health = 160;
			
			//if a demon gets the drop on the player, it gets to have it's shroud on the first turn
			//if (!client->turn)
			//{
			//decided to give demons their shroud no matter what to make them more interesting
			enemy->demonShroud1 = true;
			//}

			//loot values
			enemy->gunpowderValue = enemy->gunpowderValue + 10;
			enemy->goldValue = enemy->goldValue + 60;
			break;

		case MONSTER_DRAGON:
			//setting enemy type and health
			enemy->enemy1Type = MONSTER_DRAGON;
			enemy->enemy1Health = 180;

			//loot values
			enemy->scaleValue = enemy->scaleValue + 30;
			enemy->goldValue = enemy->goldValue + 50;
			break;
		}
		break;

	//enemy number 2 (cannot be a dragon)
	case 2:
		switch (enemyType)
		{
		case MONSTER_NONE:
			enemy->enemy2Type = MONSTER_NONE;
			break;

		case MONSTER_GOBLIN:
			//setting enemy type and health
			enemy->enemy2Type = MONSTER_GOBLIN;
			enemy->enemy2Health = 40;

			//loot values
			enemy->gunpowderValue = enemy->gunpowderValue + 3;
			enemy->goldValue = enemy->goldValue + 5;
			break;

		case MONSTER_ORC:
			//setting enemy type and health
			enemy->enemy2Type = MONSTER_ORC;
			enemy->enemy2Health = 60;

			//loot values
			enemy->goldValue = enemy->goldValue + 15;
			break;

		case MONSTER_DRAKE:
			//setting enemy type and health
			enemy->enemy2Type = MONSTER_DRAKE;
			enemy->enemy2Health = 100;

			//loot values
			enemy->scaleValue = enemy->scaleValue + 10;
			enemy->goldValue = enemy->goldValue + 20;
			break;

		case MONSTER_DEMON:
			//setting enemy type and health
			enemy->enemy2Type = MONSTER_DEMON;
			enemy->enemy2Health = 160;

			//if a demon gets the drop on the player, it gets to have it's shroud on the first turn
			//if (!client->turn)
			//{
			//decided to give demons their shroud no matter what
			enemy->demonShroud2 = true;
			//}

			//loot values
			enemy->gunpowderValue = enemy->gunpowderValue + 10;
			enemy->goldValue = enemy->goldValue + 60;
			break;
		}
		break;

	//enemy number 3 can only be an orc or a goblin
	case 3:
		switch (enemyType)
		{
		case MONSTER_NONE:
			enemy->enemy3Type = MONSTER_NONE;
			break;

		case MONSTER_GOBLIN:
			//setting enemy type and health
			enemy->enemy3Type = MONSTER_GOBLIN;
			enemy->enemy3Health = 40;

			//loot values
			enemy->gunpowderValue = enemy->gunpowderValue + 3;
			enemy->goldValue = enemy->goldValue + 5;
			break;

		case MONSTER_ORC:
			//setting enemy type and health
			enemy->enemy3Type = MONSTER_ORC;
			enemy->enemy3Health = 60;

			//loot values
			enemy->goldValue = enemy->goldValue + 15;
			break;
		}
	}
	gi.cprintf(ent, 1, "Monster type is: %d\n", enemyType);
}

qboolean firstCombat = true;
int numEnemies = 0;
void Cmd_CombatBegin_f(edict_t* ent)
{
	int i;
	int enemyType;
	gclient_t* client;

	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}
	//sets starting stats
	/*if (firstCombat)
	{
		client->heroMP = 100;

		client->rangerHealth = 100;
		client->rangerMP = 100;

		client->wizardHealth = 100;
		client->wizardMP = 100;

		client->warriorHealth = 100;
		client->warriorMP = 100;

		firstCombat = false;
	}*/

	//determines number of enemies for combat
	numEnemies = (int)(crandom() * 3);
	if (numEnemies < 0)
	{
		numEnemies = numEnemies * -1;
	}
	numEnemies += 1;

	while (numEnemies == 0)
	{
		numEnemies = (int)(crandom() * 3);
		if (numEnemies < 0)
		{
			numEnemies = numEnemies * -1;
		}
		numEnemies += 1;
	}

	gi.cprintf(ent, 1, "Number of enemies: %d\n", numEnemies);
	//determines enemy types and loot values
	switch (numEnemies)
	{
		//spawns stronger enemies if lower number of enemies
	case 1:
		//range
		enemyType = (int)(crandom() * 5);

		//invert negatives
		if (enemyType < 0)
		{
			enemyType = enemyType * -1;
		}

		//increase min values
		enemyType += 3;

		//skewing the values to make drakes and shadow demons spawn more often
		if (enemyType == 4)
		{
			enemyType = 3;
		}
		if (enemyType == 5 || enemyType == 6)
		{
			enemyType = 4;
		}
		if (enemyType == 7)
		{
			enemyType = 5;
		}
	
		//repeat unitl non zero answer
		while (enemyType == 0)
		{
			enemyType = (int)(crandom() * 3);
			if (enemyType < 0)
			{
				enemyType = enemyType * -1;
			}
			enemyType += 3;
			//skewing the values to make drakes and shadows spawn more often
			if (enemyType == 4)
			{
				enemyType = 3;
			}
			if (enemyType == 5 || enemyType == 6)
			{
				enemyType = 4;
			}
			if (enemyType == 7)
			{
				enemyType = 5;
			}
		}

		Cmd_LootStatVals_f(ent, enemyType, 1);
		Cmd_LootStatVals_f(ent, MONSTER_NONE, 2);
		Cmd_LootStatVals_f(ent, MONSTER_NONE, 3);
		break;

		//can spawn some minibosses, but they are rarer than goblins or orcs
	case 2:
		for (i = 1; i < 3; i++)
		{
			//range
			enemyType = (int)(crandom() * 6);

			//invert negatives
			if (enemyType < 0)
			{
				enemyType = enemyType * -1;
			}

			//increase min values
			enemyType += 1;

			//skewing the values to make goblins and orcs spawn more often
			if (enemyType == 2)
			{
				enemyType = 1;
			}
			if (enemyType == 3 || enemyType == 4)
			{
				enemyType = 2;
			}
			if (enemyType == 5)
			{
				enemyType = 3;
			}
			if (enemyType == 6)
			{
				enemyType = 4;
			}
			//repeat unitl non zero answer
			while (enemyType == 0)
			{
				enemyType = (int)(crandom() * 6);
				if (enemyType < 0)
				{
					enemyType = enemyType * -1;
				}
				enemyType += 1;

				//skewing the values to make goblins and orcs spawn more often
				if (enemyType == 2)
				{
					enemyType = 1;
				}
				if (enemyType == 3 || enemyType == 4)
				{
					enemyType = 2;
				}
				if (enemyType == 5)
				{
					enemyType = 3;
				}
				if (enemyType == 6)
				{
					enemyType = 4;
				}
			}
			Cmd_LootStatVals_f(ent, enemyType, i);
		}
		Cmd_LootStatVals_f(ent, MONSTER_NONE, 3);
		break;

		//only goblins or orcs can spawn when there are three enemies
	case 3:
		for (i = 1; i < 4; i++)
		{
			//range
			enemyType = (int)(crandom() * 2);

			//invert negatives
			if (enemyType < 0)
			{
				enemyType = enemyType * -1;
			}

			//increase min values
			enemyType += 1;

			//repeat unitl non zero answer
			while (enemyType == 0)
			{
				enemyType = (int)(crandom() * 2);
				if (enemyType < 0)
				{
					enemyType = enemyType * -1;
				}
				enemyType += 1;
			}
			Cmd_LootStatVals_f(ent, enemyType, i);
		}
		break;
	}
}


qboolean test;
void Cmd_Test_f (edict_t* ent)
{
	//G_FreeEdict(ent->client->enemy);
	if (ent->client->inCombat && !ent->client->turn)
	{
		ent->client->turn = true;
	}

	if (!ent->client->inCombat)
	{
		return;
	}
	if (gi.argc() == 2)
	{
		partyIndex = atoi(gi.argv(1));
	}
}

//calls help screen and resets it, telling the player what the combat state is
void Cmd_Guide_f(edict_t* ent)
{
	CombatScreen(ent);
}

void Cmd_Turn_f(edict_t* ent)
{
	gclient_t* client;

	if (ent->client)
	{
		client = ent->client;
	}
	else
	{
		return;
	}

	if (!client->inCombat)
	{
		gi.cprintf(ent, 1, "Not in combat!\n");
		return;
	}

	if (!client->turn)
	{
		gi.cprintf(ent, 1, "It is the monster\'s turn! Press \'G\' to continue\n");
		return;
	}

	switch (partyIndex)
	{
	case CLASS_HERO:
		gi.cprintf(ent, 1, "It is the hero\'s turn!\n");
		return;
		break;
	case CLASS_RANGER:
		gi.cprintf(ent, 1, "It is the ranger\'s turn!\n");
		return;
		break;
	case CLASS_WIZARD:
		gi.cprintf(ent, 1, "It is the wizard\'s turn!\n");
		return;
		break;
	case CLASS_WARRIOR:
		gi.cprintf(ent, 1, "It is the warrior\'s turn!\n");
		return;
		break;
	}
}
/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	if (Q_stricmp(cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "help") == 0)
	{
		Cmd_Help_f (ent);
		return;
	}

	//mod commands
	if (Q_stricmp(cmd, "shop") == 0)
	{
		Cmd_Shop_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "buy") == 0)
	{
		Cmd_Buy_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "weapons") == 0)
	{
		Cmd_WeaponPrices_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "armor") == 0)
	{
		Cmd_ArmorPrices_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "consumable") == 0)
	{
		Cmd_ConsumablePrices_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "resources") == 0)
	{
		Cmd_Resources_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "items") == 0)
	{
		Cmd_Items_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "gold") == 0)
	{
		Cmd_Gold_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "scales") == 0)
	{
		Cmd_Scales_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "gunpowder") == 0)
	{
		Cmd_Gunpowder_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "run") == 0)
	{
		Cmd_Run_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "roll") == 0)
	{
		Cmd_Roll_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "test") == 0)
	{
		Cmd_Test_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "skills") == 0)
	{
		Cmd_Skills_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "useskill") == 0)
	{
		Cmd_UseSkill_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "useitem") == 0)
	{
		Cmd_UseItem_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "guide") == 0)
	{
		Cmd_Guide_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "attack") == 0)
	{
		Cmd_Attack_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "turn") == 0)
	{
		Cmd_Turn_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "monster") == 0)
	{
		Cmd_MonsterBehave_f(ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_stricmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	else if (Q_stricmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "inven") == 0)
		Cmd_Inven_f (ent);
	else if (Q_stricmp (cmd, "invnext") == 0)
		SelectNextItem (ent, -1);
	else if (Q_stricmp (cmd, "invprev") == 0)
		SelectPrevItem (ent, -1);
	else if (Q_stricmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_stricmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_stricmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_stricmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	else if (Q_stricmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);
	else if (Q_stricmp (cmd, "wave") == 0)
		Cmd_Wave_f (ent);
	else if (Q_stricmp(cmd, "playerlist") == 0)
		Cmd_PlayerList_f(ent);
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
