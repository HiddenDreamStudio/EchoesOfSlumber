#pragma once

#include "Module.h"
#include <list>
#include <vector>
#include <map>
#include "Player.h"

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

    unsigned int Get(int i, int j) const
    {
        return tiles[(j * width) + i];
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
};

struct AnimatedPlantObject
{
    float x;
    float y;
    float w;
    float h;
    bool  isFront = false;
    std::string tsxPath;        
    AnimationSet anim;            
    SDL_Texture* texture = nullptr;
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

    float lastDt = 0.0f;

    // Translate between map and world coordinates
    Vector2D MapToWorld(int x, int y) const;
    Vector2D WorldToMap(int x, int y);

    TileSet* GetTilesetFromTileId(int gid) const;

    bool LoadProperties(pugi::xml_node& node, Properties& properties);

    Vector2D GetMapSizeInPixels();
    Vector2D GetMapSizeInTiles();

    MapLayer* GetNavigationLayer();

    int GetTileWidth() {
        return mapData.tileWidth;
    }

    int GetTileHeight() {
        return mapData.tileHeight;
    }

    void LoadEntities(std::shared_ptr<Player>& player);
    void SaveEntities(std::shared_ptr<Player> player);

    Vector2D GetCameraPositionInTiles();
    Vector2D GetCameraLimitsInTiles(Vector2D camPosTile);

    void LoadImageLayers();
    void LoadDecorationObjects();
    void LoadAnimatedPlants();

public:
    std::string mapFileName;
    std::string mapPath;
    MapData mapData;

private:
    bool mapLoaded;
    pugi::xml_document mapFileXML;
    //
    std::list<PhysBody*> colliderList;
};
