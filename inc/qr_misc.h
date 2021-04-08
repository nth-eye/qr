#ifndef QR_MISC_H
#define QR_MISC_H

#include <cstdint>
#include <cstring>

#include "qr_tables.h"

enum ECC { L, M, Q, H };

enum Mode { 
    M_NUMERIC,
    M_ALPHANUMERIC,
    M_BYTE,
    M_KANJI,
};

// Return n-th bit of arr starting from LSB.
constexpr uint8_t get_bit(const uint8_t *arr, int n)
{
    return (arr[n >> 3] >> (n & 7)) & 1;
}

// Return n-th bit of arr starting from MSB.
constexpr uint8_t get_bit_r(const uint8_t *arr, int n)
{
    return (arr[n >> 3] >> (7 - (n & 7))) & 1;
}

// Set n-th bit in array of bytes starting from LSB.
constexpr void set_bit(uint8_t *arr, int n)
{
    arr[n >> 3] |= 1 << (n & 7);
}

// Clear n-th bit in array of bytes starting from LSB.
constexpr void clr_bit(uint8_t *arr, int n)
{
    arr[n >> 3] &= ~(1 << (n & 7));
}

// Add up to 16 bits to arr. Data starts from MSB as well as each byte of an array.
constexpr void add_bits(uint16_t data, int n, uint8_t *arr, size_t &pos)
{
    while (n--) {
        arr[pos >> 3] |= ((data >> n) & 1) << (7 - (pos & 7)); 
        ++pos;
    }
}

// Get num of bytes required to store N bits.
constexpr int bytes(int n_bits)
{
    return (n_bits >> 3) + !!(n_bits & 7);
}

// Translate char to alphanumeric encoding value,
constexpr int QR_Alphanumeric(char c)
{
    if (c >= '0' && c <= '9') 
        return c - '0';

    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;

    switch (c) {
        case ' ': return 36;
        case '$': return 37;
        case '%': return 38;
        case '*': return 39;
        case '+': return 40;
        case '-': return 41;
        case '.': return 42;
        case '/': return 43;
        case ':': return 44;
    }
    return -1;
}

// Check if string can be encoded in alphanumeric mode.
constexpr bool QR_IsAlphanumeric(const char *str, size_t len) 
{
    for (int i = 0; i < len; ++i)
        if (QR_Alphanumeric(str[i]) == -1) 
            return false;
    return true;
}

// Check if string can be encoded in numeric mode.
constexpr bool QR_IsNumeric(const char *str, size_t len) 
{
    for (int i = 0; i < len; ++i)
        if (str[i] < '0' || str[i] > '9') 
            return false;
    return true;
}

// Check if string can be encoded in kanji mode.
constexpr bool QR_IsKanji(const char *str, size_t len) 
{
    for (int i = 0; i < len; i += 2) {

        uint16_t val = ((uint8_t) str[i]) | (((uint8_t) str[i + 1]) << 8);
        
        if (val < 0x8140 || val > 0xebbf || (val > 0x9ffc && val < 0xe040))
            return false;
    }
    return true;
}

// Select appropriate encoding mode for string.
constexpr Mode QR_SelectMode(const char *str, size_t len)
{
    if (QR_IsNumeric(str, len))
        return M_NUMERIC;
    
    if (QR_IsAlphanumeric(str, len))
        return M_ALPHANUMERIC;

    if (QR_IsKanji(str, len))
        return M_KANJI;

    return M_BYTE;
}

// Return size of Character Control Indicator in bits for given version and mode.
constexpr int QR_CCI(int ver, Mode mode)
{
    constexpr int cnt[4][3] = {
        { 10, 12, 14 },
        { 9,  11, 13 },
        { 8,  16, 16 },
        { 8,  10, 12 },
    };

    if (ver < 10)
        return cnt[mode][0];

    if (ver < 27)
        return cnt[mode][1];

    return cnt[mode][2];
}

// Galois Field multiplication using Russian Peasant Multiplication algorithm.
constexpr uint8_t GF_Mul(uint8_t x, uint8_t y) 
{
    uint8_t r = 0; 

    while (y) {
        if (y & 1)
            r ^= x; 
        x = (x << 1) ^ ((x >> 7) * 0x11d);
        y >>= 1;
    }
    return r;
}

// Reed-Solomon ECC generator polynomial for the given degree.
constexpr void GF_GenPoly(int degree, uint8_t *poly)
{
    memset(poly, 0, degree);
    
    uint8_t root = poly[degree - 1] = 1;

	for (int i = 0; i < degree; ++i) {
		for (int j = 0; j < degree - 1; ++j)
			poly[j] = GF_Mul(poly[j], root) ^ poly[j + 1];
        poly[degree - 1] = GF_Mul(poly[degree - 1], root);
        root = (root << 1) ^ ((root >> 7) * 0x11d);
	}
}

// Polynomial division if Galois Field.
constexpr void GF_PolyDiv(const uint8_t *dividend, size_t len, const uint8_t *divisor, int degree, uint8_t *result) 
{
	memset(result, 0, degree);

	for (int i = 0; i < len; ++i) {

		uint8_t factor = dividend[i] ^ result[0];

        memmove(&result[0], &result[1], degree - 1);

        result[degree - 1] = 0;

        for (int j = 0; j < degree; ++j)
            result[j] ^= GF_Mul(divisor[j], factor);
	}
}

#endif // QR_MISC_H