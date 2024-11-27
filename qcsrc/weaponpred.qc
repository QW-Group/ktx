#define WEAPONINFO_INDEX        (1 << 0)
#define WEAPONINFO_AMMO_SHELLS  (1 << 1)
#define WEAPONINFO_AMMO_NAILS   (1 << 2)
#define WEAPONINFO_AMMO_ROCKETS (1 << 3)
#define WEAPONINFO_AMMO_CELLS   (1 << 4)
#define WEAPONINFO_ATTACK       (1 << 5)
#define WEAPONINFO_TIMING       (1 << 6)
#define WEAPONINFO_PRED_PING    (1 << 7)

#define	IT_SHOTGUN              1
#define	IT_SUPER_SHOTGUN        2
#define	IT_NAILGUN              4
#define	IT_SUPER_NAILGUN        8
#define	IT_GRENADE_LAUNCHER     16
#define	IT_ROCKET_LAUNCHER      32
#define	IT_LIGHTNING            64
#define	IT_AXE                  4096
#define IT_HOOK                 32768

#define PRDFL_MIDAIR    1
#define PRDFL_COILGUN   2
#define PRDFL_FORCEOFF  255

#define WEPSTATE_FRESH      1
#define WEPSTATE_PREDICTED  2

#define MASK_PRED_VIEWMODEL	256

float		ping_time;
float		chat_info;

.vector		oldorigin;
.vector		s_origin;
.float		s_time;
.float		p_time;
.float		traileffectnum;
.float		ownnum;
.float		starttime;
.float		endtime;


float		model_axe;
float		model_shotgun;
float		model_sshotgun;
float		model_nailgun;
float		model_snailgun;
float		model_grenadel;
float		model_rocketl;
float		model_light;
float		model_hook;
float		model_coilgun;

int			last_effectframe;
int			is_effectframe;

float		t_width;

.entity		_prev;
.entity		_next;
entity		predicted_projectiles;

float autocvar_cl_predict_weaponsound;
float autocvar_cl_predict_projectiles;

typedef struct weapon_state
{
	int		state;

	int		impulse;
	int		weapon;
	int		items;

	int		frame;

	int		ammo_shells;
	int		ammo_nails;
	int		ammo_rockets;
	int		ammo_cells;

	float	attack_finished;
	float	client_time;

	float	client_nextthink;
	int		client_thinkindex;

	int		client_predflags;
	float	client_ping;
};

weapon_state weapon_server;
weapon_state weapon_predicted;


void WeaponPred_Init()
{
	model_axe			= getmodelindex("progs/v_axe.mdl");
	model_shotgun		= getmodelindex("progs/v_shot.mdl");
	model_sshotgun		= getmodelindex("progs/v_shot2.mdl");
	model_nailgun		= getmodelindex("progs/v_nail.mdl");
	model_snailgun		= getmodelindex("progs/v_nail2.mdl");
	model_grenadel		= getmodelindex("progs/v_rock.mdl");
	model_rocketl		= getmodelindex("progs/v_rock2.mdl");
	model_light			= getmodelindex("progs/v_light.mdl");
	model_hook			= getmodelindex("progs/v_star.mdl");
	model_coilgun		= getmodelindex("progs/v_coil.mdl");
}




float autocvar_cl_gunx;
float autocvar_cl_guny;
float autocvar_cl_gunz;
void WeaponPred_SetModel()
{
	self.origin = [autocvar_cl_gunz, -autocvar_cl_gunx, -autocvar_cl_guny];

	float oldframe = self.frame;
	float oldmodel = self.modelindex;


	switch (weapon_predicted.weapon)
	{
		case IT_AXE:					self.modelindex = model_axe; break;
		case IT_SHOTGUN:				self.modelindex = model_shotgun; break;
		case IT_SUPER_SHOTGUN:			self.modelindex = model_sshotgun; break;
		case IT_NAILGUN:				self.modelindex = model_nailgun; break;
		case IT_SUPER_NAILGUN:			self.modelindex = model_snailgun; break;
		case IT_GRENADE_LAUNCHER:		self.modelindex = model_grenadel; break;
		case IT_ROCKET_LAUNCHER:		self.modelindex = model_rocketl; break;
		case IT_LIGHTNING:				self.modelindex = model_light; break;
	}

	self.frame = weapon_predicted.frame;



	if (oldframe != self.frame)
	{
		self.frame2 = oldframe;
		self.lerpfrac = 1;
	}

	if (oldmodel != self.modelindex)
	{
		self.lerpfrac = 0;
	}

	self.lerpfrac = max(0, self.lerpfrac - frametime * 10);
}



void PredProjectile_Cleanup(entity who)
{
	if (predicted_projectiles == who)
		predicted_projectiles = who._next;

	if (who._prev)
		who._prev._next = who._next;

	if (who._next)
		who._next._prev = who._prev;

	remove(who);
}


int Projectile_FindTrail(int is_grenade);
float Predraw_Projectile();
float PredProjectile_Predraw()
{
	if (time > self.endtime + 0.05)
	{
		PredProjectile_Cleanup(self);
		return PREDRAW_NEXT;
	}

	if (time < self.starttime || time > self.endtime)
		return PREDRAW_NEXT;

	return Predraw_Projectile();
}


float PredProjectile_MatchProjectile()
{
	self.origin = (self.s_origin + self.velocity*(time - self.s_time));

	entity proj;
	for(proj = predicted_projectiles; proj != __NULL__; proj = proj._next)
	{
		if (proj.modelindex != self.modelindex)
			continue;

		float diff = vlen(proj.origin - self.origin);
		if (diff > 32)
			continue;

		if (fabs(time - (proj.endtime - 0.05)) > 0.04)
			continue;

		self.oldorigin = proj.origin;
		PredProjectile_Cleanup(proj);

		return TRUE;
	}

	return FALSE;
}


entity WeaponPred_CreateProjectile()
{
	entity proj = spawn();

	proj.starttime = time + max(ping_time - weapon_predicted.client_ping, 0);
	proj.endtime = time + ping_time + 0.05;
	proj.s_time = proj.starttime - 0.025;


	proj.predraw = PredProjectile_Predraw;
	proj.drawmask = MASK_ENGINE;

	proj._next = predicted_projectiles;
	if (predicted_projectiles)
		predicted_projectiles._prev = proj;
	predicted_projectiles = proj;

	return proj;
}



float PM_FilterWeaponSound(string sample)
{
	if (autocvar_cl_predict_weaponsound & 2)
		if (sample == "weapons/lstart.wav")
			return FALSE;
	if (autocvar_cl_predict_weaponsound & 4)
		if (sample == "weapons/guncock.wav")
			return FALSE;
	if (autocvar_cl_predict_weaponsound & 8)
		if (sample == "weapons/shotgn2.wav")
			return FALSE;
	if (autocvar_cl_predict_weaponsound & 16)
		if (sample == "weapons/rocket1i.wav")
			return FALSE;
	if (autocvar_cl_predict_weaponsound & 32)
		if (sample == "weapons/spike2.wav")
			return FALSE;
	if (autocvar_cl_predict_weaponsound & 64)
		if (sample == "weapons/grenade.wav")
			return FALSE;
	if (autocvar_cl_predict_weaponsound & 128)
		if (sample == "weapons/sgun1.wav")
			return FALSE;
	if (autocvar_cl_predict_weaponsound & 256)
		if (sample == "weapons/lhit.wav")
			return FALSE;
	return TRUE;
}

void PM_SoundEffect_Weapon(string sample, int chan, int weap)
{
	if (!is_effectframe)
		return;

	if (autocvar_cl_predict_weaponsound == 0)
		return;

	if (autocvar_cl_predict_weaponsound & weap)
		return;

	//PM_SoundEffect(sample, chan);
	sound(self, CHAN_WEAPON, sample, 1, ATTN_NONE, 0, 0);
}


float W_ChangeWeapon(int impulse)
{
	int fl = 0;
	int am = 0;

	switch (impulse)
	{
		case 1: {
			fl = IT_AXE;
			if (weapon_predicted.items & IT_HOOK && weapon_predicted.weapon == IT_AXE)
				fl = IT_HOOK;

			am = 1;
		} break;

		case 2: {
			fl = IT_SHOTGUN;
			if (weapon_predicted.ammo_shells >= 1)
				am = 1;
		} break;

		case 3: {
			fl = IT_SUPER_SHOTGUN;
			if (weapon_predicted.ammo_shells >= 2)
				am = 1;
		} break;

		case 4: {
			fl = IT_NAILGUN;
			if (weapon_predicted.ammo_nails >= 1)
				am = 1;
		} break;

		case 5: {
			fl = IT_SUPER_NAILGUN;
			if (weapon_predicted.ammo_nails >= 2)
				am = 1;
		} break;

		case 6: {
			fl = IT_GRENADE_LAUNCHER;
			if (weapon_predicted.ammo_rockets >= 1)
				am = 1;
		} break;

		case 7: {
			fl = IT_ROCKET_LAUNCHER;
			if (weapon_predicted.ammo_rockets >= 1)
				am = 1;
		} break;

		case 8: {
			fl = IT_LIGHTNING;
			if (weapon_predicted.ammo_cells >= 1)
				am = 1;
		} break;

		case 22: {
			fl = IT_HOOK;
			am = 1;
		} break;
	}

	if (weapon_predicted.items & fl && am)
	{
		if (weapon_predicted.weapon != fl)
			weapon_predicted.frame = 0;

		weapon_predicted.weapon = fl;
		//W_SetCurrentAmmo();
	}

	return TRUE;
}


int CycleWeaponCommand()
{
	int i, it, am;

	it = weapon_predicted.items;

	for (i = 0; i < 20; i++)
	{
		am = 0;
		switch (weapon_predicted.weapon)
		{
		case IT_LIGHTNING:
			weapon_predicted.weapon = IT_AXE;
			break;

		case IT_AXE:
			weapon_predicted.weapon = IT_HOOK;
			break;

		case IT_HOOK:
			weapon_predicted.weapon = IT_SHOTGUN;
			if (weapon_predicted.ammo_shells < 1)
			{
				am = 1;
			}
			break;

		case IT_SHOTGUN:
			weapon_predicted.weapon = IT_SUPER_SHOTGUN;
			if (weapon_predicted.ammo_shells < 2)
			{
				am = 1;
			}
			break;

		case IT_SUPER_SHOTGUN:
			weapon_predicted.weapon = IT_NAILGUN;
			if (weapon_predicted.ammo_nails < 1)
			{
				am = 1;
			}
			break;

		case IT_NAILGUN:
			weapon_predicted.weapon = IT_SUPER_NAILGUN;
			if (weapon_predicted.ammo_nails < 2)
			{
				am = 1;
			}
			break;

		case IT_SUPER_NAILGUN:
			weapon_predicted.weapon = IT_GRENADE_LAUNCHER;
			if (weapon_predicted.ammo_rockets < 1)
			{
				am = 1;
			}
			break;

		case IT_GRENADE_LAUNCHER:
			weapon_predicted.weapon = IT_ROCKET_LAUNCHER;
			if (weapon_predicted.ammo_rockets < 1)
			{
				am = 1;
			}
			break;

		case IT_ROCKET_LAUNCHER:
			weapon_predicted.weapon = IT_LIGHTNING;
			if (weapon_predicted.ammo_cells < 1)
			{
				am = 1;
			}
			break;
		}

		if ((it & weapon_predicted.weapon) && (am == 0))
		{
			return TRUE;
		}
	}

	return TRUE;
}


int CycleWeaponReverseCommand()
{
	int i, it, am;

	it = weapon_predicted.items;

	for (i = 0; i < 20; i++)
	{
		am = 0;
		switch (weapon_predicted.weapon)
		{
		case IT_LIGHTNING:
			weapon_predicted.weapon = IT_ROCKET_LAUNCHER;
			if (weapon_predicted.ammo_rockets < 1)
			{
				am = 1;
			}

			break;

		case IT_ROCKET_LAUNCHER:
			weapon_predicted.weapon = IT_GRENADE_LAUNCHER;
			if (weapon_predicted.ammo_rockets < 1)
			{
				am = 1;
			}

			break;

		case IT_GRENADE_LAUNCHER:
			weapon_predicted.weapon = IT_SUPER_NAILGUN;
			if (weapon_predicted.ammo_nails < 2)
			{
				am = 1;
			}

			break;

		case IT_SUPER_NAILGUN:
			weapon_predicted.weapon = IT_NAILGUN;
			if (weapon_predicted.ammo_nails < 1)
			{
				am = 1;
			}

			break;

		case IT_NAILGUN:
			weapon_predicted.weapon = IT_SUPER_SHOTGUN;
			if (weapon_predicted.ammo_shells < 2)
			{
				am = 1;
			}

			break;

		case IT_SUPER_SHOTGUN:
			weapon_predicted.weapon = IT_SHOTGUN;
			if (weapon_predicted.ammo_shells < 1)
			{
				am = 1;
			}

			break;

		case IT_SHOTGUN:
			weapon_predicted.weapon = IT_HOOK;
			break;

		case IT_HOOK:
			weapon_predicted.weapon = IT_AXE;
			break;

		case IT_AXE:
			weapon_predicted.weapon = IT_LIGHTNING;
			if (weapon_predicted.ammo_cells < 1)
			{
				am = 1;
			}

			break;
		}

		if ((it & weapon_predicted.weapon) && (am == 0))
		{
			return TRUE;
		}
	}

	return TRUE;
}


void player_run(void)
{
	weapon_predicted.client_nextthink = 0;
	weapon_predicted.client_thinkindex = 0;
	weapon_predicted.frame = 0;
}


void anim_axe(void)
{
	weapon_predicted.client_nextthink = weapon_predicted.client_time + 0.1;
	weapon_predicted.frame++;
	weapon_predicted.client_thinkindex++;
	if (weapon_predicted.client_thinkindex > 4)
	{
		weapon_predicted.client_thinkindex = 0;
	}
	else if (weapon_predicted.client_thinkindex == 4)
	{
		//W_FireAxe();
	}
}


void anim_shotgun(void)
{
	weapon_predicted.client_nextthink = weapon_predicted.client_time + 0.1;
	weapon_predicted.frame = weapon_predicted.client_thinkindex;
	weapon_predicted.client_thinkindex++;
	if (weapon_predicted.client_thinkindex > 6)
	{
		weapon_predicted.client_thinkindex = 0;
	}
}


void launch_spike(float off)
{
	if (weapon_predicted.weapon == IT_SUPER_NAILGUN)
	{
		//newmis = PM_AddEvent_FakeProj(IT_SUPER_NAILGUN);
		PM_SoundEffect_Weapon("weapons/spike2.wav", 1, 32);
		off = 0;
	}
	else
	{
		//newmis = PM_AddEvent_FakeProj(IT_NAILGUN);
		PM_SoundEffect_Weapon("weapons/rocket1i.wav", 1, 16);
	}



	if (is_effectframe && autocvar_cl_predict_projectiles && weapon_predicted.client_ping)
	{
		entity proj = WeaponPred_CreateProjectile();
		if (weapon_predicted.weapon == IT_SUPER_NAILGUN)
			setmodel(proj, "progs/s_spike.mdl");
		else
			setmodel(proj, "progs/spike.mdl");
		proj.s_origin = pmove_org;
		proj.s_origin += v_forward * 8;
		proj.s_origin += v_right * off;
		proj.s_origin[2] += 16;
		proj.oldorigin = proj.s_origin + (0.05 * proj.velocity);
		proj.origin = proj.oldorigin;


		proj.velocity = 1000 * v_forward;

		proj.angles = input_angles; proj.angles[0] *= -1;
	}
}


void player_nail1(void)
{
	weapon_predicted.client_nextthink = weapon_predicted.client_time + 0.1;
	weapon_predicted.client_thinkindex = 2;

	if (!(input_buttons & 1) || weapon_predicted.impulse)
	{
		player_run();
		return;
	}

	weapon_predicted.frame = weapon_predicted.frame + 1;
	if (weapon_predicted.frame >= 9)
	{
		weapon_predicted.frame = 1;
	}

	weapon_predicted.ammo_nails -= 1;
	weapon_predicted.attack_finished = weapon_predicted.client_time + 0.2;
	launch_spike(4);
}


void player_nail2(void)
{
	weapon_predicted.client_nextthink = weapon_predicted.client_time + 0.1;
	weapon_predicted.client_thinkindex = 1;

	if (!(input_buttons & 1) || weapon_predicted.impulse)
	{
		player_run();
		return;
	}

	weapon_predicted.frame = weapon_predicted.frame + 1;
	if (weapon_predicted.frame >= 9)
	{
		weapon_predicted.frame = 1;
	}

	weapon_predicted.ammo_nails -= 1;
	weapon_predicted.attack_finished = weapon_predicted.client_time + 0.2;
	launch_spike(-4);
}

void anim_nailgun(void)
{
	if (weapon_predicted.client_thinkindex < 2)
		player_nail1();
	else
		player_nail2();
}


void player_light1(void)
{
	weapon_predicted.client_nextthink = weapon_predicted.client_time + 0.1;
	weapon_predicted.client_thinkindex = 2;

	if (!(input_buttons & 1) || weapon_predicted.impulse)
	{
		player_run();
		return;
	}

	weapon_predicted.frame = weapon_predicted.frame + 1;
	if (weapon_predicted.frame >= 5)
	{
		weapon_predicted.frame = 1;
	}

	weapon_predicted.attack_finished = weapon_predicted.client_time + 0.2;

	weapon_predicted.ammo_cells -= 1;
	/*
	if (pmove.waterlevel <= 1)
	{
		prediction_event_fakeproj_t *beam = PM_AddEvent_FakeProj(IT_LIGHTNING);
		VectorCopy(pmove.origin, beam->origin);
		beam->origin[2] += 16;

		VectorCopy(pmove.cmd.angles, beam->angles);
	}
	*/
}

void player_light2(void)
{
	weapon_predicted.client_nextthink = weapon_predicted.client_time + 0.1;
	weapon_predicted.client_thinkindex = 1;

	if (!(input_buttons & 1) || weapon_predicted.impulse)
	{
		player_run();
		return;
	}

	weapon_predicted.frame = weapon_predicted.frame + 1;
	if (weapon_predicted.frame >= 5)
	{
		weapon_predicted.frame = 1;
	}

	weapon_predicted.attack_finished = weapon_predicted.client_time + 0.2;

	weapon_predicted.ammo_cells -= 1;
	/*
	if (pmove.waterlevel <= 1)
	{
		prediction_event_fakeproj_t *beam = PM_AddEvent_FakeProj(IT_LIGHTNING);
		VectorCopy(pmove.origin, beam->origin);
		beam->origin[2] += 16;

		VectorCopy(pmove.cmd.angles, beam->angles);
	}
	*/
}

void anim_lightning(void)
{
	if (weapon_predicted.client_thinkindex < 2)
		player_light1();
	else
		player_light2();


	if (is_effectframe)
	{
		if (weapon_predicted.client_time >= t_width)
		{
			t_width = weapon_predicted.client_time + (0.6);
			PM_SoundEffect_Weapon("weapons/lhit.wav", 1, 256);
			//if !((autocvar_cl_predict_weaponsound == 0) || (autocvar_cl_predict_weaponsound & 256))
				//PM_SoundEffect_Weapon(cl.playernum + 1, 1, cl_sfx_lghit, pmove.origin, 1, 0);
		}
	}
}

void anim_rocket(void)
{
	weapon_predicted.client_nextthink = weapon_predicted.client_time + 0.1;
	weapon_predicted.frame = weapon_predicted.client_thinkindex;
	weapon_predicted.client_thinkindex++;
	if (weapon_predicted.client_thinkindex > 6)
	{
		weapon_predicted.client_thinkindex = 0;
	}
}


int W_CheckNoAmmo()
{
	if ((weapon_predicted.weapon == IT_AXE) || (weapon_predicted.weapon == IT_HOOK))
		return TRUE;
	else if (weapon_predicted.weapon == IT_SHOTGUN && weapon_predicted.ammo_shells >= 1)
		return TRUE;
	else if (weapon_predicted.weapon == IT_SUPER_SHOTGUN && weapon_predicted.ammo_shells >= 2)
		return TRUE;
	else if (weapon_predicted.weapon == IT_NAILGUN && weapon_predicted.ammo_nails >= 1)
		return TRUE;
	else if (weapon_predicted.weapon == IT_SUPER_NAILGUN && weapon_predicted.ammo_nails >= 2)
		return TRUE;
	else if (weapon_predicted.weapon == IT_GRENADE_LAUNCHER && weapon_predicted.ammo_rockets >= 1)
		return TRUE;
	else if (weapon_predicted.weapon == IT_ROCKET_LAUNCHER && weapon_predicted.ammo_rockets >= 1)
		return TRUE;
	else if (weapon_predicted.weapon == IT_LIGHTNING && weapon_predicted.ammo_cells >= 1)
		return TRUE;

	//weapon_predicted.weapon = W_BestWeapon();

	//	drop the weapon down
	return FALSE;
}


void W_Attack(void)
{
	if (!W_CheckNoAmmo())
	{
		return;
	}

	switch (weapon_predicted.weapon)
	{
		case IT_AXE: {
			weapon_predicted.attack_finished = weapon_predicted.client_time + 0.5;
			//PM_SoundEffect_Weapon(cl_sfx_ax1, 1, 2);

			float r = fabs((((int)(weapon_predicted.client_time * 931.75) << 11) + ((int)(weapon_predicted.client_time) >> 6)) % 1000) / 1000;
			if (r < 0.25)
			{
				weapon_predicted.frame = 0;
			}
			else if (r < 0.5)
			{
				weapon_predicted.frame = 4;
			}
			else if (r < 0.75)
			{
				weapon_predicted.frame = 0;
			}
			else
			{
				weapon_predicted.frame = 4;
			}
			weapon_predicted.client_thinkindex = 1;
			anim_axe();
		} break;
		case IT_SHOTGUN: {
			if (weapon_predicted.client_predflags & PRDFL_COILGUN)
			{
				if (weapon_predicted.client_predflags & PRDFL_MIDAIR)
					weapon_predicted.attack_finished = weapon_predicted.client_time + 0.7;
				else
					weapon_predicted.attack_finished = weapon_predicted.client_time + 0.5;
				//PM_SoundEffect_Weapon(cl_sfx_coil, 1, 4);
			}
			else
			{
				weapon_predicted.ammo_shells -= 1;
				weapon_predicted.attack_finished = weapon_predicted.client_time + 0.5;
				PM_SoundEffect_Weapon("weapons/guncock.wav", 1, 4);
			}

			weapon_predicted.client_thinkindex = 1;
			anim_shotgun();
		} break;
		case IT_SUPER_SHOTGUN: {
			weapon_predicted.attack_finished = weapon_predicted.client_time + 0.7;
			PM_SoundEffect_Weapon("weapons/shotgn2.wav", 1, 8);
			weapon_predicted.ammo_shells -= 1;
			weapon_predicted.client_thinkindex = 1;
			anim_shotgun();
		} break;
		case IT_NAILGUN: {
			anim_nailgun();
		} break;
		case IT_SUPER_NAILGUN: {
			anim_nailgun();
		} break;
		case IT_GRENADE_LAUNCHER: {
			weapon_predicted.attack_finished = weapon_predicted.client_time + 0.6;
			weapon_predicted.ammo_rockets -= 1;
			PM_SoundEffect_Weapon("weapons/grenade.wav", 1, 64);
			/*
			prediction_event_fakeproj_t *newmis = PM_AddEvent_FakeProj(IT_GRENADE_LAUNCHER);
			vec3_t forward, right, up;
			AngleVectors(pmove.cmd.angles, forward, right, up);

			newmis->velocity[0] = forward[0] * 600 + up[0] * 200;
			newmis->velocity[1] = forward[1] * 600 + up[1] * 200;
			newmis->velocity[2] = forward[2] * 600 + up[2] * 200;

			VectorCopy(pmove.origin, newmis->origin);

			vectoangles(newmis->velocity, newmis->angles);
			VectorSet(newmis->avelocity, 300, 300, 300);
			*/
			weapon_predicted.client_thinkindex = 1;
			anim_rocket();
		} break;
		case IT_ROCKET_LAUNCHER: {
			weapon_predicted.attack_finished = weapon_predicted.client_time + 0.8;
			weapon_predicted.ammo_rockets -= 1;
			PM_SoundEffect_Weapon("weapons/sgun1.wav", 1, 128);
			/*

			prediction_event_fakeproj_t *newmis = PM_AddEvent_FakeProj(IT_ROCKET_LAUNCHER);
			vec3_t forward;
			AngleVectors(pmove.cmd.angles, forward, NULL, NULL);

			if (pmove.client_predflags & PRDFL_MIDAIR)
			{
				VectorScale(forward, 2000, newmis->velocity);
			}
			else
			{
				VectorScale(forward, 1000, newmis->velocity);
			}

			VectorCopy(pmove.origin, newmis->origin);
			newmis->origin[0] += forward[0] * 8;
			newmis->origin[1] += forward[1] * 8;
			newmis->origin[2] += 16 + forward[2] * 8;

			VectorCopy(pmove.cmd.angles, newmis->angles);
			newmis->angles[0] = -newmis->angles[0];
			*/

			if (is_effectframe && autocvar_cl_predict_projectiles && weapon_predicted.client_ping)
			{
				entity proj = WeaponPred_CreateProjectile();
				setmodel(proj, "progs/missile.mdl");
				proj.traileffectnum = Projectile_FindTrail(FALSE);
				proj.s_origin = pmove_org;
				proj.s_origin += v_forward * 8;
				proj.s_origin[2] += 16;
				proj.velocity = 1000 * v_forward;
				proj.oldorigin = proj.s_origin + (0.05 * proj.velocity);
				proj.origin = proj.oldorigin;

				proj.angles = input_angles; proj.angles[0] *= -1;
			}


			weapon_predicted.client_thinkindex = 1;
			anim_rocket();
		} break;
		case IT_LIGHTNING: {
			//PM_SoundEffect_Weapon(cl_sfx_lg, 0, 256);
			anim_lightning();
		} break;
		case IT_HOOK: {
			//PM_SoundEffect(cl_sfx_hook, 1);
			weapon_predicted.attack_finished = weapon_predicted.client_time + 0.1;
		} break;
	}
}


void execute_clientthink(void)
{
	if (weapon_predicted.client_thinkindex == 0)
	{
		player_run();
		return;
	}

	switch (weapon_predicted.weapon)
	{
	case IT_AXE: {
		anim_axe();
	} break;
	case IT_SHOTGUN: {
		anim_shotgun();
	} break;
	case IT_SUPER_SHOTGUN: {
		anim_shotgun();
	} break;
	case IT_NAILGUN: {
		anim_nailgun();
	} break;
	case IT_SUPER_NAILGUN: {
		anim_nailgun();
	} break;
	case IT_GRENADE_LAUNCHER: {
		anim_rocket();
	} break;
	case IT_ROCKET_LAUNCHER: {
		anim_rocket();
	} break;
	case IT_LIGHTNING: {
		anim_lightning();
	} break;
	case IT_HOOK: {
		anim_shotgun();
	} break;
	}
}


void WeaponPred_ImpulseCommands()
{
	if !(weapon_predicted.impulse)
		return;

	int clear = FALSE;
	if (((weapon_predicted.impulse >= 1) && (weapon_predicted.impulse <= 8)) || (weapon_predicted.impulse == 22))
		clear = W_ChangeWeapon(weapon_predicted.impulse);
	else if (weapon_predicted.impulse == 10)
	{
		clear = CycleWeaponCommand();
	}
	else if (weapon_predicted.impulse == 12)
	{
		clear = CycleWeaponReverseCommand();
	}

	if (clear)
		weapon_predicted.impulse = 0;
}



float WeaponPred_Think()
{
	weapon_server.items = getstatbits(STAT_ITEMS, 0, 23);
	weapon_predicted = weapon_server;


	int pframe = servercommandframe + 1;
	int eframe = clientcommandframe;
	for(; pframe <= eframe; pframe++)
	{
		int success = getinputstate(pframe);
		if (!success || input_timelength <= 0)
			break;

		makevectors(input_angles);

		if (chat_info)
			input_buttons = 0;

		if (pframe == eframe-1 && pframe > last_effectframe)
		{
			is_effectframe = TRUE;
			last_effectframe = pframe;
		}
		else
			is_effectframe = FALSE;

		weapon_predicted.client_time += input_timelength;

		if (input_impulse)
			weapon_predicted.impulse = input_impulse;


		if (weapon_predicted.client_nextthink && weapon_predicted.client_time >= weapon_predicted.client_nextthink)
		{
			float held_client_time = weapon_predicted.client_time;

			weapon_predicted.client_time = weapon_predicted.client_nextthink;
			weapon_predicted.client_nextthink = 0;
			execute_clientthink();
			weapon_predicted.client_time = held_client_time;
		}


		if (weapon_predicted.client_time >= weapon_predicted.attack_finished)
		{
			WeaponPred_ImpulseCommands();

			if (input_buttons & 1)
				W_Attack();
		}
	}


	WeaponPred_SetModel();

	return PREDRAW_AUTOADD;
}





void EntUpdate_WeaponPred(float is_new)
{
	float sendflags = readbyte();

	weapon_server.state = WEPSTATE_FRESH;

	if (sendflags & WEAPONINFO_INDEX)
	{
		weapon_server.impulse = readbyte();
		weapon_server.weapon = readbyte();
	}
	if (sendflags & WEAPONINFO_AMMO_SHELLS)
		weapon_server.ammo_shells = readbyte();
	if (sendflags & WEAPONINFO_AMMO_NAILS)
		weapon_server.ammo_nails = readbyte();
	if (sendflags & WEAPONINFO_AMMO_ROCKETS)
		weapon_server.ammo_rockets = readbyte();
	if (sendflags & WEAPONINFO_AMMO_CELLS)
		weapon_server.ammo_cells = readbyte();


	if (sendflags & WEAPONINFO_ATTACK)
	{
		weapon_server.attack_finished = readfloat();
		weapon_server.client_nextthink = readfloat();
		weapon_server.client_thinkindex = readbyte();
	}

	if (sendflags & WEAPONINFO_TIMING)
	{
		weapon_server.client_time = readfloat();
		weapon_server.frame = readbyte();
	}

	if (sendflags & WEAPONINFO_PRED_PING)
	{
		weapon_server.client_predflags = readbyte();
		weapon_server.client_ping = readbyte() / 1000;
	}

	if (is_new)
	{
		self.drawmask = MASK_PRED_VIEWMODEL;
		self.predraw = WeaponPred_Think;
		self.renderflags = RF_VIEWMODEL;
	}
}
















