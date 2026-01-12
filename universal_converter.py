import sys
import os
from PIL import Image

Image.MAX_IMAGE_PIXELS = None

def convert_to_temp_png(input_path):
    try:
        # 準備暫存檔名
        output_path = "temp_converted.png"
        
        # 開啟圖片
        with Image.open(input_path) as img:
            # 強制轉為 RGB
            img = img.convert('RGB')
            # 存為標準 PNG
            img.save(output_path, "PNG")
            
        # 告訴C成功了，並回傳暫存檔的路徑
        print(f"SUCCESS|{output_path}")
        
    except Exception as e:
        # 告訴C失敗原因
        print(f"ERROR|{e}")

if __name__ == "__main__":
    # 接收C傳來的檔案路徑
    if len(sys.argv) > 1:
        # 處理Windows路徑可能有的編碼問題
        input_file = sys.argv[1]
        convert_to_temp_png(input_file)