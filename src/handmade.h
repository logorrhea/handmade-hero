#if !defined(HANDMADE_H)
#define internal_func static 

/*
 * TODO(tyler): Services that the playform layer provides to the game
 */

/*
 * NOTE(tyler): Services that the game provides to the platform layer.
 * (this may expand in the future - sound on separate thread, etc.)
 * (but not until we know that we are going to need it
 *
 * @param timing
 * @param controller/keyboard input
 * @param bitmap buffer to use
 * @param sound buffer to use
 */
struct game_offscreen_buffer
{
    void *memory;
    int width;
    int height;
    int pitch;
};
internal_func void GameUpdateAndRender(game_offscreen_buffer *buff, int blueOffset, int greenOffset);

#define HANDMADE_H
#endif
