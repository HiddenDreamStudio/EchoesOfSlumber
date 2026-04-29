#pragma once
#include "Module.h"
#include "Entity.h"
#include <list>
#include <cmath>           // for floor in METERS_TO_PIXELS
#include <box2d/box2d.h>   // Box2D 3.x single header

#define GRAVITY_X 0.0f
#define GRAVITY_Y -10.0f

#define PIXELS_PER_METER 50.0f // if touched change METER_PER_PIXEL too
#define METER_PER_PIXEL  0.02f // this is 1 / PIXELS_PER_METER !

#define METERS_TO_PIXELS(m) ((int) floor(PIXELS_PER_METER * (m)))
#define PIXEL_TO_METERS(p)  ((float) METER_PER_PIXEL * (p))

#define DEGTORAD 0.0174532925199432957f
#define RADTODEG 57.295779513082320876f

enum bodyType {
    DYNAMIC,
    STATIC,
    KINEMATIC
};

enum class ColliderType {
    PLAYER,
    ITEM,
    PLATFORM,
    ENEMY,
    ATTACK,
    LEDGE,
    CHECKPOINT,
    BOX,
    PUSH_ROCK,
    PROJECTILE,
    UNKNOWN
    // ..
};

// Small class to return to other modules to track position and rotation of physics bodies
class PhysBody
{
public:
    PhysBody() : listener(NULL), body(b2_nullBodyId), ctype(ColliderType::UNKNOWN) {}
    ~PhysBody() {}

    void  GetPosition(int& x, int& y) const;
    void  SetPosition(int x, int y);
    float GetRotation() const;
    bool  Contains(int x, int y) const;
    int   RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const;

public:
    b2BodyId body;              // id instead of pointer (v3.x)
    Entity* listener;
    ColliderType ctype;
};

// Module --------------------------------------
class Physics : public Module
{
public:

    // Constructors & Destructors
    Physics();
    ~Physics();

    // Main module steps
    bool Start();
    bool PreUpdate();
    bool PostUpdate();
    bool CleanUp();

    // Create basic physics objects
    PhysBody* CreateRectangle(int x, int y, int width, int height, bodyType type, float friction = 0.6f);
    PhysBody* CreateCircle(int x, int y, int radious, bodyType type);
    PhysBody* CreateCapsule(int x, int y, int width, int height, bodyType type, float friction = 0.6f);
    PhysBody* CreateRectangleSensor(int x, int y, int width, int height, bodyType type);
    PhysBody* CreateChain(int x, int y, int* points, int size, bodyType type, float friction = 0.6f);
    PhysBody* CreateConvexPolygon(int x, int y, int* points, int size, bodyType type, float friction = 0.6f);

    // Invoked from our event processing
    void BeginContact(b2ShapeId shapeA, b2ShapeId shapeB);
    void EndContact(b2ShapeId shapeA, b2ShapeId shapeB);

    void DeletePhysBody(PhysBody* physBody);
    bool IsPendingToDelete(PhysBody* physBody);
    bool IsDebug() const { return debug; }

    // --- Velocity helpers (thin wrappers over Box2D 3.x C API)
    b2Vec2 GetLinearVelocity(const PhysBody* p) const;
    float  GetXVelocity(const PhysBody* p) const;
    float  GetYVelocity(const PhysBody* p) const;

    void   SetLinearVelocity(PhysBody* p, const b2Vec2& v) const;
    void   SetLinearVelocity(PhysBody* p, float vx, float vy) const;
    void   SetXVelocity(PhysBody* p, float vx) const;
    void   SetYVelocity(PhysBody* p, float vy) const;

    // --- Impulse helper (handy for jumps/dashes)
    void   ApplyLinearImpulseToCenter(PhysBody* p, float ix, float iy, bool wake = true) const;

    // --- World raycast helper (pixel coords, returns true if hit)
    bool   RayCastWorld(int x1, int y1, int x2, int y2, float& hitX, float& hitY) const;

private:
    // helpers
    static b2BodyType ToB2Type(bodyType t);
    static void* ToUserData(PhysBody* p) { return (void*)p; }
    static PhysBody* FromUserData(void* ud) { return (PhysBody*)ud; }
    static PhysBody* BodyToPhys(b2BodyId b) { return FromUserData(b2Body_GetUserData(b)); }

    // --- Debug draw callbacks (Box2D 3.1 signatures)
    static void DrawSegmentCb(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* ctx);
    static void DrawPolygonCb(const b2Vec2* verts, int count, b2HexColor color, void* ctx);
    static void DrawSolidPolygonCb(b2Transform xf, const b2Vec2* verts, int count, float radius, b2HexColor color, void* ctx);
    static void DrawCircleCb(b2Vec2 center, float radius, b2HexColor color, void* ctx);
    static void DrawSolidCircleCb(b2Transform xf, float radius, b2HexColor color, void* ctx);

    // --- Defensive no-op stubs (so Box2D never calls a null fn ptr)
    static void DrawSolidCapsuleStub(b2Vec2 a, b2Vec2 b, float r, b2HexColor c, void* ctx);
    static void DrawPointStub(b2Vec2 p, float size, b2HexColor c, void* ctx);
    static void DrawStringStub(b2Vec2 p, const char* s, b2HexColor c, void* ctx);
    static void DrawTransformStub(b2Transform xf, void* ctx);

private:

    // Debug mode
    bool debug;

    // Box2D World (id instead of pointer)
    b2WorldId world;

    // List of physics bodies
    std::list<PhysBody*> bodiesToDelete;
};
