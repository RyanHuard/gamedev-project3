#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDTypes.h"
#include "TDCourseActor.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

enum class ETDEnd : uint8
{
	Rest,    // came to rest on dry ground
	Basket,  // caught by the basket
	Water    // splashed into a pond (+1 stroke, rethrow from the previous lie)
};

struct FTDThrow
{
	FVector Start = FVector::ZeroVector;  // cm
	float Yaw = 0.f;                      // degrees
	float Arc = 12.f;                     // launch angle, degrees
	float Power = 1.f;                    // 0..1
	float Curve = 0.f;                    // -1 (left) .. +1 (right)
	int32 Disc = 1;
};

struct FTDFlight
{
	TArray<FVector> Points;  // one per Dt
	float Dt = 1.f / 60.f;
	int32 LandIndex = 0;     // first ground contact
	ETDEnd End = ETDEnd::Rest;
	FVector Rest = FVector::ZeroVector;
	bool bHitTree = false;
};

/** Builds one hole at a time (ground, hills, ponds, trees, tee, basket) and simulates disc flights on it. */
UCLASS()
class ATDCourseActor : public AActor
{
	GENERATED_BODY()

public:
	ATDCourseActor();
	void Tick(float DeltaSeconds) override;

	void BuildHole(const FTDHole& Hole, int32 Seed);

	/** Height of the ground (cm) under an XY point, ignoring trees. */
	float GroundZ(const FVector2D& XY) const;
	bool IsWater(const FVector2D& XY) const;
	ETDLie LieAt(const FVector2D& XY) const;
	FVector GetBasketBase() const { return BasketBase; }
	FVector GetTee() const { return TeeLoc; }
	float GetTeeYaw() const { return TeeYawDeg; }
	FVector GetFlagTop() const { return BasketBase + FVector(0.f, 0.f, 330.f); }

	/** Deterministic flight: the same throw always lands in the same place. */
	void Simulate(const FTDThrow& Throw, FTDFlight& Out, float Dt = 1.f / 60.f, float MaxTime = 9.f) const;

	/** Dotted preview of a flight plus a ring where it comes to rest. */
	void ShowPreview(const FTDFlight* Flight, bool bStrong);

	void Splash(const FVector& At);

private:
	FTDHole Hole;
	FVector BasketBase = FVector::ZeroVector;
	FVector TeeLoc = FVector::ZeroVector;

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> HoleParts;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> PreviewDots;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> PreviewRing;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> PreviewMat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RingMat;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Flag;

	/** Simple collision shapes for the flight (trees, the basket pole). Visual meshes have no collision. */
	struct FObstacle
	{
		bool bSphere = true;
		FVector Center = FVector::ZeroVector;  // sphere center, or cylinder base
		float Radius = 0.f;
		float Height = 0.f;                    // cylinders only
	};
	TArray<FObstacle> Obstacles;
	FVector2D TeeCenter = FVector2D::ZeroVector;
	float TeeYawDeg = 0.f;
	float TeeBaseZ = 0.f;
	bool bHasTeePad = false;

	bool HitsObstacle(const FVector& P, FVector& OutNormal) const;
	float DistToFairway(const FVector2D& Pm) const;

	struct FFx
	{
		TWeakObjectPtr<UStaticMeshComponent> C;
		float Life = 0.f;
		float MaxLife = 0.f;
		FVector BaseScale = FVector::OneVector;
	};
	TArray<FFx> Fx;
	float Clock = 0.f;
	bool bSkyBuilt = false;

	void BuildSky();
	UStaticMeshComponent* Part(UStaticMesh* Mesh, UMaterialInterface* Mat, const FVector& Center, const FVector& SizeCm,
		const FRotator& Rot = FRotator::ZeroRotator);
	void AddTree(const FVector2D& Pos, float Scale, uint32 Variant);
	void AddBasket(const FVector& Base);
	bool CageHit(const FVector& A, const FVector& B) const;
};
