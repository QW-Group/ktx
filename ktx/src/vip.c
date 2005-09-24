
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

