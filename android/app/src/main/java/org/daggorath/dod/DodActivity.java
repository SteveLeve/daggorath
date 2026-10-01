package org.daggorath.dod;

import org.libsdl.app.SDLActivity;

// SDLActivity loads libSDL3.so and libmain.so and calls SDL_main, which is
// sdl_app.cpp's main(). Nothing else lives on the Java side.
public class DodActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL3", "main" };
    }
}
