# Nostos System - Phase 2 (G18)
## Compile

```bash
make          # builds ithaca, island and odysseus
make clean    # removes the object files and the executables
```

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
