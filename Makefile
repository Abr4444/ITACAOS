#################################################
#
# Nostos System - Makefile
# Authors: Stefan Radulescu, Albert Baiget Rodulfo
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

GROUP = Gx
PHASE = F2

.PHONY: all clean tar

all: ithaca island odysseus

#################################################
# Executables
#################################################

ithaca: $(OBJ)/ithaca.o $(OBJ)/ithacaServer.o $(OBJ)/ithacaConfig.o $(OBJ)/server.o $(OBJ)/frame.o $(OBJ)/socket.o $(OBJ)/io.o
	$(CC) $(CFLAGS) $(OBJ)/ithaca.o $(OBJ)/ithacaServer.o $(OBJ)/ithacaConfig.o $(OBJ)/server.o $(OBJ)/frame.o $(OBJ)/socket.o $(OBJ)/io.o -o ithaca

island: $(OBJ)/island.o $(OBJ)/islandServer.o $(OBJ)/islandConfig.o $(OBJ)/server.o $(OBJ)/frame.o $(OBJ)/socket.o $(OBJ)/io.o lib/sphragis.o
	$(CC) $(CFLAGS) $(OBJ)/island.o $(OBJ)/islandServer.o $(OBJ)/islandConfig.o $(OBJ)/server.o $(OBJ)/frame.o $(OBJ)/socket.o $(OBJ)/io.o lib/sphragis.o -o island

odysseus: $(OBJ)/odysseus.o $(OBJ)/ship.o $(OBJ)/odysseusConfig.o $(OBJ)/terminal.o $(OBJ)/frame.o $(OBJ)/socket.o $(OBJ)/io.o
	$(CC) $(CFLAGS) $(OBJ)/odysseus.o $(OBJ)/ship.o $(OBJ)/odysseusConfig.o $(OBJ)/terminal.o $(OBJ)/frame.o $(OBJ)/socket.o $(OBJ)/io.o -o odysseus

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

#################################################
# Ithaca
#################################################

$(OBJ)/ithaca.o: $(SRC)/ithaca/ithaca.c $(SRC)/ithaca/ithacaServer.h $(SRC)/common/server.h $(SRC)/ithaca/ithacaConfig.h $(SRC)/common/frame.h $(SRC)/common/socket.h $(SRC)/common/io.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/ithaca/ithaca.c -o $(OBJ)/ithaca.o

$(OBJ)/ithacaServer.o: $(SRC)/ithaca/ithacaServer.c $(SRC)/ithaca/ithacaServer.h $(SRC)/common/server.h $(SRC)/ithaca/ithacaConfig.h $(SRC)/common/frame.h $(SRC)/common/socket.h $(SRC)/common/io.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/ithaca/ithacaServer.c -o $(OBJ)/ithacaServer.o

$(OBJ)/ithacaConfig.o: $(SRC)/ithaca/ithacaConfig.c $(SRC)/ithaca/ithacaConfig.h $(SRC)/common/io.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/ithaca/ithacaConfig.c -o $(OBJ)/ithacaConfig.o

#################################################
# Island
#################################################

$(OBJ)/island.o: $(SRC)/island/island.c $(SRC)/island/islandServer.h $(SRC)/common/server.h $(SRC)/island/islandConfig.h $(SRC)/common/frame.h $(SRC)/common/socket.h $(SRC)/common/io.h lib/sphragis.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/island/island.c -o $(OBJ)/island.o

$(OBJ)/islandServer.o: $(SRC)/island/islandServer.c $(SRC)/island/islandServer.h $(SRC)/common/server.h $(SRC)/island/islandConfig.h $(SRC)/common/frame.h $(SRC)/common/socket.h $(SRC)/common/io.h lib/sphragis.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/island/islandServer.c -o $(OBJ)/islandServer.o

$(OBJ)/islandConfig.o: $(SRC)/island/islandConfig.c $(SRC)/island/islandConfig.h $(SRC)/common/io.h lib/sphragis.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/island/islandConfig.c -o $(OBJ)/islandConfig.o

#################################################
# Odysseus
#################################################

$(OBJ)/odysseus.o: $(SRC)/odysseus/odysseus.c $(SRC)/odysseus/ship.h $(SRC)/odysseus/odysseusConfig.h $(SRC)/odysseus/terminal.h $(SRC)/common/io.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/odysseus/odysseus.c -o $(OBJ)/odysseus.o

$(OBJ)/ship.o: $(SRC)/odysseus/ship.c $(SRC)/odysseus/ship.h $(SRC)/odysseus/odysseusConfig.h $(SRC)/odysseus/terminal.h $(SRC)/common/frame.h $(SRC)/common/socket.h $(SRC)/common/io.h
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $(SRC)/odysseus/ship.c -o $(OBJ)/ship.o

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