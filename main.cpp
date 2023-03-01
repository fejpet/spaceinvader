#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

const int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

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
private:
    EntityID GetNextEntityID()
    {
        static EntityID nextEntityId = 0;
        return nextEntityId++;
    }

public:
    // Create entity with given ID
    EntityID CreateEntity()
    {
        EntityID id = GetNextEntityID();
        m_entities.emplace_back(id);
        return id;
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

struct PlayerComponent
{
    std::string name;
    int health;
};

struct EnemyComponent
{
    int health;
};

// Define components
struct PositionComponent
{
    float x, y;
};
struct ProjectileComponent
{
    int damage;
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

struct VelocityComponent
{
    int x, y;
};
struct InputComponent
{
    bool up;
    bool down;
    bool left;
    bool right;
    bool spacebar;
    bool shoot;
    bool restart;
    bool quit;

    void Reset()
    {
        up = false;
        down = false;
        left = false;
        right = false;
        spacebar = false;
        shoot = false;
        restart = false;
        quit = false;
    }
};

struct InputSystem
{
    void handleEvent(const SDL_Event &event, ECS &ecs)
    {
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
        {
            for (auto entity_id : ecs.GetEntities())
            {
                InputComponent *input = ecs.GetComponent<InputComponent>(entity_id);
                if (input)
                {
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_UP:
                        input->up = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_DOWN:
                        input->down = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_LEFT:
                        input->left = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_RIGHT:
                        input->right = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_RETURN:
                        input->restart = event.type == SDL_KEYDOWN;
                        break;
                    case SDLK_SPACE:
                        input->shoot = (event.type == SDL_KEYDOWN && !input->spacebar);
                        input->spacebar = (event.type == SDL_KEYDOWN);
                        break;
                    case SDLK_ESCAPE:
                        input->quit = event.type == SDL_KEYDOWN;
                        break;
                    }
                }
            }
        }
    }
};

class EnemyMovementSystem
{
    const float speed = 5.0f;

public:
    // Update entity with given ID
    void Update(float deltaTime, ECS &ecs)
    {
        for (auto entity_id : ecs.GetEntities())
        {
            EnemyComponent *enemy = ecs.GetComponent<EnemyComponent>(entity_id);
            PositionComponent *position = ecs.GetComponent<PositionComponent>(entity_id);
            VelocityComponent *velocity = ecs.GetComponent<VelocityComponent>(entity_id);

            if (enemy && position && velocity)
            {
                position->x += velocity->x * deltaTime;

                if (position->x < 10 || position->x > SCREEN_WIDTH - 64)
                {
                    // all enemy one line down and reverse direction
                    for (auto enemy_id : ecs.GetEntities())
                    {
                        EnemyComponent *enemy = ecs.GetComponent<EnemyComponent>(enemy_id);
                        PositionComponent *position = ecs.GetComponent<PositionComponent>(enemy_id);
                        VelocityComponent *velocity = ecs.GetComponent<VelocityComponent>(enemy_id);
                        if (enemy && position && velocity)
                        {
                            position->y += 64;
                            velocity->x = velocity->x * -1;
                        }
                    }
                }
            }
        }
    }
};

// Define systems
class MovementSystem
{
    const float speed = 5.0f;

public:
    // Update entity with given ID
    void Update(float deltaTime, EntityID player_id, ECS &ecs)
    {
        InputComponent *input = ecs.GetComponent<InputComponent>(player_id);
        PositionComponent *position = ecs.GetComponent<PositionComponent>(player_id);
        VelocityComponent *velocity = ecs.GetComponent<VelocityComponent>(player_id);
        auto text = ecs.GetComponent<TextComponent>(player_id);

        if (input && position)
        {
            if (input->left)
            {
                if (position->x > 0)
                {
                    position->x -= speed;
                }
            }
            else if (input->right)
            {
                if (position->x < SCREEN_WIDTH - 64)
                {
                    position->x += speed;
                }
            }
        }

        if (text)
        {
            std::stringstream ss;
            ss << "x:";
            ss << position->x;

            text->text = ss.str();
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

class ProjectileSystem
{
    SDL_Texture *projectileTexture;

public:
    ProjectileSystem(SDL_Texture *projectile_texture)
    {
        projectileTexture = projectile_texture;
    }
    void Update(float deltaTime, EntityID player_id, ECS &ecs)
    {
        // Check if the space bar is pressed
        for (auto entity_id : ecs.GetEntities())
        {
            InputComponent *input = ecs.GetComponent<InputComponent>(entity_id);
            PositionComponent *position = ecs.GetComponent<PositionComponent>(entity_id);
            VelocityComponent *velocity = ecs.GetComponent<VelocityComponent>(entity_id);
            ProjectileComponent *projectile = ecs.GetComponent<ProjectileComponent>(entity_id);

            // handle input
            if (input)
            {
                if (input->shoot)
                {
                    FireProjectile(entity_id, ecs);
                    input->shoot = false;
                }
            }

            if (position && velocity && projectile)
            {
                position->x += velocity->x * deltaTime;
                position->y += velocity->y * deltaTime;

                if (position->y < 0)
                {
                    std::cout << "item run out above id: " << entity_id << std::endl;
                    // out of screen remove it
                    ecs.DestroyEntity(entity_id);
                }

                for (auto enemy_id : ecs.GetEntities())
                {
                    auto enemy = ecs.GetComponent<EnemyComponent>(enemy_id);
                    if (enemy)
                    {
                        PositionComponent *enemyPos = ecs.GetComponent<PositionComponent>(enemy_id);

                        SDL_Rect projectileLoc{static_cast<int>(position->x), static_cast<int>(position->y), 3, 10};
                        SDL_Rect enemyLoc{static_cast<int>(enemyPos->x), static_cast<int>(enemyPos->y), 64, 64};
                        if (SDL_HasIntersection(&projectileLoc, &enemyLoc))
                        {
                            std::cout << "Hit by:" << entity_id << " at :" << enemy_id << std::endl;
                            ecs.DestroyEntity(entity_id);
                            ecs.DestroyEntity(enemy_id);
                            break;
                        }
                    }
                }
            }
        }
    }
    void FireProjectile(EntityID player_id, ECS &ecs)
    {
        PlayerComponent *player = ecs.GetComponent<PlayerComponent>(player_id);
        PositionComponent *position = ecs.GetComponent<PositionComponent>(player_id);
        VelocityComponent *velocity = ecs.GetComponent<VelocityComponent>(player_id);
        if (player != nullptr)
        {
            auto projectile_id = ecs.CreateEntity();
            std::cout << "Fire projectile id:" << projectile_id << std::endl;
            ecs.AddComponent<PositionComponent>(projectile_id, PositionComponent{position->x + 32, position->y - 30});
            ecs.AddComponent<VelocityComponent>(projectile_id, VelocityComponent{0, -100});
            ecs.AddComponent<ProjectileComponent>(projectile_id, ProjectileComponent{1});
            ecs.AddComponent<SpriteComponent>(projectile_id, SpriteComponent{"", projectileTexture, 3, 10});
        }
    }
};

class TextRenderingSystem
{
public:
    // Render all entities with text and position components
    void Render(SDL_Renderer *renderer, ECS &ecs)
    {
        for (auto entity_id : ecs.GetEntities())
        {
            auto position = ecs.GetComponent<PositionComponent>(entity_id);
            auto text = ecs.GetComponent<TextComponent>(entity_id);
            if (position && text)
            {
                // Load font
                TTF_Font *font = TTF_OpenFont(text->font.c_str(), text->size);
                if (font == nullptr)
                {
                    continue;
                }
                SDL_Surface *surfaceMessage = TTF_RenderText_Solid(font, text->text.c_str(), {255, 255, 255});
                text->texture = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
                SDL_Rect dstRect{static_cast<int>(position->x), static_cast<int>(position->y - 5), surfaceMessage->w, surfaceMessage->h};
                SDL_RenderCopy(renderer, text->texture, NULL, &dstRect);
                SDL_FreeSurface(surfaceMessage);
                TTF_CloseFont(font);
            }
        }
    }
};

SDL_Texture *LoadTexture(std::string path, SDL_Renderer *renderer)
{
    SDL_Surface *surface = IMG_Load(path.c_str());
    if (surface == nullptr)
    {
        std::cout << "IMG_Load Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture *sprite_texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (sprite_texture == nullptr)
    {
        std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return nullptr;
    }
    SDL_FreeSurface(surface);
    return sprite_texture;
}

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
    SDL_Window *window = SDL_CreateWindow("SDL Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
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

    // Load sprite
    std::string playerTexturePath = "resources/ship.png";
    SDL_Texture *player_texture = LoadTexture(playerTexturePath, renderer);
    if (player_texture == nullptr)
    {
        std::cout << "Player Texture Load Error: " << playerTexturePath << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::string enemyTexturePath = "resources/enemy.png";
    SDL_Texture *enemy_texture = LoadTexture(enemyTexturePath, renderer);
    if (enemy_texture == nullptr)
    {
        std::cout << "Enemy Texture Load Error: " << enemyTexturePath << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::string projectileTexturePath = "resources/projectile.png";
    SDL_Texture *projectile_texture = LoadTexture(projectileTexturePath, renderer);
    if (projectile_texture == nullptr)
    {
        std::cout << "Enemy Texture Load Error: " << projectileTexturePath << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::cout << "start create ECS" << std::endl;
    // Create entity-component system
    ECS ecs;

    // Create player entity
    InputComponent playerInputComponent = InputComponent{false, false, false, false, false, false, false, false};

    EntityID player_id = ecs.CreateEntity();
    ecs.AddComponent(player_id, PositionComponent{320.0f, SCREEN_HEIGHT - 64});
    ecs.AddComponent(player_id, PlayerComponent{"Player 1", 10});
    ecs.AddComponent(player_id, SpriteComponent{"", player_texture, 64, 64});
    ecs.AddComponent(player_id, TextComponent{"Player", "resources/arial.ttf", 28, nullptr});
    ecs.AddComponent(player_id, playerInputComponent);

    int textureSize = 64;
    int enemyLines = 3;
    for (size_t j = 0; j < enemyLines; j++)
    {
        for (size_t i = 10; i < SCREEN_WIDTH - textureSize; i += (textureSize * 2))
        {
            EntityID enemy_id = ecs.CreateEntity();
            ecs.AddComponent(enemy_id, PositionComponent{(float)i, (float)j * textureSize});
            ecs.AddComponent(enemy_id, SpriteComponent{"", enemy_texture, 64, 64});
            ecs.AddComponent(enemy_id, TextComponent{"Enemy", "resources/arial.ttf", 10, nullptr});
            ecs.AddComponent(enemy_id, VelocityComponent{10, 0});
            ecs.AddComponent(enemy_id, EnemyComponent{1});
        }
    }
    // Define systems
    MovementSystem movement_system;
    EnemyMovementSystem enemy_movement_system;

    RenderingSystem rendering_system;
    TextRenderingSystem text_rendering_system;
    ProjectileSystem projectile_system(projectile_texture);
    InputSystem input_system;

    // Start game loop
    uint32_t previousTime = SDL_GetTicks();
    bool quit = false;
    SDL_Event event;
    std::cout << "before loop" << std::endl;
    while (!quit)
    {
        // Process events
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || playerInputComponent.quit)
            {
                std::cout << "exiting" << std::endl;
                quit = true;
                break;
            }
            input_system.handleEvent(event, ecs);
        }
        // collected events

        uint32_t currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - previousTime) / 1000.0f;
        previousTime = currentTime;
        // Update game state
        movement_system.Update(deltaTime, player_id, ecs);
        enemy_movement_system.Update(deltaTime, ecs);
        projectile_system.Update(deltaTime, player_id, ecs);
        //  Render game state
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        rendering_system.Render(renderer, ecs);
        text_rendering_system.Render(renderer, ecs);

        SDL_RenderPresent(renderer);
    }

    // Clean up

    SDL_DestroyTexture(player_texture);
    SDL_DestroyTexture(enemy_texture);
    SDL_DestroyTexture(projectile_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
