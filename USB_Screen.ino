#include <String.h>
#include "pico/multicore.h"
#include "font_data.h"

// レイヤーおよび文字列データ
byte VRAM_Layer[217][225]; // VRAMレイヤーのみ
byte TEXT_Layer[217][225];
char FontSet[24][32];      // 縦24文字×横32文字
char que[768];

int que_position = 0;
int cursor_x = 0, cursor_y = 0; // カーソル位置

volatile bool need_update = false; // 描画更新フラグ

void setup() {
  Serial.begin(12000000); // 12Mbps

  // レイヤーの初期化
  memset(VRAM_Layer, 0, sizeof(VRAM_Layer));
  memset(TEXT_Layer, 0, sizeof(TEXT_Layer));
  memset(FontSet, ' ', sizeof(FontSet)); // FontSet初期化
  memset(que, ' ', sizeof(que));
  que_position = 0;

  // コア1を起動
  multicore_launch_core1(core1_main);
}

void core1_main() {
  while (true) {
    if (need_update) {
      need_update = false;
      renderTextAndCursor(); // テキストとカーソルの描画
      mergeAndSendScreen();  // レイヤー統合と送信
    }
  }
}

void renderTextAndCursor() {
  // テキストレイヤーの初期化
  memset(TEXT_Layer, 0, sizeof(TEXT_Layer));

  // FontSet内容をテキストレイヤーに描画
  for (int y = 0; y < 24; y++) {
    for (int x = 0; x < 32; x++) {
      char c = FontSet[y][x];
      if (c < 32 || c > 126) continue;

      const byte *bitmap = font_data[c - 32];
      int base_x = x * 7 + 1;
      int base_y = y * 9 + 1;

      for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 6; col++) {
          if (bitmap[row] & (1 << (5 - col))) {
            TEXT_Layer[base_y + row][base_x + col] = 1;
          }
        }
      }
    }
  }

  // カーソルを描画
  int cursor_base_x = cursor_x * 7 + 1;
  int cursor_base_y = cursor_y * 9 + 8; // カーソル位置
  for (int col = 0; col < 6; col++) {
    TEXT_Layer[cursor_base_y][cursor_base_x + col] = 1;
  }
}

void mergeAndSendScreen() {
  // レイヤーを統合してVRAMに保存
  for (int y = 0; y < 217; y++) {
    for (int x = 0; x < 225; x++) {
      VRAM_Layer[y][x] = TEXT_Layer[y][x]; // DRAW_Layerとの統合は不要
    }
  }
  // 統合したVRAMを送信
  Serial.write((byte *)VRAM_Layer, sizeof(VRAM_Layer));
}

void DeleteOperate() {
  // VRAM, TEXT, FontSet のみ再初期化
  memset(VRAM_Layer, 0, sizeof(VRAM_Layer));
  memset(TEXT_Layer, 0, sizeof(TEXT_Layer));
  memset(FontSet, ' ', sizeof(FontSet));
  memset(que, ' ', sizeof(que));
  que_position = 0;
  cursor_x = 0;
  cursor_y = 0;
  need_update = true; // 再描画フラグ
}

void read_char(char c) {
  if (c == '\b') { // バックスペース
    if (que_position > 0) {
      que[--que_position] = ' ';
      if (cursor_x > 0) {
        FontSet[cursor_y][--cursor_x] = ' ';
      }
      need_update = true; // 再描画フラグ
    }
  } else if (c == 0x7F) { // Deleteキー
    DeleteOperate();
  } /*else if (c == '\n' || c == '\r') { // 改行文字 (LF または CR)
    cursor_x = 0; // 次の行の先頭へ移動
    if (++cursor_y >= 24) { // 行数が最大を超えたら
      cursor_y = 23; // 最後の行に固定
      scrollFontSet(); // スクロール処理
    }
    need_update = true; // 再描画フラグ
}*/ else if (c >= 32 && c <= 126) { // 表示可能文字
    if (que_position < 768) {
      que[que_position++] = c;
      FontSet[cursor_y][cursor_x++] = c;
      if (cursor_x >= 32) { // 行末を超えたら
        cursor_x = 0; // 次の行の先頭へ移動
        if (++cursor_y >= 24) {
          cursor_y = 23; // 最後の行に固定
          scrollFontSet();
        }
      }
      need_update = true; // 再描画フラグ
    } else {
      DeleteOperate();
    }
  }
}

// スクロール処理
void scrollFontSet() {
  for (int y = 0; y < 23; y++) { // 上に詰める
    memcpy(FontSet[y], FontSet[y + 1], 32);
  }
  memset(FontSet[23], ' ', 32); // 最後の行を空白にする
}


void loop() {
  if (Serial.available() > 0) {
    char input_text = Serial.read();
    Serial.println(input_text, HEX); // 16進数で表示
    read_char(input_text);
  }
}
