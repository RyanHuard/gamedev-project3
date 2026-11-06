#include "TDPlayerController.h"

#include "TDGameMode.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Misc/App.h"

ATDPlayerController::ATDPlayerController()
{
	bShowMouseCursor = false;
}

void ATDPlayerController::BeginPlay()
{
	Super::BeginPlay();
	FInputModeGameOnly Mode;
	SetInputMode(Mode);
	bShowMouseCursor = false;
}

void ATDPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	ATDGameMode* GM = GetWorld()->GetAuthGameMode<ATDGameMode>();
	if (!GM)
	{
		return;
	}
	const float RealDt = (float)FApp::GetDeltaTime();

	if (Pressed(EKeys::Escape) || Pressed(EKeys::Gamepad_Special_Right))
	{
		GM->OnBack();
		return;
	}

	if (GM->GetState() != ETDState::Aim)
	{
		if (Pressed(EKeys::LeftMouseButton) || Pressed(EKeys::SpaceBar) || Pressed(EKeys::Enter) || Pressed(EKeys::Gamepad_FaceButton_Bottom))
		{
			GM->OnConfirm();
		}
		return;
	}

	// ---- Aim: mouse left/right turns, up/down changes the arc ----
	float MX = 0.f, MY = 0.f;
	GetInputMouseDelta(MX, MY);
	const float PadX = GetInputAnalogKeyState(EKeys::Gamepad_RightX);
	const float PadY = GetInputAnalogKeyState(EKeys::Gamepad_RightY);
	float KeyYaw = 0.f;
	if (Down(EKeys::Left)) KeyYaw -= 1.f;
	if (Down(EKeys::Right)) KeyYaw += 1.f;
	float KeyArc = 0.f;
	if (Down(EKeys::Up)) KeyArc += 1.f;
	if (Down(EKeys::Down)) KeyArc -= 1.f;
	GM->OnAimDelta(MX * 0.6f + (PadX + KeyYaw) * 40.f * RealDt, MY * 0.35f + (PadY + KeyArc) * 20.f * RealDt);

	// ---- Disc ----
	if (Pressed(EKeys::One)) GM->OnSelectDisc(0);
	if (Pressed(EKeys::Two)) GM->OnSelectDisc(1);
	if (Pressed(EKeys::Three)) GM->OnSelectDisc(2);
	if (Pressed(EKeys::MouseScrollUp) || Pressed(EKeys::Gamepad_RightShoulder)) GM->OnCycleDisc(1);
	if (Pressed(EKeys::MouseScrollDown) || Pressed(EKeys::Gamepad_LeftShoulder)) GM->OnCycleDisc(-1);

	// ---- Curve ----
	if (Pressed(EKeys::A) || Pressed(EKeys::Q) || Pressed(EKeys::Gamepad_DPad_Left)) GM->OnCurve(-1);
	if (Pressed(EKeys::D) || Pressed(EKeys::E) || Pressed(EKeys::Gamepad_DPad_Right)) GM->OnCurve(1);

	// ---- Throw: hold to charge, release to throw, right click cancels ----
	if (Pressed(EKeys::LeftMouseButton) || Pressed(EKeys::SpaceBar) || Pressed(EKeys::Gamepad_RightTrigger)) GM->OnChargeStart();
	if (Released(EKeys::LeftMouseButton) || Released(EKeys::SpaceBar) || Released(EKeys::Gamepad_RightTrigger)) GM->OnChargeRelease();
	if (Pressed(EKeys::RightMouseButton) || Pressed(EKeys::Gamepad_FaceButton_Right)) GM->OnChargeCancel();
	if (Pressed(EKeys::R)) GM->OnResetAim();

	// ---- Hold C (or TAB) to look at the whole hole ----
	GM->SetOverview(Down(EKeys::C) || Down(EKeys::Tab) || Down(EKeys::Gamepad_FaceButton_Top));
}
