/*
 * $Id$
 */

// native_lib.c -- different OS'es have different lib functions implementation, work around.

// this file is only included when building a dll
#ifdef Q3_VM
#error "Do not use in VM build"
#endif

#include "g_local.h"


#ifdef _WIN32

// ripped from VVD code

int Q_vsnprintf(char *buffer, size_t count, const char *format, va_list argptr)
{
	int ret;
	if (!count)
		return 0;

	ret = _vsnprintf(buffer, count, format, argptr);
	buffer[count - 1] = 0;

	return ret;
}

#if !defined(_MSC_VER) || (_MSC_VER < 1900)
int snprintf(char *buffer, size_t count, char const *format, ...)
{
	int ret;
	va_list argptr;

	if (!count)
		return 0;

	va_start(argptr, format);
	ret = _vsnprintf(buffer, count, format, argptr);
	buffer[count - 1] = 0;
	va_end(argptr);

	return ret;
}
#endif

#endif // _WIN32

