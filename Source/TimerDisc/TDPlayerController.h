#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TDPlayerController.generated.h"

/** Polls keyboard / mouse / gamepad directly (no input assets needed). */
UCLASS()
class ATDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATDPlayerController();
	void BeginPlay() override;
	void PlayerTick(float DeltaTime) override;

private:
	bool Pressed(const FKey& Key) const { return WasInputKeyJustPressed(Key); }
	bool Released(const FKey& Key) const { return WasInputKeyJustReleased(Key); }
	bool Down(const FKey& Key) const { return IsInputKeyDown(Key); }
};
