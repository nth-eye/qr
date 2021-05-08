#include <cstdio>
#include <ctime>
#define QR_PRINT printf
#include "qr.h"

template<size_t N = 1, class Fn, class Ptr, class ...Args>
clock_t measure_time(Fn &&fn, Ptr *ptr, Args &&...args)
{
    clock_t begin = clock();
    for (size_t i = 0; i < N; ++i) 
        (ptr->*fn)(args...);
    clock_t end = clock();

    return (end - begin) / N;
}

int main(int, char**) 
{
    constexpr int ver = 3;
    constexpr ECC ecc = ECC::H;
    constexpr const char *str = "HELLO WORLD";

    QR<ver> qr;

    printf("Automatic mask: %ld clock_t\n", 
        measure_time<10000>(&QR<ver>::encode, &qr, str, strlen(str), ecc, -1));

    printf("Manual mask 0:  %3ld clock_t\n", 
        measure_time<10000>(&QR<ver>::encode, &qr, str, strlen(str), ecc, 0));

    qr.print();
}
