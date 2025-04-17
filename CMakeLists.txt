cmake_minimum_required(VERSION 3.10)
project(KGPKeeper)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare")

include_directories(include)
file(GLOB UTILS_SRC src/coordination.cpp)

# Create executables
add_executable(server_api src/server_api.cpp ${UTILS_SRC})
add_executable(client_api src/client_api.cpp)
add_executable(admin src/admin.cpp ${UTILS_SRC})
add_executable(loadbalancer src/loadbalancer.cpp)
add_executable(init src/init.cpp)

# Link libraries
target_link_libraries(server_api curl pthread)
target_link_libraries(client_api curl pthread)
target_link_libraries(admin curl pthread)
target_link_libraries(loadbalancer curl pthread)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
