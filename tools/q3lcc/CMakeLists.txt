cmake_minimum_required( VERSION 3.4.0 )

project(q3lcc)

add_executable(lburg lburg/gram.c lburg/lburg.c)       
target_include_directories (lburg  PRIVATE "lburg ")

set( DAGCHECK_SRC dagcheck.c )
set( RCC_SOURCES
    "src/alloc.c"
    "src/bind.c"
    "src/bytecode.c"
    "src/dag.c"
    "src/decl.c"
    "src/enode.c"
    "src/error.c"
    "src/event.c"
    "src/expr.c"
    "src/gen.c"
    "src/init.c"
    "src/inits.c"
    "src/input.c"
    "src/lex.c"
    "src/list.c"
    "src/main.c"
    "src/null.c"
    "src/output.c"
    "src/prof.c"
    "src/profio.c"
    "src/simp.c"
    #    "src/stab.c"
    "src/stmt.c"
    "src/string.c"
    "src/sym.c"
    "src/symbolic.c"
    "src/trace.c"
    "src/tree.c"
    "src/types.c"
    )

add_custom_command(OUTPUT ${DAGCHECK_SRC}
    COMMAND lburg "${CMAKE_CURRENT_SOURCE_DIR}/src/dagcheck.md" ${DAGCHECK_SRC}
        DEPENDS lburg 
    )
add_executable(q3rcc ${RCC_SOURCES} ${DAGCHECK_SRC})       
target_include_directories (q3rcc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

set( CPP_SOURCES
    "cpp/cpp.c"
    "cpp/eval.c"
    "cpp/getopt.c"
    "cpp/hideset.c"
    "cpp/include.c"
    "cpp/lex.c"
    "cpp/macro.c"
    "cpp/nlist.c"
    "cpp/tokens.c"
    "cpp/unix.c"
    )

add_executable(q3cpp ${CPP_SOURCES} )       
target_include_directories (q3cpp  PRIVATE "cpp ")

add_executable(q3lcc etc/bytecode.c etc/lcc.c )       
add_dependencies(q3lcc q3cpp q3rcc)
target_include_directories (q3lcc  PRIVATE "cpp ")

