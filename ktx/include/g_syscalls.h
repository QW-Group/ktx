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
 *  $Id: g_syscalls.h,v 1.8 2007/06/15 16:03:55 qqshka Exp $
 */

int     trap_GetApiVersion(  );
qboolean trap_GetEntityToken( char *token, int size );
void    trap_DPrintf( const char *fmt );
void 	trap_conprint( const char *fmt );

void    trap_BPrint( int level, const char *fmt );
void    trap_SPrint( int edn, int level, const char *fmt );
void    trap_CenterPrint( int edn, const char *fmt );
void    trap_Error( const char *fmt );
int     trap_spawn(  );
void    trap_remove( int edn );
void    trap_precache_sound( char *name );
void    trap_precache_model( char *name );
#ifdef VWEP_TEST
void    trap_precache_vwep_model( int pos, char *name );
#endif
void    trap_setorigin( int edn, float origin_x, float origin_y, float origin_z );
void    trap_setsize( int edn, float min_x, float min_y, float min_z, float max_x,
		      float max_y, float max_z );
void    trap_setmodel( int edn, char *model );
void    trap_ambientsound( float pos_x, float pos_y, float pos_z, char *samp, float vol,
			   float atten );
void    trap_sound( int edn, int channel, char *samp, float vol, float att );
int     trap_checkclient(  );
void    trap_traceline( float v1_x, float v1_y, float v1_z, float v2_x, float v2_y,
			float v2_z, int nomonst, int edn );
void    trap_stuffcmd( int edn, const char *fmt );
void    trap_localcmd( const char *fmt );
void 	trap_executecmd();
void 	trap_readcmd( const char *str, char* buf, int size );
void 	trap_redirectcmd( gedict_t* ent, char* str );

float   trap_cvar( const char *var );
void 	trap_cvar_string( const char *var, char *buffer, int bufsize );
void    trap_cvar_set( const char *var, const char *val );
void    trap_cvar_set_float( const char *var, float val );
int     trap_droptofloor( int edn );
int     trap_walkmove( int edn, float yaw, float dist );
void    trap_lightstyle( int style, char *val );
int     trap_checkbottom( int edn );
int     trap_pointcontents( float origin_x, float origin_y, float origin_z );
int     trap_nextent( int n );
gedict_t*	trap_nextclient( gedict_t* ent );
//int 	trap_find( int n,int fofs, char*str );
gedict_t*	trap_find( gedict_t* ent,int fofs, char*str );
gedict_t*	trap_findradius( gedict_t* ent, float *org, float rad );

void    trap_makestatic( int edn );
void    trap_setspawnparam( int edn );
void    trap_changelevel( const char *name );
int     trap_multicast( float origin_x, float origin_y, float origin_z, int to );
void    trap_logfrag( int killer, int killee );
void    trap_infokey( int edn, char *key, char *valbuff, int sizebuff );
void    trap_WriteByte( int to, int data );
void    trap_WriteChar( int to, int data );
void    trap_WriteShort( int to, int data );
void    trap_WriteLong( int to, int data );
void    trap_WriteAngle( int to, float data );
void    trap_WriteCoord( int to, float data );
void    trap_WriteString( int to, char *data );
void    trap_WriteEntity( int to, int edn );
void    trap_FlushSignon(  );
void    trap_disableupdates( int edn, float time );
int     trap_CmdArgc(  );
void    trap_CmdArgv( int arg, char *valbuff, int sizebuff );
void	trap_CmdArgs( char *valbuff, int sizebuff );
void	trap_CmdTokenize( char *str );
void    trap_TraceCapsule( float v1_x, float v1_y, float v1_z, 
			float v2_x, float v2_y, float v2_z, 
			int nomonst, int edn ,
			float min_x, float min_y, float min_z, 
			float max_x, float max_y, float max_z);

int	trap_FS_OpenFile(char*name, fileHandle_t* handle, fsMode_t fmode );
void	trap_FS_CloseFile( fileHandle_t handle );
int	trap_FS_ReadFile( char*dest, int quantity, fileHandle_t handle );
int	trap_FS_WriteFile( char*src, int quantity, fileHandle_t handle );
int	trap_FS_SeekFile( fileHandle_t handle, int offset, int type );
int	trap_FS_TellFile( fileHandle_t handle );
int 	trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );

int 	trap_Map_Extension( const char* ext_name, int mapto);
/*  return:
    0 	success maping
   -1	not found
   -2	cannot map
*/

int 	trap_AddBot( const char* name, int bottomcolor, int topcolor, const char* skin);
int 	trap_RemoveBot( int edn );
int 	trap_SetBotUserInfo( int edn, const char* varname, const char* value );
int 	trap_SetBotCMD( int edn,int msec, float angles_x, float angles_y, float angles_z, 
                                int forwardmove, int sidemove, int upmove, 
                                int buttons, int impulse);

int QVMstrftime( char *valbuff, int sizebuff, const char *fmt, int offset );

void trap_makevectors( float *v );

#if defined( __linux__ ) || defined( _WIN32 ) /* || defined( __APPLE__ ) require?*/
size_t strlcpy(char *dst, char *src, size_t siz);
size_t strlcat(char *dst, char *src, size_t siz);
#endif

