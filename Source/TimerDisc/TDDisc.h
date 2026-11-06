#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDDisc.generated.h"

class USceneComponent;
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;

/** A clock you throw like a disc. Also carries the camera that follows it in flight. */
UCLASS()
class ATDDisc : public AActor
{
	GENERATED_BODY()

public:
	ATDDisc();
	void Tick(float DeltaSeconds) override;

	void SetKind(int32 Kind);
	int32 GetKind() const { return Kind; }

	/** Flight pose: spinning flat, tilted by the curve. */
	void SetFlying(bool bInFlying, float InCurve = 0.f, float InYaw = 0.f);
	void SetCameraYaw(float Yaw);

private:
	UPROPERTY() TObjectPtr<USceneComponent> Spin;
	UPROPERTY() TObjectPtr<USceneComponent> HourPivot;
	UPROPERTY() TObjectPtr<USceneComponent> MinutePivot;
	UPROPERTY() TObjectPtr<USpringArmComponent> Boom;
	UPROPERTY() TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	int32 Kind = -1;
	bool bFlying = false;
	float Curve = 0.f;
	float CamYaw = 0.f;
	float Clock = 0.f;

	void Clear();
	UStaticMeshComponent* Add(USceneComponent* Parent, class UStaticMesh* Mesh, const FLinearColor& Color, const FVector& Loc, const FVector& SizeCm,
		const FRotator& Rot = FRotator::ZeroRotator, bool bGlow = false);
};
