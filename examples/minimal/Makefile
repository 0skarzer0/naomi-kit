# naomi-kit's example: source /opt/toolchains/dc/kos-naomi/environ.sh; make; then
# $(KOS_BASE)/utils/naomibintool/naomibintool build example.bin -t NAOMIKIT -d HOMEBREW -e example.elf
TARGET = example.elf
NAOMI_KIT := ../..
include $(NAOMI_KIT)/kit.mk
OBJS = main.o $(NAOMI_KIT_OBJS)
KOS_CFLAGS += $(NAOMI_KIT_CFLAGS) -Wall -Wextra

all: $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)
