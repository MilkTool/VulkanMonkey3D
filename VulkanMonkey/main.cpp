#include "include/Window.h"
#include "include/Timer.h"
#include <iostream>
#include <cstring>
#include <string>

using namespace vm;

int main(int argc, char* argv[])
{
	Window::create();

    while(true)
	{
        Timer timer;
		timer.minFrameTime(1.f / (float)GUI::fps);

		if (!Window::processEvents(timer.getDelta()))
			break;

		for (auto& renderer : Window::renderer) {
			renderer->update(timer.getDelta());
			renderer->present();
        }
    }

	Window::destroyAll();

    return 0;
}