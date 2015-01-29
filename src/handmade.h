#if !defined(HANDMADE_H)

struct platform_window;
platform_window* PlatformOpenWindow(char* title);
void PlatformCloseWindow(platform_window* window);

#define HANDMADE_H
#endif
