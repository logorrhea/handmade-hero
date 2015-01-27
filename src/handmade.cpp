#include <windows.h>
#include <stdint.h>
#include <xinput.h>

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
struct win32_offscreen_buffer
{
    BITMAPINFO info;
    void *memory;
    int height;
    int width;
    int bytesPerPixel;
    int pitch;
};

struct win32_window_dimension
{
    int width;
    int height;
};

// Artificial XInputGetState support
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(0);
}
global_var x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// Artificial XInputSetState support
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(0);
}
global_var x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal_func void
Win32LoadXInput(void)
{
    HMODULE xLib = LoadLibrary("xinput1_3.dll");
    if (xLib)
    {
        XInputGetState = (x_input_get_state*) GetProcAddress(xLib, "XInputGetState");
        XInputSetState = (x_input_set_state*) GetProcAddress(xLib, "XInputSetState");
    }
}


win32_window_dimension
Win32GetWindowDimension(HWND window)
{
    win32_window_dimension result;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    return(result); 
}

global_var win32_offscreen_buffer gBackBuffer;


internal_func void
RenderWeirdGradient(win32_offscreen_buffer buffer, int xOffset, int yOffset)
{
    // TODO(tyler): Let's see what the optimizer does

    uint8* row = (uint8*)buffer.memory;
    for (int y = 0; y < buffer.height; y++)
    {
        uint32* pixel = (uint32*)row;
        for (int x = 0; x < buffer.width; x++)
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
        row += buffer.pitch;
    }
}

/**
 * Resize DIB Section, or initialize it
 * if it hasn't been before
 *
 * DIB = Device-Independent Bitmap
 */
internal_func void
Win32ResizeDIBSection(win32_offscreen_buffer* buffer, int width, int height)
{
    // TODO(tyler): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails

    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    buffer->bytesPerPixel = 4;

    // NOTE(tyler): When the biHeight field is negative, this is the clue to
    // Windows to treat this bitmap as top-down, not bottom-up, leaning that
    // the first three bytes of the image are the color for the top-left
    // pixel in the bitmap, not the bottom-left
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = width*height*buffer->bytesPerPixel;
    buffer->memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = width * buffer->bytesPerPixel;

}

internal_func void
Win32CopyBufferToWindow(HDC DeviceContext,
                    win32_offscreen_buffer buffer,
                    int X, int Y, int width, int height)
{
    // TODO(tyler): Aspect ratio correction
    StretchDIBits(DeviceContext,
                  //X, Y, Width, Height,
                  //X, Y, Width, Height,
                  0, 0, width, height,
                  0, 0, buffer.width, buffer.height,
                  buffer.memory,
                  &buffer.info,
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
            win32_window_dimension dimensions = Win32GetWindowDimension(Window);
            Win32ResizeDIBSection(&gBackBuffer, dimensions.width, dimensions.height);
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

            win32_window_dimension dimensions = Win32GetWindowDimension(Window);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            Win32CopyBufferToWindow(DeviceContext, gBackBuffer, X, Y, Width, Height);
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
    Win32LoadXInput();

    // Define parts of Window class

    // Didn't really need CS_OWNDC since we are
    // only having one window
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
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
            int xOffset = 0;
            int yOffset = 0;

            Running = true;
            while(Running)
            {
                // Use while to process entire message queue in-between
                // draw calls
                MSG message;
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

                // TODO(tyler): Should we poll this more frequently?
                for(DWORD controllerIndex = 0;
                    controllerIndex < XUSER_MAX_COUNT;
                    controllerIndex++)
                {
                    XINPUT_STATE controllerState;
                    if(XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                    {
                        // NOTE(tyler): This controller is plugged in
                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;
                        
                        // Read state of controller buttons
                        bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
                        bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool leftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool rightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
                        bool bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
                        bool xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
                        bool yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);
                        
                        // Read state of left thumb stick
                        int16 leftStickX = pad->sThumbLX;
                        int16 leftStickY = pad->sThumbLY;
                        
                        if (aButton)
                        {
                            ++yOffset;
                        }

                        if (bButton)
                        {
                            XINPUT_VIBRATION vibration;
                            vibration.wLeftMotorSpeed = 60000;
                            vibration.wRightMotorSpeed = 60000;
                            XInputSetState(0, &Vibration);
                        }
                    }
                    else
                    {
                        // NOTE(tyler): This controller is not available
                    }
                }

                // While not handling messages, render things
                RenderWeirdGradient(gBackBuffer, xOffset, yOffset);
                HDC deviceContext = GetDC(window);
                win32_window_dimension dimensions = Win32GetWindowDimension(window);

                Win32CopyBufferToWindow(deviceContext, gBackBuffer, 0, 0,
                        dimensions.width, dimensions.height);

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
