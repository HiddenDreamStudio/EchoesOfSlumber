#include "Map.h"
#include "Antagonist.h"
#include "BlockCrawler.h"
#include "Boss1.h"
#include "Boss2.h"
#include "Bouncer.h"
#include "Box.h"
#include "Checkpoint.h"
#include "Door.h"
#include "DropDoll.h"
#include "EnemyB.h"
#include "EnemyC.h"
#include "EnemyCarmel.h"
#include "EnemyPlush.h"
#include "EnemyStitchling.h"
#include "EnemyWindUpScurry.h"
#include "Engine.h"
#include "EntityManager.h"
#include "HidingRock.h"
#include "Lever.h"
#include "Log.h"
#include "MemoryFragment.h"
#include "Physics.h"
#include "Platform.h"
#include "Player.h"
#include "PushRock.h"
#include "Render.h"
#include "RopedRock.h"
#include "Scene.h"
#include "Textures.h"
#include "Window.h"
#include "tracy/Tracy.hpp"
#include <algorithm>
#include <math.h>
#include <sstream>

// Helper for SDL flip flags
static SDL_FlipMode flipMode(bool h, bool v) {
  if (h && v)
    return (SDL_FlipMode)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
  if (h)
    return SDL_FLIP_HORIZONTAL;
  if (v)
    return SDL_FLIP_VERTICAL;
  return SDL_FLIP_NONE;
}

Map::Map() : Module(), mapLoaded(false), hasInitCamera(false), initCameraX(0.0f), initCameraY(0.0f)
{
    name = "map";
}

// Destructor
Map::~Map() {}

// Called before render is available
bool Map::Awake() {
  name = "map";
  LOG("Loading Map Parser");

  return true;
}

bool Map::Start() { return true; }

bool Map::Update(float dt)
{
    ZoneScoped;

    bool ret = true;

    if (mapLoaded) {

      if (Engine::GetInstance().scene->GetCurrentScene() != SceneID::GAMEPLAY) {
            return true;
        }

        Render* render = Engine::GetInstance().render.get();

        if (!hasInitCamera && render->IsCameraInitialized()) {
            initCameraX = (float)render->camera.x;
            initCameraY = (float)render->camera.y;
            hasInitCamera = true;
            LOG("PARALLAX DEBUG: Pinned to camera position X: %f, Y: %f", initCameraX, initCameraY);
        }

        if (!hasInitCamera) return true; // Don't draw parallax until camera is pinned

        Vector2D playerPos(0.0f, 0.0f);
        bool hasPlayer = (Engine::GetInstance().scene->player != nullptr);
        if (hasPlayer) playerPos = Engine::GetInstance().scene->player->position;
        const float renderRadiusSq = 1500.0f * 1500.0f; // Squared distance for performance

        for (const auto& imgLayer : mapData.imageLayers) {
            if (imgLayer->texture) {
                // Image layers are typically global backgrounds, always draw them
                float pinnedX = imgLayer->offsetX + initCameraX * (1.0f - imgLayer->parallaxFactorX);
                float pinnedY = imgLayer->offsetY + initCameraY * (1.0f - imgLayer->parallaxFactorY);
                
                Engine::GetInstance().render->DrawTexture(
                    imgLayer->texture,
                    static_cast<int>(pinnedX),
                    static_cast<int>(pinnedY),
                    nullptr,
                    imgLayer->parallaxFactorX,
                    imgLayer->parallaxFactorY
                );
            }
        }
        
        if (Engine::GetInstance().scene->player) {
            Engine::GetInstance().scene->player->DrawBehindMap(dt);
        }

        for (const auto& deco : mapData.decorationObjects) {
            if (deco->texture && !deco->isFront) {
                float px = deco->parallaxSpeed;

                // Optimization: Distance check from player
                // Always draw fondo (px=0) or objects within radius
                if (hasPlayer && px > 0.0f) {
                    float dx = deco->x - playerPos.getX();
                    float dy = deco->y - playerPos.getY();
                    if ((dx*dx + dy*dy) > renderRadiusSq) continue;
                }

                float py = 1.0f; // All backgrounds scroll 1:1 vertically with the camera
                if (px > 1.0f) py = 1.0f + (px - 1.0f) * 0.5f; // Keep subtle FG vertical parallax

                // Pin parallax to start position
                float pinnedX = deco->x + initCameraX * (1.0f - px);
                float pinnedY = deco->y + initCameraY * (1.0f - py);

                int tw = 0, th = 0;
                Engine::GetInstance().textures->GetSize(deco->texture, tw, th);
                float drawScaleX = (tw > 0) ? (deco->width / (float)tw) : 1.0f;
                float drawScaleY = (th > 0) ? (deco->height / (float)th) : 1.0f;

                render->DrawTexture(deco->texture, (int)pinnedX, (int)(pinnedY - deco->height), nullptr, px, py, deco->rotation, 0, (int)deco->height, flipMode(deco->flipH, deco->flipV), drawScaleX, drawScaleY);
            }
        }

        for (const auto& plant : mapData.animatedPlants) {
            if (plant->isFront) continue;
            plant->anim.Update(dt);

            float px = plant->parallaxSpeed;

            // Optimization: Distance check from player
            if (hasPlayer && px > 0.0f) {
                float dx = plant->x - playerPos.getX();
                float dy = plant->y - playerPos.getY();
                if ((dx*dx + dy*dy) > renderRadiusSq) continue;
            }

            float py = 1.0f; // Background plants scroll 1:1 vertically with the camera
            if (px > 1.0f) py = 1.0f + (px - 1.0f) * 0.5f; // Keep subtle FG vertical parallax

            float pinnedX = plant->x + initCameraX * (1.0f - px);
            float pinnedY = plant->y + initCameraY * (1.0f - py);

            if (!render->IsOnScreenWorldRect(pinnedX, pinnedY, plant->w, plant->h, 1000))
                continue;

            const SDL_Rect& frame = plant->anim.GetCurrentFrame();
            render->DrawTexture(plant->texture, (int)pinnedX, (int)pinnedY, &frame, px, py);
        }
        
        // Draw Checkpoints BEFORE map layers so they appear behind the floor
        if (Engine::GetInstance().entityManager) {
            for (const auto& entity : Engine::GetInstance().entityManager->entities) {
                if (entity && entity->type == EntityType::CHECKPOINT) {
                    auto cp = std::dynamic_pointer_cast<Checkpoint>(entity);
                    if (cp) {
                        cp->DrawBehindMap();
                    }
                }
            }
        }

        for (const auto& mapLayer : mapData.layers) {
            auto* drawProp = mapLayer->properties.GetProperty("Draw");
            if (drawProp == nullptr || drawProp->value == true) {
                float px = mapLayer->parallaxFactorX;
                float py = mapLayer->parallaxFactorY;

                std::string lowerName = mapLayer->name;
                for (char& c : lowerName) c = ::tolower(c);
                if (px == 1.0f) {
                    if (lowerName.find("fondo") != std::string::npos) px = 0.0f;
                    else if (lowerName.find("background") != std::string::npos || lowerName.find("back") != std::string::npos) px = 1.0f;
                    else if (lowerName.find("middle") != std::string::npos || lowerName.find("medio") != std::string::npos) px = 1.0f;
                    else if (lowerName.find("foreground") != std::string::npos || lowerName.find("front") != std::string::npos) px = 1.2f;
                }
                
                if (py == 1.0f) {
                    if (px > 1.0f) py = 1.0f + (px - 1.0f) * 0.5f; // Subtle Y parallax for foregrounds
                    // Otherwise py stays 1.0f (scrolls with camera)
                }

                // Optimization: Pre-calculate tile bounds based on player radius
                int minI = 0, maxI = mapData.width;
                int minJ = 0, maxJ = mapData.height;

                if (hasPlayer && px > 0.0f) {
                    minI = std::max(0, (int)((playerPos.getX() - 1500.0f) / mapData.tileWidth));
                    maxI = std::min(mapData.width, (int)((playerPos.getX() + 1500.0f) / mapData.tileWidth) + 1);
                    minJ = std::max(0, (int)((playerPos.getY() - 1500.0f) / mapData.tileHeight));
                    maxJ = std::min(mapData.height, (int)((playerPos.getY() + 1500.0f) / mapData.tileHeight) + 1);
                }

                for (int i = minI; i < maxI; i++) {
                    for (int j = minJ; j < maxJ; j++) {
                        unsigned int rawGid = mapLayer->Get(i, j);
                        if (rawGid != 0) {
                            const unsigned int FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
                            const unsigned int FLIPPED_VERTICALLY_FLAG = 0x40000000;
                            const unsigned int FLIPPED_DIAGONALLY_FLAG = 0x20000000;

                            int gid = rawGid & ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

                            TileSet* tileSet = GetTilesetFromTileId(gid);
                            if (tileSet != nullptr && tileSet->texture != nullptr) {
                                SDL_Rect tileRect = tileSet->GetRect(gid);
                                Vector2D mapCoord = MapToWorld(i, j);
                                
                                // Pin parallax to start position
                                float pinnedX = mapCoord.getX() + initCameraX * (1.0f - px);
                                float pinnedY = mapCoord.getY() + initCameraY * (1.0f - py);

                                SDL_FlipMode flip = SDL_FLIP_NONE;
                                if (rawGid & FLIPPED_HORIZONTALLY_FLAG) flip = (SDL_FlipMode)(flip | SDL_FLIP_HORIZONTAL);
                                if (rawGid & FLIPPED_VERTICALLY_FLAG) flip = (SDL_FlipMode)(flip | SDL_FLIP_VERTICAL);

                                render->DrawTexture(tileSet->texture, (int)pinnedX, (int)pinnedY, &tileRect, px, py, 0.0, INT_MAX, INT_MAX, flip);
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
bool Map::PostUpdate() {
  if (mapLoaded == false)
    return true;

  if (Engine::GetInstance().scene->GetCurrentScene() != SceneID::GAMEPLAY) {
    return true;
  }

  Render *render = Engine::GetInstance().render.get();

  // Find player position for player-centric parallax
  Vector2D playerPos(0.0f, 0.0f);
  bool hasPlayer = false;
  if (Engine::GetInstance().scene->player) {
    playerPos = Engine::GetInstance().scene->player->position;
    hasPlayer = true;
  }
  const float renderRadiusSq = 1500.0f * 1500.0f;

  for (const auto &deco : mapData.decorationObjects) {
    if (deco->texture && deco->isFront) {
      float px = deco->parallaxSpeed;

      // Optimization: Distance check from player
      if (hasPlayer && px > 0.0f) {
        float dx = deco->x - playerPos.getX();
        float dy = deco->y - playerPos.getY();
        if ((dx * dx + dy * dy) > renderRadiusSq)
          continue;
      }

      float py = 1.0f;
      if (px != 1.0f) py = 1.0f + (px - 1.0f) * 0.5f;

      // Pin parallax to start position
      float pinnedX = deco->x + initCameraX * (1.0f - px);
      float pinnedY = deco->y + initCameraY * (1.0f - py);

      if (!render->IsOnScreenWorldRect(pinnedX, pinnedY - deco->height,
                                       deco->width, deco->height))
        continue;

      int tw = 0, th = 0;
      Engine::GetInstance().textures->GetSize(deco->texture, tw, th);
      float drawScaleX = (tw > 0) ? (deco->width / (float)tw) : 1.0f;
      float drawScaleY = (th > 0) ? (deco->height / (float)th) : 1.0f;

      render->DrawTexture(deco->texture, (int)pinnedX, (int)(pinnedY - deco->height),
                          nullptr, px, py, deco->rotation, 0, (int)deco->height,
                          flipMode(deco->flipH, deco->flipV), drawScaleX,
                          drawScaleY);
    }
  }

  for (const auto &plant : mapData.animatedPlants) {
    if (!plant->isFront)
      continue;

    float px = plant->parallaxSpeed;

    // Optimization: Distance check from player
    if (hasPlayer && px > 0.0f) {
      float dx = plant->x - playerPos.getX();
      float dy = plant->y - playerPos.getY();
      if ((dx * dx + dy * dy) > renderRadiusSq)
        continue;
    }

    float py = 1.0f;
    if (px != 1.0f) py = 1.0f + (px - 1.0f) * 0.5f;

    float pinnedX = plant->x + initCameraX * (1.0f - px);
    float pinnedY = plant->y + initCameraY * (1.0f - py);

    if (!render->IsOnScreenWorldRect(pinnedX, pinnedY, plant->w, plant->h))
      continue;

    const SDL_Rect &frame = plant->anim.GetCurrentFrame();
    render->DrawTexture(plant->texture, (int)pinnedX, (int)plant->y, &frame, px, py);
  }

  return true;
}

TileSet *Map::GetTilesetFromTileId(int gid) const {
  TileSet *bestMatch = nullptr;

  for (const auto &tileset : mapData.tilesets) {
    if (gid >= tileset->firstGid) {
      if (bestMatch == nullptr || tileset->firstGid > bestMatch->firstGid) {
        bestMatch = tileset;
      }
    }
  }

  // Verificar si el gid realment pertany al tileset (dins del seu rang de
  // tileCount)
  if (bestMatch && (bestMatch->tileCount == 0 ||
                    gid < bestMatch->firstGid + bestMatch->tileCount)) {
    return bestMatch;
  }

  return nullptr;
}

// Called before quitting
bool Map::CleanUp() {
  LOG("Unloading map");

  for (const auto &tileset : mapData.tilesets) {
    if (tileset->texture) {
      Engine::GetInstance().textures->UnLoad(tileset->texture);
    }
    delete tileset;
  }
  mapData.tilesets.clear();

  for (const auto &layer : mapData.layers) {
    delete layer;
  }
  mapData.layers.clear();

  for (const auto &collider : colliderList) {
    Engine::GetInstance().physics->DeletePhysBody(collider);
  }
  colliderList.clear();

  for (const auto &imgLayer : mapData.imageLayers) {
    if (imgLayer->texture)
      Engine::GetInstance().textures->UnLoad(imgLayer->texture);
    delete imgLayer;
  }
  mapData.imageLayers.clear();

  for (const auto &deco : mapData.decorationObjects) {
    if (deco->texture)
      Engine::GetInstance().textures->UnLoad(deco->texture);
    delete deco;
  }
  mapData.decorationObjects.clear();

  for (const auto &plant : mapData.animatedPlants) {
    if (plant->texture)
      Engine::GetInstance().textures->UnLoad(plant->texture);
    delete plant;
  }
  mapData.animatedPlants.clear();

  for (const auto &cp : mapData.checkpoints) {
    delete cp;
  }
  mapData.checkpoints.clear();

    mapData.capeFound = false;
    mapLoaded = false;
    hasInitCamera = false;
    initCameraX = 0.0f;
    initCameraY = 0.0f;

  return true;
}

// Load new map
bool Map::Load(std::string path, std::string fileName) {
  bool ret = false;

  mapFileName = fileName;
  mapPath = path;
  std::string mapPathName = mapPath + mapFileName;

  pugi::xml_parse_result result = mapFileXML.load_file(mapPathName.c_str());

  if (result == NULL) {
    LOG("Could not load map xml file %s. pugi error: %s", mapPathName.c_str(),
        result.description());
    ret = false;
  } else {

    mapData.width = mapFileXML.child("map").attribute("width").as_int();
    mapData.height = mapFileXML.child("map").attribute("height").as_int();
    mapData.tileWidth = mapFileXML.child("map").attribute("tilewidth").as_int();
    mapData.tileHeight =
        mapFileXML.child("map").attribute("tileheight").as_int();

    for (pugi::xml_node tilesetNode = mapFileXML.child("map").child("tileset");
         tilesetNode != NULL;
         tilesetNode = tilesetNode.next_sibling("tileset")) {
      TileSet *tileSet = new TileSet();
      tileSet->firstGid = tilesetNode.attribute("firstgid").as_int();
      tileSet->name = tilesetNode.attribute("name").as_string();
      tileSet->tileWidth = tilesetNode.attribute("tilewidth").as_int();
      tileSet->tileHeight = tilesetNode.attribute("tileheight").as_int();
      tileSet->spacing = tilesetNode.attribute("spacing").as_int();
      tileSet->margin = tilesetNode.attribute("margin").as_int();
      tileSet->tileCount = tilesetNode.attribute("tilecount").as_int();
      tileSet->columns = tilesetNode.attribute("columns").as_int();

      std::string tsxSrc = tilesetNode.attribute("source").as_string();
      if (!tsxSrc.empty() && tileSet->tileCount == 0) {
        pugi::xml_document tsxDoc;
        if (tsxDoc.load_file((mapPath + tsxSrc).c_str())) {
          tileSet->tileCount =
              tsxDoc.child("tileset").attribute("tilecount").as_int();
          tileSet->columns =
              tsxDoc.child("tileset").attribute("columns").as_int();
        }
      }

      // Load the tileset image (skip if tileset uses per-tile images)
      std::string imgName =
          tilesetNode.child("image").attribute("source").as_string();
      if (!imgName.empty()) {
        tileSet->texture =
            Engine::GetInstance().textures->Load((mapPath + imgName).c_str());
      } else {
        tileSet->texture = nullptr;
      }

      for (pugi::xml_node tileNode = tilesetNode.child("tile");
           tileNode != NULL; tileNode = tileNode.next_sibling("tile")) {
        int tileId = tileNode.attribute("id").as_int();
        pugi::xml_node objectGroupNode = tileNode.child("objectgroup");
        if (objectGroupNode != NULL) {
          std::vector<ObjectCollision> collisions;
          for (pugi::xml_node objectNode = objectGroupNode.child("object");
               objectNode != NULL;
               objectNode = objectNode.next_sibling("object")) {
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
              while (std::getline(ss, pointPair, ' ')) {
                if (pointPair.empty())
                  continue;
                std::stringstream ssPair(pointPair);
                std::string xStr, yStr;
                if (std::getline(ssPair, xStr, ',') &&
                    std::getline(ssPair, yStr, ',')) {
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

    for (pugi::xml_node layerNode = mapFileXML.child("map").child("layer");
         layerNode != NULL; layerNode = layerNode.next_sibling("layer")) {

      MapLayer *mapLayer = new MapLayer();
      mapLayer->id = layerNode.attribute("id").as_int();
      mapLayer->name = layerNode.attribute("name").as_string();
      mapLayer->width = layerNode.attribute("width").as_int();
      mapLayer->height = layerNode.attribute("height").as_int();
      mapLayer->parallaxFactorX =
          layerNode.attribute("parallaxx").as_float(1.0f);
      mapLayer->parallaxFactorY =
          layerNode.attribute("parallaxy").as_float(1.0f);

      LoadProperties(layerNode, mapLayer->properties);

      std::string encoding =
          layerNode.child("data").attribute("encoding").as_string();

      if (encoding == "csv") {
        std::string csvStr = layerNode.child("data").child_value();
        std::stringstream ss(csvStr);
        std::string token;
        while (std::getline(ss, token, ',')) {
          token.erase(0, token.find_first_not_of(" \n\r\t"));
          token.erase(token.find_last_not_of(" \n\r\t") + 1);
          if (!token.empty()) {
            // Use stoul instead of stoi to handle large GIDs with flip flags
            mapLayer->tiles.push_back((int)std::stoul(token));
          }
        }
      } else {
        for (pugi::xml_node tileNode = layerNode.child("data").child("tile");
             tileNode != NULL; tileNode = tileNode.next_sibling("tile")) {
          mapLayer->tiles.push_back(tileNode.attribute("gid").as_int());
        }
      }

      mapData.layers.push_back(mapLayer);
    }

    for (const auto &mapLayer : mapData.layers) {
      if (mapLayer->name == "Collisions")
        continue;

      std::vector<bool> hasRectCollider(mapData.width * mapData.height, false);

      for (int j = 0; j < mapData.height; j++) {
        for (int i = 0; i < mapData.width; i++) {
          int gid = mapLayer->Get(i, j);
          if (gid == 0)
            continue;

          TileSet *tileSet = GetTilesetFromTileId(gid);
          if (tileSet == nullptr || tileSet->tileCollisions.empty())
            continue;

          int relativeId = gid - tileSet->firstGid;
          auto it = tileSet->tileCollisions.find(relativeId);
          if (it == tileSet->tileCollisions.end())
            continue;

          for (const auto &col : it->second) {
            if (col.polygonPoints.size() > 0) {
              Vector2D mapCoord = MapToWorld(i, j);
              int numVerts = (int)col.polygonPoints.size() / 2;
              PhysBody *c1 = nullptr;
              if (numVerts >= 4) {
                c1 = Engine::GetInstance().physics.get()->CreateChain(
                    (int)(mapCoord.getX() + col.x),
                    (int)(mapCoord.getY() + col.y),
                    (int *)col.polygonPoints.data(),
                    (int)col.polygonPoints.size(), STATIC);
              } else if (numVerts >= 3) {
                c1 = Engine::GetInstance().physics.get()->CreateConvexPolygon(
                    (int)(mapCoord.getX() + col.x),
                    (int)(mapCoord.getY() + col.y),
                    (int *)col.polygonPoints.data(),
                    (int)col.polygonPoints.size(), STATIC);
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
          if (!hasRectCollider[j * mapData.width + i])
            continue;
          if (visited[j * mapData.width + i])
            continue;

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
            if (canExtend)
              h++;
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

          PhysBody *c1 = Engine::GetInstance().physics.get()->CreateRectangle(
              (int)(px + totalW / 2.0f), (int)(py + totalH / 2.0f), (int)totalW,
              (int)totalH, STATIC);
          c1->ctype = ColliderType::PLATFORM;
          colliderList.push_back(c1);
        }
      }
    }

    LoadImageLayers();
    LoadDecorationObjects();
    LoadAnimatedPlants();

    ret = true;

    if (ret == true) {
      LOG("Successfully parsed map XML file :%s", fileName.c_str());
    } else {
      LOG("Error while parsing map file: %s", mapPathName.c_str());
    }
  }

  mapLoaded = ret;
  return ret;
}

Vector2D Map::MapToWorld(int x, int y) const {
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

bool Map::LoadProperties(pugi::xml_node &node, Properties &properties) {
  bool ret = false;

  for (pugi::xml_node propertieNode =
           node.child("properties").child("property");
       propertieNode; propertieNode = propertieNode.next_sibling("property")) {
    Properties::Property *p = new Properties::Property();
    p->name = propertieNode.attribute("name").as_string();
    p->value = propertieNode.attribute("value").as_bool();

    properties.propertyList.push_back(p);
  }

  return ret;
}

Vector2D Map::GetMapSizeInPixels() {
  Vector2D sizeInPixels;
  sizeInPixels.setX((float)(mapData.width * mapData.tileWidth));
  sizeInPixels.setY((float)(mapData.height * mapData.tileHeight));
  return sizeInPixels;
}

Vector2D Map::GetMapSizeInTiles() {
  return Vector2D((float)mapData.width, (float)mapData.height);
}

bool Map::GetCapePosition(float &outX, float &outY) const {
  if (mapData.capeFound) {
    outX = mapData.capeX;
    outY = mapData.capeY;
    return true;
  }
  return false;
}

bool Map::GetSlingshotPosition(float &outX, float &outY) const {
  if (mapData.slingshotFound) {
    outX = mapData.slingshotX;
    outY = mapData.slingshotY;
    return true;
  }
  return false;
}

bool Map::GetStuffedAnimalPosition(float &outX, float &outY) const {
  if (mapData.stuffedAnimalFound) {
    outX = mapData.stuffedAnimalX;
    outY = mapData.stuffedAnimalY;
    return true;
  }
  return false;
}

MapLayer *Map::GetNavigationLayer() {
  for (const auto &layer : mapData.layers) {
    if (layer->properties.GetProperty("Navigation") != NULL &&
        layer->properties.GetProperty("Navigation")->value) {
      return layer;
    }
  }

  return nullptr;
}

void Map::LoadEntities(std::shared_ptr<Player> &player, bool portalTransition,
                       float spawnX, float spawnY) {

  for (pugi::xml_node objectGroupNode =
           mapFileXML.child("map").child("objectgroup");
       objectGroupNode != NULL;
       objectGroupNode = objectGroupNode.next_sibling("objectgroup")) {
    std::string groupName = objectGroupNode.attribute("name").as_string();
    if (groupName == "MovingPlatform" || groupName == "Entities") {

      for (pugi::xml_node objectNode = objectGroupNode.child("object");
           objectNode != NULL; objectNode = objectNode.next_sibling("object")) {

        std::string entityType = objectNode.attribute("type").as_string();
        if (entityType.empty())
          entityType = objectNode.attribute("class").as_string();
        if (entityType.empty()) {
          entityType = objectNode.attribute("class").as_string();
        }

        float rawX = objectNode.attribute("x").as_float();
        float rawY = objectNode.attribute("y").as_float();
        float width = objectNode.attribute("width").as_float(0.0f);
        float height = objectNode.attribute("height").as_float(0.0f);
        int gid = objectNode.attribute("gid").as_int(0);

        // Fix Centrado Coordenadas Tiled -> Box2D
        float x, y;
        if (gid != 0) {
          // Tiled: el origen está abajo a la izquierda para tiles (GID != 0)
          x = rawX + (width / 2.0f);
          y = rawY - (height / 2.0f);
        } else {
          // Tiled: el origen está arriba a la izquierda para formas (GID == 0)
          x = rawX + (width / 2.0f);
          y = rawY + (height / 2.0f);
        }

        if (entityType == "Player") {
          if (player == nullptr) {
            player = std::dynamic_pointer_cast<Player>(
                Engine::GetInstance().entityManager->CreateEntity(
                    EntityType::PLAYER));
            player->position = portalTransition ? Vector2D(spawnX, spawnY)
                                                : Vector2D(x + 32, y + 32);
            player->Start();
            LOG("Player spawned at: %f, %f", x, y);
          } else {
            if (portalTransition)
              player->SetPosition(Vector2D(spawnX, spawnY));
            else
              player->SetPosition(Vector2D(x, y));
          }
          // IMMEDIATELY CALCULATE AND PIN CAMERA POSITION FOR PARALLAX
          Render* render = Engine::GetInstance().render.get();
          float vW = render->GetWorldViewportWidth();
          float vH = render->GetWorldViewportHeight();
          // Use exact math from Render::FollowTarget to avoid any pixel shift
          initCameraX = (float)static_cast<int>(-(player->position.getX() - vW / 2.0f));
          initCameraY = (float)static_cast<int>(-(player->position.getY() - vH * 0.75f));
          hasInitCamera = true;
          LOG("PARALLAX PINNED in LoadEntities: %f, %f", initCameraX, initCameraY);
        } else if (entityType == "Enemy" || entityType == "SpiderCandy") {
          auto enemy = std::dynamic_pointer_cast<EnemyCarmel>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::ENEMY));
          enemy->position = Vector2D(x, y);

          float patrolLeft = x - 200.0f;
          float patrolRight = x + 200.0f;
          pugi::xml_node props = objectNode.child("properties");
          if (props) {
            for (pugi::xml_node prop = props.child("property"); prop;
                 prop = prop.next_sibling("property")) {
              std::string propName = prop.attribute("name").as_string();
              if (propName == "patrol_left")
                patrolLeft = prop.attribute("value").as_float();
              if (propName == "patrol_right")
                patrolRight = prop.attribute("value").as_float();
            }
          }
          enemy->SetPatrolPoints(patrolLeft, patrolRight);
          enemy->Start();
          LOG("Enemy (SpiderCandy) spawned at: %f, %f (patrol: %.0f-%.0f)", x,
              y, patrolLeft, patrolRight);
        } else if (entityType == "BlockCrawler") {
          auto blockCrawler = std::dynamic_pointer_cast<BlockCrawler>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::BLOCK_CRAWLER));
          blockCrawler->position = Vector2D(x, y);

          float patrolLeft = x - 200.0f;
          float patrolRight = x + 200.0f;
          pugi::xml_node props = objectNode.child("properties");
          if (props) {
            for (pugi::xml_node prop = props.child("property"); prop;
                 prop = prop.next_sibling("property")) {
              std::string propName = prop.attribute("name").as_string();
              if (propName == "patrol_left")
                patrolLeft = prop.attribute("value").as_float();
              if (propName == "patrol_right")
                patrolRight = prop.attribute("value").as_float();
            }
          }
          blockCrawler->SetPatrolPoints(patrolLeft, patrolRight);
          blockCrawler->Start();
          LOG("BlockCrawler spawned at: %f, %f (patrol: %.0f-%.0f)", x, y,
              patrolLeft, patrolRight);
        } else if (entityType == "EnemyB") {
          auto enemyB = std::dynamic_pointer_cast<EnemyB>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::ENEMY_B));
          enemyB->position = Vector2D(x, y);
          float patrolLeft = x - 200.0f;
          float patrolRight = x + 200.0f;
          pugi::xml_node props = objectNode.child("properties");
          if (props) {
            for (pugi::xml_node prop = props.child("property"); prop;
                 prop = prop.next_sibling("property")) {
              std::string propName = prop.attribute("name").as_string();
              if (propName == "patrol_left")
                patrolLeft = prop.attribute("value").as_float();
              if (propName == "patrol_right")
                patrolRight = prop.attribute("value").as_float();
            }
          }
          enemyB->SetPatrolPoints(patrolLeft, patrolRight);
          enemyB->Start();
          LOG("EnemyB spawned at: %f, %f (patrol: %.0f-%.0f)", x, y, patrolLeft,
              patrolRight);
        } else if (entityType == "EnemyC") {
          auto enemyC = std::dynamic_pointer_cast<EnemyC>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::ENEMY_C));
          enemyC->position = Vector2D(x, y);
          enemyC->Start();
          LOG("EnemyC spawned at: %f, %f", x, y);
        } else if (entityType == "EnemyPlush") {
          auto enemyPlush = std::dynamic_pointer_cast<EnemyPlush>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::ENEMY_PLUSH));
          enemyPlush->position = Vector2D(x, y);
          enemyPlush->Start();
          LOG("EnemyPlush spawned at: %f, %f", x, y);
        } else if (entityType == "EnemyStitchling") {
          auto stitchling = std::dynamic_pointer_cast<EnemyStitchling>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::ENEMY_STITCHLING));
          stitchling->position = Vector2D(x, y);
          stitchling->Start();
          LOG("EnemyStitchling spawned at: %f, %f", x, y);
        } else if (entityType == "EnemyWindUpScurry" ||
                   entityType == "WindUpScurry") {
          auto scurry = std::dynamic_pointer_cast<EnemyWindUpScurry>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::ENEMY_WINDUP_SCURRY));
          scurry->position = Vector2D(x, y);
          scurry->Start();
          LOG("EnemyWindUpScurry spawned at: %f, %f", x, y);
        } else if (entityType == "Bouncer") {
          float w = objectNode.attribute("width").as_float(96.0f);
          float h = objectNode.attribute("height").as_float(96.0f);
          auto bouncer = std::dynamic_pointer_cast<Bouncer>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::BOUNCER));
          bouncer->position = Vector2D(x, y);
          bouncer->SetSpawnSize(w, h);
          bouncer->Start();
          LOG("Bouncer spawned at: %f, %f (size: %.0fx%.0f)", x, y, w, h);
        } else if (entityType == "Boss1") {
          auto boss = std::dynamic_pointer_cast<Boss1>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::BOSS_1));
          boss->position = Vector2D(x, y);

          float arenaMin = x - 400.0f;
          float arenaMax = x + 400.0f;
          pugi::xml_node props = objectNode.child("properties");
          if (props) {
            for (auto prop = props.child("property"); prop;
                 prop = prop.next_sibling("property")) {
              std::string pname = prop.attribute("name").as_string();
              if (pname == "arena_min_x")
                arenaMin = prop.attribute("value").as_float();
              if (pname == "arena_max_x")
                arenaMax = prop.attribute("value").as_float();
            }
          }
          boss->SetArenaLimits(arenaMin, arenaMax);
          boss->Start();
          LOG("Boss1 spawned at: %f, %f (arena: %.0f-%.0f)", x, y, arenaMin,
              arenaMax);
        } else if (entityType == "RopedRock") {
          auto rr = std::dynamic_pointer_cast<RopedRock>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::ROPE_ROCK));
          rr->position = Vector2D(x, y);

          pugi::xml_node rrProps = objectNode.child("properties");
          if (rrProps) {
            for (auto prop = rrProps.child("property"); prop;
                 prop = prop.next_sibling("property")) {
              std::string pname = prop.attribute("name").as_string();
              if (pname == "ropeLength")
                rr->SetRopeLength(prop.attribute("value").as_float());
            }
          }
          rr->Start();
        } else if (entityType == "Checkpoint") {
          float w = objectNode.attribute("width").as_float(64.0f);
          float h = objectNode.attribute("height").as_float(64.0f);
          std::string objectId = objectNode.attribute("id").as_string();
          std::string checkpointId =
              mapFileName + ":" +
              (objectId.empty()
                   ? std::to_string((int)x) + "," + std::to_string((int)y)
                   : objectId);
          auto checkpoint = std::dynamic_pointer_cast<Checkpoint>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::CHECKPOINT));
          checkpoint->Configure(checkpointId, w, h);
          checkpoint->position = Vector2D(x + w * 0.5f, y + h * 0.5f);
          checkpoint->Start();
          LOG("Checkpoint spawned at: %f, %f (%s)", x, y, checkpointId.c_str());
        } else if (entityType == "Box") {
          auto box = std::dynamic_pointer_cast<Box>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::BOX));
          box->position = Vector2D(x, y);
          box->Start();
          LOG("Box spawned at: %f, %f", x, y);
        } else if (entityType == "DropDoll") {
          auto doll = std::dynamic_pointer_cast<DropDoll>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::DROP_DOLL));

          float objW = objectNode.attribute("width").as_float(80.0f);
          float objH = objectNode.attribute("height").as_float(56.0f);
          doll->position = Vector2D(x + objW * 0.5f, y + objH * 0.5f);

          float triggerW = objW;
          for (pugi::xml_node prop =
                   objectNode.child("properties").child("property");
               prop; prop = prop.next_sibling("property")) {
            if (std::string(prop.attribute("name").as_string()) ==
                "trigger_width")
              triggerW = prop.attribute("value").as_float(objW);
          }
          doll->SetTriggerWidth(triggerW);
          doll->Start();
          LOG("DropDoll spawned at (%.0f, %.0f), trigger width %.0f",
              doll->position.getX(), doll->position.getY(), triggerW);
        } else if (entityType == "HidingRock") {
          auto rock = std::dynamic_pointer_cast<HidingRock>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::HIDING_ROCK));
          rock->position = Vector2D(x, y);

          int rockType = 1;
          for (pugi::xml_node prop =
                   objectNode.child("properties").child("property");
               prop; prop = prop.next_sibling("property")) {
            if (std::string(prop.attribute("name").as_string()) == "rock_type")
              rockType = prop.attribute("value").as_int(1);
          }
          rock->SetRockType(rockType);
          rock->Start();
          LOG("HidingRock spawned at (%.0f, %.0f), type %d", x, y, rockType);
        } else if (entityType == "Antagonist") {
          auto ant = std::dynamic_pointer_cast<Antagonist>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::ANTAGONIST));
          ant->position = Vector2D(x, y);
          for (pugi::xml_node prop =
                   objectNode.child("properties").child("property");
               prop; prop = prop.next_sibling("property")) {
            std::string pname = prop.attribute("name").as_string();
            if (pname == "appear_range")
              ant->SetAppearRange(prop.attribute("value").as_float(400.0f));
            else if (pname == "alpha")
              ant->SetAlpha(prop.attribute("value").as_int(180));
            else if (pname == "scale")
              ant->SetScale(prop.attribute("value").as_float(0.5f));
          }
          ant->Start();
          LOG("Antagonist spawned at (%.0f, %.0f)", ant->position.getX(),
              ant->position.getY());
        } else if (entityType == "Platform") {
          auto platform = std::dynamic_pointer_cast<Platform>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::PLATFORM));

          float baseX = objectNode.attribute("x").as_float();
          float baseY = objectNode.attribute("y").as_float();

          platform->texW = objectNode.attribute("width").as_int(192);
          platform->texH = objectNode.attribute("height").as_int(64);

          pugi::xml_node props = objectNode.child("properties");
          if (props) {
            for (pugi::xml_node prop = props.child("property"); prop;
                 prop = prop.next_sibling("property")) {
              std::string pname = prop.attribute("name").as_string();

              // Read name property
              if (pname == "name") {
                platform->name = prop.attribute("value").as_string();
              }

              if (pname == "speed")
                platform->speed = prop.attribute("value").as_float();

              if (pname == "texture")
                platform->texturePath = prop.attribute("value").as_string();

              if (pname == "trigger")
                platform->triggerOnPlayer = prop.attribute("value").as_bool();

              if (pname == "require_lever")
                platform->requireLever = prop.attribute("value").as_bool();

              if (pname == "platform_name")
                platform->platformName = prop.attribute("value").as_string();

              if (pname == "path") {
                std::string pathStr = prop.attribute("value").as_string();
                std::stringstream ss(pathStr);
                std::string pair;
                while (std::getline(ss, pair, ';')) {
                  std::stringstream ssPair(pair);
                  std::string xStr, yStr;
                  if (std::getline(ssPair, xStr, ',') &&
                      std::getline(ssPair, yStr, ',')) {
                    platform->AddWaypoint(
                        Vector2D(std::stof(xStr), std::stof(yStr)));
                  }
                }
              }
            }
          }

          if (!platform->waypoints.empty())
            platform->position = platform->waypoints[0];
          else
            platform->position = Vector2D(baseX, baseY);

          platform->Start();
          LOG("Platform spawned at: %f, %f", baseX, baseY);
        } else if (entityType == "Lever") {
          auto lever = std::dynamic_pointer_cast<Lever>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::LEVER));
          lever->position = Vector2D(x, y);

          // Leemos el tamaño para la colisión y dibujo
          lever->texW = objectNode.attribute("width").as_int(64);
          lever->texH = objectNode.attribute("height").as_int(64);

          pugi::xml_node props = objectNode.child("properties");
          if (props) {
            for (pugi::xml_node prop = props.child("property"); prop;
                 prop = prop.next_sibling("property")) {
              std::string pname = prop.attribute("name").as_string();

              // Read name property
              if (pname == "name") {
                lever->name = prop.attribute("value").as_string();
              }

              if (pname == "target_platform")
                lever->targetPlatformName = prop.attribute("value").as_string();

              if (pname == "locked_by_puzzle")
                lever->isLockedByPuzzle = prop.attribute("value").as_bool();
            }
          }
          lever->Start();
          LOG("Lever spawned at: %f, %f targeting %s", x, y,
              lever->targetPlatformName.c_str());
        } else if (entityType == "MemoryFragment") {
          auto frag = std::dynamic_pointer_cast<MemoryFragment>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::MEMORY_FRAGMENT));
          frag->position = Vector2D(x, y);
          for (pugi::xml_node prop =
                   objectNode.child("properties").child("property");
               prop; prop = prop.next_sibling("property")) {
            std::string pname = prop.attribute("name").as_string();
            if (pname == "video_path")
              frag->SetVideoPath(prop.attribute("value").as_string());
            else if (pname == "collect_range")
              frag->SetCollectRange(prop.attribute("value").as_float(100.0f));
            else if (pname == "scale")
              frag->SetScale(prop.attribute("value").as_float(0.5f));
            else if (pname == "fragment_id")
              frag->SetFragmentId(prop.attribute("value").as_int(-1));
          }
          frag->Start();
          LOG("MemoryFragment spawned at (%.0f, %.0f)", frag->position.getX(),
              frag->position.getY());
        }
        // Parse MovingPlatform from Tiled polylines
        else if (entityType == "MovingPlatform") {
          auto platform = std::dynamic_pointer_cast<Platform>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::PLATFORM));

          // Read the custom "speed" property if it exists
          pugi::xml_node props = objectNode.child("properties");
          if (props) {
            for (pugi::xml_node prop = props.child("property"); prop;
                 prop = prop.next_sibling("property")) {
              if (std::string(prop.attribute("name").as_string()) == "speed") {
                platform->speed = prop.attribute("value").as_float();
              }
            }
          }

          float baseX = objectNode.attribute("x").as_float();
          float baseY = objectNode.attribute("y").as_float();

          // Extract the polyline points and add them as waypoints
          pugi::xml_node polyNode = objectNode.child("polyline");
          if (polyNode) {
            std::string pointsStr = polyNode.attribute("points").as_string();
            std::stringstream ss(pointsStr);
            std::string pointPair;

            while (std::getline(ss, pointPair, ' ')) {
              if (pointPair.empty())
                continue;
              std::stringstream ssPair(pointPair);
              std::string xStr, yStr;

              if (std::getline(ssPair, xStr, ',') &&
                  std::getline(ssPair, yStr, ',')) {
                platform->AddWaypoint(
                    Vector2D(baseX + std::stof(xStr), baseY + std::stof(yStr)));
              }
            }
          }

          // Set the initial position and start the platform
          if (!platform->waypoints.empty()) {
            platform->position = platform->waypoints[0];
          } else {
            platform->position = Vector2D(baseX, baseY);
          }

          platform->Start();
          LOG("MovingPlatform spawned at: %f, %f with speed %f", baseX, baseY,
              platform->speed);
        } else if (entityType == "Boss2") {
          auto boss = std::dynamic_pointer_cast<Boss2>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::BOSS_2));
          boss->position = Vector2D(x, y);

          for (pugi::xml_node prop =
                   objectNode.child("properties").child("property");
               prop; prop = prop.next_sibling("property")) {
            if (std::string(prop.attribute("name").as_string()) ==
                "trigger_radius")
              boss->SetTriggerRadius(prop.attribute("value").as_float(500.0f));
          }
          boss->Start();
          LOG("Boss2 spawned at (%.0f, %.0f)", x, y);
        } else if (entityType == "Cape") {
          mapData.capeFound = true;
          mapData.capeX = x;
          mapData.capeY = y;
          LOG("Cape position loaded from TMX at: %f, %f", x, y);
        } else if (entityType == "Door") {
          auto door = std::dynamic_pointer_cast<Door>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::DOOR));
          door->position = Vector2D(x, y);
          door->Start();
          LOG("Door spawned at: %f, %f", x, y);
        } else if (entityType == "Wall") {
          float w = objectNode.attribute("width").as_float(64.0f);
          float h = objectNode.attribute("height").as_float(64.0f);
          PhysBody *wall = Engine::GetInstance().physics.get()->CreateRectangle(
              (int)(x + w / 2), (int)(y + h / 2), (int)w, (int)h,
              bodyType::STATIC, 0.0f);
          wall->ctype = ColliderType::PLATFORM;
          colliderList.push_back(wall);
          LOG("Wall spawned at: %f, %f (%.0fx%.0f)", x, y, w, h);
        } else if (entityType == "Tirachinas") {
          mapData.slingshotFound = true;
          mapData.slingshotX = x;
          mapData.slingshotY = y;
          LOG("Slingshot position loaded from TMX at: %f, %f", x, y);
        } else if (entityType == "Oso" || entityType == "Peluche" ||
                   entityType == "StuffedAnimal") {
          mapData.stuffedAnimalFound = true;
          mapData.stuffedAnimalX = x;
          mapData.stuffedAnimalY = y;
          LOG("StuffedAnimal position loaded from TMX at: %f, %f", x, y);
        } else if (entityType == "Wall") {
          float w = objectNode.attribute("width").as_float(64.0f);
          float h = objectNode.attribute("height").as_float(64.0f);
          int cx = (int)(x + w * 0.5f);
          int cy = (int)(y + h * 0.5f);
          PhysBody *wall = Engine::GetInstance().physics->CreateRectangle(
              cx, cy, (int)w, (int)h, bodyType::STATIC, 0.0f);
          if (wall)
            wall->ctype = ColliderType::UNKNOWN;
          LOG("Invisible wall spawned at: %f, %f (%dx%d)", x, y, (int)w,
              (int)h);
        }
      }
    } else if (groupName == "Checkpoint" || groupName == "Checkpoints" ||
               groupName == "CheckPoints") {
      for (pugi::xml_node objectNode = objectGroupNode.child("object");
           objectNode != NULL; objectNode = objectNode.next_sibling("object")) {
        std::string objClass = objectNode.attribute("type").as_string();
        if (objClass.empty())
          objClass = objectNode.attribute("class").as_string();
        if (!objClass.empty() && objClass != "Checkpoint" &&
            objClass != "CheckPoint")
          continue;

        float x = objectNode.attribute("x").as_float();
        float y = objectNode.attribute("y").as_float();
        float w = objectNode.attribute("width").as_float(64.0f);
        float h = objectNode.attribute("height").as_float(64.0f);
        std::string objectId = objectNode.attribute("id").as_string();
        std::string checkpointId =
            mapFileName + ":" +
            (objectId.empty()
                 ? std::to_string((int)x) + "," + std::to_string((int)y)
                 : objectId);

        auto checkpoint = std::dynamic_pointer_cast<Checkpoint>(
            Engine::GetInstance().entityManager->CreateEntity(
                EntityType::CHECKPOINT));
        checkpoint->Configure(checkpointId, w, h);
        checkpoint->position = Vector2D(x + w * 0.5f, y + h * 0.5f);
        checkpoint->Start();
        LOG("Checkpoint from specialized layer spawned at: %f, %f (%s)", x, y,
            checkpointId.c_str());
      }
    } else if (objectGroupNode.attribute("name").as_string() ==
               std::string("InteractiveAssets")) {
      for (pugi::xml_node objectNode = objectGroupNode.child("object");
           objectNode != NULL; objectNode = objectNode.next_sibling("object")) {
        std::string objClass = objectNode.attribute("class").as_string();
        if (objClass.empty())
          objClass = objectNode.attribute("type").as_string();

        if (objClass == "Push_Rock") {
          float x = objectNode.attribute("x").as_float();
          float y = objectNode.attribute("y").as_float();
          float w = objectNode.attribute("width").as_float(64.0f);
          float h = objectNode.attribute("height").as_float(64.0f);

          auto rock = std::dynamic_pointer_cast<PushRock>(
              Engine::GetInstance().entityManager->CreateEntity(
                  EntityType::PUSH_ROCK));

          // Read name property
          pugi::xml_node props = objectNode.child("properties");
          if (props) {
            for (pugi::xml_node prop = props.child("property"); prop;
                 prop = prop.next_sibling("property")) {
              if (std::string(prop.attribute("name").as_string()) == "name") {
                rock->name = prop.attribute("value").as_string();
              }
            }
          }

          // Tiled objects with GID have origin at bottom-left, otherwise
          // top-left
          rock->position = Vector2D(x + w / 2.0f, y + h / 2.0f);
          rock->rockWidth = w;
          rock->rockHeight = h;
          rock->Start();
          LOG("PushRock spawned at: %f, %f (size: %.0fx%.0f)", x, y, w, h);
        } else if (objClass == "Tirachinas") {
          float x = objectNode.attribute("x").as_float();
          float y = objectNode.attribute("y").as_float();
          mapData.slingshotFound = true;
          mapData.slingshotX = x;
          mapData.slingshotY = y;
          LOG("Slingshot position loaded from InteractiveAssets at: %f, %f", x,
              y);
        } else if (objClass == "Oso" || objClass == "Peluche" ||
                   objClass == "StuffedAnimal") {
          float x = objectNode.attribute("x").as_float();
          float y = objectNode.attribute("y").as_float();
          mapData.stuffedAnimalFound = true;
          mapData.stuffedAnimalX = x;
          mapData.stuffedAnimalY = y;
          LOG("StuffedAnimal position loaded from InteractiveAssets at: %f, %f",
              x, y);
        }
      }
    } else if (objectGroupNode.attribute("name").as_string() ==
               std::string("Weapons")) {
      for (pugi::xml_node objectNode = objectGroupNode.child("object");
           objectNode != NULL; objectNode = objectNode.next_sibling("object")) {
        std::string objClass = objectNode.attribute("class").as_string();
        if (objClass.empty())
          objClass = objectNode.attribute("type").as_string();

        if (objClass == "Tirachinas") {
          float x = objectNode.attribute("x").as_float();
          float y = objectNode.attribute("y").as_float();
          mapData.slingshotFound = true;
          mapData.slingshotX = x;
          mapData.slingshotY = y;
          LOG("Slingshot position loaded from Weapons at: %f, %f", x, y);
        } else if (objClass == "Oso" || objClass == "Peluche" ||
                   objClass == "StuffedAnimal") {
          float x = objectNode.attribute("x").as_float();
          float y = objectNode.attribute("y").as_float();
          mapData.stuffedAnimalFound = true;
          mapData.stuffedAnimalX = x;
          mapData.stuffedAnimalY = y;
          LOG("StuffedAnimal position loaded from Weapons at: %f, %f", x, y);
        }
      }
    }
  }

  // Ensure the player is drawn on top of all other entities (like checkpoints)
  // by moving it to the end of the EntityManager's entities list
  auto &em = Engine::GetInstance().entityManager;
  if (player != nullptr) {
    em->entities.remove(player);
    em->entities.push_back(player);
  }
}

void Map::SaveEntities(std::shared_ptr<Player> player) {

  for (pugi::xml_node objectGroupNode =
           mapFileXML.child("map").child("objectgroup");
       objectGroupNode != NULL;
       objectGroupNode = objectGroupNode.next_sibling("objectgroup")) {

    if (objectGroupNode.attribute("name").as_string() ==
        std::string("Entities")) {

      for (pugi::xml_node objectNode = objectGroupNode.child("object");
           objectNode != NULL; objectNode = objectNode.next_sibling("object")) {

        std::string entityType = objectNode.attribute("type").as_string();

        if (entityType == "Player" && player != nullptr) {
          objectNode.attribute("x").set_value(player->position.getX() - 32);
          objectNode.attribute("y").set_value(player->position.getY() - 32);
          LOG("Player position saved to XML: %f, %f",
              player->position.getX() - 32, player->position.getY() - 32);
        }
      }
    }
  }

  std::string mapPathName = mapPath + mapFileName;
  if (mapFileXML.save_file(mapPathName.c_str())) {
    LOG("Successfully saved entities to XML file: %s", mapFileName.c_str());
  } else {
    LOG("Error saving entities to XML file: %s", mapPathName.c_str());
  }
}

void Map::LoadImageLayers() {
  for (pugi::xml_node imgNode = mapFileXML.child("map").child("imagelayer");
       imgNode != NULL; imgNode = imgNode.next_sibling("imagelayer")) {
    ImageLayer *imgLayer = new ImageLayer();
    imgLayer->name = imgNode.attribute("name").as_string();
    imgLayer->offsetX = imgNode.attribute("offsetx").as_float(0.0f);
    imgLayer->offsetY = imgNode.attribute("offsety").as_float(0.0f);

    // Determine default parallax factor based on layer name if not explicitly
    // set in TMX
    float defaultParallax = 1.0f;
    std::string lowerName = imgLayer->name;
    for (char &c : lowerName)
      c = ::tolower(c);

        if (lowerName.find("fondo") != std::string::npos) 
        {
            defaultParallax = 0.0f; // Fondo is static on screen
        }
        else if (lowerName.find("background") != std::string::npos || 
                 lowerName.find("back") != std::string::npos) 
        {
            defaultParallax = 1.0f; // Background is static in the world
        }
        else if (lowerName.find("middle") != std::string::npos ||
                 lowerName.find("medio") != std::string::npos)
        {
            defaultParallax = 1.0f; // Middleground is static in the world
        }
        else if (lowerName.find("foreground") != std::string::npos || 
                 lowerName.find("front") != std::string::npos) 
        {
            defaultParallax = 0.5f; // Foreground moves fast (at old fondo speed)
        }

    // Check if parallaxx attribute is explicitly set in TMX, otherwise use
    // defaultParallax 
    pugi::xml_attribute pxAttr = imgNode.attribute("parallaxx");
    imgLayer->parallaxFactorX = pxAttr ? pxAttr.as_float() : defaultParallax;

    // Set vertical parallax to 1.0f so background scrolls with the camera vertically
    imgLayer->parallaxFactorY = imgNode.attribute("parallaxy").as_float(1.0f);

    imgLayer->source = imgNode.child("image").attribute("source").as_string();

    std::string fullPath = mapPath + imgLayer->source;
    imgLayer->texture = Engine::GetInstance().textures->Load(fullPath.c_str());

    mapData.imageLayers.push_back(imgLayer);
  }
}

void Map::LoadDecorationObjects() {
  const std::vector<std::string> excludedNames = {
      "Entities",       "Collisions",           "Navigation",
      "Checkpoint",     "Checkpoints",          "CheckPoints",
      "AnimatedPlants", "AnimatedPlants front", "InteractiveAssets"};

  for (pugi::xml_node groupNode = mapFileXML.child("map").child("objectgroup");
       groupNode != NULL; groupNode = groupNode.next_sibling("objectgroup")) {
    std::string groupName = groupNode.attribute("name").as_string();
    bool skip = false;
    for (const std::string &excluded : excludedNames) {
      if (groupName == excluded) {
        skip = true;
        break;
      }
    }
    if (skip)
      continue;

    // Determine parallax speed and front/back based on Tiled layer name
    // Hollow Knight-style depth: BG slower, Middleground 1:1, FG faster
    float layerParallax = 1.0f;
    bool layerIsFront = false;

    std::string lowerGroupName = groupName;
    for (char &c : lowerGroupName)
      c = ::tolower(c);

    if (lowerGroupName.find("fondo") != std::string::npos) {
      layerParallax = 0.0f; // Fondo is static on screen
      layerIsFront = false;
    } else if (lowerGroupName.find("background") != std::string::npos ||
               lowerGroupName.find("back") != std::string::npos ||
               lowerGroupName.find("bakground") != std::string::npos) {
      layerParallax = 1.0f; // Background is static in the world
      layerIsFront = false;
    } else if (lowerGroupName.find("middle") != std::string::npos ||
               lowerGroupName.find("medio") != std::string::npos) {
      layerParallax = 1.0f; // Middleground is static in the world
      layerIsFront = false;
    } else if (lowerGroupName.find("foreground") != std::string::npos ||
               lowerGroupName.find("front") != std::string::npos) {
      layerParallax = 1.02f; // Extremely subtle foreground decoration parallax
      layerIsFront = true;
    }
    // Middleground and everything else stays at 1.0, isFront = false

    // TMX parallaxx attribute overrides the default if present
    float tmxParallaxx = groupNode.attribute("parallaxx").as_float(0.0f);
    if (tmxParallaxx > 0.0f) {
      layerParallax = tmxParallaxx;
    }

    std::vector<DecorationObject *> layerDecos;

    for (pugi::xml_node objNode = groupNode.child("object"); objNode != NULL;
         objNode = objNode.next_sibling("object")) {
      unsigned int rawGid = objNode.attribute("gid").as_uint(0);
      if (rawGid == 0)
        continue;

      const unsigned int FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
      const unsigned int FLIPPED_VERTICALLY_FLAG = 0x40000000;
      const unsigned int FLIPPED_DIAGONALLY_FLAG = 0x20000000;

      int gid = rawGid & ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG |
                           FLIPPED_DIAGONALLY_FLAG);

      TileSet *ts = GetTilesetFromTileId(gid);
      if (ts == nullptr)
        continue;

      int relativeId = gid - ts->firstGid;

      if (ts->tileTextures.find(relativeId) == ts->tileTextures.end()) {
        for (pugi::xml_node tsNode = mapFileXML.child("map").child("tileset");
             tsNode != NULL; tsNode = tsNode.next_sibling("tileset")) {
          if (tsNode.attribute("firstgid").as_int() != ts->firstGid)
            continue;

          std::string tsxSource = tsNode.attribute("source").as_string();
          if (tsxSource.empty())
            break;

          std::string fullTsxPath = mapPath + tsxSource;
          std::string tsxFolder =
              fullTsxPath.substr(0, fullTsxPath.find_last_of("/\\") + 1);

          pugi::xml_document tsxDoc;
          if (!tsxDoc.load_file(fullTsxPath.c_str())) {
            LOG("WARNING: Could not load tsx file: %s", fullTsxPath.c_str());
            break;
          }

          for (pugi::xml_node tileNode = tsxDoc.child("tileset").child("tile");
               tileNode != NULL; tileNode = tileNode.next_sibling("tile")) {
            if (tileNode.attribute("id").as_int(-1) != relativeId)
              continue;

            std::string imgSrc =
                tileNode.child("image").attribute("source").as_string();
            if (!imgSrc.empty()) {
              std::string fullPath = tsxFolder + imgSrc;
              SDL_Texture *tex =
                  Engine::GetInstance().textures->Load(fullPath.c_str());
              ts->tileTextures[relativeId] = tex;
            }

          }
        }
      }

      DecorationObject *deco = new DecorationObject();
      deco->x = objNode.attribute("x").as_float();
      deco->y = objNode.attribute("y").as_float();
      deco->width = objNode.attribute("width").as_float();
      deco->height = objNode.attribute("height").as_float();
      deco->rotation = objNode.attribute("rotation").as_double(0.0);
      deco->isFront = layerIsFront;
      deco->gid = gid;
      deco->flipH = (rawGid & FLIPPED_HORIZONTALLY_FLAG) != 0;
      deco->flipV = (rawGid & FLIPPED_VERTICALLY_FLAG) != 0;
      deco->parallaxSpeed = layerParallax;

      auto it = ts->tileTextures.find(relativeId);
      deco->texture = (it != ts->tileTextures.end()) ? it->second : nullptr;

      layerDecos.push_back(deco);
    }

    std::sort(layerDecos.begin(), layerDecos.end(),
              [](const DecorationObject *a, const DecorationObject *b) {
                return a->y < b->y;
              });

    for (auto *deco : layerDecos) {
      mapData.decorationObjects.push_back(deco);
    }
  }
}

void Map::LoadAnimatedPlants() {
  for (pugi::xml_node groupNode = mapFileXML.child("map").child("objectgroup");
       groupNode != NULL; groupNode = groupNode.next_sibling("objectgroup")) {
    std::string layerName = groupNode.attribute("name").as_string();
    if (layerName != "AnimatedPlants" && layerName != "AnimatedPlants front")
      continue;

    for (pugi::xml_node objNode = groupNode.child("object"); objNode != NULL;
         objNode = objNode.next_sibling("object")) {
      unsigned int rawGid = objNode.attribute("gid").as_uint(0);

      if (rawGid == 0)
        continue;

      const unsigned int FLIP_H = 0x80000000;
      const unsigned int FLIP_V = 0x40000000;
      const unsigned int FLIP_D = 0x20000000;
      int gid = rawGid & ~(FLIP_H | FLIP_V | FLIP_D);

      TileSet *ts = GetTilesetFromTileId(gid);

      if (ts == nullptr)
        continue;

      std::string tsxSource = "";
      for (pugi::xml_node tsNode = mapFileXML.child("map").child("tileset");
           tsNode != NULL; tsNode = tsNode.next_sibling("tileset")) {
        if (tsNode.attribute("firstgid").as_int() == ts->firstGid) {
          tsxSource = tsNode.attribute("source").as_string();
          break;
        }
      }

      if (tsxSource.empty())
        continue;

      std::string fullTsxPath = mapPath + tsxSource;

      AnimatedPlantObject *plant = new AnimatedPlantObject();
      plant->x = objNode.attribute("x").as_float();
      plant->y = objNode.attribute("y").as_float() -
                 objNode.attribute("height").as_float();
      plant->w = objNode.attribute("width").as_float();
      plant->h = objNode.attribute("height").as_float();
      plant->isFront = (layerName == "AnimatedPlants front");
      plant->tsxPath = tsxSource;

      std::unordered_map<int, std::string> aliases = {{0, "idle"}};
      bool loaded = plant->anim.LoadFromTSX(fullTsxPath.c_str(), aliases);

      if (!loaded) {
        delete plant;
        continue;
      }

      plant->anim.SetCurrent("idle");
      pugi::xml_document tsxDoc;
      if (tsxDoc.load_file(fullTsxPath.c_str())) {
        std::string imgSource = tsxDoc.child("tileset")
                                    .child("image")
                                    .attribute("source")
                                    .as_string();
        std::string tsxFolder =
            tsxSource.substr(0, tsxSource.find_last_of("/\\") + 1);
        std::string pngPath = mapPath + tsxFolder + imgSource;
        plant->texture = Engine::GetInstance().textures->Load(pngPath.c_str());
      }
      mapData.animatedPlants.push_back(plant);
    }
  }
}

std::vector<PortalData> Map::GetPortals() const {
  std::vector<PortalData> result;

  for (pugi::xml_node groupNode = mapFileXML.child("map").child("objectgroup");
       groupNode; groupNode = groupNode.next_sibling("objectgroup")) {
    if (std::string(groupNode.attribute("name").as_string()) != "Portals")
      continue;

    for (pugi::xml_node obj = groupNode.child("object"); obj;
         obj = obj.next_sibling("object")) {
      PortalData p;
      p.x = obj.attribute("x").as_float();
      p.y = obj.attribute("y").as_float();
      p.w = obj.attribute("width").as_float(64.0f);
      p.h = obj.attribute("height").as_float(128.0f);

      for (pugi::xml_node prop = obj.child("properties").child("property");
           prop; prop = prop.next_sibling("property")) {
        std::string name = prop.attribute("name").as_string();
        if (name == "target")
          p.targetFile = prop.attribute("value").as_string();
        if (name == "spawnId")
          p.spawnId = prop.attribute("value").as_string();
      }

      if (!p.targetFile.empty())
        result.push_back(p);
    }
  }
  return result;
}

bool Map::GetSpawnById(const std::string &id, float &outX, float &outY) const {
  for (pugi::xml_node groupNode = mapFileXML.child("map").child("objectgroup");
       groupNode; groupNode = groupNode.next_sibling("objectgroup")) {
    if (std::string(groupNode.attribute("name").as_string()) != "Spawns")
      continue;

    for (pugi::xml_node obj = groupNode.child("object"); obj;
         obj = obj.next_sibling("object")) {
      for (pugi::xml_node prop = obj.child("properties").child("property");
           prop; prop = prop.next_sibling("property")) {
        if (std::string(prop.attribute("name").as_string()) == "spawnId" &&
            std::string(prop.attribute("value").as_string()) == id) {
          outX = obj.attribute("x").as_float();
          outY = obj.attribute("y").as_float();
          return true;
        }
      }
    }
  }
  return false;
}
std::vector<Map::MapObject> Map::GetPuzzleObjects() const
{
    std::vector<Map::MapObject> result;

    for (pugi::xml_node groupNode = mapFileXML.child("map").child("objectgroup");
        groupNode; groupNode = groupNode.next_sibling("objectgroup"))
    {
        if (std::string(groupNode.attribute("name").as_string()) != "PuzzleObjects") continue;

        for (pugi::xml_node obj = groupNode.child("object"); obj; obj = obj.next_sibling("object"))
        {
            Map::MapObject mo;
            mo.name = obj.attribute("name").as_string();
            mo.rect.x = obj.attribute("x").as_float();
            mo.rect.y = obj.attribute("y").as_float();
            mo.rect.w = obj.attribute("width").as_float(48.0f);
            mo.rect.h = obj.attribute("height").as_float(48.0f);
            result.push_back(mo);
        }
        break;
    }

    return result;
}

std::vector<Map::MapObject> Map::GetPuzzleObjects3() const
{
    std::vector<Map::MapObject> result;

    for (pugi::xml_node groupNode = mapFileXML.child("map").child("objectgroup");
        groupNode; groupNode = groupNode.next_sibling("objectgroup"))
    {
        if (std::string(groupNode.attribute("name").as_string()) != "PuzzleObjects3") continue;

        for (pugi::xml_node obj = groupNode.child("object"); obj; obj = obj.next_sibling("object"))
        {
            Map::MapObject mo;
            mo.name = obj.attribute("name").as_string();
            mo.rect.x = obj.attribute("x").as_float();
            mo.rect.y = obj.attribute("y").as_float();
            mo.rect.w = obj.attribute("width").as_float(48.0f);
            mo.rect.h = obj.attribute("height").as_float(48.0f);

            for (pugi::xml_node prop = obj.child("properties").child("property");
                prop; prop = prop.next_sibling("property"))
            {
                std::string propName = prop.attribute("name").as_string();
                if (propName == "requiredClicks") {
                    mo.requiredClicks = prop.attribute("value").as_int(1);
                    mo.name += "|" + std::to_string(mo.requiredClicks);
                }
                if (propName == "platformTarget") {
                    mo.platformTarget = prop.attribute("value").as_string();
                }
                if (propName == "flipH") {
                    mo.flipH = prop.attribute("value").as_bool();
                }
            }

            result.push_back(mo);
        }
        break;
    }

    return result;
}
