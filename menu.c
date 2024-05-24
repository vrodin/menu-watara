#include <supervision.h>
#include <string.h>
#include <stdio.h>
#include "font.h"

#define CART_BUS (*(unsigned char *)0x8000)
#define WORDS_PER_LINE (160 / 8 + 4)

#define CHAR_ON_LINE 18
#define ROM_INFO_SIZE 22

unsigned char mem[0x500];

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

    for (i = 0; i < 8; ++i)
    {
        byte = ch[i];
        if (invert)
        {
            byte = ~byte;
        }
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

    for (i = 0; i < len; i++)
    {
        char character = *(str + i * sizeof(char));
        display_char(x + i, y, font + (character - 32) * sizeof(unsigned char) * 8, invert);
    }
}

void sleep(int count)
{
    while (count)
    {
        __asm__("nop");
        count--;
    }
}

void reset()
{
    sleep(1);
    __asm__("jsr zerobss");
    __asm__("jsr copydata");
    __asm__("lda #<(__RAM_START__ + __RAM_SIZE__ + __STACKSIZE__)");
    __asm__("ldx #>(__RAM_START__ + __RAM_SIZE__ + __STACKSIZE__)");
    __asm__("sta sp");
    __asm__("stx sp+1");
    __asm__("jsr initlib");
    __asm__("jsr _main");
}

void main(void)
{
    char i, cursor = 1;

    init();
    clear_screen();

    memset(mem, 'X', sizeof(mem));

    for (i = 1; i < 19; i++)
    {
        print(mem + (ROM_INFO_SIZE * i), CHAR_ON_LINE, 1, i, i == cursor);
    }

    while (1)
    {
        if (~SV_CONTROL & JOY_DOWN_MASK && cursor < 18)
        {
            
            print(mem + (ROM_INFO_SIZE * cursor), CHAR_ON_LINE, 1, cursor + 1, 1);
            if (cursor > 0)
            {
                print(mem + (ROM_INFO_SIZE * (cursor - 1)), CHAR_ON_LINE, 1, cursor, 0);
            }
            cursor++;
            beep();
            while(~SV_CONTROL & JOY_DOWN_MASK){};
        }
        if (~SV_CONTROL & JOY_UP_MASK && cursor > 0)
        {
            print(mem + (ROM_INFO_SIZE * (cursor-1)), CHAR_ON_LINE, 1, cursor, 1);
            if (cursor < 18)
            {
                print(mem + (ROM_INFO_SIZE * (cursor)), CHAR_ON_LINE, 1, cursor + 1, 0);
            }
            
            cursor--;
            beep();
            while(~SV_CONTROL & JOY_UP_MASK){};
        }

        if (~SV_CONTROL & JOY_START_MASK)
        {
            SV_DMA.control = 1;

            reset();
        }

        sleep(100);
    }
}
