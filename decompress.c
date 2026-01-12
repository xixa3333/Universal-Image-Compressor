#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "huffman_compression.h"
#include "deflate.h"
#include "choose_file.h"
#include "dct_compression.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define DPCM_ALPHABET_SIZE 512
#define Search_Buffer 4096
#define MAX_LENGTH 258

// 解碼串流函式 (LZ77 + Huffman)
long decompress_stream(
    int** output_buffer_ptr, 
    long* output_capacity_ptr,
    PixelDelta* ll_root,
    PixelDelta* dist_root,
    BitReader* reader) 
{
    int* output_buffer = *output_buffer_ptr;
    long output_capacity = *output_capacity_ptr;
    long output_size = 0;

    while (1) {
        if (output_size >= output_capacity - MAX_LENGTH) { 
            output_capacity *= 2;
            int* new_buffer = (int*)realloc(output_buffer, output_capacity * sizeof(int));
            if (!new_buffer) return -1;
            output_buffer = new_buffer; 
            *output_buffer_ptr = output_buffer;
            *output_capacity_ptr = output_capacity;
        }

        int ll_symbol = decode_symbol(ll_root, reader);
        if (ll_symbol == -1) return -1;
        if (ll_symbol == DPCM_ALPHABET_SIZE - 1) break;

        if (ll_symbol < DPCM_ALPHABET_SIZE) {
            output_buffer[output_size++] = ll_symbol;
        } else {
            if (!dist_root) return -1;

            int length_code = (ll_symbol - DPCM_ALPHABET_SIZE) + 257;
            int len_extra_bits = length_extra_bits_table[length_code - 257];
            int len_extra_value = 0;
            if (len_extra_bits > 0) len_extra_value = read_n_bits(reader, len_extra_bits);
            int length = length_base_table[length_code - 257] + len_extra_value;

            int distance_code = decode_symbol(dist_root, reader);
            if (distance_code == -1) return -1;
            int dist_extra_bits = dist_extra_bits_table[distance_code];
            int dist_extra_value = 0;
            if (dist_extra_bits > 0) dist_extra_value = read_n_bits(reader, dist_extra_bits);
            int distance = dist_base_table[distance_code] + dist_extra_value;

            long start_pos = output_size - distance; 
            if (start_pos < 0) return -1;
            for (int i = 0; i < length; i++) {
                output_buffer[output_size++] = output_buffer[start_pos + i];
            }
        }
    }
    return output_size;
}

// 計算 MSE
void calculate_distortion(unsigned char* original, unsigned char* restored, int width, int height, int channels) {
    long long total_pixels = width * height * channels;
    double mse = 0.0;

    for (long long i = 0; i < total_pixels; i++) {
        int diff = original[i] - restored[i];
        mse += diff * diff;
    }
    mse /= (double)total_pixels;

    printf("\n---影像品質分析---\n");
    if (mse == 0) {
        printf("MSE: 0.00 (完美無失真)\n");
    } else {
        printf("MSE: %.4f\n", mse);
    }
}

int main() {
    SetConsoleOutputCP(65001);
    
    char input_filepath[MAX_PATH] = "";
    if (!choose_huf_file(input_filepath)) return -1;

    FILE* f_in = fopen(input_filepath, "rb");
    if (!f_in) {
        printf("無法開啟檔案\n");
        return -1;
    }

    char magic[4];
    char fileType = 0;
    int width = 0, height = 0;
    int channels = 0;

    // 讀取檔頭
    fread(magic, sizeof(char), 4, f_in);
    if (magic[0] != 'H' || magic[1] != 'U' || magic[2] != 'F' || magic[3] != 'Z') {
        printf("檔案格式錯誤 (Magic Number Error)\n");
        fclose(f_in); return -1;
    }

    fread(&fileType, sizeof(char), 1, f_in);
    fread(&width, sizeof(int), 1, f_in);
    fread(&height, sizeof(int), 1, f_in);
    fread(&channels, sizeof(int), 1, f_in);

    // 檢查支援的類型
    if (fileType != 'D' && fileType != 'L' && fileType != 'G') {
        printf("錯誤：未知的壓縮類型 '%c'\n", fileType);
        fclose(f_in); return -1;
    }

    if (fileType == 'G') {
        printf("檔案資訊: 通用無損檔案 (Raw), 大小: %d bytes\n", width);
    } else {
        printf("檔案資訊: %s 影像, 尺寸 %dx%d\n", (fileType == 'D' ? "DCT" : "DPCM"), width, height);
    }

    // 重建 Huffman 樹
    int ll_symbol_count = 0, dist_symbol_count = 0;
    PixelDelta** ll_node_list_ptr = NULL;
    PixelDelta** dist_node_list_ptr = NULL;
    
    PixelDelta* ll_root = rebuild_huffman_tree(f_in, &ll_symbol_count, &ll_node_list_ptr);
    if (!ll_root) { fclose(f_in); return -1; }
    
    PixelDelta* dist_root = rebuild_huffman_tree(f_in, &dist_symbol_count, &dist_node_list_ptr);

    // 執行 LZ77 解碼
    BitReader* reader = create_bit_reader(f_in);
    long output_capacity = 1024 * 1024;
    int* decoded_stream = (int*)malloc(output_capacity * sizeof(int));
    
    printf("正在解碼 Huffman 與 LZ77\n");
    long output_size = decompress_stream(&decoded_stream, &output_capacity, ll_root, dist_root, reader);
    fclose(f_in);

    if (output_size == -1) {
        printf("解碼失敗 (Data Corrupted)\n");
        free(decoded_stream);
        return -1;
    }

    // 根據 fileType 進行資料還原

    // === CASE 1: 通用檔案 (G) ===
    if (fileType == 'G') {
        printf("偵測到通用檔案\n");
        printf("請輸入要儲存的完整檔名 (包含副檔名, 例如 file.txt 或 app.exe):\n");
        
        char output_filename[MAX_PATH] = "";
        if (save_any_file_dialog(output_filename)) {
            FILE* f_out = fopen(output_filename, "wb");
            if (f_out) {
                unsigned char* write_buffer = (unsigned char*)malloc(width);
                for(int i=0; i<width; i++) {
                    write_buffer[i] = (unsigned char)decoded_stream[i];
                }
                fwrite(write_buffer, 1, width, f_out);
                fclose(f_out);
                free(write_buffer);
                printf("檔案還原成功: %s\n", output_filename);
            } else {
                printf("存檔失敗\n");
            }
        }
        
        // 通用檔案結束程式 (不需要算 MSE)
        free(decoded_stream);
        free_huffman_tree(ll_root);
        if(ll_node_list_ptr) free(ll_node_list_ptr);
        free_huffman_tree(dist_root);
        if(dist_node_list_ptr) free(dist_node_list_ptr);
        if(reader) free(reader);
        system("Pause");
        return 0;
    }

    // === CASE 2 & 3: 影像檔案 (D 或 L) ===
    unsigned char* output_img = (unsigned char*)malloc(width * height * channels);
    int new_w = (width + 7) & ~7;
    int new_h = (height + 7) & ~7;

    if (fileType == 'D') {
        // DCT 模式 (IDCT)
        printf("正在執行 IDCT\n");
        int buffer_idx = 0;
        unsigned char block_pixels[8][8];
        
        for (int by = 0; by < new_h; by += 8) {
            for (int bx = 0; bx < new_w; bx += 8) {
                for (int k = 0; k < channels; k++) {
                    // IDCT 處理
                    process_block_idct(decoded_stream, buffer_idx, block_pixels);
                    buffer_idx += 64;

                    // 填入像素
                    for (int y = 0; y < 8; y++) {
                        for (int x = 0; x < 8; x++) {
                            int ix = bx + x; 
                            int iy = by + y;
                            if (ix < width && iy < height) {
                                output_img[(iy * width + ix) * channels + k] = block_pixels[x][y];
                            }
                        }
                    }
                }
            }
        }
    } 
    else if (fileType == 'L') {
        // DPCM 模式 (反差分)
        printf("執行反差分還原像素\n");
        int buffer_idx = 0;

        for (int by = 0; by < new_h; by += 8) {
            for (int bx = 0; bx < new_w; bx += 8) {
                for (int k = 0; k < channels; k++) {
                    
                    int prev_val = 0;
                    
                    for (int y = 0; y < 8; y++) {
                        for (int x = 0; x < 8; x++) {
                            // 1. 讀取平移後的差值
                            int mapped_diff = decoded_stream[buffer_idx++];
                            
                            // 2. 還原真實差值 (扣除 256)
                            int diff = mapped_diff - 256;
                            
                            // 3. 還原像素
                            int val = prev_val + diff;
                            
                            // 4. 防止溢位
                            if (val < 0) val = 0;
                            if (val > 255) val = 255;
                            
                            // 5. 寫入
                            int ix = bx + x; 
                            int iy = by + y;
                            if (ix < width && iy < height) {
                                output_img[(iy * width + ix) * channels + k] = (unsigned char)val;
                            }
                            
                            // 6. 更新預測值
                            prev_val = val;
                        }
                    }
                }
            }
        }
    }

    // 影像存檔與分析
    char output_filename[MAX_PATH] = "";
    printf("\n解壓縮完成，請選擇儲存位置 (.png)\n");

    if (save_file_dialog(output_filename, "restored.png", "PNG Image\0*.png\0", "png")) {
        if (stbi_write_png(output_filename, width, height, channels, output_img, width * channels)) {
            printf("解壓縮成功！影像已儲存至: %s\n", output_filename);
            if (fileType == 'D') {
                // 計算失真率 (可選)
                printf("是否進行原圖比對? (請按任意鍵繼續，或直接關閉視窗)\n");
                system("Pause");

                char original_path[MAX_PATH] = "";
                int orig_w, orig_h, orig_c;
                printf("請選擇原始未壓縮的影像檔案\n");
                unsigned char* original_img = choose_img(&orig_w, &orig_h, &orig_c, original_path);

                if (original_img) {
                    if (orig_w != width || orig_h != height) {
                        printf("錯誤：尺寸不符，無法比對。\n");
                    } else {
                        calculate_distortion(original_img, output_img, width, height, channels);
                    }
                    stbi_image_free(original_img);
                }
            }
        } 
        else {
            printf("影像存檔失敗\n");
        }
    }

    // 清理記憶體
    free(decoded_stream);
    free(output_img);
    free(reader);
    free_huffman_tree(ll_root);
    if (ll_node_list_ptr) free(ll_node_list_ptr);
    free_huffman_tree(dist_root);
    if (dist_node_list_ptr) free(dist_node_list_ptr);

    system("Pause");
    return 0;
}