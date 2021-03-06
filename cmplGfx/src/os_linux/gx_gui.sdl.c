#include "../gx_gui.h"
#include <SDL2/SDL.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

#define SDL_Log(...) do{}while(0)

struct GxWindow {
	SDL_Window *window;
	SDL_Surface *image;
	SDL_Surface *screen;
};

GxWindow createWindow(GxImage offs, const char *title) {
	if (offs == NULL) {
		return NULL;
	}

	GxWindow result = malloc(sizeof(struct GxWindow));
	if (result == NULL) {
		return NULL;
	}

	SDL_Init(SDL_INIT_VIDEO);
	result->image = SDL_CreateRGBSurfaceFrom(offs->basePtr, offs->width, offs->height, offs->depth, offs->scanLen, rch(255), gch(255), bch(255), 0);
	result->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, offs->width, offs->height, SDL_WINDOW_HIDDEN);
	result->screen = SDL_GetWindowSurface(result->window);
	SDL_ShowWindow(result->window);
	return result;
}

int getWindowEvent(GxWindow window, int *button, int *x, int *y) {
//	static int btnstate = 0;
	SDL_Event event;

	event.type = 0;
	while (SDL_PollEvent(&event)) {
		if (event.type != SDL_MOUSEMOTION &&
			event.type != SDL_FINGERMOTION) {
			// consume motion events
			break;
		}
	}
	*button = 0;
	*x = *y = 0;
	switch (event.type) {
		default:
			*button = event.type;
			break;

		case SDL_QUIT:
			return WINDOW_CLOSE;

		case SDL_WINDOWEVENT:
			switch (event.window.event) {

				case SDL_WINDOWEVENT_CLOSE:
					//SDL_Log("Window %d closed", event.window.windowID);
					return WINDOW_CLOSE;

				case SDL_WINDOWEVENT_ENTER:
					//SDL_Log("Mouse entered window %d", event.window.windowID);
					return WINDOW_ENTER;

				case SDL_WINDOWEVENT_LEAVE:
					//SDL_Log("Mouse left window %d", event.window.windowID);
					return WINDOW_LEAVE;

				case SDL_WINDOWEVENT_SHOWN:
					SDL_Log("Window %d shown", event.window.windowID);
					break;
				case SDL_WINDOWEVENT_HIDDEN:
					SDL_Log("Window %d hidden", event.window.windowID);
					break;
				case SDL_WINDOWEVENT_EXPOSED:
					SDL_Log("Window %d exposed", event.window.windowID);
					break;
				case SDL_WINDOWEVENT_MOVED:
					SDL_Log("Window %d moved to %d,%d", event.window.windowID, event.window.data1, event.window.data2);
					break;
				case SDL_WINDOWEVENT_RESIZED:
					SDL_Log("Window %d resized to %dx%d", event.window.windowID, event.window.data1, event.window
						.data2
					);
					break;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					SDL_Log("Window %d size changed to %dx%d", event.window.windowID, event.window.data1, event.window
						.data2
					);
					break;
				case SDL_WINDOWEVENT_MINIMIZED:
					SDL_Log("Window %d minimized", event.window.windowID);
					break;
				case SDL_WINDOWEVENT_MAXIMIZED:
					SDL_Log("Window %d maximized", event.window.windowID);
					break;
				case SDL_WINDOWEVENT_RESTORED:
					SDL_Log("Window %d restored", event.window.windowID);
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					SDL_Log("Window %d gained keyboard focus", event.window.windowID);
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					SDL_Log("Window %d lost keyboard focus", event.window.windowID);
					break;
				default:
					SDL_Log("Window %d got unknown event %d", event.window.windowID, event.window.event);
					break;
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			*button = event.button.button;
			*x = event.button.x;
			*y = event.button.y;
			return MOUSE_PRESS;

		case SDL_MOUSEBUTTONUP:
			*button = event.button.button;
			*x = event.button.x;
			*y = event.button.y;
			return MOUSE_RELEASE;

		case SDL_FINGERDOWN:
			*button = 1;
			*x = event.tfinger.x * window->image->w;
			*y = event.tfinger.y * window->image->h;
			return FINGER_PRESS;

		case SDL_FINGERUP:
			*button = 1;
			*x = event.tfinger.x * window->image->w;
			*y = event.tfinger.y * window->image->h;
			return FINGER_RELEASE;

		case SDL_FINGERMOTION:
			*button = 1;
			*x = event.tfinger.x * window->image->w;
			*y = event.tfinger.y * window->image->h;
			return FINGER_MOTION;

		case SDL_MOUSEMOTION:
			*button = event.button.button;
			*x = event.motion.x;
			*y = event.motion.y;
			return MOUSE_MOTION;

		case SDL_MOUSEWHEEL:
			*button = event.wheel.direction;
			*x = event.wheel.x;
			*y = event.wheel.y;
			return MOUSE_WHEEL;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
			*button = event.key.keysym.sym;
			*x = event.key.keysym.scancode;
			*y = 0;
			if (event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
				*y |= KEY_MASK_SHIFT;
			}
			if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
				*y |= KEY_MASK_CTRL;
			}
			if (event.key.keysym.mod & (KMOD_LALT | KMOD_RALT)) {
				*y |= KEY_MASK_ALT;
			}
			switch (event.type) {
				case SDL_KEYDOWN:
					return KEY_PRESS;

				case SDL_KEYUP:
					return KEY_RELEASE;
			}
			break;
	}
	(void)window;
	return 0;
}

void setWindowTitle(GxWindow window, const char *caption) {
	SDL_SetWindowTitle(window->window, caption);
}

void flushWindow(GxWindow window) {
	SDL_BlitSurface(window->image, NULL, window->screen, NULL);
	SDL_UpdateWindowSurface(window->window);
}

void destroyWindow(GxWindow window) {
	SDL_FreeSurface(window->image);
	SDL_FreeSurface(window->screen);
	SDL_DestroyWindow(window->window);
	SDL_Quit();
	free(window);
}
