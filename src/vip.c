/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id$
 */

// vip.c

// qqshka:
// put here some vip stuff %)

#include "g_local.h"

// get vip bit flags of client, if any

int VIP(gedict_t* cl)
{
	char vip[10];

  return 0; //atoi(infokey(cl, "*VIP", vip, sizeof(vip)));
}

// check if client have ALL bit 'flags'

int VIP_IsFlags(gedict_t* cl, int flags)
{
 	return ( ( VIP( cl ) & flags ) == flags );
}

void VIP_ShowRights(gedict_t* cl)
{
	int flags = VIP( cl );
	char *rights = "";

	if ( !flags )
		return;

	if ( flags & VIP_NORMAL ) {
		flags &= ~VIP_NORMAL;
		rights = va("%s normal", rights);
	}

	if ( flags & VIP_NOTKICKABLE ) {
		flags &= ~VIP_NOTKICKABLE;
		rights = va("%s not_kick", rights);
	}

	if ( flags & VIP_ADMIN ) {
		flags &= ~VIP_ADMIN;
		rights = va("%s admin", rights);
	}

	if ( flags & VIP_RCON ) {
		flags &= ~VIP_RCON;
		rights = va("%s rcon_adm", rights);
	}

	if ( strnull( rights ) || flags )
		rights = va("%s UNKNOWN", rights);

	G_sprint(cl, 2, "You are a VIP with rights:%s\n", rights);
}

