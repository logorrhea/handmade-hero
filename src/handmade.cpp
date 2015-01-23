#include <windows.h>
#include <stdint.h>

#define internal_func static 
#define local_persist static 
#define global_var static 

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// TODO(tyler): This is a global for now.
global_var bool Running;

// TODO(tyler): global for now
// Bitmap information
global_var BITMAPINFO BitmapInfo;
global_var void *BitmapMemory;
global_var int BitmapHeight;
global_var int BitmapWidth;
global_var int BytesPerPixel = 4;


internal_func void
RenderWeirdGradient(int xOffset, int yOffset)
{
    int width = BitmapWidth;
    int height = BitmapHeight;

    uint8* row = (uint8*)BitmapMemory;
    int pitch = width * BytesPerPixel;
    for (int y = 0; y < BitmapHeight; y++)
    {
        uint32* pixel = (uint32*)row;
        for (int x = 0; x < BitmapWidth; x++)
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
        row += pitch;
    }
}

/**
 * Resize DIB Section, or initialize it
 * if it hasn't been before
 *
 * DIB = Device-Independent Bitmap
 */
internal_func void
Win32ResizeDIBSection(int Width, int Height)
{
    // TODO(tyler): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails

    if (BitmapMemory)
    {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = BitmapWidth*BitmapHeight*BytesPerPixel;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    RenderWeirdGradient(0, 0);

}

internal_func void
Win32UpdateWindow(HDC DeviceContext, RECT* WindowRect, int X, int Y, int Width, int Height)
{
    int WindowWidth = WindowRect->right - WindowRect->left;
    int WindowHeight = WindowRect->bottom - WindowRect->top;
    StretchDIBits(DeviceContext,
                  //X, Y, Width, Height,
                  //X, Y, Width, Height,
                  0, 0, BitmapWidth, BitmapHeight,
                  0, 0, WindowWidth, WindowHeight,
                  BitmapMemory,
                  &BitmapInfo,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVEAPP\n");
        } break;

        case WM_DESTROY:
        {
            // TODO(tyler): Handle this as an error = recreate window?
            Running = false;
        } break;

        case WM_CLOSE:
        {
            // TODO(tyler): Handle this with a message to the user?
            Running = false;
        } break;

        case WM_PAINT:
        {
            // Paint the window white
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
            break;
        }
    }
    return(Result);
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    WNDCLASS WindowClass = {};

    // Define parts of Window class
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    // Register window class
    if (RegisterClass(&WindowClass))
    {
        HWND window =
            CreateWindowEx(
              0,
              WindowClass.lpszClassName,
              "Handmade Hero",
              WS_OVERLAPPEDWINDOW|WS_VISIBLE,
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              0,
              0,
              Instance,
              0);
        if (window)
        {
            Running = true;
            MSG message;
            int xOffset = 0;
            int yOffset = 0;
            while(Running)
            {
                // Use while to process entire message queue in-between
                // draw calls
                while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    // Make sure WM_QUI message gets handled properly
                    if (message.message == WM_QUIT)
                    {
                        Running = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                // While not handling messages, render things
                RenderWeirdGradient(xOffset, yOffset);
                HDC deviceContext = GetDC(window);
                RECT clientRect;
                GetClientRect(window, &clientRect);

                int windowWidth = clientRect.right - clientRect.left;
                int windowHeight = clientRect.bottom - clientRect.top;
                Win32UpdateWindow(deviceContext, &clientRect, 0, 0, windowWidth, windowHeight);

                ++xOffset;
            }
        }
        else
        {
            // TODO(tyler): Logging
        }
    }
    else
    {
        // TODO(tyler): Logging
    }

    return(0);
}
