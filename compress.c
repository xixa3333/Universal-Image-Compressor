#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "huffman_compression.h"
#include "LZ77.h"
#include "deflate.h"
#include "choose_file.h"
#include "dct_compression.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define DPCM_ALPHABET_SIZE 512
#define COMP_ALPHABET_SIZE (DPCM_ALPHABET_SIZE + 30)

// 統計頻率
PixelDelta** frequency_calculation(int size, int* Pixels, int* Grayscale_frequency, int* count){
    for (int i = 0; i < size; i++) Grayscale_frequency[Pixels[i]]++;

    for (int i = 0; i < DPCM_ALPHABET_SIZE + 30; i++) {
        if (Grayscale_frequency[i] != 0) {
            (*count)++;
        }
    }

    PixelDelta** node_list = (PixelDelta**)malloc(*count * sizeof(PixelDelta*));
    int current_node = 0;
    for (int i = 0; i < DPCM_ALPHABET_SIZE + 30; i++){
        if(Grayscale_frequency[i] != 0){
            PixelDelta* newNode = (PixelDelta*)malloc(sizeof(PixelDelta));
            newNode->gray = i;
            newNode->frequency = Grayscale_frequency[i];
            newNode->left_node = NULL;
            newNode->right_node = NULL;
            node_list[current_node++] = newNode;
        }
    }
    return node_list;
}

// 輸出壓縮檔案
long long compressed_archives_LZ(
    char fileType, int width, int height, int channels,
    int lz_output_count, LZ77Symbol* lz_stream, 
    char** ll_codes, char** dist_codes, 
    int* ll_freq, int ll_symbol_count,
    int* dist_freq, int dist_symbol_count,
    const char* output_filename) 
{
    printf("將儲存壓縮檔案至: %s\n", output_filename);

    long long total_encoded_length = 0;
    unsigned char byte_buffer = 0;
    int bit_count = 0;
    
    FILE* f_out = fopen(output_filename, "wb");
    if (!f_out) {
        printf("無法開啟輸出檔案\n");
        return -1;
    }

    char magic[4] = {'H', 'U', 'F', 'Z'}; 
    fwrite(magic, sizeof(char), 4, f_out);
    fwrite(&fileType, sizeof(char), 1, f_out);
    fwrite(&width, sizeof(int), 1, f_out);
    fwrite(&height, sizeof(int), 1, f_out);
    fwrite(&channels, sizeof(int), 1, f_out);
    
    // 寫入 L/L 樹
    fwrite(&ll_symbol_count, sizeof(int), 1, f_out);
    for (int i = 0; i < COMP_ALPHABET_SIZE; i++) {
        if (ll_freq[i] > 0) {
            unsigned short symbol_index = (unsigned short)i;
            fwrite(&symbol_index, sizeof(unsigned short), 1, f_out);
            fwrite(&ll_freq[i], sizeof(int), 1, f_out);
        }
    }
    
    // 寫入 Distance 樹
    fwrite(&dist_symbol_count, sizeof(int), 1, f_out);
    for (int i = 0; i < COMP_ALPHABET_SIZE; i++) {
        if (dist_freq[i] > 0) {
            unsigned short symbol_index = (unsigned short)i;
            fwrite(&symbol_index, sizeof(unsigned short), 1, f_out);
            fwrite(&dist_freq[i], sizeof(int), 1, f_out);
        }
    }

    for (int i = 0; i < lz_output_count; i++) {
        if (lz_stream[i].type == LITERAL) {
            int literal = lz_stream[i].value;
            write_bits(f_out, ll_codes[literal], &byte_buffer, &bit_count, &total_encoded_length);
        } else { 
            int length = lz_stream[i].value;
            int length_code = get_length_code(length);
            int remapped_length_code = (length_code - 257) + DPCM_ALPHABET_SIZE;
            
            int distance = lz_stream[i].distance;
            int distance_code = get_distance_code(distance);

            write_bits(f_out, ll_codes[remapped_length_code], &byte_buffer, &bit_count, &total_encoded_length);

            int len_extra_bits = length_extra_bits_table[length_code - 257];
            if (len_extra_bits > 0) {
                int len_extra_value = length - length_base_table[length_code - 257];
                write_integer_bits(f_out, len_extra_value, len_extra_bits, &byte_buffer, &bit_count, &total_encoded_length);
            }

            write_bits(f_out, dist_codes[distance_code], &byte_buffer, &bit_count, &total_encoded_length);

            int dist_extra_bits = dist_extra_bits_table[distance_code];
            if (dist_extra_bits > 0) {
                int dist_extra_value = distance - dist_base_table[distance_code];
                write_integer_bits(f_out, dist_extra_value, dist_extra_bits, &byte_buffer, &bit_count, &total_encoded_length);
            }
        }
    }

    const char* eob_code = ll_codes[DPCM_ALPHABET_SIZE - 1];
    if (eob_code) {
        write_bits(f_out, eob_code, &byte_buffer, &bit_count, &total_encoded_length);
    }

    if (bit_count > 0) {
        byte_buffer <<= (8 - bit_count);
        fwrite(&byte_buffer, 1, 1, f_out);
    }
    
    fclose(f_out);
    long long compressed_size_bytes = (total_encoded_length + 7) / 8;
    return compressed_size_bytes;
}

int main() {
    SetConsoleOutputCP(65001);
    
    char input_filepath[MAX_PATH] = "";
    int width = 0, height = 0, channels = 0;
    int* data_buffer = NULL; 
    int size = 0;
    int mode = 0; 
    
    // 'D': DCT Image, 'L': Lossless Image (DPCM), 'G': General/Raw
    char fileType = 'G'; 

    printf("全能壓縮系統\n");
    printf("1. 圖片-DCT 失真模式 (適合照片，高壓縮率)\n");
    printf("2. 圖片-無損 DPCM 模式 (適合醫學影像/截圖，畫質無損)\n");
    printf("3. 通用-無損 Raw 模式 (適合文字/執行檔/任何檔案)\n");
    printf("輸入選項 (1-3): ");
    scanf("%d", &mode);
    getchar(); 

    if (mode == 1 || mode == 2) {
        printf("請選擇影像檔案\n");
        unsigned char* img = choose_img(&width, &height, &channels, input_filepath);
        if (img == NULL) return -1;
        
        // 設定檔案類型
        fileType = (mode == 1) ? 'D' : 'L';

        // 計算 Padding 尺寸
        int new_w = (width + 7) & ~7;
        int new_h = (height + 7) & ~7;
        size = new_w * new_h * channels;
        
        data_buffer = (int*)malloc(size * sizeof(int));
        
        if (mode == 1) {
            // DCT 流程
            printf("執行: DCT -> 量化 -> ZigZag\n");
            float block_proc[8][8];
            int buffer_idx = 0;
            for (int by = 0; by < new_h; by += 8) {
                for (int bx = 0; bx < new_w; bx += 8) {
                    for (int k = 0; k < channels; k++) {
                        for (int y = 0; y < 8; y++) {
                            for (int x = 0; x < 8; x++) {
                                int ix = bx + x, iy = by + y;
                                block_proc[x][y] = (ix < width && iy < height) ? 
                                    (float)img[(iy * width + ix) * channels + k] : 0.0f;
                            }
                        }
                        process_block_dct(block_proc, data_buffer, buffer_idx);
                        buffer_idx += 64;
                    }
                }
            }
        } else {
            // DPCM 流程
            printf("執行: DPCM (Current - Previous)\n");
            memset(data_buffer, 0, size * sizeof(int));
            int buffer_idx = 0;
            for (int by = 0; by < new_h; by += 8) {
                for (int bx = 0; bx < new_w; bx += 8) {
                    for (int k = 0; k < channels; k++) {
                        int prev_val = 0;
                        for (int y = 0; y < 8; y++) {
                            for (int x = 0; x < 8; x++) {
                                int ix = bx + x, iy = by + y;
                                int curr = (ix < width && iy < height) ? 
                                    img[(iy * width + ix) * channels + k] : 0;
                                
                                data_buffer[buffer_idx++] = (curr - prev_val) + 256;
                                prev_val = curr;
                            }
                        }
                    }
                }
            }
        }
        stbi_image_free(img);
    } 
    // 通用檔案處理路徑
    else if (mode == 3) {
        fileType = 'G';
        printf("請選擇任意檔案\n");
        
        if (!choose_any_file(input_filepath)) return 0;
        
        FILE* f_raw = fopen(input_filepath, "rb");
        if (!f_raw) {
            printf("無法開啟檔案！\n");
            return -1;
        }

        fseek(f_raw, 0, SEEK_END);
        long fileSize = ftell(f_raw);
        fseek(f_raw, 0, SEEK_SET);

        printf("檔案大小: %ld bytes\n", fileSize);

        width = (int)fileSize;
        height = 1;
        channels = 1;
        size = width;

        data_buffer = (int*)malloc(size * sizeof(int));
        unsigned char* temp_read_buf = (unsigned char*)malloc(size);

        fread(temp_read_buf, 1, size, f_raw);
        fclose(f_raw);

        for (int i = 0; i < size; i++) {
            data_buffer[i] = (int)temp_read_buf[i];
        }
        free(temp_read_buf);
        
    } else {
        printf("無效的選項\n");
        return -1;
    }

    // LZ77 + Huffman
    printf("執行LZ77\n");
    int lz_output_count = 0;
    LZ77Symbol* lz_stream = LZ77(data_buffer, size, &lz_output_count);
    free(data_buffer);

    if (lz_stream == NULL) {
        printf("LZ77 失敗\n");
        return -1;
    }

    // 轉換為 Huffman 符號流
    int* ll_stream = (int*)malloc((lz_output_count + 1) * sizeof(int));
    int* dist_stream = (int*)malloc(lz_output_count * sizeof(int));
    int ll_count = 0;
    int dist_count = 0;

    for(int i = 0; i < lz_output_count; i++){
        if(lz_stream[i].type == LITERAL){
            ll_stream[ll_count++] = lz_stream[i].value;
        }
        else{
            int length_code = get_length_code(lz_stream[i].value);
            ll_stream[ll_count++] = (length_code - 257) + DPCM_ALPHABET_SIZE;
            dist_stream[dist_count++] = get_distance_code(lz_stream[i].distance);
        }
    }
    ll_stream[ll_count++] = DPCM_ALPHABET_SIZE - 1;

    printf("執行Huffman\n");
    // 建立 Huffman 樹
    int ll_freq[COMP_ALPHABET_SIZE] = {0};
    int ll_symbol_count = 0;
    PixelDelta** ll_node_list = frequency_calculation(ll_count, ll_stream, ll_freq, &ll_symbol_count);
    PixelDelta* ll_root = Huffman(ll_node_list, ll_symbol_count);

    int dist_freq[COMP_ALPHABET_SIZE] = {0};
    int dist_symbol_count = 0;
    PixelDelta** dist_node_list = frequency_calculation(dist_count, dist_stream, dist_freq, &dist_symbol_count);
    PixelDelta* dist_root = NULL;
    if (dist_count > 0) {
        dist_root = Huffman(dist_node_list, dist_symbol_count);
    }

    // 產生編碼表
    char* ll_codes[COMP_ALPHABET_SIZE] = {NULL};
    char code_buffer[COMP_ALPHABET_SIZE];
    generate_codes(ll_root, code_buffer, 0, ll_codes);

    char* dist_codes[COMP_ALPHABET_SIZE] = {NULL};
    if(dist_root) generate_codes(dist_root, code_buffer, 0, dist_codes);

    char output_filename[MAX_PATH] = "";

    // 產生一個預設檔名建議
    char default_name[MAX_PATH];
    const char* base_name = strrchr(input_filepath, '\\');
    if (!base_name) base_name = input_filepath; else base_name++;
    strcpy(default_name, base_name);
    char* dot = strrchr(default_name, '.');
    if (dot) *dot = '\0';
    strcat(default_name, ".huf");

    printf("\n準備存檔\n");
    // 呼叫另存新檔視窗
    if (!save_file_dialog(output_filename, default_name, "HUF Compressed Files\0*.huf\0All Files\0*.*\0", "huf")) {
        printf("已取消存檔。\n");
        return 0;
    }
        
    // 輸出檔案
    long long compressed_bytes = compressed_archives_LZ(
        fileType, width, height, channels, 
        lz_output_count, lz_stream, 
        ll_codes, dist_codes, 
        ll_freq, ll_symbol_count,
        dist_freq, dist_symbol_count,
        output_filename
    );

    if (compressed_bytes != -1) {
        // 計算並顯示壓縮率
        long long original_size_bytes = (long long)width * height * channels;
        double ratio = ((double)compressed_bytes / original_size_bytes) * 100.0;

        printf("\n--- 壓縮效能分析 ---\n");
        printf("原始影像/檔案大小: %lld bytes\n", original_size_bytes);
        printf("壓縮檔案大小: %lld bytes\n", compressed_bytes);
        printf("壓縮比率:     %.2f%%\n", ratio);
        printf("節省空間:     %.2f%%\n", 100.0 - ratio);
    }

    // 清理
    free(lz_stream);
    free(ll_stream);
    free(dist_stream);
    free_huffman_tree(ll_root);
    free_huffman_tree(dist_root);
    free(ll_node_list);
    free(dist_node_list);
    for (int i = 0; i < COMP_ALPHABET_SIZE; i++) {
        if (ll_codes[i]) free(ll_codes[i]);
        if (dist_codes[i]) free(dist_codes[i]);
    }
    
    if (compressed_bytes != -1) {
        printf("\n壓縮完成！檔案大小: %lld bytes\n", compressed_bytes);
    }
    system("Pause");
    return 0;
}