#include <stdio.h>
#include <stdlib.h>

// 將長度跟距離額外位元寫入位元流
void write_integer_bits(FILE* f_out, int value, int n_bits, unsigned char* byte_buffer, int* bit_count, long long* total_bits) {
    for (int i = n_bits - 1; i >= 0; i--) {
        (*byte_buffer) <<= 1;
        if ((value >> i) & 1) (*byte_buffer) |= 1;
        (*bit_count)++;
        (*total_bits)++;

        if (*bit_count == 8) {
            fwrite(byte_buffer, 1, 1, f_out);
            *byte_buffer = 0;
            *bit_count = 0;
        }
    }
}

// 位元讀取器
typedef struct {
    FILE* f_in;
    unsigned char buffer;
    int bits_remaining;
} BitReader;

// 初始化位元讀取器
BitReader* create_bit_reader(FILE* f) {
    BitReader* reader = (BitReader*)malloc(sizeof(BitReader));
    reader->f_in = f;
    reader->buffer = 0;
    reader->bits_remaining = 0;
    return reader;
}

// 讀取一個位元
int read_bit(BitReader* reader) {
    if (reader->bits_remaining == 0) {
        // 從檔案讀取下一個 byte
        if (fread(&reader->buffer, 1, 1, reader->f_in) != 1) {
            // 如果讀不到 (檔案結尾)，這是一個錯誤，
            // 除非 EOB 符號被正確處理了
            return -1; 
        }
        reader->bits_remaining = 8;
    }

    // 取得 MSB (最高有效位元)
    int bit = (reader->buffer >> 7) & 1;
    
    // 將 buffer 左移一位
    reader->buffer <<= 1;
    reader->bits_remaining--;

    return bit;
}

// 讀取 n 個位元並組合成一個整數
int read_n_bits(BitReader* reader, int n) {
    int value = 0;
    for (int i = 0; i < n; i++) {
        int bit = read_bit(reader);
        if (bit == -1) return -1;
        value = (value << 1) | bit;
    }
    return value;
}