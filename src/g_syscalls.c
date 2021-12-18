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

static intptr_t ( QDECL *syscall)(intptr_t arg, ...) =
( intptr_t ( QDECL * ) ( intptr_t, ... ) ) -1;

void VISIBILITY_VISIBLE dllEntry(intptr_t ( QDECL *syscallptr)(intptr_t arg, ...))
{
	syscall = syscallptr;
}

typedef union fi_s
{
	float _float;
	intptr_t _int;
} fi_t;

static intptr_t PASSFLOAT(float x)
{
	fi_t rc;

	rc._float = x;

	return rc._int;
}

intptr_t trap_GetApiVersion()
{
	return syscall(G_GETAPIVERSION);
}

qbool trap_GetEntityToken(char *token, intptr_t size)
{
	return (qbool)syscall(G_GetEntityToken, (intptr_t) token, size);
}

void trap_DPrintf(const char *fmt)
{
	syscall(G_DPRINT, (intptr_t) fmt);
}

void trap_conprint(const char *fmt)
{
	syscall(G_conprint, (intptr_t) fmt);
}

void trap_BPrint(intptr_t level, const char *fmt, intptr_t flags)
{
	syscall(G_BPRINT, level, (intptr_t) fmt, flags);
}

void trap_SPrint(intptr_t edn, intptr_t level, const char *fmt, intptr_t flags)
{
	syscall(G_SPRINT, edn, level, (intptr_t) fmt, flags);
}

void trap_CenterPrint(intptr_t edn, const char *fmt)
{
	syscall(G_CENTERPRINT, edn, (intptr_t) fmt);
}

void trap_Error(const char *fmt)
{
	syscall(G_ERROR, (intptr_t) fmt);
}

intptr_t trap_spawn()
{
	return syscall(G_SPAWN_ENT);
}

void trap_remove(intptr_t edn)
{
	syscall(G_REMOVE_ENT, edn);
}

void trap_precache_sound(char *name)
{
	syscall(G_PRECACHE_SOUND, (intptr_t) name);
}
void trap_precache_model(char *name)
{
	syscall(G_PRECACHE_MODEL, (intptr_t) name);
}
intptr_t trap_precache_vwep_model(char *name)
{
	return syscall(G_PRECACHE_VWEP_MODEL, (intptr_t) name);
}

void trap_setorigin(intptr_t edn, float origin_x, float origin_y, float origin_z)
{
	syscall(G_SETORIGIN, edn, PASSFLOAT(origin_x), PASSFLOAT(origin_y), PASSFLOAT(origin_z));
}

void trap_setsize(intptr_t edn, float min_x, float min_y, float min_z, float max_x, float max_y,
					float max_z)
{
	syscall(G_SETSIZE, edn, PASSFLOAT(min_x), PASSFLOAT(min_y), PASSFLOAT(min_z), PASSFLOAT(max_x),
			PASSFLOAT(max_y), PASSFLOAT(max_z));
}

void trap_setmodel(intptr_t edn, char *model)
{
	syscall(G_SETMODEL, edn, (intptr_t) model);
}

void trap_ambientsound(float pos_x, float pos_y, float pos_z, char *samp, float vol, float atten)
{
	syscall(G_AMBIENTSOUND, PASSFLOAT(pos_x), PASSFLOAT(pos_y), PASSFLOAT(pos_z), (intptr_t) samp,
			PASSFLOAT(vol), PASSFLOAT(atten));
}

void trap_sound(intptr_t edn, intptr_t channel, char *samp, float vol, float att)
{
	syscall(G_SOUND, edn, channel, (intptr_t) samp, PASSFLOAT(vol), PASSFLOAT(att));
}

intptr_t trap_checkclient()
{
	return syscall(G_CHECKCLIENT);
}

void trap_traceline(float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z,
					intptr_t nomonst, intptr_t edn)
{
	syscall(G_TRACELINE, PASSFLOAT(v1_x), PASSFLOAT(v1_y), PASSFLOAT(v1_z), PASSFLOAT(v2_x),
			PASSFLOAT(v2_y), PASSFLOAT(v2_z), nomonst, edn);
}

void trap_stuffcmd(intptr_t edn, const char *fmt, intptr_t flags)
{
	syscall(G_STUFFCMD, edn, (intptr_t) fmt, flags);
}

void trap_localcmd(const char *fmt)
{
	syscall(G_LOCALCMD, (intptr_t) fmt);
}

void trap_executecmd()
{
	syscall(G_executecmd);
}

void trap_readcmd(const char *str, char *buf, intptr_t size)
{
	syscall(G_readcmd, (intptr_t) str, (intptr_t) buf, size);
}

void trap_redirectcmd(gedict_t *ent, char *str)
{
	syscall(G_redirectcmd, (intptr_t) ent, (intptr_t) str);
}

float trap_cvar(const char *var)
{
	fi_t tmp;

	tmp._int = syscall(G_CVAR, (intptr_t) var);

	return tmp._float;
}

void trap_cvar_string(const char *var, char *buffer, intptr_t buffsize)
{
	syscall(G_CVAR_STRING, (intptr_t) var, (intptr_t) buffer, buffsize);
}

void trap_cvar_set(const char *var, const char *val)
{
	syscall(G_CVAR_SET, (intptr_t) var, (intptr_t) val);
}

void trap_cvar_set_float(const char *var, float val)
{
	syscall(G_CVAR_SET_FLOAT, (intptr_t) var, PASSFLOAT(val));
}

intptr_t trap_droptofloor(intptr_t edn)
{
	return syscall(G_DROPTOFLOOR, edn);
}

intptr_t trap_walkmove(intptr_t edn, float yaw, float dist)
{
	return syscall(G_WALKMOVE, edn, PASSFLOAT(yaw), PASSFLOAT(dist));
}

intptr_t trap_movetogoal(float dist)
{
	return syscall(G_MOVETOGOAL, PASSFLOAT(dist));
}

void trap_lightstyle(intptr_t style, char *val)
{
	syscall(G_LIGHTSTYLE, style, (intptr_t) val);
}

intptr_t trap_checkbottom(intptr_t edn)
{
	return syscall(G_CHECKBOTTOM, edn);
}

intptr_t trap_pointcontents(float origin_x, float origin_y, float origin_z)
{
	return syscall(G_POINTCONTENTS, PASSFLOAT(origin_x), PASSFLOAT(origin_y), PASSFLOAT(origin_z));
}

intptr_t trap_nextent(intptr_t n)
{
	return syscall(G_NEXTENT, n);
}

gedict_t* trap_nextclient(gedict_t *ent)
{
	return (gedict_t*) syscall(G_NEXTCLIENT, (intptr_t) ent);
}

/*intptr_t trap_find( intptr_t n,intptr_t fofs, char*str )
 {
 return syscall( G_Find, n, fofs, (intptr_t)str );
 }*/
gedict_t* trap_find(gedict_t *ent, intptr_t fofs, char *str)
{
	return (gedict_t*) syscall(G_Find, (intptr_t) ent, fofs, (intptr_t) str);
}

gedict_t* trap_findradius(gedict_t *ent, float *org, float rad)
{
	return (gedict_t*) syscall(G_FINDRADIUS, (intptr_t) ent, (intptr_t) org, PASSFLOAT(rad));
}

void trap_makestatic(intptr_t edn)
{
	syscall(G_MAKESTATIC, edn);
}

void trap_setspawnparam(intptr_t edn)
{
	syscall(G_SETSPAWNPARAMS, edn);
}

void trap_changelevel(const char *name, const char *entityname)
{
	syscall(G_CHANGELEVEL, name, entityname);
}

intptr_t trap_multicast(float origin_x, float origin_y, float origin_z, intptr_t to)
{
	return syscall(G_MULTICAST, PASSFLOAT(origin_x), PASSFLOAT(origin_y), PASSFLOAT(origin_z), to);
}

void trap_logfrag(intptr_t killer, intptr_t killee)
{
	syscall(G_LOGFRAG, killer, killee);
}

void trap_infokey(intptr_t edn, char *key, char *valbuff, intptr_t sizebuff)
{
	syscall(G_GETINFOKEY, edn, (intptr_t) key, (intptr_t) valbuff, sizebuff);
}

void trap_WriteByte(intptr_t to, intptr_t data)
{
	syscall(G_WRITEBYTE, to, data);
}

void trap_WriteChar(intptr_t to, intptr_t data)
{
	syscall(G_WRITECHAR, to, data);
}

void trap_WriteShort(intptr_t to, intptr_t data)
{
	syscall(G_WRITESHORT, to, data);
}

void trap_WriteLong(intptr_t to, intptr_t data)
{
	syscall(G_WRITELONG, to, data);
}

void trap_WriteAngle(intptr_t to, float data)
{
	syscall(G_WRITEANGLE, to, PASSFLOAT(data));
}

void trap_WriteCoord(intptr_t to, float data)
{
	syscall(G_WRITECOORD, to, PASSFLOAT(data));
}

void trap_WriteString(intptr_t to, char *data)
{
	syscall(G_WRITESTRING, to, (intptr_t) data);
}

void trap_WriteEntity(intptr_t to, intptr_t edn)
{
	syscall(G_WRITEENTITY, to, edn);
}

void trap_FlushSignon()
{
	syscall(G_FLUSHSIGNON);
}

void trap_disableupdates(intptr_t edn, float time)
{
	syscall(G_DISABLEUPDATES, edn, PASSFLOAT(time));
}

intptr_t trap_CmdArgc()
{
	return syscall(G_CMD_ARGC);
}
void trap_CmdArgv(intptr_t arg, char *valbuff, intptr_t sizebuff)
{
	syscall(G_CMD_ARGV, arg, (intptr_t) valbuff, sizebuff);
}

void trap_CmdArgs(char *valbuff, intptr_t sizebuff)
{
	syscall(G_CMD_ARGS, (intptr_t) valbuff, sizebuff);
}

void trap_CmdTokenize(char *str)
{
	syscall(G_CMD_TOKENIZE, (intptr_t) str);
}

void trap_TraceCapsule(float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z,
						intptr_t nomonst, intptr_t edn, float min_x, float min_y, float min_z,
						float max_x, float max_y, float max_z)
{
	syscall(G_TraceCapsule, PASSFLOAT(v1_x), PASSFLOAT(v1_y), PASSFLOAT(v1_z), PASSFLOAT(v2_x),
			PASSFLOAT(v2_y), PASSFLOAT(v2_z), nomonst, edn, PASSFLOAT(min_x), PASSFLOAT(min_y),
			PASSFLOAT(min_z), PASSFLOAT(max_x), PASSFLOAT(max_y), PASSFLOAT(max_z));
}

intptr_t trap_FS_OpenFile(char *name, fileHandle_t *handle, fsMode_t fmode)
{
	return syscall(G_FSOpenFile, (intptr_t) name, (intptr_t) handle, fmode);
}

void trap_FS_CloseFile(fileHandle_t handle)
{
	syscall(G_FSCloseFile, handle);
}

intptr_t trap_FS_ReadFile(char *dest, intptr_t quantity, fileHandle_t handle)
{
	return syscall(G_FSReadFile, (intptr_t) dest, quantity, handle);
}

intptr_t trap_FS_WriteFile(char *src, intptr_t quantity, fileHandle_t handle)
{
	return syscall(G_FSWriteFile, (intptr_t) src, quantity, handle);
}

intptr_t trap_FS_SeekFile(fileHandle_t handle, intptr_t offset, intptr_t type)
{
	return syscall(G_FSSeekFile, handle, offset, type);
}

intptr_t trap_FS_GetFileList(const char *path, const char *extension, char *listbuf,
								intptr_t bufsize, intptr_t flags)
{
	return syscall(G_FSGetFileList, (intptr_t) path, (intptr_t) extension, (intptr_t) listbuf,
					bufsize, flags);
}

intptr_t trap_Map_Extension(const char *ext_name, intptr_t mapto)
{
	return syscall(G_Map_Extension, (intptr_t) ext_name, mapto);
}

intptr_t trap_AddBot(const char *name, intptr_t bottomcolor, intptr_t topcolor, const char *skin)
{
	return syscall(G_Add_Bot, (intptr_t) name, bottomcolor, topcolor, (intptr_t) skin);
}

intptr_t trap_RemoveBot(intptr_t edn)
{
	return syscall(G_Remove_Bot, edn);
}

intptr_t trap_SetBotUserInfo(intptr_t edn, const char *varname, const char *value, intptr_t flags)
{
	return syscall(G_SetBotUserInfo, edn, (intptr_t) varname, (intptr_t) value, flags);
}

intptr_t trap_SetBotCMD(intptr_t edn, intptr_t msec, float angles_x, float angles_y, float angles_z,
						intptr_t forwardmove, intptr_t sidemove, intptr_t upmove, intptr_t buttons,
						intptr_t impulse)
{
	return syscall(G_SetBotCMD, edn, msec, PASSFLOAT(angles_x), PASSFLOAT(angles_y),
					PASSFLOAT(angles_z), forwardmove, sidemove, upmove, buttons, impulse);
}

void trap_setpause(intptr_t pause)
{
	syscall(G_SETPAUSE, pause);
}

intptr_t QVMstrftime(char *valbuff, intptr_t sizebuff, const char *fmt, intptr_t offset)
{
	return syscall(G_QVMstrftime, (intptr_t) valbuff, sizebuff, (intptr_t) fmt, offset);
}

void trap_makevectors(float *v)
{
	syscall(G_MAKEVECTORS, (intptr_t) v);
}

#if defined( __linux__ ) || defined( _WIN32 ) /* || defined( __APPLE__ ) require?*/
size_t strlcpy(char *dst, char *src, size_t siz)
{
	return syscall(g_strlcpy, (intptr_t) dst, (intptr_t) src, (intptr_t) siz);
}

size_t strlcat(char *dst, char *src, size_t siz)
{
	return syscall(g_strlcat, (intptr_t) dst, (intptr_t) src, (intptr_t) siz);
}
#endif

intptr_t trap_SetUserInfo(intptr_t edn, const char *varname, const char *value, intptr_t flags)
{
	return syscall(G_SETUSERINFO, edn, (intptr_t) varname, (intptr_t) value, flags);
}
