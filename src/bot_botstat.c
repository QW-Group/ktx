// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

// FIXME: Copy/paste from combat.c
#ifndef Q3_VM
// qvm have some bugs/round problem as i get from SD-Angel, so this trick
float newceil( float f )
{
	return ceil(((int)(f*1000.0))/1000.0);
}
#else
// native use lib ceil function
#define newceil ceil
#endif

float TotalStrength (float health, float armorValue, float armorType)
{
	return max (0, min (
		health / (1 - armorType),
		health + armorValue
	) );
}

float TotalStrengthAfterDamage (float health, float armorValue, float armorType, float damage)
{
	float damage_saved = newceil (damage * armorType);
	if (damage_saved > armorValue) {
		// lost all armor
		damage_saved = armorValue;
		armorType = 0;
	}
	damage = newceil (damage - damage_saved);
	health -= damage;

	return health <= 0 ? 0 : TotalStrength (health, armorValue, armorType);
}

// Called every time the player's statistics change (item pickups etc)
// Evaluate desire for armor, health etc based on the improvement it would cause
void FrogbotSetHealthArmour(gedict_t* client)
{
	float min_first = 0, min_second = 0;
	client->fb.total_armor = client->s.v.armortype * client->s.v.armorvalue;
	client->fb.total_damage = TotalStrength (client->s.v.health, client->s.v.armorvalue, client->s.v.armortype);

	client->fb.desire_armor1 = client->fb.desire_armor2 = client->fb.desire_armorInv = 0;
	if (client->fb.total_armor < 160) {
		client->fb.desire_armorInv = max(0, TotalStrength(client->s.v.health, 200.0f, 0.8f) - client->fb.total_damage);

		if (client->fb.total_armor < 90) {
			client->fb.desire_armor2 = max(0, TotalStrength(client->s.v.health, 150.0f, 0.6f) - client->fb.total_damage);

			if (client->fb.total_armor < 30) {
				client->fb.desire_armor1 = max(0, 2 * (TotalStrength(client->s.v.health, 100, 0.3f) - client->fb.total_damage));
			}
		}
	}

	client->fb.desire_health0 = client->fb.desire_mega_health = 0;
	if (client->s.v.health < 250) {
		float new_health = min (client->s.v.health + 100, 250);
		client->fb.desire_mega_health = TotalStrength (new_health, client->s.v.armorvalue, client->s.v.armortype) - client->fb.total_damage;

		client->fb.desire_health0 = 0;
		if (client->s.v.health < 100) {
			new_health = min(client->s.v.health + 25, 100);

			client->fb.desire_health0 = 2 * (TotalStrength(new_health, client->s.v.armorvalue, client->s.v.armortype) - client->fb.total_damage);
		}
	}

	if ((int)client->ctf_flag & CTF_RUNE_RES) {
		client->fb.total_damage *= 2;
	}
}

void FrogbotSetFirepower(gedict_t* self)
{
	int items_ = (int) self->s.v.items;
	float firepower_ = 100.0f;
	int attackbonus = 0;

	self->fb.weapon_refresh_time = 1000000;
	if (deathmatch != 4) {
		firepower_ = 0;
		if (items_ & IT_ROCKET_LAUNCHER) {
			firepower_ = self->s.v.ammo_rockets * 8;
			if (self->s.v.ammo_rockets) {
				attackbonus = 50;
			}
		}
		else if (items_ & IT_GRENADE_LAUNCHER) {
			firepower_ = self->s.v.ammo_rockets * 6;
			if (firepower_ > 50) {
				firepower_ = 50;
			}
		}

		if (items_ & IT_LIGHTNING) {
			firepower_ = firepower_ + self->s.v.ammo_cells;
			if (self->s.v.ammo_cells >= 10) {
				attackbonus = attackbonus + 50;
			}
		}
		if (items_ & IT_EITHER_NAILGUN) {
			firepower_ = firepower_ + (self->s.v.ammo_nails * 0.1);
		}
		if (items_ & IT_SUPER_SHOTGUN) {
			if (self->s.v.ammo_shells >= 50) {
				firepower_ = firepower_ + 20;
			}
			else {
				firepower_ = firepower_ + self->s.v.ammo_shells * 0.4;
			}
		}
		else {
			if (self->s.v.ammo_shells >= 25) {
				firepower_ = firepower_ + 10;
			}
			else {
				firepower_ = firepower_ + self->s.v.ammo_shells * 0.4;
			}
		}
		firepower_ = min (firepower_, 100);

		self->fb.desire_rockets = max(5, 20 - self->s.v.ammo_rockets);
		self->fb.desire_cells = max(2.5, (50 - self->s.v.ammo_cells) * 0.2);
		self->fb.desire_rocketlauncher = max(100 - firepower_, self->fb.desire_rockets);
		self->fb.desire_lightning = max(self->fb.desire_rocketlauncher, self->fb.desire_cells);

		if (items_ & IT_ROCKET_LAUNCHER) {
			self->fb.desire_rockets = self->fb.desire_grenadelauncher = self->fb.desire_rocketlauncher;
		}
		else {
			self->fb.desire_grenadelauncher = 0;
			if (firepower_ < 50) {
				self->fb.desire_grenadelauncher = 50 - firepower_;
			}

			if (self->fb.desire_grenadelauncher < self->fb.desire_rockets) {
				self->fb.desire_grenadelauncher = self->fb.desire_rockets;
			}
			if (items_ & IT_GRENADE_LAUNCHER) {
				self->fb.desire_rockets = self->fb.desire_grenadelauncher;
			}
		}

		if (items_ & IT_LIGHTNING) {
			self->fb.desire_cells = self->fb.desire_lightning;
		}

		self->fb.desire_nails = self->fb.desire_shells = 0;
		if (firepower_ < 20) {
			self->fb.desire_nails = 2.5 - (self->s.v.ammo_nails * 0.0125);
			if (self->s.v.ammo_shells < 50) {
				self->fb.desire_shells = 2.5 - (self->s.v.ammo_shells * 0.05);
			}
		}

		self->fb.desire_supershotgun = max(0, 20 - firepower_);
		self->fb.desire_nailgun = self->fb.desire_supernailgun = max(self->fb.desire_supershotgun, self->fb.desire_nails);
		self->fb.desire_supershotgun = max(self->fb.desire_supershotgun, self->fb.desire_shells);

		if (items_ & IT_EITHER_NAILGUN) {
			self->fb.desire_nails = self->fb.desire_supernailgun;
		}
		if (items_ & IT_SUPER_SHOTGUN) {
			self->fb.desire_shells = self->fb.desire_supershotgun;
		}

		firepower_ = bound(0, firepower_ + attackbonus, 100);
	}

	if (self->super_damage_finished > g_globalvars.time) {
		firepower_ *= (deathmatch == 4 ? 8 : 4);
	}
	if (self->ctf_flag & CTF_RUNE_STR) {
		firepower_ *= 2;
	}
	self->fb.firepower = firepower_;
}

void FrogbotWeaponFiredEvent(gedict_t* self) {
	self->fb.weapon_refresh_time = min (g_globalvars.time + 1, self->fb.weapon_refresh_time);
}
