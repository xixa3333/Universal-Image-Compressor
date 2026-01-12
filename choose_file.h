#ifndef CHOOSE_FILE_H
#define CHOOSE_FILE_H

#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include "stb_image.h" 

#pragma comment(lib, "comdlg32.lib")

// 取得 data 資料夾路徑
void get_data_dir_path(char* buffer, int size) {
    if (GetCurrentDirectory(size, buffer)) {
        strncat(buffer, "\\data", size - strlen(buffer) - 1);
    } else {
        strncpy(buffer, ".\\data", size);
    }
}

// 另存新檔視窗
int save_file_dialog(char* buffer, const char* default_name, const char* filter, const char* def_ext) {
    OPENFILENAME ofn;
    char szFile[MAX_PATH] = "";
    strncpy(szFile, default_name, sizeof(szFile) - 1);
    char initialDir[MAX_PATH];
    get_data_dir_path(initialDir, MAX_PATH);

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = initialDir;
    ofn.lpstrDefExt = def_ext;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileName(&ofn)) {
        strcpy(buffer, ofn.lpstrFile);
        return 1;
    }
    return 0;
}

// 呼叫 Python 進行轉檔
int try_universal_convert(const char* input_path, char* temp_output_path) {
    char cmd[1024];

    // 取得 exe 所在的目錄
    char current_dir[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, current_dir);
    
    sprintf(cmd, "python \"%s\\universal_converter.py\" \"%s\" > \"%s\\temp_convert_log.txt\"", current_dir, input_path, current_dir);
    
    printf("正在嘗試使用轉檔\n");
    int result = system(cmd);
    
    // 讀取 Log 檢查是否成功
    FILE* f = fopen("temp_convert_log.txt", "r");
    if (!f) return 0;

    char buffer[512];
    if (fgets(buffer, sizeof(buffer), f)) {
        if (strncmp(buffer, "SUCCESS|", 8) == 0) {
            // 抓出檔名
            char* path = buffer + 8;
            path[strcspn(path, "\r\n")] = 0; // 去除換行
            strcpy(temp_output_path, path);
            fclose(f);
            return 1;
        } else {
            printf("Python 轉檔回報錯誤: %s", buffer);
        }
    }
    fclose(f);
    return 0;
}

// 選擇壓縮檔
int choose_huf_file(char* file_path_buffer) {
    OPENFILENAME ofn;
    char szFile[MAX_PATH] = "";
    char initialDir[MAX_PATH];
    get_data_dir_path(initialDir, MAX_PATH);

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "HUF Compressed Files\0*.huf\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = initialDir;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileName(&ofn)) {
        strcpy(file_path_buffer, ofn.lpstrFile);
        return 1;
    }
    return 0;
}

// 影像選擇器
unsigned char* choose_img(int* width, int* height, int* channels, char* file_path_buffer) {
    OPENFILENAME ofn;
    char szFile[MAX_PATH] = "";
    char initialDir[MAX_PATH];
    get_data_dir_path(initialDir, MAX_PATH);

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    
    // 允許選擇所有檔案
    ofn.lpstrFilter = "All Image Files\0*.*\0Supported Files\0*.png;*.bmp;*.jpg;*.tif\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = initialDir;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileName(&ofn)) {
        strcpy(file_path_buffer, ofn.lpstrFile);
        printf("已選擇檔案: %s\n", file_path_buffer);
        
        // 1. 先嘗試直接讀取
        unsigned char* img = stbi_load(file_path_buffer, width, height, channels, 3);
        
        // 2. 如果讀取失敗，則呼叫 Python 轉檔
        if (img == NULL) {
            printf("原生讀取失敗：(%s)，嘗試啟動轉檔器\n", stbi_failure_reason());
            
            char temp_png_path[MAX_PATH] = "";
            if (try_universal_convert(file_path_buffer, temp_png_path)) {
                
                // 讀取轉好的暫存檔
                img = stbi_load(temp_png_path, width, height, channels, 3);
                
                if (img) {
                    printf("轉檔成功！已載入影像。\n");
                    // 讀完後刪除暫存檔
                    remove(temp_png_path);
                    remove("temp_convert_log.txt");
                } else {
                    printf("轉檔後依然讀取失敗\n");
                    system("Pause");
                }
            } else {
                printf("轉檔失敗 請確認已安裝 Python 與 Pillow\n");
                system("Pause");
            }
        }
        
        if (img != NULL) {
            *channels = 3;
            return img;
        }
    }
    
    return NULL;
}

// 通用開啟檔案
int choose_any_file(char* file_path_buffer) {
    OPENFILENAME ofn;
    char szFile[MAX_PATH] = "";
    char initialDir[MAX_PATH];
    get_data_dir_path(initialDir, MAX_PATH);

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0Text Files\0*.txt\0"; 
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = initialDir;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileName(&ofn)) {
        strcpy(file_path_buffer, ofn.lpstrFile);
        return 1;
    }
    return 0;
}

// 通用存檔 (讓使用者自己輸入副檔名)
int save_any_file_dialog(char* file_path_buffer) {
    OPENFILENAME ofn;
    char szFile[MAX_PATH] = "restored_file.bin";
    char initialDir[MAX_PATH];
    get_data_dir_path(initialDir, MAX_PATH);

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = initialDir;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileName(&ofn)) {
        strcpy(file_path_buffer, ofn.lpstrFile);
        return 1;
    }
    return 0;
}

#endif