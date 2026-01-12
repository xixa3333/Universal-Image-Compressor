#ifndef DCT_COMPRESSION_H
#define DCT_COMPRESSION_H

#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 將 DCT 負數係數平移為正數
#define DCT_OFFSET 256 

// 標準 JPEG 亮度量化表
const int std_quant_table[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}
};

// ZigZag 掃描順序
const int zigzag_flat[64] = {
     0,  1,  5,  6, 14, 15, 27, 28,
     2,  4,  7, 13, 16, 26, 29, 42,
     3,  8, 12, 17, 25, 30, 41, 43,
     9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

float C(int k) {
    return (k == 0) ? (1.0f / sqrt(2.0f)) : 1.0f;
}

// 壓縮用
void process_block_dct(float input_block[8][8], int* output_buffer, int offset_idx) {
    float dct_coeffs[8][8] = {0};

    // DCT 轉換
    for (int u = 0; u < 8; u++) {
        for (int v = 0; v < 8; v++) {
            float sum = 0.0f;
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    sum += (input_block[x][y] - 128.0f) * cos(((2.0f * x + 1.0f) * u * M_PI) / 16.0f) * cos(((2.0f * y + 1.0f) * v * M_PI) / 16.0f);
                }
            }
            dct_coeffs[u][v] = 0.25f * C(u) * C(v) * sum;
        }
    }

    for (int i = 0; i < 64; i++) {
        int r = zigzag_flat[i] / 8;
        int c = zigzag_flat[i] % 8;
        
        // 量化
        int quantized = (int)round(dct_coeffs[r][c] / (float)std_quant_table[r][c]);
        
        // 加上 256 讓它變成正數
        int final_val = quantized + DCT_OFFSET; 

        if (final_val < 0) final_val = 0;
        if (final_val > 511) final_val = 511;

        output_buffer[offset_idx + i] = final_val;
    }
}

// 解壓縮
void process_block_idct(int* input_buffer, int offset_idx, unsigned char output_block[8][8]) {
    float dequantized[8][8] = {0};

    for (int i = 0; i < 64; i++) {
        int r = zigzag_flat[i] / 8;
        int c = zigzag_flat[i] % 8;
        
        // 讀取並扣除 Offset
        int val = input_buffer[offset_idx + i] - DCT_OFFSET;
        
        dequantized[r][c] = (float)val * std_quant_table[r][c];
    }

    // IDCT 逆轉換
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            float sum = 0.0f;
            for (int u = 0; u < 8; u++) {
                for (int v = 0; v < 8; v++) {
                    sum += C(u) * C(v) * dequantized[u][v] *
                           cos(((2.0f * x + 1.0f) * u * M_PI) / 16.0f) * cos(((2.0f * y + 1.0f) * v * M_PI) / 16.0f);
                }
            }
            // IDCT 後加回 128
            float pixel = 0.25f * sum + 128.0f;
            
            if (pixel < 0) pixel = 0;
            if (pixel > 255) pixel = 255;
            
            output_block[x][y] = (unsigned char)pixel;
        }
    }
}

#endif