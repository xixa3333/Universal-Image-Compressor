#include <stdio.h>
#include <stdlib.h>

typedef struct HuffmanNode{
    int gray;                   // 符號值
    int frequency;              // 頻率
    struct HuffmanNode* right_node;
    struct HuffmanNode* left_node;
} PixelDelta;

//交換數值
void swap(PixelDelta** A,PixelDelta** B){
    PixelDelta* temp = *A;
    *A = *B;
    *B = temp;
}

//父節點向下交換變回最小heap
void min_heapify(PixelDelta** Grayscale,int index,int heap_size){
    int left = index * 2 + 1;
    int right = index * 2 + 2;
    int smallest = index;

    if(left < heap_size && Grayscale[smallest]->frequency > Grayscale[left]->frequency)smallest = left;
    if(right < heap_size && Grayscale[smallest]->frequency > Grayscale[right]->frequency)smallest = right;

    if(smallest != index){
        swap(&Grayscale[index],&Grayscale[smallest]);
        min_heapify(Grayscale,smallest,heap_size);
    }
}

//建立最小heap
void build_min_heap(PixelDelta** Grayscale,int heap_size){
    for (int i = (heap_size - 2) / 2; i >= 0; i --) min_heapify(Grayscale,i,heap_size);
}

//將最上面的節點取出
PixelDelta* extract_min(PixelDelta** Grayscale,int* heap_size){
    PixelDelta* min_node = Grayscale[0];
    swap(&Grayscale[0],&Grayscale[*(heap_size)-1]);
    (*heap_size) --;
    min_heapify(Grayscale,0,*heap_size);
    return min_node;
}

//向上更新
void heap_up(PixelDelta** Grayscale,int index){
    int parent = (index - 1) / 2;

    if(index != 0 && Grayscale[parent]->frequency > Grayscale[index]->frequency) {
        swap(&Grayscale[parent],&Grayscale[index]);
        heap_up(Grayscale,parent);
    }
}

//把霍夫曼結合的節點放入最小heap最後面並更新
void insert_node(PixelDelta** Grayscale,PixelDelta* node,int* heap_size){
    (*heap_size) ++;
    Grayscale[*heap_size-1] = node;
    heap_up(Grayscale,*heap_size-1);
}

//建立霍夫曼樹
PixelDelta* Huffman(PixelDelta** Grayscale,int lenth){
    int heap_size = lenth;
    build_min_heap(Grayscale,heap_size);

    while(heap_size > 1){
        PixelDelta* Huffman_right = extract_min(Grayscale,&heap_size);
        PixelDelta* Huffman_left = extract_min(Grayscale,&heap_size);

        PixelDelta* head = (PixelDelta*)malloc(sizeof(PixelDelta));
        head->frequency = Huffman_right->frequency + Huffman_left->frequency;
        head->gray = -1;
        head->right_node = Huffman_right;
        head->left_node = Huffman_left;
        
        insert_node(Grayscale,head,&heap_size);
    }
    return Grayscale[0];
}

//將霍夫曼樹釋放
void free_huffman_tree(PixelDelta* tree){
    if (tree == NULL) return;
    free_huffman_tree(tree->right_node);
    free_huffman_tree(tree->left_node);
    free(tree);
}