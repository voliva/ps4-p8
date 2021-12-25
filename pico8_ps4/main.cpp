#include <sstream>
#include <iostream>
#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "log.h"
#include "TTF.h"
#include "Color.h"

#define FRAME_WIDTH     1920
#define FRAME_HEIGHT    1080

// extern "C" {
//     #include "lua.h"
//     #include "lauxlib.h"
//     #include "lualib.h"
// }

// Logging
std::stringstream debugLogStream;

// SDL window and software renderer
SDL_Window* window;
SDL_Renderer* renderer;

FT_Face fontDebug;

Color bgColor = { 0x10, 0x10, 0x10 };
Color fgColor = { 255, 255, 255 };

int main(void)
{
    int rc;
    SDL_Surface* windowSurface;

    // No buffering
    setvbuf(stdout, NULL, _IONBF, 0);

    // Initialize SDL functions
    DEBUGLOG << "Initializing SDL";

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
    {
        DEBUGLOG << "Failed to initialize SDL: " << SDL_GetError();
        return -1;
    }

    // We have to force load the freetype module or we'll get unresolved NID crashes
    DEBUGLOG << "Initializing TTF";

    rc = sceSysmoduleLoadModule(0x009A);

    if (rc < 0)
    {
        DEBUGLOG << "Failed to load freetype module: " << std::string(strerror(errno));
        return -1;
    }

    // Finally initialize freetype
    rc = FT_Init_FreeType(&ftLib);

    if (rc < 0)
    {
        DEBUGLOG << "Failed to initialize freetype: " << std::string(strerror(errno));
        return -1;
    }

    // Create a font face for debug and score text
    const char* debugFontPath = "/app0/assets/fonts/VeraMono.ttf";

    DEBUGLOG << "Initializing debug font (" << debugFontPath << ")";

    if (!InitFont(&fontDebug, debugFontPath, 16))
    {
        DEBUGLOG << "Failed to initialize debug font '" << debugFontPath << "'";
        return -1;
    }

    // Create a window context
    DEBUGLOG << "Creating a window";

    window = SDL_CreateWindow("main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, FRAME_WIDTH, FRAME_HEIGHT, 0);

    if (!window)
    {
        DEBUGLOG << "Failed to create window: " << SDL_GetError();
        for (;;);
    }

    // Create a software rendering instance for the window
    windowSurface = SDL_GetWindowSurface(window);
    renderer = SDL_CreateSoftwareRenderer(windowSurface);

    if (!renderer)
    {
        DEBUGLOG << "Failed to create software renderer: " << SDL_GetError();
        SDL_Quit();
        return -1;
    }

    // Initialize input / joystick
    // if (SDL_NumJoysticks() < 1)
    // {
    //     DEBUGLOG << "No controllers available!";
    //     for (;;);
    // }

    // controller = SDL_JoystickOpen(0);

    // if (controller == NULL)
    // {
    //     DEBUGLOG << "Couldn't open controller handle: " << SDL_GetError();
    //     for (;;);
    // }

    // initGame(renderer);

    // Enter the render loop
    DEBUGLOG << "Entering draw loop...";

    for (int frame = 0; frame < 1000; frame++)
    {
        // Clear the canvas
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, 0xFF);
        SDL_RenderClear(renderer);

        // Run all rendering routines

        //render(renderer);
        SDL_Texture* helloTexture = CreateText(renderer, (char*)"Hello world", fontDebug, fgColor, bgColor);

        SDL_Rect fpsCounterDest;
        fpsCounterDest.x      = 20;
        fpsCounterDest.y      = 980;
        SDL_QueryTexture(helloTexture, NULL, NULL, &fpsCounterDest.w, &fpsCounterDest.h);
        SDL_RenderCopy(renderer, helloTexture, NULL, &fpsCounterDest);

        // Propagate the updated window to the screen
        SDL_UpdateWindowSurface(window);

        // Run all update routines
        //update(renderer, deltaFrameTicks, frameCounter++, startFrameTicks);
    }

    SDL_Quit();
    return 0;
}

// void *L = (void *)(luaL_newstate);
/*char buff[256];
int error;
lua_State* L = lua_open();   // opens Lua *
luaopen_base(L);             // opens the basic library *
luaopen_table(L);            // opens the table library *
luaopen_io(L);               // opens the I/O library *
luaopen_string(L);           // opens the string lib. *
luaopen_math(L);             // opens the math lib. *

while (fgets(buff, sizeof(buff), stdin) != NULL) {
    error = luaL_loadbuffer(L, buff, strlen(buff), "line") ||
        lua_pcall(L, 0, 0, 0);
    if (error) {
        fprintf(stderr, "%s", lua_tostring(L, -1));
        lua_pop(L, 1);  // pop error message from the stack *
    }
}

lua_close(L);*/
