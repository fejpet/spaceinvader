#include <iostream>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

const int WIDTH = 800, HEIGHT = 600;
int main(int argc, char *argv[])
{
    std::cout << "hello" << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO) != 0 || ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) || (TTF_Init() != 0))
    {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    if (window == nullptr)
    {
        std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        std::cerr << "Error creating SDL renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    char *basePath = SDL_GetBasePath();
    if (basePath == nullptr)
    {
        std::cout << "Error: SDL_GetBasePath failed." << SDL_GetError() << std::endl;
        return 1;
    }
    std::string shipPath = basePath;
    shipPath += "resources/ship.png";

    // Load sprite texture
    SDL_Surface *surface = IMG_Load(shipPath.c_str());
    if (surface == nullptr)
    {
        std::cerr << "Error loading PNG file: " << IMG_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    // Set texture blend mode and color modulation
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(texture, 255, 255, 255);

    // Set texture position and size
    int shipWidth = 64;
    int shipHeight = 64;

    int x = 100;
    int y = HEIGHT - shipHeight;
    int width = shipWidth;
    int height = shipHeight;

    // Set movement variables
    int speed = 1;
    bool movingLeft = false;
    bool movingRight = false;

    // Load font and set text color
    std::string fontPath = basePath;
    fontPath += "resources/arial.ttf";

    TTF_Font *font = TTF_OpenFont(fontPath.c_str(), 16);
    if (font == nullptr)
    {
        std::cerr << "Error loading font: " << TTF_GetError() << std::endl;
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Color textColor = {255, 255, 255};

    bool quit = false;
    SDL_Event event;
    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                case SDLK_LEFT:
                    movingLeft = true;
                    break;
                case SDLK_RIGHT:
                    movingRight = true;
                    break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym)
                {
                case SDLK_LEFT:
                    movingLeft = false;
                    break;
                case SDLK_RIGHT:
                    movingRight = false;
                    break;
                }
                break;
            }
        }

        // Update game state here...

        // Move sprite left or right
        if (movingLeft)
        {
            if (x > 0)
            {
                x -= speed;
            }
        }
        else if (movingRight)
        {
            if ((x + shipWidth + speed) < WIDTH)
            {
                x += speed;
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render texture
        SDL_Rect destRect = {x, y, width, height};
        SDL_RenderCopy(renderer, texture, nullptr, &destRect);

        // Render x and y as text
        std::string text = "x: " + std::to_string(x) + ", y: " + std::to_string(y) + ", shipWidth:" + std::to_string(shipWidth);
        SDL_Surface *textSurface = TTF_RenderText_Solid(font, text.c_str(), textColor);
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        int textWidth = textSurface->w;
        int textHeight = textSurface->h;
        SDL_Rect textRect = {10, 10, textWidth, textHeight};
        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);

        // Update screen
        SDL_RenderPresent(renderer);
    }

    SDL_free(basePath);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}