cmake_minimum_required(VERSION 3.2)

PROJECT(ssman)

INCLUDE_DIRECTORIES(
	include
	)

AUX_SOURCE_DIRECTORY(src DIR_SRCS)
SET(DEMO_PATH demo)
ADD_EXECUTABLE(ssman ${DIR_SRCS} ${DEMO_PATH}/ssman_demo.c)
ADD_EXECUTABLE(ssman_db ${DIR_SRCS} ${DEMO_PATH}/ssman_db_demo.c)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#ADD_DEFINITIONS("-W -Wall")
#if(CMAKE_COMPILER_IS_GNUCC)
#	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
#	message(STATUS "optional:-std=c11")
#endif(CMAKE_COMPILER_IS_GNUCC)

target_link_libraries(ssman -lev -lm -lsqlite3)
target_link_libraries(ssman_db -lev -lm -lsqlite3)
