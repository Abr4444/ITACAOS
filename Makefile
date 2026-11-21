#################################################
#
# Nostos System - Makefile
#
# make          builds ithaca, island and odysseus
# make clean    removes the object files and the executables
# make tar      packs the delivery (set GROUP first)
#
#################################################

CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -Isrc/common -Ilib

SRC = src
OBJ = obj

GROUP = G18
PHASE = F3

# Headers that many modules include
COMMON_H = $(SRC)/common/frame.h $(SRC)/common/socket.h $(SRC)/common/io.h $(SRC)/common/transfer.h
ITHACA_H = $(SRC)/ithaca/ithacaServer.h $(SRC)/ithaca/ithacaConfig.h $(SRC)/common/server.h $(COMMON_H) lib/sphragis.h
ISLAND_H = $(SRC)/island/islandServer.h $(SRC)/island/islandConfig.h $(SRC)/common/server.h $(COMMON_H) lib/sphragis.h
SHIP_H = $(SRC)/odysseus/ship.h $(SRC)/odysseus/map.h $(SRC)/odysseus/odysseusConfig.h $(SRC)/odysseus/terminal.h $(COMMON_H)

# Object files of each executable
ITHACA_OBJ = $(OBJ)/ithaca.o $(OBJ)/ithacaServer.o $(OBJ)/ithacaConfig.o $(OBJ)/server.o $(OBJ)/frame.o $(OBJ)/transfer.o $(OBJ)/socket.o $(OBJ)/io.o lib/sphragis.o
ISLAND_OBJ = $(OBJ)/island.o $(OBJ)/islandServer.o $(OBJ)/islandConfig.o $(OBJ)/server.o $(OBJ)/frame.o $(OBJ)/transfer.o $(OBJ)/socket.o $(OBJ)/io.o lib/sphragis.o
ODYSSEUS_OBJ = $(OBJ)/odysseus.o $(OBJ)/ship.o $(OBJ)/map.o $(OBJ)/voyages.o $(OBJ)/sailing.o $(OBJ)/market.o $(OBJ)/delivery.o \
               $(OBJ)/odysseusConfig.o $(OBJ)/terminal.o $(OBJ)/frame.o $(OBJ)/transfer.o $(OBJ)/socket.o $(OBJ)/io.o lib/sphragis.o

.PHONY: all clean tar

all: ithaca island odysseus

#################################################
# Executables
#################################################

ithaca: $(ITHACA_OBJ)
	$(CC) $(CFLAGS) $(ITHACA_OBJ) -o ithaca

island: $(ISLAND_OBJ)
	$(CC) $(CFLAGS) $(ISLAND_OBJ) -o island

odysseus: $(ODYSSEUS_OBJ)
	$(CC) $(CFLAGS) $(ODYSSEUS_OBJ) -o odysseus

#################################################
# Common modules
#################################################

$(OBJ)/io.o: $(SRC)/common/io.c $(SRC)/common/io.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/common/io.c -o $(OBJ)/io.o

$(OBJ)/socket.o: $(SRC)/common/socket.c $(SRC)/common/socket.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/common/socket.c -o $(OBJ)/socket.o

$(OBJ)/server.o: $(SRC)/common/server.c $(SRC)/common/server.h $(SRC)/common/socket.h $(SRC)/common/io.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/common/server.c -o $(OBJ)/server.o

$(OBJ)/frame.o: $(SRC)/common/frame.c $(SRC)/common/frame.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/common/frame.c -o $(OBJ)/frame.o

$(OBJ)/transfer.o: $(SRC)/common/transfer.c $(SRC)/common/transfer.h $(SRC)/common/frame.h $(SRC)/common/io.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/common/transfer.c -o $(OBJ)/transfer.o

#################################################
# Ithaca
#################################################

$(OBJ)/ithaca.o: $(SRC)/ithaca/ithaca.c $(ITHACA_H)
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/ithaca/ithaca.c -o $(OBJ)/ithaca.o

$(OBJ)/ithacaServer.o: $(SRC)/ithaca/ithacaServer.c $(ITHACA_H)
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/ithaca/ithacaServer.c -o $(OBJ)/ithacaServer.o

$(OBJ)/ithacaConfig.o: $(SRC)/ithaca/ithacaConfig.c $(SRC)/ithaca/ithacaConfig.h $(SRC)/common/io.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/ithaca/ithacaConfig.c -o $(OBJ)/ithacaConfig.o

#################################################
# Island
#################################################

$(OBJ)/island.o: $(SRC)/island/island.c $(ISLAND_H)
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/island/island.c -o $(OBJ)/island.o

$(OBJ)/islandServer.o: $(SRC)/island/islandServer.c $(ISLAND_H)
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/island/islandServer.c -o $(OBJ)/islandServer.o

$(OBJ)/islandConfig.o: $(SRC)/island/islandConfig.c $(SRC)/island/islandConfig.h $(SRC)/common/io.h lib/sphragis.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/island/islandConfig.c -o $(OBJ)/islandConfig.o

#################################################
# Odysseus
#################################################

$(OBJ)/odysseus.o: $(SRC)/odysseus/odysseus.c $(SRC)/odysseus/sailing.h $(SHIP_H)
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/odysseus/odysseus.c -o $(OBJ)/odysseus.o

$(OBJ)/ship.o: $(SRC)/odysseus/ship.c $(SRC)/odysseus/voyages.h $(SRC)/odysseus/sailing.h $(SRC)/odysseus/market.h $(SRC)/odysseus/delivery.h $(SHIP_H)
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/odysseus/ship.c -o $(OBJ)/ship.o

$(OBJ)/map.o: $(SRC)/odysseus/map.c $(SRC)/odysseus/map.h $(SRC)/common/io.h lib/sphragis.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/odysseus/map.c -o $(OBJ)/map.o

$(OBJ)/voyages.o: $(SRC)/odysseus/voyages.c $(SRC)/odysseus/voyages.h $(SHIP_H)
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/odysseus/voyages.c -o $(OBJ)/voyages.o

$(OBJ)/sailing.o: $(SRC)/odysseus/sailing.c $(SRC)/odysseus/sailing.h $(SHIP_H)
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/odysseus/sailing.c -o $(OBJ)/sailing.o

$(OBJ)/market.o: $(SRC)/odysseus/market.c $(SRC)/odysseus/market.h $(SHIP_H)
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/odysseus/market.c -o $(OBJ)/market.o

$(OBJ)/delivery.o: $(SRC)/odysseus/delivery.c $(SRC)/odysseus/delivery.h $(SRC)/odysseus/market.h $(SHIP_H)
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/odysseus/delivery.c -o $(OBJ)/delivery.o

$(OBJ)/odysseusConfig.o: $(SRC)/odysseus/odysseusConfig.c $(SRC)/odysseus/odysseusConfig.h $(SRC)/common/io.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/odysseus/odysseusConfig.c -o $(OBJ)/odysseusConfig.o

$(OBJ)/terminal.o: $(SRC)/odysseus/terminal.c $(SRC)/odysseus/terminal.h $(SRC)/common/io.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/odysseus/terminal.c -o $(OBJ)/terminal.o

#################################################
# Utilities
#################################################

clean:
	rm -rf $(OBJ) ithaca island odysseus

tar: clean
	tar cf $(GROUP)_$(PHASE).tar Makefile README.md src lib data docs
