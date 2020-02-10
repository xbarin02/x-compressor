x &ndash; minimalist data compressor
====================================

[![Build Status](https://travis-ci.org/xbarin02/x-compressor.svg?branch=master)](https://travis-ci.org/xbarin02/x-compressor)

Why?
----

The **x** is an easily verifiable and portable lossless data compressor.
Source codes count less than 700 lines in total.
A core library is less than 500 lines in pure C89.

Benchmarks
----------

Benchmark evaluates the compression of the reference [enwik8] file.
All compressors have been compiled with GCC 9.2 on 64-bit Linux.
The reference system uses an AMD Ryzen Threadripper 2990WX.
All measurements use default settings (no extra arguments).
The elapsed Compression and Decompression times (wall clock) are given in seconds.
The compression Ratio is given as uncompressed/compressed (more is better).
SLOC means Source Lines Of Code.
Bold font indicates the best result.

[enwik8]: http://prize.hutter1.net/

|  Compressor             | Ratio    | Compression time | Decompression time | SLOC    |
|  ----------             | -----    | ---------------- | ------------------ | ----    |
|  x                      | 1.88     | 1.07             | 0.93               | **663** |
|  bzip2 1.0.6            | **3.45** | 7.39             | 3.36               | 8117    |
|  lzop 1.04              | 1.78     | 0.36             | 0.33               | 17123   |
|  lz4 1.9.2              | 1.75     | **0.29**         | **0.11**           | 20619   |
|  gzip 1.9               | 2.74     | 4.69             | 0.63               | 48552   |

The algorithm
-------------

The **x** uses an adaptive Golomb-Rice coding based on context modeling.

How to build?
-------------

```
make BUILD=release
```

or

```
make build-pgo
```

How to use?
-----------

Compress:

```
./x INPUT-FILE [OUTPUT-FILE]
```

Decompress:

```
./unx INPUT-FILE [OUTPUT-FILE]
```

Author
------

- [David Barina](mailto:ibarina@fit.vutbr.cz)
