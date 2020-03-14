cmake_minimum_required(VERSION 3.10)

project(imp C)

set(CMAKE_C_STANDARD 99)

aux_source_directory(src/utils DIR_UTILS)
aux_source_directory(src/runtime DIR_RUNTIME)
aux_source_directory(src/compiler DIR_COMPILER)
aux_source_directory(src/compiler/phase DIR_COMPILER_PHASE)
aux_source_directory(src/compiler/backend DIR_COMPILER_BACKEND)

include_directories(src)
add_definitions(-DUSE_TYPEOF)
add_definitions(-Wall -g3)

message("message : ${DIR_SRC}")
add_library(${PROJECT_NAME}_runtime SHARED ${DIR_RUNTIME} ${DIR_UTILS})

add_executable(${PROJECT_NAME}_compiler
  ${DIR_RUNTIME}
  ${DIR_COMPILER}
  ${DIR_UTILS}
  ${DIR_COMPILER_PHASE}
  ${DIR_COMPILER_BACKEND}
  )

# 测试
add_executable(arena_test
  src/utils/arena.c
  test/utils/arena_test.c
  )

add_executable(list_test test/utils/list_test.c)
add_executable(bit_test test/utils/bit_test.c)
add_executable(buffer_test
  src/utils/buffer.c
  test/utils/buffer_test.c
  )
add_executable(hashmap_test
  src/utils/hashmap.c
  test/utils/hashmap_test.c
  ${DIR_UTILS}
  )
add_executable(gc_test
  src/runtime/gc.c
  test/runtime/gc_test.c
  )
add_executable(runtime_test
  ${DIR_RUNTIME}
  ${DIR_UTILS}
  test/runtime/runtime_test.c
  )

enable_testing()

add_test(arena_test arena_test)
add_test(list_test list_test)
add_test(bit_test bit_test)
add_test(buffer_test buffer_test)
add_test(hashmap_test hashmap_test)
add_test(gc_test gc_test)
add_test(runtime_test runtime_test ${PROJECT_SOURCE_DIR}/test/runtime1.scm)
