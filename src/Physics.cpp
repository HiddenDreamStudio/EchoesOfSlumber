#include "Physics.h"
#include "Input.h"
#include "Engine.h"
#include "Log.h"
#include "math.h"
#include <SDL3/SDL_keycode.h>
#include "Render.h"
#include "Player.h"
#include "Window.h"
#include "Scene.h"
#include <vector>
#include <box2d/box2d.h>

Physics::Physics() : Module()
{
    world = b2_nullWorldId;
    debug = false; 
}

// Destructor
Physics::~Physics()
{
}

bool Physics::Start()
{
    LOG("Creating Physics 2D environment");

    b2WorldDef wdef = b2DefaultWorldDef();
    wdef.gravity.x = GRAVITY_X;
    wdef.gravity.y = -GRAVITY_Y;
    world = b2CreateWorld(&wdef);

    return true;
}

bool Physics::PreUpdate()
{
    bool ret = true;

    if (Engine::GetInstance().scene->isPaused_) return true;

    float dt = Engine::GetInstance().GetDt() / 1000.0f;
    b2World_Step(world, dt, 4);

    const b2SensorEvents sensorEvents = b2World_GetSensorEvents(world);
    for (int i = 0; i < sensorEvents.beginCount; ++i)
    {
        const b2SensorBeginTouchEvent& e = sensorEvents.beginEvents[i];
        if (!b2Shape_IsValid(e.sensorShapeId) || !b2Shape_IsValid(e.visitorShapeId)) continue;
        BeginContact(e.sensorShapeId, e.visitorShapeId);
    }
    for (int i = 0; i < sensorEvents.endCount; ++i)
    {
        const b2SensorEndTouchEvent& e = sensorEvents.endEvents[i];
        if (!b2Shape_IsValid(e.sensorShapeId) || !b2Shape_IsValid(e.visitorShapeId)) continue;
        EndContact(e.sensorShapeId, e.visitorShapeId);
    }

    const b2ContactEvents contactEvents = b2World_GetContactEvents(world);
    for (int i = 0; i < contactEvents.beginCount; ++i)
    {
        const b2ContactBeginTouchEvent& e = contactEvents.beginEvents[i];
        if (!b2Shape_IsValid(e.shapeIdA) || !b2Shape_IsValid(e.shapeIdB)) continue;
        BeginContact(e.shapeIdA, e.shapeIdB);
    }
    for (int i = 0; i < contactEvents.endCount; ++i)
    {
        const b2ContactEndTouchEvent& e = contactEvents.endEvents[i];
        if (!b2Shape_IsValid(e.shapeIdA) || !b2Shape_IsValid(e.shapeIdB)) continue;
        EndContact(e.shapeIdA, e.shapeIdB);
    }
    return ret;
}

PhysBody* Physics::CreateRectangle(int x, int y, int width, int height, bodyType type, float friction)
{
    b2BodyDef def = b2DefaultBodyDef();
    def.type = ToB2Type(type);
    def.position = { PIXEL_TO_METERS(x), PIXEL_TO_METERS(y) };

    b2BodyId b = b2CreateBody(world, &def);

    b2Polygon box = b2MakeBox(PIXEL_TO_METERS(width) * 0.5f, PIXEL_TO_METERS(height) * 0.5f);
    b2ShapeDef sdef = b2DefaultShapeDef();
    sdef.density = 1.0f;
    sdef.material.friction = friction;
    sdef.enableContactEvents = true;   
    sdef.enableSensorEvents = true;   

    b2CreatePolygonShape(b, &sdef, &box);

    PhysBody* pbody = new PhysBody();
    pbody->body = b;
    b2Body_SetUserData(b, ToUserData(pbody));

    return pbody;
}

PhysBody* Physics::CreateCircle(int x, int y, int radious, bodyType type)
{
    b2BodyDef def = b2DefaultBodyDef();
    def.type = ToB2Type(type);
    def.position = { PIXEL_TO_METERS(x), PIXEL_TO_METERS(y) };

    b2BodyId b = b2CreateBody(world, &def);

    b2Circle circle;
    circle.center = { 0.0f, 0.0f };
    circle.radius = PIXEL_TO_METERS(radious);
    b2ShapeDef sdef = b2DefaultShapeDef();
    sdef.density = 1.0f;
    sdef.enableContactEvents = true;
    sdef.enableSensorEvents = true;

    b2CreateCircleShape(b, &sdef, &circle);

    PhysBody* pbody = new PhysBody();
    pbody->body = b;
    b2Body_SetUserData(b, ToUserData(pbody));
    return pbody;
}

PhysBody* Physics::CreateCapsule(int x, int y, int width, int height, bodyType type, float friction)
{
    b2BodyDef def = b2DefaultBodyDef();
    def.type = ToB2Type(type);
    def.position = { PIXEL_TO_METERS(x), PIXEL_TO_METERS(y) };
    def.fixedRotation = true;

    b2BodyId b = b2CreateBody(world, &def);

    float radius = PIXEL_TO_METERS(width) * 0.5f;
    float halfHeight = PIXEL_TO_METERS(height) * 0.5f;
    float segmentHalf = halfHeight - radius;
    if (segmentHalf < 0.0f) segmentHalf = 0.0f;

    b2Capsule capsule;
    capsule.center1 = { 0.0f, -segmentHalf };
    capsule.center2 = { 0.0f,  segmentHalf };
    capsule.radius = radius;

    b2ShapeDef sdef = b2DefaultShapeDef();
    sdef.density = 1.0f;
    sdef.material.friction = friction;
    sdef.enableContactEvents = true;
    sdef.enableSensorEvents = true;

    b2CreateCapsuleShape(b, &sdef, &capsule);

    PhysBody* pbody = new PhysBody();
    pbody->body = b;
    b2Body_SetUserData(b, ToUserData(pbody));
    return pbody;
}

PhysBody* Physics::CreateRectangleSensor(int x, int y, int width, int height, bodyType type)
{
    b2BodyDef def = b2DefaultBodyDef();
    def.type = ToB2Type(type);
    def.position = { PIXEL_TO_METERS(x), PIXEL_TO_METERS(y) };

    b2BodyId b = b2CreateBody(world, &def);

    b2Polygon box = b2MakeBox(PIXEL_TO_METERS(width) * 0.5f, PIXEL_TO_METERS(height) * 0.5f);
    b2ShapeDef sdef = b2DefaultShapeDef();
    sdef.density = 1.0f;
    sdef.isSensor = true;
    sdef.enableContactEvents = true;
    sdef.enableSensorEvents = true;

    b2CreatePolygonShape(b, &sdef, &box);

    PhysBody* pbody = new PhysBody();
    pbody->body = b;
    b2Body_SetUserData(b, ToUserData(pbody));
    return pbody;
}

PhysBody* Physics::CreateChain(int x, int y, int* points, int size, bodyType type, float friction)
{
    b2BodyDef def = b2DefaultBodyDef();
    def.type = ToB2Type(type);
    def.position = { PIXEL_TO_METERS(x), PIXEL_TO_METERS(y) };

    b2BodyId b = b2CreateBody(world, &def);

    const int count = size / 2;
    std::vector<b2Vec2> verts(count);
    for (int i = 0; i < count; ++i)
    {
        verts[i].x = PIXEL_TO_METERS(points[i * 2 + 0]);
        verts[i].y = PIXEL_TO_METERS(points[i * 2 + 1]);
    }

    b2ChainDef cdef = b2DefaultChainDef();
    cdef.points = verts.data();
    cdef.count = count;
    cdef.isLoop = true;
    cdef.enableSensorEvents = true;
    b2CreateChain(b, &cdef);

    PhysBody* pbody = new PhysBody();
    pbody->body = b;
    b2Body_SetUserData(b, ToUserData(pbody));
    return pbody;
}

PhysBody* Physics::CreateConvexPolygon(int x, int y, int* points, int size, bodyType type, float friction)
{
    b2BodyDef def = b2DefaultBodyDef();
    def.type = ToB2Type(type);
    def.position = { PIXEL_TO_METERS(x), PIXEL_TO_METERS(y) };

    b2BodyId b = b2CreateBody(world, &def);

    const int count = size / 2;
    std::vector<b2Vec2> verts(count);
    for (int i = 0; i < count; ++i)
    {
        verts[i].x = PIXEL_TO_METERS(points[i * 2 + 0]);
        verts[i].y = PIXEL_TO_METERS(points[i * 2 + 1]);
    }

    b2Hull hull = b2ComputeHull(verts.data(), count);
    if (hull.count > 0) {
        b2Polygon poly = b2MakePolygon(&hull, 0.0f);
        b2ShapeDef sdef = b2DefaultShapeDef();
        sdef.density = 1.0f;
        sdef.material.friction = friction;
        sdef.enableContactEvents = true;
        sdef.enableSensorEvents = true;
        b2CreatePolygonShape(b, &sdef, &poly);
    }

    PhysBody* pbody = new PhysBody();
    pbody->body = b;
    b2Body_SetUserData(b, ToUserData(pbody));
    return pbody;
}

bool Physics::PostUpdate()
{
    bool ret = true;

    if (Engine::GetInstance().input.get()->GetKey(SDL_SCANCODE_F9) == KEY_DOWN)
        debug = !debug;

    if (debug)
    {
        if (B2_IS_NULL(world) == false)
        {
            b2DebugDraw dd = {};
            dd.context = this;
            dd.drawShapes = true;
            dd.drawContacts = true;
            dd.DrawSegmentFcn = &Physics::DrawSegmentCb;
            dd.DrawPolygonFcn = &Physics::DrawPolygonCb;
            dd.DrawSolidPolygonFcn = &Physics::DrawSolidPolygonCb;
            dd.DrawCircleFcn = &Physics::DrawCircleCb;
            dd.DrawSolidCircleFcn = &Physics::DrawSolidCircleCb;
            dd.DrawSolidCapsuleFcn = &Physics::DrawSolidCapsuleStub; 
            dd.DrawPointFcn = &Physics::DrawPointStub;
            dd.DrawStringFcn = &Physics::DrawStringStub;
            dd.DrawTransformFcn = &Physics::DrawTransformStub;
            b2World_Draw(world, &dd);
        }
    }

    for (PhysBody* physBody : bodiesToDelete) {
        if (b2Body_IsValid(physBody->body))
            b2DestroyBody(physBody->body);
    }
    bodiesToDelete.clear();

    return ret;
}

bool Physics::CleanUp()
{
    LOG("Destroying physics world");
    if (!B2_IS_NULL(world))
    {
        b2DestroyWorld(world);
        world = b2_nullWorldId;
    }
    return true;
}

void Physics::BeginContact(b2ShapeId shapeA, b2ShapeId shapeB)
{
    if (!b2Shape_IsValid(shapeA) || !b2Shape_IsValid(shapeB)) return;

    b2BodyId bodyA = b2Shape_GetBody(shapeA);
    b2BodyId bodyB = b2Shape_GetBody(shapeB);
    if (B2_IS_NULL(bodyA) || B2_IS_NULL(bodyB)) return;

    PhysBody* physA = BodyToPhys(bodyA);
    PhysBody* physB = BodyToPhys(bodyB);
    if (!physA || !physB) return;

    if (physA->listener && !IsPendingToDelete(physA)) physA->listener->OnCollision(physA, physB);
    if (physB->listener && !IsPendingToDelete(physB)) physB->listener->OnCollision(physB, physA);
}

void Physics::EndContact(b2ShapeId shapeA, b2ShapeId shapeB)
{
    if (!b2Shape_IsValid(shapeA) || !b2Shape_IsValid(shapeB)) return;

    b2BodyId bodyA = b2Shape_GetBody(shapeA);
    b2BodyId bodyB = b2Shape_GetBody(shapeB);
    if (B2_IS_NULL(bodyA) || B2_IS_NULL(bodyB)) return;

    PhysBody* physA = BodyToPhys(bodyA);
    PhysBody* physB = BodyToPhys(bodyB);
    if (!physA || !physB) return;
    if (IsPendingToDelete(physA) || IsPendingToDelete(physB)) return;

    if (physA->listener && !IsPendingToDelete(physA)) physA->listener->OnCollisionEnd(physA, physB);
    if (physB->listener && !IsPendingToDelete(physB)) physB->listener->OnCollisionEnd(physB, physA);
}

void Physics::DeletePhysBody(PhysBody* physBody)
{
	if (B2_IS_NULL(world)) return;
    if (physBody && !B2_IS_NULL(physBody->body) && physBody->listener && physBody->listener->active)
    {
        b2Body_SetUserData(physBody->body, nullptr);
    }
    bodiesToDelete.push_back(physBody);
}

bool Physics::IsPendingToDelete(PhysBody* physBody) {
    bool pendingToDelete = false;
    for (PhysBody* _physBody : bodiesToDelete) {
        if (_physBody == physBody) {
            pendingToDelete = true;
            break;
        }
    }
    return pendingToDelete;
}

void Physics::FlushPendingDeletes()
{
    for (PhysBody* physBody : bodiesToDelete) {
        if (physBody && b2Body_IsValid(physBody->body))
            b2DestroyBody(physBody->body);
        delete physBody;
    }
    bodiesToDelete.clear();
}

b2Vec2 Physics::GetLinearVelocity(const PhysBody* p) const
{
    return b2Body_GetLinearVelocity(p->body);
}

float Physics::GetXVelocity(const PhysBody* p) const
{
    return b2Body_GetLinearVelocity(p->body).x;
}

float Physics::GetYVelocity(const PhysBody* p) const
{
    return b2Body_GetLinearVelocity(p->body).y;
}

void Physics::SetLinearVelocity(PhysBody* p, const b2Vec2& v) const
{
    b2Body_SetLinearVelocity(p->body, v);
}

void Physics::SetLinearVelocity(PhysBody* p, float vx, float vy) const
{
    b2Vec2 v = { vx, vy };
    b2Body_SetLinearVelocity(p->body, v);
}

void Physics::SetXVelocity(PhysBody* p, float vx) const
{
    b2Vec2 v = b2Body_GetLinearVelocity(p->body);
    v.x = vx;
    b2Body_SetLinearVelocity(p->body, v);
}

void Physics::SetYVelocity(PhysBody* p, float vy) const
{
    b2Vec2 v = b2Body_GetLinearVelocity(p->body);
    v.y = vy;
    b2Body_SetLinearVelocity(p->body, v);
}

void Physics::ApplyLinearImpulseToCenter(PhysBody* p, float ix, float iy, bool wake) const
{
    b2Vec2 imp = { ix, iy };
    b2Body_ApplyLinearImpulseToCenter(p->body, imp, wake);
}

bool Physics::RayCastWorld(int x1, int y1, int x2, int y2, float& hitX, float& hitY) const
{
    const b2Vec2 origin = { PIXEL_TO_METERS(x1), PIXEL_TO_METERS(y1) };
    const b2Vec2 target = { PIXEL_TO_METERS(x2), PIXEL_TO_METERS(y2) };
    const b2Vec2 translation = { target.x - origin.x, target.y - origin.y };
    b2QueryFilter qf = b2DefaultQueryFilter();
    const b2RayResult res = b2World_CastRayClosest(world, origin, translation, qf);
    if (!res.hit) return false;
    hitX = (float)METERS_TO_PIXELS(origin.x + translation.x * res.fraction);
    hitY = (float)METERS_TO_PIXELS(origin.y + translation.y * res.fraction);
    return true;
}

void PhysBody::GetPosition(int& x, int& y) const
{
    b2Vec2 pos = b2Body_GetPosition(body);
    x = METERS_TO_PIXELS(pos.x);
    y = METERS_TO_PIXELS(pos.y);
}

void PhysBody::SetPosition(int x, int y)
{
    b2Vec2 pos = { PIXEL_TO_METERS(x), PIXEL_TO_METERS(y) };
    b2Body_SetTransform(body, pos, b2MakeRot(0));
    b2Body_SetAwake(body, true);
}

float PhysBody::GetRotation() const
{
    b2Transform xf = b2Body_GetTransform(body);
    return RADTODEG * b2Rot_GetAngle(xf.q);
}

bool PhysBody::Contains(int x, int y) const
{
    const b2Vec2 p = { PIXEL_TO_METERS(x), PIXEL_TO_METERS(y) };
    const int shapeCount = b2Body_GetShapeCount(body);
    if (shapeCount == 0) return false;
    std::vector<b2ShapeId> shapes(shapeCount);
    b2Body_GetShapes(body, shapes.data(), shapeCount);
    for (int i = 0; i < shapeCount; ++i)
    {
        if (b2Shape_TestPoint(shapes[i], p))
            return true;
    }
    return false;
}

int PhysBody::RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const
{
    const b2Vec2 p1 = { PIXEL_TO_METERS(x1), PIXEL_TO_METERS(y1) };
    const b2Vec2 d = { PIXEL_TO_METERS(x2 - x1), PIXEL_TO_METERS(y2 - y1) };
    b2WorldId world = b2Body_GetWorld(body);
    b2QueryFilter qf = b2DefaultQueryFilter();
    const b2RayResult res = b2World_CastRayClosest(world, p1, d, qf);
    if (!res.hit) return -1;
    normal_x = res.normal.x;
    normal_y = res.normal.y;
    const float fx = float(x2 - x1);
    const float fy = float(y2 - y1);
    const float distPixels = sqrtf(fx * fx + fy * fy);
    return int(floorf(res.fraction * distPixels));
}

b2BodyType Physics::ToB2Type(bodyType t)
{
    switch (t)
    {
    case DYNAMIC:   return b2_dynamicBody;
    case STATIC:    return b2_staticBody;
    case KINEMATIC: return b2_kinematicBody;
    default:        return b2_staticBody;
    }
}

void Physics::DrawSegmentCb(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* ctx)
{
    auto& r = *Engine::GetInstance().render.get();
    r.DrawLine(METERS_TO_PIXELS(p1.x), METERS_TO_PIXELS(p1.y),
        METERS_TO_PIXELS(p2.x), METERS_TO_PIXELS(p2.y), 255, 255, 255);
}

void Physics::DrawPolygonCb(const b2Vec2* v, int n, b2HexColor color, void* ctx)
{
    auto& r = *Engine::GetInstance().render.get();
    for (int i = 0; i < n; ++i)
    {
        const b2Vec2 a = v[i];
        const b2Vec2 b = v[(i + 1) % n];
        r.DrawLine(METERS_TO_PIXELS(a.x), METERS_TO_PIXELS(a.y),
            METERS_TO_PIXELS(b.x), METERS_TO_PIXELS(b.y), 255, 255, 100);
    }
}

void Physics::DrawSolidPolygonCb(b2Transform xf, const b2Vec2* v, int n, float radius, b2HexColor color, void* ctx)
{
    std::vector<b2Vec2> world(n);
    for (int i = 0; i < n; ++i) world[i] = b2TransformPoint(xf, v[i]);
    DrawPolygonCb(world.data(), n, color, ctx);
}

void Physics::DrawCircleCb(b2Vec2 center, float radius, b2HexColor color, void* ctx)
{
    auto& r = *Engine::GetInstance().render.get();
    r.DrawCircle(METERS_TO_PIXELS(center.x), METERS_TO_PIXELS(center.y),
        METERS_TO_PIXELS(radius) * Engine::GetInstance().window.get()->GetScale(), 255, 255, 255);
}

void Physics::DrawSolidCircleCb(b2Transform xf, float radius, b2HexColor color, void* ctx)
{
    DrawCircleCb(xf.p, radius, color, ctx);
}

void Physics::DrawSolidCapsuleStub(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* ctx)
{
    auto& r = *Engine::GetInstance().render.get();
    int scale = Engine::GetInstance().window.get()->GetScale();
    int radiusPx = METERS_TO_PIXELS(radius) * scale;
    r.DrawCircle(METERS_TO_PIXELS(p1.x), METERS_TO_PIXELS(p1.y), radiusPx, 0, 255, 0);
    r.DrawCircle(METERS_TO_PIXELS(p2.x), METERS_TO_PIXELS(p2.y), radiusPx, 0, 255, 0);
    float segX = p2.x - p1.x;
    float segY = p2.y - p1.y;
    float len = sqrtf(segX * segX + segY * segY);
    float dx = (len > 0.0001f) ? (-segY / len) * radius : radius;
    float dy = (len > 0.0001f) ? (segX / len) * radius : 0.0f;
    r.DrawLine(METERS_TO_PIXELS(p1.x + dx), METERS_TO_PIXELS(p1.y + dy), METERS_TO_PIXELS(p2.x + dx), METERS_TO_PIXELS(p2.y + dy), 0, 255, 0);
    r.DrawLine(METERS_TO_PIXELS(p1.x - dx), METERS_TO_PIXELS(p1.y - dy), METERS_TO_PIXELS(p2.x - dx), METERS_TO_PIXELS(p2.y - dy), 0, 255, 0);
}
void Physics::DrawPointStub(b2Vec2, float, b2HexColor, void*) {}
void Physics::DrawStringStub(b2Vec2, const char*, b2HexColor, void*) {}
void Physics::DrawTransformStub(b2Transform, void*) {}
