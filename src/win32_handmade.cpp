/******************************************************
 * Windows Platform code for Handmade Hero
 *
 * Author: Tyler Funk
 ******************************************************/

/*
   TODO(tyler): This is not a final platform layer
 
   - Saved game locations
   - Getting a handle to our own executable file
   - Asset loading path
   - Threading (launch a thread)
   - Raw Input (support for multiple keyboards)
   - Sleep/timeBeginPeriod
   - ClipCursor() (multimonitor support)
   - Fullscreen support
   - WM_SETCURSOR (cursor visibility control)
   - QueryCancelAutoplay
   - WM_ACTIVEATEAPP (for when we are not the active application)
   - Blit speed improvements (BitBlt)
   - Hardware acceleration (OpenGL or Direct3D or BOTH??)
   - GetKeyboardLayout (for French keyboards, international WASD support)

   Just a partial list of stuff!!
 */


#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

#include "handmade.cpp"

#define internal_func static 
#define local_persist static 
#define global_var static 
#define PI32 3.14159265359f

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

struct win32_sound_output
{
    int samplesPerSecond;
    int toneHz;
    int16 toneVolume;
    uint32 runningSampleIndex;
    int wavePeriod;
    int bytesPerSample;
    int secondaryBufferSize;
};


// Artificial XInputGetState support
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_var x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// Artificial XInputSetState support
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_var x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// Artificial DirectSound support
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);
global_var LPDIRECTSOUNDBUFFER gSecondaryBuffer;

void*
PlatformLoadFile(char* fileName)
{
    return(0);
}

internal_func void
Win32LoadXInput(void)
{
    // TODO: Test this on Windows 8
    // Try to load xinput 1.4 first. If it fails,
    // try to load xinput 1.3
    HMODULE xLib = LoadLibraryA("xinput1_4.dll");
    if (!xLib)
    {
        HMODULE xLib = LoadLibraryA("xinput1_3.dll");
    }

    if (xLib)
    {
        XInputGetState = (x_input_get_state*) GetProcAddress(xLib, "XInputGetState");
        XInputSetState = (x_input_set_state*) GetProcAddress(xLib, "XInputSetState");
    }
}


internal_func void
Win32InitDSound(HWND window, int32 samplesPerSecond, int32 bufferSize)
{
    // Load the library
    HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");

    // Result object for printing error messages
    HRESULT result;

    // Get a DirectSound object
    if (dSoundLibrary)
    {
        // "Create" a primary buffer
        direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(dSoundLibrary, "DirectSoundCreate");

        // TODO(tyler): Double-check that this works on XP - DirectSound8 or 7?
        LPDIRECTSOUND ds;
        result = DirectSoundCreate(0, &ds, 0);
        if (DirectSoundCreate && SUCCEEDED(result))
        {
            WAVEFORMATEX waveFormat = {};
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.nSamplesPerSec = samplesPerSecond;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nBlockAlign =
                (waveFormat.nChannels*waveFormat.wBitsPerSample) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec*waveFormat.nBlockAlign;
            waveFormat.cbSize = 0;

            result = ds->SetCooperativeLevel(window, DSSCL_PRIORITY);
            if (SUCCEEDED(result))
            {
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(bufferDescription);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER primaryBuffer;
                result = ds->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0);
                if (SUCCEEDED(result))
                {
                    result = primaryBuffer->SetFormat(&waveFormat);
                    if(SUCCEEDED(result))
                    {
                        OutputDebugStringA("We have set the format of the primary buffer\n");
                        // We have finally set the format
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
            }
            else
            {
                // TODO(tyler): Logging
            }

            // "Create" a secondary buffer, which is what we'll actually write to
            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = 0;
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            result = ds->CreateSoundBuffer(&bufferDescription, &gSecondaryBuffer, 0);
            if(SUCCEEDED(result))
            {
                // Start it playing
                OutputDebugStringA("Primary buffer format was set.\n");
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
    }
    else
    {
        // TODO(tyler): Logging
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
RenderWeirdGradient(win32_offscreen_buffer* buffer, int xOffset, int yOffset)
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
                    win32_offscreen_buffer* buffer,
                    int X, int Y, int width, int height)
{
    // TODO(tyler): Aspect ratio correction
    StretchDIBits(DeviceContext,
                  //X, Y, Width, Height,
                  //X, Y, Width, Height,
                  0, 0, width, height,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory,
                  &buffer->info,
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
            Win32CopyBufferToWindow(DeviceContext, &gBackBuffer, X, Y, Width, Height);
            EndPaint(Window, &Paint);
        } break;

        // Handle keyboard input
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 vkCode = WParam;
            bool wasDown = ((LParam & (1 << 30)) != 0);
            bool isDown = ((LParam & (1 << 31)) == 0);
            if (wasDown == isDown) break;
            if (vkCode == 'W')
            {
                OutputDebugStringA("W\n");
            }
            else if (vkCode == 'A')
            {
                OutputDebugStringA("A\n");
            }
            else if (vkCode == 'S')
            {
                OutputDebugStringA("S\n");
            }
            else if (vkCode == 'D')
            {
                OutputDebugStringA("D\n");
            }
            else if (vkCode == VK_UP)
            {
                OutputDebugStringA("UP\n");
            }
            else if (vkCode == VK_DOWN)
            {
                OutputDebugStringA("DOWN\n");
            }
            else if (vkCode == VK_LEFT)
            {
                OutputDebugStringA("LEFT\n");
            }
            else if (vkCode == VK_RIGHT)
            {
                OutputDebugStringA("RIGHT\n");
            }
            else if (vkCode == VK_ESCAPE)
            {
                OutputDebugStringA("Esc\n");
                if (isDown)
                {
                    OutputDebugStringA("is down\n");
                }
                if (wasDown)
                {
                    OutputDebugStringA("was down\n");
                }
            }
            else if (vkCode == VK_SPACE)
            {
                OutputDebugStringA("Space\n");
            }

            bool altKeyDown = (LParam & (1 << 29)) != 0;
            if (vkCode == VK_F4 && altKeyDown)
            {
                Running = false;
            }
        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
            break;
        }
    }
    return(Result);
}

void
Win32FillSoundBuffer(win32_sound_output* soundOutput,
                     DWORD byteToLock, DWORD bytesToWrite)
{
    VOID* region1;
    DWORD region1Size;
    VOID* region2;
    DWORD region2Size;
    if (SUCCEEDED(gSecondaryBuffer->Lock(byteToLock,
                                         bytesToWrite,
                                         &region1, &region1Size,
                                         &region2, &region2Size,
                                         0)))
    {
        // TODO(tyler): Assert that region1Size/region2Size is valid
        DWORD region1SampleCount = region1Size/soundOutput->bytesPerSample;
        int16* sampleOut = (int16*) region1;
        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex)
        {
            real32 t = 2.0f * PI32 *
                       (real32)soundOutput->runningSampleIndex /
                       (real32)soundOutput->wavePeriod;
            real32 sineValue = sinf(t);
            int16 sampleValue = (int16)(sineValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            ++soundOutput->runningSampleIndex;
        }
        DWORD region2SampleCount = region2Size/soundOutput->bytesPerSample;
        sampleOut = (int16*) region2;
        for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex)
        {
            real32 t = 2.0f * PI32 *
                       (real32)soundOutput->runningSampleIndex /
                       (real32)soundOutput->wavePeriod;
            real32 sineValue = sinf(t);
            int16 sampleValue = (int16)(sineValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            ++soundOutput->runningSampleIndex;
        }

        gSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
    
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    // Cache performance counter frequency
    LARGE_INTEGER perfCountFrequencyResult;
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    int64 perfCountFrequency = perfCountFrequencyResult.QuadPart;

    WNDCLASSA WindowClass = {};
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
    if (RegisterClassA(&WindowClass))
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
            // Graphics test
            int xOffset = 0;
            int yOffset = 0;

            // Direct Sound Test
            win32_sound_output soundOutput = {};
            soundOutput.samplesPerSecond = 48000;
            //soundOutput.toneHz = 512;
            soundOutput.toneHz = 256;
            //soundOutput.toneHz = 128;
            soundOutput.toneVolume = 3000;
            soundOutput.runningSampleIndex = 0;
            soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
            soundOutput.bytesPerSample = sizeof(int16)*2;
            soundOutput.secondaryBufferSize =
                soundOutput.samplesPerSecond*soundOutput.bytesPerSample;
            Win32InitDSound(window, soundOutput.samplesPerSecond,
                    soundOutput.secondaryBufferSize);
            Win32FillSoundBuffer(&soundOutput, 0, soundOutput.secondaryBufferSize);
            gSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            Running = true;

            LARGE_INTEGER lastCounter;
            QueryPerformanceCounter(&lastCounter);

            int64 lastCycleCount = __rdtsc();

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
                            XInputSetState(0, &vibration);
                        }
                    }
                    else
                    {
                        // NOTE(tyler): This controller is not available
                    }
                }

                // While not handling messages, render things
                RenderWeirdGradient(&gBackBuffer, xOffset, yOffset);

                // DirectSound output test
                DWORD playCursor;
                DWORD writeCursor;
                if (SUCCEEDED(gSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                {
                    DWORD byteToLock = (soundOutput.runningSampleIndex*soundOutput.bytesPerSample)%soundOutput.secondaryBufferSize;
                    DWORD bytesToWrite;

                    // TODO(tyler): Change this to using a lower latency offset
                    // from the playCursor
                    if (byteToLock == playCursor)
                    {
                        bytesToWrite = 0;
                    }
                    else if (byteToLock > playCursor)
                    {
                        bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock) + playCursor;
                    }
                    else
                    {
                        bytesToWrite = playCursor - byteToLock;
                    }

                    Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
                }

                HDC deviceContext = GetDC(window);
                win32_window_dimension dimensions = Win32GetWindowDimension(window);

                Win32CopyBufferToWindow(deviceContext, &gBackBuffer, 0, 0,
                        dimensions.width, dimensions.height);

                ++xOffset;

                int64 endCycleCount = __rdtsc();

                LARGE_INTEGER endCounter;
                QueryPerformanceCounter(&endCounter);

                GameLoop();

                int64 cyclesElapsed = endCycleCount - lastCycleCount;
                int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
                int32 msPerFrame = (int32)(1000 * counterElapsed / perfCountFrequency);
                int32 fps = 1000 / msPerFrame;
                int32 mcpf = (int32)(cyclesElapsed / (1000*1000));

                // Display ms/frame value
                // TODO(tyler): Debug only
                char strbuf[256];
                wsprintf(strbuf, "%dms/f, %dfps, %dc/f\n", msPerFrame, fps, mcpf);
                OutputDebugStringA(strbuf);

                lastCounter = endCounter;
                lastCycleCount = endCycleCount;
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
