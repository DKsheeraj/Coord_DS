# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/datta/Documents/SEM-8/KGPKeeper

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/datta/Documents/SEM-8/KGPKeeper/build

# Include any dependencies generated for this target.
include CMakeFiles/server_api.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/server_api.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/server_api.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/server_api.dir/flags.make

CMakeFiles/server_api.dir/src/server_api.cpp.o: CMakeFiles/server_api.dir/flags.make
CMakeFiles/server_api.dir/src/server_api.cpp.o: ../src/server_api.cpp
CMakeFiles/server_api.dir/src/server_api.cpp.o: CMakeFiles/server_api.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/datta/Documents/SEM-8/KGPKeeper/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/server_api.dir/src/server_api.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/server_api.dir/src/server_api.cpp.o -MF CMakeFiles/server_api.dir/src/server_api.cpp.o.d -o CMakeFiles/server_api.dir/src/server_api.cpp.o -c /home/datta/Documents/SEM-8/KGPKeeper/src/server_api.cpp

CMakeFiles/server_api.dir/src/server_api.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/server_api.dir/src/server_api.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/datta/Documents/SEM-8/KGPKeeper/src/server_api.cpp > CMakeFiles/server_api.dir/src/server_api.cpp.i

CMakeFiles/server_api.dir/src/server_api.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/server_api.dir/src/server_api.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/datta/Documents/SEM-8/KGPKeeper/src/server_api.cpp -o CMakeFiles/server_api.dir/src/server_api.cpp.s

CMakeFiles/server_api.dir/src/coordination.cpp.o: CMakeFiles/server_api.dir/flags.make
CMakeFiles/server_api.dir/src/coordination.cpp.o: ../src/coordination.cpp
CMakeFiles/server_api.dir/src/coordination.cpp.o: CMakeFiles/server_api.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/datta/Documents/SEM-8/KGPKeeper/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/server_api.dir/src/coordination.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/server_api.dir/src/coordination.cpp.o -MF CMakeFiles/server_api.dir/src/coordination.cpp.o.d -o CMakeFiles/server_api.dir/src/coordination.cpp.o -c /home/datta/Documents/SEM-8/KGPKeeper/src/coordination.cpp

CMakeFiles/server_api.dir/src/coordination.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/server_api.dir/src/coordination.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/datta/Documents/SEM-8/KGPKeeper/src/coordination.cpp > CMakeFiles/server_api.dir/src/coordination.cpp.i

CMakeFiles/server_api.dir/src/coordination.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/server_api.dir/src/coordination.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/datta/Documents/SEM-8/KGPKeeper/src/coordination.cpp -o CMakeFiles/server_api.dir/src/coordination.cpp.s

# Object files for target server_api
server_api_OBJECTS = \
"CMakeFiles/server_api.dir/src/server_api.cpp.o" \
"CMakeFiles/server_api.dir/src/coordination.cpp.o"

# External object files for target server_api
server_api_EXTERNAL_OBJECTS =

server_api: CMakeFiles/server_api.dir/src/server_api.cpp.o
server_api: CMakeFiles/server_api.dir/src/coordination.cpp.o
server_api: CMakeFiles/server_api.dir/build.make
server_api: CMakeFiles/server_api.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/datta/Documents/SEM-8/KGPKeeper/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable server_api"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/server_api.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/server_api.dir/build: server_api
.PHONY : CMakeFiles/server_api.dir/build

CMakeFiles/server_api.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/server_api.dir/cmake_clean.cmake
.PHONY : CMakeFiles/server_api.dir/clean

CMakeFiles/server_api.dir/depend:
	cd /home/datta/Documents/SEM-8/KGPKeeper/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/datta/Documents/SEM-8/KGPKeeper /home/datta/Documents/SEM-8/KGPKeeper /home/datta/Documents/SEM-8/KGPKeeper/build /home/datta/Documents/SEM-8/KGPKeeper/build /home/datta/Documents/SEM-8/KGPKeeper/build/CMakeFiles/server_api.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/server_api.dir/depend

