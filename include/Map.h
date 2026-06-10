#pragma once

#include "Module.h"
#include <list>
#include <vector>
#include <map>
#include "Player.h"
#include "Animation.h"

struct ObjectCollision {
    float x;
    float y;
    float width;
    float height;
    std::vector<int> polygonPoints;
};

struct Properties
{
    struct Property
    {
        std::string name;
        bool value;
    };

    std::list<Property*> propertyList;

    ~Properties()
    {
        for (const auto& property : propertyList)
        {
            delete property;
        }

        propertyList.clear();
    }

    Property* GetProperty(const char* name)
    {
        for (const auto& property : propertyList) {
            if (property->name == name) {
                return property;
            }
        }

        return nullptr;
    }

};

struct MapLayer
{
    int id;
    std::string name;
    int width;
    int height;
    std::vector<int> tiles;
    Properties properties;
    float parallaxFactorX = 1.0f;
    float parallaxFactorY = 1.0f;

    unsigned int Get(int i, int j) const
    {
        // Strip Tiled flip flags (top 3 bits) to return the actual clean tile ID
        return (unsigned int)tiles[(j * width) + i] & 0x1FFFFFFF;
    }
};

struct TileSet
{
    int firstGid;
    std::string name;
    int tileWidth;
    int tileHeight;
    int spacing;
    int margin;
    int tileCount;
    int columns;
    SDL_Texture* texture;
    std::map<int, std::vector<ObjectCollision>> tileCollisions;

    std::map<int, SDL_Texture*> tileTextures;
    // Get the source rect for a tile gid
    SDL_Rect GetRect(unsigned int gid) {
        SDL_Rect rect = { 0 };

        int relativeIndex = gid - firstGid;
        rect.w = tileWidth;
        rect.h = tileHeight;
        rect.x = margin + (tileWidth + spacing) * (relativeIndex % columns);
        rect.y = margin + (tileHeight + spacing) * (relativeIndex / columns);

        return rect;
    }

};

struct ImageLayer
{
    std::string name;
    float offsetX;
    float offsetY;
    std::string source;
    SDL_Texture* texture = nullptr;
    float parallaxFactorX = 1.0f;
    float parallaxFactorY = 1.0f;
};

struct DecorationObject
{
    float x;      
    float y;     
    float width;  
    float height; 
    int   gid;    
    double rotation = 0.0;
    bool  isFront = false;
    SDL_Texture* texture = nullptr; 
    bool flipH = false;  
    bool flipV = false;
    float parallaxSpeed = 1.0f;  // Parallax scroll speed (< 1 = BG slower, > 1 = FG faster)
};

struct AnimatedPlantObject
{
    float x;
    float y;
    float w;
    float h;
    bool  isFront = false;
    float parallaxSpeed = 1.0f;
    std::string tsxPath;        
    AnimationSet anim;            
    SDL_Texture* texture = nullptr;
};

struct CheckpointObject {
    float x, y, width, height;
    bool visited = false;
};

struct PortalData {
    float x, y, w, h;       
    std::string targetFile; 
    std::string targetSpawn; 
    std::string selfId;     
    std::string spawnId;
    std::string target;  
};

struct SpawnData {
    float x, y;
    std::string spawnId;
};

struct MapData
{
    int width;
    int height;
    int tileWidth;
    int tileHeight;
    std::list<TileSet*> tilesets;
    // L07: TODO 2: Add the info to the MapLayer Struct
    std::list<MapLayer*> layers;
    std::list<ImageLayer*> imageLayers;
    std::list<DecorationObject*> decorationObjects;
    std::list<AnimatedPlantObject*> animatedPlants;
    std::vector<CheckpointObject*> checkpoints;

    // Cape collectible spawn position (read from Entities layer)
    bool  capeFound = false;
    float capeX = 0.0f;
    float capeY = 0.0f;

    // Slingshot collectible spawn position (read from Entities/InteractiveAssets)
    bool  slingshotFound = false;
    float slingshotX = 0.0f;
    float slingshotY = 0.0f;

    // Stuffed animal collectible spawn position (read from Entities)
    bool  stuffedAnimalFound = false;
    float stuffedAnimalX = 0.0f;
    float stuffedAnimalY = 0.0f;
};

class Map : public Module
{
public:

    Map();

    // Destructor
    virtual ~Map();

    // Called before render is available
    bool Awake();

    // Called before the first frame
    bool Start();

    // Called each loop iteration
    bool Update(float dt);

    // Called after Update, for foreground elements
    bool PostUpdate();

    // Called before quitting
    bool CleanUp();

    // Load new map
    bool Load(std::string path, std::string mapFileName);

    // Translate between map and world coordinates
    Vector2D MapToWorld(int x, int y) const;
    Vector2D WorldToMap(int x, int y);

    TileSet* GetTilesetFromTileId(int gid) const;

    bool LoadProperties(pugi::xml_node& node, Properties& properties);

    Vector2D GetMapSizeInPixels();
    Vector2D GetMapSizeInTiles();

    // Cape position read from TMX Entities layer
    bool  GetCapePosition(float& outX, float& outY) const;

    // Slingshot position read from TMX
    bool  GetSlingshotPosition(float& outX, float& outY) const;

    // Stuffed animal position read from TMX
    bool  GetStuffedAnimalPosition(float& outX, float& outY) const;

    MapLayer* GetNavigationLayer();

    int GetTileWidth() {
        return mapData.tileWidth;
    }

    int GetTileHeight() {
        return mapData.tileHeight;
    }

    void LoadEntities(std::shared_ptr<Player>& player, bool portalTransition = false, float spawnX = 0.0f, float spawnY = 0.0f);
    void SaveEntities(std::shared_ptr<Player> player);

    void LoadImageLayers();
    void LoadDecorationObjects();
    void LoadAnimatedPlants();

    std::vector<PortalData> GetPortals() const;
    bool GetSpawnById(const std::string& id, float& outX, float& outY) const;

    struct MapObject {
        std::string name;
        SDL_FRect rect;
        std::string platformTarget;
        int requiredClicks = 1;
        bool flipH = false;

    };
    std::vector<MapObject> GetPuzzleObjects() const;
    std::vector<MapObject> GetPuzzleObjects3() const;


public:
    std::string mapFileName;
    std::string mapPath;
    MapData mapData;

private:
    bool mapLoaded;
    pugi::xml_document mapFileXML;
    //
    std::list<PhysBody*> colliderList;

public:
    bool hasInitCamera = false;
    float initCameraX = 0.0f;
    float initCameraY = 0.0f;
};
