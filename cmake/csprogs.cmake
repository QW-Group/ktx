# cmake -DFTEQCC=<fteqcc> -DSRC=<qcsrc> -DOUT=<dir> -P csprogs.cmake
#
# Compiles the client-side QuakeC in SRC into OUT/csprogs.dat. fteqcc writes
# where progs.src says, beside it, so it compiles a copy in OUT/qcsrc and the
# source tree is left alone. It also exits with success when it could not write
# its output, so the file is checked for. Quiet unless it fails.

file(REMOVE_RECURSE "${OUT}/qcsrc")
file(MAKE_DIRECTORY "${OUT}/qcsrc")
file(GLOB sources "${SRC}/*.qc" "${SRC}/progs.src")
file(COPY ${sources} DESTINATION "${OUT}/qcsrc")

execute_process(COMMAND "${FTEQCC}" -srcfile progs.src -O2 -Wall
	WORKING_DIRECTORY "${OUT}/qcsrc"
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE output)
if(NOT result EQUAL 0 OR NOT EXISTS "${OUT}/qcsrc/csprogs.dat")
	message(FATAL_ERROR "fteqcc couldn't compile ${SRC} (${result}):\n${output}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy "${OUT}/qcsrc/csprogs.dat" "${OUT}/csprogs.dat")
