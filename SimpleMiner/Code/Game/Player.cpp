#include "Player.hpp"

#include "GameCommon.hpp"
#include "Game.hpp"
#include "Scene.hpp"
#include "World.hpp"
#include "Actor.hpp"
#include "ActorDefinition.hpp"
#include "BlockDef.hpp"
#include "ChunkProvider.hpp"

#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/DebugRender.hpp"

constexpr int PLAYER_INVENTORY_SIZE = 8;
unsigned char PLAYER_INVENTORY[PLAYER_INVENTORY_SIZE];

Player::Player(int index, bool keyboard, const XboxController* controller)
	: m_index(index)
	, m_keyboard(keyboard)
	, m_controller(controller)
{
	m_transform.m_position.x = 10.0f;
	m_transform.m_position.y = 10.0f;
	m_transform.m_position.z = 90.0f;
}

void Player::Possess(Actor* actor)
{
	Controller::Possess(actor);

	actor->m_physics->SetMode(PhysicsMode::FLYING);
}

void Player::Update(float deltaSeconds)
{
	PLAYER_INVENTORY[0] = Blocks::BLOCK_DIRT;
	PLAYER_INVENTORY[1] = Blocks::BLOCK_GRASS;
	PLAYER_INVENTORY[2] = Blocks::BLOCK_STONE;
	PLAYER_INVENTORY[3] = Blocks::BLOCK_LOG;
	PLAYER_INVENTORY[4] = Blocks::BLOCK_RED_BRICK;
	PLAYER_INVENTORY[5] = Blocks::BLOCK_COBBLESTONE;
	PLAYER_INVENTORY[6] = Blocks::BLOCK_GLOWSTONE;
	PLAYER_INVENTORY[7] = Blocks::BLOCK_WATER;

	if (m_skipFrame)
	{
		m_skipFrame = false;
		return;
	}

	Vec3 forward, up, left;
	m_transform.m_orientation.GetVectors_XFwd_YLeft_ZUp(forward, left, up);
	g_theAudio->UpdateListener(m_index,m_transform.m_position, forward, up);

	HandleInput(deltaSeconds);

	g_theGame->GetCurrentMap()->GetChunkManager()->SetHotspot(m_index, GetEyePosition());

	HandleDebugRender(deltaSeconds);
}

void Player::ChangeCameraState()
{
	Transformation prevCamera = GetCameraTransform();

	m_cameraState = (CameraState)((int)m_cameraState + 1);
	if (m_cameraState == CameraState::SIZE)
		m_cameraState = CameraState(0);

	switch (m_cameraState)
	{
	case CameraState::FIRST_PERSON:
	case CameraState::THIRD_PERSON:
	case CameraState::FIX_ANGLE:
		m_moveActor = true;
		m_freefly = false;
		break;
	case CameraState::UNPOSSESED:
		m_moveActor = true;
		m_freefly = true;
		m_transform = prevCamera;
		break;
	case CameraState::FREEFLY:
		m_moveActor = false;
		m_freefly = true;
		break;
	default:
		m_moveActor = false;
		m_freefly = true;
		break;
	}
}

Vec3 Player::GetEyePosition() const
{
	Actor* owner = GetActor();

	if (owner && !m_freefly)
	{
		return owner->GetEyePosition();
	}
	else
	{
		return m_transform.m_position;
	}
}

EulerAngles Player::GetOrientation() const
{
	Actor* owner = GetActor();

	if (owner && !m_freefly)
	{
		return owner->m_transform.m_orientation;
	}
	else
	{
		return m_transform.m_orientation;
	}
}

float Player::GetCameraFOV() const
{
	Actor* actor = GetActor();
	return actor ? actor->m_definition->m_cameraFOVDegrees : 60.0f;
}

Transformation Player::GetCameraTransform() const
{
	switch (m_cameraState)
	{
	case CameraState::FIRST_PERSON:
	{
		Transformation transform;
		transform.m_position = GetEyePosition();
		transform.m_orientation = GetOrientation();
		return transform;
	}
	case CameraState::THIRD_PERSON:
	{
		Transformation transform;
		transform.m_position = GetEyePosition();
		transform.m_orientation = GetOrientation();

		Vec3 forward = transform.GetForward();
		Actor* actor = GetActor();
		if (actor)
		{
			auto result = actor->m_world->FastRaycastVsTiles(transform.m_position, transform.m_position - forward * 4.0f);
			if (result.m_result.DidImpact())
			{
				transform.m_position -= forward * result.m_result.GetImpactDistance();
				return transform;
			}
		}

		transform.m_position -= forward * 4.0f;
		return transform;
	}
	case CameraState::FIX_ANGLE:
	{
		Transformation transform;
		transform.m_position = GetEyePosition();
		transform.m_orientation = EulerAngles(40.0f, 30.0f, 0.0f);
		transform.m_position -= transform.GetForward() * 10.0f;
		return transform;
	}
	case CameraState::UNPOSSESED:
	{
		Transformation transform;
		transform.m_position = GetEyePosition();
		transform.m_orientation = GetOrientation();
		return transform;
	}
	case CameraState::FREEFLY:
	{
		Transformation transform;
		transform.m_position = GetEyePosition();
		transform.m_orientation = GetOrientation();
		return transform;
	}
	default:
	{
		Transformation transform;
		transform.m_position = GetEyePosition();
		transform.m_orientation = GetOrientation();
		return transform;
	}
	}
}

void Player::TryPosses()
{
	Actor* owner = GetActor();

	WorldRaycastResult result = g_theGame->GetCurrentMap()->DoRaycast(GetEyePosition(), GetOrientation().GetVectorXForward(), 10.0f, owner);
	g_theGame->GetCurrentMap()->AddRaycast(GetEyePosition(), GetOrientation().GetVectorXForward(), 10.0f, owner);

	if (result.m_hitActor)
	{
		Actor* toPosses = *result.m_actor;
		if (toPosses->m_definition->m_canBePossessed)
		{
			Possess(toPosses);
			SetFreeFlyMode(false);
		}
	}
}

void Player::TryJump() const
{
	Actor* actor = GetActor();
	if (actor && actor->m_physics)
	{
		if (!actor->m_physics->IsFlying())
		{
			if (actor->m_physics->IsOnGround())
				actor->m_physics->AddImpulse(Vec3(0.0f, 0.0f, 40.0f));
			else
				actor->m_physics->SwitchMode();
		}
	}
}

bool Player::RenderOwner() const
{
	return m_cameraState != CameraState::FIRST_PERSON;
}

void Player::SetFreeFlyMode(bool mode)
{
	if (m_freefly != mode)
	{
		if (mode)
			m_transform = GetCameraTransform();
		m_freefly = mode;
	}
}

bool Player::IsInWater() const
{
	World* world = g_theGame->GetCurrentMap();
	return world ? world->IsInWater(GetEyePosition()) : false;
}

void Player::HandleDebugRender(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	// Debug screen info
	Vec2 screenPos = g_theGame->GetCurrentScene()->GetScreenCamera().GetOrthoTopRight();
	std::string text;
	const Clock* clock;
	screenPos.x -= 3.0f;
	screenPos.y -= 3.0f;
	clock = g_theGame->GetCurrentScene()->GetClock();
	text = Stringf("Game         | Time: %.1f, FPS: %.1f, Dilation: %.1f", clock->GetTotalTime(), 1.0f / clock->GetDeltaTime() * clock->GetTimeDilation(), clock->GetTimeDilation());
	DebugAddScreenText(text, screenPos, 0.0f, Vec2(1.0f, 1.0f), 15.0f, Rgba8::WHITE, Rgba8::WHITE);
	text = Stringf("Current Inventory: %s (MOUSE WHEEL TO CHANGE)", Block(PLAYER_INVENTORY[m_inventoryIndex]).GetBlockDef()->m_name.c_str());
	DebugAddMessage(text, 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	Actor* actor = GetActor();

	if (actor)
	{
		// Debug raycast in world
		Vec3 position = actor->GetEyePosition();
		Vec3 forward = actor->m_transform.GetForward();

		auto result = g_theGame->GetCurrentMap()->DoRaycast(position, forward, 10.0f, nullptr);
		if (result.m_result.DidImpact())
		{
			DebugAddWorldRay(position, forward, 10.0f, result.m_result, 0.025f, 0.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::XRAY);
		}

		if (actor->m_physics)
		{
			text = Stringf("Walking=%s, Flying=%s, NoClip=%s, OnGround=%s, Camera=%s", actor->m_physics->IsFlying() ? "false" : "true", actor->m_physics->IsFlying() ? "true" : "false", actor->m_physics->IsNoclip() ? "true" : "false", actor->m_physics->IsOnGround() ? "true" : "false", GetName(m_cameraState));
			DebugAddMessage(text, 0.0f, Rgba8::WHITE, Rgba8::WHITE);
		}
	}
}

void Player::HandleMoveFromKeyboard(float deltaSeconds)
{
	Actor* owner = GetActor();

	Vec3* position;
	EulerAngles* orientation;
	if (owner && m_moveActor)
	{
		position = &owner->m_transform.m_position;
		orientation = &owner->m_transform.m_orientation;
	}
	else
	{
		position = &m_transform.m_position;
		orientation = &m_transform.m_orientation;
	}

	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;

	deltaSeconds *= 4.5f;

	constexpr float rotateSpeed = 0.25f;

	EulerAngles(orientation->m_yawDegrees, 0, 0).GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);

	orientation->m_yawDegrees -= g_theInput->GetMouseClientDelta().x * rotateSpeed;
	orientation->m_pitchDegrees += g_theInput->GetMouseClientDelta().y * rotateSpeed;
	orientation->m_pitchDegrees = Clamp(orientation->m_pitchDegrees, -85.0f, 85.0f);
	orientation->m_rollDegrees = Clamp(orientation->m_rollDegrees, -45.0f, 45.0f);

	// m_transform.m_orientation = *orientation;

	if (g_theInput->IsKeyDown(KEYCODE_LSHIFT))
	{
		deltaSeconds *= 10.0f;
	}

	if (!m_moveActor)
	{
		if (g_theInput->IsKeyDown('W'))
		{
			*position += iBasis * deltaSeconds;
		}
		if (g_theInput->IsKeyDown('S'))
		{
			*position -= iBasis * deltaSeconds;
		}
		if (g_theInput->IsKeyDown('A'))
		{
			*position += jBasis * deltaSeconds;
		}
		if (g_theInput->IsKeyDown('D'))
		{
			*position -= jBasis * deltaSeconds;
		}
		if (g_theInput->IsKeyDown('E'))
		{
			*position += kBasis * deltaSeconds;
		}
		if (g_theInput->IsKeyDown('Q'))
		{
			*position -= kBasis * deltaSeconds;
		}
	}
	else
	{
		Vec3 acceleration;
		if (g_theInput->IsKeyDown('W'))
		{
			acceleration += iBasis;
		}
		if (g_theInput->IsKeyDown('S'))
		{
			acceleration -= iBasis;
		}
		if (g_theInput->IsKeyDown('A'))
		{
			acceleration += jBasis;
		}
		if (g_theInput->IsKeyDown('D'))
		{
			acceleration -= jBasis;
		}
		if (owner && owner->m_physics && !owner->m_physics->IsFlying())
		{
			if (g_theInput->WasKeyJustPressed(' '))
			{
				TryJump();
			}
		}
		else
		{
			if (g_theInput->IsKeyDown('E') || g_theInput->IsKeyDown(' '))
			{
				acceleration += kBasis;
			}
			if (g_theInput->IsKeyDown('Q') || g_theInput->IsKeyDown(KEYCODE_LCTRL))
			{
				acceleration -= kBasis;

				if (!owner->m_physics->IsNoclip() && owner->m_physics->IsOnGround())
				{
					owner->m_physics->SwitchMode();
					owner->m_physics->SwitchMode();
				}
			}
		}
		owner->m_physics->AddForce(acceleration * (g_theInput->IsKeyDown(KEYCODE_LSHIFT) ? owner->m_definition->m_runSpeed : owner->m_definition->m_walkSpeed) * owner->m_definition->m_drag);
	}

}

void Player::HandleMoveFromController(float deltaSeconds)
{
	auto& controller = *m_controller;
	Actor* owner = GetActor();

	Vec3* position;
	EulerAngles* orientation;
	if (owner && m_moveActor)
	{
		position = &owner->m_transform.m_position;
		orientation = &owner->m_transform.m_orientation;
	}
	else
	{
		position = &m_transform.m_position;
		orientation = &m_transform.m_orientation;
	}

	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;

	deltaSeconds *= 2.5f;

	EulerAngles(orientation->m_yawDegrees, 0, 0).GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);

	orientation->m_yawDegrees -= controller.GetRightJoystick().GetPositionCorrected().x;
	m_transform.m_orientation.m_pitchDegrees += controller.GetRightJoystick().GetPositionCorrected().y;

	m_transform.m_orientation.m_yawDegrees = orientation->m_yawDegrees;
	m_transform.m_orientation.m_pitchDegrees = Clamp(m_transform.m_orientation.m_pitchDegrees, -85.0f, 85.0f);
	m_transform.m_orientation.m_rollDegrees = Clamp(m_transform.m_orientation.m_rollDegrees, -45.0f, 45.0f);

	if (controller.IsButtonDown(XboxButtonID::XBOXBTN_A))
	{
		deltaSeconds *= 10.0f;
	}

	*position += iBasis * deltaSeconds * controller.GetLeftJoystick().GetPositionCorrected().y;
	*position += -jBasis * deltaSeconds * controller.GetLeftJoystick().GetPositionCorrected().x;
	if (controller.IsButtonDown(XboxButtonID::XBOXBTN_LSHOULDER))
	{
		*position -= kBasis * deltaSeconds;
	}
	if (controller.IsButtonDown(XboxButtonID::XBOXBTN_RSHOULDER))
	{
		*position += kBasis * deltaSeconds;
	}
}

void Player::HandleInteractFromKeyboard(float deltaSeconds)
{
	if (m_inventoryIndex == -1)
		m_inventoryIndex = int(PLAYER_INVENTORY_SIZE) - 1;

	UNUSED(deltaSeconds);

	if (g_theInput->GetMouseWheel() > 0)
	{
		m_inventoryIndex += 1;
		m_inventoryIndex %= PLAYER_INVENTORY_SIZE;
	}
	if (g_theInput->GetMouseWheel() < 0)
	{
		m_inventoryIndex += int(PLAYER_INVENTORY_SIZE) - 1;
		m_inventoryIndex %= PLAYER_INVENTORY_SIZE;
	}

	Actor* actor = GetActor();

	if (actor)
	{
		Vec3 position = actor->GetEyePosition();
		Vec3 forward = actor->m_transform.GetForward();

		if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
		{
			auto* world = g_theGame->GetCurrentMap();
			auto result = g_theGame->GetCurrentMap()->DoRaycast(position, forward, 10.0f, nullptr);
			if (result.m_hitBlock)
			{
				world->GetChunkManager()->SetBlockId(result.m_blockIte.GetWorldCoords(), Blocks::BLOCK_AIR);
				const char* text = "Touched block %d,%d,%d(Chunk %d,%d Block %d,%d,%d)";
				WorldCoords w = result.m_blockIte.GetWorldCoords();
				LocalCoords l = Chunk::GetLocalCoords(w);
				ChunkCoords c = Chunk::GetChunkCoords(w);
				DebugAddMessage(Stringf(text, w.x, w.y, w.z, c.x, c.y, l.x, l.y, l.z), 5.0f);
			}
		}
		if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
		{
			auto* world = g_theGame->GetCurrentMap();
			auto result = g_theGame->GetCurrentMap()->DoRaycast(position, forward, 10.0f, nullptr);
			if (result.m_hitBlock)
			{
				world->GetChunkManager()->SetBlockId(result.m_blockIte.GetWorldCoords() + IntVec3(result.m_result.GetImpactNormal()), PLAYER_INVENTORY[m_inventoryIndex]);
				const char* text = "Touched block %d,%d,%d(Chunk %d,%d Block %d,%d,%d)";
				WorldCoords w = result.m_blockIte.GetWorldCoords() + IntVec3(result.m_result.GetImpactNormal());
				LocalCoords l = Chunk::GetLocalCoords(w);
				ChunkCoords c = Chunk::GetChunkCoords(w);
				DebugAddMessage(Stringf(text, w.x, w.y, w.z, c.x, c.y, l.x, l.y, l.z), 5.0f);
			}
		}
	}
}

void Player::HandleInteractFromController(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	auto& controller = *m_controller;

	// TODO
	UNUSED(controller);
}

void Player::HandleControlFromKeyboard(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	// switch camera
	if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
		ChangeCameraState();

	Actor* actor = GetActor();
	if (actor)
	{
		if (g_theInput->WasKeyJustPressed(KEYCODE_F3))
			actor->m_physics->SwitchMode();
	}

// 	// click posses
// 	if (g_theInput->WasKeyJustPressed(KEYCODE_M))
// 		TryPosses();
}

void Player::HandleInput(float deltaSeconds)
{
// 	if (m_controller && m_controller->IsConnected())
// 	{
// 		HandleMoveFromController(deltaSeconds);
// 		HandleInteractFromController(deltaSeconds);
// 	}
	if (m_keyboard)
	{
		HandleMoveFromKeyboard(deltaSeconds);
		HandleInteractFromKeyboard(deltaSeconds);
	}

	HandleControlFromKeyboard(deltaSeconds);
}

const char* GetName(CameraState enumCameraState)
{
	static const char* names[(int)CameraState::SIZE] = { "FIRST_PERSON", "THIRD_PERSON", "FIX_ANGLE", "UNPOSSESED", "FREEFLY" };
	return names[(int)enumCameraState];
}
