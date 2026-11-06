#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TDPlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;

/** The thrower. No walking: after every throw you are placed where your disc landed. */
UCLASS()
class ATDPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	ATDPlayer();
	void BeginPlay() override;

	void PlaceAt(const FVector& Ground, float Yaw);
	void PlayThrow();
	void PlayCheer();
	FVector ReleasePoint(float Yaw) const;
	void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<USpringArmComponent> Boom;
	UPROPERTY() TObjectPtr<UCameraComponent> Camera;
	float BackToIdle = 0.f;
	void PlayOnce(class UAnimSequence* Seq, float Rate);
};
