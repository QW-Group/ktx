// match.c

#include "g_local.h"

void NextLevel ();
void StopTimer ( int removeDemo );
void IdlebotForceStart ();
void CheckAll();
void StartMatch ();
void StartTimer ();
void remove_specs_wizards ();

float CountALLPlayers ()
{
	gedict_t	*p;
	float		num = 0;

	p = find( world, FOFCLSN, "player" );
	while ( p ) {
		if ( !strnull( p->s.v.netname ) )
			num++;
		p = find ( p, FOFCLSN, "player" );
	}
	return num;
}

float CountPlayers()
{
	gedict_t	*p;
	float		num = 0;

	p = find( world, FOFCLSN, "player" );
	while ( p ) {
		if ( !strnull( p->s.v.netname ) && p->k_accepted == 2 )
			num++;
		p = find ( p, FOFCLSN, "player" );
	}
	return num;
}

float CountRPlayers()
{
	gedict_t	*p;
	float		num = 0;

	p = find( world, FOFCLSN, "player" );
	while ( p ) {
		if ( !strnull( p->s.v.netname ) && p->k_accepted == 2 && p->ready )
			num++;
		p = find ( p, FOFCLSN, "player" );
	}
	return num;
}

float CountTeams()
{
	gedict_t	*p, *p2;
	float		num;
	char		*s1, *s2;

	num = 0;
	p = find ( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull( p->s.v.netname ) && p->k_accepted == 2 )
			p->k_flag = 0;

		p = find ( p, FOFCLSN, "player" );
	}

	p = find ( world, FOFCLSN, "player" );
	while( p ) { // unaccepted players may have !k_flag so check k_accepted anyway
		if( !strnull( p->s.v.netname ) && !p->k_flag && p->k_accepted == 2 ) {
			p->k_flag = 1;

			s1 = ezinfokey ( p, "team" );
			if( !strnull( s1 ) ) {
				num++;

				p2 = find ( p, FOFCLSN, "player" );
				while( p2 ) {
					s1 = ezinfokey ( p, "team" );
					s2 = ezinfokey ( p2, "team" );
					if( streq( s1, s2 ) )
						p2->k_flag = 1;
					p2 = find ( p2, FOFCLSN, "player" );
				}
			}
		}
		p = find ( p, FOFCLSN, "player" );
	}
	return num;
}

float CountRTeams()
{
	gedict_t	*p, *p2;
	float		num;
	char		*s1, *s2;

	num = 0;
	p = find ( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull( p->s.v.netname ) && p->k_accepted == 2 )
			p->k_flag = 0;

		p = find ( p, FOFCLSN, "player" );
	}

	p = find ( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull( p->s.v.netname ) && !p->k_flag && p->ready && p->k_accepted == 2 ) {
			p->k_flag = 1;

			s1 = ezinfokey ( p, "team" );
			if( !strnull( s1 ) ) {
				num++;

				p2 = find ( p, FOFCLSN, "player" );
				while( p2 ) {
					s1 = ezinfokey ( p, "team" );
					s2 = ezinfokey ( p2, "team" );
					if( streq( s1, s2 ) )
						p2->k_flag = 1;
					p2 = find ( p2, FOFCLSN, "player" );
				}
			}
		}
		p = find ( p, FOFCLSN, "player" );
	}
	return num;
}

// check count of members in each team i'm guess
// and return 0 if at least one team has less members than 'memcnt'
// else return 1 (even we have more mebers than memcnt, dunno is this bug <- FIXME)

float CheckMembers ( float memcnt )
{
	gedict_t	*p, *p2;
	float		f1;
	char		*s1, *s2;

	p = find ( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull( p->s.v.netname ) && p->k_accepted == 2 )
			p->k_flag = 0;

		p = find ( p, FOFCLSN, "player" );
	}

	p = find ( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull( p->s.v.netname ) && !p->k_flag && p->ready && p->k_accepted == 2 ) {
			p->k_flag = 1;
			f1 = 1;
			s1 = ezinfokey ( p, "team" );

			if( !strnull ( s1 ) ) {
				p2 = find ( p, FOFCLSN, "player" );

				while( p2 ) {
					s1 = ezinfokey ( p, "team" );
					s2 = ezinfokey ( p2, "team" );

					if( streq( s1, s2 ) ) {
						p2->k_flag = 1;
						f1++;
					}
					p2 = find ( p2, FOFCLSN, "player" );
				}
			}

			if ( f1 < memcnt )
				return 0;
		}

		p = find ( p, FOFCLSN, "player" );
	}
	return 1;
}

void ShowTeamsBanner ( )
{
	int i;

	G_bprint(2, "ùûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûü\n");


	for( i = 666 + 1; i <= k_teamid ; i++ )
		G_bprint(2, "%sê%së", (i != (666+1) ? " vs " : ""), ezinfokey(world, va("%d", i)));

	G_bprint(2, " %s:\n", redtext("match statistics"));

	G_bprint(2, "ùûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûü\n");
}


void SummaryTPStats ( )
{
	gedict_t	*p, *p2;
	float	dmg_g, dmg_t;
	int   ra, ya, ga;
	int   mh;
	int   quad, pent, ring;
	float h_rl, a_rl, h_lg, a_lg, h_sg, a_sg, h_ssg, a_ssg;
	char  *tmp, *tmp2;


	ShowTeamsBanner ();

	p = find ( world, FOFCLSN, "player" );
	while( p ) {
		p->ready = 0; // clear mark
		p = find ( p, FOFCLSN, "player" );
	}

	p = find ( world, FOFCLSN, "ghost" );
	while( p ) {
		p->ready = 0; // clear mark
		p = find ( p, FOFCLSN, "ghost" );
	}

	G_bprint(2, "\n%s, %s, %s, %s\n", redtext("weapons"), redtext("powerups"),
									  redtext("armors & mhs"), redtext("damage"));
	G_bprint(2, "ùûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûü\n");

//	get one player and search all his mates, mark served players via ->ready field 
//  ghosts is served too

	p = find ( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull ( p->s.v.netname ) && !p->ready
			 && ( !strnull( ezinfokey (p, "team") ) && p->k_accepted == 2 ) ) {

			dmg_g = dmg_t = ra = ya = ga = mh = quad = pent = ring = 0;
			h_rl = a_rl = h_lg = a_lg = h_sg = a_sg = h_ssg = a_ssg = 0;

			// stats from normal players

			p2 = p;
			while ( p2 ) {
				if( !p2->ready ) {
					tmp = ezinfokey(p, "team");
					tmp2 = ezinfokey(p2, "team");
					if( streq( tmp, tmp2 ) ) {
				    	dmg_g += p2->ps.dmg_g;
				    	dmg_t += p2->ps.dmg_t;
				    	ra    += p2->ps.ra;
				    	ya    += p2->ps.ya;
				    	ga    += p2->ps.ga;
				    	mh    += p2->ps.mh;
				    	quad  += p2->ps.quad;
				    	pent  += p2->ps.pent;
				    	ring  += p2->ps.ring;

						h_rl  += p2->ps.h_rl;
						a_rl  += p2->ps.a_rl;
						h_lg  += p2->ps.h_lg;
						a_lg  += p2->ps.a_lg;
						h_sg  += p2->ps.h_sg;
						a_sg  += p2->ps.a_sg;
						h_ssg += p2->ps.h_ssg;
						a_ssg += p2->ps.a_ssg;

						p2->ready = 1; // set mark
					}
				}

				p2 = find ( p2, FOFCLSN, "player" );
			}

			// stats from ghost players

			p2 = find( world, FOFCLSN, "ghost" );
			while( p2 ) {
				if( !p2->ready ) {
					tmp  = ezinfokey(p, "team");
					tmp2 = ezinfokey(world, va("%d", (int)p2->k_teamnum));
					if ( streq( tmp, tmp2 ) ) {
				    	dmg_g += p2->ps.dmg_g;
				    	dmg_t += p2->ps.dmg_t;
				    	ra    += p2->ps.ra;
				    	ya    += p2->ps.ya;
				    	ga    += p2->ps.ga;
				    	mh    += p2->ps.mh;
				    	quad  += p2->ps.quad;
				    	pent  += p2->ps.pent;
				    	ring  += p2->ps.ring;

						h_rl  += p2->ps.h_rl;
						a_rl  += p2->ps.a_rl;
						h_lg  += p2->ps.h_lg;
						a_lg  += p2->ps.a_lg;
						h_sg  += p2->ps.h_sg;
						a_sg  += p2->ps.a_sg;
						h_ssg += p2->ps.h_ssg;
						a_ssg += p2->ps.a_ssg;

						p2->ready = 1; // set mark
					}
				}

				p2 = find ( p2, FOFCLSN, "ghost" );
			}

			h_sg  = 100.0 * h_sg  / max(1, a_sg);
			h_ssg = 100.0 * h_ssg / max(1, a_ssg);
#if 0 /* percentage */
//			h_gl  = 100.0 * h_gl  / max(1, a_gl);
			h_rl  = 100.0 * h_rl  / max(1, a_rl);
#else /* just count of direct hits */
//			h_gl  = h_gl;
			h_rl  = h_rl;
#endif
			h_lg  = 100.0 * h_lg  / max(1, a_lg);

			// weapons
			G_bprint(2, "ê%së: %s:%s%s%s%s\n", ezinfokey ( p, "team" ), redtext("Wp"),
						(h_lg  ? va(" %s%.1f%%", redtext("lg"),   h_lg) : ""),
						(h_rl  ? va(" %s%.0f",   redtext("rl"),   h_rl) : ""), 
						(h_sg  ? va(" %s%.1f%%", redtext("sg"),   h_sg) : ""),
						(h_ssg ? va(" %s%.1f%%", redtext("ssg"), h_ssg) : ""));
			// powerups
			G_bprint(2, "%s: %s:%d %s:%d %s:%d\n", redtext("Powerups"),
					redtext("Q"), quad, redtext("P"), pent, redtext("R"), ring);
			// armors + megahealths
			G_bprint(2, "%s: %s:%d %s:%d %s:%d %s:%d\n", redtext("Armr&mhs"),
					redtext("ga"), ga, redtext("ya"), ya, redtext("ra"), ra, redtext("mh"), mh);
			// damage
			G_bprint(2, "%s: %s:%.1f %s:%.1f\n", redtext("  Damage"),
						redtext("Taken"), dmg_t, redtext("Given"), dmg_g);
		}

		p = find ( p, FOFCLSN, "player" );
	}

	G_bprint(2, "ùûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûü\n");
}


void TeamsStats ( )
{
	gedict_t	*p=NULL, *p2=NULL;
	float		f1, f2;
	char		*tmp=NULL, *tmp2=NULL;
	int		sumfrags = 0, wasPrint = 0;

	// Summing up the frags to calculate team percentages
	p = find ( world, FOFCLSN, "player" );
	while( p ) {
		p->ready = 0; // clear mark

		if( !strnull ( p->s.v.netname )
			&& ( !strnull( ezinfokey (p, "team") ) && p->k_accepted == 2 ) )
			sumfrags += p->s.v.frags;

		p = find ( p, FOFCLSN, "player" );
	}

	p = find ( world, FOFCLSN, "ghost" );
	while( p ) {
		p->ready = 0; // clear mark
		sumfrags += p->s.v.frags;

		p = find ( p, FOFCLSN, "ghost" );
	}
	// End of summing

	G_bprint(2, "\n%s: %s\n"
				"ùûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûü\n", redtext("Team scores"),
													     redtext("frags è percentage"));

//	get one player and search all his mates, mark served players via ->ready field 
//  ghosts is served too

	p = find ( world, FOFCLSN, "player" );
	while( p ) {
		if( !strnull ( p->s.v.netname ) && !p->ready
			 && ( !strnull( ezinfokey (p, "team") ) && p->k_accepted == 2 ) ) {

			f1 = 0; // frags from normal players

			p2 = p;
			while ( p2 ) {
				tmp = ezinfokey(p, "team");
				tmp2 = ezinfokey(p2, "team");
				if( streq( tmp, tmp2 ) ) {
					f1 += p2->s.v.frags;
					p2->ready = 1; // set mark
				}

				p2 = find ( p2, FOFCLSN, "player" );
			}

			f2 = 0; // frags from ghost players

			p2 = find( world, FOFCLSN, "ghost" );
			while( p2 ) {
				if( !p2->ready ) {
					tmp2 = ezinfokey(world, va("%d", (int)p2->k_teamnum));
					tmp  = ezinfokey(p, "team");
					if ( streq( tmp, tmp2 ) ) {
						f2 += p2->s.v.frags;
						p2->ready = 1; // set mark
					}
				}

				p2 = find ( p2, FOFCLSN, "ghost" );
			}

			G_bprint(2, "ê%së: %d", ezinfokey ( p, "team" ), (int) f1 );

			if( f2 )
				G_bprint( 2, tmp, " + (%d) = %d", (int)f2, (int)(f1+f2) );

			// effi
			G_bprint(2, " è %.1f%%\n", ((sumfrags > 0)? ((f1 + f2)/sumfrags * 100) : 0));

			wasPrint = 1;
		}

		p = find ( p, FOFCLSN, "player" );
	}

	if ( wasPrint )
		G_bprint(2, "ùûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûü\n");
}

float maxfrags, maxdeaths, maxfriend, maxeffi;

void OnePlayerStats(gedict_t *p, int tp)
{
	float	dmg_g, dmg_t;
	int   ra, ya, ga;
	int   mh;
	int   quad, pent, ring;
	float h_rl, a_rl, h_gl, a_gl, h_lg, a_lg, h_sg, a_sg, h_ssg, a_ssg;

	dmg_g = p->ps.dmg_g;
	dmg_t = p->ps.dmg_t;
	ra    = p->ps.ra;
	ya    = p->ps.ya;
	ga    = p->ps.ga;
	mh    = p->ps.mh;
	quad  = p->ps.quad;
	pent  = p->ps.pent;
	ring  = p->ps.ring;

	h_rl  = p->ps.h_rl;
	a_rl  = p->ps.a_rl;
	h_gl  = p->ps.h_gl;
	a_gl  = p->ps.a_gl;
	h_lg  = p->ps.h_lg;
	a_lg  = p->ps.a_lg;
	h_sg  = p->ps.h_sg;
	a_sg  = p->ps.a_sg;
	h_ssg = p->ps.h_ssg;
	a_ssg = p->ps.a_ssg;

	h_sg  = 100.0 * h_sg  / max(1, a_sg);
	h_ssg = 100.0 * h_ssg / max(1, a_ssg);
#if 0 /* percentage */
	h_gl  = 100.0 * h_gl  / max(1, a_gl);
	h_rl  = 100.0 * h_rl  / max(1, a_rl);
#else /* just count of direct hits */
	h_gl  = h_gl;
	h_rl  = h_rl;
#endif
	h_lg  = 100.0 * h_lg  / max(1, a_lg);

	if ( tp )
		G_bprint(2,"ùûûûûûûûûü\n" );

	G_bprint(2, "\x87 %s%s:\n"
			"  %d (%d) %s%.1f%%\n", ( isghost( p ) ? "\x83" : "" ), getname(p),
			(int)p->s.v.frags,	(int)(p->s.v.frags - p->deaths),
			( tp ? va("%d ", (int)p->friendly ) : "" ),
			p->efficiency);

// qqshka - force show this always
//	if ( !tp || atoi( ezinfokey(world, "tp_players_stats") ) ) {
		// weapons
		G_bprint(2, "%s:%s%s%s%s%s\n", redtext("Wp"),
				(h_lg  ? va(" %s%.1f%%", redtext("lg"),   h_lg) : ""),
				(h_rl  ? va(" %s%.0f",   redtext("rl"),   h_rl) : ""),
				(h_gl  ? va(" %s%.0f",   redtext("gl"),   h_gl) : ""),
				(h_sg  ? va(" %s%.1f%%", redtext("sg"),   h_sg) : ""),
				(h_ssg ? va(" %s%.1f%%", redtext("ssg"), h_ssg) : ""));
		// armors + megahealths
		G_bprint(2, "%s: %s:%d %s:%d %s:%d %s:%d\n", redtext("Armr&mhs"),
				redtext("ga"), ga, redtext("ya"), ya, redtext("ra"), ra, redtext("mh"), mh);
		// damage
		G_bprint(2, "%s: %s:%.1f %s:%.1f\n", redtext("  Damage"),
				redtext("Taken"), dmg_t, redtext("Given"), dmg_g);
		//  endgame h & a
		G_bprint(2, "  %s  H&A: êH:%dëèêA:%s:%dë\n", redtext("EndGame"),
				(int)p->s.v.health, armor_type((int)p->s.v.items), (int)p->s.v.armorvalue );
		// overtime h & a
		if ( k_overtime )
			G_bprint(2, "  %s H&A: êH:%dëèêA:%s:%dë\n", redtext("OverTime"),
				(int)p->ps.ot_h, armor_type((int)p->ps.ot_items), (int)p->ps.ot_a );
		// spawnfrags
		G_bprint(2, "  %s: ê%dë\n", redtext("SpawnFrags"), p->ps.spawn_frags);
//	}

	if ( !tp )
		G_bprint(2,"ùûûûûûûûûü\n" );

	if (maxfrags < p->s.v.frags)
		maxfrags = p->s.v.frags;
	if (maxdeaths < p->deaths)
		maxdeaths = p->deaths;
	if (maxfriend < p->friendly)
		maxfriend = p->friendly;
	if (maxeffi < p->efficiency)
		maxeffi = p->efficiency;
}

// Players statistics printout here
void PlayersStats ()
{
	gedict_t	*p, *p2;
	char		*tmp, *tmp2;
	int			tp, first, from1, from2;

	from1 = 0;
	p = find_plr ( world, &from1 );
	while( p ) {
		p->ready = 0; // clear mark
		p = find_plr ( p, &from1 );
	}

	// Probably low enough for a start value :)
	maxfrags = -999999;

	maxeffi = maxfriend = maxdeaths = 0;

	tp = isTeam();

	G_bprint(2, "\n%s:\n"
				"%s (%s) %sè %s\n"
				"ùûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûü\n", redtext("Player statistics"),
				redtext( "Frags"), redtext( "rank"), ( tp ? redtext("friendkills "): "" ), redtext( "efficiency" ));

	from1 = 0;
	p = find_plr ( world, &from1 );
	while( p ) {
		if( !p->ready ) {

			first = 1;

			from2 = 0;
			p2 = find_plr ( world, &from2 );
			while ( p2 ) {
				if( !p2->ready ) {
					// sort by team
					tmp = getteam(p);
					tmp2 = getteam(p2);

					if( streq ( tmp, tmp2 ) ) {
						p2->ready = 1; // set mark

						if ( first ) {
							first = 0;
							if ( tp )
								G_bprint(2, "Team ê%së:\n", tmp );
						}

						if(p2->s.v.frags < 1)
							p2->efficiency = 0;
						else
							p2->efficiency = p2->s.v.frags / (p2->s.v.frags + p2->deaths) * 100;

						OnePlayerStats(p2, tp);
					}
				}

				p2 = find_plr ( p2, &from2 );

				if ( !p2 )
					G_bprint(2, "\n"); // split players from different teams via \n
			}
		}

		p = find_plr ( p, &from1 );
	}
}

// Print the high score table
void TopStats ( )
{
	gedict_t	*p;
	float		f1;
	int			from;

	G_bprint(2, "ê%së %s:\n"
				"ùûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûü\n"
				"      Frags: ", g_globalvars.mapname, redtext("top scorers"));

	from = f1 = 0;
	p = find_plr ( world, &from );
	while( p ) {
		if( p->s.v.frags == maxfrags ) {
			G_bprint(2, "%s%s%s ê%dë\n", (f1 ? "             " : ""),
								( isghost( p ) ? "\x83" : "" ), getname( p ), (int)maxfrags);
			f1 = 1;
		}

		p = find_plr ( p, &from );
	}


	G_bprint(2, "     Deaths: ");

	from = f1 = 0;
	p = find_plr ( world, &from );
	while( p ) {
		if( p->deaths == maxdeaths ) {
			G_bprint(2, "%s%s%s ê%dë\n", (f1 ? "             " : ""),
								( isghost( p ) ? "\x83" : "" ),	getname( p ), (int)maxdeaths);
			f1 = 1;
		}

		p = find_plr ( p, &from );
	}

	if( maxfriend ) {
		G_bprint(2, "Friendkills: ");

		from = f1 = 0;
		p = find_plr ( world, &from );
		while( p ) {
			if( p->friendly == maxfriend ) {
				G_bprint(2, "%s%s%s ê%dë\n", (f1 ? "             " : ""),
								( isghost( p ) ? "\x83" : "" ),	getname( p ), (int)maxfriend);
				f1 = 1;
			}

			p = find_plr ( p, &from );
		}
	}

	G_bprint(2, " Efficiency: ");

	from = f1 = 0;
	p = find_plr ( world, &from );
	while( p ) {
		if( p->efficiency == maxeffi ) {
			G_bprint(2, "%s%s%s ê%.1f%%ë\n", (f1 ? "             " : ""),
								( isghost( p ) ? "\x83" : "" ),	getname( p ), maxeffi);
			f1 = 1;
		}

		p = find_plr ( p, &from );
	}

	G_bprint(2, "ùûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûûü\n");
}

// WARNING: if we are skip log, we are also delete demo

void EndMatch ( float skip_log )
{
	gedict_t	*p;

	int fpd = iKey ( world, "fpd" );
	int old_match_in_progress = match_in_progress;
	char *tmp;
	float f1;

	if( match_over )
		return;

	match_over = 1;

// s: zero the flag
	k_berzerk = k_sudden_death = 0;

	tmp = ezinfokey( world, "k_host" );
	if( !strnull( tmp ) )
		trap_cvar_set( "hostname", tmp );

	trap_lightstyle(0, "m");

	fpd = fpd & ~64;
	localcmd("serverinfo fpd %d\n", fpd);

	localcmd("sv_spectalk 1\n");

	G_bprint( 2, "The match is over\n");
	G_cprint("RESULT");

	if( skip_log )
		G_cprint("%%stopped\n");
	else {
		p = find( world, FOFCLSN, "player" );
		while( p ) 
		{
			if( !strnull( p->s.v.netname ) && p->k_accepted == 2 )
			{
				G_cprint("%%%s", p->s.v.netname);
				G_cprint("%%t%%%s", ezinfokey(p, "team"));
				G_cprint("%%fr%%%d", (int)p->s.v.frags);

				// take away powerups so scoreboard looks normal
				p->s.v.items = (int)p->s.v.items & ~(IT_INVISIBILITY | IT_INVULNERABILITY | IT_SUIT | IT_QUAD);
				p->s.v.effects = (int)p->s.v.effects & ~(EF_DIMLIGHT | EF_BLUE | EF_RED );
				p->invisible_finished = 0;
				p->invincible_finished = 0;
				p->super_damage_finished = 0;
				p->radsuit_finished = 0;
			}

			p = find ( p, FOFCLSN, "player" );
		}

		G_cprint("%%fl%%%d", (int)fraglimit);
		G_cprint("%%tl%%%d", (int)timelimit);
		G_cprint("%%map%%%s\n", g_globalvars.mapname);

        if( isTeam() )
			SummaryTPStats ();

		PlayersStats ();

        if( !isDuel() ) // top stats only in non duel modes
			TopStats ();

        if( isTeam() )
			TeamsStats ();

		if ( p = find( world, FOFCLSN, "ghost" ) ) // show legend :)
			G_bprint(2, "\n\x83 - %s player\n\n", redtext("disconnected"));
	}

	p = find( world, FOFCLSN, "ghost" );
	while( p ) {
		ent_remove( p );
		p = find( p, FOFCLSN, "ghost" );
	}

	StopTimer( skip_log ); // WARNING: if we are skip log, we are also delete demo

	for( f1 = 666; k_teamid >= f1 ; f1++ )
		localcmd("localinfo %d \"\"\n", (int)f1); //removing key

	for( f1 = 1; k_userid >= f1; f1++ )
		localcmd("localinfo %d \"\"\n", (int)f1); //removing key

	if ( old_match_in_progress == 2 ) {
		for ( p = world; p = find( p, FOFCLSN, "player" ); )
			p->ready = 0; // force players be not ready after match is end.
	}

	NextLevel();
}

void SaveOvertimeStats ()
{
	gedict_t	*p;

	if ( k_overtime ){
		p = find ( world, FOFCLSN, "player" );

		while( p ) 
		{
		 	// save overtime stats
			p->ps.ot_a	    = (int)p->s.v.armorvalue;
			p->ps.ot_items	=      p->s.v.items; // float
			p->ps.ot_h	    = (int)p->s.v.health;

			p = find ( p, FOFCLSN, "player" );
		}

	}
}

void TimerThink ()
// Called every second during a match. cnt = minutes, cnt2 = seconds left.
// Tells the g_globalvars.time every now and then.
{
	gedict_t	*p;
	float f1, f2, f3, f4, k_exttime, player1scores, player2scores, player1found;
	int k_mb_overtime = 0;

	f1 = k_matchLess ? 1 : 0; // don't stop match if no players left


//	G_bprint(2, "left %2d:%2d\n", (int)self->cnt, (int)self->cnt2);


	// moved out the score checking every second to every minute only.  And if someone
	// types scores it recalculates it for them.

	if( k_sudden_death )
		return;      

	if( k_pause ) {
		self->s.v.nextthink = g_globalvars.time + 1;
		return;
	}

	if( self->k_teamnum < g_globalvars.time && !k_checkx ) {
		k_checkx = 1; // global which set to true when some time spend after match start
	}

	if( !f1 )
		f1 = CountPlayers();

	if( !f1 ) {
		EndMatch( 1 );
		return;
	}

	( self->cnt2 )--;

    if( k_berzerkenabled ){
        f1 = k_berzerkenabled;
		f2 = floor(f1 / 60);
		f1 = f1 - (f2 * 60);
		f2++;

		if( self->cnt2 == f1 && self->cnt == f2 ) {
			G_bprint(2, "BERZERK!!!!\n");
			trap_lightstyle ( 0, "ob" );
			k_berzerk = 1;

			p = find ( world, FOFCLSN, "player" );
			while( p ) {
				if( !strnull ( p->s.v.netname ) && ISLIVE( p ) ) {
					p->s.v.items = (int)p->s.v.items | (4194304 | 1048576); // FIXME wtf?
					p->super_time = 1;
					p->super_damage_finished = g_globalvars.time + 3600;
					p->invincible_time = 1;
					p->invincible_finished = g_globalvars.time + 2;
					p->k_666 = 1;
				}

				p = find ( p, FOFCLSN, "player" );
			}
		}
	}

	if( !self->cnt2 ) 
	{
		int scores1 = get_scores1();
		int scores2 = get_scores2();

		self->cnt2 = 60;
		self->cnt  -= 1;

//		if ( self->cnt < 0 )
//				self->cnt = 0;

		localcmd("serverinfo status \"%d min left\"\n", (int)self->cnt);

		if( !self->cnt )
		{
			k_mb_overtime = atoi( ezinfokey( world, "k_overtime" ) );
			
			// If 0 no overtime, 1 overtime, 2 sudden death
			// And if its neither then well we exit
			if( k_mb_overtime )
			{
				f3 = CountTeams();
				f4 = CountPlayers();
				k_exttime = atoi( ezinfokey ( world, "k_exttime" ) );

				
                // Overtime.
				// Ok we have now decided that the game is ending,
				// 	so decide overtime wise here what to do.
				// Check to see if duel first

				if( k_matchLess ){
					; // no overtime in matchLess mode
				}
				if( teamplay && f3 != 2 ){
					; // no overtime in case of less then 2 or more then 2 teams
				}			
				else if( isDuel() && f4 == 2 )
				{
					player1scores = player2scores = player1found = 0;

					p = find ( world, FOFCLSN, "player" );
					while( p ) 
					{
						if ( !strnull ( p->s.v.netname ) && p->k_accepted == 2 ) {
		
							if( player1found == 0 ) 
							{
                            	player1scores = p->s.v.frags;
								player1found = 1;					
							}
							else{
                            	player2scores = p->s.v.frags;
							}
						}

						p = find ( p, FOFCLSN, "player" );
					}
					
					// In player1scores and player2scores we have scores(lol?)
					if( player1scores == player2scores )
					{
						k_overtime = k_mb_overtime;

						SaveOvertimeStats ();

						G_bprint(2, "time over, the game is a draw\n");
						if( k_overtime == 1 ) {
							// Ok its increase time
							self->cnt =  k_exttime;
							self->cnt2 = 60;

							G_bprint(2, "\x90%d\x91 minute%s overtime follows\n", 
									(int)k_exttime, count_s(k_exttime));
							self->s.v.nextthink = g_globalvars.time + 1;

							return;	
						} else {
							G_bprint(2, "Sudden death %s\n", redtext("overtime begins"));
						// added timer removal at sudden death beginning
							k_sudden_death = 1;

							p = find ( world, FOFCLSN, "timer");
							while( p ) {
								p->s.v.nextthink = g_globalvars.time + 0.1;
								p->s.v.think = ( func_t ) SUB_Remove;

								p = find(p, FOFCLSN, "timer");
							}
							return;
						}
					}
				}
				// Or it can be a team game.
				// Handle a 2v2 or above team game
				else if( teamplay && f3 == 2 && f4 > 2 && scores1 == scores2 )
				{
					k_overtime = k_mb_overtime;

					SaveOvertimeStats ();

					G_bprint(2, "time over, the game is a draw\n");
					if( k_overtime == 1 ) {
						// Ok its increase time
						self->cnt =  k_exttime;
						self->cnt2 = 60;

						G_bprint(2, "\x90%d\x91 minute%s overtime follows\n", 
								(int)k_exttime, count_s(k_exttime));
						self->s.v.nextthink = g_globalvars.time + 1;

						return;	
					} else {                      
						G_bprint(2, "Sudden death %s\n", redtext("overtime begins"));
						// added timer removal at sudden death beginning
						k_sudden_death = 1;

						p = find ( world, FOFCLSN, "timer");
						while( p ) {
							p->s.v.nextthink = g_globalvars.time + 0.1;
							p->s.v.think = ( func_t ) SUB_Remove;

							p = find(p, FOFCLSN, "timer");
						}
						return;
					}
				}
			}

			EndMatch( 0 );

			return;
		}

		G_bprint(2, "\x90%s\x91 minute%s remaining\n", dig3(self->cnt), count_s(self->cnt));

		self->s.v.nextthink = g_globalvars.time + 1;

		if( k_showscores ) {
			if( scores1 == scores2 ) {
				G_bprint(2, "The game is currently a tie\n");
			} else if( scores1 != scores2 ) {
				f1 = scores1 - scores2;
				G_bprint(2, "%s \x90%s\x91 leads by %s frag%s\n",
						redtext("Team"), ezinfokey ( world, ( f1 > 0 ? "k_team1" : "k_team2" ) ),
						dig3(abs( (int)f1 )), count_s( f1 ) );
			}
		}
		return;
	}

	if( self->cnt2 == 20 || self->cnt2 == 40 )
		CheckAll();

	if( self->cnt == 1 && ( self->cnt2 == 30 || self->cnt2 == 15 || self->cnt2 <= 10 ) )
		G_bprint(2, "\x90%s\x91 second%s\n", dig3( self->cnt2 ), count_s( self->cnt2 ) );

	self->s.v.nextthink = g_globalvars.time + 1;
}

void StartMatchLess ()
{
	if ( !k_matchLess )
		return;

	StartTimer ();
}

// remove some items from map regardind with dmm
void SM_PrepareMap()
{
	gedict_t *p;

	for( p = world; p = ( k_matchLess ? findradius2(p, VEC_ORIGIN, 999999) :
					 				    findradius(p, VEC_ORIGIN, 999999) );
	   ) {

	// going for the if content record..

		if (    streq( p->s.v.classname, "rocket" )
			 || streq( p->s.v.classname, "grenade" )
		   ) { // this must be removed in any cases
				ent_remove( p );
		}
		else if( deathmatch > 3 ) {
			if(    streq( p->s.v.classname, "weapon_nailgun" )
				|| streq( p->s.v.classname, "weapon_supernailgun" )
				|| streq( p->s.v.classname, "weapon_supershotgun" )
				|| streq( p->s.v.classname, "weapon_rocketlauncher" )
				|| streq( p->s.v.classname, "weapon_grenadelauncher" )
				|| streq( p->s.v.classname, "weapon_lightning" )
			  ) { // no weapons for any of this deathmatches (4 or 5)
				ent_remove( p );
			}
			else if ( deathmatch == 4 ) {
				if(    streq( p->s.v.classname, "item_shells" )
					|| streq( p->s.v.classname, "item_spikes" )
					|| streq( p->s.v.classname, "item_rockets" )
					|| streq( p->s.v.classname, "item_cells" )
					|| (streq( p->s.v.classname, "item_health" ) && ( int ) p->s.v.spawnflags & H_MEGA)
			      ) { // no weapon ammo and megahealth for dmm4
					ent_remove( p );
				}
			}
		} else {
			if( deathmatch == 2 && iKey( world, "k_dm2mod" ) &&
			 							(   streq( p->s.v.classname, "item_armor1" )
			  	 						 || streq( p->s.v.classname, "item_armor2" )
			     						 || streq( p->s.v.classname, "item_armorInv")
                						)
			
			  ) // no armors in modified dmm2
				ent_remove( p );
		}
	}
}

// put clients in server and reset some params
void SM_PrepareClients()
{
	int hdc, i;
	char *pl_team;
	gedict_t *p, *old;

	k_teamid = 666;
	localcmd("localinfo 666 \"\"\n");
	trap_executecmd (); // <- this really needed

	for( p = world;	p = find ( p, FOFCLSN, "player" ); ) {
		if( !k_matchLess ) { // skip setup k_teamnum in matchLess mode
			pl_team = getteam( p );
			G_cprint("%%%s%%t%%%s", p->s.v.netname, pl_team);

			p->k_teamnum = 0;

			if( !strnull( pl_team ) ) {
				i = 665;

				while( k_teamid > i && !p->k_teamnum ) {
					i++;

					if( streq( pl_team, ezinfokey(world, va("%d", i)) ) )
						p->k_teamnum = i;
				}

				if( !p->k_teamnum ) { // team not found in localinfo, so put it in
					i++;
					p->k_teamnum = k_teamid = i;
					localcmd( "localinfo %d \"%s\"\n", i, pl_team );
					trap_executecmd (); // <- this really needed
				}
			} 
			else
				p->k_teamnum = 666;
		}

		p->friendly = p->deaths = p->s.v.frags = 0;

		hdc = p->ps.handicap; // save player handicap

		memset( (void*) &( p->ps ), 0, sizeof(p->ps) ); // clear player stats

		p->ps.handicap = hdc; // restore player handicap

		old = self;
		self = p;

		SetNewParms( false );
		PutClientInServer();

		self = old;
	}

	G_cprint("\n");
}

void SM_PrepareShowscores()
{
	gedict_t *p;
	char *team1 = "", *team2 = "";

	if( k_matchLess ) // skip this in matchLess mode
		return;

	if ( !isTeam() || CountRTeams() != 2 ) // we need 2 teams
		return;

	if ( p = find ( world, FOFCLSN, "player" ) ) 
		team1 = getteam( p );

	if ( strnull( team1 ) )
		return;

	while( p = find ( p, FOFCLSN, "player" ) ) {
		team2 = getteam( p );

		if( strneq( team1, team2 ) )
			break;
	}

	if ( strnull( team2 ) )
		return;

	k_showscores = 1;

	localcmd("localinfo k_team1 \"%s\"\n", team1);
	localcmd("localinfo k_team2 \"%s\"\n", team2);
	localcmd("serverinfo hostname \"%s (%s vs. %s)\"\n", 
					ezinfokey(world, "hostname"), team1, team2);
}

// Reset player frags and start the timer.
void StartMatch ()
{
	k_berzerk    = 0;
	k_nochange   = 0;
	k_showscores = 0;
	k_standby    = 0;
	k_checkx     = 0;

	k_userid   = 1;
	localcmd("localinfo 1 \"\"\n");
	trap_executecmd (); // <- this really needed

    // Check to see if berzerk is set.
    if( iKey( world, "k_bzk" ) ) {
        k_berzerkenabled = iKey( world, "k_btime" );
    } else {
        k_berzerkenabled = 0;
    }

	SM_PrepareMap(); // remove some items from map regardind with dmm

	G_cprint("MATCH STARTED\n");

	match_in_progress = 2;

	remove_specs_wizards (); // remove wizards

	SM_PrepareClients(); // put clients in server and reset some params

	if ( !k_matchLess || cvar( "k_matchless_countdown" ) )
		G_bprint(2, "The match has begun!\n");

	{ // spec silence
		int fpd = iKey( world, "fpd" );
		int k_spectalk = bound(0, iKey(world, "k_spectalk"), 1);
		localcmd( "sv_spectalk %d\n", k_spectalk);

 		// remove 64 from fpd and add 64 to fpd if k_spectalk == 1
		fpd = (fpd & ~64) | (k_spectalk * 64);
		localcmd( "serverinfo fpd %d\n", fpd );
	}

	self->k_teamnum = g_globalvars.time + 3;  //dirty i know, but why waste space?
											  // FIXME: waste space, but be clean
	self->cnt = timelimit;
	self->cnt2 = 60;
	localcmd("serverinfo status \"%d min left\"\n", (int)timelimit);

	self->s.v.think = ( func_t ) TimerThink;
	self->s.v.nextthink = g_globalvars.time + 1;

	localcmd( "localinfo k_host \"%s\"\n", ezinfokey(world, "hostname") ); // save host name

	SM_PrepareShowscores();
}

void PrintCountdown( int seconds )
{
// Countdown: seconds
//
//
// Deathmatch  x
// Mode		  D u e l | T e a m | F F A
// Teamplay    x
// Timelimit  xx
// Fraglimit xxx
// Overtime   xx		Overtime printout, supports sudden death display
// Powerups   On|Off|Jammed
// Noweapon

	char text[1024] = {0};
	char *mode = "";
	char *pwr  = "";
	char *ot   = "";
	char *nowp = "";


	strlcat(text, va("%s: %2s\n\n\n", redtext("Countdown"), dig3(seconds)), sizeof(text));
	strlcat(text, va("%s %2s\n", "Deathmatch", dig3(deathmatch)), sizeof(text));

	if( isDuel() )
		mode = redtext("D u e l");
	else if ( isTeam() )
		mode = redtext("T e a m");
	else if ( isFFA() )
		mode = redtext("F F A");
	else
		mode = redtext("Unknown");

	strlcat(text, va("%s %8s\n", "Mode", mode), sizeof(text));
	if ( /*isTeam()*/ teamplay )
		strlcat(text, va("%s %4s\n", "Teamplay", dig3(teamplay)), sizeof(text));
	if ( timelimit )
		strlcat(text, va("%s %3s\n", "Timelimit", dig3(timelimit)), sizeof(text));
	if ( fraglimit )
		strlcat(text, va("%s %3s\n", "Fraglimit", dig3(fraglimit)), sizeof(text));

	switch ( atoi( ezinfokey( world, "k_overtime" ) ) ) {
		case 0:  ot = redtext("Off"); break;
		case 1:  ot = dig3( iKey( world, "k_exttime" ) ); break;
		case 2:  ot = redtext("sd"); break;
		default: ot	= redtext("Unkn"); break;
	}

	if ( atoi( ezinfokey( world, "k_overtime" ) ) )
		strlcat(text, va("%s %4s\n", "Overtime", ot), sizeof(text));

	switch ( Get_Powerups() ) {
		case 0:  pwr = redtext("Off"); break;
		case 1:  pwr = redtext( "On"); break;
		case 2:  pwr = redtext("Jam"); break;
		default: pwr = redtext("Unkn"); break;
	}

	strlcat(text, va("%s %4s\n", "Powerups", pwr), sizeof(text));

	if (    deathmatch == 4 
		 && !strnull( nowp = str_noweapon((int)cvar("k_disallow_weapons") & DA_WPNS) )
	   )
		strlcat(text, va("\n%s %4s\n", "Noweapon", 
					redtext(nowp[0] == 32 ? (nowp+1) : nowp)), sizeof(text));

	G_cp2all(text);
}

qboolean isCanStart ( gedict_t *s, qboolean forceMembersWarn )
{
    int k_lockmin     = iKey( world, "k_lockmin" );
    int k_lockmax     = iKey( world, "k_lockmax" );
	int k_membercount = iKey( world, "k_membercount" );
	int i = CountRTeams();
	int sub, nready;
	char *txt = "";
	gedict_t *p;

	if ( !isTeam() ) // no rules limitation in non team game
		return true;

    if( i < k_lockmin )
    {
		sub = k_lockmin - i;
		txt = va("%d more team%s required!\n", sub, ( sub != 1 ? "s" : "" ));

		if ( s )
        	G_sprint(s, 2, "%s", txt);
		else
        	G_bprint(2, "%s", txt);

        return false;
    }

    if( i > k_lockmax )
    {
		sub = i - k_lockmax;
		txt = va("Get rid of %d team%s!\n", sub, ( sub != 1 ? "s" : "" ));

		if ( s )
        	G_sprint(s, 2, "%s", txt);
		else
        	G_bprint(2, "%s", txt);

        return false;
    }

	nready = 0;
	for( p = world; p = find ( p, FOFCLSN, "player" ); )
		if( p->ready )
			nready++;

	if ( !CheckMembers( k_membercount ) ) {
		if( !forceMembersWarn ) // warn anyway if we want
		if( nready != k_attendees && !s )
			return false; // inform not in all cases, less annoying

		txt = va("%s %d %s\n"
				 "%s\n",
			 redtext("Server wants at least"), k_membercount, redtext("players in each team"),
			 redtext("Waiting..."));
					
		if ( s )
        	G_sprint(s, 2, "%s", txt);
		else
        	G_bprint(2, "%s", txt);

		return false;
	}

	return true;
}

// Called every second during the countdown.
void TimerStartThink ()
{
	gedict_t *p;

	k_attendees = CountPlayers();		

	if( !isCanStart( NULL, true ) ) {
		G_bprint(2, "Aborting...\n");

		StopTimer( 1 );

		return;
	}

	self->cnt2 -= 1;

	if( self->cnt2 == 1 ) {
		k_standby = 1;

		for( p = world;	p = find ( p, FOFCLSN, "player" ); ) {
			if( !strnull ( p->s.v.netname ) ) {
				//set to ghost, 1 second before matchstart
				p->s.v.takedamage = 0;
				p->s.v.solid      = 0;
				p->s.v.movetype   = 0;
				p->s.v.modelindex = 0;
				p->s.v.model      = "";
			}
		}
	}
    else if( self->cnt2 <= 0 ) {
		G_cp2all("");

		StartMatch();

		return;
	}

	PrintCountdown( self->cnt2 );

	if( self->cnt2 < 6 )
		for( p = world;	p = find (p, FOFCLSN, "player"); )
			stuffcmd (p, "play buttons/switch04.wav\n");

	self->s.v.nextthink = g_globalvars.time + 1;
}


void ShowMatchSettings()
{
	int i;
	char *txt = "";

	switch ( iKey( world, "k_spw" ) ) {
		case 0: txt = "Normal QW respawns"; break;
		case 1: txt = "KT SpawnSafety"; break;
		case 2: txt = "Kombat Teams respawns"; break;
		default: txt = "!Unknown!"; break;
	}

	G_bprint(2, "Spawnmodel: %s\n", redtext(txt));

// changed to print only if other than default

	if( i = iKey( world, "k_frp" ) ) {
		// Output the Fairpack setting here
		switch ( i ) {
			case 0: txt = "off"; break;
			case 1: txt = "best weapon"; break;
			case 2: txt = "last weapon fired"; break;
			default: txt = "!Unknown!"; break;
		}

		G_bprint(2, "Fairpacks setting: %s\n", redtext(txt));
	}

// print qizmo ( FPD ) settings
	i = iKey( world, "fpd" );
	if( i & 170 ) {
		char buf[256] = {0};

		if( i & 2 )
			strlcat(buf, " timer", sizeof(buf));
		if( i & 8 )
			strlcat(buf, " lag", sizeof(buf));
		if( i & 32 )
			strlcat(buf, " enemy", sizeof(buf));
		if( i & 128 )
			strlcat(buf, " point", sizeof(buf));

		G_bprint(2, "QiZmo:%s disabled\n", redtext(buf));
	}
}

void StartDemoRecord ()
{
	if ( iKey( world, "demo_tmp_record" ) ) { // FIXME: TODO: make this more like ktpro
		qboolean record = false;

		if ( isFFA() && iKey( world, "demo_skip_ktffa_record" ) )
			record = false;
		else
			record = true;

		if ( record )
			localcmd( "easyrecord\n" );
	}
}

void StartTimer ()
// Spawns the timer and starts the countdown.
{
	gedict_t *timer;

	k_force = 0;

	for( timer = world; timer = find(timer, FOFCLSN, "idlebot"); )
		ent_remove( timer );

	for( timer = world; timer = find(timer, FOFCLSN, "timer"); )
		ent_remove( timer );

	if ( !k_matchLess ) {
		ShowMatchSettings ();

		for( timer = world; timer = find(timer, FOFCLSN, "player"); )
			stuffcmd(timer, "play items/protect2.wav\n");
	}

	timer = spawn();
	timer->s.v.owner = EDICT_TO_PROG( world );
	timer->s.v.classname = "timer";
	timer->cnt = 0;

	if( iKey( world, "k_count" ) > 0 )
        timer->cnt2 = iKey( world, "k_count" );
    else
        timer->cnt2 = 3; // at the least we want a 3 second countdown


	if ( k_matchLess ) // check if we need countdown in case of matchless
		if ( !cvar("k_matchless_countdown") )
			timer->cnt2 = 0; // ok - no countdown

	( timer->cnt2 )++;

    timer->s.v.nextthink = g_globalvars.time + 0.001;
	timer->s.v.think = ( func_t ) TimerStartThink;

	match_in_progress = 1;

	localcmd( "serverinfo status Countdown\n" );

	StartDemoRecord (); // if allowed
}

// Whenever a countdown or match stops, remove the timer and reset everything.
// also stop/cancel demo recording
void StopTimer ( int removeDemo )
{
	gedict_t *timer, *p;

	if ( match_in_progress == 1 )
		G_cp2all(""); // clear center print

	k_force = 0;
	match_in_progress = 0;

	if ( k_standby )
	{
		// Stops the bug where players are set to ghosts 1 second to go and countdown aborts.
		// standby flag needs clearing (sturm)
		k_standby = 0;

		for( p = world; p = find ( p, FOFCLSN, "player" ); ) 
		{
			p->s.v.takedamage = 2;
			p->s.v.solid      = 3;
			p->s.v.movetype   = 3;
			setmodel (p, "progs/player.mdl");
		}
	}

	for( timer = world; timer = find(timer, FOFCLSN, "timer"); )
		ent_remove( timer );

	if ( removeDemo && !strnull( cvar_string( "serverdemo" ) ) )
		localcmd("cancel\n");  // demo is recording and must be removed, do it

	localcmd("serverinfo status Standby\n");
}

void IdlebotForceStart ()
{
    gedict_t *p;
    int i;

    G_bprint ( 2, "server is tired of waiting\n"
				  "match WILL commence!\n" );

    i = 0;
    for( p = world; p = find(p, FOFCLSN, "player"); )
    {
		if( p->ready && p->k_accepted == 2 ) {
    		i++;
		}
		else
		{
    		G_bprint(2, "%s was kicked by IDLE BOT\n", p->s.v.netname);
    		G_sprint(p, 2, "Bye bye! Pay attention next time.\n");

    		p->k_accepted = 0;
    		p->s.v.classname = "";
    		stuffcmd(p, "disconnect\n"); // FIXME: stupid way
		}
    }

    k_attendees = i;

    if( k_attendees > 1 ) {
        StartTimer();
	}
    else
    {
        G_bprint(2, "Can't start! More players needed.\n");
		EndMatch( 1 );
    }
}

void IdlebotThink ()
{
	gedict_t *p;
	int i;

	if ( iKey(world, "k_idletime") <= 0 ) {
		ent_remove( self );
		return;
	}

	self->attack_finished -= 1;

	i = CountPlayers();

	if( 0.5f * i > CountRPlayers() || i < 2 ) {
		G_bprint(2, "console: bah! chickening out?\n"
					"server disables the %s\n", redtext("idle bot"));

		ent_remove( self ) ;

		return;
	}

	k_attendees = CountPlayers();

	if ( !isCanStart(NULL, true) ) {
        G_bprint(2, "%s removed\n", redtext("idle bot"));

        ent_remove ( self );

        return;
	}

	if( self->attack_finished < 1 ) {

		IdlebotForceStart();

		ent_remove( self );

		return;

	} else {
		i = self->attack_finished;

		if( i < 5 || !(i % 5) ) {
			for( p = world; p = find ( p, FOFCLSN, "player" ); )
				if( !p->ready )
					G_sprint(p, 2, "console: %d second%s to go ready\n", i, ( i == 1 ? "" : "s" ));
		}
	}

	self->s.v.nextthink = g_globalvars.time + 1;
}

void IdlebotCheck ()
{
	gedict_t *p;
	int i;

	if ( iKey(world, "k_idletime") <= 0 ) {
		if ( p = find ( world, FOFCLSN, "idlebot" ) )
			ent_remove( p );
		return;
	}

	i = CountPlayers();

	if( 0.5f * i > CountRPlayers() || i < 2 ) {
		p = find ( world, FOFCLSN, "idlebot" );

		if( p ) {
			G_bprint(2, "console: bah! chickening out?\n"
						"server disables the %s\n", redtext("idle bot"));

			ent_remove( p );
		}

		return;
	} 

	if( match_in_progress || intermission_running || k_force )
		return;

	// no idele bot in practice mode
	if ( k_practice ) // #practice mode#
		return;

	if( p = find ( world, FOFCLSN, "idlebot" ) ) // already have idlebot
		return;

	//50% or more of the players are ready! go-go-go

	k_attendees = CountPlayers();

	if ( !isCanStart( NULL, true ) ) {
        G_sprint(self, 2, "Can't issue %s!\n", redtext("idle bot"));
		return;
	}

	p = spawn();
	p->s.v.classname = "idlebot";
	p->s.v.think = (func_t) IdlebotThink;
	p->s.v.nextthink = g_globalvars.time + 0.001;

	p->attack_finished = max( 3, iKey(world, "k_idletime") );

	G_bprint(2, "\n"
				"server activates the %s\n", redtext("idle bot"));
}

// Called by a player to inform that (s)he is ready for a match.
void PlayerReady ()
{
	gedict_t *p;
	float nready;

	if( intermission_running || match_in_progress == 2 )
			return;

	if ( k_practice ) { // #practice mode#
		G_sprint(self, 2, "%s\n", redtext("Server in practice mode"));
		return;
	}

	if( self->ready ) {
		G_sprint(self, 2, "Type break to unready yourself\n");
		return;
	}

    if( k_force && isTeam() ) {
		nready = 0;
		for( p = world; p = find ( p, FOFCLSN, "player" ); ) {
			if( p->ready ) {
				if( streq( getteam(self), getteam(p) ) && !strnull( getteam(self) ) ){
					nready = 1;
					break;
				}
			}
		}

		if( !nready ) {
			G_sprint(self, 2, "Join an existing team!\n");
			return;
		}
	}

	if ( GetHandicap(self) != 100 )
		G_sprint(self, 2, "\x87%s you are using handicap!\n", redtext( "WARNING:" ));

	self->ready = 1;
	self->v.brk = 0;
	self->k_teamnum = 0;

	G_bprint(2, "%s %s%s\n", self->s.v.netname, redtext("is ready"),
							( isTeam() ? va(" \x90%s\x91", getteam( self ) ) : "" ) );

	nready = 0;
	for( p = world; p = find ( p, FOFCLSN, "player" ); )
		if( p->ready )
			nready++;


	k_attendees = CountPlayers();

	if ( !isCanStart ( NULL, false ) )
		return; // rules does't allow us to start match, idlebot ignored because of same reason

	if ( k_force )
		return; // admin forces match - timer will started somewhere else

	if( nready != k_attendees ) { // not all players ready, check idlebot and return

		IdlebotCheck();

		return;
	}
	
	// ok all players ready

	if ( nready < 2 ) // only one or less players ready, match is pointless
		return;

	G_bprint(2, "All players ready\n"
				"Timer started\n");

	StartTimer();
}

void PlayerBreak ()
{
	int votes;
	gedict_t *p;

	if( !self->ready || intermission_running )
		return;

	if( !match_in_progress ) {
		self->ready = 0;

		G_bprint(2, "%s %s\n", self->s.v.netname, redtext("is not ready"));

		return;
	}

	if( !k_matchLess ) // u can't stop countdown (but match u can) in matchless mode
	if( match_in_progress == 1 ) {
		p = find ( world, FOFCLSN, "timer");
		if(p->cnt2 > 1) {
			self->ready = 0;

			G_bprint(2, "%s %s\n", self->s.v.netname, redtext("stops the countdown"));

			StopTimer( 1 );
		}
		return;
	}

	if( self->v.brk ) {
		self->v.brk = 0;

		G_bprint(2, "%s %s %s vote%s\n", self->s.v.netname,
				redtext("withdraws"), redtext(SexStr(self)),
				((votes = get_votes_req( OV_BREAK, true )) ? va(" (%d)", votes) : ""));

		return;
	}

	self->v.brk = 1;

	
	G_bprint(2, "%s %s%s\n", self->s.v.netname, redtext("votes for stopping the match"),
				((votes = get_votes_req( OV_BREAK, true )) ? va(" (%d)", votes) : ""));


	// blocking stop countdown in matchless mode by one player
	if ( CountPlayers() == 1 && k_matchLess && match_in_progress == 1 ) {
		G_sprint(self, 2, "You can't stop countdown alone\n");
		return;
	}

	vote_check_break ();
}

