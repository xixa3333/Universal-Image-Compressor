// 將 Length (3-258) 轉換為 L/L 符號碼 (257-285)
// 這是 Deflate 演算法的標準查找表
int get_length_code(int length) {
    if (length == 258) return 285;
    if (length < 11) return length - 3 + 257;
    if (length < 19) return (length - 11) / 2 + 265;
    if (length < 35) return (length - 19) / 4 + 269;
    if (length < 67) return (length - 35) / 8 + 273;
    if (length < 131) return (length - 67) / 16 + 277;
    if (length < 258) return (length - 131) / 32 + 281;
    return 285;
}

// 將 Distance (1-32768) 轉換為 D 符號碼 (0-29)
// 這是 Deflate 演算法的標準查找表
int get_distance_code(int distance) {
    if (distance < 5) return distance - 1 + 0;
    if (distance < 9) return (distance - 5) / 2 + 4;
    if (distance < 17) return (distance - 9) / 4 + 6;
    if (distance < 33) return (distance - 17) / 8 + 8;
    if (distance < 65) return (distance - 33) / 16 + 10;
    if (distance < 129) return (distance - 65) / 32 + 12;
    if (distance < 257) return (distance - 129) / 64 + 14;
    if (distance < 513) return (distance - 257) / 128 + 16;
    if (distance < 1025) return (distance - 513) / 256 + 18;
    if (distance < 2049) return (distance - 1025) / 512 + 20;
    if (distance < 4097) return (distance - 2049) / 1024 + 22;
    if (distance < 8193) return (distance - 4097) / 2048 + 24;
    if (distance < 16385) return (distance - 8193) / 4096 + 26;
    if (distance < 32769) return (distance - 16385) / 8192 + 28;
    return 29; // > 16385
}

const int length_extra_bits_table[] = { 
    0,0,0,0, 0,0,0,0, 1,1, 1,1, 2,2, 2,2, 3,3, 3,3, 4,4, 4,4, 5,5, 5,5, 0 
};
const int length_base_table[] = { 
    3,4,5,6, 7,8,9,10, 11,13, 15,17, 19,23, 27,31, 35,43, 51,59, 67,83, 99,115, 131,163, 195,227, 258 
};
const int dist_extra_bits_table[] = { 
    0,0,0,0, 1,1, 2,2, 3,3, 4,4, 5,5, 6,6, 7,7, 8,8, 9,9, 10,10, 11,11, 12,12, 13,13 
};
const int dist_base_table[] = { 
    1,2,3,4, 5,7, 9,13, 17,25, 33,49, 65,97, 129,193, 257,385, 513,769, 1025,1537, 2049,3073, 4097,6145, 8193,12289, 16385,24577 
};