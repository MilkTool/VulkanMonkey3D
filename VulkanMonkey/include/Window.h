#pragma once
#include "Renderer.h"

class Window
{
    public:
        static void create(const char* title, int w, int h, Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
		static void destroyAll();
		static bool processEvents(float delta);

		static std::vector<std::unique_ptr<Renderer>> renderer; // window pointer is also stored in here
	private:
        Window();
        ~Window();
		static int width, height;
};