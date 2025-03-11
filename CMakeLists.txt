cmake_minimum_required(VERSION 3.10)
project(KGPKeeper)

set(CMAKE_CXX_STANDARD 20)  
set(CMAKE_CXX_FLAGS "-Wall -Wextra")

include_directories(include)  
file(GLOB UTILS_SRC src/utils/*.cpp)  

add_executable(server src/server.cpp ${UTILS_SRC})
add_executable(client src/client.cpp ${UTILS_SRC})
add_executable(admin src/admin.cpp ${UTILS_SRC})
add_executable(assistant src/AssistantServer.cpp ${UTILS_SRC})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

