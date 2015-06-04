#if !defined(HANDMADE_H_)

#include "handmade.h"

internal_func void
GameOutputSound(game_sound_output_buffer* soundBuffer, int toneHz)
{
    local_persist real32 tSine;
    int16 toneVolume = 3000;
    int wavePeriod = soundBuffer->samplesPerSecond/toneHz;

    int16* sampleOut = soundBuffer->samples;
    for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; ++sampleIndex)
    {
        real32 sineValue = sinf(tSine);
        int16 sampleValue = (int16)(sineValue * toneVolume);
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;

        tSine += 2.0f*PI32*1.0f/(real32)wavePeriod;
    }
}

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
GameUpdateAndRender(game_offscreen_buffer *buff, int blueOffset, int greenOffset,
                    game_sound_output_buffer *soundBuffer, int toneHz)
{
    // TODO(tyler): Allow sample offsets here for more robust platform options
    GameOutputSound(soundBuffer, toneHz);
    RenderWeirdGradient(buff, blueOffset, greenOffset);
}

#define HANDMADE_H_
#endif
