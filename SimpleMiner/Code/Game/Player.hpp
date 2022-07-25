#include "Controller.hpp"

#include "Components.hpp"

class XboxController;

enum class CameraState
{
	FIRST_PERSON,
	THIRD_PERSON,
	FIX_ANGLE,
	UNPOSSESED,
	FREEFLY,
	SIZE,
};

const char* GetName(CameraState enumCameraState);

class Player : public Controller
{
public:
	Player(int index, bool keyboard, const XboxController* controller);

	virtual void Possess(Actor* actor) override;


	void Update(float deltaSeconds) override;

	void ChangeCameraState();

	Vec3 GetEyePosition() const;
	EulerAngles GetOrientation() const;
	float GetCameraFOV() const;
	Transformation GetCameraTransform() const;

	void TryPosses();
	void TryJump() const;

	bool RenderOwner() const;
	void SetFreeFlyMode(bool mode);

	bool IsInWater() const;

private:
	void HandleInput(float deltaSeconds);
	void HandleMoveFromKeyboard(float deltaSeconds);
	void HandleMoveFromController(float deltaSeconds);
	void HandleInteractFromKeyboard(float deltaSeconds);
	void HandleInteractFromController(float deltaSeconds);
	void HandleControlFromKeyboard(float deltaSeconds);
	void HandleDebugRender(float deltaSeconds);

private:
	int                   m_index = 0;
	bool                  m_keyboard = false;
	const XboxController* m_controller = nullptr;
	bool                  m_freefly = false;
	bool                  m_moveActor = true;
	Transformation        m_transform;
	int                   m_inventoryIndex = -1;
	CameraState           m_cameraState = CameraState(0);

};
