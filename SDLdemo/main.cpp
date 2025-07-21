#include <SDL.h>
#include <sdl_ttf.h>
#include <iostream>
#include <string>
void eventHandler(SDL_Event& event,bool& running,int* x,int* y,int* clicks) {
	// Handle events here
    switch (event.type) {
    case SDL_QUIT: std::cerr << "switch quit\n";
        running = false;
        break;
    case SDL_KEYDOWN:std::cerr << "key down" << SDL_GetKeyName(event.key.keysym.sym) << "\n";
        break;
    case  SDL_MOUSEBUTTONDOWN:
        std::cerr << "Mouse button down event received at ("
            << event.button.x << ", "
            << event.button.y << ")\n";
        if (event.button.y > 600 || event.button.x > 800)std::cerr << "garbage click\n";
        if ((event.button.x > *x && event.button.x < *x + 200) && (event.button.y >= *y && event.button.y <= *y + 200)) {
            std::cerr << "Mouse clicked on the rectangle at (" << event.button.x << ", " << event.button.y << ")\n";
			(*clicks)++;
			std::cerr << "Click count: " << *clicks << "\n";
        }
        else {
            std::cerr << "Mouse clicked outside the rectangle at (" << event.button.x << ", " << event.button.y << ")\n";
        }

        break;
    default:
        // Handle other events if necessary
        break;
    }
	
}

int main(int argc, char* argv[]) {
    // Initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << "\n";
		return EXIT_FAILURE;
	}
	// Initialize SDL_ttf
	if (TTF_Init() == -1) {
		std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << "\n";
		SDL_Quit();
		return EXIT_FAILURE;
	}
	// Main loop flag
	bool running = true;
    // Create window
	int WindowWidth = 800;
	int WindowHeight = 600;
    SDL_Window* window = SDL_CreateWindow(
        "madness",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WindowWidth, WindowHeight ,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
	// Load a font
    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);
	if (!font) {
		std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << "\n";
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	int mouseX = 0, mouseY = 0; // Variables to store mouse position
    int x = 0, y = 0, clicks = 0; // Initial position of the rectangle
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			eventHandler(event, running,&x,&y,&clicks);
			
			}
		
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (x <= 0) x = 0;
        if (x >= 600) x = 600;  // 800 - rect width
        if (y <= 0) y = 0;
        if (y >= 400) y = 400;  // 600 - rect height

        if (keystate[SDL_SCANCODE_W]) {
            y -= 5;
        }
        if (keystate[SDL_SCANCODE_S]) {
            y += 5;
        }
        if (keystate[SDL_SCANCODE_A]) {
            x -= 5;
        }
        if (keystate[SDL_SCANCODE_D]) {
            x += 5;
        }
        
        

        // In your game loop:
        

			
            SDL_SetRenderDrawColor(renderer, 0, 0, 100, 255); // Blue
            
            SDL_Rect boxrect = { x, y, 200, 200 }; // Example rectangle
			SDL_RenderClear(renderer); // Clear the renderer with the current draw color

			// Draw the rectangle
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
			SDL_GetMouseState(&mouseX, &mouseY); // Get the current mouse position
			if (mouseX > x && mouseX < x + 200 && mouseY > y && mouseY < y + 200) {
				SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green if mouse is over the rectangle
			}
			else {
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red otherwise
			}
            SDL_RenderFillRect(renderer, &boxrect);
            
            std::string counterText = "Clicks: " + std::to_string(clicks);
            SDL_Color color = { 255, 255, 255, 255 };
            SDL_Surface* surface = TTF_RenderText_Blended(font, counterText.c_str(), color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textrect = { (WindowWidth /2)-(surface->w/2), 10, surface->w, surface->h};
            SDL_FreeSurface(surface);

            SDL_RenderCopy(renderer, texture, NULL, &textrect);
            SDL_DestroyTexture(texture);

            // Present the renderer
            SDL_RenderPresent(renderer);
   			SDL_Delay(16); // Delay to limit frame rate (approx. 60 FPS)
		}
	

    // Cleanup
	TTF_CloseFont(font);
	TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
