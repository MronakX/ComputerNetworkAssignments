#cmake_minimum_required(VERSION 2.8)
project(assignment3)

set(CMAKE_CXX_STANDARD 11)


#添加头文件搜索路径
include_directories(/usr/local/include)

#添加库文件搜索路径
link_directories(/usr/local/lib)

add_executable(agent
        util.cpp
		common.cpp
		agent.cpp)
target_link_libraries(agent)

add_executable(router
        util.cpp
		common.cpp
		router.cpp)
target_link_libraries(router)
