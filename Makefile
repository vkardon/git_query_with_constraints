
# Target(s) to build
TARGET_TESTAPP = app

# Sources
PROJECT_HOME = .
SRC_DIR = $(PROJECT_HOME)/.
OBJ_DIR = $(PROJECT_HOME)/_obj

SRCS_TESTAPP = $(SRC_DIR)/app.cpp \
               $(SRC_DIR)/constraints.cpp

# Detect operating system
OS = $(shell uname -s)

# Include directories
INCS = 

# Libraries
ifeq "$(OS)" "Darwin"
  LIBS = -ldl
else
  LIBS = -ldl -lrt
endif

# Objective files to build
OBJS_LIB     = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(basename $(notdir $(SRCS_LIB)))))
OBJS_TESTAPP = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(basename $(notdir $(SRCS_TESTAPP)))))

# Compiler and linker to use
ifeq "$(OS)" "Linux"
  CC = /usr/local/geneva/packages/devtoolset-7/root/usr/bin/g++
  CC_LIB_PATH = $(LD_LIBRARY_PATH):/usr/local-rhel7/geneva/packages/devtoolset-7/root/lib64
else
  CC = g++
  CC_LIB_PATH = $(LD_LIBRARY_PATH)
endif

CFLAGS = -std=c++1z -Wall -pthread
LD = $(CC)
LDFLAGS = -pthread

#CFLAGS += -g
CFLAGS += -D NDEBUG

# Build target(s)
all: $(TARGET_TESTAPP)

$(TARGET_TESTAPP): $(OBJS_TESTAPP)
	export LD_LIBRARY_PATH=$(CC_LIB_PATH); $(LD) $(LDFLAGS) -o $(TARGET_TESTAPP) $(OBJS_TESTAPP) $(LIBS)

# Compile source files
# Add -MP to generate dependency list
# Add -MMD to not include system headers
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile   
	-mkdir -p $(OBJ_DIR)
	export LD_LIBRARY_PATH=$(CC_LIB_PATH); $(CC) -c -MP -MMD $(CFLAGS) $(INCS) -o $(OBJ_DIR)/$*.o $<
	
# Delete all intermediate files
clean: 
#	@echo OBJS_TESTAPP = $(OBJS_TESTAPP)
	rm -rf $(TARGET_TESTAPP) $(OBJ_DIR) 

#
# Read the dependency files.
# Note: use '-' prefix to don't display error or warning
# if include file do not exist (just remade it)
#
-include $(OBJS_TESTAPP:.o=.d)


