#include <SDL.h>
#include <sdl_ttf.h>
#include <iostream>
#include <string>
#include <enet/enet.h>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
template <typename T> 
//utils
inline T myMin(T a, T b) {
    if (a > b)return b;
    else return a;
}
template <typename T>
inline T myMax(T a, T b) {
    if (a < b)return b;
    else return a;
}

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
    PACKET_SHOOT = 3,
    PACKET_STOPSHOOT = 4,
    PACKET_PLAYERHEALTH = 5,
	PACKET_DISCONNECT = 6 // Disconnect packet

};
struct Packet {
	uint8_t type;// Type of the packet (e.g., text input, movement, etc.)
    uint8_t hp;
	char name[32]; // Player name
	int32_t x; // Player x position
	int32_t y; // Player y position
    char message[128];//messsage content
    float dirx, diry;

};
//zombies
struct zombieState {
    float x, y;
    int hp;
    bool alive;
};
struct zombie {
    int z_id;
    int z_HP = 100;
    float x, y;
    float startX = 0.0f, startY = 0.0f;
    float targetX, targetY;
    float targetRadi = 250;
    std::string target = "";
    float closeSpeed =150.00f, farSpeed = 170.00f;
    bool isalive = false;
    float lerpTimer = 0.0f;
    float lerpDuration = 0.1f; // 100ms smoothing window
};

//player stats
struct player {
    ENetPeer* user = NULL;
    int p_HP = 100;
    std::string name;
    std::string serverInput; // Server text input
    float x = 200, y =200; // Position of the player
    bool isConnected = false; // Connection status of the player
    bool isShooting = false;
    float xdir =0, ydir =0;
};
std::random_device rd;
std::mt19937 gen(rd());
int getRandInt(int min, int max) {
    std::uniform_int_distribution<>dist(min, max);
    return dist(gen);

}
void getplayeraim(player* player, int mouseX, int mouseY,int cameraX ,int cameraY) {
  
    int worldMouseX = (float)mouseX + cameraX;
    int worldMouseY =(float)mouseY + cameraY;
    player->xdir =(float)worldMouseX-(player->x +25);
    player->ydir =(float)worldMouseY-(player->y +25);
    float len = sqrtf(player->xdir * player->xdir + player->ydir * player->ydir);
    if (len > 0) {
        player->xdir /= len;
        player->ydir /= len;
        int xdir = std::round(player->xdir);
        int ydir = std::round(player->ydir);
    }
    else {
        player->xdir = 0;
        player->ydir = 0;
    }
    
}
bool raycast(float playerX, float playerY, float playerDx, float playerDy, float zomX, float zomY,float &outT) {
    const float ESP = 1e-6f;
    float tmin = -1e30f;
    float tmax = 1e30f;

    if (fabsf(playerDx) < ESP) {
        if (playerX < zomX || playerX > zomX + 50) return false;
    }
    else {
        float tx1 = (zomX - playerX) / playerDx;
        float tx2 = (zomX + 50 - playerX) / playerDx;
        if (tx1 > tx2) { float tmp = tx1; tx1 = tx2; tx2 = tmp; }
        if (tx1 > tmin) tmin = tx1;
        if (tx2 < tmax) tmax = tx2;
    }
    if (fabsf(playerDy) < ESP) {
        if (playerY < zomY || playerY > zomY + 50) return false;
    }
    else {
        float ty1 = (zomY - playerY) / playerDy;
        float ty2 = (zomY + 50 - playerY) / playerDy;
        if (ty1 > ty2) { float tmp = ty1; ty1 = ty2; ty2 = tmp; }
        if (ty1 > tmin) tmin = ty1;
        if (ty2 < tmax) tmax = ty2;
    }
    if (tmax >= tmin && tmax >= 0.0f) {
        outT = (tmin >= 0.0f) ? tmin : tmax;
        return true;
    }
    return false;
}

  
    
    

void checkplayerShots(player players[], zombie zombies[]) {
    float outT = 0.00f;
    for (int i = 0; i < 4; i++) {
        if (!players[i].isShooting)continue;
        if (!players[i].isConnected)continue;
        for (int j = 0; j < 20; j++) {
            if (raycast(players[i].x+25, players[i].y+25, players[i].xdir, players[i].ydir, zombies[j].x, zombies[j].y,outT)) {
                zombies[j].z_HP -= 5;
                
                break;
            }
        }
    }
}
bool playerzomcollision(float* x1, float* y1, float* x2, float* y2, int h1, int w1, int h2, int w2) {
    return *x1 < *x2 + w2 &&
        *x1 + w1 > *x2 &&
        *y1 < *y2 + h2 &&
        *y1 + h1 > *y2;
}
bool checkcollision(float* x1, float* y1, float* x2, float* y2, int h1, int w1, int h2, int w2) {
    
    if (*x1 < *x2 + w2 &&
        *x1 + w1 > *x2 &&
        *y1 < *y2 + h2 &&
        *y1 + h1 > *y2) {
        float centerX1 = *x1 + (w1 / 2);
        float centerY1 = *y1 + (h1 / 2);
        float centerX2 = *x2 + (w2 / 2);
        float centerY2 = *y2 + (h2 / 2);

        float dx = centerX1 - centerX2;
        float dy = centerY1 - centerY2;

        float overlapX = ((w1 / 2) + (w2 / 2)) - abs(dx);
        float overlapY = ((h1 / 2) + (h2 / 2)) - abs(dy);
        if (overlapX > 0 && overlapY > 0) {
            if (overlapX < overlapY) {
                // Push along X
                if (dx > 0) *x1 += overlapX;
                else        *x1 -= overlapX;
            }
            else {
                // Push along Y
                if (dy > 0) *y1 += overlapY;
                else        *y1 -= overlapY;
            }
            return true;
        }
        return false;
    }


}
float getdistance(float playerX, float zombieX, float playerY, float zombieY) {
    float dx = playerX - zombieX;
    float dy = playerY - zombieY;
    return sqrtf(dx*dx + dy*dy);
}
void gettarget(zombie* zombie, player players[]) {
    float min = 0;
    int pointer = 0;
    min = getdistance((float)players[0].x,(float)zombie->x,(float)players[0].y, (float)zombie->y);
    for (int i = 0;i < 4;i++) {
        if (!players[i].isConnected)continue;
        float newMin = getdistance((float)players[i].x, (float)zombie->x, (float)players[i].y, (float)zombie->y);
        if (min > newMin) {
            pointer = i;
            min = newMin;
        }
    }
    if (min < zombie->targetRadi ) {
        zombie->target = players[pointer].name;
    }
    else {
        zombie->target = "";
    }
}
void spawnZom(zombie* zombie) {
    if (zombie->isalive == false){
        zombie->z_HP = 100;
        zombie->x = getRandInt(0, 2000);
        zombie->y = getRandInt(0, 2000);
        zombie->isalive = true;
        std::cout << "zombie spawned at x:" << zombie->x << " ,y:" << zombie->y << std::endl;
    }
}
void zomMove(zombie* zombie, player players[],float deltaTime) {
    float DirX = 0;
    float DirY = 0;
    gettarget(zombie, players);
    for (int i = 0; i < 4;i++) {
        
        
        if (zombie->target == players[i].name  && players[i].isConnected) {
            DirX = players[i].x - zombie->x;
            DirY = players[i].y - zombie->y;
            float len = sqrtf(DirX * DirX + DirY * DirY);
            if (len > 0) {
                DirX /= len;
                DirY /= len;
            }
            if (getdistance((float)players[i].x, (float)zombie->x, (float)players[i].y, (float)zombie->y) > 150) {
                
                zombie->x += DirX * zombie->farSpeed * deltaTime;
                zombie->y += DirY * zombie->farSpeed * deltaTime;
               
                break;
            }
            zombie->x += DirX * zombie->closeSpeed * deltaTime;
            zombie->y += DirY * zombie->closeSpeed * deltaTime;
            break;
        }
    }

}
void zomUpdate(Uint32 lastupdate, Uint32 currentTime) {

}

//check available slot
int availableIndex(struct player players[]) {
	for (int i = 0; i < 4; i++) {
		if (!players[i].isConnected ) {
			return i; // Return the index of the first available player slot
            break;
		}
	}
    return -1;
}
//broadcast
void sentToALL(int localPlayerIndex,player players[], ENetPacket* enetPacket) {
    for (int i = 0; i < 4;i++) {
        if (i == localPlayerIndex)continue;

        if (players[i].user != NULL) {
            enet_peer_send(players[i].user, 0, enetPacket); // Send the packet to the server

        }
    }
}
void broadcastZombies(zombie zombies[], int zombieCount, int localPlayerIndex, player players[]) {
    size_t dataSize = sizeof(zombieState) * zombieCount;
    zombieState buffer[20];
    for (int i = 0;i < zombieCount;i++) {
        buffer[i].x = zombies[i].x;
        buffer[i].y = zombies[i].y;
        buffer[i].hp = zombies[i].z_HP;
        buffer[i].alive = zombies[i].isalive;
    }
    ENetPacket* packet = enet_packet_create(buffer, dataSize, ENET_PACKET_FLAG_UNSEQUENCED);
    sentToALL(localPlayerIndex, players, packet);
}

//enet event
void enetEventHandler(ENetEvent *enetEvent, ENetHost* Host,zombie zombies[], struct player players[], ENetPeer** peer, bool isHost, bool isClient, int localPlayerIndex) {
    //host event
    if (isHost && !isClient) {
        while (enet_host_service(Host, enetEvent, 0) > 0) {
            switch (enetEvent->type) {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "A new client connected from " << enetEvent->peer->address.host << ":" << enetEvent->peer->address.port << "\n";
                for (int i = 0;i < 4;i++) {
                    if (i== localPlayerIndex ) continue;
                    
                    if (players[i].user == NULL) {
                        std::cout << "player is connected" << std::endl;
                        players[i].user = enetEvent->peer;
                        players[i].isConnected = true;
                        break;
                    }
                }
                *peer = enetEvent->peer; // Store the peer for later use
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                if (enetEvent->packet->dataLength == sizeof(Packet)) {
                    Packet pkt;
                    memcpy(&pkt, enetEvent->packet->data, sizeof(Packet));
                    for (int i = 0; i < 4; i++) {
                        if (!(players[i].user == enetEvent->peer))continue;
                        
                        if (pkt.type == PACKET_CHAT) {
                            players[i].name = pkt.name;
                            players[i].serverInput = pkt.message;
                        }
                        else if (pkt.type == PACKET_MOVE) {
                            players[i].x = pkt.x;
                            players[i].y = pkt.y;
                            players[i].xdir = pkt.dirx;
                            players[i].ydir = pkt.diry;
                        }
                        else if (pkt.type == PACKET_SHOOT) {
                            players[i].xdir = pkt.dirx;
                            players[i].ydir = pkt.diry;
                            players[i].isShooting = true;
                        }
                        else if (pkt.type == PACKET_STOPSHOOT) {
                            players[i].isShooting = false;
                        }
                        else if (pkt.type == PACKET_PLAYERHEALTH) {
                            players[i].p_HP = pkt.hp;
                        }
                    }
                    
                }
                else {
                    std::cerr << "Received packet of unexpected size!\n";
                }
                    enet_packet_destroy(enetEvent->packet);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "A client disconnected.\n";
                    for (int i = 0; i < 4;i++) {
                        if (enetEvent->peer == players[i].user) {
                            std::cout << "player name::" << players[i].name << ".is disconneted from the server" << std::endl;
                            players[i].isConnected = false;
                            break;

                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
    //client event
    if (isClient && !isHost) {
        while (enet_host_service(Host, enetEvent, 0) > 0) {
            switch (enetEvent->type) {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "Connected to server.\n";
                for (int i = 0;i < 4;i++) {
                    if (localPlayerIndex == i) continue;

                    if (players[i].user == NULL) {
                        players[i].user = enetEvent->peer;
                        players[i].isConnected = true;
                        break;
                    }
                }
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                if (enetEvent->packet->dataLength == sizeof(Packet)) {
                    Packet pkt;
                    memcpy(&pkt, enetEvent->packet->data, sizeof(Packet));

                    for (int i = 0; i < 4; i++) {
                        if (!(players[i].user == enetEvent->peer))continue;
                        if (pkt.type == PACKET_CHAT) {
                            players[i].name = pkt.name;
                            players[i].serverInput = pkt.message;
                        }
                        else if (pkt.type == PACKET_MOVE) {
                            players[i].x = pkt.x;
                            players[i].y = pkt.y;
                            players[i].xdir = pkt.dirx;
                            players[i].ydir = pkt.diry;
                        }
                        else if (pkt.type == PACKET_SHOOT) {
                            players[i].xdir = pkt.dirx;
                            players[i].ydir = pkt.diry;
                            players[i].isShooting = true;
                        }
                        else if (pkt.type == PACKET_STOPSHOOT) {
                            players[i].isShooting = false;
                        }
                        else if (pkt.type == PACKET_PLAYERHEALTH) {
                            players[i].p_HP = pkt.hp;
                        }
                    }  
                }
                else if (enetEvent->packet->dataLength == sizeof(zombieState)*20) {
                    zombieState pkt[20];
                    memcpy(&pkt, enetEvent->packet->data, sizeof(zombieState)*20);
                    for (int i = 0; i < 20; i++) {
                       
                        zombies[i].startX = zombies[i].x;
                        zombies[i].startY = zombies[i].y;
                        zombies[i].targetX = pkt[i].x;
                        zombies[i].targetY = pkt[i].y;
                        zombies[i].lerpTimer = 0.0f;  // restart interpolation

                        zombies[i].z_HP = pkt[i].hp;
                        zombies[i].isalive = pkt[i].alive;

                        float dx = pkt[i].x - zombies[i].x;
                        float dy = pkt[i].y - zombies[i].y;
                        float dist2 = dx * dx + dy * dy;

                        const float TELEPORT_THRESHOLD_SQ = 200.0f * 200.0f; // if > 200 px, teleport
                        if (dist2 > TELEPORT_THRESHOLD_SQ) {
                            // big jump -> snap immediately
                            zombies[i].x = pkt[i].x;
                            zombies[i].y = pkt[i].y;
                            zombies[i].startX = zombies[i].x;
                            zombies[i].startY = zombies[i].y;
                            zombies[i].targetX = pkt[i].x;
                            zombies[i].targetY = pkt[i].y;
                            zombies[i].lerpTimer = 0.0f;
                        }
                        else {
                            // small update -> interpolate from current render pos to new target
                            zombies[i].startX = zombies[i].x;   // store current render pos as start
                            zombies[i].startY = zombies[i].y;
                            zombies[i].targetX = pkt[i].x;
                            zombies[i].targetY = pkt[i].y;
                            zombies[i].lerpTimer = 0.0f;        // restart interpolation
                        }
                        zombies[i].z_HP = pkt[i].hp;
                        zombies[i].isalive = pkt[i].alive;
                    
                    }

                }
                else {
                    std::cerr << "Received packet of unexpected size!\n";
                }
                enet_packet_destroy(enetEvent->packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnected from server.\n";
                for (int i = 0; i < 4;i++) {
                    if (enetEvent->peer == players[i].user) {
                        std::cout << "player name::" << players[i].name << ".is disconneted from the server" << std::endl;
                        players[i].isConnected = false;
                        break;
                    }
                }
                break;
            default:
                break;
            }
        }
    }
}

void eventHandler(SDL_Event& event, bool& running, float* x, float* y, int* clicks, GameStates* CurrentState, std::string* textInput, std::string* serverTextInput,float deltaTime ,player *localplayer,SDL_Rect *camera,bool* mouseButtonPressed) {



    while (SDL_PollEvent(&event)) {
        // Handle events here
        switch (event.type) {
        case SDL_QUIT: std::cerr << "switch quit\n";
            running = false;
            break;
        case SDL_KEYDOWN:
            break;
        case SDL_MOUSEMOTION:
            getplayeraim(localplayer, event.motion.x, event.motion.y,camera->x,camera->y);
            break;
        case  SDL_MOUSEBUTTONDOWN:
            *mouseButtonPressed = true;
            localplayer->isShooting = true;
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
        case SDL_MOUSEBUTTONUP:
            *mouseButtonPressed = false;
            localplayer->isShooting = false;
            std::cout << "player stopped shooting" << std::endl;
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
    float playerSpeed = 200.0f;
    
    if (*CurrentState == MOVEMENT) {
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        
        if (keystate[SDL_SCANCODE_W]) {
            *y -= playerSpeed * deltaTime;
        }
        if (keystate[SDL_SCANCODE_S]) {
            *y += playerSpeed * deltaTime;
        }
        if (keystate[SDL_SCANCODE_A]) {
            *x -= playerSpeed * deltaTime;
        }
        if (keystate[SDL_SCANCODE_D]) {
            *x += playerSpeed * deltaTime;
        }
    }
}

//main function

int main(int argc, char* argv[]) {
	//util input
    int prevPHP = 100;
    bool PrevmouseButtonPressed = false;
    bool mouseButtonPressed = false;
    //players
    struct player players[4];
    bool isHost = false;
    bool isClient = false;
	int localPlayerIndex = 0; // Index of the local player
    //zombies
    struct zombie zombies[20];
    int zombieCount = sizeof(zombies) / sizeof(zombies[0]);
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
        enet_address_set_host(&address, "192.168.0.104"); // or public IP
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
    SDL_Rect camera = { 0 , 0 , WindowWidth , WindowHeight };
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

    int prevX = 0, prevY =0 ;
    int x = WindowHeight / 2, y = WindowWidth / 2, clicks = 0; // Initial position of the rectangle

    // Input handling variables
    std::string textInput = "";
	std::string serverTextInput = "";
    std::string prevServerInput = "";
    
	std::string message = "";
    SDL_Event inputEvent;
    GameStates CurrentState = MOVEMENT;
     
    //enet event handling
    ENetEvent enetEvent;
    //fps
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    int fps = 0;
    int frames = 0;
    Uint32 lastPlayerHit = SDL_GetTicks();
    Uint32 fpslasttime = SDL_GetTicks();
    Uint32 lastZomUpdate = SDL_GetTicks();
    Uint32 lasttime = SDL_GetTicks();
    while (running) {
        
        Uint32 now = SDL_GetTicks();
        frames++;
        if (now - fpslasttime >= 1000) {
            fps = frames;
            frames = 0;
            fpslasttime = now;
            std::cout << "fps::" << fps << std::endl;
        }
        //game loop begins
        Uint32 framestart = SDL_GetTicks();
        float deltaTime = (framestart - lasttime) / 1000.0f;
        lasttime = framestart;


        // Handle events
        prevX = players[localPlayerIndex].x, prevY = players[localPlayerIndex].y;
        prevServerInput = players[localPlayerIndex].serverInput;

        //input handling
        enetEventHandler(&enetEvent, Host, zombies, players, &peer, isHost, isClient, localPlayerIndex); // Handle ENet events
        eventHandler(inputEvent, running, & players[localPlayerIndex].x, &players[localPlayerIndex].y, &clicks, &CurrentState, &textInput, &serverTextInput, deltaTime, &players[localPlayerIndex], &camera,&mouseButtonPressed);
        if (user == 'c') {
            for (int i = 0; i < zombieCount; ++i) {
                if (!zombies[i].isalive) continue;

                zombies[i].lerpTimer += deltaTime;
                float s = (zombies[i].lerpDuration > 0.0f) ? (zombies[i].lerpTimer / zombies[i].lerpDuration) : 1.0f;
                if (s > 1.0f) s = 1.0f;

                // smoothstep easing
                float t = s * s * (3.0f - 2.0f * s);

                zombies[i].x = zombies[i].startX * (1.0f - t) + zombies[i].targetX * t;
                zombies[i].y = zombies[i].startY * (1.0f - t) + zombies[i].targetY * t;

                // once done, clamp and avoid numerical drift
                if (s >= 1.0f) {
                    zombies[i].x = zombies[i].targetX;
                    zombies[i].y = zombies[i].targetY;
                    zombies[i].lerpTimer = 0.0f;
                    // Optionally set startX=start=target so next packet will start from a clean state
                    zombies[i].startX = zombies[i].targetX;
                    zombies[i].startY = zombies[i].targetY;
                }
            }
        }

        
        //update camera
        // where the camera should be if locked
        float targetX = players[localPlayerIndex].x + 25 - WindowWidth / 2;
        float targetY = players[localPlayerIndex].y + 25 - WindowHeight / 2;

        // interpolate towards target
        float factor = 0.1f;  // try values between 0.05f and 0.2f
        camera.x += (targetX - camera.x) * factor;
        camera.y += (targetY - camera.y) * factor;

        //name and message update
        players[localPlayerIndex].serverInput = players[localPlayerIndex].name;
        players[localPlayerIndex].serverInput += ":" + serverTextInput;// Combine player name and server text input

        //caht packets update and broad cast
        if (textInput != players[localPlayerIndex].name || prevServerInput != players[localPlayerIndex].serverInput) {
            std::cout << "prev server text input==" << prevServerInput << std::endl;
            std::cout << "local player server input==" << players[localPlayerIndex].serverInput << std::endl;
            Packet Chat_pkt = {};
            players[localPlayerIndex].name = textInput;

            Chat_pkt.type = PACKET_CHAT;
            strncpy_s(Chat_pkt.name, players[localPlayerIndex].name.c_str(), sizeof(Chat_pkt.name));

            strncpy_s(Chat_pkt.message, players[localPlayerIndex].serverInput.c_str(), sizeof(Chat_pkt.message));


            ENetPacket* enetPacket_chat = enet_packet_create(&Chat_pkt, sizeof(Chat_pkt), ENET_PACKET_FLAG_RELIABLE);
            sentToALL(localPlayerIndex, players, enetPacket_chat);
        }
        //move packet update and braodcast
        if (players[localPlayerIndex].x != prevX || players[localPlayerIndex].y != prevY) {
            Packet Move_pkt = {};
            Move_pkt.type = PACKET_MOVE;
            Move_pkt.x = players[localPlayerIndex].x;
            Move_pkt.y = players[localPlayerIndex].y;
            Move_pkt.dirx = players[localPlayerIndex].xdir;
            Move_pkt.diry = players[localPlayerIndex].ydir;
            ENetPacket* enetPacket_move = enet_packet_create(&Move_pkt, sizeof(Move_pkt), ENET_PACKET_FLAG_UNSEQUENCED);
            sentToALL(localPlayerIndex, players, enetPacket_move);
        }
        //shoot packets
        if (PrevmouseButtonPressed != mouseButtonPressed){
            PrevmouseButtonPressed = mouseButtonPressed;
            if (mouseButtonPressed && players[localPlayerIndex].isShooting) {
                Packet shooting_pkt = {};
                shooting_pkt.type = PACKET_SHOOT;
                shooting_pkt.dirx = players[localPlayerIndex].xdir;
                shooting_pkt.diry = players[localPlayerIndex].ydir;
                ENetPacket* enetPacket_shoot = enet_packet_create(&shooting_pkt, sizeof(shooting_pkt),ENET_PACKET_FLAG_RELIABLE);
                sentToALL(localPlayerIndex, players, enetPacket_shoot);
            }
            else if (!mouseButtonPressed || !players[localPlayerIndex].isShooting) {
                Packet shooting_pkt = {};
                shooting_pkt.type = PACKET_STOPSHOOT;
                ENetPacket* enetPacket_shoot = enet_packet_create(&shooting_pkt, sizeof(shooting_pkt), ENET_PACKET_FLAG_RELIABLE);
                sentToALL(localPlayerIndex, players, enetPacket_shoot);
                std::cout << "shooting stop packet sent" << std::endl;
            }
            PrevmouseButtonPressed = mouseButtonPressed;
        }
        //playerHEALTH packet
        if (players[localPlayerIndex].p_HP != prevPHP) {
            Packet healthupdate_pkt = {};
            healthupdate_pkt.type = PACKET_PLAYERHEALTH;
            healthupdate_pkt.hp = players[localPlayerIndex].p_HP;
            prevPHP = players[localPlayerIndex].p_HP;
            ENetPacket* enetpacket_health = enet_packet_create(&healthupdate_pkt, sizeof(healthupdate_pkt),ENET_PACKET_FLAG_RELIABLE);
            sentToALL(localPlayerIndex, players, enetpacket_health);
        }
        
        //zombies packet
        Uint32 currentZomupdate = SDL_GetTicks();
        if(currentZomupdate-lastZomUpdate >=100 &&user =='h') {
            std::cout << "packetsent" << std::endl;
            
            broadcastZombies(zombies, zombieCount, localPlayerIndex, players);

            lastZomUpdate = currentZomupdate;
        }
        
        checkplayerShots(players, zombies);
        //rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Set the draw color to black
        SDL_RenderClear(renderer); // Clear the renderer with the current draw color
        
        
        //zombie rendering
        for (int i = 0; i < sizeof(zombies) / sizeof(zombies[0]);i++) {
            if (!zombies[i].isalive&& user == 'h') {
                spawnZom(&zombies[i]);

            }
            
                SDL_Rect zomRect = { (int)zombies[i].x - camera.x,(int)zombies[i].y - camera.y,50,50 };
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);//red for zom
                SDL_RenderFillRect(renderer, &zomRect);

               
                SDL_Color white = { 255, 255, 255, 255 }; // White color for text
                SDL_Surface* surface = TTF_RenderText_Solid(font, ("health:"+std::to_string(zombies[i].z_HP)).c_str(), white);
                if (surface) {
                        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                        SDL_Rect nameRect = { zombies[i].x - camera.x + 25 - surface->w / 2, zombies[i].y - camera.y - 30, surface->w, surface->h }; // Center over player rect
                        SDL_RenderCopy(renderer, texture, nullptr, &nameRect);
                        SDL_FreeSurface(surface);
                        SDL_DestroyTexture(texture);
                }
                
            
            if (zombies[i].z_HP == 0) {
                zombies[i].isalive = false;
                zombies[i].z_HP = 100;
                zombies[i].target = "";
            }
        }
        if (user == 'h') {
            for (int i = 0; i < sizeof(zombies) / sizeof(zombies[0]);i++) {
                zomMove(&zombies[i], players, deltaTime);
            }
        }
        //palyer rendering

        for (int i = 0; i < 4; i++) {
            if (!players[i].isConnected) continue; // Skip disconnected players
            if (players[i].p_HP < 0) {
                players[i].isConnected = false;
            }
            SDL_Rect playerRect;

            playerRect = { (int)players[i].x - camera.x,(int)players[i].y - camera.y,50,50 };

            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue for player
            SDL_RenderFillRect(renderer, &playerRect); // Draw player rectangle

            






            //player name rect
            if (!players[i].name.empty() && CurrentState == MOVEMENT) {
                SDL_Color white = { 255, 255, 255, 255 }; // White color for text
                SDL_Surface* surface = TTF_RenderText_Solid(font, players[i].serverInput.c_str(), white);
                if (surface) {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                    SDL_Rect nameRect = { players[i].x - camera.x + 25 - surface->w / 2, players[i].y - camera.y - 30, surface->w, surface->h }; // Center over player rect
                    SDL_RenderCopy(renderer, texture, nullptr, &nameRect);
                    SDL_FreeSurface(surface);
                    SDL_DestroyTexture(texture);
                }
            }

        }
       
        for (int i = 0; i < 20; i++) {
            if (!zombies[i].isalive)continue;
            for (int j = i+1; j < 20; j++) {
                if (!zombies[j].isalive)continue;
                else if (checkcollision(&zombies[j].x, &zombies[j].y, &zombies[i].x, &zombies[i].y, 50, 50, 50, 50)) {  
                }
            }
        }
        if (user == 'h') {
            for (int i = 0; i < 20; i++) {
                if (!zombies[i].isalive)continue;
                for (int j = 0; j < 4; j++) {
                    if (!players[j].isConnected)continue;
                    Uint32 playernowHit = SDL_GetTicks();
                    if (playerzomcollision(&players[j].x, &players[j].y, &zombies[i].x, &zombies[i].y, 50, 50, 50, 50) && playernowHit - lastPlayerHit >= 500) {
                        std::cout << "damage" << std::endl;
                        players[j].p_HP -= 1;
                        lastPlayerHit = playernowHit;
                    }
                }
            }
        }
        
        


        //aim
        for (int i = 0; i < 4; i++) {
            if (!players[i].isConnected) continue;
            float startX = players[i].x - camera.x + 25;
            float startY = players[i].y - camera.y + 25;
            float endX = startX + players[i].xdir * 100;
            float endY = startY + players[i].ydir * 100;
            SDL_RenderDrawLine(renderer, (int)startX, (int)startY, (int)endX, (int)endY);
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
        //fps counter
        std::string FPS_COUNTER = "FPS:: " + std::to_string(fps);
        SDL_Color color = { 255, 255, 255, 255 };
        SDL_Surface* fpssurface = TTF_RenderText_Blended(font, FPS_COUNTER.c_str(), color);
        SDL_Texture* fpstexture = SDL_CreateTextureFromSurface(renderer, fpssurface);
        SDL_Rect fpstextrect = { 0, 0, fpssurface->w, fpssurface->h };
        SDL_FreeSurface(fpssurface);

        SDL_RenderCopy(renderer, fpstexture, NULL, &fpstextrect);
        SDL_DestroyTexture(fpstexture);
        //click counter
        std::string counterText = "health:: " + std::to_string(players[localPlayerIndex].p_HP);
        SDL_Color colorhp = { 255, 255, 255, 255 };
        SDL_Surface* surface = TTF_RenderText_Blended(font, counterText.c_str(), colorhp);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect textrect = { (WindowWidth / 2) - (surface->w / 2), 10, surface->w, surface->h };
        SDL_FreeSurface(surface);

        SDL_RenderCopy(renderer, texture, NULL, &textrect);
        SDL_DestroyTexture(texture);

        // Present the renderer
        SDL_RenderPresent(renderer);

        Uint32 frametime = SDL_GetTicks() - framestart;
        if (frametime < frameDelay) {
            SDL_Delay(frameDelay - frametime);
        }

    }

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