#ifndef QR_H
#define QR_H

#include <cmath>
#include "qr_misc.h"

#ifndef QR_PRINT
#define QR_PRINT(...)
#warning "To print QR code please define QR_PRINT"
#endif

template<int V>
struct QR {
    static constexpr int SIDE           = 17 + V * 4;
    static constexpr int N_BITS         = SIDE * SIDE;
    static constexpr int N_ALIGN        = V == 1 ? 0 : V / 7 + 2;

    static constexpr int N_ALIGN_BITS   = V > 1 ? (N_ALIGN * N_ALIGN - 3) * 25 : 0;
    static constexpr int N_TIMING_BITS  = (SIDE - 16) * 2 - (10 * (V > 1 ? N_ALIGN - 2 : 0));
    static constexpr int N_VER_BITS     = V > 6 ? 36 : 0;
    static constexpr int N_DAT_BITS     = N_BITS - (192 + N_ALIGN_BITS + N_TIMING_BITS + 31 + N_VER_BITS);

    static constexpr int N_BYTES        = bytes(N_BITS);        // Actual number of bytes required to store whole QR code
    static constexpr int N_DAT_BYTES    = bytes(N_DAT_BITS);    // Actual number of bytes required to store [data + ecc]
    static constexpr int N_DAT_CAPACITY = N_DAT_BITS >> 3;      // Capacity of [data + ecc] without remainder bits

    void print();
    bool encode(const char *str, size_t len, ECC ecc, int mask = -1);
private:
    bool encode_data(const char *data, size_t len, ECC ecc, uint8_t *out);
    void encode_ecc(const uint8_t *data, ECC ecc, uint8_t *out);

    void add_data(const uint8_t *data, const uint8_t *patterns);
    void add_patterns(ECC ecc);
    void add_version();
    void add_format(ECC ecc, int mask);

    void draw_rect(int y, int x, int height, int width, bool black, uint8_t *out);
    void draw_bound(int y, int x, int height, int width, bool black, uint8_t *out);

    void reserve_patterns(uint8_t *out);

    template<bool Horizontal>
    int  rule_1_3_score();
    int  penalty_score();
    int  select_mask(ECC ecc, const uint8_t *patterns);
    void apply_mask(int mask, const uint8_t *patterns);

    uint8_t code[N_BYTES] = {};
    bool status = false;
};


// Print resulting QR code. You need to define QR_PRINT. For example as printf.
template<int V>
void QR<V>::print()
{
    if (status) {
        QR_PRINT("\n\n\n\n");
        for (int y = 0; y < N_BITS; y += SIDE) {
            QR_PRINT("        ");
            for (int x = 0; x < SIDE; ++x)
                QR_PRINT(get_bit(code, y + x) ? "\u2588\u2588" : "  ");
            QR_PRINT("\n");
        }
        QR_PRINT("\n\n\n\n");
    } else {
        QR_PRINT("\n---Last encoding operation wasn't successful---\n");
    }
}

// Create QR code with given error correction level. If mask == -1, 
// then best mask selected autimatically. NOTE: Automatic mask is the 
// most expensive operation. Takes about 95 % of all computation time.
template<int V>
bool QR<V>::encode(const char *str, size_t len, ECC ecc, int mask)
{
    uint8_t data[N_DAT_BYTES]           = {};
    uint8_t data_with_ecc[N_DAT_BYTES]  = {};
    uint8_t patterns[N_BYTES]           = {};

    if (!encode_data(str, len, ecc, data)) {
        QR_PRINT("\n---Error during data encoding---\n");
        return status = false;
    }
    encode_ecc(data, ecc, data_with_ecc);

    reserve_patterns(patterns);
    memcpy(code, patterns, N_BYTES);

    add_data(data_with_ecc, patterns);
    add_patterns(ecc);
    add_version();

    mask = mask != -1 ? mask & 7 : select_mask(ecc, patterns);

    add_format(ecc, mask);
    apply_mask(mask, patterns);

    return status = true;
}

template<int V>
bool QR<V>::encode_data(const char *data, size_t len, ECC ecc, uint8_t *out)
{
    Mode mode = QR_SelectMode(data, len);

    size_t pos = 0;

    add_bits(1 << mode, 4, out, pos);
    add_bits(len, QR_CCI(V, mode), out, pos);

    if (mode == M_NUMERIC) {

        int i = 0, j;

        while (i < len) {

            char buf[4];

            for (j = 0; j < 3 && i < len; ++j, ++i)
                buf[j] = data[i];
            buf[j] = 0;

            uint16_t num = strtol(buf, NULL, 10);
            add_bits(num, num < 100 ? num < 10 ? 4 : 7 : 10, out, pos);
        }
    } else if (mode == M_ALPHANUMERIC) {

        for (int i = 0; i < (int)(len & ~1); i += 2) {
            uint16_t num = QR_Alphanumeric(data[i]) * 45 + QR_Alphanumeric(data[i + 1]);
            add_bits(num, 11, out, pos);
        }
        if (len & 1)
            add_bits(QR_Alphanumeric(data[len - 1]), 6, out, pos);

    } else if (mode == M_BYTE) {

        for (int i = 0; i < len; ++i)
            add_bits(data[i], 8, out, pos);

    } else {

        for (int i = 0; i < len; i += 2) {
            uint16_t val = ((uint8_t) data[i]) | (((uint8_t) data[i + 1]) << 8);
            uint16_t res = 0;
            val -= val < 0x9FFC ? 0x8140 : 0xC140;
            res += val & 0xff;
            res += (val >> 8) * 0xc0;
            add_bits(res, 13, out, pos);
        }
    }

    // Padding
    int n_bits = (N_DAT_CAPACITY - ECC_CODEWORDS_PER_BLOCK[ecc][V] * N_ECC_BLOCKS[ecc][V]) << 3;
    int padding = n_bits - pos;
    int i = 0;

    if (padding >= 0)
        add_bits(0, padding > 4 ? 4 : padding, out, pos);
    else 
        return false;  // If data is too big for the version
    
    if (pos & 7)
        add_bits(0, 8 - pos & 7, out, pos);

    while (pos < n_bits)
        add_bits(++i & 1 ? 0xec : 0x11, 8, out, pos);

    return true;
}

template<int V>
void QR<V>::encode_ecc(const uint8_t *data, ECC ecc, uint8_t *out)
{
    int n_blocks        = N_ECC_BLOCKS[ecc][V];
    int ecc_len         = ECC_CODEWORDS_PER_BLOCK[ecc][V];

    int n_data_bytes    = N_DAT_CAPACITY - ecc_len * n_blocks;

    int n_short_blocks  = n_blocks - N_DAT_CAPACITY % n_blocks;
    int short_len       = N_DAT_CAPACITY / n_blocks - ecc_len;

    uint8_t gen_poly[30];
    uint8_t ecc_buf[30];

    GF_GenPoly(ecc_len, gen_poly);

    const uint8_t *data_ptr = data;

    for (int i = 0; i < n_blocks; ++i) {

        int data_len = short_len;

        if (i >= n_short_blocks)
            ++data_len;

        GF_PolyDiv(data_ptr, data_len, gen_poly, ecc_len, ecc_buf);

        for (int j = 0, k = i; j < data_len; ++j, k += n_blocks) {
            if (j == short_len)
                k -= n_short_blocks;
            out[k] = data_ptr[j];
        }
        for (int j = 0, k = n_data_bytes + i; j < ecc_len; ++j, k += n_blocks)
            out[k] = ecc_buf[j];

        data_ptr += data_len;
    }
}

template<int V>
void QR<V>::add_data(const uint8_t *data, const uint8_t *patterns)
{
    int data_pos = 0;

    for (int x = SIDE - 1; x >= 1; x -= 2) {

        if (x == 6)
            x = 5;

        for (int i = 0; i < SIDE; ++i) {

            int y = !((x + 1) & 2) ? SIDE - 1 - i : i;
            int coord = y * SIDE + x;

            if (!get_bit(patterns, coord)) {

                if (get_bit_r(data, data_pos))
                    set_bit(code, coord);
                
                ++data_pos;
            }

            if (!get_bit(patterns, coord - 1)) {

                if (get_bit_r(data, data_pos))
                    set_bit(code, coord - 1);
                
                ++data_pos;
            }
        }
    }
}

template<int V>
void QR<V>::add_patterns(ECC ecc)
{
    // White bounds inside finders
    draw_bound(1, 1, 5, 5, false, code);
    draw_bound(1, SIDE - 6, 5, 5, false, code);
    draw_bound(SIDE - 6, 1, 5, 5, false, code);

    // Finish alignment patterns
    for (int i = 0; i < N_ALIGN; ++i) {
        for (int j = 0; j < N_ALIGN; ++j) {
            if ((!i && !j) || 
                (!i && j == N_ALIGN - 1) || 
                (!j && i == N_ALIGN - 1) )
                continue;
            draw_bound(ALIGN_POS[V][i] - 1, ALIGN_POS[V][j] - 1, 3, 3, false, code);
        }
    }

    // Draw white separators
    draw_rect(7, 0, 1, 8, false, code);
    draw_rect(0, 7, 8, 1, false, code);
    draw_rect(SIDE - 8, 0, 1, 8, false, code);
    draw_rect(SIDE - 8, 7, 8, 1, false, code);
    draw_rect(7, SIDE - 8, 1, 8, false, code);
    draw_rect(0, SIDE - 8, 8, 1, false, code);

    // Perforate timing patterns
    for (int i = 7; i < SIDE - 7; i += 2) {
        clr_bit(code, 6 * SIDE + i);
        clr_bit(code, i * SIDE + 6);
    }
}

template<int V>
void QR<V>::add_version()
{
    if (V < 7)
        return;

    uint32_t rem = V;

    for (uint8_t i = 0; i < 12; ++i)
        rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);

    uint32_t data = V << 12 | rem;
    
    for (int x = 0; x < 6; ++x) {
        for (int j = 0; j < 3; ++j) {

            int y = SIDE - 11 + j;

            bool black = (data >> (x * 3 + j)) & 1;

            if (!black) {
                clr_bit(code, y * SIDE + x);
                clr_bit(code, y + SIDE * x);
            }
        }
    }
}

template<int V>
void QR<V>::add_format(ECC ecc, int mask)
{
    int data = (ecc ^ 1) << 3 | mask;
    int rem = data;

    for (int i = 0; i < 10; i++)
        rem = (rem << 1) ^ ((rem >> 9) * 0b10100110111);

    int res = (data << 10 | rem) ^ 0b101010000010010;

    for (int i = 0; i < 6; ++i) {
        if ((res >> i) & 1) {
            set_bit(code, SIDE * 8 + SIDE - 1 - i);
            set_bit(code, SIDE * i + 8);
        } else {
            clr_bit(code, SIDE * 8 + SIDE - 1 - i);
            clr_bit(code, SIDE * i + 8);
        }
    }

    for (int i = 6; i < 8; ++i) {
        if ((res >> i) & 1) {
            set_bit(code, SIDE * 8 + SIDE - 1 - i);
            set_bit(code, SIDE * (i + 1) + 8);
        } else {
            clr_bit(code, SIDE * 8 + SIDE - 1 - i);
            clr_bit(code, SIDE * (i + 1) + 8);
        }
    }

    if ((res >> 8) & 1) {
        set_bit(code, SIDE * 8 + 7);
        set_bit(code, SIDE * (SIDE - 7) + 8);
    } else {
        clr_bit(code, SIDE * 8 + 7);
        clr_bit(code, SIDE * (SIDE - 7) + 8); 
    }

    for (int i = 9, j = 5; i < 15; ++i, --j) {
        if ((res >> i) & 1) {
            set_bit(code, SIDE * 8 + j);
            set_bit(code, SIDE * (SIDE - 1 - j) + 8);
        } else {
            clr_bit(code, SIDE * 8 + j);
            clr_bit(code, SIDE * (SIDE - 1 - j) + 8);
        }
    }
}

template<int V>
void QR<V>::draw_rect(int y, int x, int height, int width, bool black, uint8_t *out)
{
    if (black) {
        for (int dy = y * SIDE; dy < (y + height) * SIDE; dy += SIDE)
            for (int dx = x; dx < x + width; ++dx) 
                set_bit(out, dy + dx);
    } else {
        for (int dy = y * SIDE; dy < (y + height) * SIDE; dy += SIDE)
            for (int dx = x; dx < x + width; ++dx)
                clr_bit(out, dy + dx);
    }
}

template<int V>
void QR<V>::draw_bound(int y, int x, int height, int width, bool black, uint8_t *out)
{
    if (black) {
        for (int i = y * SIDE + x;              i < y * SIDE + x+width;                 ++i)
            set_bit(out, i);
        for (int i = (y+height-1) * SIDE + x;   i < (y+height-1) * SIDE + x+width;      ++i)
            set_bit(out, i);
        for (int i = (y+1) * SIDE + x;          i < (y+height-1) * SIDE + x;            i += SIDE)
            set_bit(out, i);
        for (int i = (y+1) * SIDE + x+width-1;  i < (y+height-1) * SIDE + x+width-1;    i += SIDE)
            set_bit(out, i);
    } else {
        for (int i = y * SIDE + x;              i < y * SIDE + x+width;                 ++i)
            clr_bit(out, i);
        for (int i = (y+height-1) * SIDE + x;   i < (y+height-1) * SIDE + x+width;      ++i)
            clr_bit(out, i);
        for (int i = (y+1) * SIDE + x;          i < (y+height-1) * SIDE + x;            i += SIDE)
            clr_bit(out, i);
        for (int i = (y+1) * SIDE + x+width-1;  i < (y+height-1) * SIDE + x+width-1;    i += SIDE)
            clr_bit(out, i);
    }
}

template<int V>
void QR<V>::reserve_patterns(uint8_t *out)
{
    draw_rect(0, 6, SIDE, 1, true, out);
    draw_rect(6, 0, 1, SIDE, true, out);
    
    draw_rect(0, 0, 9, 9, true, out);
    draw_rect(SIDE - 8, 0, 8, 9, true, out);
    draw_rect(0, SIDE - 8, 9, 8, true, out);

    for (int i = 0; i < N_ALIGN; ++i) {
        for (int j = 0; j < N_ALIGN; ++j) {
            if ((!i && !j) || 
                (!i && j == N_ALIGN - 1) || 
                (!j && i == N_ALIGN - 1) )
                continue;
            draw_rect(ALIGN_POS[V][i] - 2, ALIGN_POS[V][j] - 2, 5, 5, true, out);
        }
    }

    if (V >= 7) {
        draw_rect(SIDE - 11, 0, 3, 6, true, out);
        draw_rect(0, SIDE - 11, 6, 3, true, out);
    }
}

template<int V>
template<bool H>
int QR<V>::rule_1_3_score()
{
    constexpr int y_max  = H ? N_BITS : SIDE;
    constexpr int x_max  = H ? SIDE : N_BITS;
    constexpr int y_step = H ? SIDE : 1;
    constexpr int x_step = H ? 1 : SIDE;
    
    int res = 0;

    for (int y = 0; y < y_max; y += y_step) {
        bool color = get_bit(code, y);
        int finder = color;
        int cnt    = 1;
        for (int x = 1; x < x_max; x += x_step) {
            if (get_bit(code, y + x) == color) {
                ++cnt;
                if (cnt == 5)
                    res += 3;
                if (cnt > 5)
                    ++res;
            } else {
                color = !color;
                cnt = 1;
            }
            // Finder-like
            finder = ((finder << 1) & 0x7ff) | color;
            if (x >= x_step * 10) {
                if (finder == 0x05d || finder == 0x5d0)
                    res += 40;
            }
        }
    }
    return res;
}

template<int V>
int QR<V>::penalty_score()
{
    int res = 0;

    res += rule_1_3_score<true>();
    res += rule_1_3_score<false>();

    for (int y = 0; y < N_BITS - SIDE; y += SIDE) {
        for (int x = 0; x < SIDE - 1; ++x) {

            bool c = get_bit(code, y + x);

            if (c == get_bit(code, y + x + 1)  &&
                c == get_bit(code, y + x + SIDE) &&
                c == get_bit(code, y + x + SIDE + 1))
                res += 3;
        }
    }

    int black = 0;
    for (int y = 0; y < N_BITS; y += SIDE) {
        for (int x = 0; x < SIDE; ++x)
            black += get_bit(code, y + x);
    }
    res += abs((black * 100) / N_BITS - 50) / 5 * 10;

    return res;
}

template<int V>
int QR<V>::select_mask(ECC ecc, const uint8_t *patterns)
{
    unsigned min_score = -1;
    unsigned score = 0;
    uint8_t mask = 0;

    for (int i = 0; i < 8; ++i) {
        add_format(ecc, i);
        apply_mask(i, patterns);
        score = penalty_score();
        if (score < min_score) {
            mask = i;
            min_score = score;
        }
        apply_mask(i, patterns);
    }
    return mask;
}

template<int V>
void QR<V>::apply_mask(int mask, const uint8_t *patterns)
{
    for (int y = 0, dy = 0; y < SIDE; ++y, dy += SIDE) {
        for (int x = 0; x < SIDE; ++x) {

            int coord = dy + x;

            if (get_bit(patterns, coord))
                continue;

            bool keep = true;

            switch (mask) {
                case 0: keep = x + y & 1;                  break;
                case 1: keep =  y & 1;                      break;
                case 2: keep =  x % 3;                      break;
                case 3: keep = (x + y) % 3;                 break;
                case 4: keep =  y / 2 + x / 3 & 1;          break;
                case 5: keep =  x * y  % 2 + x * y % 3;     break;
                case 6: keep =  x * y  % 2 + x * y % 3 & 1; break;
                case 7: keep = (x + y) % 2 + x * y % 3 & 1; break;
            }

            if (!keep) {
                if (get_bit(code, coord))
                    clr_bit(code, coord);
                else
                    set_bit(code, coord);
            }
        }
    }
}

#endif // QR_H