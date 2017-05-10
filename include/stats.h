
#ifndef STATS_H
#define STATS_H

#define MAX_TM_STATS (MAX_CLIENTS)

typedef struct teamStats_s {
	char *name; // team name
	int gfrags; // frags from ghosts
	int frags, deaths, tkills;
	float dmg_t, dmg_g, dmg_team, dmg_eweapon;
	// { ctf
	float res, str, rgn, hst;
	int caps, pickups, returns, f_defends, c_defends;
	// }
	wpType_t wpn[wpMAX];
	itType_t itm[itMAX];
	int transferred_packs;
} teamStats_t;

extern teamStats_t tmStats[MAX_TM_STATS];
extern int tmStats_cnt;

typedef struct stats_format_s {
	char* name;
	void(*match_header)(fileHandle_t handle, char* ip, int port);
	void(*match_footer)(fileHandle_t handle);

	void(*teams_header)(fileHandle_t handle);
	void(*team_detail)(fileHandle_t handle, int num, teamStats_t* stats);
	void(*teams_footer)(fileHandle_t handle, int teams);

	void(*players_header)(fileHandle_t handle);
	void(*player_detail)(fileHandle_t handle, int num, gedict_t* plr, const char* team);
	void(*players_footer)(fileHandle_t handle, int players);

	void(*race_detail)(fileHandle_t handle);
} stats_format_t;

#define FILE_FORMAT_DECL(x) \
void x##_match_header(fileHandle_t handle, char* ip, int port ); \
void x##_match_footer(fileHandle_t handle); \
void x##_teams_header(fileHandle_t handle); \
void x##_team_detail(fileHandle_t handle, int num, teamStats_t* stats); \
void x##_teams_footer(fileHandle_t handle, int team_count); \
void x##_players_header(fileHandle_t handle); \
void x##_player_detail(fileHandle_t handle, int num, gedict_t* plr, const char* team); \
void x##_players_footer(fileHandle_t handle, int players); \
void x##_race_detail(fileHandle_t handle);

#define FILE_FORMAT_DEF(x) { \
	#x, \
	x##_match_header, \
	x##_match_footer, \
	x##_teams_header, \
	x##_team_detail, \
	x##_teams_footer, \
	x##_players_header, \
	x##_player_detail, \
	x##_players_footer, \
	x##_race_detail \
}

void s2di(fileHandle_t file_handle, const char *fmt, ...);
qbool itPowerup(itemName_t it);
char *ItName(itemName_t it);
char *GetMode(void);

#endif // STATS_H