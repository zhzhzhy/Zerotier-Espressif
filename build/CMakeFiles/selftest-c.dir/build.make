# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.23

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
CMAKE_COMMAND = /home/zzy/.espressif/tools/cmake/3.23.1/bin/cmake

# The command to remove a file.
RM = /home/zzy/.espressif/tools/cmake/3.23.1/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/zzy/PROJ/libzt

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zzy/PROJ/libzt/build

# Include any dependencies generated for this target.
include CMakeFiles/selftest-c.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/selftest-c.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/selftest-c.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/selftest-c.dir/flags.make

CMakeFiles/selftest-c.dir/test/selftest.c.obj: CMakeFiles/selftest-c.dir/flags.make
CMakeFiles/selftest-c.dir/test/selftest.c.obj: ../test/selftest.c
CMakeFiles/selftest-c.dir/test/selftest.c.obj: CMakeFiles/selftest-c.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zzy/PROJ/libzt/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/selftest-c.dir/test/selftest.c.obj"
	/home/zzy/.espressif/tools/xtensa-esp32-elf/esp-2021r2-patch3-8.4.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/selftest-c.dir/test/selftest.c.obj -MF CMakeFiles/selftest-c.dir/test/selftest.c.obj.d -o CMakeFiles/selftest-c.dir/test/selftest.c.obj -c /home/zzy/PROJ/libzt/test/selftest.c

CMakeFiles/selftest-c.dir/test/selftest.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/selftest-c.dir/test/selftest.c.i"
	/home/zzy/.espressif/tools/xtensa-esp32-elf/esp-2021r2-patch3-8.4.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/zzy/PROJ/libzt/test/selftest.c > CMakeFiles/selftest-c.dir/test/selftest.c.i

CMakeFiles/selftest-c.dir/test/selftest.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/selftest-c.dir/test/selftest.c.s"
	/home/zzy/.espressif/tools/xtensa-esp32-elf/esp-2021r2-patch3-8.4.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/zzy/PROJ/libzt/test/selftest.c -o CMakeFiles/selftest-c.dir/test/selftest.c.s

# Object files for target selftest-c
selftest__c_OBJECTS = \
"CMakeFiles/selftest-c.dir/test/selftest.c.obj"

# External object files for target selftest-c
selftest__c_EXTERNAL_OBJECTS =

bin/selftest-c: CMakeFiles/selftest-c.dir/test/selftest.c.obj
bin/selftest-c: CMakeFiles/selftest-c.dir/build.make
bin/selftest-c: lib/libzt.a
bin/selftest-c: CMakeFiles/selftest-c.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zzy/PROJ/libzt/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable bin/selftest-c"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/selftest-c.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/selftest-c.dir/build: bin/selftest-c
.PHONY : CMakeFiles/selftest-c.dir/build

CMakeFiles/selftest-c.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/selftest-c.dir/cmake_clean.cmake
.PHONY : CMakeFiles/selftest-c.dir/clean

CMakeFiles/selftest-c.dir/depend:
	cd /home/zzy/PROJ/libzt/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zzy/PROJ/libzt /home/zzy/PROJ/libzt /home/zzy/PROJ/libzt/build /home/zzy/PROJ/libzt/build /home/zzy/PROJ/libzt/build/CMakeFiles/selftest-c.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/selftest-c.dir/depend

