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
CMAKE_SOURCE_DIR = /home/mayukha/Documents/DS/KGPKeeper

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/mayukha/Documents/DS/KGPKeeper/build

# Include any dependencies generated for this target.
include CMakeFiles/client.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/client.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/client.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/client.dir/flags.make

CMakeFiles/client.dir/src/client.cpp.o: CMakeFiles/client.dir/flags.make
CMakeFiles/client.dir/src/client.cpp.o: ../src/client.cpp
CMakeFiles/client.dir/src/client.cpp.o: CMakeFiles/client.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mayukha/Documents/DS/KGPKeeper/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/client.dir/src/client.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/client.dir/src/client.cpp.o -MF CMakeFiles/client.dir/src/client.cpp.o.d -o CMakeFiles/client.dir/src/client.cpp.o -c /home/mayukha/Documents/DS/KGPKeeper/src/client.cpp

CMakeFiles/client.dir/src/client.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/client.dir/src/client.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/mayukha/Documents/DS/KGPKeeper/src/client.cpp > CMakeFiles/client.dir/src/client.cpp.i

CMakeFiles/client.dir/src/client.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/client.dir/src/client.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/mayukha/Documents/DS/KGPKeeper/src/client.cpp -o CMakeFiles/client.dir/src/client.cpp.s

CMakeFiles/client.dir/src/utils/assistantUtils.cpp.o: CMakeFiles/client.dir/flags.make
CMakeFiles/client.dir/src/utils/assistantUtils.cpp.o: ../src/utils/assistantUtils.cpp
CMakeFiles/client.dir/src/utils/assistantUtils.cpp.o: CMakeFiles/client.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mayukha/Documents/DS/KGPKeeper/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/client.dir/src/utils/assistantUtils.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/client.dir/src/utils/assistantUtils.cpp.o -MF CMakeFiles/client.dir/src/utils/assistantUtils.cpp.o.d -o CMakeFiles/client.dir/src/utils/assistantUtils.cpp.o -c /home/mayukha/Documents/DS/KGPKeeper/src/utils/assistantUtils.cpp

CMakeFiles/client.dir/src/utils/assistantUtils.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/client.dir/src/utils/assistantUtils.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/mayukha/Documents/DS/KGPKeeper/src/utils/assistantUtils.cpp > CMakeFiles/client.dir/src/utils/assistantUtils.cpp.i

CMakeFiles/client.dir/src/utils/assistantUtils.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/client.dir/src/utils/assistantUtils.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/mayukha/Documents/DS/KGPKeeper/src/utils/assistantUtils.cpp -o CMakeFiles/client.dir/src/utils/assistantUtils.cpp.s

CMakeFiles/client.dir/src/utils/leaderElection.cpp.o: CMakeFiles/client.dir/flags.make
CMakeFiles/client.dir/src/utils/leaderElection.cpp.o: ../src/utils/leaderElection.cpp
CMakeFiles/client.dir/src/utils/leaderElection.cpp.o: CMakeFiles/client.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mayukha/Documents/DS/KGPKeeper/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/client.dir/src/utils/leaderElection.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/client.dir/src/utils/leaderElection.cpp.o -MF CMakeFiles/client.dir/src/utils/leaderElection.cpp.o.d -o CMakeFiles/client.dir/src/utils/leaderElection.cpp.o -c /home/mayukha/Documents/DS/KGPKeeper/src/utils/leaderElection.cpp

CMakeFiles/client.dir/src/utils/leaderElection.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/client.dir/src/utils/leaderElection.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/mayukha/Documents/DS/KGPKeeper/src/utils/leaderElection.cpp > CMakeFiles/client.dir/src/utils/leaderElection.cpp.i

CMakeFiles/client.dir/src/utils/leaderElection.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/client.dir/src/utils/leaderElection.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/mayukha/Documents/DS/KGPKeeper/src/utils/leaderElection.cpp -o CMakeFiles/client.dir/src/utils/leaderElection.cpp.s

# Object files for target client
client_OBJECTS = \
"CMakeFiles/client.dir/src/client.cpp.o" \
"CMakeFiles/client.dir/src/utils/assistantUtils.cpp.o" \
"CMakeFiles/client.dir/src/utils/leaderElection.cpp.o"

# External object files for target client
client_EXTERNAL_OBJECTS =

client: CMakeFiles/client.dir/src/client.cpp.o
client: CMakeFiles/client.dir/src/utils/assistantUtils.cpp.o
client: CMakeFiles/client.dir/src/utils/leaderElection.cpp.o
client: CMakeFiles/client.dir/build.make
client: CMakeFiles/client.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/mayukha/Documents/DS/KGPKeeper/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking CXX executable client"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/client.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/client.dir/build: client
.PHONY : CMakeFiles/client.dir/build

CMakeFiles/client.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/client.dir/cmake_clean.cmake
.PHONY : CMakeFiles/client.dir/clean

CMakeFiles/client.dir/depend:
	cd /home/mayukha/Documents/DS/KGPKeeper/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/mayukha/Documents/DS/KGPKeeper /home/mayukha/Documents/DS/KGPKeeper /home/mayukha/Documents/DS/KGPKeeper/build /home/mayukha/Documents/DS/KGPKeeper/build /home/mayukha/Documents/DS/KGPKeeper/build/CMakeFiles/client.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/client.dir/depend

