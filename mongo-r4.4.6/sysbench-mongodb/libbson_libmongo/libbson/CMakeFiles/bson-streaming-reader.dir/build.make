# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.14

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /root/yyz/mongo-c-driver-1.11.0

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/yyz/mongo-c-driver-1.11.0/cmake-build

# Include any dependencies generated for this target.
include src/libbson/CMakeFiles/bson-streaming-reader.dir/depend.make

# Include the progress variables for this target.
include src/libbson/CMakeFiles/bson-streaming-reader.dir/progress.make

# Include the compile flags for this target's objects.
include src/libbson/CMakeFiles/bson-streaming-reader.dir/flags.make

src/libbson/CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.o: src/libbson/CMakeFiles/bson-streaming-reader.dir/flags.make
src/libbson/CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.o: ../src/libbson/examples/bson-streaming-reader.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/yyz/mongo-c-driver-1.11.0/cmake-build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/libbson/CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.o"
	cd /root/yyz/mongo-c-driver-1.11.0/cmake-build/src/libbson && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.o   -c /root/yyz/mongo-c-driver-1.11.0/src/libbson/examples/bson-streaming-reader.c

src/libbson/CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.i"
	cd /root/yyz/mongo-c-driver-1.11.0/cmake-build/src/libbson && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /root/yyz/mongo-c-driver-1.11.0/src/libbson/examples/bson-streaming-reader.c > CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.i

src/libbson/CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.s"
	cd /root/yyz/mongo-c-driver-1.11.0/cmake-build/src/libbson && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /root/yyz/mongo-c-driver-1.11.0/src/libbson/examples/bson-streaming-reader.c -o CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.s

# Object files for target bson-streaming-reader
bson__streaming__reader_OBJECTS = \
"CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.o"

# External object files for target bson-streaming-reader
bson__streaming__reader_EXTERNAL_OBJECTS =

src/libbson/bson-streaming-reader: src/libbson/CMakeFiles/bson-streaming-reader.dir/examples/bson-streaming-reader.c.o
src/libbson/bson-streaming-reader: src/libbson/CMakeFiles/bson-streaming-reader.dir/build.make
src/libbson/bson-streaming-reader: src/libbson/libbson-1.0.so.0.0.0
src/libbson/bson-streaming-reader: /usr/lib64/librt.so
src/libbson/bson-streaming-reader: /usr/lib64/libm.so
src/libbson/bson-streaming-reader: src/libbson/CMakeFiles/bson-streaming-reader.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/root/yyz/mongo-c-driver-1.11.0/cmake-build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable bson-streaming-reader"
	cd /root/yyz/mongo-c-driver-1.11.0/cmake-build/src/libbson && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/bson-streaming-reader.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/libbson/CMakeFiles/bson-streaming-reader.dir/build: src/libbson/bson-streaming-reader

.PHONY : src/libbson/CMakeFiles/bson-streaming-reader.dir/build

src/libbson/CMakeFiles/bson-streaming-reader.dir/clean:
	cd /root/yyz/mongo-c-driver-1.11.0/cmake-build/src/libbson && $(CMAKE_COMMAND) -P CMakeFiles/bson-streaming-reader.dir/cmake_clean.cmake
.PHONY : src/libbson/CMakeFiles/bson-streaming-reader.dir/clean

src/libbson/CMakeFiles/bson-streaming-reader.dir/depend:
	cd /root/yyz/mongo-c-driver-1.11.0/cmake-build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/yyz/mongo-c-driver-1.11.0 /root/yyz/mongo-c-driver-1.11.0/src/libbson /root/yyz/mongo-c-driver-1.11.0/cmake-build /root/yyz/mongo-c-driver-1.11.0/cmake-build/src/libbson /root/yyz/mongo-c-driver-1.11.0/cmake-build/src/libbson/CMakeFiles/bson-streaming-reader.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/libbson/CMakeFiles/bson-streaming-reader.dir/depend

