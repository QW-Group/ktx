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
 *  $Id: vip.c,v 1.2 2006/04/11 20:38:33 qqshka Exp $
 */

// vip.c

// qqshka:
// put here some vip stuff %)

#include "g_local.h"

// get vip bit flags of client, if any

int Vip_Flags(gedict_t* cl)
{
// qqshka: is it possible to fake *VIP info with cheaty client? imho yes, so replace with more secure way
//	return atoi( ezinfokey(cl, "*VIP") );
	return cl->vip;
}

// check if client have ALL bit 'flags'

int Vip_IsFlags(gedict_t* cl, int flags)
{
 	return ( ( Vip_Flags( cl ) & flags ) == flags );
}

void Vip_ShowRights(gedict_t* cl)
{
	int flags = Vip_Flags( cl );
	char *rights = "";

	if ( !flags )
		return;

	if ( flags & VIP_ADMIN ) {
		flags &= ~VIP_ADMIN;
		rights = va("%s admin", rights);
	}

	if ( strnull( rights ) || flags )
		rights = va("%s UNKNOWN", rights);

	G_sprint(cl /* self */, 2, "You are a VIP with rights:%s\n", rights);
}

