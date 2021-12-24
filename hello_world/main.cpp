#include <sstream>
#include <iostream>
#include <orbis/libkernel.h>
#include "../../_common/log.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

// Logging
std::stringstream debugLogStream;

int main(void)
{
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

    int sleepSeconds = 2;
    
    // No buffering
    setvbuf(stdout, NULL, _IONBF, 0);
    
    DEBUGLOG << "Hello world! Waiting " << sleepSeconds << " seconds!";
    sceKernelUsleep(2 * 1000000);
    DEBUGLOG << "Done. Infinitely looping...";

    for (;;) {}
}
