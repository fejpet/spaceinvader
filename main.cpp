#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

const int WIDTH = 800, HEIGHT = 600;

// Define entity ID type
using EntityID = unsigned int;

// Define component ID type
using ComponentID = size_t;

// Define component storage type
template <typename T>
using ComponentStorage = std::vector<T>;

// Define component ID generator
template <typename T>
ComponentID GetComponentID()
{
    static ComponentID id = 0;
    return id++;
}

// Define ECS class
class ECS
{
public:
    // Create entity with given ID
    void CreateEntity(EntityID id)
    {
        m_entities.emplace_back(id);
    }

    // Destroy entity with given ID
    void DestroyEntity(EntityID id)
    {
        for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
        {
            if (it->GetID() == id)
            {
                m_entities.erase(it);
                break;
            }
        }
    }

    // Add component to entity with given ID
    template <typename T>
    void AddComponent(EntityID id, T component)
    {
        GetComponentStorage<T>().emplace_back(component);
        GetComponentMap<T>()[id] = GetComponentStorage<T>().size() - 1;
    }

    // Remove component from entity with given ID
    template <typename T>
    void RemoveComponent(EntityID id)
    {
        auto &component_map = GetComponentMap<T>();
        auto it = component_map.find(id);
        if (it != component_map.end())
        {
            auto &component_storage = GetComponentStorage<T>();
            auto index = it->second;
            component_storage.erase(component_storage.begin() + index);
            component_map.erase(it);
            for (auto &entity : m_entities)
            {
                auto &entity_component_map = GetComponentMap<T>();
                if (entity_component_map.find(entity.GetID()) != entity_component_map.end() && entity_component_map[entity.GetID()] > index)
                {
                    --entity_component_map[entity.GetID()];
                }
            }
        }
    }

    // Get component of entity with given ID
    template <typename T>
    T *GetComponent(EntityID id)
    {
        auto &component_map = GetComponentMap<T>();
        auto it = component_map.find(id);
        if (it != component_map.end())
        {
            return &GetComponentStorage<T>()[it->second];
        }
        return nullptr;
    }

    // Get all entities
    const std::vector<EntityID> GetEntities() const
    {
        std::vector<EntityID> result;
        for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
        {
            result.push_back(it->GetID());
        }

        return result;
    }

private:
    // Define entity class
    class Entity
    {
    public:
        Entity(EntityID id) : m_id(id) {}
        EntityID GetID() const { return m_id; }

    private:
        EntityID m_id;
    };

    // Get component storage of given type
    template <typename T>
    ComponentStorage<T> &GetComponentStorage()
    {
        static ComponentStorage<T> storage;
        return storage;
    }

    // Get component map of given type
    template <typename T>
    std::unordered_map<EntityID, size_t> &GetComponentMap()
    {
        static std::unordered_map<EntityID, size_t> component_map;
        return component_map;
    }

    // Store entities
    std::vector<Entity> m_entities;
};

// Define components
struct PositionComponent
{
    float x, y;
};

struct SpriteComponent
{
    std::string filepath;
    SDL_Texture *texture;
    int w, h;
};

struct TextComponent
{
    std::string text;
    std::string font;
    int size;
    SDL_Texture *texture;
};

// Define systems
class MovementSystem
{
public:
    // Update entity with given ID
    void Update(EntityID id, PositionComponent &position)
    {
        const Uint8 *currentKeyStates = SDL_GetKeyboardState(NULL);
        if (currentKeyStates[SDL_SCANCODE_LEFT])
        {
            if (position.x > 0)
            {
                position.x -= 1.0f;
            }
        }
        if (currentKeyStates[SDL_SCANCODE_RIGHT])
        {
            if (position.x + 64 < WIDTH)
            {
                position.x += 1.0f;
            }
        }
    }
};

class RenderingSystem
{
public:
    // Render all entities with sprite and position components
    void Render(SDL_Renderer *renderer, ECS &ecs)
    {
        for (auto entity_id : ecs.GetEntities())
        {
            auto position = ecs.GetComponent<PositionComponent>(entity_id);
            auto sprite = ecs.GetComponent<SpriteComponent>(entity_id);
            if (position && sprite)
            {
                SDL_Rect dstRect{static_cast<int>(position->x), static_cast<int>(position->y), sprite->w, sprite->h};
                SDL_RenderCopy(renderer, sprite->texture, NULL, &dstRect);
            }
        }
    }
};

class TextRenderingSystem
{
public:
    // Render all entities with text and position components
    void Render(SDL_Renderer *renderer, TTF_Font *font, ECS &ecs)
    {
        for (auto entity_id : ecs.GetEntities())
        {
            auto position = ecs.GetComponent<PositionComponent>(entity_id);
            auto text = ecs.GetComponent<TextComponent>(entity_id);
            if (position && text)
            {
                SDL_Surface *surfaceMessage = TTF_RenderText_Solid(font, text->text.c_str(), {255, 255, 255});
                text->texture = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
                SDL_Rect dstRect{static_cast<int>(position->x), static_cast<int>(position->y), surfaceMessage->w, surfaceMessage->h};
                SDL_RenderCopy(renderer, text->texture, NULL, &dstRect);
                SDL_FreeSurface(surfaceMessage);
            }
        }
    }
};

// Main function
int main(int argc, char *argv[])
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    // Initialize SDL_image
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG)
    {
        std::cout << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() != 0)
    {
        std::cout << "TTF_Init Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create SDL window
    SDL_Window *window = SDL_CreateWindow("SDL Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr)
    {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create SDL renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // Load font
    TTF_Font *font = TTF_OpenFont("resources/arial.ttf", 28);
    if (font == nullptr)
    {
        std::cout << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load sprite
    SDL_Surface *surface = IMG_Load("resources/ship.png");
    if (surface == nullptr)
    {
        std::cout << "IMG_Load Error: " << IMG_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *sprite_texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (sprite_texture == nullptr)
    {
        std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_FreeSurface(surface);

    std::cout << "start create ECS" << std::endl;
    // Create entity-component system
    ECS ecs;

    // Create player entity
    EntityID player_id = 1;
    ecs.CreateEntity(player_id);
    ecs.AddComponent(player_id, PositionComponent{320.0f, HEIGHT - 64});
    ecs.AddComponent(player_id, SpriteComponent{"", sprite_texture, 64, 64});
    ecs.AddComponent(player_id, TextComponent{"Player", "resources/arial.ttf", 28, nullptr});

    // Define systems
    MovementSystem movement_system;
    RenderingSystem rendering_system;
    TextRenderingSystem text_rendering_system;

    // Start game loop
    bool quit = false;
    SDL_Event event;
    std::cout << "before loop" << std::endl;
    while (!quit)
    {
        // Process events
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            }
        }

        // Update game state
        movement_system.Update(player_id, *ecs.GetComponent<PositionComponent>(player_id));

        // Render game state
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        rendering_system.Render(renderer, ecs);
        //  text_rendering_system.Render(renderer, font, ecs);

        SDL_RenderPresent(renderer);
    }

    // Clean up
    TTF_CloseFont(font);
    SDL_DestroyTexture(sprite_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
