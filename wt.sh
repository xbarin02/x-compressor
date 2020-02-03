#!/bin/bash

make

# level 0

./hist enwik8

./grenc enwik8 enwik8.gr
du -ch enwik8.gr

# level 1

./split

./hist {L,H}

for s in {L,H}; do
	./grenc ${s} ${s}.gr
done
du -ch {L,H}.gr

# level 2

for s in {L,H}; do
	./split ${s}{,L,H}
done

./hist {L,H}{L,H}

for s in {L,H}{L,H}; do
	./grenc ${s} ${s}.gr
done
du -ch {L,H}{L,H}.gr

# level 3

for s in {L,H}{L,H}; do
	./split ${s}{,L,H}
done

./hist {L,H}{L,H}{L,H}

for s in {L,H}{L,H}{L,H}; do
	./grenc ${s} ${s}.gr
done
du -ch {L,H}{L,H}{L,H}.gr

# level 4

for s in {L,H}{L,H}{L,H}; do
	./split ${s}{,L,H}
done

./hist {L,H}{L,H}{L,H}{L,H}

for s in {L,H}{L,H}{L,H}{L,H}; do
	./grenc ${s} ${s}.gr
done
du -ch {L,H}{L,H}{L,H}{L,H}.gr

# level 5

for s in {L,H}{L,H}{L,H}{L,H}; do
	./split ${s}{,L,H}
done

./hist {L,H}{L,H}{L,H}{L,H}{L,H}

for s in {L,H}{L,H}{L,H}{L,H}{L,H}; do
	./grenc ${s} ${s}.gr
done
du -ch {L,H}{L,H}{L,H}{L,H}{L,H}.gr

# level 6

for s in {L,H}{L,H}{L,H}{L,H}{L,H}; do
	./split ${s}{,L,H}
done

./hist {L,H}{L,H}{L,H}{L,H}{L,H}{L,H}

for s in {L,H}{L,H}{L,H}{L,H}{L,H}{L,H}; do
	./grenc ${s} ${s}.gr
done
du -ch {L,H}{L,H}{L,H}{L,H}{L,H}{L,H}.gr

# level 7

for s in {L,H}{L,H}{L,H}{L,H}{L,H}{L,H}; do
	./split ${s}{,L,H}
done

./hist {L,H}{L,H}{L,H}{L,H}{L,H}{L,H}{L,H}

for s in {L,H}{L,H}{L,H}{L,H}{L,H}{L,H}{L,H}; do
	./grenc ${s} ${s}.gr
done
du -ch {L,H}{L,H}{L,H}{L,H}{L,H}{L,H}{L,H}.gr
