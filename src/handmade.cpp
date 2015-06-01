#if !defined(HANDMADE_H_)

#include "handmade.h"

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int bool32;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

internal_func void
RenderWeirdGradient(game_offscreen_buffer* buffer, int xOffset, int yOffset)
{
    // TODO(tyler): Let's see what the optimizer does

    uint8* row = (uint8*)buffer->memory;
    for (int y = 0; y < buffer->height; y++)
    {
        uint32* pixel = (uint32*)row;
        for (int x = 0; x < buffer->width; x++)
        {
            /*            pixel +0 +1 +2 +3
             * pixel in memory: 00 00 00 00
             *                  BB GG RR xx
             *
             * Where xx is padding
             */
            uint8 blue = (x + xOffset);
            uint8 green = (y + yOffset);

            *pixel++ = (green << 8) | blue; 

        }
        row += buffer->pitch;
    }
}

void
GameMain(void)
{
}

void
GameShutDown(void)
{
}

internal_func void
GameUpdateAndRender(game_offscreen_buffer *buff)
{
    int blueOffset = 0;
    int greenOffset = 0;
    RenderWeirdGradient(buff, blueOffset, greenOffset);
}

#define HANDMADE_H_
#endif
