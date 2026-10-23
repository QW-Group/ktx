/*
 * projectiles.qc - server-authoritative CSQC projectile rendering plus
 * client-predicted local projectiles with grenade bounce physics.
 *
 * Ported from unezQuake EZCSQC (Predraw_Projectile, WeaponPred_SpawnProjectile,
 * CL_EZCSQC_ProjectileBounce et al). Local projectiles are a purely visual
 * bridge until the authoritative server projectile arrives; hits/damage remain
 * server-side.
 */

entity  predicted_projectiles;      // singly/doubly linked list head

//============================================================================
// Trail selection
//============================================================================

int Projectile_FindTrail(int is_grenade)
{
	string cvar_to_use;
	int select;
	int trail_index = 0;

	if (is_grenade)
		cvar_to_use = autocvar_r_grenadetrail;
	else
		cvar_to_use = autocvar_r_rockettrail;
	select = stoi(cvar_to_use);

	switch (select)
	{
	case 0:
		if (cvar_to_use == "0")
		{
			trail_index = 0;
			break;
		}
		else
		{
			int effect = particleeffectnum(cvar_to_use);
			if (effect)
			{
				trail_index = effect;
				break;
			}
		}
		// fall through
	case 1:
	default:
		if (is_grenade)
			trail_index = particleeffectnum("TR_GRENADE");
		else
			trail_index = particleeffectnum("TR_ROCKET");
		break;
	case 2:
		if (is_grenade)
			trail_index = particleeffectnum("TR_ROCKET");
		else
			trail_index = particleeffectnum("TR_GRENADE");
		break;
	case 3: trail_index = particleeffectnum("TR_ALTROCKET"); break;
	case 4: trail_index = particleeffectnum("TR_BLOOD"); break;
	case 5: trail_index = particleeffectnum("TR_SLIGHTBLOOD"); break;
	case 6: trail_index = particleeffectnum("TR_WIZSPIKE"); break;
	case 7: trail_index = particleeffectnum("TR_KNIGHTSPIKE"); break;
	case 8: trail_index = particleeffectnum("TR_VORESPIKE"); break;
	case 9: trail_index = particleeffectnum("TE_RAILTRAIL"); break;
	}

	return trail_index;
}

//============================================================================
// Bounce physics (shared by local and server grenades)
//============================================================================

float Projectile_Gravity()
{
	if (autocvar_csqc_gravity > 0)
		return autocvar_csqc_gravity;
	return 800;
}

vector Projectile_ClipVelocity(vector vel, vector norm, float f)
{
	vector vel2 = norm * (vel * norm) * f;
	vel = vel - vel2;

	if (vel_x > -0.1 && vel_x < 0.1) vel_x = 0;
	if (vel_y > -0.1 && vel_y < 0.1) vel_y = 0;
	if (vel_z > -0.1 && vel_z < 0.1) vel_z = 0;
	return vel;
}

void Projectile_AdvanceAngles(entity proj, float dt)
{
	if (proj.projectile_type == 1)
	{
		proj.sim_angles = proj.sim_angles + proj.avel * dt;
		proj.angles = proj.sim_angles;
	}
	else
	{
		proj.angles = proj.angles + proj.avel * dt;
	}
}

void Projectile_Bounce(entity proj, float dt, float update_angles)
{
	float gravity_value = Projectile_Gravity();
	float movetime, bump;

	// A grounded grenade stays still until given upward velocity again.
	if (proj.grounded && proj.vel_z >= 1.0 / 32.0)
		proj.grounded = FALSE;
	else if (proj.grounded)
		return;

	proj.vel_z = proj.vel_z - 0.5 * dt * gravity_value;
	if (update_angles)
		Projectile_AdvanceAngles(proj, dt);

	movetime = dt;
	for (bump = 0; bump < 5 && movetime > 0; bump++)
	{
		vector move = proj.vel * movetime;
		vector to = proj.origin + move;
		traceline(proj.origin, to, MOVE_NOMONSTERS, proj);
		proj.origin = trace_endpos;

		if (trace_fraction == 1)
			break;

		movetime = movetime * (1 - min(1, trace_fraction));
		float bouncestp = (60.0 / 800.0) * gravity_value;

		proj.vel = Projectile_ClipVelocity(proj.vel, trace_plane_normal, 1 + 0.5);
		float d = trace_plane_normal * proj.vel;
		if (trace_plane_normal_z > 0.7 && d < bouncestp && d > -bouncestp)
		{
			proj.grounded = TRUE;
			proj.vel = '0 0 0';
			proj.avel = '0 0 0';
		}
		else
		{
			proj.grounded = FALSE;
		}
	}

	if (!proj.grounded)
		proj.vel_z = proj.vel_z - 0.5 * dt * gravity_value;
}

// True if a straight segment passes through solid world (keeps predicted
// projectiles from visibly drawing through nearby walls).
float Projectile_TraceBlocked(vector start, vector end)
{
	if (pointcontents(start) == CONTENT_SOLID)
		return TRUE;
	if (pointcontents(end) == CONTENT_SOLID)
		return TRUE;

	traceline(start, end, MOVE_NOMONSTERS, self);
	if (trace_fraction < 1)
		return TRUE;

	// A few interior samples catch thin geometry the endpoints miss.
	float i;
	for (i = 1; i < 4; i++)
	{
		vector p = start + (end - start) * (i / 4);
		if (pointcontents(p) == CONTENT_SOLID)
			return TRUE;
	}
	return FALSE;
}

//============================================================================
// Predicted local projectiles
//============================================================================

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

float PredProjectile_Predraw()
{
	if (!autocvar_cl_predict_projectiles)
		return PREDRAW_NEXT;

	if (time < self.starttime)
		return PREDRAW_NEXT;
	if (time >= self.endtime)
	{
		PredProjectile_Cleanup(self);
		return PREDRAW_NEXT;
	}

	if (self.projectile_type == 1)
	{
		float dt = max(time - self.simtime, 0);
		self.simtime = time;
		if (dt > 0)
			Projectile_Bounce(self, dt, TRUE);
	}
	else
	{
		vector trace_start;
		self.origin = self.start + self.vel * (time - self.starttime);
		vector trace_end = self.origin + self.vel * 0.02;
		if (self.has_last_draw_origin)
			trace_start = self.last_draw_origin;
		else
			trace_start = self.start;

		// Phase 6 hooks owner-rocket world-impact explosion prediction here.
		if (Predict_OwnerRocketWorldImpact(self, trace_start, trace_end) ||
			Projectile_TraceBlocked(self.start, trace_end))
		{
			PredProjectile_Cleanup(self);
			return PREDRAW_NEXT;
		}
	}

	setorigin(self, self.origin);
	self.last_draw_origin = self.origin;
	self.has_last_draw_origin = TRUE;

	// Trail, spaced out so high fps does not overdraw.
	if (self.traileffectnum && time > self.p_time)
	{
		trailparticles(self.traileffectnum, self, self.oldorigin, self.origin);
		self.p_time = time + 0.013;
		self.oldorigin = self.origin;
	}

	if (self.modelflags & MF_ROCKET)
		dynamiclight_add(self.origin, 200, autocvar_r_rocketlight_colour, 0);

	return PREDRAW_AUTOADD;
}

entity WeaponPred_CreateProjectile()
{
	entity proj = spawn();

	proj.drawmask = MASK_ENGINE;
	proj.predraw = PredProjectile_Predraw;
	proj.local_projectile = TRUE;

	proj._next = predicted_projectiles;
	proj._prev = __NULL__;
	if (predicted_projectiles)
		predicted_projectiles._prev = proj;
	predicted_projectiles = proj;

	return proj;
}

float WeaponPred_SpawnProjectile(int windex, int animindex)
{
	int modelindex = wpredict_definitions[windex].anims[animindex].projectile_model;
	if (!autocvar_cl_predict_projectiles)
		return TRUE;
	if (!modelindex)
		return FALSE;

	entity proj = WeaponPred_CreateProjectile();
	setmodel(proj, modelnameforindex(modelindex));

	// Lifetime: predict until the authoritative server projectile should arrive.
	proj.starttime = time;
	proj.endtime = time + max(ping_time + 0.013, 0.05);
	proj.ownnum = player_localentnum;
	proj.projectile_type = (proj.modelflags & MF_GRENADE) ? 1 : 0;

	// Weapon-local spawn: makevectors(input_angles) already ran this frame.
	vector off = wpredict_definitions[windex].anims[animindex].projectile_offset;
	vector pvel = wpredict_definitions[windex].anims[animindex].projectile_velocity;

	vector sorg = pmove_org;
	sorg += v_right * off_x;
	sorg += v_forward * off_y;
	sorg_z += off_z;

	vector svel = v_right * pvel_x + v_forward * pvel_y + v_up * pvel_z;

	// Point-blank rocket impacts explode locally instead of spawning a model.
	if (Predict_RocketSpawnTouch(proj, sorg, svel))
	{
		PredProjectile_Cleanup(proj);
		return TRUE;
	}

	proj.angles = vectoangles(svel);
	proj.origin = sorg;
	proj.start = sorg;
	proj.vel = svel;
	proj.oldorigin = sorg;
	proj.avel = '0 0 0';

	if (proj.projectile_type == 1)
	{
		// Match the fixed native newmis phase (0.05s of flight at spawn).
		proj.prediction_start_state_time = weapon_predicted.client_time - 0.05;
		proj.avel = '300 300 300';
		proj.sim_angles = proj.angles;
		proj.simtime = time;
		Projectile_Bounce(proj, 0.05, TRUE);
	}
	else
	{
		float speed = vlen(svel);
		if (speed > 1)
		{
			proj.prediction_start_state_time = weapon_predicted.client_time - 0.05;
			proj.starttime -= 0.05;
			proj.origin = sorg + svel * 0.05;
			proj.oldorigin = proj.origin;
		}
		else
		{
			proj.prediction_start_state_time = weapon_predicted.client_time;
		}
		proj.traileffectnum = Projectile_FindTrail(FALSE);
	}

	return TRUE;
}

//============================================================================
// Server-authoritative projectile rendering
//============================================================================

void Projectile_SetGrenadeServerState(float reset_angles)
{
	// Position/velocity are authoritative; angle phase is client-owned unless reset.
	self.origin = self.s_origin;
	self.sim_origin = self.s_origin;
	if (reset_angles)
	{
		self.sim_angles = self.angles;
		self.angle_simtime = time;
	}
	else if (!self.angle_simtime)
	{
		self.angle_simtime = time;
	}
	self.vel = self.s_velocity;
	if (vlen(self.vel) < 1)
	{
		self.vel = '0 0 0';
		self.avel = '0 0 0';
		self.grounded = TRUE;
	}
	else
	{
		self.avel = '300 300 300';
		self.grounded = FALSE;
	}
	self.simtime = self.s_time;
	self.projectile_type = 1;
}

void Projectile_ApplyHandoffSmoothing(vector authoritative_origin)
{
	if (self.handoff_smoothing)
	{
		if (time < self.handoff_endtime)
		{
			float frac = 1;
			if (self.handoff_endtime > self.handoff_starttime)
				frac = (time - self.handoff_starttime) / (self.handoff_endtime - self.handoff_starttime);
			frac = bound(0, frac, 1);
			frac = frac * frac * (3 - 2 * frac);
			self.origin = authoritative_origin - self.handoff_delta * (1 - frac);
		}
		else
		{
			self.handoff_smoothing = FALSE;
			self.origin = authoritative_origin;
		}
	}
	else
	{
		self.origin = authoritative_origin;
	}
}

void Projectile_BeginHandoff(vector smooth_origin, vector server_origin, float basis, float local_speed, float has_prior)
{
	self.handoff_smoothing = TRUE;
	self.handoff_starttime = time;
	if (has_prior)
		self.handoff_endtime = time + bound(0.150, basis / max(local_speed, 1) * 6.0, 0.300);
	else
		self.handoff_endtime = time + bound(0.026, basis / max(local_speed, 1) * 0.5, 0.050);
	self.origin = smooth_origin;
	self.handoff_delta = server_origin - smooth_origin;
	self.oldorigin = smooth_origin;
}

float Predraw_Projectile()
{
	vector previous_origin = self.oldorigin;
	vector authoritative_origin;

	if (self.projectile_type == 1)
	{
		// Server grenades receive sparse corrections then bounce locally between them.
		self.origin = self.sim_origin;
		float dt = max(servertime - self.simtime, 0);
		self.simtime = servertime;
		if (dt > 0)
			Projectile_Bounce(self, dt, FALSE);
		authoritative_origin = self.origin;
		self.sim_origin = authoritative_origin;

		// Spin advances on render time so packet jitter does not tumble grenades.
		dt = max(time - self.angle_simtime, 0);
		self.angle_simtime = time;
		if (dt > 0)
			Projectile_AdvanceAngles(self, min(dt, 0.050));

		Projectile_ApplyHandoffSmoothing(authoritative_origin);
	}
	else
	{
		float delta = servertime - self.s_time;
		if (autocvar_cl_sproj_xerp && self.ownnum != player_localentnum && self.ownnum != 0)
			delta += ping_time;

		authoritative_origin = self.s_origin + self.s_velocity * delta;
		Projectile_ApplyHandoffSmoothing(authoritative_origin);

		vector trace_end = self.origin + self.s_velocity * 0.02;
		if (Predict_OwnerRocketWorldImpact(self, previous_origin, trace_end) ||
			Projectile_TraceBlocked(previous_origin, trace_end))
		{
			// Can't remove an engine-owned networked entity; hide it until the
			// server's own remove message arrives.
			self.drawmask = 0;
			return PREDRAW_NEXT;
		}
	}

	setorigin(self, self.origin);

	// Trail, spaced out so high fps does not overdraw.
	if (self.traileffectnum && time > self.p_time)
	{
		trailparticles(self.traileffectnum, self, self.oldorigin, self.origin);
		self.p_time = time + 0.013;
		self.oldorigin = self.origin;
	}

	if (self.modelflags & MF_ROCKET)
		dynamiclight_add(self.origin, 200, autocvar_r_rocketlight_colour, 0);

	return PREDRAW_AUTOADD;
}

// When the authoritative projectile arrives, adopt a matching local prediction:
// the (engine-owned) server entity inherits the local visual phase and smooths
// into authority. self is the newly-arrived server projectile.
float PredProjectile_MatchProjectile()
{
	float is_gren = (self.projectile_type == 1);
	vector server_origin;

	if (is_gren)
	{
		entity tmp = spawn();
		tmp.origin = self.s_origin;
		tmp.sim_origin = self.s_origin;
		tmp.vel = self.s_velocity;
		tmp.projectile_type = 1;
		if (vlen(self.s_velocity) < 1)
		{
			tmp.vel = '0 0 0';
			tmp.avel = '0 0 0';
			tmp.grounded = TRUE;
		}
		else
		{
			tmp.avel = '300 300 300';
			tmp.grounded = FALSE;
		}
		float gdt = max(servertime - self.s_time, 0);
		if (gdt > 0)
			Projectile_Bounce(tmp, gdt, FALSE);
		server_origin = tmp.origin;
		remove(tmp);
	}
	else
	{
		server_origin = self.s_origin + self.s_velocity * (servertime - self.s_time);
	}

	float owner_is_me = (self.ownnum == player_localnum + 1);

	entity proj = predicted_projectiles;
	while (proj)
	{
		entity nextp = proj._next;

		if (!proj.local_projectile || proj.ownnum != self.ownnum)
		{
			proj = nextp;
			continue;
		}
		if (proj.modelindex && self.modelindex && proj.modelindex != self.modelindex)
		{
			proj = nextp;
			continue;
		}
		if (time > proj.endtime + 0.1)
		{
			proj = nextp;
			continue;
		}

		vector local_origin;
		if (proj.projectile_type == 1)
			local_origin = proj.origin;
		else
			local_origin = proj.start + proj.vel * (time - proj.starttime);

		vector diff = server_origin - local_origin;
		float local_speed = vlen(proj.vel);
		float parallel = 0;
		vector diff_parallel = '0 0 0';
		if (local_speed > 1)
		{
			vector dir = proj.vel * (1 / local_speed);
			parallel = diff * dir;
			diff_parallel = dir * parallel;
		}
		float perpendicular = vlen(diff - diff_parallel);

		if (vlen(diff) > 64)
		{
			// Larger gaps only accepted for nearly-collinear owner rockets/nails.
			if (!owner_is_me || proj.projectile_type == 1 || perpendicular >= 8 || fabs(parallel) > 128)
			{
				proj = nextp;
				continue;
			}
		}

		if (owner_is_me)
		{
			vector smooth_origin = proj.has_last_draw_origin ? proj.last_draw_origin : local_origin;

			if (is_gren)
			{
				// Owner grenade: adopt authority but keep the local spin phase.
				Projectile_SetGrenadeServerState(FALSE);
				self.angles = proj.sim_angles;
				self.sim_angles = proj.sim_angles;
				self.angle_simtime = time;
				Projectile_BeginHandoff(smooth_origin, server_origin, vlen(server_origin - smooth_origin), local_speed, proj.has_last_draw_origin);
				PredProjectile_Cleanup(proj);
				return TRUE;
			}
			else if (perpendicular < 8)
			{
				Projectile_BeginHandoff(smooth_origin, server_origin, fabs(parallel), local_speed, proj.has_last_draw_origin);
				PredProjectile_Cleanup(proj);
				return TRUE;
			}
		}

		// Matched but not adopted: retire the local prediction so the server
		// entity takes over cleanly.
		PredProjectile_Cleanup(proj);
		return FALSE;
	}

	return FALSE;
}

void EntUpdate_Projectile(float is_new)
{
	int sendflags = readbyte();

	if (sendflags & PROJECTILE_ORIGIN)
	{
		self.s_origin = [readcoord(), readcoord(), readcoord()];
		self.s_velocity = [readcoord(), readcoord(), readcoord()];
		self.s_time = readfloat();
	}

	if (sendflags & PROJECTILE_MODEL)
	{
		setmodel(self, modelnameforindex(readshort()));
		self.effects = readshort();
		self.projectile_type = (self.modelflags & MF_GRENADE) ? 1 : 0;
	}

	if (sendflags & PROJECTILE_ANGLES)
		self.angles = [readangle(), readangle(), readangle()];

	if (sendflags & PROJECTILE_OWNER)
		self.ownnum = readentitynum();

	if (sendflags & PROJECTILE_SPAWN_ORIGIN)
	{
		vector spawn_origin = [readcoord(), readcoord(), readcoord()];
		// A trail seed only, applied on creation (never as a later correction).
		if (is_new && vlen(spawn_origin - self.s_origin) < 150)
			self.oldorigin = spawn_origin;
	}

	if (is_new)
	{
		self.drawmask = MASK_ENGINE;
		self.predraw = Predraw_Projectile;
		self.effects |= EF_NOSHADOW;

		if (self.modelflags & MF_ROCKET)
			self.traileffectnum = Projectile_FindTrail(FALSE);
		else if (self.modelflags & MF_GRENADE)
			self.traileffectnum = Projectile_FindTrail(TRUE);

		if (self.oldorigin == '0 0 0')
			self.oldorigin = self.s_origin;

		if (self.projectile_type == 1)
			Projectile_SetGrenadeServerState(TRUE);
	}
	else if ((sendflags & PROJECTILE_ORIGIN) && self.projectile_type == 1)
	{
		// Live grenade correction: reset physics but keep client-owned spin.
		Projectile_SetGrenadeServerState(FALSE);
	}
	else if (sendflags & PROJECTILE_ORIGIN)
	{
		// Trail continuity: reset the trail seed on a large positional jump.
		if (vlen(self.s_origin - self.oldorigin) > 200)
			self.oldorigin = self.s_origin;
	}

	if (is_new)
	{
		// Adopt or retire any matching local prediction once authority arrives.
		if (PredProjectile_MatchProjectile())
			return;

		if (self.projectile_type != 1)
		{
			vector origin = self.s_origin + self.s_velocity * max(servertime - self.s_time, 0);
			if (Projectile_TraceBlocked(self.s_origin, origin))
			{
				self.drawmask = 0;
				return;
			}
		}
	}
}

void EntRemove_Projectile()
{
}
