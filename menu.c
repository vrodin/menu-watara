#include <supervision.h>
#include <string.h>
#include <stdio.h>
#include "font.h"

#define CART_BUS (*(unsigned char *)0x8000)
#define WORDS_PER_LINE (160 / 8 + 4)

#define CHAR_ON_LINE 18
#define ROM_INFO_SIZE 22
#define GAMES_PER_PAGE 18

#define GAMECOUNT *((unsigned char*)0x9100)
#define GAMEINFO ((unsigned char*)0x9101)
#define GAMEINFO_SIZE 56

struct sv_vram
{
  unsigned int v[160 / 8][8][WORDS_PER_LINE];
};

#define SV_VRAM ((*(struct sv_vram *)0x4000).v)

static void clear_screen(void)
{
  memset(SV_VIDEO, 0, 0x2000);
}

/* Necessary conversion to have 2 bits per pixel with darkest hue */
/* Remark: The Supervision uses 2 bits per pixel, and bits are mapped into pixels in reversed order */
static unsigned int __fastcall__ double_bits(unsigned char)
{
  __asm__("stz ptr2");
  __asm__("stz ptr2+1");
  __asm__("ldy #$08");
L1:
  __asm__("lsr a");
  __asm__("php");
  __asm__("ror ptr2");
  __asm__("ror ptr2+1");
  __asm__("plp");
  __asm__("ror ptr2");
  __asm__("ror ptr2+1");
  __asm__("dey");
  __asm__("bne %g", L1);
  __asm__("lda ptr2+1");
  __asm__("ldx ptr2");
  return __AX__;
}

static void __fastcall__ beep()
{
  SV_LEFT.delay = 300;
  SV_LEFT.timer = 2;
  SV_LEFT.control = 0b00111111;
  SV_RIGHT.delay = 254;
  SV_RIGHT.timer = 2;
  SV_RIGHT.control = 0b00111111;
}

static void display_char(const unsigned char x, const unsigned char y, const unsigned char *ch, const char invert)
{
  unsigned char i, byte;

  for (i = 0; i < 8; ++i) {
    byte = ch[i];
    if (invert) byte = ~byte;
    SV_VRAM[y][i][x] = double_bits(byte);
  }
}

static void init(void)
{
  SV_LCD.width = 160;
  SV_LCD.height = 160;
}

static void print(char *str, int len, unsigned char x, unsigned char y, char invert)
{
  unsigned char i;

  for (i = 0; i < len; i++) {
    char character = *(str + i * sizeof(char));
    display_char(x + i, y, font + (character - 32) * sizeof(unsigned char) * 8, invert);
  }
}

void sleep(int count)
{
  while (count) {
    __asm__("nop");
    count--;
  }
}

void reset()
{
  __asm__("SEI");
  __asm__("LDA #$00");
  __asm__("STA $2010");
  __asm__("STA $2014");
  __asm__("STA $201B");
  __asm__("STA $202A");
  __asm__("STA $200D");
  __asm__("LDA #$01");
  __asm__("STA $2026");
  __asm__("LDA #$00");
  __asm__("STA $2026");
  __asm__("JMP ($FFFA)");
}

void print_game_item(unsigned char page, unsigned char line, unsigned char selected) 
{
    print(
        (char*)&GAMEINFO[4 + GAMEINFO_SIZE * (page * GAMES_PER_PAGE + line)], 
        CHAR_ON_LINE,
        1,
        line,
        selected
    );
}

void draw_games_list(unsigned char page, unsigned char cursor) 
{
    unsigned char start_game = page * GAMES_PER_PAGE;
    unsigned char end_game = start_game + GAMES_PER_PAGE;
    unsigned char i = start_game;
    clear_screen();
    if (end_game > GAMECOUNT) 
    {
        end_game = GAMECOUNT;
    }

    for (i; i < end_game; i++) 
    {
        unsigned char line = i - start_game + 1;  // Номер строки (1..19)
        print_game_item(page, line, line == cursor);
    }
}

void main(void) 
{
    unsigned char cursor = 1;  // Текущая позиция курсора (1..19)
    unsigned char page = 0;    // Текущая страница (0..N)
    unsigned char total_pages = (GAMECOUNT + GAMES_PER_PAGE - 1) / GAMES_PER_PAGE;
    
    init();
    clear_screen();
    draw_games_list(page, cursor);  // Первоначальная отрисовка списка

    while (1) 
    {
        if ((~SV_CONTROL & JOY_DOWN_MASK) && (cursor < GAMES_PER_PAGE)) 
        {           
            // Сдвиг курсора вниз
            print_game_item(page, cursor, 0);  // Убираем выделение
            cursor++;
            print_game_item(page, cursor, 1);  // Выделяем новую позицию
            beep();
            while(~SV_CONTROL & JOY_DOWN_MASK){};
        }
        else if ((~SV_CONTROL & JOY_UP_MASK) && (cursor > 1)) 
        {
            // Сдвиг курсора вверх
            print_game_item(page, cursor, 0);  // Убираем выделение
            cursor--;
            print_game_item(page, cursor, 1);  // Выделяем новую позицию
            beep();
            while(~SV_CONTROL & JOY_UP_MASK){};
        }
        else if ((~SV_CONTROL & JOY_LEFT_MASK) && (page > 0)) 
        {
            // Переход на предыдущую страницу
            page--;
            cursor = 1;  // Сбрасываем курсор на 1-ю позицию
            draw_games_list(page, cursor);  // Полная перерисовка
            beep();
            while(~SV_CONTROL & JOY_LEFT_MASK){};
        }
        else if ((~SV_CONTROL & JOY_RIGHT_MASK) && (page < total_pages - 1)) 
        {  
            // Переход на следующую страницу
            page++;
            cursor = 1;  // Сбрасываем курсор на 1-ю позицию
            draw_games_list(page, cursor);  // Полная перерисовка
            beep();
            while(~SV_CONTROL & JOY_RIGHT_MASK){};
        }
        else if (~SV_CONTROL & JOY_START_MASK) 
        {
            // Запуск выбранной игры
            unsigned char game_index = page * GAMES_PER_PAGE + cursor;
            SV_LCD.ypos = *(unsigned char*)(0x9000 + game_index);
            while(~SV_CONTROL & JOY_START_MASK){};
            reset();
        }
    }
}
