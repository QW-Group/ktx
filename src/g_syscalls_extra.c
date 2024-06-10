#include "g_local.h"
#include "g_syscalls.h"

typedef union fi_s
{
	float _float;
	intptr_t _int;
} fi_t;

void trap_SetExtField_i(gedict_t *ed, const char *fieldname, int val)
{
	if (HAVEEXTENSION(G_SETEXTFIELD))
	{
		trap_SetExtField(ed, fieldname, val);
	}
	else
	{
		G_bprint(PRINT_HIGH, "SetExtField(%s, %s, %d) not supported by server\n", ed->classname, fieldname, val);
	}
}

void trap_SetExtField_f(gedict_t *ed, const char *fieldname, float val)
{
	if (HAVEEXTENSION(G_SETEXTFIELD))
	{
		fi_t rc;
		rc._float = val;
		trap_SetExtField(ed, fieldname, rc._int);
	}
	else
	{
		G_bprint(PRINT_HIGH, "SetExtField(%s, %s, %f) not supported by server\n", ed->classname, fieldname, val);
	}
}

int trap_GetExtField_i(gedict_t *ed, const char *fieldname)
{
	int ival = -1;
	if (HAVEEXTENSION(G_GETEXTFIELD))
	{
		ival = trap_GetExtField(ed, fieldname);
	}
	return ival;
}

float trap_GetExtField_f(gedict_t *ed, const char *fieldname)
{
	fi_t tmp;
	tmp._float = -1.0f;
	if (HAVEEXTENSION(G_GETEXTFIELD))
	{
		tmp._int = trap_GetExtField(ed, fieldname);
	}
	return tmp._float;
}
