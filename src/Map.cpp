#include "Engine.h"
#include "Render.h"
#include "Textures.h"
#include "Map.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Enemy.h"
#include "tracy/Tracy.hpp"

#include <math.h>
#include <sstream>

Map::Map() : Module(), mapLoaded(false)
{
    name = "map";
}

// Destructor
Map::~Map()
{
}

// Called before render is available
bool Map::Awake()
{
    name = "map";
    LOG("Loading Map Parser");

    return true;
}

bool Map::Start() {

    return true;
}

bool Map::Update(float dt)
{
    ZoneScoped;

    bool ret = true;

    if (mapLoaded) {

        for (const auto& imgLayer : mapData.imageLayers) {
            if (imgLayer->texture) {
                Engine::GetInstance().render->DrawTexture(
                    imgLayer->texture,
                    (int)imgLayer->offsetX,
                    (int)imgLayer->offsetY
                );
            }
        }
        for (const auto& deco : mapData.decorationObjects) {
            if (deco->texture) {
                int drawX = (int)deco->x;
                int drawY = (int)(deco->y - deco->height);
                Engine::GetInstance().render->DrawTexture(deco->texture, drawX, drawY, nullptr, 1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, 1.0f);
            }
        }
        // L07 TODO 5: Prepare the loop to draw all tiles in a layer + DrawTexture()
        // iterate all tiles in a layer
        for (const auto& mapLayer : mapData.layers) {
            //L09 TODO 7: Check if the property Draw exist get the value, if it's true draw the lawyer
            if (mapLayer->properties.GetProperty("Draw") != NULL && mapLayer->properties.GetProperty("Draw")->value == true) {
                for (int i = 0; i < mapData.width; i++) {
                    for (int j = 0; j < mapData.height; j++) {

                        // L07 TODO 9: Complete the draw function

                        //Get the gid from tile
                        int gid = mapLayer->Get(i, j);

                        //Check if the gid is different from 0 - some tiles are empty
                        if (gid != 0) {
                            //L09: TODO 3: Obtain the tile set using GetTilesetFromTileId
                            TileSet* tileSet = GetTilesetFromTileId(gid);
                            if (tileSet != nullptr) {
                                //Get the Rect from the tileSetTexture;
                                SDL_Rect tileRect = tileSet->GetRect(gid);
                                //Get the screen coordinates from the tile coordinates
                                Vector2D mapCoord = MapToWorld(i, j);
                                //Draw the texture
                                Engine::GetInstance().render->DrawTexture(tileSet->texture, (int)mapCoord.getX(), (int)mapCoord.getY(), &tileRect);
                            }
                        }
                    }
                }
            }
        }
    }

    return ret;
}

// L09: TODO 2: Implement function to the Tileset based on a tile id
TileSet* Map::GetTilesetFromTileId(int gid) const
{
    TileSet* set = nullptr;
    for (const auto& tileset : mapData.tilesets) {
        set = tileset;
        if (gid >= tileset->firstGid && gid < tileset->firstGid + tileset->tileCount) {
            break;
        }
    }
    return set;
}

// Called before quitting
bool Map::CleanUp()
{
    LOG("Unloading map");

    // L06: TODO 2: Make sure you clean up any memory allocated from tilesets/map
    for (const auto& tileset : mapData.tilesets) {
        if (tileset->texture) {
            Engine::GetInstance().textures->UnLoad(tileset->texture);
        }
        delete tileset;
    }
    mapData.tilesets.clear();

    // L07 TODO 2: clean up all layer data
    for (const auto& layer : mapData.layers)
    {
        delete layer;
    }
    mapData.layers.clear();

    // Clean up collider list
    for (const auto& collider : colliderList) {
        Engine::GetInstance().physics->DeletePhysBody(collider);
    }
    colliderList.clear();

    for (const auto& imgLayer : mapData.imageLayers) {
        if (imgLayer->texture)
            Engine::GetInstance().textures->UnLoad(imgLayer->texture);
        delete imgLayer;
    }
    mapData.imageLayers.clear();

    for (const auto& deco : mapData.decorationObjects) {
        delete deco;
    }
    mapData.decorationObjects.clear();

    return true;
}

// Load new map
bool Map::Load(std::string path, std::string fileName)
{
    bool ret = false;

    // Assigns the name of the map file and the path
    mapFileName = fileName;
    mapPath = path;
    std::string mapPathName = mapPath + mapFileName;

    //L15 TODO 2: make mapFileXML an attribute of the Map class
    pugi::xml_parse_result result = mapFileXML.load_file(mapPathName.c_str());

    if (result == NULL)
    {
        LOG("Could not load map xml file %s. pugi error: %s", mapPathName.c_str(), result.description());
        ret = false;
    }
    else {

        // L06: TODO 3: Implement LoadMap to load the map properties
        // retrieve the paremeters of the <map> node and store the into the mapData struct
        mapData.width = mapFileXML.child("map").attribute("width").as_int();
        mapData.height = mapFileXML.child("map").attribute("height").as_int();
        mapData.tileWidth = mapFileXML.child("map").attribute("tilewidth").as_int();
        mapData.tileHeight = mapFileXML.child("map").attribute("tileheight").as_int();

        // L06: TODO 4: Implement the LoadTileSet function to load the tileset properties

        //Iterate the Tileset
        for (pugi::xml_node tilesetNode = mapFileXML.child("map").child("tileset"); tilesetNode != NULL; tilesetNode = tilesetNode.next_sibling("tileset"))
        {
            //Load Tileset attributes
            TileSet* tileSet = new TileSet();
            tileSet->firstGid = tilesetNode.attribute("firstgid").as_int();
            tileSet->name = tilesetNode.attribute("name").as_string();
            tileSet->tileWidth = tilesetNode.attribute("tilewidth").as_int();
            tileSet->tileHeight = tilesetNode.attribute("tileheight").as_int();
            tileSet->spacing = tilesetNode.attribute("spacing").as_int();
            tileSet->margin = tilesetNode.attribute("margin").as_int();
            tileSet->tileCount = tilesetNode.attribute("tilecount").as_int();
            tileSet->columns = tilesetNode.attribute("columns").as_int();

            //Load the tileset image (skip if tileset uses per-tile images)
            std::string imgName = tilesetNode.child("image").attribute("source").as_string();
            if (!imgName.empty()) {
                tileSet->texture = Engine::GetInstance().textures->Load((mapPath + imgName).c_str());
            } else {
                tileSet->texture = nullptr;
            }

            // Parse tile object groups for collisions
            for (pugi::xml_node tileNode = tilesetNode.child("tile"); tileNode != NULL; tileNode = tileNode.next_sibling("tile")) {
                int tileId = tileNode.attribute("id").as_int();
                pugi::xml_node objectGroupNode = tileNode.child("objectgroup");
                if (objectGroupNode != NULL) {
                    std::vector<ObjectCollision> collisions;
                    for (pugi::xml_node objectNode = objectGroupNode.child("object"); objectNode != NULL; objectNode = objectNode.next_sibling("object")) {
                        ObjectCollision col;
                        col.x = objectNode.attribute("x").as_float(0.0f);
                        col.y = objectNode.attribute("y").as_float(0.0f);
                        col.width = objectNode.attribute("width").as_float(0.0f);
                        col.height = objectNode.attribute("height").as_float(0.0f);


                        
                        pugi::xml_node polyNode = objectNode.child("polygon");
                        if (polyNode != NULL) {
                            std::string pointsStr = polyNode.attribute("points").as_string();
                            std::stringstream ss(pointsStr);
                            std::string pointPair;
                            while(std::getline(ss, pointPair, ' ')) {
                                if(pointPair.empty()) continue;
                                std::stringstream ssPair(pointPair);
                                std::string xStr, yStr;
                                if(std::getline(ssPair, xStr, ',') && std::getline(ssPair, yStr, ',')) {
                                    col.polygonPoints.push_back((int)std::stof(xStr));
                                    col.polygonPoints.push_back((int)std::stof(yStr));
                                }
                            }
                        }
                        collisions.push_back(col);
                    }
                    if (!collisions.empty()) {
                        tileSet->tileCollisions[tileId] = collisions;
                    }
                }
            }

            mapData.tilesets.push_back(tileSet);
        }

        // L07: TODO 3: Iterate all layers in the TMX and load each of them
        for (pugi::xml_node layerNode = mapFileXML.child("map").child("layer"); layerNode != NULL; layerNode = layerNode.next_sibling("layer")) {

            // L07: TODO 4: Implement the load of a single layer 
            //Load the attributes and saved in a new MapLayer
            MapLayer* mapLayer = new MapLayer();
            mapLayer->id = layerNode.attribute("id").as_int();
            mapLayer->name = layerNode.attribute("name").as_string();
            mapLayer->width = layerNode.attribute("width").as_int();
            mapLayer->height = layerNode.attribute("height").as_int();

            //L09: TODO 6 Call Load Layer Properties
            LoadProperties(layerNode, mapLayer->properties);

            //Iterate over all the tiles and assign the values in the data array
            for (pugi::xml_node tileNode = layerNode.child("data").child("tile"); tileNode != NULL; tileNode = tileNode.next_sibling("tile")) {
                mapLayer->tiles.push_back(tileNode.attribute("gid").as_int());
            }

            //add the layer to the map
            mapData.layers.push_back(mapLayer);
        }

        // L08 TODO 3: Create colliders
        // L08 TODO 7: Assign collider type
        // Optimized with greedy meshing: merge rectangular colliders both horizontally and vertically

        for (const auto& mapLayer : mapData.layers) {
            // Skip the old Collisions metadata layer
            if (mapLayer->name == "Collisions") continue;

            // First pass: create polygon colliders individually (can't merge these)
            // and build a grid marking tiles that need rectangular colliders
            std::vector<bool> hasRectCollider(mapData.width * mapData.height, false);

            for (int j = 0; j < mapData.height; j++) {
                for (int i = 0; i < mapData.width; i++) {
                    int gid = mapLayer->Get(i, j);
                    if (gid == 0) continue;

                    TileSet* tileSet = GetTilesetFromTileId(gid);
                    if (tileSet == nullptr || tileSet->tileCollisions.empty()) continue;

                    int relativeId = gid - tileSet->firstGid;
                    auto it = tileSet->tileCollisions.find(relativeId);
                    if (it == tileSet->tileCollisions.end()) continue;

                    for (const auto& col : it->second) {
                        if (col.polygonPoints.size() > 0) {
                            // Polygon: create individually
                            Vector2D mapCoord = MapToWorld(i, j);
                            int numVerts = (int)col.polygonPoints.size() / 2;
                            PhysBody* c1 = nullptr;
                            if (numVerts >= 4) {
                                c1 = Engine::GetInstance().physics.get()->CreateChain(
                                    (int)(mapCoord.getX() + col.x),
                                    (int)(mapCoord.getY() + col.y),
                                    (int*)col.polygonPoints.data(),
                                    (int)col.polygonPoints.size(),
                                    STATIC);
                            } else if (numVerts >= 3) {
                                c1 = Engine::GetInstance().physics.get()->CreateConvexPolygon(
                                    (int)(mapCoord.getX() + col.x),
                                    (int)(mapCoord.getY() + col.y),
                                    (int*)col.polygonPoints.data(),
                                    (int)col.polygonPoints.size(),
                                    STATIC);
                            }
                            if (c1 != nullptr) {
                                c1->ctype = ColliderType::PLATFORM;
                                colliderList.push_back(c1);
                            }
                        } else {
                            // Mark for rectangle merging
                            hasRectCollider[j * mapData.width + i] = true;
                        }
                    }
                }
            }

            // Second pass: greedy mesh to merge rectangles
            std::vector<bool> visited(mapData.width * mapData.height, false);

            for (int j = 0; j < mapData.height; j++) {
                for (int i = 0; i < mapData.width; i++) {
                    if (!hasRectCollider[j * mapData.width + i]) continue;
                    if (visited[j * mapData.width + i]) continue;

                    // Extend right as far as possible
                    int w = 1;
                    while (i + w < mapData.width &&
                        hasRectCollider[j * mapData.width + (i + w)] &&
                        !visited[j * mapData.width + (i + w)]) {
                        w++;
                    }

                    // Extend down as far as possible for the full width
                    int h = 1;
                    bool canExtend = true;
                    while (j + h < mapData.height && canExtend) {
                        for (int k = 0; k < w; k++) {
                            int idx = (j + h) * mapData.width + (i + k);
                            if (!hasRectCollider[idx] || visited[idx]) {
                                canExtend = false;
                                break;
                            }
                        }
                        if (canExtend) h++;
                    }

                    // Mark all tiles in this merged rect as visited
                    for (int jj = 0; jj < h; jj++) {
                        for (int ii = 0; ii < w; ii++) {
                            visited[(j + jj) * mapData.width + (i + ii)] = true;
                        }
                    }

                    // Create one large rectangle collider
                    float px = (float)(i * mapData.tileWidth);
                    float py = (float)(j * mapData.tileHeight);
                    float totalW = (float)(w * mapData.tileWidth);
                    float totalH = (float)(h * mapData.tileHeight);

                    PhysBody* c1 = Engine::GetInstance().physics.get()->CreateRectangle(
                        (int)(px + totalW / 2.0f),
                        (int)(py + totalH / 2.0f),
                        (int)totalW,
                        (int)totalH,
                        STATIC);
                    c1->ctype = ColliderType::PLATFORM;
                    colliderList.push_back(c1);
                }
            }
        }

        LoadImageLayers();
        LoadDecorationObjects();

        ret = true;

        // L06: TODO 5: LOG all the data loaded iterate all tilesetsand LOG everything
        if (ret == true)
        {
            LOG("Successfully parsed map XML file :%s", fileName.c_str());
            LOG("width : %d height : %d", mapData.width, mapData.height);
            LOG("tile_width : %d tile_height : %d", mapData.tileWidth, mapData.tileHeight);
            LOG("Tilesets----");

            //iterate the tilesets
            for (const auto& tileset : mapData.tilesets) {
                LOG("name : %s firstgid : %d", tileset->name.c_str(), tileset->firstGid);
                LOG("tile width : %d tile height : %d", tileset->tileWidth, tileset->tileHeight);
                LOG("spacing : %d margin : %d", tileset->spacing, tileset->margin);
            }

            LOG("Layers----");

            for (const auto& layer : mapData.layers) {
                LOG("id : %d name : %s", layer->id, layer->name.c_str());
                LOG("Layer width : %d Layer height : %d", layer->width, layer->height);
            }
        }
        else {
            LOG("Error while parsing map file: %s", mapPathName.c_str());
        }

        //L15 TODO 2: Remove mapFileXML.reset(); we want keep a reference to the XML

    }

    mapLoaded = ret;
    return ret;
}

// L07: TODO 8: Create a method that translates x,y coordinates from map positions to world positions
Vector2D Map::MapToWorld(int x, int y) const
{
    Vector2D ret;

    ret.setX((float)(x * mapData.tileWidth));
    ret.setY((float)(y * mapData.tileHeight));

    return ret;
}

Vector2D Map::WorldToMap(int x, int y) {

    Vector2D ret(0, 0);
    ret.setX((float)(x / mapData.tileWidth));
    ret.setY((float)(y / mapData.tileHeight));

    return ret;
}

// L09: TODO 6: Load a group of properties from a node and fill a list with it
bool Map::LoadProperties(pugi::xml_node& node, Properties& properties)
{
    bool ret = false;

    for (pugi::xml_node propertieNode = node.child("properties").child("property"); propertieNode; propertieNode = propertieNode.next_sibling("property"))
    {
        Properties::Property* p = new Properties::Property();
        p->name = propertieNode.attribute("name").as_string();
        p->value = propertieNode.attribute("value").as_bool(); // (!!) I'm assuming that all values are bool !!

        properties.propertyList.push_back(p);
    }

    return ret;
}

// L10: TODO 7: Create a method to get the map size in pixels
Vector2D Map::GetMapSizeInPixels()
{
    Vector2D sizeInPixels;
    sizeInPixels.setX((float)(mapData.width * mapData.tileWidth));
    sizeInPixels.setY((float)(mapData.height * mapData.tileHeight));
    return sizeInPixels;
}

Vector2D Map::GetMapSizeInTiles()
{
    return Vector2D((float)mapData.width, (float)mapData.height);
}

// Method to get the navigation layer from the map
MapLayer* Map::GetNavigationLayer() {
    for (const auto& layer : mapData.layers) {
        if (layer->properties.GetProperty("Navigation") != NULL &&
            layer->properties.GetProperty("Navigation")->value) {
            return layer;
        }
    }

    return nullptr;
}

//L15 TODO 2: Define a method to load entities from the map XML
void Map::LoadEntities(std::shared_ptr<Player>& player) {

    //Iterate the object groups
    for (pugi::xml_node objectGroupNode = mapFileXML.child("map").child("objectgroup"); objectGroupNode != NULL; objectGroupNode = objectGroupNode.next_sibling("objectgroup")) {
        //Check if the object group is "Entities"
        if (objectGroupNode.attribute("name").as_string() == std::string("Entities")) {

            //Iterate the objects
            for (pugi::xml_node objectNode = objectGroupNode.child("object"); objectNode != NULL; objectNode = objectNode.next_sibling("object")) {

                //Get the entity type and position
                std::string entityType = objectNode.attribute("type").as_string();
                float x = objectNode.attribute("x").as_float();
                float y = objectNode.attribute("y").as_float();

                // Create entity based on type
                if (entityType == "Player") {
                    // Create Player entity
                    if (player == nullptr) {
                        player = std::dynamic_pointer_cast<Player>(Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));
                        player->position = Vector2D(x + 32, y + 32);
                        player->Start(); //L17: Importan to call Start to initialize teh Entity
                        LOG("Player spawned at: %f, %f", x, y);
                    }
                    //If the player already exists, just set its position
                    else {
                        player->SetPosition(Vector2D(x, y));
                    }
                }
                else if (entityType == "Enemy") {
                    auto enemy = std::dynamic_pointer_cast<Enemy>(Engine::GetInstance().entityManager->CreateEntity(EntityType::ENEMY));
                    enemy->position = Vector2D(x, y);
                    enemy->Start();
                    LOG("Enemy spawned at: %f, %f", x, y);
                }
            }
        }
    }
}

//L15 TODO 4: Define a method to save entities to the map XML
void Map::SaveEntities(std::shared_ptr<Player> player) {

    //Iterate the object groups
    for (pugi::xml_node objectGroupNode = mapFileXML.child("map").child("objectgroup"); objectGroupNode != NULL; objectGroupNode = objectGroupNode.next_sibling("objectgroup")) {

        //Check if the object group is "Entities"
        if (objectGroupNode.attribute("name").as_string() == std::string("Entities")) {

            //Iterate the objects
            for (pugi::xml_node objectNode = objectGroupNode.child("object"); objectNode != NULL; objectNode = objectNode.next_sibling("object")) {
                std::string entityType = objectNode.attribute("type").as_string();
                // Modify entity based on type
                if (entityType == "Player") {
                    // Modify the Player entity values
                    Vector2D playerPos = player->GetPosition();
                    objectNode.attribute("x").set_value(playerPos.getX());
                    objectNode.attribute("y").set_value(playerPos.getY());
                }
            }
        }
    }

    //Important: save the modifications to the XML 
    std::string mapPathName = mapPath + mapFileName;
    mapFileXML.save_file(mapPathName.c_str());

}

void Map::LoadImageLayers()
{
    for (pugi::xml_node imgNode = mapFileXML.child("map").child("imagelayer");
        imgNode != NULL;
        imgNode = imgNode.next_sibling("imagelayer"))
    {
        ImageLayer* imgLayer = new ImageLayer();
        imgLayer->name = imgNode.attribute("name").as_string();
        imgLayer->offsetX = imgNode.attribute("offsetx").as_float(0.0f);
        imgLayer->offsetY = imgNode.attribute("offsety").as_float(0.0f);
        imgLayer->source = imgNode.child("image").attribute("source").as_string();

        std::string fullPath = mapPath + imgLayer->source;
        imgLayer->texture = Engine::GetInstance().textures->Load(fullPath.c_str());

        mapData.imageLayers.push_back(imgLayer);
        LOG("ImageLayer loaded: %s", imgLayer->name.c_str());
    }
}


void Map::LoadDecorationObjects()
{
    for (pugi::xml_node groupNode = mapFileXML.child("map").child("objectgroup");
        groupNode != NULL;
        groupNode = groupNode.next_sibling("objectgroup"))
    {
        if (groupNode.attribute("name").as_string() != std::string("Decoracion"))
            continue;

        for (pugi::xml_node objNode = groupNode.child("object");
            objNode != NULL;
            objNode = objNode.next_sibling("object"))
        {
            int gid = objNode.attribute("gid").as_int(0);
            if (gid == 0) continue; 

          
            TileSet* ts = GetTilesetFromTileId(gid);
            if (ts == nullptr) continue;

            int relativeId = gid - ts->firstGid; 

           
            if (ts->tileTextures.find(relativeId) == ts->tileTextures.end())
            {
             
                for (pugi::xml_node tsNode = mapFileXML.child("map").child("tileset");
                    tsNode != NULL;
                    tsNode = tsNode.next_sibling("tileset"))
                {
                    if (tsNode.attribute("firstgid").as_int() != ts->firstGid) continue;

                    for (pugi::xml_node tileNode = tsNode.child("tile");
                        tileNode != NULL;
                        tileNode = tileNode.next_sibling("tile"))
                    {
                        if (tileNode.attribute("id").as_int(-1) != relativeId) continue;

                        std::string imgSrc = tileNode.child("image").attribute("source").as_string();
                        if (!imgSrc.empty())
                        {
                            std::string fullPath = mapPath + imgSrc;
                            SDL_Texture* tex = Engine::GetInstance().textures->Load(fullPath.c_str());
                            ts->tileTextures[relativeId] = tex;
                            LOG("DecorationSprite loaded: id=%d path=%s", relativeId, fullPath.c_str());
                        }
                        break;
                    }
                    break;
                }
            }

            DecorationObject* deco = new DecorationObject();
            deco->x = objNode.attribute("x").as_float();
            deco->y = objNode.attribute("y").as_float();
            deco->width = objNode.attribute("width").as_float();
            deco->height = objNode.attribute("height").as_float();
            deco->gid = gid;

            auto it = ts->tileTextures.find(relativeId);
            deco->texture = (it != ts->tileTextures.end()) ? it->second : nullptr;

            mapData.decorationObjects.push_back(deco);
        }

        break; 
    }

    LOG("DecorationObjects loaded: %d sprites", (int)mapData.decorationObjects.size());
}
