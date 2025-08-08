#include <SDL.h>
#include <sdl_ttf.h>
#include <iostream>
#include <string>
#include <enet/enet.h>
#include <vector>
//GAMESTATES
enum GameStates
{
	MOVEMENT,
	TEXTINPUT,
	serverTEXTINPUT
};
//PACKET TYPES
enum PacketType {
	PACKET_CHAT = 1, // Chat message packet
	PACKET_MOVE = 2, // Player movement packet
	PACKET_DISCONNECT = 3 // Disconnect packet
};
struct Packet {
	uint8_t type;// Type of the packet (e.g., text input, movement, etc.)
	char name[32]; // Player name
	int32_t x; // Player x position
	int32_t y; // Player y position
    char message[128];//messsage content

};

struct player {
	std::string name;
	std::string serverInput; // Server text input
	int x =100, y=100; // Position of the player
	bool isConnected = false; // Connection status of the player
	
};
int availableIndex(struct player players[]) {
	for (int i = 0; i < 4; i++) {
		if (!players[i].isConnected && i !=1) {
			return i; // Return the index of the first available player slot
		}
	}
}
void enetEventHandler(ENetEvent *enetEvent, ENetHost* Host,struct player players[],ENetPeer** peer,bool isHost,bool isClient) {
    if (isHost && !isClient) {
        while (enet_host_service(Host, enetEvent, 0) > 0) {
            switch (enetEvent->type) {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "A new client connected from " << enetEvent->peer->address.host << ":" << enetEvent->peer->address.port << "\n";
                *peer = enetEvent->peer; // Store the peer for later use
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                if (enetEvent->packet->dataLength == sizeof(Packet)) {
                    Packet pkt;
                    memcpy(&pkt, enetEvent->packet->data, sizeof(Packet));

                    if (pkt.type == PACKET_CHAT) {
                        std::cout << pkt.name << " says: " << pkt.message << "\n";
                    }
                    else if (pkt.type == PACKET_MOVE) {
                        std::cout << pkt.name << " moved to: " << pkt.x << "," << pkt.y << "\n";
                    }
                }
                else {
                    std::cerr << "Received packet of unexpected size!\n";
                }
                    enet_packet_destroy(enetEvent->packet);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "A client disconnected.\n";
                    break;
                default:
                    break;
            }
        }
    }
    if (isClient && !isHost) {
        while (enet_host_service(Host, enetEvent, 0) > 0) {
            switch (enetEvent->type) {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "Connected to server.\n";
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                if (enetEvent->packet->dataLength == sizeof(Packet)) {
                    Packet pkt;
                    memcpy(&pkt, enetEvent->packet->data, sizeof(Packet));
                    if (pkt.type == PACKET_CHAT) {
                        std::cout << pkt.name << " says: " << pkt.message << "\n";
                    }
                    else if (pkt.type == PACKET_MOVE) {
                        std::cout << pkt.name << " moved to: " << pkt.x << "," << pkt.y << "\n";
                    }
                }
                else {
                    std::cerr << "Received packet of unexpected size!\n";
                }
                enet_packet_destroy(enetEvent->packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnected from server.\n";
                break;
            default:
                break;
            }
        }
    }
}

void eventHandler(SDL_Event& event, bool& running, int* x, int* y, int* clicks, GameStates* CurrentState, std::string* textInput, std::string* serverTextInput) {



    while (SDL_PollEvent(&event)) {
        // Handle events here
        switch (event.type) {
        case SDL_QUIT: std::cerr << "switch quit\n";
            running = false;
            break;
        case SDL_KEYDOWN:std::cerr << "key down" << SDL_GetKeyName(event.key.keysym.sym) << "\n";
            break;
        case  SDL_MOUSEBUTTONDOWN:
            std::cerr << "Mouse button down event received at (" << event.button.x << ", " << event.button.y << ")\n";
            if (event.button.y > 600 || event.button.x > 800)std::cerr << "garbage click\n";
            if ((event.button.x > *x && event.button.x < *x + 200) && (event.button.y >= *y && event.button.y <= *y + 200)) {
                std::cerr << "Mouse clicked on the rectangle at (" << event.button.x << ", " << event.button.y << ")\n";(*clicks)++;
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
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_RETURN && !(*CurrentState == serverTEXTINPUT)) {

                if (*CurrentState == TEXTINPUT) {
                    std::cout << "Text input: " << *textInput << "\n"; // Print the text input to console
                    SDL_StopTextInput();// Stop text input when Escape is pressed
                    *CurrentState = MOVEMENT; // Switch to movement state
                    std::cout << *CurrentState << "\n"; // Print the current state to console    
                }
                else {
                    std::cout << "gamemode/\n";
                    SDL_StartTextInput(); // Start text input when Escape is pressed
                    *CurrentState = TEXTINPUT; // Switch to text input state
                    std::cout << *CurrentState << "\n"; // Print the current state to console

                }
            }
            //chat packets 


            else if (event.key.keysym.sym == SDLK_BACKSPACE && *CurrentState == serverTEXTINPUT) {
                // Handle backspace to remove the last character
                if (!(serverTextInput->empty()))serverTextInput->pop_back();

            }
            else if (*CurrentState == serverTEXTINPUT && (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_RETURN)) {
                // Stop server text input when Escape is pressed
                *CurrentState = MOVEMENT; // Switch back to movement state
                SDL_StopTextInput(); // Stop text input
                std::cout << "server text input stopped\n";
                std::cout << "server text input: " << *serverTextInput << "\n"; // Print the server text input to console

            }
            else if (event.key.keysym.sym == SDLK_t && !(*CurrentState == TEXTINPUT)) {
                if (*CurrentState == TEXTINPUT || *CurrentState == MOVEMENT) {
                    SDL_StartTextInput(); // Start text input when 't' is pressed
                    *CurrentState = serverTEXTINPUT; // Switch to server text input state
                    *serverTextInput = ""; // Clear previous server text input
                    std::cout << "server text input\n";
                }

            }
            else if (event.key.keysym.sym == SDLK_ESCAPE) {
                // Stop text input when Escape is pressed
                SDL_StopTextInput();
                *CurrentState = MOVEMENT; // Switch to movement state
                std::cout << "text input stopped\n";
            }
            else if (event.key.keysym.sym == SDLK_BACKSPACE) {
                // Handle backspace to remove the last character
                if (!(textInput->empty()))textInput->pop_back();

            }
        }
        else if (event.type == SDL_TEXTINPUT && *CurrentState == serverTEXTINPUT) {
            // Handle text input here if needed
            *serverTextInput += *event.text.text; // Append the new text input

        }
        else if (event.type == SDL_TEXTINPUT && *CurrentState == TEXTINPUT) {
            // Handle text input here if needed
            *textInput += *event.text.text; // Append the new text input
        }
    }
    if (*CurrentState == MOVEMENT) {
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (*x <= 0) *x = 0;
        if (*x >= 750) *x = 750;  // 800 - rect width
        if (*y <= 0) *y = 0;
        if (*y >= 550) *y = 550;  // 600 - rect height

        if (keystate[SDL_SCANCODE_W]) {
            *y -= 5;
        }
        if (keystate[SDL_SCANCODE_S]) {
            *y += 5;
        }
        if (keystate[SDL_SCANCODE_A]) {
            *x -= 5;
        }
        if (keystate[SDL_SCANCODE_D]) {
            *x += 5;
        }
    }
}

//main function

int main(int argc, char* argv[]) {
	
    //players
    struct player players[4];
    bool isHost = false;
    bool isClient = false;
	int localPlayerIndex = 0; // Index of the local player

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
    // Initialize ENet
    if (enet_initialize() != 0) {
        std::cerr << "An error occurred while initializing ENet.\n";
        return EXIT_FAILURE;
    }
    std::cout << "SDL and SDL_ttf and enet initialized successfully.\n";

    ENetHost* client = NULL; // ENet host pointer for client
    ENetHost* server = NULL; // ENet host pointer
    ENetHost* Host = NULL; // ENet host pointer for server
    ENetPeer* peer = NULL; // ENet peer pointer
    ENetAddress address;
    address.host = ENET_HOST_ANY; // Bind to any address
    address.port = 1234; // Port number for the server
    char user;
    std::cout << "Enter 'h' for host (server) or 'c' for client: ";
    std::cin >> user; // Get user input for server or client mode
    if (user == 'h') {
		isHost = true; // Set host mode
        server = enet_host_create(&address, 32, 2, 0, 0);
        Host = server;
        if (server == NULL) {
            std::cerr << "An error occurred while trying to create an ENet server host.\n";
            enet_deinitialize();
            SDL_Quit();
            return EXIT_FAILURE;
        }
        localPlayerIndex = 0;
        std::cout << "ENet server created successfully.\n";
		players[localPlayerIndex].isConnected = true; // Mark the first player as connected
    }


    if (user == 'c') {
		isClient = true; // Set client mode
        client = enet_host_create(NULL, 1, 2, 0, 0); // local client
        enet_address_set_host(&address, "127.0.0.1"); // or public IP
        address.port = 1234;
        Host = client;
        peer = enet_host_connect(client, &address, 2, 0);
        if (!peer) {
            std::cerr << "An error occurred while trying to connect to the ENet server.\n";
            enet_host_destroy(client);
            enet_deinitialize();
            SDL_Quit();
            return EXIT_FAILURE;
        }
		localPlayerIndex = availableIndex(players); // Get the index of the first available player slot
		players[localPlayerIndex].isConnected = true; // Mark the player as connected
		std::cout << "ENet client created successfully.\n";
    }
	// Wait for connection to be established
	

    // Main loop flag
    bool running = true;
    // Create window
    int WindowWidth = 800;
    int WindowHeight = 600;
    SDL_Window* window = SDL_CreateWindow(
        "madness",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WindowWidth, WindowHeight,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return EXIT_FAILURE;
    }


    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    // Load a font
    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    //for the blink cursor
    Uint32 lastBlinkTime = 0;

    int mouseX = 0, mouseY = 0; // Variables to store mouse position

    int x = WindowHeight / 2, y = WindowWidth / 2, clicks = 0; // Initial position of the rectangle

    // Input handling variables
    std::string textInput = "";
	std::string serverTextInput = "";
	std::string message = "";
    SDL_Event inputEvent;
    GameStates CurrentState = TEXTINPUT;
    SDL_StartTextInput(); // Start text input mode
    //enet event handling
    ENetEvent enetEvent;
    

	
    while (running) {
		// Handle events
       
		enetEventHandler(&enetEvent, Host,players,&peer,isHost,isClient); // Handle ENet events
        eventHandler(inputEvent, running, &players[localPlayerIndex].x, &players[localPlayerIndex].y, &clicks, &CurrentState, &textInput, &serverTextInput);

        Packet pkt;
        pkt.type = PACKET_CHAT;
        strncpy_s(pkt.name, players[localPlayerIndex].name.c_str(), sizeof(pkt.name));
        pkt.x = 100;
        pkt.y = 200;
        strncpy_s(pkt.message,players[localPlayerIndex].serverInput.c_str(), sizeof(pkt.message));

        // Create a packet from the raw memory of the struct
        ENetPacket* enetPacket = enet_packet_create(
            &pkt, sizeof(pkt), ENET_PACKET_FLAG_RELIABLE);
        if (peer != nullptr) {
            enet_peer_send(peer, 0, enetPacket); // Send the packet to the server
            
        }



        //rendering
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Set the draw color to black

        for (player& p : players) {
			if (!p.isConnected) continue; // Skip disconnected players
           
			SDL_Rect playerRect = {p.x, p.y, 50, 50 }; // Example player rectangle
			SDL_RenderClear(renderer); // Clear the renderer with the current draw color
			
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue for player
			SDL_RenderFillRect(renderer, &playerRect); // Draw player rectangle

            //player input
			

            p.name = textInput;
            p.serverInput = p.name;
            p.serverInput += ":" + serverTextInput; // Combine player name and server text input

            //player name rect
            if (!p.name.empty() && CurrentState == MOVEMENT) {
                SDL_Color white = { 255, 255, 255, 255 }; // White color for text
                SDL_Surface* surface = TTF_RenderText_Solid(font, p.serverInput.c_str(), white);
                if (surface) {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                    SDL_Rect nameRect = { p.x + 25 - surface->w / 2, p.y - 30, surface->w, surface->h }; // Center over player rect
                    SDL_RenderCopy(renderer, texture, nullptr, &nameRect);
                    SDL_FreeSurface(surface);
                    SDL_DestroyTexture(texture);
                }
            }

        }



       

        //textInput render
        //blink text input
        Uint32 currentTime = SDL_GetTicks();

        std::string displayText = textInput;
        const Uint32 blinkInterval = 500; // Blink every 500 ms
        static bool blinkState = false;
        if (currentTime - lastBlinkTime > 500) { // Blink every 500 ms
            blinkState = !blinkState;
            lastBlinkTime = currentTime;
        }
        if (CurrentState == TEXTINPUT && blinkState) {
            displayText += "_"; // Add a cursor if in text input mode and blinking
        }
		//servertextinput render
		std::string serverText = serverTextInput;
		std::string servertext = serverText;
		if (CurrentState == serverTEXTINPUT && blinkState) {
			servertext += "_"; // Add a cursor if in server text input mode and blinking
		}
        if (CurrentState == serverTEXTINPUT) {
            SDL_Color textColor = { 255, 255, 255, 255 }; // White color for text
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, servertext.c_str(), textColor);
            if (textSurface) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                SDL_Rect textRect = { (WindowWidth / 2) - (textSurface->w / 2), (WindowHeight / 2) - (textSurface->h / 2), textSurface->w, textSurface->h };
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
                SDL_FreeSurface(textSurface);
            }
        }
		
        // Render text input if in TEXTINPUT state
        if (CurrentState == TEXTINPUT) {
            SDL_Color textColor = { 255, 255, 255, 255 }; // White color for text
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, displayText.c_str(), textColor);
            if (textSurface) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                SDL_Rect textRect = { (WindowWidth / 2) - (textSurface->w / 2), (WindowHeight / 2) - (textSurface->h / 2), textSurface->w, textSurface->h };
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
                SDL_FreeSurface(textSurface);
            }
        }
        //click counter
        std::string counterText = "Clicks: " + std::to_string(clicks);
        SDL_Color color = { 255, 255, 255, 255 };
        SDL_Surface* surface = TTF_RenderText_Blended(font, counterText.c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect textrect = { (WindowWidth / 2) - (surface->w / 2), 10, surface->w, surface->h };
        SDL_FreeSurface(surface);

        SDL_RenderCopy(renderer, texture, NULL, &textrect);
        SDL_DestroyTexture(texture);

        // Present the renderer
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // Delay to limit frame rate (approx. 60 FPS)
    }
    std::cout << peer << "\n";

        // Cleanup
        if (peer) enet_peer_disconnect(peer, 0);
        if (client) enet_host_destroy(client);
        if (server) enet_host_destroy(server);
        enet_deinitialize();

        SDL_StopTextInput();
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
		std::cout << "SDL and ENet cleaned up successfully.\n";

        return 0;
    }