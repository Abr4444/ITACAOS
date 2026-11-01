# Nostos System - Phase 2 (G18)

`ithaca` (voyages server), `island` (port, market and map) and `odysseus` (the ship), communicating through TCP sockets.

## Compile

```bash
make          # builds ithaca, island and odysseus
make clean    # removes the object files and the executables
```

Compile on Montserrat / Matagalls (Linux x86-64), where `lib/sphragis.o` links.

## Execute

Ithaca and Aeaea run on different machines (IPs in `data/configs/*.dat`). Start the servers first.

```bash
# Montserrat
./ithaca data/configs/ithaca.dat data/voyages.dat
# Matagalls
./island data/configs/island.dat data/island_stock/Aeaea.db
# Montserrat
./odysseus data/configs/odysseus.dat
```

Every process ends with CTRL+C.

## Execute with valgrind

```bash
valgrind --leak-check=full --show-leak-kinds=all ./ithaca data/configs/ithaca.dat data/voyages.dat
valgrind --leak-check=full --show-leak-kinds=all ./island data/configs/island.dat data/island_stock/Aeaea.db
valgrind --leak-check=full --show-leak-kinds=all ./odysseus data/configs/odysseus.dat
```
