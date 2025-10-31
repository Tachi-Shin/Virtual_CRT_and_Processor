import pygame
import serial
import time

# シリアル通信の設定
SERIAL_PORT = 'COM10'  # Raspberry Pi Pico Wが接続されたポート
BAUD_RATE = 12000000  # ボーレート
VRAM_WIDTH = 225
VRAM_HEIGHT = 217
PIXEL_SIZE = 4  # ピクセルの拡大サイズ（これを調整して大きさを変える）

# Pygameの初期化
pygame.init()
screen = pygame.display.set_mode((VRAM_WIDTH * PIXEL_SIZE, VRAM_HEIGHT * PIXEL_SIZE))  # 画面サイズも変更
pygame.display.set_caption("Raspberry Pi Pico W VRAM Viewer")

# シリアル接続の初期化
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

def draw_vram(vram):
    """VRAMデータを画面に描画する"""
    for y in range(VRAM_HEIGHT):
        for x in range(VRAM_WIDTH):
            color = (255, 255, 255) if vram[y][x] == 1 else (0, 0, 0)
            pygame.draw.rect(screen, color, pygame.Rect(x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE))  # ピクセルを矩形として描画
    pygame.display.flip()

def send_character(char):
    """1文字をPico Wに送信"""
    ser.write(char.encode())

def receive_vram():
    """Pico WからVRAMデータを受信"""
    data = ser.read(VRAM_WIDTH * VRAM_HEIGHT)  # VRAM全体を一度に受信
    if len(data) == VRAM_WIDTH * VRAM_HEIGHT:
        # 1次元の受信データを2次元配列に変換
        vram = [[data[y * VRAM_WIDTH + x] for x in range(VRAM_WIDTH)] for y in range(VRAM_HEIGHT)]
        return vram
    return None

def main():
    clock = pygame.time.Clock()
    running = True

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                # キーが押されたらPico Wに送信
                send_character(event.unicode)

        # VRAMを受信して描画
        vram = receive_vram()
        if vram:
            draw_vram(vram)

        clock.tick(30)  # 最大30FPS

    ser.close()
    pygame.quit()

if __name__ == "__main__":
    # Pico Wが起動するのを待機
    time.sleep(2)
    main()
