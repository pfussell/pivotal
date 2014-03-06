# Compiler Options
OS = mingw
SYSTEM = 64
OPTIMIZATION = 0

LANG_FLAGS = -std=c++11 -fstack-protector -g -O$(OPTIMIZATION)
WARN_FLAG = -Wall -Wextra -Wstrict-aliasing -Wstrict-overflow=5 -Wcast-align -Wmissing-declarations -Wpointer-arith -Wcast-qual -Wold-style-cast -Wzero-as-null-pointer-constant
LIBS = $(shell botan-config-1.11 --libs)
INCLUDE_DIRS = $(shell botan-config-1.11 --cflags)

ifeq ($(OS), mingw)
	CXX = g++
	LANG_FLAGS += -D_UNICODE -DUNICODE
	LIBS += -lssp -lws2_32 -lmswsock -lwininet -static -static-libgcc -static-libstdc++
else
	CXX = clang++
endif

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
APP = pivotal.exe

all: $(APP)

# File Lists

OBJS = build/request_forwarder_wininet.o \
	   build/request_parser.o \
	   build/response.o \
	   build/server.o \
	   build/session.o \
	   build/main_nodll.o

# Build Commands

build/%.o: ./source/%.cpp Makefile
	$(CXX) -m$(SYSTEM) $(LANG_FLAGS) $(WARN_FLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(OBJDIR):
	$(MKDIR) $(OBJDIR)

$(APP): $(OBJDIR) $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) $(LIBS) -o $(APP)


# Fake Targets
.PHONY = clean

clean:
	$(RM_R) $(OBJDIR)
	$(RM) $(APP)
