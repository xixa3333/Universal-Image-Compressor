#include <stdio.h>
#include <stdlib.h>

#define Search_Buffer 4096
#define MAX_LENGTH 258
#define MIN_LENGTH 3

//放LZ77找到的匹配字串
typedef struct {
    int length;
    int distance;
} Match;

//區分匹配/未匹配
typedef enum {
    LITERAL,
    POINTER
} SymbolType;

//放LZ77匹配/未匹配字串
typedef struct {
    SymbolType type;
    int value;    // 如果是 LITERAL, 這裡存 ASCII 碼
                  // 如果是 POINTER, 這裡存 Length
    int distance; // 如果是 POINTER, 這裡存 Distance
} LZ77Symbol;

//利用貪心找到最好的匹配字串
Match find_longest_match(int* data,int forward_first,int data_size){
    Match best_match = {0,0};
    int search_first = (forward_first > Search_Buffer) ? (forward_first - Search_Buffer) : 0;

    for(int search_current = search_first; search_current < forward_first; search_current++){
        if(data[search_current] != data[forward_first]) continue;
        int length = 1;

        while(forward_first + length < data_size && 
            length < MAX_LENGTH && 
            data[search_current + length] == data[forward_first + length]
        ){
            length++;
        }

        if(best_match.length < length){
            best_match.length = length;
            best_match.distance = forward_first - search_current;
        }
    }

    return best_match;
}

//移動緩衝區
LZ77Symbol* LZ77(int* data,int size,int* lz_output_index){
    LZ77Symbol* lz_output = (LZ77Symbol*)malloc(size * sizeof(LZ77Symbol));
    Match best_match = {0,0};

    for(int data_index = 0; data_index < size;){
        best_match = find_longest_match(data,data_index,size);

        if(best_match.length >= MIN_LENGTH){
            lz_output[*lz_output_index].type = POINTER;
            lz_output[*lz_output_index].value = best_match.length;
            lz_output[*lz_output_index].distance = best_match.distance;
            data_index += best_match.length;
        }
        else{
            lz_output[*lz_output_index].type = LITERAL;
            lz_output[*lz_output_index].value = data[data_index];
            lz_output[*lz_output_index].distance = 0;
            data_index++;
        }
        (*lz_output_index)++;
    }
    return lz_output;
}

