#!/bin/bash

make

# level 0

./hist enwik8

# level 1

./split

./hist L H

# level 2

./split L{,L,H}
./split H{,L,H}

./hist L{L,H} H{L,H}

# level 3

./split LL{,L,H}
./split HL{,L,H}
./split LH{,L,H}
./split HH{,L,H}

./hist LL{L,H} LH{L,H} HL{L,H} HH{L,H}

# level 4

./split LLL{,L,H}
./split LLH{,L,H}
./split LHL{,L,H}
./split LHH{,L,H}
./split HLL{,L,H}
./split HLH{,L,H}
./split HHL{,L,H}
./split HHH{,L,H}

./hist LLL{L,H} LLH{L,H} LHL{L,H} LHH{L,H} HLL{L,H} HLH{L,H} HHL{L,H} HHH{L,H}
