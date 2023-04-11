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
qboolean shopOpen = false;
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
	gi.cprintf(ent, 1, "Weapon Upgrades\nIron\t10g\nDarksteel\t30g\nDragon Scale\t15 Scales\n");
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
	gi.cprintf(ent, 1, "Armor Upgrades\nLight\t10g\nDarksteel\t30g\nDragon Scale\t15 Scales\n");
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
	gi.cprintf(ent, 1, "Consumables \nHealth Potion\t15g\nMagic Potion\t20g\nBomb\t5 Gunpowder\n");
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
		gi.cprintf(ent,1,"Current Gold:\t%d\nCurrent Scales:\t%d\nCurrent Gunpowder:\t%d", client->gold,client->scales,client->gunpowder);
	}
	else
	{
		gi.centerprintf(ent,"Cannot shop while in combat.");
	}
}

void Cmd_Items_f(edict_t* ent)
{
	gi.cprintf(ent, 1, "Potions:\t\t%d\nMagic Potions:\t%d\nBombs\t\t%d\n",ent->client->potions, ent->client->magicPotions, ent->client->bombs);
}

void Cmd_Stats_f(edict_t* ent)
{
	//to be finished
}

void Cmd_Buy_f(edict_t* ent)
{
	char* name;;
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
				ent->client->potions = ent->client->potions + quantity;
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
				ent->client->potions = ent->client->potions + 1;
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
				ent->client->potions = ent->client->potions + quantity;
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
				ent->client->potions = ent->client->potions + 1;
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
	if (client->inCombat)
	{
		client->inCombat = false;
		client->enemy = NULL;

		if(client->showhelp == true)
		Cmd_Help_f(ent);
		
		gi.centerprintf(ent, "Fled from combat!");
	}
	else
	{
		gi.centerprintf(ent, "You are not in combat!");
	}
}

//if battle ends or you run from battle, enemy stats and values are cleaned
void Cmd_CleanValues_f(edict_t* ent)
{

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

			//loot values
			enemy->gunpowderValue = enemy->gunpowderValue + 10;
			enemy->goldValue = enemy->goldValue + 60;
			break;

		case MONSTER_DRAGON:
			//setting enemy type and health
			enemy->enemy1Type = MONSTER_DRAKE;
			enemy->enemy1Health = 180;

			//loot values
			enemy->scaleValue = enemy->scaleValue + 30;
			enemy->goldValue = enemy->goldValue + 50;
			break;
		}
		break;

	//enemy number 2
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

			//loot values
			enemy->goldValue = enemy->goldValue + 60;
			break;
		}
		break;

	//enemy number 3
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
	if (firstCombat)
	{
		client->heroMP = 100;

		client->rangerHealth = 100;
		client->rangerMP = 100;

		client->wizardHealth = 100;
		client->wizardMP = 100;

		client->warriorHealth = 100;
		client->warriorMP = 100;

		firstCombat = false;
	}

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
		enemyType = (int)(crandom() * 3);

		//invert negatives
		if (enemyType < 0)
		{
			enemyType = enemyType * -1;
		}

		//increase min values
		enemyType += 3;

		//repeat unitl non zero answer
		while (enemyType == 0)
		{
			enemyType = (int)(crandom() * 3);
			if (enemyType < 0)
			{
				enemyType = enemyType * -1;
			}
			enemyType += 3;
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

//function for testing random
void Cmd_Roll_f(edict_t* ent)
{
	int i;
	int random;
	for (i = 0; i < 5; i++)
	{
		//range
		random = (int)(crandom() * 2);

		//invert negatives
		if (random < 0)
		{
			random = random * -1;
		}

		//increase min values
		random += 1;

		//repeat unitl non zero answer
		while (random == 0)
		{
			random = (int)(crandom() * 2);
			if (random < 0)
			{
				random = random * -1;
			}
			random += 1;
		}
		gi.cprintf(ent, 1, "random int: %d\n", random);
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

	if (Q_stricmp (cmd, "players") == 0)
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
