# naomi-kit for a KallistiOS Makefile (KOS_SUBARCH=naomi). In the game's Makefile:
#
#   NAOMI_KIT := path/to/naomi-kit
#   include $(NAOMI_KIT)/kit.mk
#   OBJS += $(NAOMI_KIT_OBJS)
#   CFLAGS += $(NAOMI_KIT_CFLAGS)       (or KOS_CFLAGS, as the Makefile builds its objects)
#
# The objects are built next to their sources (naomi-kit/src/*.o) by KallistiOS's Makefile.rules.

NAOMI_KIT_SOURCES := $(wildcard $(NAOMI_KIT)/src/nk_*.c)
NAOMI_KIT_OBJS := $(NAOMI_KIT_SOURCES:.c=.o)
NAOMI_KIT_CFLAGS := -I$(NAOMI_KIT)/include
