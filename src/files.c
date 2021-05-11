#include "g_local.h"
#define MAX_TXTLEN	128

fileHandle_t std_fropen(const char *fmt, ...)
{
	va_list argptr;
	fileHandle_t handle;
	char text[MAX_TXTLEN] =
		{ 0 };

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	text[sizeof(text) - 1] = 0;

	if (trap_FS_OpenFile(text, &handle, FS_READ_BIN) < 0)
	{
		//G_bprint( 2, "Failed to open file: %s\n", text );
		return -1;
	}

	//G_bprint( 2, "Succesfully opened file: %s\n", text );
	return handle;
}

int std_fgetc(fileHandle_t handle)
{
	char c;
	int retval;

	if (handle < 0)
	{
		return -2;
	}

	retval = trap_FS_ReadFile(&c, 1, handle);
	//G_bprint( 2, "====> Read char: %d\n", c );

	return (retval == 1 ? c : -1);
}

char* std_fgets(fileHandle_t handle, char *buf, int limit)
{
	int c = '\0';
	char *string;

	if (handle < 0)
	{
		return NULL;
	}

	string = buf;
	while (--limit > 0 && ((c = std_fgetc(handle)) != -1))
	{
		if ((*string++ = c) == '\n')
		{
			break;
		}
	}

	*string = '\0';
	//G_bprint( 2, "====> Read string: %s\n", buf );

	return ((c == -1) && (string = buf)) ? NULL : buf;
}

void std_fclose(fileHandle_t handle)
{
	if (handle < 0)
	{
		return;
	}

	trap_FS_CloseFile(handle);
}

// Writing
fileHandle_t std_fwopen(const char *fmt, ...)
{
	va_list argptr;
	fileHandle_t handle;
	char text[MAX_TXTLEN] =
		{ 0 };

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	text[sizeof(text) - 1] = 0;

	if (trap_FS_OpenFile(text, &handle, FS_WRITE_BIN) < 0)
	{
		//G_bprint( 2, "Failed to open file: %s\n", text );
		return -1;
	}

	//G_bprint( 2, "Succesfully opened file: %s\n", text );
	return handle;
}

void std_fprintf(fileHandle_t handle, const char *fmt, ...)
{
	va_list argptr;
	char text[MAX_TXTLEN] =
		{ 0 };

	if (handle < 0)
	{
		return;
	}

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	text[sizeof(text) - 1] = 0;

	trap_FS_WriteFile(text, strlen(text), handle);
}
