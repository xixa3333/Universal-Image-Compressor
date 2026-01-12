#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffman.h"
#include "bitstream.h"

//產生霍夫曼編碼
void generate_codes(PixelDelta* tree, char* code, int depth, char** huffman_codes){
    if (tree == NULL) return;
    if(tree->right_node == NULL && tree->left_node == NULL){
        code[depth] = '\0';
        
        huffman_codes[tree->gray] = (char*)malloc(sizeof(char) * (depth + 1));
        strcpy(huffman_codes[tree->gray], code);
        return;
    }
    
    code[depth] = '0';
    generate_codes(tree->left_node, code, depth + 1, huffman_codes);
    
    code[depth] = '1';
    generate_codes(tree->right_node, code, depth + 1, huffman_codes);
}

// 將霍夫曼碼寫入位元流
void write_bits(FILE* f_out, const char* code, unsigned char* byte_buffer, int* bit_count, long long* total_bits) {
    for (int i = 0; code[i] != '\0'; i++) {
        (*byte_buffer) <<= 1;
        if (code[i] == '1') (*byte_buffer) |= 1;
        (*bit_count)++;
        (*total_bits)++;

        if (*bit_count == 8) {
            fwrite(byte_buffer, 1, 1, f_out);
            *byte_buffer = 0;
            *bit_count = 0;
        }
    }
}

// 使用霍夫曼樹解碼一個符號
int decode_symbol(PixelDelta* root, BitReader* reader) {
    PixelDelta* node = root;
    while (node->left_node != NULL) { // 只要不是葉節點
        int bit = read_bit(reader);
        if (bit == 0) {
            node = node->left_node;
        } else {
            node = node->right_node;
        }
        if (node == NULL) {
            printf("錯誤：霍夫曼樹解碼時遇到 NULL 節點！\n");
            return -1; // 樹結構錯誤
        }
    }
    // 抵達葉節點
    return node->gray;
}

// 從檔案讀取稀疏頻率表並重建霍夫曼樹
PixelDelta* rebuild_huffman_tree(FILE* f_in, int* out_symbol_count, PixelDelta*** out_node_list) {
    int symbol_count = 0;
    
    // 1. 讀取有多少個符號
    if (fread(&symbol_count, sizeof(int), 1, f_in) != 1) return NULL;
    *out_symbol_count = symbol_count;

    if (symbol_count == 0) {
        *out_node_list = NULL;
        return NULL;
    }

    // 2. 分配節點列表
    PixelDelta** node_list = (PixelDelta**)malloc(symbol_count * sizeof(PixelDelta*));
    if (!node_list) return NULL;
    *out_node_list = node_list;

    // 3. 讀取索引, 頻率並建立節點
    for (int i = 0; i < symbol_count; i++) {
        unsigned short symbol_index = 0;
        int frequency = 0;
        
        if (fread(&symbol_index, sizeof(unsigned short), 1, f_in) != 1) return NULL;
        if (fread(&frequency, sizeof(int), 1, f_in) != 1) return NULL;

        PixelDelta* newNode = (PixelDelta*)malloc(sizeof(PixelDelta));
        newNode->gray = (int)symbol_index;
        newNode->frequency = frequency;
        newNode->left_node = NULL;
        newNode->right_node = NULL;
        node_list[i] = newNode;
    }

    PixelDelta* root = Huffman(node_list, symbol_count);

    return root;
}