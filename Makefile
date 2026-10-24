CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean run

all: ithaca odysseus island

ithaca: ithaca.o ithacaConfig.o io.o
odysseus: odysseus.o odysseusConfig.o io.o terminal.o
island: island.o islandConfig.o io.o lib/sphragis.o

run: all
	./ithaca data/configs/ithaca.dat data/voyages.dat &
	./island data/configs/island.dat data/island_stock/Aeaea.db &
	./odysseus data/configs/odysseus.dat

clean:
	rm -f *.o ithaca odysseus island