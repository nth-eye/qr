#include "qr.h"
#include "utl/time.h"
#include <cstdio>

template<class T>
void print_qr(const T &codec)
{
    printf("\n\n\n\n");
    for (int y = 0; y < codec.side_size(); ++y) {
        printf("        ");
        for (int x = 0; x < codec.side_size(); ++x)
            printf(codec.module(x, y) ? "\u2588\u2588" : "  ");
        printf("\n");
    }
    printf("\n\n\n\n");
}

int main(int, char**) 
{
    constexpr auto ver = 3;
    constexpr auto ecc = qr::Ecc::H;
    constexpr auto str = "HELLO WORLD";

    qr::Qr<ver> qr;

    qr.encode(str, strlen(str), ecc, -1);
    print_qr(qr);
    qr.encode(str, strlen(str), ecc, 0);
    print_qr(qr);

    // printf("Automatic mask: %3ld clock_t\n", 
    //     utl::m_exec_time<10000>(&qr::Qr<ver>::encode, &qr, str, strlen(str), ecc, -1));

    // printf("Manual mask 0:  %3ld clock_t\n", 
    //     utl::m_exec_time<10000>(&qr::Qr<ver>::encode, &qr, str, strlen(str), ecc, 0));
}
