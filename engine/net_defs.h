// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// DESCRIPTION:
//     Definitions for use in networking code.
//
//-----------------------------------------------------------------------------

#ifndef NET_DEFS_H
#define NET_DEFS_H

#include "doomdef.h"
#include "doomtype.h"
#include "d_ticcmd.h"

#define MAX_MODULES 16

typedef struct _net_packet_s
{
	unsigned char* data;
	unsigned int len;
	unsigned int alloced;
	unsigned int pos;
}net_packet_t;

typedef struct net_module_s
{
	// Initialise this module for use as a client

	boolean(*InitClient)(void);

	// Initialise this module for use as a server

	boolean(*InitServer)(void);

	// Send a packet

	void (*SendPacket)(struct net_addr_s* addr, net_packet_t* packet);

	// Check for new packets to receive
	//
	// Returns true if packet received

	boolean(*RecvPacket)(struct net_addr_s** addr, net_packet_t** packet);

	// Converts an address to a string

	void (*AddrToString)(struct net_addr_s* addr, char* buffer, int buffer_len);

	// Free back an address when no longer in use

	void (*FreeAddress)(struct net_addr_s* addr);

	// Try to resolve a name to an address

	struct net_addr_s* (*ResolveAddress)(char* addr);
}net_module_t;

typedef struct net_addr_s
{
	net_module_t* module;
	void* handle;
}net_addr_t;


// net_addr_t
typedef struct net_context_s
{
	net_module_t* modules[MAX_MODULES];
	int num_modules;
}net_context_t;

// magic number sent when connecting to check this is a valid client

#define NET_MAGIC_NUMBER 3436803284U

// header field value indicating that the packet is a reliable packet

#define NET_RELIABLE_PACKET (1 << 15)

// packet types

typedef enum
{
	NET_PACKET_TYPE_SYN,
	NET_PACKET_TYPE_ACK,
	NET_PACKET_TYPE_REJECTED,
	NET_PACKET_TYPE_KEEPALIVE,
	NET_PACKET_TYPE_WAITING_DATA,
	NET_PACKET_TYPE_GAMESTART,
	NET_PACKET_TYPE_GAMEDATA,
	NET_PACKET_TYPE_GAMEDATA_ACK,
	NET_PACKET_TYPE_DISCONNECT,
	NET_PACKET_TYPE_DISCONNECT_ACK,
	NET_PACKET_TYPE_RELIABLE_ACK,
	NET_PACKET_TYPE_GAMEDATA_RESEND,
	NET_PACKET_TYPE_CONSOLE_MESSAGE,
	NET_PACKET_TYPE_QUERY,
	NET_PACKET_TYPE_QUERY_RESPONSE,
	NET_PACKET_TYPE_CVAR_UPDATE,
	NET_PACKET_TYPE_CHEAT_REQUEST,
} net_packet_type_t;

typedef struct
{
	int ticdup;
	int extratics;
	int deathmatch;
	int nomonsters;
	int fast_monsters;
	int respawn_monsters;
	int respawn_items;
	int map;
	int skill;
	int gameversion;
	int lowres_turn;
	int new_sync;
	int timelimit;
	int loadgame;
	int compatflags;
	int gameflags;
} net_gamesettings_t;

#define NET_TICDIFF_FORWARD      (1 << 0)
#define NET_TICDIFF_SIDE         (1 << 1)
#define NET_TICDIFF_TURN         (1 << 2)
#define NET_TICDIFF_BUTTONS      (1 << 3)
#define NET_TICDIFF_CONSISTANCY  (1 << 4)
#define NET_TICDIFF_CHATCHAR     (1 << 5)
#define NET_TICDIFF_BUTTONS2     (1 << 6)
#define NET_TICDIFF_PITCH		 (1 << 7)

typedef struct
{
	int diff;
	ticcmd_t cmd;
} net_ticdiff_t;

// Complete set of ticcmds from all players

typedef struct
{
	int latency;
	unsigned int seq;
	boolean playeringame[MAXPLAYERS];
	net_ticdiff_t cmds[MAXPLAYERS];
} net_full_ticcmd_t;

// Data sent in response to server queries

typedef struct
{
	char* version;
	int server_state;
	int num_players;
	int max_players;
	int gamemode;
	int gamemission;
	char* description;
} net_querydata_t;

#endif /* #ifndef NET_DEFS_H */