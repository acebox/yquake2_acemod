# ----------------------------------------------------- #
# Makefile for the acemod game module for Quake II      #
# (based on Zaero Makefile used for reference)			#
#                                                       #
# Just type "make" to compile the                       #
#  - Quake II: Acemod Test (game.so / game.dll)         #
#                                                       #
# Dependencies:                                         #
# - Quake II Client already configured.                 #
#   (in theory every one should work).					#
#														#
# Tested on: Yamagi Quake II "AceMod"					#
# basically stock w/ effect tweaks (physics/etc..)      #
# running xUbuntu 14.04 LTS (Kernel 3.13.0 x86_64).		#
#                                                       #
# Version: Pre-Alpha TEST #1 2-10-2015					#
#                                                       #
# Platforms:                                            #
# - Linux                                               #
# - FreeBSD                                             #
# ----------------------------------------------------- #

# Detect the OS
ifdef SystemRoot
OSTYPE := Windows
else
OSTYPE := $(shell uname -s)
endif

# Detect the architecture
ifeq ($(OSTYPE), Windows)
# At this time only i386 is supported on Windows
ARCH := i386
else
# Some platforms call it "amd64" and some "x86_64"
ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/amd64/x86_64/)
endif

# Refuse all other platforms as a firewall against PEBKAC
# (You'll need some #ifdef for your unsupported  plattform!)
ifeq ($(findstring $(ARCH), i386 x86_64 sparc64 ia64),)
$(error arch $(ARCH) is currently not supported)
endif

# ----------

# Base CFLAGS.
#
# -O2 are enough optimizations.
#
# -fno-strict-aliasing since the source doesn't comply
#  with strict aliasing rules and it's next to impossible
#  to get it there...
#
# -fomit-frame-pointer since the framepointer is mostly
#  useless for debugging Quake II and slows things down.
#
# -g to build allways with debug symbols. Please do not
#  change this, since it's our only chance to debug this
#  crap when random crashes happen!
#
# -fPIC for position independend code.
#
# -MMD to generate header dependencies.
ifeq ($(OSTYPE), Darwin)
CFLAGS := -O2 -fno-strict-aliasing -fomit-frame-pointer \
		  -Wall -pipe -g -arch i386 -arch x86_64
else
CFLAGS := -O2 -fno-strict-aliasing -fomit-frame-pointer \
		  -Wall -pipe -g -MMD
endif

# ----------

# Base LDFLAGS.
ifeq ($(OSTYPE), Darwin)
LDFLAGS := -shared -arch i386 -arch x86_64
else
LDFLAGS := -shared
endif

# ----------

# Builds everything
all: acemod

# ----------

# When make is invoked by "make VERBOSE=1" print
# the compiler and linker commands.

ifdef VERBOSE
Q :=
else
Q := @
endif

# ----------

# Phony targets
.PHONY : all clean acemod

# ----------

# Cleanup
ifeq ($(OSTYPE), Windows)
clean:
	@echo "===> CLEAN"
	@-rmdir /S /Q acemod build
else
clean:
	@echo "===> CLEAN"
	${Q}rm -Rf build acemod
endif

# ----------

# The acemod game
ifeq ($(OSTYPE), Windows)
acemod:
	@echo "===> Building game.dll"
	${Q}tools/mkdir.exe -p acemod
	${MAKE} acemod/game.dll

build/%.o: %.c
	@echo "===> CC $<"
	${Q}tools/mkdir.exe -p $(@D)
	${Q}$(CC) -c $(CFLAGS) -o $@ $<
else
acemod:
	@echo "===> Building game.so"
	${Q}mkdir -p acemod
	$(MAKE) acemod/game.so

build/%.o: %.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	${Q}$(CC) -c $(CFLAGS) -o $@ $<

acemod/game.so : CFLAGS += -fPIC
endif 

# ----------

ACEMOD_OBJS_ = \
	src/common/shared/flash.o \
	src/common/shared/rand.o \
	src/common/shared/shared.o \
	src/game/g_ai.o \
	src/game/g_chase.o \
	src/game/g_cmds.o \
	src/game/g_combat.o \
	src/game/g_func.o \
	src/game/g_items.o \
	src/game/g_main.o \
	src/game/g_misc.o \
	src/game/g_monster.o \
	src/game/g_phys.o \
	src/game/g_spawn.o \
	src/game/g_svcmds.o \
	src/game/g_target.o \
	src/game/g_trigger.o \
	src/game/g_turret.o \
	src/game/g_utils.o \
	src/game/g_weapon.o \
	src/game/monster/berserker/berserker.o \
	src/game/monster/boss2/boss2.o \
	src/game/monster/boss3/boss3.o \
	src/game/monster/boss3/boss31.o \
	src/game/monster/boss3/boss32.o \
	src/game/monster/zboss/zboss.o \
	src/game/monster/brain/brain.o \
	src/game/monster/chick/chick.o \
	src/game/monster/flipper/flipper.o \
	src/game/monster/float/float.o \
	src/game/monster/flyer/flyer.o \
	src/game/monster/gladiator/gladiator.o \
	src/game/monster/gladiator/gladb.o \
	src/game/monster/gunner/gunner.o \
	src/game/monster/hover/hover.o \
	src/game/monster/infantry/infantry.o \
	src/game/monster/handler/handler.o \
	src/game/monster/hound/hound.o \
	src/game/monster/insane/insane.o \
	src/game/monster/medic/medic.o \
	src/game/monster/misc/move.o \
	src/game/monster/mutant/mutant.o \
	src/game/monster/parasite/parasite.o \
	src/game/monster/soldier/soldier.o \
	src/game/monster/supertank/supertank.o \
	src/game/monster/tank/tank.o \
	src/game/monster/boss5/boss5.o \
	src/game/monster/carrier/carrier.o \
	src/game/monster/fixbot/fixbot.o \
	src/game/monster/gekk/gekk.o \
	src/game/monster/stalker/stalker.o \
	src/game/monster/turret/turret.o \
	src/game/monster/widow/widow.o \
	src/game/monster/widow/widow2.o \
	src/game/player/client.o \
	src/game/player/hud.o \
	src/game/player/trail.o \
	src/game/player/view.o \
	src/game/player/weapon.o \
	src/game/savegame/savegame.o \
	src/game/acemod/acannon.o \
	src/game/acemod/a_ai.o \
	src/game/acemod/a_monsters.o \
	src/game/acemod/a_misc.o \
	src/game/acemod/a_spawn.o \
	src/game/acemod/a_items.o \
	src/game/acemod/a_weapons.o

# ----------

# Rewrite pathes to our object directory
ACEMOD_OBJS = $(patsubst %,build/%,$(ACEMOD_OBJS_))

# ----------

# Generate header dependencies
ACEMOD_DEPS= $(ACEMOD_OBJS:.o=.d)

# ----------

# Suck header dependencies in
-include $(ACEMOD_DEPS)

# ----------

ifeq ($(OSTYPE), Windows)
acemod/game.dll : $(ACEMOD_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) -o $@ $(ACEMOD_OBJS)
else
acemod/game.so : $(ACEMOD_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) -o $@ $(ACEMOD_OBJS)
endif

# ----------