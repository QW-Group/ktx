#include "g_local.h"
#include "g_syscalls.h"

typedef union fi_s
{
	float _float;
	int _int;
} fi_t;

static unsigned int field_ref_alpha = 0;
static unsigned int field_ref_colormod = 0;
static unsigned int field_ref_sendentity = 0;
static unsigned int field_ref_pvsflags = 0;

void ExtFieldSetAlpha(gedict_t *ed, float alpha)
{
	alpha = bound(0.0f, alpha, 1.0f);
	if (!field_ref_alpha && HAVEEXTENSION(G_MAPEXTFIELDPTR) && HAVEEXTENSION(G_SETEXTFIELDPTR))
	{
		field_ref_alpha = trap_MapExtFieldPtr("alpha");
	}
	if (field_ref_alpha)
	{
		trap_SetExtFieldPtr(ed, field_ref_alpha, (void*)&alpha, sizeof(float));
	}
	else if (HAVEEXTENSION(G_SETEXTFIELD))
	{
		fi_t v;
		v._float = alpha;
		trap_SetExtField(ed, "alpha", v._int);
	}
	else if (cvar("developer"))
	{
		G_bprint(PRINT_HIGH, "alpha needs SetExtField or MapExtFieldPtr and SetExtFieldPtr support in server\n");
	}
}

float ExtFieldGetAlpha(gedict_t *ed)
{
	fi_t tmp;
	tmp._float = -1.0f;
	if (!field_ref_alpha && HAVEEXTENSION(G_MAPEXTFIELDPTR) && HAVEEXTENSION(G_GETEXTFIELDPTR))
	{
		field_ref_alpha = trap_MapExtFieldPtr("alpha");
	}
	if (field_ref_alpha)
	{
		trap_GetExtFieldPtr(ed, field_ref_alpha, (void*)&tmp._float, sizeof(float));
	}
	else if (HAVEEXTENSION(G_GETEXTFIELD))
	{
		tmp._int = trap_GetExtField(ed, "alpha");
	}
	else if (cvar("developer"))
	{
		G_bprint(PRINT_HIGH, "alpha needs GetExtField or MapExtFieldPtr and GetExtFieldPtr support in server\n");
	}
	return tmp._float;
}

void ExtFieldSetColorMod(gedict_t *ed, float r, float g, float b)
{
	if (!field_ref_colormod && HAVEEXTENSION(G_MAPEXTFIELDPTR) && HAVEEXTENSION(G_SETEXTFIELDPTR))
	{
		field_ref_colormod = trap_MapExtFieldPtr("colormod");
	}
	if (field_ref_colormod)
	{
		float rgb[3];
		rgb[0] = max(0.0f, r);
		rgb[1] = max(0.0f, g);
		rgb[2] = max(0.0f, b);
		trap_SetExtFieldPtr(ed, field_ref_colormod, (void*)&rgb, sizeof(rgb));
	}
	else if (cvar("developer"))
	{
		G_bprint(PRINT_HIGH, "colormod needs MapExtFieldPtr and SetExtFieldPtr support in server\n");
	}
}

void ExtFieldSetSendEntity(gedict_t *ed, func_t callback)
{
	if (!field_ref_sendentity && HAVEEXTENSION(G_MAPEXTFIELDPTR) && HAVEEXTENSION(G_SETEXTFIELDPTR))
	{
		field_ref_sendentity = trap_MapExtFieldPtr("SendEntity");
	}
	if (field_ref_sendentity)
	{
		int enable = 1;
		trap_SetExtFieldPtr(ed, field_ref_sendentity, (void*)&enable, sizeof(int));
		ed->SendEntity = callback; // invoked via GAME_EDICT_CSQCSEND

	}
	else if (HAVEEXTENSION(G_SETEXTFIELD))
	{
		trap_SetExtField(ed, "SendEntity", 1);
		ed->SendEntity = callback; // invoked via GAME_EDICT_CSQCSEND
	}
	else if (cvar("developer"))
	{
		G_bprint(PRINT_HIGH, "SendEntity needs SetExtField or MapExtFieldPtr and SetExtFieldPtr support in server\n");
	}
}

void ExtFieldSetPvsFlags(gedict_t *ed, float pvsflags)
{
	if (!field_ref_pvsflags && HAVEEXTENSION(G_MAPEXTFIELDPTR) && HAVEEXTENSION(G_SETEXTFIELDPTR))
	{
		field_ref_pvsflags = trap_MapExtFieldPtr("pvsflags");
	}
	if (field_ref_pvsflags)
	{
		trap_SetExtFieldPtr(ed, field_ref_pvsflags, (void*)&pvsflags, sizeof(float));
	}
	else if (HAVEEXTENSION(G_SETEXTFIELD))
	{
		fi_t tmp;
		tmp._float = pvsflags;
		trap_SetExtField(ed, "pvsflags", tmp._int);
	}
	else if (cvar("developer"))
	{
		G_bprint(PRINT_HIGH, "pvsflags needs SetExtField or MapExtFieldPtr and SetExtFieldPtr support in server\n");
	}
}

void SetSendNeeded(gedict_t *ed, int sendflags, int unicast)
{
	if (!HAVEEXTENSION(G_SETSENDNEEDED))
	{
		G_bprint(PRINT_HIGH, "SetSendNeeded needs support in server\n");
		return;
	}
	trap_SetSendNeeded(NUM_FOR_EDICT(ed), sendflags, unicast);
}
