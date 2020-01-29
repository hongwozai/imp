cmake_minimum_required(VERSION 3.10)

project(imp C)

set(CMAKE_C_STANDARD 99)

aux_source_directory(src DIR_SRC)

include_directories(src)
add_definitions(-DUSE_TYPEOF)
add_definitions(-Wall -g3)

add_executable(${PROJECT_NAME} ${DIR_SRC})

# 测试
add_executable(arena_test
  src/utils/arena.c
  test/utils/arena_test.c
  )

add_executable(list_test test/utils/list_test.c)

enable_testing()

add_test(arena_test arena_test)
add_test(list_test list_test)