# Nostos System - Phase 3 (G18)

## Compile

```bash
make          # builds ithaca, island and odysseus
make clean    # removes the object files and the executables
make tar      # packs the delivery: G18_F3.tar
```

## Execute

Montserrat is 172.16.214.21, Matagalls 172.16.214.20 and Puigpedros 172.16.214.18.
Change the IP and port in the config files to run them elsewhere

```bash
# Montserrat
./ithaca data/configs/ithaca.dat data/voyages.dat
./island data/configs/thrinacia.dat data/island_stock/Thrinacia.db
./odysseus data/configs/odysseus.dat      # Polyphemus
./odysseus data/configs/eumaeus.dat
# Matagalls
./island data/configs/island.dat data/island_stock/Aeaea.db
./island data/configs/aeolia.dat data/island_stock/Aeolia.db
./odysseus data/configs/telemachus.dat
# Puigpedros
./island data/configs/scheria.dat data/island_stock/Scheria.db
./island data/configs/ogygia.dat data/island_stock/Ogygia.db
./island data/configs/ismarus.dat data/island_stock/Ismarus.db
./odysseus data/configs/penelope.dat
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