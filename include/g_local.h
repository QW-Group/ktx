/*
 *  QWProgs-DM
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
 *  $Id: g_local.h 562 2007-09-24 10:11:18Z d3urk $
 */

// g_local.h -- local definitions for game module

//#define DEBUG
#define NO_K_PAUSE    /* k_pause is buggy/unstable, so don't use it */
#define CTF_RELOADMAP /* changing ctf status will force map reload */

#include "q_shared.h"
#include "mathlib.h"
#include "progs.h"
#include "g_public.h"
#include "g_consts.h"
#include "g_syscalls.h"
#include "player.h"

#define MOD_VERSION			("1.34 (dev build)")
#define MOD_NAME			("KTX")
#define MOD_SERVERINFO_MOD_KEY		("xmod")
#define MOD_SERVERINFO_BUILD_KEY	("xbuild")
#define MOD_URL    			("http://ktx.sourceforge.net")
#define MOD_SRC_URL    			("http://ktx.svn.sourceforge.net/viewvc/ktx/")
#define REV_NUMBER			("$Revision$")

// qqshka - hmm, seems in C this is macros
#undef max
#undef min

#ifndef min
//#define min(a,b) ((a) < (b) ? (a) : (b))
float min( float a, float b );
#define KTX_MIN
#endif
#ifndef max
//#define max(a,b) ((a) > (b) ? (a) : (b))
float max( float a, float b );
#define KTX_MAX
#endif

float bound( float a, float b, float c );
//#define bound(a,b,c) ((a) >= (c) ? (a) : (b) < (a) ? (a) : (b) > (c) ? (c) : (b))

#ifdef DEBUG
#define DebugTrap(x) *(char*)0=x
#else
#define DebugTrap(x) G_Error(x)
#endif

#define MOTD_LINES (15)

#define MAX_LASTSCORES (30)

#define MAX_STRINGS 128

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	1024	// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	FOFS(x) ((int)&(((gedict_t *)0)->x))

#define FOFCLSN ( FOFS ( s.v.classname ) )

int             NUM_FOR_EDICT( gedict_t * e );


// health macros

#define ISLIVE(e) ((e)->s.v.health > 0)
#define ISDEAD(e) ((e)->s.v.health <= 0)


// possible disallowed weapons

#define DA_WPNS (IT_AXE|IT_SHOTGUN|IT_SUPER_SHOTGUN|IT_NAILGUN|IT_SUPER_NAILGUN|IT_ROCKET_LAUNCHER|IT_GRENADE_LAUNCHER|IT_LIGHTNING)

extern gameData_t gamedata;
extern gedict_t g_edicts[];
extern globalvars_t g_globalvars;
extern gedict_t *world;
extern gedict_t *self, *other;
extern gedict_t *newmis;
extern int      timelimit, fraglimit, teamplay, deathmatch, framecount;
//extern float	rj;

#define	EDICT_TO_PROG(e) ((byte *)(e) - (byte *)g_edicts)
#define PROG_TO_EDICT(e) ((gedict_t *)((byte *)g_edicts + (e)))

void            G_Printf( const char *fmt, ... );
void            G_Error( const char *fmt, ... );

#define PASSVEC3(x) (x[0]),(x[1]),(x[2])
#define SetVector(v,x,y,z) (v[0]=x,v[1]=y,v[2]=z)

// some types

typedef enum
{
	gtUnknown = 0,
	gtDuel,
	gtTeam,
	gtFFA,
	gtCTF
} gameType_t;

typedef enum
{
	lsUnknown = 0,
	lsDuel,
	lsTeam,
	lsFFA,
	lsCTF,
	lsRA 	// note no correspoding gameType_t for lsType
} lsType_t; // lastscores type

typedef enum
{
	dtNONE = 0,
	dtAXE,
	dtSG,
	dtSSG,
	dtNG,
	dtSNG,
	dtGL,
	dtRL,
	dtLG_BEAM,
	dtLG_DIS, // discharge
	dtLG_DIS_SELF, // sometimes this sort of discharge happens in dmm > 3
	dtHOOK,
	dtCHANGELEVEL,
	dtLAVA_DMG,
	dtSLIME_DMG,
	dtWATER_DMG,
	dtFALL,
	dtSTOMP,
	dtTELE1,
	dtTELE2,
	dtTELE3,
	dtEXPLO_BOX,
	dtLASER,
	dtFIREBALL,
	dtSQUISH,
	dtTRIGGER_HURT,
	dtSUICIDE, // client use /kill command
	dtUNKNOWN
} deathType_t;

typedef union fi_s
{
	float			_float;
	int			_int;
} fi_t;	


#if defined( Q3_VM )

// bg_lib.c

int Q_vsnprintf(char *buffer, size_t count, const char *format, va_list argptr);
int snprintf(char *buffer, size_t count, char const *format, ...);

#else

// native_lib.c

	#if defined( _WIN32 )

		int Q_vsnprintf(char *buffer, size_t count, const char *format, va_list argptr);
		int snprintf(char *buffer, size_t count, char const *format, ...);

	#else

		#define Q_vsnprintf vsnprintf

	#endif // defined( _WIN32 )

#endif // !defined( Q3_VM )

//g_utils.c

int				PASSFLOAT(float x);

float           g_random( void );
float           crandom( void );
int				i_rnd( int from, int to );
gedict_t       *spawn( void );
void            ent_remove( gedict_t * t );

gedict_t       *nextent( gedict_t * ent );
gedict_t       *find( gedict_t * start, int fieldoff, char *str );
int				find_cnt( int fieldoff, char *str );
gedict_t	   *find_idx( int idx, int fieldoff, char *str );
gedict_t       *findradius( gedict_t * start, vec3_t org, float rad );
gedict_t 	   *findradius2( gedict_t * start, vec3_t org, float rad );
void            normalize( vec3_t value, vec3_t newvalue );
float           vlen( vec3_t value1 );
float           vectoyaw( vec3_t value1 );
void            vectoangles( vec3_t value1, vec3_t ret );
void            changeyaw( gedict_t * ent );
void            makevectors( vec3_t vector );

char			*va(char *format, ...);
char			*redtext(const char *format, ...);
char 			*dig3(int d);
char			*dig3s(const char *format, ...);
char			*striphigh(const char *format, ...);

void            G_sprint( gedict_t * ed, int level, const char *fmt, ... );
void            G_sprint_flags( gedict_t * ed, int level, int flags, const char *fmt, ... );
void            G_bprint( int level, const char *fmt, ... );
void            G_centerprint( gedict_t * ed, const char *fmt, ... );
/* centerprint too all clients */
void 			G_cp2all(const char *fmt, ... );

void			G_cprint( const char *fmt, ... );
void            G_dprint( const char *fmt, ... );

void			localcmd( const char *fmt, ... );

int				streq( const char *s1, const char *s2 );
int				strneq( const char *s1, const char *s2 );
int				strnull ( const char *s1 );

/*
#define strneq(s1,s2)	(strcmp( (s1) ? (s1) : "", (s2) ? (s2) : "" ))
#define streq(s1,s2)	(!strneq((s1), (s2)))
#define strnull(s1)		((s1) ? !*(s1) : true)
*/

void		aim( vec3_t ret );
void		setorigin( gedict_t * ed, float origin_x, float origin_y, float origin_z );
void		setsize( gedict_t * ed, float min_x, float min_y, float min_z, float max_x,
		 			float max_y, float max_z );
void    	setmodel( gedict_t * ed, char *model );
void    	sound( gedict_t * ed, int channel, char *samp, float vol, float att );
gedict_t 	*checkclient(  );
void    	traceline( float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z,
		   int nomonst, gedict_t * ed );
void 		TraceCapsule( float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z, int nomonst, gedict_t * ed ,
			float min_x, float min_y, float min_z, 
			float max_x, float max_y, float max_z);

void    	stuffcmd( gedict_t * ed, const char *fmt , ...);
int     	droptofloor( gedict_t * ed );
int     	walkmove( gedict_t * ed, float yaw, float dist );
int     	checkbottom( gedict_t * ed );
void    	makestatic( gedict_t * ed );
void    	setspawnparam( gedict_t * ed );
void    	logfrag( gedict_t * killer, gedict_t * killee );
char    	*infokey( gedict_t * ed, char *key, char *valbuff, int sizebuff );

char		*ezinfokey( gedict_t * ed, char *key );
int			iKey( gedict_t * ed, char *key );
float		fKey( gedict_t * ed, char *key );

void    	WriteEntity( int to, gedict_t * ed );

void		WriteByte( int to, int data );
void		WriteShort( int to, int data );
void		WriteLong( int to, int data );
void		WriteString( int to, char *data );
void		WriteAngle( int to, float data );
void		WriteCoord( int to, float data );

float		cvar( const char *var );
char		*cvar_string( const char *var );
void        cvar_set( const char *var, const char *val );
void		cvar_fset( const char *var, float val );

#define MAX_TEAM_NAME (32)  //  qqshka: 32 - is max len of team name?

//extern char teams[MAX_CLIENTS][MAX_TEAM_NAME];
int 		getteams(char teams[MAX_CLIENTS][MAX_TEAM_NAME]);
char		*getteam( gedict_t * ed );
char		*getname( gedict_t * ed );

char		*g_his( gedict_t * ed );
char		*g_he( gedict_t * ed );
char		*g_himself( gedict_t * ed );


gedict_t	*find_client( gedict_t *start );
gedict_t	*find_plr( gedict_t *start );
gedict_t	*find_spc( gedict_t *start );
gedict_t	*find_plrghst( gedict_t *start, int *from );
gedict_t	*find_plrspc( gedict_t *start, int *from );
gedict_t 	*player_by_id( int id );
gedict_t	*player_by_name( const char *name );
gedict_t	*player_by_IDorName( const char *IDname );
gedict_t	*spec_by_id( int id );
gedict_t	*spec_by_name( const char *name );
gedict_t	*spec_by_IDorName( const char *IDname );

gedict_t	*SpecPlayer_by_IDorName( const char *IDname );
gedict_t	*SpecPlayer_by_id( int id );

gedict_t	*not_connected_by_id( int id );
gedict_t	*not_connected_by_name( const char *name );
gedict_t	*not_connected_by_IDorName( const char *IDname );

char		*armor_type( int items );

qboolean	isghost( gedict_t *ed );

qboolean	isDuel( );
qboolean	isTeam( );
qboolean	isFFA( );
qboolean	isCTF( );
qboolean	isUnknown( );
int			tp_num();
int			GetUserID(gedict_t *p);
char		*TrackWhom(gedict_t *p);
int			GetHandicap( gedict_t *p );
qboolean	SetHandicap( gedict_t *p, int nhdc );
void		changelevel( const char *name );
int			Get_Powerups ();

char 		*count_s( int cnt );
char		*Enables( float f );
char		*Enabled( float f );
char		*Allows( float f );
char		*Allowed( float f );
char		*OnOff( float f );

int			get_scores1();
int			get_scores2();
gedict_t	*get_ed_scores1();
gedict_t	*get_ed_scores2();

int			build_number ();

gedict_t	*get_ed_best1();
gedict_t	*get_ed_best2();
gedict_t	*get_ed_bestPow();

void		show_sv_version();
char		*str_noweapon(int k_disallow_weapons);

void		cvar_toggle_msg( gedict_t *p, char *cvarName, char *msg );

qboolean	can_exec( char *name );

void		ghostClearScores( gedict_t *g );
void		update_ghosts ();

// { events

void		on_enter();
void		on_connect();
void		on_match_start( gedict_t *p );
void		on_match_end( gedict_t *p );
void		on_match_break( gedict_t *p );
void		on_admin( gedict_t *p );
void		on_unadmin( gedict_t *p );

void		info_ev_update( gedict_t *p, char *from, char *to );
void		info_kf_update( gedict_t *p, char *from, char *to );

// }

void		cl_refresh_plus_scores (gedict_t *p);
void		refresh_plus_scores ();

void    	disableupdates( gedict_t * ed, float time );

int			only_digits(const char *s);

char		*params_str( int from, int to );

char		*SD_type_str(); // sudden death type string

char		*respawn_model_name( int mdl_num );

int			get_fair_pack();
int			get_fallbunny();

//
//  subs.c
//
void            SUB_CalcMove( vec3_t tdest, float tspeed, void ( *func ) () );
void            SUB_CalcMoveEnt( gedict_t * ent, vec3_t tdest, float tspeed,
				 void ( *func ) () );
void            SUB_UseTargets();
void            SetMovedir();
void            InitTrigger();
extern gedict_t *activator;

//
// g_mem.c
//
void           *G_Alloc( int size );
void            G_InitMemory( void );

//
// g_spawn.c
//

void            G_SpawnEntitiesFromString( void );
qboolean        G_SpawnString( const char *key, const char *defaultString, char **out );
qboolean        G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean        G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean        G_SpawnVector( const char *key, const char *defaultString, float *out );
void            SUB_Remove();
void			SUB_RM_01( gedict_t *ent );
void            SUB_Null();

//world.c
void            CopyToBodyQue( gedict_t * ent );
void			ClearBodyQue();

// client.c

extern vec3_t	VEC_ORIGIN;
extern float    intermission_running;
extern float    intermission_exittime;
extern int      modelindex_eyes, modelindex_player;

qboolean		CheckRate (gedict_t *p, char *newrate);

int				tiecount();
void			Check_SD( gedict_t *p );

void            SetChangeParms();
void            SetNewParms( qboolean from_vmMain );
void            ClientConnect();
void            PutClientInServer();
void            ClientDisconnect();
void            PlayerPreThink();
void			BothPostThink(); // <- called for player and spec
void            PlayerPostThink();
void            SuperDamageSound();

gedict_t        *SelectSpawnPoint( char *spawnname );

#define         WP_STATS_UPDATE (0.3f)
void			Print_Wp_Stats();
#define         SC_STATS_UPDATE (0.8f)
void			Print_Scores();

//spectate.c
void            SpectatorConnect();
void			PutSpectatorInServer();
void            SpectatorDisconnect();
void            SpectatorThink();

// weapons.c
extern int      impulse;
float           W_BestWeapon();
void            W_Precache();
void            W_SetCurrentAmmo();
void            SpawnBlood( vec3_t, float );
void            W_FireAxe();
void            W_FireSpikes( float ox );
void            W_FireLightning();
qboolean		W_CanSwitch( int wp, qboolean warn );

// match.c

float			CountPlayers();
float			CountRTeams();
qboolean 		isCanStart ( gedict_t *s, qboolean forceMembersWarn );
void			StartTimer ();
void			StopTimer ( int removeDemo );

//combat.c
extern gedict_t *damage_attacker, *damage_inflictor;
void            T_Damage( gedict_t * targ, gedict_t * inflictor, gedict_t * attacker,
			  float damage );
void            T_RadiusDamage( gedict_t * inflictor, gedict_t * attacker, float damage,
				gedict_t * ignore, deathType_t dtype );
void            T_BeamDamage( gedict_t * attacker, float damage );

//items.c
void			DropPowerups();
void			ShowSpawnPoints();
void			HideSpawnPoints();
void            DropBackpack();

void			adjust_pickup_time( float *current, float *total );

//triggers.c
void			play_teleport( gedict_t *sndspot );
void            spawn_tfog( vec3_t org );
void            spawn_tdeath( vec3_t org, gedict_t * death_owner );

// runes.c
void            DropRune();
void            SpawnRunes( qboolean yes );
void            TossRune();
void            ResistanceSound( gedict_t *player );
void            HasteSound( gedict_t *player );
void            RegenerationSound( gedict_t *player );

// ctf.c
void            PlayerDropFlag( gedict_t *player );
void            RegenFlags( qboolean yes );
void			AddHook( qboolean yes );
void			CTF_Obituary( gedict_t *targ, gedict_t *attacker );

// logs.c
void log_open( const char *fmt, ... );
void log_printf( const char *fmt, ... );
void log_close(void);

extern fileHandle_t log_handle;

// commands.c
typedef struct cmd_s {
	char    *name;
//	func_t	f;
	void ( *f )();
	float	arg;
	int		cf_flags;
	const char *description;
} cmd_t;

#define CF_PLAYER			( 1<<0 ) /* command valid for players */
#define CF_SPECTATOR		( 1<<1 ) /* command valid for specs */
#define CF_BOTH				( CF_PLAYER | CF_SPECTATOR ) /* command valid for both: specs and players */
#define CF_PLR_ADMIN		( 1<<2 ) /* client is player, so this command require admin rights */
#define CF_SPC_ADMIN		( 1<<3 ) /* client is spectator, so this command require admin rights */
#define CF_BOTH_ADMIN		( CF_PLR_ADMIN | CF_SPC_ADMIN ) /* this command require admin rights, any way */
#define CF_MATCHLESS		( 1<<4 ) /* command valid for matchLess mode */
#define CF_PARAMS			( 1<<5 ) /* command have some params */
#define CF_NOALIAS			( 1<<6 ) /* command haven't alias and may be accessed only via /cmd commandname */
#define CF_REDIRECT			( 1<<7 ) /* command will be redirected to server as /cmd commandname */
#define CF_MATCHLESS_ONLY	( 1<<8 ) /* command valid for matchLess mode _only_ */

extern cmd_t cmds[];

extern int cmds_cnt; // count of commands in 'cmds'

//
// DoCommand/DoCommand_Name return codes
// return non-negative value if command success
// 
#define DO_OUT_OF_RANGE_CMDS			(-1) // if command is out of range in 'cmds' array or not found
#define DO_WRONG_CLASS					(-2) // if wrong class
#define DO_ACCESS_DENIED				(-3) // if access denied
#define DO_FUNCTION_IS_WRONG			(-4) // if function is wrong
#define DO_CMD_DISALLOWED_MATCHLESS		(-5) // if cmd does't allowed in matchLess mode
#define DO_FLOOD_PROTECT				(-6) // if command is blocked due to flood protect
#define DO_CMD_MATCHLESS_ONLY			(-7) // if cmd allowed in matchLess mode _only_

int DoCommand(int icmd);
int DoCommand_Name(char *cmd_name);

qboolean isCmdFlood(gedict_t *p);

void Init_cmds(void);

float StuffDeltaTime(int iDelta);

#define STUFFCMDS_PER_PORTION	(1)

void SetPractice(int srv_practice_mode, const char *mapname);

qboolean check_master();

// { usermodes

// um_flags

#define UM_1ON1		( 1<<0  )
#define UM_2ON2		( 1<<1  )
#define UM_3ON3		( 1<<2  )
#define UM_4ON4		( 1<<3  )
#define UM_10ON10	( 1<<4  )
#define UM_FFA		( 1<<5  )
#define UM_CTF		( 1<<6  )

typedef struct usermode_s {
	const char 	  *name;
	const char 	  *displayname;
	const char    *initstring;
	int		um_flags;
} usermode;

extern usermode um_list[];
extern int um_cnt;  // count of entrys in 'um_list'

// for user call this like UserMode( 1 )
// for server call like UserMode( -1 )
void UserMode(float umode);

int um_idx_byname(char *name); // return -1 if not found

// }

// { spec more info

qboolean	mi_on();
qboolean	mi_adm_only();
void		mi_print( gedict_t *tooker, int it, char *msg );
void		info_mi_update( gedict_t *p, char *from, char *to );

// }

// { communication commands

void s_lr_clear( gedict_t *dsc );

// }

// vip.c

// qqshka: hmm, like ktpro

#define VIP_NORMAL      ( 1) // normal VIP (default)
#define VIP_NOTKICKABLE ( 2) // not kickable VIP by elected admins
#define	VIP_ADMIN       ( 4) // VIP with admin rights
//		 8 - VIP with demo admin rights
//		16 - VIP with judge rights
#define VIP_RCON		(32) // VIP with rcon admin rights


int				VIP(gedict_t* cl);
int				VIP_IsFlags(gedict_t* cl, int flags);
void			VIP_ShowRights(gedict_t* cl);

// g_userinfo.c

typedef struct cmdinfo_s {
	char    *key;
	void ( *f )( gedict_t *p, char *from, char *to );
} cmdinfo_t;

char	*cmdinfo_getkey( gedict_t *p, char *key );
int		cmdinfo_setkey( gedict_t *p, char *key, char *value );
void	cmdinfo_infoset ( gedict_t *p );
void	cmdinfo_clear ( gedict_t *p );

// vote.c

typedef struct votemap_s {
	int		map_id;
	int		map_votes;
	int		admins;
} votemap_t;

extern  votemap_t maps_voted[];
int 	vote_get_maps ();

qboolean is_elected(gedict_t *p, electType_t et);

int  	get_elect_type ();
char 	*get_elect_type_str ();

void	vote_clear( int fofs );
int		get_votes_req( int fofs, qboolean diff );
int 	get_votes( int fofs );
int		get_votes_by_value( int fofs, int value );
int		is_admins_vote( int fofs );

void	vote_check_map ();
void	vote_check_break ();
void	vote_check_elect ();
void	vote_check_pickup ();
void	vote_check_rpickup ();
void 	vote_check_all ();

#define	VOTE_FOFS(x) ((int)&(((vote_t *)0)->x))

#define OV_BREAK ( VOTE_FOFS ( brk ) )
#define OV_ELECT ( VOTE_FOFS ( elect ) )
#define OV_PICKUP ( VOTE_FOFS ( pickup ) )
#define OV_RPICKUP ( VOTE_FOFS ( rpickup ) )
#define OV_MAP ( VOTE_FOFS ( map ) )

void 	ElectThink();
void	AbortElect();

// admin.c

// admin flags
#define AF_ADMIN       (1<<0) // elected admin
#define AF_REAL_ADMIN  (1<<1) // pass/vip granted admin (real admin in terms of ktpro)

qboolean is_real_adm(gedict_t *p); // is pass/vip granted admin (real admin in terms of ktpro)
qboolean is_adm(gedict_t *p);      // is elected admin (admin rigths granted by /elect command)

void	KickThink ();
void	ExitKick(gedict_t *kicker);

void 	ModPause (int pause);
void 	BecomeAdmin(gedict_t *p, int adm_flags);
void 	VoteAdmin();

void	PlayerStopFire(gedict_t *p);

// arena.c

void		ra_init_que();
gedict_t	*ra_que_first();
void		ra_in_que( gedict_t *p );
void		ra_out_que( gedict_t *p );
qboolean	ra_isin_que( gedict_t *p );
int			ra_pos_que( gedict_t *p );
qboolean	isRA( ); // not game mode, but just modificator of duel
qboolean	isWinner( gedict_t *p );
qboolean	isLoser( gedict_t *p );
gedict_t	*getWinner();
gedict_t	*getLoser();
void		ra_Precache();
void		ra_ClientDisconnect();
void		ra_ClientObituary( gedict_t *targ, gedict_t *attacker );
void		ra_PutClientInServer();
void		RocketArenaPre();
qboolean	readytostart();
void		ra_Frame();
void		setfullwep( gedict_t *anent );

// { ra commands
void		ra_PlayerStats();
void		ra_PrintPos();
void		ra_break();
// }

// captain.c

int		capt_num(gedict_t *p);

// maps.c

char 	*GetMapName(int imp);
void 	DoSelectMap(int iMap);
void 	SelectMap();

// match.c

void	EndMatch ( float skip_log );

// grapple.c
void    GrappleThrow();
void    GrappleService();
void    GrappleReset(gedict_t *rhook);

// globals.c

extern	float framechecks;	    // if timedemo/uptime bugs are tolerated
extern	float k_attendees;      // stores number of players on server - used in 'ready' checking
extern	float k_berzerk;        // boolean - whether or not the game is currently in berzerk time
extern	float k_berzerkenabled; // actually stores the amount of berzerk time at the end of match
extern	float k_captains;	    // number of captains
extern	float k_captainturn;	// which captain comes in line to pick
extern	float k_checkx;
extern	float k_force;          // used in forcing matchstart
extern	float k_maxspeed;       // used to store server maxspeed to allow switching by admins
extern	float k_oldmaxspeed;    // used to store old value of maxspeed prior to freezing the map
extern	float k_pause;
extern	float k_pausetime;      // stores time at which server was paused
extern	float k_showscores;     // whether or not should print the scores or not
extern	float k_nochange;       // used to indicate if frags changes somehow since last time 'scores' command was called
extern	float k_standby;        // if server is in standy mode
extern	float k_sudden_death;	// to mark if sudden death overtime is currently the case
extern	float k_teamid;
extern	float k_userid;
extern	float k_whonottime;     // NOT_SURE: 
extern	float match_in_progress;// if a match has begun
extern  float match_start_time;	// time when match has been started
extern	float match_over;       // boolean - whether or not the match stats have been printed at end of game
extern	gedict_t *newcomer;     // stores last player who joined
extern	int   k_overtime;		// is overtime is going on

extern	float current_maxfps;	// current value of serverinfo maxfps

// { jawn mode
extern	int   k_jawnmode;		// is server in jawn mode
extern	int   k_fallbunny_cap;	// fallbunny cap procent in jawn mode
extern	int   k_teleport_cap;	// cap for keeping velocity through tele
// }

extern	int   k_practice;		// is server in practice mode
extern	int   k_matchLess;	    // is server in matchLess mode
extern  gameType_t 	  k_mode;   // game type: DUEL, TP, FFA
extern	int   k_lastvotedmap;	// last voted map, used for agree command?
// { CTF
extern  int k_ctf_custom_models;// use or not custom models
extern  int k_allowed_free_modes; // reflect appropriate cvar - but changed only at map load
#ifdef CTF_RELOADMAP
extern  qboolean k_ctf;			// is ctf was active at map load
#endif
// }


// { cmd flood protection

extern  int   k_cmd_fp_count;    // 10 commands allowed ..
extern  float k_cmd_fp_per;      // per 4 seconds
extern  float k_cmd_fp_for;      // skip commands for 5 seconds
extern  int   k_cmd_fp_kick;     // kick after 4 warings
extern  int   k_cmd_fp_dontkick; // if 1 - don't use kick
extern  int   k_cmd_fp_disabled; // if 1 - don't use cmd floodprot

// }

extern  float k_sv_locktime; // some time before non VIP players can't connect, spectators not affected

extern  qboolean	vw_available; // vwep extension available
extern  qboolean	vw_enabled; // vweps enabled

// { rocket arena

extern  float		time_to_start;	//time to start match
extern	int			ra_match_fight;	// have winner and loser fighting
extern  int			k_rocketarena;	// is RA active or not, since we must catch changes, we can't use cvar("k_rocketarena")

// }

extern	int	jumpf_flag; // falling velocity criteria

extern	float f_check; // is we in state of some f_xxx check

extern	float lastTeamLocationTime; // next udate for CheckTeamStatus()

extern	qboolean first_rl_taken; // true when some one alredy took rl

extern	int sv_minping; // used to broadcast changes

// heh, some hack for mvdsv for grabbing some data

#define	OFS_NULL		0
#define	OFS_RETURN		1
#define	OFS_PARM0		4		/* leave 3 ofs for each parm to hold vectors */
#define	OFS_PARM1		7
#define	OFS_PARM2		10
#define	OFS_PARM3		13
#define	OFS_PARM4		16
#define	OFS_PARM5		19
#define	OFS_PARM6		22
#define	OFS_PARM7		25
#define	RESERVED_OFS	28

#define	G_FLOAT(o) ( ((float*)(&g_globalvars))[o])

#define LOCALINFO_MAPS_LIST_START	1000
#define LOCALINFO_MAPS_LIST_END		1999

