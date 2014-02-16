# Compiler Options
CXX = clang++ -m64 -stdlib=libc++
LANG_FLAGS = -std=c++11 -fstack-protector -g -O0
WARN_FLAGS = -Werror -Wfatal-errors -Wall -Wextra -Wstrict-aliasing -Wstrict-overflow=5 -Wcast-align -Wmissing-declarations -Wpointer-arith -Wcast-qual -Wold-style-cast
INCLUDE_DIRS = `botan-config-1.11 --cflags`
LIBS = `botan-config-1.11 --libs`

# Version Numbers
VERSION = 0.1.0
BRANCH = 0.1

OBJDIR = build

# Program aliases
AR = ar cr
COPY = cp
COPY_R = cp -r
CD = @cd
ECHO = @echo
LN = ln -fs
MKDIR = @mkdir
RANLIB = ranlib
RM = @rm -f
RM_R = @rm -rf

# Targets
APP = pivotal

all: $(APP)

# File Lists

OBJS = build/connection.o \
	   build/connection_manager.o \
	   build/request_forwarder_boost.o \
	   build/request_parser.o \
	   build/response.o \
	   build/server.o \
	   build/main_nodll.o

# Build Commands

build/%.o: ./source/%.cpp
	$(CXX) $(LANG_FLAGS) $(WARN_FLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(OBJDIR):
	$(MKDIR) $(OBJDIR)

$(APP): $(OBJDIR) $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) $(LIBS) -o $(APP)


# Fake Targets
.PHONY = clean

clean:
	$(RM_R) $(OBJDIR)
	$(RM) $(APP)
