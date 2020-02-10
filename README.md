x &ndash; minimalist data compressor
====================================

[![Build Status](https://travis-ci.org/xbarin02/x-compressor.svg?branch=master)](https://travis-ci.org/xbarin02/x-compressor)

Why?
----

Because readable and maintainable code is key.
The **x** is an easily verifiable and portable lossless data compressor.
Source codes count less than 600 lines in total.
A core library is 400 lines in pure C.

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
|  ----------             | -----    | ---------------- | ------------------ | ----:   |
|  lz4 1.9.2              | 1.75     | **0.29**         | **0.11**           |  20 619 |
|  lzop 1.04              | 1.78     | 0.36             | 0.33               |  17 123 |
|  **x**                  | 1.88     | 1.07             | 0.93               | **593** |
|  gzip 1.9               | 2.74     | 4.69             | 0.63               |  48 552 |
|  zstd 1.3.7             | 2.80     | 0.55             | 0.18               | 111 948 |
|  bzip2 1.0.6            | **3.45** | 7.39             | 3.36               |   8 117 |

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

Authors
-------

- David Barina, <ibarina@fit.vutbr.cz>

License
-------

This project is licensed under the MIT License.
See the [LICENSE.md](LICENSE.md) file for details.
