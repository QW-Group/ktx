# fteqcc, which compiles the client-side QuakeC (qcsrc) into csprogs.dat: the
# one the FTEQCC environment variable names, else one on the PATH, else one
# built here at configure. That one is fteqw's qclib at the commit below
# (cmake/fteqcc), fetched with git and built with the host's C compiler, also
# when the mod itself is cross compiled. It is installed under KTX_DEPS_DIR in
# a directory named for what builds it, where later configures find it.
# KTX_FTEQCC_EXE is the one used.

set(KTX_FTEQW_COMMIT f937b9d88f71fc4429db5fe56c6a98d922711b2e)

set(KTX_DEPS_DIR "${CMAKE_BINARY_DIR}/deps" CACHE PATH
	"Where the build installs what it fetches and builds itself (fteqcc)")

# cached either way, so one configure with FTEQCC set is enough
if(DEFINED ENV{FTEQCC} AND NOT "$ENV{FTEQCC}" STREQUAL "")
	set(KTX_FTEQCC "$ENV{FTEQCC}" CACHE FILEPATH "fteqcc, which compiles the client-side QuakeC" FORCE)
else()
	find_program(KTX_FTEQCC NAMES fteqcc64 fteqcc DOC "fteqcc, which compiles the client-side QuakeC")
endif()

if(KTX_FTEQCC)
	set(KTX_FTEQCC_EXE "${KTX_FTEQCC}")
	message(STATUS "fteqcc: ${KTX_FTEQCC_EXE}")
	return()
endif()

file(TO_CMAKE_PATH "${KTX_DEPS_DIR}" ktx_deps_dir)
get_filename_component(ktx_deps_dir "${ktx_deps_dir}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
file(SHA1 "${CMAKE_CURRENT_LIST_DIR}/fteqcc/CMakeLists.txt" ktx_fteqcc_hash)
string(SHA1 ktx_fteqcc_hash "${KTX_FTEQW_COMMIT}${ktx_fteqcc_hash}")
string(SUBSTRING "${ktx_fteqcc_hash}" 0 12 ktx_fteqcc_hash)
set(ktx_fteqcc_prefix "${ktx_deps_dir}/fteqcc-${ktx_fteqcc_hash}")
# the host's suffix, not the mod's
if(CMAKE_HOST_WIN32)
	set(KTX_FTEQCC_EXE "${ktx_fteqcc_prefix}/bin/fteqcc.exe")
else()
	set(KTX_FTEQCC_EXE "${ktx_fteqcc_prefix}/bin/fteqcc")
endif()

if(NOT EXISTS "${KTX_FTEQCC_EXE}")
	if(CMAKE_VERSION VERSION_LESS 3.15)
		message(FATAL_ERROR "fteqcc: none on the PATH or in FTEQCC, and building one needs CMake 3.15; "
			"provide one, or configure with -DCSQC_SUPPORT=OFF")
	endif()
	find_package(Git REQUIRED)
	message(STATUS "fteqcc: none on the PATH or in FTEQCC; building fteqw ${KTX_FTEQW_COMMIT}'s into ${ktx_fteqcc_prefix}")

	# fetched, built and installed beside the mod, moved in place whole, so a
	# build that fails leaves no prefix behind; what the steps print goes to a
	# log, and shows when one fails. The fetch is tried again after a while.
	set(ktx_src "${CMAKE_BINARY_DIR}/fteqcc-src")
	set(ktx_build "${CMAKE_BINARY_DIR}/fteqcc-build")
	set(ktx_log "${CMAKE_BINARY_DIR}/fteqcc-build.log")
	file(WRITE "${ktx_log}" "")
	function(ktx_fteqcc_step tries)
		foreach(attempt RANGE 1 ${tries})
			execute_process(COMMAND ${ARGN} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
			file(APPEND "${ktx_log}" "${output}")
			if(NOT result)
				return()
			endif()
			if(attempt LESS tries)
				message(STATUS "fteqcc: that failed, trying again in 15 s (${ktx_log})")
				execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 15)
			endif()
		endforeach()
		message("${output}")
		message(FATAL_ERROR "fteqcc: couldn't build it (${ktx_log}); put one on the PATH, name it in FTEQCC, "
			"or configure with -DCSQC_SUPPORT=OFF")
	endfunction()

	# the one commit, and of it qclib's files only
	file(REMOVE_RECURSE "${ktx_src}" "${ktx_build}" "${ktx_fteqcc_prefix}.partial")
	file(MAKE_DIRECTORY "${ktx_src}")
	set(ktx_git "${GIT_EXECUTABLE}" -C "${ktx_src}" -c advice.detachedHead=false)
	ktx_fteqcc_step(1 ${ktx_git} init -q)
	ktx_fteqcc_step(3 ${ktx_git} fetch -q --depth 1 --filter=blob:none https://github.com/fte-team/fteqw.git
		${KTX_FTEQW_COMMIT})
	ktx_fteqcc_step(1 ${ktx_git} sparse-checkout set --no-cone /engine/qclib/)
	ktx_fteqcc_step(3 ${ktx_git} checkout -q FETCH_HEAD)
	execute_process(COMMAND ${ktx_git} rev-parse --short=7 HEAD OUTPUT_VARIABLE ktx_rev OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND ${ktx_git} log -1 --format=%cs OUTPUT_VARIABLE ktx_date OUTPUT_STRIP_TRAILING_WHITESPACE)

	# The host's compiler. A native build hands on its own; a cross build's
	# toolchain is for the target, so the helper is configured without it and
	# without the environment's compiler settings, and takes the system's.
	set(ktx_args -S "${CMAKE_CURRENT_LIST_DIR}/fteqcc" -B "${ktx_build}" -G "${CMAKE_GENERATOR}"
		"-DFTEQW_QCLIB=${ktx_src}/engine/qclib" "-DFTEQW_REVISION=git-${ktx_rev}" "-DFTEQW_DATE=${ktx_date}"
		-DCMAKE_BUILD_TYPE=Release)
	if(CMAKE_MAKE_PROGRAM)
		list(APPEND ktx_args "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}")
	endif()
	if(NOT CMAKE_CROSSCOMPILING)
		list(APPEND ktx_args "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
		set(ktx_configure "${CMAKE_COMMAND}" ${ktx_args})
	else()
		set(ktx_configure "${CMAKE_COMMAND}" -E env --unset=CC --unset=CFLAGS --unset=LDFLAGS "${CMAKE_COMMAND}"
			${ktx_args})
	endif()
	ktx_fteqcc_step(1 ${ktx_configure})
	ktx_fteqcc_step(1 "${CMAKE_COMMAND}" --build "${ktx_build}" --config Release)
	ktx_fteqcc_step(1 "${CMAKE_COMMAND}" --install "${ktx_build}" --config Release --prefix "${ktx_fteqcc_prefix}.partial")
	file(RENAME "${ktx_fteqcc_prefix}.partial" "${ktx_fteqcc_prefix}")
	file(REMOVE_RECURSE "${ktx_src}" "${ktx_build}")
	if(NOT EXISTS "${KTX_FTEQCC_EXE}")
		message(FATAL_ERROR "fteqcc: built into ${ktx_fteqcc_prefix}, but there is no ${KTX_FTEQCC_EXE}")
	endif()
endif()
message(STATUS "fteqcc: ${KTX_FTEQCC_EXE}")
