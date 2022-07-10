# qr

Simple (if you consider QR code as simple) headers-only C++ implementation of QR code generation algorithm.
Inspiration is mainly taken from [nayuki's library][1] and some code parts are borrowed, but the approach is
mainly different. This library is designed for convenient use when QR code version (size) is known at 
compile time, and all necessary arrays are alocated and encapsulated in QR<> class.

## How to use

User must choose appropriate QR code version, Error Correction Level and either choose mask manually or leave it
as `-1` to select automaically.

```cpp
constexpr auto ver = 3;
constexpr auto ecc = qr::Ecc::H;
constexpr auto str = "HELLO WORLD";

qr::Qr<ver> codec;

codec.encode(str, strlen(str), ecc, 0); // Manual mask 0
codec.encode(str, strlen(str), ecc, -1); // Automatic mask

for (int y = 0; y < codec.side_size(); ++y) { // Print manually module by module
    for (int x = 0; x < codec.side_size(); ++x)
        printf("%s", codec.module(x, y) ? "\u2588\u2588" : "  ");
    printf("\n");
}
```

## TODO

- [ ] tests

[1]: https://github.com/nayuki/QR-Code-generator/tree/master/cpp