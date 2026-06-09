import re

with open('src/BlockCrawler.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

boxes_data = """
namespace {
    struct FrameBox {
        int w, h, bottomGap;
    };
    const FrameBox moveBoxes[15] = {
        { 49, 43, 0 }, { 39, 34, 0 }, { 43, 37, 0 }, { 48, 40, 0 }, { 50, 41, 0 },
        { 49, 41, 0 }, { 42, 39, 0 }, { 36, 36, 0 }, { 41, 40, 0 }, { 47, 43, 0 },
        { 49, 44, 0 }, { 43, 41, 0 }, { 39, 37, 2 }, { 38, 36, 2 }, { 45, 40, 0 }
    };
    const FrameBox wallBoxes[11] = {
        { 45, 33, 0 }, { 45, 33, 0 }, { 45, 34, 0 }, { 46, 40, 0 }, { 46, 57, 0 },
        { 47, 94, 0 }, { 48, 130, 0 }, { 49, 147, 0 }, { 49, 153, 0 }, { 49, 155, 0 }, { 49, 155, 0 }
    };
    const FrameBox hitBoxes[7] = {
        { 49, 155, 0 }, { 49, 154, 0 }, { 49, 154, 0 }, { 48, 154, 0 }, { 48, 154, 0 }, { 48, 154, 0 }, { 49, 155, 0 }
    };
    const FrameBox dieBoxes[29] = {
        { 49, 155, 0 }, { 48, 134, 0 }, { 47, 114, 0 }, { 47, 94, 0 }, { 46, 77, 0 },
        { 46, 66, 0 }, { 46, 60, 0 }, { 46, 54, 0 }, { 47, 49, 0 }, { 45, 43, 0 },
        { 45, 38, 0 }, { 45, 35, 0 }, { 45, 33, 0 }, { 45, 33, 0 }, { 45, 33, 0 },
        { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 },
        { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 },
        { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }, { 45, 32, 0 }
    };
}
"""
code = code.replace('#include "Physics.h"', '#include "Physics.h"\n' + boxes_data)
code = code.replace('UpdatePhysicsBody(false);', 'currentFrameIndex_ = -1;\n\tUpdatePhysicsForCurrentFrame();')

update_physics_body = """void BlockCrawler::UpdatePhysicsForCurrentFrame()
{
	int frameIdx = 0;
	const FrameBox* box = nullptr;

	switch (currentState_) {
		case State::MOVE: frameIdx = moveAnims_.GetCurrentFrameIndex(); if(frameIdx >= 0 && frameIdx < 15) box = &moveBoxes[frameIdx]; break;
		case State::WALL: frameIdx = wallAnims_.GetCurrentFrameIndex(); if(frameIdx >= 0 && frameIdx < 11) box = &wallBoxes[frameIdx]; break;
		case State::HIT:  frameIdx = hitAnims_.GetCurrentFrameIndex();  if(frameIdx >= 0 && frameIdx < 7)  box = &hitBoxes[frameIdx];  break;
		case State::DEATH:frameIdx = dieAnims_.GetCurrentFrameIndex();  if(frameIdx >= 0 && frameIdx < 29) box = &dieBoxes[frameIdx];  break;
	}

	if (!box) return;

	if (frameIdx == currentFrameIndex_) return;
	currentFrameIndex_ = frameIdx;

	float renderScale = 0.6f;
	int newPhysW = (int)(box->w * renderScale);
	int newPhysH = (int)(box->h * renderScale);
	if (newPhysW <= 0) newPhysW = 10;
	if (newPhysH <= 0) newPhysH = 10;

	int bx = (int)position.getX();
	int by = (int)position.getY();

	if (pbody != nullptr) {
		pbody->GetPosition(bx, by);
		int floorY = by + currentPhysHalfH_;
		by = floorY - (newPhysH / 2);
		
		b2Vec2 vel = Engine::GetInstance().physics->GetLinearVelocity(pbody);
		Engine::GetInstance().physics->DeletePhysBody(pbody);
		pbody = Engine::GetInstance().physics->CreateRectangle(bx, by, newPhysW, newPhysH, DYNAMIC);
		Engine::GetInstance().physics->SetLinearVelocity(pbody, vel);
	} else {
		pbody = Engine::GetInstance().physics->CreateRectangle(bx, by, newPhysW, newPhysH, DYNAMIC);
	}

	currentPhysHalfH_ = newPhysH / 2;
	pbody->listener = this;
	pbody->ctype = (currentState_ == State::DEATH) ? ColliderType::UNKNOWN : ColliderType::ENEMY;
}"""

code = re.sub(r'void BlockCrawler::UpdatePhysicsBody\(bool isWall\).*?ctype = ColliderType::ENEMY;\n}', update_physics_body, code, flags=re.DOTALL)

code = code.replace('Draw(dt);', 'Draw();')

fsm_cases = {
    'case State::MOVE: {': 'case State::MOVE: {\n\t\t\tmoveAnims_.Update(dt);',
    'case State::WALL: {': 'case State::WALL: {\n\t\t\twallAnims_.Update(dt);',
    'case State::HIT: {': 'case State::HIT: {\n\t\t\thitAnims_.Update(dt);',
    'case State::DEATH: {': 'case State::DEATH: {\n\t\t\tdieAnims_.Update(dt);'
}
for k, v in fsm_cases.items():
    code = code.replace(k, v)

code = code.replace('\t}\n}\n\nvoid BlockCrawler::EnterState(State newState)', '\t}\n\tUpdatePhysicsForCurrentFrame();\n}\n\nvoid BlockCrawler::EnterState(State newState)')

enter_state = """void BlockCrawler::EnterState(State newState)
{
	currentState_ = newState;
	stateTimer_ = 0.0f;
	currentFrameIndex_ = -1;

	switch (currentState_) {
		case State::MOVE:
			moveAnims_.SetCurrent("move");
			break;
		case State::WALL:
			wallAnims_.SetCurrent("wall");
			wallAnims_.ResetCurrent();
			break;
		case State::HIT:
			hitAnims_.SetCurrent("hit");
			hitAnims_.ResetCurrent();
			break;
		case State::DEATH:
			if (pbody != nullptr) {
				pbody->ctype = ColliderType::UNKNOWN;
			}
			dieAnims_.SetCurrent("die");
			dieAnims_.ResetCurrent();
			break;
	}
	UpdatePhysicsForCurrentFrame();
}"""
code = re.sub(r'void BlockCrawler::EnterState\(State newState\).*?break;\n\t}\n}', enter_state, code, flags=re.DOTALL)

code = code.replace('void BlockCrawler::Draw(float dt)', 'void BlockCrawler::Draw()')
code = code.replace('\t\t\tmoveAnims_.Update(dt);\n', '')
code = code.replace('\t\t\twallAnims_.Update(dt);\n', '')
code = code.replace('\t\t\thitAnims_.Update(dt);\n', '')
code = code.replace('\t\t\tdieAnims_.Update(dt);\n', '')

draw_replace = """		// Align bottom of sprite to bottom of physics body
		int spriteBottomOffset = 15; // Visual tweak in case the sprite has transparent padding at the bottom
		
		int ry = (int)(position.getY() + currentPhysHalfH_ - renderH) + spriteBottomOffset;"""
code = re.sub(r'\t\t// Align bottom of sprite to bottom of physics body.*?\+ spriteBottomOffset;', draw_replace, code, flags=re.DOTALL)

with open('src/BlockCrawler.cpp', 'w', encoding='utf-8') as f:
    f.write(code)
