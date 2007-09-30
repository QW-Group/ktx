/*
 *  QWProgs-QVM
 *  Copyright (C) 2004  [sd] angel
 *
 *  This code is based on Q3 VM code by Id Software, Inc.
 *
 *
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

//#include "g_local.h"

// this file is only included when building a dll
// g_syscalls.asm is included instead when building a qvm
#ifdef Q3_VM
#error "Do not use in VM build"
#endif


#include "g_local.h"

static int      ( QDECL * syscall ) ( int arg, ... ) =
    ( int ( QDECL * ) ( int, ... ) ) -1;



void dllEntry( int ( QDECL * syscallptr ) ( int arg, ... ) )
{
	syscall = syscallptr;
}

int trap_GetApiVersion()
{
	return syscall( G_GETAPIVERSION );
}

qboolean trap_GetEntityToken( char *token, int size )
{
	return ( qboolean ) syscall( G_GetEntityToken, (int)token, size );
}
void trap_DPrintf( const char *fmt )
{
	syscall( G_DPRINT, (int)fmt );
}

void trap_conprint( const char *fmt )
{
	syscall( G_conprint, (int)fmt );
}


void trap_BPrint( int level, const char *fmt )
{
	syscall( G_BPRINT, level, (int) fmt );
}

void trap_SPrint( int edn, int level, const char *fmt, int flags )
{
	syscall( G_SPRINT, edn, level, (int)fmt, flags );
}
void trap_CenterPrint( int edn, const char *fmt )
{
	syscall( G_CENTERPRINT, edn , (int)fmt );
}

void trap_Error( const char *fmt )
{
	syscall( G_ERROR, (int) fmt );
}

int	trap_spawn()
{
	return syscall( G_SPAWN_ENT ) ;
}

void trap_remove( int edn )
{
	syscall( G_REMOVE_ENT, edn  );
}

void trap_precache_sound( char *name )
{
	syscall( G_PRECACHE_SOUND, (int) name );
}
void trap_precache_model( char *name )
{
	syscall( G_PRECACHE_MODEL, (int) name );
}
int trap_precache_vwep_model( char *name )
{
	return syscall( G_PRECACHE_VWEP_MODEL, (int) name );
}

void trap_setorigin( int edn, float origin_x, float origin_y, float origin_z )
{
	syscall( G_SETORIGIN,  edn, PASSFLOAT(origin_x),
		 PASSFLOAT(origin_y), PASSFLOAT(origin_z) );
}

void trap_setsize( int edn, float min_x, float min_y, float min_z, float max_x,
		   float max_y, float max_z )
{
	syscall( G_SETSIZE, edn, PASSFLOAT( min_x), PASSFLOAT( min_y),
		 PASSFLOAT( min_z), PASSFLOAT( max_x), PASSFLOAT( max_y), PASSFLOAT( max_z ));
}

void trap_setmodel( int edn, char *model )
{
	syscall( G_SETMODEL, edn, (int)model );
}

void trap_ambientsound( float pos_x, float pos_y, float pos_z, char *samp, float vol,
			float atten )
{
	syscall( G_AMBIENTSOUND, PASSFLOAT( pos_x), PASSFLOAT( pos_y), PASSFLOAT( pos_z),
		 (int)samp, PASSFLOAT( vol), PASSFLOAT( atten ));
}

void trap_sound( int edn, int channel, char *samp, float vol, float att )
{
	syscall( G_SOUND, edn, channel, (int)samp, PASSFLOAT( vol ), PASSFLOAT( att ));
}

int trap_checkclient()
{
	return  syscall( G_CHECKCLIENT ) ;
}
void trap_traceline( float v1_x, float v1_y, float v1_z, float v2_x, float v2_y,
		     float v2_z, int nomonst, int edn )
{
	syscall( G_TRACELINE, PASSFLOAT( v1_x), PASSFLOAT( v1_y), PASSFLOAT( v1_z),
		 PASSFLOAT( v2_x), PASSFLOAT( v2_y), PASSFLOAT( v2_z), nomonst,
		 edn );
}

void trap_stuffcmd( int edn, const char *fmt )
{
	syscall( G_STUFFCMD, edn, (int)fmt );
}

void trap_localcmd( const char *fmt )
{
	syscall( G_LOCALCMD,(int) fmt );
}

void trap_executecmd()
{
	syscall( G_executecmd );
}

void trap_readcmd( const char *str, char* buf, int size )
{
	syscall( G_readcmd,(int) str, (int)buf, size );
}

void trap_redirectcmd( gedict_t* ent, char* str )
{
	syscall( G_redirectcmd,(int) ent, (int)str);
}


float trap_cvar( const char *var )
{
	fi_t tmp;
	
	tmp._int = syscall( G_CVAR, (int)var );

	return tmp._float;
}
void trap_cvar_string( const char *var, char *buffer, int buffsize )
{
	syscall( G_CVAR_STRING, (int)var, (int)buffer, buffsize );
}

void trap_cvar_set( const char *var, const char *val )
{
	syscall( G_CVAR_SET, (int)var, (int)val );
}

void    trap_cvar_set_float( const char *var, float val )
{
	syscall( G_CVAR_SET_FLOAT, (int)var, PASSFLOAT(val) );
}

int trap_droptofloor( int edn )
{
	return syscall( G_DROPTOFLOOR, edn );
}

int trap_walkmove( int edn, float yaw, float dist )
{
	return syscall( G_WALKMOVE, edn, PASSFLOAT( yaw),
			PASSFLOAT( dist ));
}

void trap_lightstyle( int style, char *val )
{
	syscall( G_LIGHTSTYLE, style, (int)val );
}

int trap_checkbottom( int edn )
{
	return syscall( G_CHECKBOTTOM, edn );
}

int trap_pointcontents( float origin_x, float origin_y, float origin_z )
{
	return syscall( G_POINTCONTENTS, PASSFLOAT( origin_x), PASSFLOAT( origin_y),
			PASSFLOAT( origin_z ));
}

int trap_nextent( int n )
{
	return syscall( G_NEXTENT, n );
}

gedict_t* trap_nextclient( gedict_t* ent )
{
	return (gedict_t*)syscall( G_NEXTCLIENT, (int)ent );
}

/*int trap_find( int n,int fofs, char*str )
{
	return syscall( G_Find, n, fofs, (int)str );
}*/
gedict_t* trap_find( gedict_t* ent,int fofs, char*str )
{
	return (gedict_t*)syscall( G_Find, (int)ent, fofs, (int)str );
}

gedict_t* trap_findradius( gedict_t* ent, float *org, float rad )
{
	return (gedict_t*)syscall( G_FINDRADIUS, (int)ent, (int)org, PASSFLOAT(rad) );
}

void trap_makestatic( int edn )
{
	syscall( G_MAKESTATIC, edn );
}

void trap_setspawnparam( int edn )
{
	syscall( G_SETSPAWNPARAMS, edn );
}

void trap_changelevel( const char *name )
{
	syscall( G_CHANGELEVEL, name );
}

int trap_multicast( float origin_x, float origin_y, float origin_z, int to )
{
	return syscall( G_MULTICAST, PASSFLOAT( origin_x), PASSFLOAT( origin_y),
			PASSFLOAT( origin_z), to );
}


void trap_logfrag( int killer, int killee )
{
	syscall( G_LOGFRAG,  killer , killee  );
}

void trap_infokey( int edn, char *key, char *valbuff, int sizebuff )
{
	syscall( G_GETINFOKEY, edn, (int)key, (int)valbuff, sizebuff );
}

void trap_WriteByte( int to, int data )
{
	syscall( G_WRITEBYTE, to, data );
}

void trap_WriteChar( int to, int data )
{
	syscall( G_WRITECHAR, to, data );
}

void trap_WriteShort( int to, int data )
{
	syscall( G_WRITESHORT, to, data );
}

void trap_WriteLong( int to, int data )
{
	syscall( G_WRITELONG, to, data );
}

void trap_WriteAngle( int to, float data )
{
	syscall( G_WRITEANGLE, to, PASSFLOAT( data ));
}

void trap_WriteCoord( int to, float data )
{
	syscall( G_WRITECOORD, to, PASSFLOAT( data ));
}

void trap_WriteString( int to, char *data )
{
	syscall( G_WRITESTRING, to, (int)data );
}

void trap_WriteEntity( int to, int edn )
{
	syscall( G_WRITEENTITY, to, edn );
}

void trap_FlushSignon()
{
	syscall( G_FLUSHSIGNON );
}


void trap_disableupdates( int edn, float time )
{
	syscall( G_DISABLEUPDATES, edn , PASSFLOAT( time ));
}

int trap_CmdArgc()
{
	return syscall( G_CMD_ARGC );
}
void trap_CmdArgv( int arg, char *valbuff, int sizebuff )
{
	syscall( G_CMD_ARGV, arg, (int)valbuff, sizebuff );
}

void trap_CmdArgs( char *valbuff, int sizebuff )
{
	syscall( G_CMD_ARGS, (int)valbuff, sizebuff );
}

void trap_CmdTokenize( char *str )
{
	syscall( G_CMD_TOKENIZE, (int)str );
}

void    trap_TraceCapsule( float v1_x, float v1_y, float v1_z, 
			float v2_x, float v2_y, float v2_z, 
			int nomonst, int edn ,
			float min_x, float min_y, float min_z, 
			float max_x, float max_y, float max_z)
{
 	syscall( G_TraceCapsule, 
 		 PASSFLOAT( v1_x), PASSFLOAT( v1_y), PASSFLOAT( v1_z),
		 PASSFLOAT( v2_x), PASSFLOAT( v2_y), PASSFLOAT( v2_z), 
		 nomonst, edn,
 		 PASSFLOAT( min_x), PASSFLOAT( min_y), PASSFLOAT( min_z),
		 PASSFLOAT( max_x), PASSFLOAT( max_y), PASSFLOAT( max_z));
}

int trap_FS_OpenFile(char*name, fileHandle_t* handle, fsMode_t fmode )
{
	return syscall( G_FSOpenFile, (int)name, (int)handle, fmode );
}

void trap_FS_CloseFile( fileHandle_t handle )
{
	syscall( G_FSCloseFile, handle );
}

int trap_FS_ReadFile( char*dest, int quantity, fileHandle_t handle )
{
	return syscall( G_FSReadFile, (int)dest, quantity, handle );
}

int trap_FS_WriteFile( char*src, int quantity, fileHandle_t handle )
{
	return syscall( G_FSWriteFile, (int)src, quantity, handle );
}

int trap_FS_SeekFile( fileHandle_t handle, int offset, int type )
{
	return syscall( G_FSSeekFile, handle,offset,type );
}

int 	trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize )
{
	return syscall( G_FSGetFileList, (int)path, (int)extension, (int)listbuf, bufsize);
}

int trap_Map_Extension( const char* ext_name, int mapto)
{
	return syscall( G_Map_Extension, (int)ext_name, mapto );
}

int 	trap_AddBot( const char* name, int bottomcolor, int topcolor, const char* skin)
{
        return syscall( G_Add_Bot, (int)name, bottomcolor, topcolor, (int)skin );
}

int 	trap_RemoveBot( int edn )
{
        return syscall( G_Remove_Bot, edn );
}

int 	trap_SetBotUserInfo( int edn, const char* varname, const char* value )
{
        return syscall( G_SetBotUserInfo, edn, (int)varname, (int)value );
}

int 	trap_SetBotCMD( int edn,int msec, float angles_x, float angles_y, float angles_z, 
                                int forwardmove, int sidemove, int upmove, 
                                int buttons, int impulse)
{
        return syscall( G_SetBotCMD, edn, msec, PASSFLOAT( angles_x), PASSFLOAT( angles_y), PASSFLOAT( angles_z), 
                                forwardmove, sidemove, upmove, buttons, impulse );
}

void 	trap_setpause( int pause )
{
	syscall( G_SETPAUSE, pause );
}


int QVMstrftime( char *valbuff, int sizebuff, const char *fmt, int offset )
{
	return syscall( G_QVMstrftime, (int)valbuff, sizebuff, (int)fmt, offset );
}

void trap_makevectors( float *v )
{
	syscall( G_MAKEVECTORS, (int)v );
}

#if defined( __linux__ ) || defined( _WIN32 ) /* || defined( __APPLE__ ) require?*/
size_t strlcpy(char *dst, char *src, size_t siz)
{
	return syscall( g_strlcpy, (int)dst, (int)src, (int)siz );
}

size_t strlcat(char *dst, char *src, size_t siz)
{
	return syscall( g_strlcat, (int)dst, (int)src, (int)siz );
}
#endif

