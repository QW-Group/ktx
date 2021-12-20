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

intptr_t trap_GetApiVersion();
qbool trap_GetEntityToken(char *token, intptr_t size);
void trap_DPrintf(const char *fmt);
void trap_conprint(const char *fmt);

#define BPRINT_IGNOREINDEMO		(1<<0) // broad cast print will be not put in demo
#define BPRINT_IGNORECLIENTS	(1<<1) // broad cast print will not be seen by clients, but may be seen in demo
#define BPRINT_QTVONLY			(1<<2) // if broad cast print goes to demo, then it will be only qtv sream, but not file
#define BPRINT_IGNORECONSOLE	(1<<3) // broad cast print will not be put in server console
void trap_BPrint(intptr_t level, const char *fmt, intptr_t flags);

// trap_SPrint() flags
#define SPRINT_IGNOREINDEMO		(   1<<0) // do not put such message in mvd demo
void trap_SPrint(intptr_t edn, intptr_t level, const char *fmt, intptr_t flags);
void trap_CenterPrint(intptr_t edn, const char *fmt);
void trap_Error(const char *fmt);
intptr_t trap_spawn();
void trap_remove(intptr_t edn);
void trap_precache_sound(char *name);
void trap_precache_model(char *name);
intptr_t trap_precache_vwep_model(char *name);
void trap_setorigin(intptr_t edn, float origin_x, float origin_y, float origin_z);
void trap_setsize(intptr_t edn, float min_x, float min_y, float min_z, float max_x, float max_y,
					float max_z);
void trap_setmodel(intptr_t edn, char *model);
void trap_ambientsound(float pos_x, float pos_y, float pos_z, char *samp, float vol, float atten);
void trap_sound(intptr_t edn, intptr_t channel, char *samp, float vol, float att);
intptr_t trap_checkclient();
void trap_traceline(float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z,
					intptr_t nomonst, intptr_t edn);

#define STUFFCMD_IGNOREINDEMO (   1<<0) // do not put in mvd demo
#define STUFFCMD_DEMOONLY     (   1<<1) // put in mvd demo only
void trap_stuffcmd(intptr_t edn, const char *fmt, intptr_t flags);
void trap_localcmd(const char *fmt);
void trap_executecmd();
void trap_readcmd(const char *str, char *buf, intptr_t size);
void trap_redirectcmd(gedict_t *ent, char *str);

float trap_cvar(const char *var);
void trap_cvar_string(const char *var, char *buffer, intptr_t bufsize);
void trap_cvar_set(const char *var, const char *val);
void trap_cvar_set_float(const char *var, float val);
intptr_t trap_droptofloor(intptr_t edn);
intptr_t trap_walkmove(intptr_t edn, float yaw, float dist);
void trap_lightstyle(intptr_t style, char *val);
intptr_t trap_checkbottom(intptr_t edn);
intptr_t trap_pointcontents(float origin_x, float origin_y, float origin_z);
intptr_t trap_nextent(intptr_t n);
gedict_t* trap_nextclient(gedict_t *ent);
//intptr_t 	trap_find( intptr_t n,intptr_t fofs, char*str );
gedict_t* trap_find(gedict_t *ent, intptr_t fofs, char *str);
gedict_t* trap_findradius(gedict_t *ent, float *org, float rad);

void trap_makestatic(intptr_t edn);
void trap_setspawnparam(intptr_t edn);
void trap_changelevel(const char *name, const char *entityname);
intptr_t trap_multicast(float origin_x, float origin_y, float origin_z, intptr_t to);
void trap_logfrag(intptr_t killer, intptr_t killee);
void trap_infokey(intptr_t edn, char *key, char *valbuff, intptr_t sizebuff);
void trap_WriteByte(intptr_t to, intptr_t data);
void trap_WriteChar(intptr_t to, intptr_t data);
void trap_WriteShort(intptr_t to, intptr_t data);
void trap_WriteLong(intptr_t to, intptr_t data);
void trap_WriteAngle(intptr_t to, float data);
void trap_WriteCoord(intptr_t to, float data);
void trap_WriteString(intptr_t to, char *data);
void trap_WriteEntity(intptr_t to, intptr_t edn);
void trap_FlushSignon();
void trap_disableupdates(intptr_t edn, float time);
intptr_t trap_CmdArgc();
void trap_CmdArgv(intptr_t arg, char *valbuff, intptr_t sizebuff);
void trap_CmdArgs(char *valbuff, intptr_t sizebuff);
void trap_CmdTokenize(char *str);
void trap_TraceCapsule(float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z,
						intptr_t nomonst, intptr_t edn, float min_x, float min_y, float min_z,
						float max_x, float max_y, float max_z);

intptr_t trap_FS_OpenFile(char *name, fileHandle_t *handle, fsMode_t fmode);
void trap_FS_CloseFile(fileHandle_t handle);
intptr_t trap_FS_ReadFile(char *dest, intptr_t quantity, fileHandle_t handle);
intptr_t trap_FS_WriteFile(char *src, intptr_t quantity, fileHandle_t handle);
intptr_t trap_FS_SeekFile(fileHandle_t handle, intptr_t offset, intptr_t type);
intptr_t trap_FS_TellFile(fileHandle_t handle);

#define FILELIST_GAMEDIR_ONLY	(1<<0) // if set then search in gamedir only
#define FILELIST_WITH_PATH		(1<<1) // include path to file
#define FILELIST_WITH_EXT		(1<<2) // include extension of file

intptr_t trap_FS_GetFileList(const char *path, const char *extension, char *listbuf,
								intptr_t bufsize, intptr_t flags);

intptr_t trap_Map_Extension(const char *ext_name, intptr_t mapto);
/*  return:
 0 	success maping
 -1	not found
 -2	cannot map
 */

intptr_t trap_AddBot(const char *name, intptr_t bottomcolor, intptr_t topcolor, const char *skin);
intptr_t trap_RemoveBot(intptr_t edn);
intptr_t trap_SetBotUserInfo(intptr_t edn, const char *varname, const char *value, intptr_t flags);
intptr_t trap_SetBotCMD(intptr_t edn, intptr_t msec, float angles_x, float angles_y, float angles_z,
						intptr_t forwardmove, intptr_t sidemove, intptr_t upmove, intptr_t buttons,
						intptr_t impulse);

void trap_setpause(intptr_t pause);

intptr_t QVMstrftime(char *valbuff, intptr_t sizebuff, const char *fmt, intptr_t offset);

void trap_makevectors(float *v);

#define SETUSERINFO_STAR          (1<<0) // allow set star keys

intptr_t trap_SetUserInfo(intptr_t edn, const char *varname, const char *value, intptr_t flags);

intptr_t trap_movetogoal(float dist);

void trap_VisibleTo(intptr_t viewer, intptr_t first, intptr_t len, byte *visible);
