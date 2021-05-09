# QR-cpp

## Description

Simple (if you consider QR code as simple) headers-only C++ implementation of QR code generation algorithm.
Inspiration is mainly taken from [nayuki's library][1] and some code parts are borrowed, but the approach is
mainly different. This library is designed for convenient use when QR code version (size) is known at 
compile time, and all necessary arrays are alocated and encapsulated in QR<> class. I hope this will increase 
performance overall, but exact measurements were not carried out yet.

## How to use

User must choose appropriate QR code version, Error Correction Level and either choose mask manually or leave it
as `-1` to select automaically.

```cpp
constexpr int ver = 3;
constexpr ECC ecc = ECC::H;

const char *str = "HELLO WORLD";

QR<ver> qr;

// Manual mask 0
qr.encode(str, strlen(str), ecc, 0);
// Use small test print method to view code. NOTE: You need to define QR_PRINT
qr.print();

// Automatic mask
qr.encode(str, strlen(str), ecc, -1);
// Or print manually module by module
for (int y = 0; y < decltype(qr)::SIDE; ++y) {
    for (int x = 0; x < decltype(qr)::SIDE; ++x)
        printf("%s", qr.module(x, y) ? "\u2588\u2588" : "  ");
    printf("\n");
}
```

## TODO

- [x] Add data length check in `encode_data()`
- [ ] Fix numeric mode

[1]: https://github.com/nayuki/QR-Code-generator/tree/master/cpp