cmake_minimum_required(VERSION 3.2)

PROJECT(ssman)

INCLUDE_DIRECTORIES(
	include
	)

AUX_SOURCE_DIRECTORY(src DIR_SRCS)

set(SRC_PATH src)

ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_PATH}/ssman_db.c ${SRC_PATH}/utils.c)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#if(CMAKE_COMPILER_IS_GNUCXX)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lev")
#	message(STATUS "optional:-std=c++11")
#endif(CMAKE_COMPILER_IS_GNUCXX)

target_link_libraries(ssman -lev -lm)
