# Nostos System - Phase 3 (G18)

## Compile

```bash
make          # builds ithaca, island and odysseus
make clean    # removes the object files and the executables
make tar      # packs the delivery: G18_F3.tar
```

## Execute

Ithaca and Scheria use 172.16.214.21, Aeaea uses 172.16.214.20
Change the IP and port in the config files to run them elsewhere

```bash
# Montserrat
./ithaca data/configs/ithaca.dat data/voyages.dat
# Matagalls
./island data/configs/island.dat data/island_stock/Aeaea.db
# Montserrat (destination island of the Scheria voyages)
./island data/configs/scheria.dat data/island_stock/Scheria.db
# Montserrat
./odysseus data/configs/odysseus.dat
```

Every process ends with CTRL+C.

Each process stores its files in the folder of the 2nd line of its config file
(created if it does not exist). Odysseus saves its gold, cargo, active voyage (with its image), discovered map and location in
`<its folder>/state.dat` after every command and loads them when it starts again (if it was at an island, it starts at sea next to it, not connected, and SAIL to that island reconnects); each island saves
its stock in its `.db` file.

## Execute with valgrind
```bash
valgrind --leak-check=full --show-leak-kinds=all ./ithaca data/configs/ithaca.dat data/voyages.dat
valgrind --leak-check=full --show-leak-kinds=all ./island data/configs/island.dat data/island_stock/Aeaea.db
valgrind --leak-check=full --show-leak-kinds=all ./island data/configs/scheria.dat data/island_stock/Scheria.db
valgrind --leak-check=full --show-leak-kinds=all ./odysseus data/configs/odysseus.dat
```