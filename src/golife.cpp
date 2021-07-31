#include <SDL2/SDL.h>
#include <iostream>
#include <stdlib.h>

#define SCR_WIDTH (800)
#define SCR_HEIGHT (600)

#define WORLD_WIDTH (2000)
#define WORLD_HEIGHT (2000)

#define WORLD_SIZE (WORLD_WIDTH * WORLD_HEIGHT)

#define COL_ALIVE 0xffff0000
#define COL_DEAD 0xff000022

struct Cell
{
    bool alive;
    unsigned int generationsAlive;
};

static SDL_Window* win;
static SDL_Surface* surface;
static unsigned int* pixels;

static Cell world0[WORLD_SIZE];
static Cell world1[WORLD_SIZE];
static Cell* worlds[2];
static int currentWorld;

static int cellWidth;
static int cellHeight;

static bool running;
static bool paused;

static int xOffset;
static int yOffset;

inline Cell createDeadCell()
{
    Cell result;
    result = {false, 0};
    return result;
}

inline Cell createAliveCellFrom(const Cell* cell)
{
    Cell result;
    if (!cell->alive)
    {
        result = {true, 0};
    }
    else
    {
        result = {true, cell->generationsAlive + 1};
        if (result.generationsAlive > 510) result.generationsAlive = 510;
    }
    return result;
}

inline Cell createIdenticalCellFrom(const Cell* cell)
{
    Cell result;
    if (cell->alive)
    {
        result = createAliveCellFrom(cell);
    }
    else result = createDeadCell();
    /*result.alive = cell->alive;
    if (result.alive)
    {
        result.generationsAlive = cell->generationsAlive + 1;
        if (result.generationsAlive > 510) result.generationsAlive = 510;
    }
    else result.generationsAlive = 0;*/
    return result;
}



inline Cell getCell(int x, int y, Cell* world)
{
    if (x < 0 || y < 0 || x >= WORLD_WIDTH || y >= WORLD_HEIGHT) return {false, 0};
    Cell result = world[x + y * WORLD_WIDTH];
    return result;
}

void randomizeWorld(Cell* world)
{
    for (int i = 0; i < WORLD_SIZE; ++i)
    {
        bool deadOrAlive = (rand() % 3 == 0);
        world[i] = {deadOrAlive, 0};
    }
}

void updateWorld()
{
    Cell* world = worlds[currentWorld];
    Cell* nextWorld = worlds[!currentWorld];
    for (int y = 0; y < WORLD_HEIGHT; ++y)
    {
        for (int x = 0; x < WORLD_WIDTH; ++x)
        {
            int index = x + y * WORLD_WIDTH;
            int aliveNeighbours = 0;
            
            aliveNeighbours += getCell(x - 1, y - 1, world).alive;
            aliveNeighbours += getCell(x    , y - 1, world).alive;
            aliveNeighbours += getCell(x + 1, y - 1, world).alive;

            aliveNeighbours += getCell(x - 1, y    , world).alive;
            aliveNeighbours += getCell(x + 1, y    , world).alive;

            aliveNeighbours += getCell(x - 1, y + 1, world).alive;
            aliveNeighbours += getCell(x    , y + 1, world).alive;
            aliveNeighbours += getCell(x + 1, y + 1, world).alive;

            if (aliveNeighbours == 2)
                nextWorld[index] = createIdenticalCellFrom(&world[index]);
            else if (aliveNeighbours == 3)
                nextWorld[index] = createAliveCellFrom(&world[index]);
            else
                nextWorld[index] = createDeadCell();
        }
    }
    currentWorld = !currentWorld;
}

static int xStart;
static int yStart;
static int xEnd;
static int yEnd;

void updateCamera()
{
    if (xOffset > 0) xStart = 0;
    else xStart = -xOffset;
    if (yOffset > 0) yStart = 0;
    else yStart = -yOffset;
    if (xStart >= SCR_WIDTH) return;
    if (yStart >= SCR_HEIGHT) return;
    if (WORLD_WIDTH - xOffset < SCR_WIDTH) xEnd = WORLD_WIDTH - xOffset;
    else xEnd = SCR_WIDTH;
    if (WORLD_HEIGHT - yOffset < SCR_HEIGHT) yEnd = WORLD_HEIGHT - yOffset;
    else yEnd = SCR_HEIGHT;
}

unsigned int createColor(unsigned char r, unsigned char g, unsigned char b)
{
    unsigned int result;
//    unsigned char bb = b & 0x000000ff >> 0;
//    unsigned char gg = g & 0x0000ff00 >> 8;
//    unsigned char rr = r & 0x000000ff >> 16;
    result = 0xff << 24 | r << 16 | g << 8 | b;
    return result;
}

void drawGrid()
{
    for (int y = yStart; y < yEnd; ++y)
    {        
        for (int x = xStart; x < xEnd; ++x)
        {
            Cell cell = getCell(x + xOffset, y + yOffset, worlds[currentWorld]);
            unsigned int color;
            if (!cell.alive) color = COL_DEAD;
            else
            {
                unsigned int green;
                unsigned int red;
                if (cell.generationsAlive <= 255)
                {
                    green = 255;
                    red = cell.generationsAlive;
                }
                else
                {
                    red = 255;
                    green = 510 - cell.generationsAlive;
                }
                //unsigned char green = 255 - (cell.generationsAlive);
                //unsigned char red = 255 - green;
                color = createColor((unsigned char)red, (unsigned char)green, 0);
            }
//unsigned int color = cell.alive ? COL_ALIVE : COL_DEAD;
            pixels[x + y * SCR_WIDTH] = color;
        }
    }
}



void run()
{    
    int camXVel = 0;
    int camYVel = 0;
    bool movLeft = false;
    bool movRight = false;
    bool movUp = false;
    bool movDown = false;
    int slowCamSpeed = 10;
    int fastCamSpeed = slowCamSpeed * 2;
    int camSpeed = slowCamSpeed;

    while (running)
    {
        SDL_Event ev;

        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_KEYDOWN)
            {
                switch(ev.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                    {
                        running = false;
                        break;
                    }
                    case SDLK_LSHIFT:
                    {
                        camSpeed = fastCamSpeed;
                        break;
                    }
                    case SDLK_LEFT:
                    case SDLK_a:
                    {
                        movLeft = true;
                        break;
                    }
                    case SDLK_RIGHT:
                    case SDLK_d:
                    {
                        movRight = true;
                        break;
                    }
                    case SDLK_UP:
                    case SDLK_w:
                    {
                        movUp = true;
                        break;
                    }
                    case SDLK_DOWN:
                    case SDLK_s:
                    {
                        movDown = true;
                        break;
                    }
                    case SDLK_SPACE:
                    {
                        paused = !paused;
                        break;
                    }
                    case SDLK_r:
                    {
                        randomizeWorld(worlds[currentWorld]);
                        break;
                    }
                }
            }
            else if (ev.type == SDL_KEYUP)
            {
                switch(ev.key.keysym.sym)
                {
                    case SDLK_LEFT:
                    case SDLK_a:
                    {
                        movLeft = false;
                        break;
                    }
                    case SDLK_RIGHT:
                    case SDLK_d:
                    {
                        movRight = false;
                        break;
                    }
                    case SDLK_UP:
                    case SDLK_w:
                    {
                        movUp = false;
                        break;
                    }
                    case SDLK_DOWN:
                    case SDLK_s:
                    {
                        movDown = false;
                        break;
                    }
                    case SDLK_LSHIFT:
                    {
                        camSpeed = slowCamSpeed;
                        break;
                    }
                }
            }
        }

        camXVel = 0;
        camYVel = 0;

        if (movLeft)
        {
            camXVel -= camSpeed;
        }
        if (movRight)
        {
            camXVel += camSpeed;
        }
        if (movUp)
        {
            camYVel -= camSpeed;
        }
        if (movDown)
        {
            camYVel += camSpeed;
        }
    
        if (!paused)
        {
            updateWorld();
        }
        if (camXVel != 0 || camYVel != 0)
        {
            xOffset += camXVel;
            yOffset += camYVel;
            if (xOffset < 0) xOffset = 0;
            if (yOffset < 0) yOffset = 0;
            if (xOffset > WORLD_WIDTH - SCR_WIDTH)
                xOffset = WORLD_WIDTH - SCR_WIDTH;
            if (yOffset > WORLD_HEIGHT - SCR_HEIGHT)
                yOffset = WORLD_HEIGHT - SCR_HEIGHT;
            updateCamera();
        }
        drawGrid();
        SDL_UpdateWindowSurface(win);
    }
}


bool start()
{
    win = SDL_CreateWindow("GOLife",
                           SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_CENTERED,
                           SCR_WIDTH, SCR_HEIGHT, SDL_WINDOW_SHOWN);
    if (!win)
    {
        return false;
    }

    surface = SDL_GetWindowSurface(win);
    pixels = (unsigned int*)surface->pixels;
    for (int i = 0; i < surface->w * surface->h; ++i)
    {        
        pixels[i] = 0xffffffff;
    }

    cellWidth = SCR_WIDTH / WORLD_WIDTH;
    cellHeight = SCR_HEIGHT / WORLD_HEIGHT;

    worlds[0] = world0;
    worlds[1] = world1;
    currentWorld = 0;
    randomizeWorld(worlds[currentWorld]);
    updateCamera();
    running = true;
    paused = false;
    return true;
        
}

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
    {
        if (!start())
        {
            std::cout << "Init failed!" << std::endl;
        }
        else
        {
            run();
        }
    }
    else
    {
        std::cout << "SDL_Init failed" << std::endl;
    }
    std::cout << "Exiting program..." << std::endl;
    SDL_Quit();
    return 0;
}
