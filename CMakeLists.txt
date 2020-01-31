cmake_minimum_required(VERSION 3.10)

project(imp C)

set(CMAKE_C_STANDARD 99)

aux_source_directory(src/utils DIR_UTILS)
aux_source_directory(src/runtime DIR_RUNTIME)

include_directories(src)
add_definitions(-DUSE_TYPEOF)
add_definitions(-Wall -g3)

message("message : ${DIR_SRC}")
add_executable(${PROJECT_NAME}_runtime ${DIR_RUNTIME} ${DIR_UTILS})

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

enable_testing()

add_test(arena_test arena_test)
add_test(list_test list_test)
add_test(bit_test bit_test)
add_test(buffer_test buffer_test)
