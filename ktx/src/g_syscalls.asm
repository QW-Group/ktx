code

equ    trap_GetApiVersion    -1
equ    trap_DPrintf          -2
equ    trap_Error            -3
equ    trap_GetEntityToken   -4
equ    trap_spawn            -5
equ    trap_remove           -6
equ    trap_precache_sound   -7
equ    trap_precache_model   -8
equ    trap_lightstyle       -9
equ    trap_setorigin        -10
equ    trap_setsize          -11
equ    trap_setmodel         -12
equ    trap_BPrint           -13
equ    trap_SPrint           -14
equ    trap_CenterPrint      -15
equ    trap_ambientsound     -16
equ    trap_sound            -17
equ    trap_traceline        -18
equ    trap_checkclient      -19
equ    trap_stuffcmd         -20
equ    trap_localcmd         -21
equ    trap_cvar             -22
equ    trap_cvar_set         -23
                             
equ    trap_walkmove         -25
equ    trap_droptofloor      -26
equ    trap_checkbottom      -27
equ    trap_pointcontents    -28
equ    trap_nextent          -29
                             
equ    trap_makestatic       -31
equ    trap_setspawnparam    -32
equ    trap_changelevel      -33
equ    trap_logfrag          -34
equ    trap_infokey          -35
equ    trap_multicast        -36
equ    trap_disableupdates   -37
equ    trap_WriteByte        -38
equ    trap_WriteChar        -39
equ    trap_WriteShort       -40
equ    trap_WriteLong        -41
equ    trap_WriteAngle       -42
equ    trap_WriteCoord       -43
equ    trap_WriteString      -44
equ    trap_WriteEntity      -45
equ    trap_FlushSignon      -46

equ	memset			-47
equ	memcpy			-48
equ	strncpy			-49
equ	sin			-50
equ	cos			-51
equ	atan2			-52
equ	sqrt			-53
equ     floor			-54
equ	ceil			-55
equ     acos			-56
equ     trap_CmdArgc            -57
equ     trap_CmdArgv            -58

equ     trap_TraceCapsule        -59

equ	trap_FS_OpenFile         -60
equ    	trap_FS_CloseFile        -61
equ    	trap_FS_ReadFile         -62
equ    	trap_FS_WriteFile        -63
equ	trap_FS_SeekFile         -64
equ	trap_FS_TellFile         -65
equ	trap_FS_GetFileList	 -66

equ     trap_cvar_set_float      -67
equ     trap_cvar_string	 -68

equ     trap_Map_Extension	 -69

equ     strcmp	 		 -70
equ     strncmp	 		 -71
equ     stricmp	 		 -72
equ     strnicmp		 -73

equ     trap_find		 -74

equ     trap_executecmd		 -75
equ     trap_conprint		 -76
equ     trap_readcmd		 -77
equ     trap_redirectcmd         -78

equ	trap_AddBot		 -79
equ	trap_RemoveBot		 -80
equ	trap_SetBotUserInfo	 -81
equ	trap_SetBotCMD		 -82
