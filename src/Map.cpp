#include "Engine.h"
#include "Render.h"
#include "Textures.h"
#include "Map.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Enemy.h"
#include "Checkpoint.h"
#include "Box.h"
#include "Window.h"
#include "tracy/Tracy.hpp"

#include <math.h>
#include <algorithm>
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

        Render* render = Engine::GetInstance().render.get();
        int scale = Engine::GetInstance().window->GetScale();

        for (const auto& imgLayer : mapData.imageLayers) {
            if (imgLayer->texture) {
                Engine::GetInstance().render->DrawTexture(
                    imgLayer->texture,
                    static_cast<int>(imgLayer->offsetX),
                    static_cast<int>(imgLayer->offsetY),
                    nullptr,
                    imgLayer->parallaxFactorX
                );
            }
        }
        for (const auto& deco : mapData.decorationObjects) {
            if (deco->texture && !deco->isFront) {
                // Posició en coordenades de món (Tiled usa l'origen a baix-esquerra per objectes gid)
                float worldX = deco->x;
                float worldY = deco->y - deco->height;

                // Aplicar la càmera i l'escala de finestra (igual que DrawTexture)
                SDL_FRect dst;
                dst.x = (float)((int)(render->camera.x) + (int)worldX * scale);
                dst.y = (float)((int)(render->camera.y) + (int)worldY * scale);
                dst.w = deco->width * scale;
                dst.h = deco->height * scale;

                // En Tiled, l'origen de rotació dels objectes GID és per defecte a baix-esquerra
                SDL_FPoint center;
                center.x = 0.0f;
                center.y = dst.h;

                SDL_RenderTextureRotated(render->renderer, deco->texture, nullptr, &dst,
                    deco->rotation, &center, SDL_FLIP_NONE);
            }
        }
        
        // Calculate camera bounds in tiles
        float camX = -render->camera.x;
        float camY = -render->camera.y;
        float camW = (float)render->camera.w / scale;
        float camH = (float)render->camera.h / scale;

        int startX = std::max(0, static_cast<int>(camX / static_cast<float>(mapData.tileWidth)) - 2);
        int startY = std::max(0, static_cast<int>(camY / static_cast<float>(mapData.tileHeight)) - 2);
        int endX = std::min(mapData.width, static_cast<int>((camX + camW) / static_cast<float>(mapData.tileWidth)) + 2);
        int endY = std::min(mapData.height, static_cast<int>((camY + camH) / static_cast<float>(mapData.tileHeight)) + 2);

        // iterate all tiles in a layer that are visible to the camera
        for (const auto& mapLayer : mapData.layers) {
            if (mapLayer->properties.GetProperty("Draw") != NULL && mapLayer->properties.GetProperty("Draw")->value == true) {
                for (int i = startX; i < endX; i++) {
                    for (int j = startY; j < endY; j++) {

                        //Get the gid from tile
                        int gid = mapLayer->Get(i, j);

                        //Check if the gid is different from 0 - some tiles are empty
                        if (gid != 0) {
                            TileSet* tileSet = GetTilesetFromTileId(gid);
                            if (tileSet != nullptr) {
                                //Get the Rect from the tileSetTexture;
                                SDL_Rect tileRect = tileSet->GetRect(gid);
                                //Get the screen coordinates from the tile coordinates
                                Vector2D mapCoord = MapToWorld(i, j);
                                //Draw the texture
                                render->DrawTexture(tileSet->texture, (int)mapCoord.getX(), (int)mapCoord.getY(), &tileRect);
                            }
                        }
                    }
                }
            }
        }
    }

    return ret;
}

// Called after Update, for foreground elements
bool Map::PostUpdate()
{
    if (mapLoaded == false)
        return true;

    float scale = (float)Engine::GetInstance().window->GetScale();

    // Dibuixar les decoracions frontals per sobre de les entitats
    for (const auto& deco : mapData.decorationObjects) {
        if (deco->texture && deco->isFront) {
            float worldX = deco->x;
            float worldY = deco->y - deco->height;

            SDL_FRect dst;
            dst.x = (float)((int)(Engine::GetInstance().render->camera.x) + (int)worldX * scale);
            dst.y = (float)((int)(Engine::GetInstance().render->camera.y) + (int)worldY * scale);
            dst.w = deco->width * scale;
            dst.h = deco->height * scale;

            SDL_FPoint center;
            center.x = 0.0f;
            center.y = dst.h;

            SDL_RenderTextureRotated(Engine::GetInstance().render->renderer, deco->texture, nullptr, &dst,
                deco->rotation, &center, SDL_FLIP_NONE);
        }
    }

    return true;
}

TileSet* Map::GetTilesetFromTileId(int gid) const
{
    TileSet* bestMatch = nullptr;

    for (const auto& tileset : mapData.tilesets) {
        if (gid >= tileset->firstGid) {
            if (bestMatch == nullptr || tileset->firstGid > bestMatch->firstGid) {
                bestMatch = tileset;
            }
        }
    }

    // Verificar si el gid realment pertany al tileset (dins del seu rang de tileCount)
    if (bestMatch && gid < bestMatch->firstGid + bestMatch->tileCount) {
        return bestMatch;
    }

    return nullptr;
}

// Called before quitting
bool Map::CleanUp()
{
    LOG("Unloading map");

    for (const auto& tileset : mapData.tilesets) {
        if (tileset->texture) {
            Engine::GetInstance().textures->UnLoad(tileset->texture);
        }
        delete tileset;
    }
    mapData.tilesets.clear();

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

    pugi::xml_parse_result result = mapFileXML.load_file(mapPathName.c_str());

    if (result == NULL)
    {
        LOG("Could not load map xml file %s. pugi error: %s", mapPathName.c_str(), result.description());
        ret = false;
    }
    else {

        mapData.width = mapFileXML.child("map").attribute("width").as_int();
        mapData.height = mapFileXML.child("map").attribute("height").as_int();
        mapData.tileWidth = mapFileXML.child("map").attribute("tilewidth").as_int();
        mapData.tileHeight = mapFileXML.child("map").attribute("tileheight").as_int();

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

        for (pugi::xml_node layerNode = mapFileXML.child("map").child("layer"); layerNode != NULL; layerNode = layerNode.next_sibling("layer")) {

            MapLayer* mapLayer = new MapLayer();
            mapLayer->id = layerNode.attribute("id").as_int();
            mapLayer->name = layerNode.attribute("name").as_string();
            mapLayer->width = layerNode.attribute("width").as_int();
            mapLayer->height = layerNode.attribute("height").as_int();

            LoadProperties(layerNode, mapLayer->properties);

            for (pugi::xml_node tileNode = layerNode.child("data").child("tile"); tileNode != NULL; tileNode = tileNode.next_sibling("tile")) {
                mapLayer->tiles.push_back(tileNode.attribute("gid").as_int());
            }

            mapData.layers.push_back(mapLayer);
        }

        for (const auto& mapLayer : mapData.layers) {
            if (mapLayer->name == "Collisions") continue;

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
                            hasRectCollider[j * mapData.width + i] = true;
                        }
                    }
                }
            }

            std::vector<bool> visited(mapData.width * mapData.height, false);

            for (int j = 0; j < mapData.height; j++) {
                for (int i = 0; i < mapData.width; i++) {
                    if (!hasRectCollider[j * mapData.width + i]) continue;
                    if (visited[j * mapData.width + i]) continue;

                    int w = 1;
                    while (i + w < mapData.width &&
                        hasRectCollider[j * mapData.width + (i + w)] &&
                        !visited[j * mapData.width + (i + w)]) {
                        w++;
                    }

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

                    for (int jj = 0; jj < h; jj++) {
                        for (int ii = 0; ii < w; ii++) {
                            visited[(j + jj) * mapData.width + (i + ii)] = true;
                        }
                    }

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

        if (ret == true)
        {
            LOG("Successfully parsed map XML file :%s", fileName.c_str());
        }
        else {
            LOG("Error while parsing map file: %s", mapPathName.c_str());
        }

    }

    mapLoaded = ret;
    return ret;
}

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

MapLayer* Map::GetNavigationLayer() {
    for (const auto& layer : mapData.layers) {
        if (layer->properties.GetProperty("Navigation") != NULL &&
            layer->properties.GetProperty("Navigation")->value) {
            return layer;
        }
    }

    return nullptr;
}

void Map::LoadEntities(std::shared_ptr<Player>& player) {

    for (pugi::xml_node objectGroupNode = mapFileXML.child("map").child("objectgroup"); objectGroupNode != NULL; objectGroupNode = objectGroupNode.next_sibling("objectgroup")) {
        if (objectGroupNode.attribute("name").as_string() == std::string("Entities")) {

            for (pugi::xml_node objectNode = objectGroupNode.child("object"); objectNode != NULL; objectNode = objectNode.next_sibling("object")) {

                std::string entityType = objectNode.attribute("type").as_string();
                float x = objectNode.attribute("x").as_float();
                float y = objectNode.attribute("y").as_float();

                if (entityType == "Player") {
                    if (player == nullptr) {
                        player = std::dynamic_pointer_cast<Player>(Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));
                        player->position = Vector2D(x + 32, y + 32);
                        player->Start();
                        LOG("Player spawned at: %f, %f", x, y);
                    }
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
                else if (entityType == "Checkpoint") {
                    auto checkpoint = std::dynamic_pointer_cast<Checkpoint>(Engine::GetInstance().entityManager->CreateEntity(EntityType::CHECKPOINT));
                    checkpoint->position = Vector2D(x, y);
                    checkpoint->Start();
                    LOG("Checkpoint spawned at: %f, %f", x, y);
                }
                else if (entityType == "Box") {
                    auto box = std::dynamic_pointer_cast<Box>(Engine::GetInstance().entityManager->CreateEntity(EntityType::BOX));
                    box->position = Vector2D(x, y);
                    box->Start();
                    LOG("Box spawned at: %f, %f", x, y);
                }
            }
        }
    }
}

void Map::SaveEntities(std::shared_ptr<Player> player) {

    for (pugi::xml_node objectGroupNode = mapFileXML.child("map").child("objectgroup"); objectGroupNode != NULL; objectGroupNode = objectGroupNode.next_sibling("objectgroup")) {

        if (objectGroupNode.attribute("name").as_string() == std::string("Entities")) {

            for (pugi::xml_node objectNode = objectGroupNode.child("object"); objectNode != NULL; objectNode = objectNode.next_sibling("object")) {
                std::string entityType = objectNode.attribute("type").as_string();
                if (entityType == "Player") {
                    Vector2D playerPos = player->GetPosition();
                    objectNode.attribute("x").set_value(playerPos.getX());
                    objectNode.attribute("y").set_value(playerPos.getY());
                }
            }
        }
    }

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
        imgLayer->parallaxFactorX = imgNode.attribute("parallaxx").as_float(1.0f);
        imgLayer->parallaxFactorY = imgNode.attribute("parallaxy").as_float(1.0f);
        imgLayer->source = imgNode.child("image").attribute("source").as_string();

        std::string fullPath = mapPath + imgLayer->source;
        imgLayer->texture = Engine::GetInstance().textures->Load(fullPath.c_str());

        mapData.imageLayers.push_back(imgLayer);
    }
}


void Map::LoadDecorationObjects()
{
    const std::vector<std::string> excludedNames = { "Entities", "Collisions", "Navigation", "Checkpoints" };

    for (pugi::xml_node groupNode = mapFileXML.child("map").child("objectgroup");
        groupNode != NULL;
        groupNode = groupNode.next_sibling("objectgroup"))
    {
        std::string groupName = groupNode.attribute("name").as_string();

        bool skip = false;
        for (const auto& excluded : excludedNames) {
            if (groupName == excluded) { skip = true; break; }
        }
        if (skip) continue;

        std::vector<DecorationObject*> layerDecos;

        for (pugi::xml_node objNode = groupNode.child("object");
            objNode != NULL;
            objNode = objNode.next_sibling("object"))
        {
            unsigned int rawGid = objNode.attribute("gid").as_uint(0);
            if (rawGid == 0) continue; 

            const unsigned int FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
            const unsigned int FLIPPED_VERTICALLY_FLAG   = 0x40000000;
            const unsigned int FLIPPED_DIAGONALLY_FLAG   = 0x20000000;

            int gid = rawGid & ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

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
            deco->rotation = objNode.attribute("rotation").as_double(0.0);
            deco->isFront = (groupName == "Assets front");
            deco->gid = gid;

            auto it = ts->tileTextures.find(relativeId);
            deco->texture = (it != ts->tileTextures.end()) ? it->second : nullptr;

            layerDecos.push_back(deco);
        }

        std::sort(layerDecos.begin(), layerDecos.end(),
            [](const DecorationObject* a, const DecorationObject* b) {
                return a->y < b->y;
            });

        for (auto* deco : layerDecos) {
            mapData.decorationObjects.push_back(deco);
        }
    }
}
