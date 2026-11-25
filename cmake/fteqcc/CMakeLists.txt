# fteqcc, FTE's QuakeC compiler, from fteqw's engine/qclib (FTEQW_QCLIB): its
# ten C files, as its Makefile's qcc target builds them, with the host's C
# compiler (cmake/fteqcc.cmake configures this project apart from the mod's),
# without zlib, which nothing compiled here needs.
cmake_minimum_required(VERSION 3.15)
project(fteqcc LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

add_executable(fteqcc
	"${FTEQW_QCLIB}/qccmain.c" "${FTEQW_QCLIB}/qcc_pr_comp.c" "${FTEQW_QCLIB}/qcc_pr_lex.c"
	"${FTEQW_QCLIB}/packager.c" "${FTEQW_QCLIB}/decomp.c" "${FTEQW_QCLIB}/comprout.c" "${FTEQW_QCLIB}/hash.c"
	"${FTEQW_QCLIB}/qcc_cmdlib.c" "${FTEQW_QCLIB}/qcd_main.c" "${FTEQW_QCLIB}/qcctui.c")
# the revision and date its version tells, as fteqw's builds name them
target_compile_definitions(fteqcc PRIVATE NO_ZLIB SVNREVISION=${FTEQW_REVISION} SVNDATE=${FTEQW_DATE})
if(WIN32)
	# the C runtime's names for what qclib calls by POSIX's
	target_compile_definitions(fteqcc PRIVATE _CRT_SECURE_NO_WARNINGS strcasecmp=_stricmp strncasecmp=_strnicmp)
else()
	target_link_libraries(fteqcc PRIVATE m)
endif()

install(TARGETS fteqcc RUNTIME DESTINATION bin)
