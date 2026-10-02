CC      = gcc
CFLAGS  = -Wall -Wextra -g

# sphragis.o is a Linux ELF — island only builds on Linux
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
    TARGETS = ithaca odysseus island
    ISLAND_LDFLAGS = lib/sphragis.o
else
    TARGETS = ithaca odysseus
    ISLAND_LDFLAGS =
endif

ITHACA_SRC   = ithaca.c ithacaConfig.c io.c interrupt.c
ODYSSEUS_SRC = odysseus.c odysseusConfig.c io.c interrupt.c
ISLAND_SRC   = island.c islandConfig.c io.c interrupt.c

ITHACA_OBJ   = $(ITHACA_SRC:.c=.o)
ODYSSEUS_OBJ = $(ODYSSEUS_SRC:.c=.o)
ISLAND_OBJ   = $(ISLAND_SRC:.c=.o)

ALL_OBJ = $(ITHACA_OBJ) $(ODYSSEUS_OBJ) $(ISLAND_OBJ)

.PHONY: all clean run

all: $(TARGETS)

ithaca: $(ITHACA_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

odysseus: $(ODYSSEUS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

island: $(ISLAND_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(ISLAND_LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -I. -Ilib -c $< -o $@

run: all
	./ithaca data/configs/ithaca.dat data/voyages.dat &
	./island data/configs/island.dat data/island_stock/Aeaea.db &
	./odysseus data/configs/odysseus.dat

clean:
	rm -f $(ALL_OBJ) ithaca odysseus island
