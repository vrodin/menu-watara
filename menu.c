#include <supervision.h>
#include <string.h>
#include <stdio.h>
#include "font.h"

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

static void beep()
{
  SV_LEFT.delay = 300;
  SV_LEFT.timer = 2;
  SV_LEFT.control = 0b00111111;
  SV_RIGHT.delay = 254;
  SV_RIGHT.timer = 2;
  SV_RIGHT.control = 0b00111111;
}

static void display_char(const unsigned char x, const unsigned char y, const unsigned int *ch, const char invert)
{
  unsigned char i;
  unsigned int word;

  for (i = 0; i < 8; ++i) {
    word = ch[i];
    if (invert) word = ~word;
    SV_VRAM[y][i][x] = word;
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
    char character = str[i];
    display_char(x + i, y, &font[(character - 32) * 8], invert);
  }
}

void wait_200ms() {
    __asm__("ldx #$C8");
    __asm__("ms200_outer:");
    
    // Внутренний цикл ~4000 циклов (1мс)
    __asm__("ldy #$00");     // 256 итераций
    __asm__("ms200_middle:");
    __asm__("lda #$0F");     // ~15 итераций по ~1 циклу = ~15 циклов
    __asm__("ms200_inner:");
    __asm__("dec a");
    __asm__("bne ms200_inner");
    __asm__("dey");
    __asm__("bne ms200_middle");
    
    __asm__("dex");
    __asm__("bne ms200_outer");
}

void wait() {
  wait_200ms();
}

void __fastcall__ reset()
{
    __asm__("sei");
    __asm__("cld");
    __asm__("ldx #$FF");
    __asm__("txs");
    __asm__("lda #$08");
    __asm__("sta $2026");
    __asm__("cli");
    __asm__("jmp ($FFFC)");
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
        unsigned char line = i - start_game + 1;
        print_game_item(page, line, line == cursor);
    }
}

void main(void) 
{
    unsigned char cursor = 1;
    unsigned char page = 0;
    unsigned char total_pages = (GAMECOUNT + GAMES_PER_PAGE - 1) / GAMES_PER_PAGE;
    unsigned char lastPageGameCount = GAMECOUNT % GAMES_PER_PAGE;
    
    init();
    clear_screen();
    draw_games_list(page, cursor);

    while (1) 
    {
        if ((~SV_CONTROL & JOY_DOWN_MASK) && (cursor < GAMES_PER_PAGE)) 
        {
            if(page == total_pages - 1 && cursor == lastPageGameCount) continue;
            print_game_item(page, cursor, 0);
            cursor++;
            print_game_item(page, cursor, 1);
            beep();
            while(~SV_CONTROL & JOY_DOWN_MASK){};
        }
        else if ((~SV_CONTROL & JOY_UP_MASK) && (cursor > 1)) 
        {
            print_game_item(page, cursor, 0);
            cursor--;
            print_game_item(page, cursor, 1);
            beep();
            while(~SV_CONTROL & JOY_UP_MASK){};
        }
        else if ((~SV_CONTROL & JOY_LEFT_MASK) && (page > 0)) 
        {
            page--;
            cursor = 1;
            draw_games_list(page, cursor);
            beep();
            while(~SV_CONTROL & JOY_LEFT_MASK){};
        }
        else if ((~SV_CONTROL & JOY_RIGHT_MASK) && (page < total_pages - 1)) 
        {  
            page++;
            cursor = 1;
            draw_games_list(page, cursor);
            beep();
            while(~SV_CONTROL & JOY_RIGHT_MASK){};
        }
        else if (~SV_CONTROL & JOY_START_MASK) 
        {
            unsigned char game_index = page * GAMES_PER_PAGE + cursor;
            SV_LCD.ypos = *(unsigned char*)(0x9000 + game_index);
            wait();
            reset();
        }
    }
}
