#!/bin/bash

make

# level 0

./hist enwik8

# level 1

./split

./hist {L,H}

# level 2

for s in {L,H}; do
	./split $s{,L,H}
done

./hist {L,H}{L,H}

# level 3

for s in {L,H}{L,H}; do
	./split $s{,L,H}
done

./hist {L,H}{L,H}{L,H}

# level 4

for s in {L,H}{L,H}{L,H}; do
	./split $s{,L,H}
done

./hist {L,H}{L,H}{L,H}{L,H}
