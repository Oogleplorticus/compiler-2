#######################
# BUILD MODE AND TYPE #
#######################

#build files of different build modes are seperate
#a release build will not overwrite a debug build but instead be built in a seperate directory

#"debug" for a debug build
#"release" for a release build (-o2, as is the default)
#"small" for a release build optimised for file size (-os)
#"fast" for a release build with more agressive compiler optimisations (-o3)
#"fastest" for a release build with very agressive compiler optimisations (-ofast), this can cause errors so be careful
BUILD_MODE ?= debug

#"executable" for an executable file
#"static" for a static library
#"shared" for a shared library
BUILD_TYPE ?= executable

#confirm that build mode and type are valid
VALID_BUILD_MODES := debug release small fast fastest
VALID_BUILD_TYPES := executable static shared
ifeq ($(filter $(BUILD_MODE),$(VALID_BUILD_MODES)),)
    $(error Build aborted: Invalid build mode "$(BUILD_MODE)". Valid build modes: $(VALID_BUILD_MODES))
endif
ifeq ($(filter $(BUILD_TYPE),$(VALID_BUILD_TYPES)),)
    $(error Build aborted: Invalid build type "$(BUILD_TYPE)". Valid build types: $(VALID_BUILD_TYPES))
endif

#####################
# COLOUR SHORTHANDS #
#####################

#No Colour, used to reset after the other colours are used
NC :=\033[0m

#Usage based colours
error_colour					:=\033[1;31m#Red, for errors
warning_colour					:=\033[0;33m#Yellow, for warnings
info_colour						:=\033[0;34m#Blue, for info messages
highlight_info_colour			:=\033[1;94m#Blue, for info messages
bright_info_colour				:=\033[0;36m#Cyan, for info messages
highlight_bright_info_colour	:=\033[1;96m#Cyan, for info messages
success_colour					:=\033[0;32m#Green, for success messages

##########################
# COMPILERS AND ARCHIVER #
##########################

CXX := g++
CC := gcc

AR := ar

###############
# DIRECTORIES #
###############

BINARY_DIRECTORY := ./bin
SOURCE_DIRECTORY := ./src
OBJECT_DIRECTORY := ./obj
DEPENDENCY_DIRECTORY := ./.deps

################
# BUILD TARGET #
################

#filename only, will be stored in the binary directory
#do not include the file extension
BUILD_FILENAME ?= build

#determine correct file extension
EXECUTABLE_FILE_EXTENSION :=
STATIC_LIBRARY_FILE_EXTENSION := .a
SHARED_LIBRARY_FILE_EXTENSION := .so

ifeq ($(BUILD_TYPE), executable)
BUILD_FILE_EXTENSION := $(EXECUTABLE_FILE_EXTENSION)
else ifeq ($(BUILD_TYPE), static)
BUILD_FILE_EXTENSION := $(STATIC_LIBRARY_FILE_EXTENSION)
else ifeq ($(BUILD_TYPE), shared)
BUILD_FILE_EXTENSION := $(SHARED_LIBRARY_FILE_EXTENSION)
endif

#setup the build target
BUILD_TARGET_NO_EXTENSION := $(BINARY_DIRECTORY)/$(BUILD_MODE)/$(BUILD_FILENAME)
BUILD_TARGET := $(BUILD_TARGET_NO_EXTENSION)$(BUILD_FILE_EXTENSION)
.DEFAULT_GOAL := $(BUILD_TARGET)

##############################
# GENERATED MAIN FILE CONFIG #
##############################

#skip if not running setup
ifeq ($(filter setup,$(MAKECMDGOALS)),)
#do nothing
else
#make sure its .c or .cpp, preferably named main
GENERATED_MAIN_FILENAME ?= main.cpp

GENERATED_MAIN_FILE := $(SOURCE_DIRECTORY)/$(GENERATED_MAIN_FILENAME)
GENERATED_MAIN_FILE_CONTENTS := int main() {\n	return 0;\n}
endif

###################################
# TARGET AND DEPENDENCY VARIABLES #
###################################

#skip variable assignment for unnecessary targets
#also prevents an error message from the $(shell find) when runing "make setup" because the source directory would not exist at that time
ifeq ($(filter setup clean,$(MAKECMDGOALS)),)
SOURCES = $(shell find $(SOURCE_DIRECTORY) -name '*.cpp' -or -name '*.c')

CPP_SOURCES := $(filter %.cpp, $(SOURCES))
C_SOURCES   := $(filter %.c,   $(SOURCES))

OBJECTS := $(patsubst $(SOURCE_DIRECTORY)/%.cpp, $(OBJECT_DIRECTORY)/$(BUILD_MODE)/%.cpp.o, $(CPP_SOURCES)) \
		   $(patsubst $(SOURCE_DIRECTORY)/%.c,   $(OBJECT_DIRECTORY)/$(BUILD_MODE)/%.c.o,   $(C_SOURCES))

DEPENDENCY_FILES := $(patsubst $(OBJECT_DIRECTORY)/%.o, $(DEPENDENCY_DIRECTORY)/%.d, $(OBJECTS))
endif

#########
# FLAGS #
#########

CXXFLAGS ?=
CFLAGS ?=
CPPFLAGS ?=
LDFLAGS ?=

#always add flags for dependency generation, this makefile wouldnt work properly without them
#since .cpp and .c are not part of the stem to differentiate the targets, this requires some string substitution to get the file name correct
DEPENDENCY_FLAGS = -MMD -MP -MT $@ -MF $(DEPENDENCY_DIRECTORY)/$(BUILD_MODE)/$*$(suffix $<).d
CPPFLAGS += $(DEPENDENCY_FLAGS)

#define build mode flags
DEBUG_BUILD_COMPILER_FLAGS = -g -O0 -Wall -Wextra
RELEASE_BUILD_COMPILER_FLAGS = -O2 -DNDEBUG
SMALL_BUILD_COMPILER_FLAGS = -Os -DNDEBUG
FAST_BUILD_COMPILER_FLAGS = -O3 -DNDEBUG
FASTEST_BUILD_COMPILER_FLAGS = -Ofast -DNDEBUG

#handle build mode flags
ifeq ($(BUILD_MODE), debug)
CXXFLAGS += $(DEBUG_BUILD_COMPILER_FLAGS)
CFLAGS += $(DEBUG_BUILD_COMPILER_FLAGS)
else ifeq ($(BUILD_MODE), release)
CXXFLAGS += $(RELEASE_BUILD_COMPILER_FLAGS)
CFLAGS += $(RELEASE_BUILD_COMPILER_FLAGS)
else ifeq ($(BUILD_MODE), small)
CXXFLAGS += $(SMALL_BUILD_COMPILER_FLAGS)
CFLAGS += $(SMALL_BUILD_COMPILER_FLAGS)
else ifeq ($(BUILD_MODE), fast)
CXXFLAGS += $(FAST_BUILD_COMPILER_FLAGS)
CFLAGS += $(FAST_BUILD_COMPILER_FLAGS)
else ifeq ($(BUILD_MODE), fastest)
CXXFLAGS += $(FASTEST_BUILD_COMPILER_FLAGS)
CFLAGS += $(FASTEST_BUILD_COMPILER_FLAGS)
endif

#define build type flags
EXECUTABLE_COMPILER_FLAGS =
STATIC_LIBRARY_COMPILER_FLAGS =
SHARED_LIBRARY_COMPILER_FLAGS = -fPIC

ifeq ($(BUILD_TYPE), executable)
CXXFLAGS += $(EXECUTABLE_COMPILER_FLAGS)
CFLAGS += $(EXECUTABLE_COMPILER_FLAGS)
else ifeq ($(BUILD_TYPE), static)
CXXFLAGS += $(STATIC_LIBRARY_COMPILER_FLAGS)
CFLAGS += $(STATIC_LIBRARY_COMPILER_FLAGS)
else ifeq ($(BUILD_TYPE), shared)
CXXFLAGS += $(SHARED_LIBRARY_COMPILER_FLAGS)
CFLAGS += $(SHARED_LIBRARY_COMPILER_FLAGS)
endif

###########
# TARGETS #
###########

#"all" target for convention, and as a backup for .DEFAULT_GOAL
all: $(BUILD_TARGET)

#make required directories if they do not already exist
REQUIRED_DIRECTORIES = $(sort $(dir $(BUILD_TARGET)) $(dir $(OBJECTS)) $(dir $(DEPENDENCY_FILES)))
$(REQUIRED_DIRECTORIES):
	@printf "$(bright_info_colour)Creating required directory: $(highlight_bright_info_colour)$@$(NC)\n"
	mkdir -p $@

#final compile and link for executable
$(BUILD_TARGET_NO_EXTENSION)$(EXECUTABLE_FILE_EXTENSION): $(OBJECTS) | $(REQUIRED_DIRECTORIES)
	@printf "$(info_colour)Linking object files: $(highlight_info_colour)$(OBJECTS)$(NC)\n"
	$(CXX) $(LDFLAGS) $^ -o $@
	@printf "$(success_colour)Build completed successfully!$(NC)\n"

$(BUILD_TARGET_NO_EXTENSION)$(STATIC_LIBRARY_FILE_EXTENSION): $(OBJECTS) | $(REQUIRED_DIRECTORIES)
	@printf "$(info_colour)Creating static library from object files: $(highlight_info_colour)$(OBJECTS)$(NC)\n"
	$(AR) rcs $@ $^
	@printf "$(success_colour)Build completed successfully!$(NC)\n"

$(BUILD_TARGET_NO_EXTENSION)$(SHARED_LIBRARY_FILE_EXTENSION): $(OBJECTS) | $(REQUIRED_DIRECTORIES)
	@printf "$(info_colour)Creating shared library from object files: $(highlight_info_colour)$(OBJECTS)$(NC)\n"
	$(CXX) -shared $(LDFLAGS) $^ -o $@

#cancel implicit rule for compiling c++ files
%.o : %.cpp
#build all .cpp files into .o files with autogenerated dependencies
$(OBJECT_DIRECTORY)/$(BUILD_MODE)/%.cpp.o: $(SOURCE_DIRECTORY)/%.cpp $(DEPENDENCY_DIRECTORY)/$(BUILD_MODE)/%.cpp.d | $(REQUIRED_DIRECTORIES)
	@printf "$(info_colour)Building object file: $(highlight_info_colour)$@$(info_colour) From c++ file: $(highlight_info_colour)$<$(NC)\n"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

#cancel implicit rule for compiling c files
%.o : %.c
#build all .c files into .o files with autogenerated dependencies
$(OBJECT_DIRECTORY)/$(BUILD_MODE)/%.c.o: $(SOURCE_DIRECTORY)/%.c $(DEPENDENCY_DIRECTORY)/$(BUILD_MODE)/%.c.d | $(REQUIRED_DIRECTORIES)
	@printf "$(info_colour)Building object file: $(highlight_info_colour)$@$(info_colour) From c file: $(highlight_info_colour)$<$(NC)\n"
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

#target for each dependency file so make wont fail if they dont exist
$(DEPENDENCY_FILES):

#removes build files, including the build target
.PHONY: clean
clean:
	@printf "$(bright_info_colour)Cleaning $(highlight_bright_info_colour)$(OBJECT_DIRECTORY)$(bright_info_colour) and $(highlight_bright_info_colour)$(DEPENDENCY_DIRECTORY)$(NC)\n"
	$(RM) -r $(OBJECT_DIRECTORY) $(DEPENDENCY_DIRECTORY)
	@printf "$(bright_info_colour)Cleaning build directories in $(highlight_bright_info_colour)$(BINARY_DIRECTORY)$(NC)\n"
	$(foreach dir,$(VALID_BUILD_MODES), $(RM) -r $(BINARY_DIRECTORY)/$(dir);)

#creates initial directories and a main file
.PHONY: setup
setup:
	@printf "$(bright_info_colour)Creating directories: $(highlight_bright_info_colour)$(SOURCE_DIRECTORY)$(bright_info_colour) and $(highlight_bright_info_colour)$(BINARY_DIRECTORY)$(NC)\n"
	mkdir -p $(SOURCE_DIRECTORY) $(BINARY_DIRECTORY)
	@printf "$(bright_info_colour)Creating main file: $(highlight_bright_info_colour)$(GENERATED_MAIN_FILE)$(NC)\n"
	touch $(GENERATED_MAIN_FILE)
	printf "$(GENERATED_MAIN_FILE_CONTENTS)" > $(GENERATED_MAIN_FILE)

.PHONY: count_lines
count_lines:
	find ./src -exec wc -l {} \; | grep -o "[0-9][0-9]*" | paste -sd+ | bc

############
# INCLUDES #
############

-include $(DEPENDENCY_FILES)
