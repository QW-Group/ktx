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
        if (log_handle < 0 )
                return;
 
        if ( !cvar("k_extralog")) 
		return;

        trap_FS_CloseFile( log_handle );
        log_handle = -1;
}
 
void log_open( const char *fmt, ... )
{
        va_list argptr;
        char    text[1024];
 
        if ( !cvar("k_extralog")) 
		return;

	va_start( argptr, fmt );
        Q_vsnprintf( text, sizeof(text), fmt, argptr );
        va_end( argptr );
 
        log_close();
 
        if ( trap_FS_OpenFile( text, &log_handle, FS_APPEND_BIN ) < 0 )
        {
                log_handle = -1;
                return;
        }
}
 
void log_printf( const char *fmt, ... )
{
        va_list argptr;
        char    text[1024];
        
	if ( !cvar("k_extralog")) 
		return;

        if (log_handle < 0)
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
	char date[32], date_c[32], *ip = "", *port = "";
	int i = 0;

        if ( strnull( ip = cvar_string( "sv_local_addr" ) ) || strnull( port = strchr(ip, ':') ) || !(i = atoi(port + 1)) )
                return;

	port[0] = 0;
	port++;

	if ( !QVMstrftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", 0) )
		date[0] = 0; // bad date
	if ( !QVMstrftime(date_c, sizeof(date_c), "%Y%m%d-%H%M%S", 0) )
		date_c[0] = 0; // bad date

	log_open("match-%s-%s.log", GetMode(), date_c);
	log_printf("%s", "<?xml version=\"1.0\"?>\n");
	log_printf("%s", "<ezqstats version=\"1\">\n");
	log_printf("\t<match date=\"%s\" map=\"%s\" hostname=\"%s\" ip=\"%s\" port=\"%d\" mode=\"%s\">\n", 
		date, g_globalvars.mapname, striphigh(cvar_string("hostname")), ip, i, GetMode());
	log_printf("\t\t<events>\n");	
}

void StopLogs()
{
	log_printf("\t\t</events>\n" );
	log_printf("\t</match>\n" );
	log_printf("</ezqstats>\n" );
	log_close();
}

