
# Universal Data & Image Compressor (基於 DCT/DPCM 與 LZ77 的全能壓縮系統)

![Language](https://img.shields.io/badge/Language-C%20%2F%20Python-blue)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)
![License](https://img.shields.io/badge/License-MIT-green)

這是一個由 **C 語言** 開發的高效能混合式壓縮系統。它結合了 **訊號處理 (DCT/DPCM)** 與 **資料結構 (LZ77/Huffman)** 技術，能夠針對不同類型的檔案（照片、醫學影像、文字檔）自動切換最佳的壓縮策略。

本專案實作了從底層的位元流操作、霍夫曼樹建構到上層的 GUI 檔案選擇器的完整流程，並整合 Python 腳本以支援廣泛的影像格式轉檔。

## 🚀 專案亮點 (Key Features)

本系統支援三種核心壓縮模式，滿足不同場景需求：

| 模式 | 名稱 | 適用場景 | 核心技術 | 特性 |
| :--- | :--- | :--- | :--- | :--- |
| **Mode 1** | **DCT 失真壓縮** | 自然照片、風景圖 | DCT 轉換 + 量化 + ZigZag | 高壓縮率，微小失真 (類 JPEG) |
| **Mode 2** | **DPCM 無損壓縮** | 醫學影像、截圖、漫畫 | 空間差分 (Spatial DPCM) | 100% 還原，利用像素空間相關性 |
| **Mode 3** | **通用無損壓縮** | 文字檔、執行檔 (.txt, .exe) | 純 LZ77 + Huffman | 100% 還原，通用於任意二進位檔 |

* **混合語言架構**：核心演算法使用 **C** 撰寫以追求效能；影像前處理整合 **Python (Pillow)**，自動將 HEIC/TIFF 等特殊格式轉為標準 RGB 數據。
* **自定義檔案格式 (.huf)**：設計了包含 Magic Number (`HUFZ`) 與完整檔頭的二進位格式。
* **完整演算法實作**：不依賴壓縮庫，手刻 LZ77 滑動視窗 (Sliding Window) 與霍夫曼編碼 (Huffman Coding)。
* **視覺化品質分析**：內建 MSE (均方誤差) 計算與完整性驗證工具。

## 🛠️ 系統架構 (System Architecture)

### 1. 壓縮流程 (Compression Pipeline)
```mermaid
graph TD
    A[輸入檔案] --> B{選擇模式}
    B -->|Mode 1: DCT| C[色彩分離 -> DCT 轉換 -> 量化 -> ZigZag]
    B -->|Mode 2: DPCM| D[色彩分離 -> 空間差分 (Current-Prev)]
    B -->|Mode 3: Raw| E[讀取原始 Bytes]
    C --> F[LZ77 編碼 (滑動視窗)]
    D --> F
    E --> F
    F --> G[Huffman 統計與建樹]
    G --> H[寫入 .huf 檔案]


### 2. 核心技術細節

* **DCT (Discrete Cosine Transform)**：將影像從空間域轉為頻率域，去除人眼不敏感的高頻訊號。
* **LZ77**：使用貪婪演算法 (Greedy Algorithm) 尋找重複出現的字串模式，將其替換為 `(Distance, Length)` 標記。
* **Huffman Coding**：根據 LZ77 輸出的符號頻率建立二元樹，高頻符號使用短編碼，低頻符號使用長編碼。

## 📦 安裝與編譯 (Installation)

### 前置需求

* GCC 編譯器 (MinGW-w64)
* Python 3.x (需安裝 Pillow: `pip install Pillow`)
* Windows 作業系統 (使用 `windows.h` 呼叫原生視窗)

### 編譯指令

專案包含多個模組，請使用以下指令進行編譯：

```bash
# 編譯壓縮器
gcc compress.c -o compress.exe -lm -O2

# 編譯解壓縮器
gcc decompress.c -o decompress.exe -lm -O2

```

## 📖 使用說明 (Usage)

### 1. 壓縮 (Compress)

執行 `compress.exe`，依照選單操作：

1. 選擇壓縮模式 (1: DCT, 2: DPCM, 3: Raw)。
2. 選擇來源檔案 (支援拖曳或視窗選擇)。
* *若選擇特殊影像格式 (如 .heic)，系統會自動呼叫 Python 進行轉檔。*


3. 程式會自動生成 `.huf` 壓縮檔並顯示壓縮率。

### 2. 解壓縮 (Decompress)

執行 `decompress.exe`：

1. 選擇 `.huf` 檔案。
2. 系統會自動辨識檔頭中的模式標記 ('D', 'L', 'G')。
3. 解碼完成後，選擇存檔路徑。
* **影像模式**：詢問是否與原圖比對計算 MSE 或驗證完整性。
* **通用模式**：直接還原原始檔案。



## 📂 專案結構 (File Structure)

```text
Project/
├── compress.c              # 壓縮主程式 (Main Logic)
├── decompress.c            # 解壓縮主程式 & 驗證工具
├── dct_compression.h       # DCT 演算法、量化表、ZigZag 掃描
├── LZ77.h                  # LZ77 演算法 (Sliding Window implementation)
├── huffman.h               # [核心] 霍夫曼樹結構定義 (PixelDelta) 與建樹演算法
├── huffman_compression.h   # [應用] 霍夫曼編碼表生成 (Generate Codes) 與 I/O
├── bitstream.h             # 位元流讀寫操作 (BitWriter/BitReader)
├── choose_file.h           # Windows 原生檔案選擇視窗 & Python 橋接器
├── deflate.h               # Deflate 標準長度/距離表
├── universal_converter.py  # Python 影像格式轉檔腳本
├── stb_image.h             # 影像讀取庫 (Third-party)
├── stb_image_write.h       # 影像寫入庫 (Third-party)
└── README.md

```

## 📊 效能範例 (Performance)

測試環境：Windows 10, Intel i7

| 測試檔案 | 原始大小 | 模式 | 壓縮後大小 | 壓縮率 | 節省空間 | 品質驗證 |
| --- | --- | --- | --- | --- | --- | --- |
| **BaboonRGB.bmp** | 769 KB | Mode 1 (DCT) | 222 KB | 28.67% | 71.33% | MSE: 108.7293 |
| **BaboonRGB.bmp** | 769 KB | Mode 2 (DPCM) | 450 KB | 85.39% | 14.61% | **Pass (100% Match)** |
| **Lyrics_GangNamStyle.txt** | 15 KB | Mode 3 (Raw) | 4 KB | 24.81% | 75.19% | **Pass (100% Match)** |

