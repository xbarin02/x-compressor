x &ndash; minimalist data compressor
====================================

Why?
----

The **x** is easily verifiable and portable lossless data compressor.
Source codes count less than 700 lines in total.
A core library is less than 500 lines in pure C89.

Benchmarks
----------

Benchmark evaluates the compression of reference [enwik8] file.
All compressors have been compiled with GCC 9.2 on 64-bit Linux.
The reference system uses a AMD Ryzen Threadripper 2990WX.
All measurements use default settings (no extra arguments).
The ellapsed Compression and Decompression times (wall clock) are given in seconds.
The compression Ratio is given as uncompressed/compressed (more is better).
SLOC means Source Lines Of Code.

[enwik8]: http://prize.hutter1.net/

|  Compressor             | Ratio   | Compression | Decompression | SLOC |
|  ----------             | -----   | ----------- | ------------- | ---- |
|  x                      | 1.88    | 1.11        | 0.98          | 678  |
|  lz4                    | 1.75    | 0.29        | 0.11          |      |
|  lzop                   | 1.78    | 0.36        | 0.33          |      |
|  gzip                   | 2.74    | 4.69        | 0.63          |      |
|  bzip2                  | 3.45    | 7.39        | 3.36          |      |

How to build?
-------------

```
make BUILD=release
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

David Barina <ibarina@fit.vutbr.cz>
