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

#include "g_local.h"

fileHandle_t log_handle = -1;

void log_close(void)
{
	if ( log_handle < 0 )
		return;

// close does't require this check
//	if ( !cvar("k_extralog"))
//		return;

	trap_FS_CloseFile( log_handle );
	log_handle = -1;
}

void log_open( const char *fmt, ... )
{
	va_list argptr;
	char	text[1024] = {0};
 
	if ( !cvar("k_extralog") )
		return;

	log_close();

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	text[sizeof(text)-1] = 0;
 
	if ( trap_FS_OpenFile( text, &log_handle, FS_APPEND_BIN ) < 0 )
	{
		log_handle = -1;
		return;
	}
}
 
void log_printf( const char *fmt, ... )
{
	va_list argptr;
	char	text[1024] = {0};

	if ( log_handle < 0 )
		return;
        
	if ( !cvar("k_extralog") )
		return;

	va_start( argptr, fmt );
	Q_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	text[sizeof(text)-1] = 0;

	trap_FS_WriteFile( text, strlen(text), log_handle );
}

char *GetMode();
void StartLogs()
{
	char date[64] = {0}, date_c[64] = {0}, *ip = "", *port = "";
	int i = 0;

	if ( strnull( ip = cvar_string( "sv_local_addr" ) ) || strnull( port = strchr(ip, ':') ) || !(i = atoi(port + 1)) )
		return;

	port[0] = 0;
	port++;

	if ( !QVMstrftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S %Z", 0) )
		date[0] = 0; // bad date
	if ( !QVMstrftime(date_c, sizeof(date_c), "%Y%m%d-%H%M%S-%Z", 0) )
		date_c[0] = 0; // bad date

	log_open("%s", cvar_string("extralogname"));
	log_printf("%s", "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");

	log_printf("%s", "<!DOCTYPE KTXLOG [\n");
	log_printf("%s", "<!ELEMENT KTXLOG (DESCRIPTION,EVENTS)>\n");
	log_printf("%s", "<!ELEMENT DESCRIPTION (#PCDATA)>\n");
	log_printf("%s", "<!ELEMENT EVENTS (DAMAGE*,PICKMI*,PICKBP*,DROPBP*,DROPPU*,DEATH*)>\n");
	log_printf("%s", "<!ELEMENT DAMAGE (#PCDATA)>\n");
	log_printf("%s", "<!ELEMENT PICKMI (#PCDATA)>\n");
	log_printf("%s", "<!ELEMENT PICKBP (#PCDATA)>\n");
	log_printf("%s", "<!ELEMENT DROPBP (#PCDATA)>\n");
	log_printf("%s", "<!ELEMENT DROPPU (#PCDATA)>\n");
	log_printf("%s", "<!ELEMENT DEATH (#PCDATA)>\n");
	log_printf("%s", "<!ATTLIST KTXLOG VERSION CDATA \"0.1\">\n");
	log_printf("%s", "<!ATTLIST DESCRIPTION TIMESTAMP CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DESCRIPTION HOSTNAME CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DESCRIPTION IP CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DESCRIPTION PORT CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DESCRIPTION MAP CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DESCRIPTION MODE CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DAMAGE TIME CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DAMAGE ATTACKER CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DAMAGE TARGET CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DAMAGE TYPE CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DAMAGE QUAD (0|1) #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DAMAGE SPLASH (0|1) #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DAMAGE VALUE CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DAMAGE ARMOR (0|1) #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST PICKMI TIME CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST PICKMI ITEM CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST PICKMI PLAYER CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST PICKMI VALUE CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST PICKBP TIME CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST PICKBP WEAPON CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST PICKBP SHELLS CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST PICKBP NAILS CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST PICKBP ROCKETS CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST PICKBP CELLS CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST PICKBP PLAYER CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DROPBP TIME CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DROPBP WEAPON CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DROPBP SHELLS CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DROPBP NAILS CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DROPBP ROCKETS CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DROPBP CELLS CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DROPBP PLAYER CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DROPPU TIME CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DROPPU ITEM	CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DROPPU PLAYER CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DROPPU TIMELEFT CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DEATH TIME CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DEATH ATTACKER CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DEATH TARGET CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DEATH TYPE CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DEATH QUAD (0|1) #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DEATH ARMORLEFT CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DEATH KILLHEIGHT CDATA #REQUIRED>\n");
	log_printf("%s", "<!ATTLIST DEATH LIFETIME CDATA #REQUIRED>\n");
	log_printf("%s", "]>\n");

	log_printf("%s", "<ktxlog version=\"%s\">\n", MOD_VERSION);
	log_printf("\t<description timestamp=\"%s\" hostname=\"%s\" ip=\"%s\" port=\"%d\" map=\"%s\" mode=\"%s\">\n", 
		date, striphigh(cvar_string("hostname")), ip, i, g_globalvars.mapname, GetMode());
	log_printf("\t\t<events>\n");	
}

void StopLogs()
{
	log_printf("\t\t</events>\n" );
	log_printf("\t</description>\n" );
	log_printf("</ktxlog>\n" );
	log_close();
}

