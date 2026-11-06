#include "TDCourseActor.h"

#include "TDAssets.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FLinearColor Grass(0.075f, 0.19f, 0.045f);     // rough
	const FLinearColor FairwayA(0.17f, 0.40f, 0.09f);
	const FLinearColor FairwayB(0.14f, 0.34f, 0.075f);
	const FLinearColor GreenColor(0.22f, 0.50f, 0.12f);
	const FLinearColor Fringe(0.12f, 0.30f, 0.07f);
	const FLinearColor Water(0.03f, 0.13f, 0.22f);
	const FLinearColor Sand(0.78f, 0.68f, 0.46f);
	const FLinearColor Bark(0.22f, 0.14f, 0.08f);
	const FLinearColor Concrete(0.5f, 0.5f, 0.48f);
	const FLinearColor Steel(0.55f, 0.57f, 0.6f);
	const FLinearColor Yellow(1.f, 0.75f, 0.05f);
	constexpr float Gravity = 980.f;
	constexpr float BasketCatchRadius = 52.f;
	constexpr float CageLow = 45.f, CageHigh = 140.f;
	constexpr int32 MaxPreviewDots = 90;
}

ATDCourseActor::ATDCourseActor()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

// ---------------------------------------------------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------------------------------------------------

UStaticMeshComponent* ATDCourseActor::Part(UStaticMesh* Mesh, UMaterialInterface* Mat, const FVector& Center, const FVector& SizeCm, const FRotator& Rot)
{
	UStaticMeshComponent* C = TDAssets::AddMesh(this, RootComponent, Mesh, Mat, FTransform(Rot, Center, SizeCm / 100.f));
	HoleParts.Add(C);
	return C;
}

void ATDCourseActor::BuildSky()
{
	bSkyBuilt = true;

	UDirectionalLightComponent* Sun = NewObject<UDirectionalLightComponent>(this);
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetRelativeRotation(FRotator(-38.f, 140.f, 0.f));
	Sun->SetIntensity(7.f);
	Sun->SetLightColor(FLinearColor(1.f, 0.9f, 0.78f));
	Sun->SetAtmosphereSunLight(true);
	Sun->RegisterComponent();
	AddInstanceComponent(Sun);

	USkyAtmosphereComponent* Atmos = NewObject<USkyAtmosphereComponent>(this);
	Atmos->SetupAttachment(RootComponent);
	Atmos->RegisterComponent();
	AddInstanceComponent(Atmos);

	USkyLightComponent* Sky = NewObject<USkyLightComponent>(this);
	Sky->SetupAttachment(RootComponent);
	Sky->SetMobility(EComponentMobility::Movable);
	Sky->bRealTimeCapture = true;
	Sky->SetIntensity(1.f);
	Sky->RegisterComponent();
	AddInstanceComponent(Sky);

	UExponentialHeightFogComponent* Fog = NewObject<UExponentialHeightFogComponent>(this);
	Fog->SetupAttachment(RootComponent);
	Fog->SetFogDensity(0.008f);
	Fog->SetFogInscatteringColor(FLinearColor(0.45f, 0.6f, 0.8f));
	Fog->RegisterComponent();
	AddInstanceComponent(Fog);

	// Endless lawn
	TDAssets::AddMesh(this, RootComponent, TDAssets::Cube(), TDAssets::MakeColor(this, Grass, 0.95f),
		FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, -50.f), FVector(1600.f, 1600.f, 1.f)));

	// Flight preview
	PreviewMat = TDAssets::MakeGlow(this, FLinearColor(2.f, 1.8f, 1.2f), 0.85f);
	PreviewDots = NewObject<UInstancedStaticMeshComponent>(this, TEXT("PreviewDots"));
	PreviewDots->SetStaticMesh(TDAssets::Sphere());
	PreviewDots->SetMaterial(0, PreviewMat);
	PreviewDots->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewDots->SetCastShadow(false);
	PreviewDots->SetupAttachment(RootComponent);
	PreviewDots->SetUsingAbsoluteLocation(true);
	PreviewDots->RegisterComponent();
	AddInstanceComponent(PreviewDots);
	RingMat = TDAssets::MakeGlow(this, FLinearColor(1.f, 0.8f, 0.2f), 0.6f);
	PreviewRing = TDAssets::AddMesh(this, RootComponent, TDAssets::Cylinder(), RingMat, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.6f, 1.6f, 0.02f)));
	PreviewRing->SetUsingAbsoluteLocation(true);
	PreviewRing->SetCastShadow(false);
	PreviewRing->SetVisibility(false);
}

void ATDCourseActor::AddTree(const FVector2D& Pos, float Scale, uint32 Variant)
{
	const float Z = GroundZ(Pos * TD::M);
	const FVector Base(Pos.X * TD::M, Pos.Y * TD::M, Z);
	Part(TDAssets::Cylinder(), TDAssets::MakeColor(this, Bark, 0.9f), Base + FVector(0.f, 0.f, 200.f * Scale), FVector(60.f, 60.f, 400.f) * Scale);
	Obstacles.Add({ false, Base, 30.f * Scale, 400.f * Scale });
	// Two or three overlapping leaf balls; slight color variety so the woods don't look copy-pasted.
	const float Shade = 0.85f + 0.3f * ((Variant % 7) / 6.f);
	UMaterialInstanceDynamic* Leaves = TDAssets::MakeColor(this, FLinearColor(0.07f * Shade, 0.24f * Shade, 0.06f * Shade), 0.9f);
	const FVector Offsets[3] = { FVector(0.f, 0.f, 470.f), FVector(70.f, -50.f, 380.f), FVector(-60.f, 60.f, 400.f) };
	const float Sizes[3] = { 440.f, 340.f, 320.f };
	const int32 Count = 2 + (Variant % 2);
	for (int32 i = 0; i < Count; ++i)
	{
		Part(TDAssets::Sphere(), Leaves, Base + Offsets[i] * Scale, FVector(Sizes[i]) * Scale);
		// The collision ball is 85% of the visible leaves, so near misses feel fair.
		Obstacles.Add({ true, Base + Offsets[i] * Scale, Sizes[i] * 0.5f * 0.85f * Scale, 0.f });
	}
}

void ATDCourseActor::AddBasket(const FVector& Base)
{
	UMaterialInstanceDynamic* Metal = TDAssets::MakeColor(this, Steel, 0.3f);
	UMaterialInstanceDynamic* Band = TDAssets::MakeColor(this, Yellow, 0.4f);
	UMaterialInstanceDynamic* Chain = TDAssets::MakeColor(this, FLinearColor(0.75f, 0.76f, 0.8f), 0.25f);
	Part(TDAssets::Cylinder(), Metal, Base + FVector(0.f, 0.f, 75.f), FVector(6.f, 6.f, 150.f));
	Obstacles.Add({ false, Base, 4.f, 150.f });
	Part(TDAssets::Cylinder(), Metal, Base + FVector(0.f, 0.f, 68.f), FVector(110.f, 110.f, 22.f));      // tray
	Part(TDAssets::Cylinder(), Band, Base + FVector(0.f, 0.f, 80.f), FVector(114.f, 114.f, 5.f));        // tray lip
	Part(TDAssets::Cylinder(), Band, Base + FVector(0.f, 0.f, 136.f), FVector(116.f, 116.f, 7.f));       // top band
	for (int32 i = 0; i < 12; ++i)
	{
		const float A = i * 30.f;
		const FVector Off = FRotator(0.f, A, 0.f).Vector() * 32.f;
		Part(TDAssets::Cylinder(), Chain, Base + Off + FVector(0.f, 0.f, 108.f), FVector(1.5f, 1.5f, 50.f));
	}
	// Tall flag so the basket can be spotted from the tee.
	Part(TDAssets::Cylinder(), Metal, Base + FVector(0.f, 0.f, 230.f), FVector(3.f, 3.f, 190.f));
	Flag = Part(TDAssets::Cube(), TDAssets::MakeGlow(this, FLinearColor(1.6f, 0.35f, 0.05f)), Base + FVector(0.f, 30.f, 300.f), FVector(4.f, 60.f, 40.f));
	Flag->SetCastShadow(false);
}

float ATDCourseActor::DistToFairway(const FVector2D& Pm) const
{
	float Best = TNumericLimits<float>::Max();
	for (int32 i = 0; i + 1 < Hole.Fairway.Num(); ++i)
	{
		const FVector2D C = FMath::ClosestPointOnSegment2D(Pm, Hole.Fairway[i], Hole.Fairway[i + 1]);
		Best = FMath::Min(Best, FVector2D::Distance(Pm, C));
	}
	return Best;
}

ETDLie ATDCourseActor::LieAt(const FVector2D& XY) const
{
	const FVector2D Pm = XY / TD::M;
	if (IsWater(XY))
	{
		return ETDLie::Water;
	}
	for (const FTDBunker& B : Hole.Bunkers)
	{
		if (((Pm - B.Center) / B.Radii).SizeSquared() < 1.f)
		{
			return ETDLie::Sand;
		}
	}
	if (FVector2D::Distance(Pm, Hole.Basket) < Hole.GreenRadius)
	{
		return ETDLie::Green;
	}
	if (bHasTeePad)
	{
		const FVector2D L = (XY - TeeCenter).GetRotated(-TeeYawDeg);
		if (FMath::Abs(L.X) < 450.f && FMath::Abs(L.Y) < 300.f)
		{
			return ETDLie::Tee;
		}
	}
	if (DistToFairway(Pm) < Hole.FairwayWidth * 0.5f)
	{
		return ETDLie::Fairway;
	}
	return ETDLie::Rough;
}

void ATDCourseActor::BuildHole(const FTDHole& InHole, int32 Seed)
{
	if (!bSkyBuilt)
	{
		BuildSky();
	}
	for (UStaticMeshComponent* C : HoleParts)
	{
		if (C)
		{
			C->DestroyComponent();
		}
	}
	HoleParts.Reset();
	Obstacles.Reset();
	Hole = InHole;
	bHasTeePad = false;
	if (Hole.Fairway.Num() < 2)
	{
		Hole.Fairway = { Hole.Tee, Hole.Basket };
	}

	UMaterialInstanceDynamic* StripeA = TDAssets::MakeColor(this, FairwayA, 0.9f);
	UMaterialInstanceDynamic* StripeB = TDAssets::MakeColor(this, FairwayB, 0.9f);
	UMaterialInstanceDynamic* GreenMat = TDAssets::MakeColor(this, GreenColor, 0.8f);
	UMaterialInstanceDynamic* FringeMat = TDAssets::MakeColor(this, Fringe, 0.9f);
	UMaterialInstanceDynamic* SandMat = TDAssets::MakeColor(this, Sand, 0.95f);
	const FVector2D TeeToGreen = Hole.Basket - Hole.Tee;
	const float TeeYaw = FMath::RadiansToDegrees(FMath::Atan2(Hole.Fairway[1].Y - Hole.Fairway[0].Y, Hole.Fairway[1].X - Hole.Fairway[0].X));

	// ---- Hills (a hill holding the basket is an elevated green; any other hill is fairway) ----
	for (const FTDHill& H : Hole.Hills)
	{
		const bool bGreenHill = FVector2D::Distance(H.Center, Hole.Basket) < H.Radius * 0.5f;
		Part(TDAssets::Sphere(), bGreenHill ? GreenMat : StripeA, TD::W(H.Center.X, H.Center.Y, 0.f),
			FVector(H.Radius * 2.f, H.Radius * 2.f, H.Height * 2.f) * TD::M);
	}

	// ---- Fairway: mown stripes along the center line, rounded at the corners ----
	const float W = Hole.FairwayWidth;
	for (int32 i = 0; i + 1 < Hole.Fairway.Num(); ++i)
	{
		const FVector2D A = Hole.Fairway[i], B = Hole.Fairway[i + 1];
		const FVector2D Dir = (B - A).GetSafeNormal();
		const float Len = FVector2D::Distance(A, B);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
		const int32 Strips = FMath::Max(1, FMath::RoundToInt(Len / 6.f));
		const float StripLen = Len / Strips;
		for (int32 k = 0; k < Strips; ++k)
		{
			const FVector2D Mid = A + Dir * (StripLen * (k + 0.5f));
			Part(TDAssets::Cube(), ((k + i) % 2) ? StripeB : StripeA, TD::W(Mid.X, Mid.Y, 0.01f), FVector(StripLen + 0.05f, W, 0.02f) * TD::M,
				FRotator(0.f, Yaw, 0.f))->SetCastShadow(false);
		}
	}
	for (const FVector2D& J : Hole.Fairway)
	{
		Part(TDAssets::Cylinder(), StripeA, TD::W(J.X, J.Y, 0.008f), FVector(W, W, 0.02f) * TD::M)->SetCastShadow(false);
	}

	// ---- Ponds: sandy shore, water, optional island green ----
	for (const FTDPond& Pd : Hole.Ponds)
	{
		Part(TDAssets::Cylinder(), SandMat, TD::W(Pd.Center.X, Pd.Center.Y, 0.02f),
			FVector((Pd.Radii.X + 1.2f) * 2.f, (Pd.Radii.Y + 1.2f) * 2.f, 0.02f) * TD::M)->SetCastShadow(false);
		Part(TDAssets::Cylinder(), TDAssets::MakeColor(this, Water, 0.04f), TD::W(Pd.Center.X, Pd.Center.Y, 0.04f),
			FVector(Pd.Radii.X * 2.f, Pd.Radii.Y * 2.f, 0.02f) * TD::M)->SetCastShadow(false);
		if (Pd.IslandRadius > 0.f)
		{
			Part(TDAssets::Cylinder(), SandMat, TD::W(Pd.IslandCenter.X, Pd.IslandCenter.Y, 0.1f),
				FVector((Pd.IslandRadius + 0.8f) * 2.f, (Pd.IslandRadius + 0.8f) * 2.f, 0.2f) * TD::M);
			Part(TDAssets::Cylinder(), GreenMat, TD::W(Pd.IslandCenter.X, Pd.IslandCenter.Y, 0.15f),
				FVector(Pd.IslandRadius * 2.f, Pd.IslandRadius * 2.f, 0.3f) * TD::M);
		}
	}

	// ---- Bunkers ----
	for (const FTDBunker& Bk : Hole.Bunkers)
	{
		Part(TDAssets::Cylinder(), FringeMat, TD::W(Bk.Center.X, Bk.Center.Y, 0.03f), FVector((Bk.Radii.X + 0.6f) * 2.f, (Bk.Radii.Y + 0.6f) * 2.f, 0.02f) * TD::M)
			->SetCastShadow(false);
		Part(TDAssets::Cylinder(), SandMat, TD::W(Bk.Center.X, Bk.Center.Y, 0.045f), FVector(Bk.Radii.X * 2.f, Bk.Radii.Y * 2.f, 0.02f) * TD::M)
			->SetCastShadow(false);
	}

	// ---- Tee box: mown rectangle, concrete pad, two tee markers ----
	const FVector2D TeeCm = Hole.Tee * TD::M;
	const float TeeZ = GroundZ(TeeCm);
	Part(TDAssets::Cube(), StripeB, FVector(TeeCm.X, TeeCm.Y, TeeZ + 2.f), FVector(900.f, 600.f, 3.f), FRotator(0.f, TeeYaw, 0.f))->SetCastShadow(false);
	Part(TDAssets::Cube(), TDAssets::MakeColor(this, Concrete, 0.8f), FVector(TeeCm.X, TeeCm.Y, TeeZ + 2.f), FVector(320.f, 170.f, 14.f), FRotator(0.f, TeeYaw, 0.f));
	for (float Side : { -1.f, 1.f })
	{
		const FVector2D M = TeeCm + FVector2D(220.f, Side * 240.f).GetRotated(TeeYaw);
		Part(TDAssets::Sphere(), TDAssets::MakeColor(this, FLinearColor(0.75f, 0.08f, 0.05f), 0.4f), FVector(M.X, M.Y, TeeZ + 6.f), FVector(13.f));
	}
	TeeCenter = TeeCm;
	TeeYawDeg = TeeYaw;
	TeeBaseZ = TeeZ;
	bHasTeePad = true;
	TeeLoc = FVector(TeeCm.X, TeeCm.Y, GroundZ(TeeCm));

	// ---- Green + basket ----
	const FVector2D BasketCm = Hole.Basket * TD::M;
	BasketBase = FVector(BasketCm.X, BasketCm.Y, GroundZ(BasketCm));
	const bool bFlatGreen = FMath::Abs(BasketBase.Z) < 5.f;
	if (bFlatGreen)
	{
		Part(TDAssets::Cylinder(), FringeMat, TD::W(Hole.Basket.X, Hole.Basket.Y, 0.02f), FVector((Hole.GreenRadius + 1.8f) * 2.f, (Hole.GreenRadius + 1.8f) * 2.f, 0.02f) * TD::M)
			->SetCastShadow(false);
		Part(TDAssets::Cylinder(), GreenMat, TD::W(Hole.Basket.X, Hole.Basket.Y, 0.035f), FVector(Hole.GreenRadius * 2.f, Hole.GreenRadius * 2.f, 0.02f) * TD::M)
			->SetCastShadow(false);
	}
	AddBasket(BasketBase);

	// ---- Hand-placed trees ----
	uint32 V = 1;
	for (const FTDTree& T : Hole.Trees)
	{
		AddTree(T.Pos, T.Scale, V++);
	}

	// ---- Tree lines: two loose rows of trees down both sides of the fairway ----
	FRandomStream Rng(Seed * 7919 + 13);
	auto ClearSpot = [&](const FVector2D& Pm, float FairwayClearance) -> bool
	{
		if (IsWater(Pm * TD::M) || IsWater((Pm + FVector2D(3.f, 0.f)) * TD::M) || IsWater((Pm - FVector2D(3.f, 0.f)) * TD::M)
			|| IsWater((Pm + FVector2D(0.f, 3.f)) * TD::M) || IsWater((Pm - FVector2D(0.f, 3.f)) * TD::M))
		{
			return false;
		}
		if (DistToFairway(Pm) < FairwayClearance || FVector2D::Distance(Pm, Hole.Basket) < Hole.GreenRadius + 7.f
			|| FVector2D::Distance(Pm, Hole.Tee) < 9.f)
		{
			return false;
		}
		for (const FTDBunker& Bk : Hole.Bunkers)
		{
			if (FVector2D::Distance(Pm, Bk.Center) < FMath::Max(Bk.Radii.X, Bk.Radii.Y) + 3.f) return false;
		}
		for (const FTDTree& T : Hole.Trees)
		{
			if (FVector2D::Distance(Pm, T.Pos) < 5.f) return false;
		}
		for (const FTDHill& H : Hole.Hills)
		{
			if (FVector2D::Distance(Pm, H.Center) < H.Radius * 0.75f) return false;
		}
		return true;
	};
	if (Hole.bTreeLined)
	{
		const float Edge = W * 0.5f + Hole.TreeLineGap;
		for (int32 i = 0; i + 1 < Hole.Fairway.Num(); ++i)
		{
			const FVector2D A = Hole.Fairway[i], B = Hole.Fairway[i + 1];
			const FVector2D Dir = (B - A).GetSafeNormal();
			const FVector2D Side(-Dir.Y, Dir.X);
			const float Len = FVector2D::Distance(A, B);
			for (float D = 4.f; D < Len + 6.f; D += Rng.FRandRange(6.5f, 9.5f))
			{
				for (float S : { -1.f, 1.f })
				{
					for (int32 Row = 0; Row < 2; ++Row)
					{
						const float Off = Edge + Row * Rng.FRandRange(6.f, 9.f) + Rng.FRandRange(0.f, 3.f);
						const FVector2D Pm = A + Dir * (D + Row * 3.5f) + Side * S * Off;
						if (ClearSpot(Pm, Edge - 0.5f))
						{
							AddTree(Pm, Rng.FRandRange(0.95f, 1.45f), Rng.RandHelper(100));
						}
					}
				}
			}
		}
	}

	// ---- Background woods ----
	FVector2D Lo(TNumericLimits<float>::Max()), Hi(-TNumericLimits<float>::Max());
	for (const FVector2D& Pt : Hole.Fairway)
	{
		Lo = FVector2D::Min(Lo, Pt);
		Hi = FVector2D::Max(Hi, Pt);
	}
	Lo -= FVector2D(60.f);
	Hi += FVector2D(60.f);
	for (int32 Try = 0, Placed = 0; Try < 700 && Placed < 90; ++Try)
	{
		const FVector2D Pm(Rng.FRandRange(Lo.X, Hi.X), Rng.FRandRange(Lo.Y, Hi.Y));
		if (ClearSpot(Pm, W * 0.5f + Hole.TreeLineGap + 16.f))
		{
			AddTree(Pm, Rng.FRandRange(1.f, 1.6f), Rng.RandHelper(100));
			++Placed;
		}
	}
	ShowPreview(nullptr, false);
}

// ---------------------------------------------------------------------------------------------------------------------
// Queries + flight
// ---------------------------------------------------------------------------------------------------------------------

float ATDCourseActor::GroundZ(const FVector2D& XY) const
{
	// The ground is analytic (flat lawn + dome hills + islands + the tee pad), so every query is exact and cheap.
	float Z = 0.f;
	const FVector2D Pm = XY / TD::M;
	for (const FTDHill& H : Hole.Hills)
	{
		const float D = FVector2D::Distance(Pm, H.Center) / H.Radius;
		if (D < 1.f)
		{
			Z = FMath::Max(Z, H.Height * TD::M * FMath::Sqrt(1.f - D * D));
		}
	}
	for (const FTDPond& P : Hole.Ponds)
	{
		if (P.IslandRadius > 0.f)
		{
			const float D = FVector2D::Distance(Pm, P.IslandCenter);
			if (D < P.IslandRadius) Z = FMath::Max(Z, 30.f);
			else if (D < P.IslandRadius + 0.8f) Z = FMath::Max(Z, 20.f);
		}
	}
	if (bHasTeePad)
	{
		const FVector2D L = (XY - TeeCenter).GetRotated(-TeeYawDeg);
		if (FMath::Abs(L.X) < 160.f && FMath::Abs(L.Y) < 85.f)
		{
			Z = FMath::Max(Z, TeeBaseZ + 9.f);
		}
	}
	return Z;
}

bool ATDCourseActor::HitsObstacle(const FVector& P, FVector& OutNormal) const
{
	for (const FObstacle& O : Obstacles)
	{
		if (O.bSphere)
		{
			const FVector D = P - O.Center;
			if (D.SizeSquared() < O.Radius * O.Radius)
			{
				OutNormal = D.GetSafeNormal();
				return true;
			}
		}
		else if (P.Z >= O.Center.Z && P.Z <= O.Center.Z + O.Height)
		{
			const FVector2D D = FVector2D(P - O.Center);
			if (D.SizeSquared() < O.Radius * O.Radius)
			{
				const FVector2D N = D.GetSafeNormal();
				OutNormal = FVector(N.X, N.Y, 0.f);
				return true;
			}
		}
	}
	return false;
}

bool ATDCourseActor::IsWater(const FVector2D& XY) const
{
	for (const FTDPond& P : Hole.Ponds)
	{
		const FVector2D D = (XY / TD::M - P.Center) / P.Radii;
		if (D.SizeSquared() < 1.f)
		{
			if (P.IslandRadius > 0.f && FVector2D::Distance(XY / TD::M, P.IslandCenter) < P.IslandRadius + 0.8f)
			{
				return false;
			}
			return true;
		}
	}
	return false;
}

bool ATDCourseActor::CageHit(const FVector& A, const FVector& B) const
{
	for (int32 i = 0; i <= 4; ++i)
	{
		const FVector Q = FMath::Lerp(A, B, i / 4.f);
		if (FVector::Dist2D(Q, BasketBase) < BasketCatchRadius && Q.Z > BasketBase.Z + CageLow && Q.Z < BasketBase.Z + CageHigh)
		{
			return true;
		}
	}
	return false;
}

void ATDCourseActor::Simulate(const FTDThrow& T, FTDFlight& Out, float Dt, float MaxTime) const
{
	const TArray<FTDDiscDef>& Discs = TDDiscs();
	const FTDDiscDef& D = Discs[FMath::Clamp(T.Disc, 0, Discs.Num() - 1)];
	const float VMax = D.MaxSpeed * TD::M;

	Out = FTDFlight();
	Out.Dt = Dt;
	FVector P = T.Start;
	FVector V = FRotator(T.Arc, T.Yaw, 0.f).Vector() * VMax * FMath::Sqrt(FMath::Clamp(T.Power, 0.02f, 1.f));
	Out.Points.Add(P);

	auto Finish = [&](ETDEnd End, const FVector& Where)
	{
		Out.End = End;
		Out.Rest = Where;
		Out.Points.Add(Where);
	};

	// ---- Air ---------------------------------------------------------------------------------------------------------
	bool bLanded = false;
	const int32 MaxSteps = FMath::CeilToInt(MaxTime / Dt);
	for (int32 Step = 0; Step < MaxSteps; ++Step)
	{
		const float HV = V.Size2D();
		const float Speed = V.Size();
		const float Fly = FMath::Min(1.f, FMath::Square(HV / VMax));
		FVector Acc(0.f, 0.f, -Gravity + D.Glide * Gravity * Fly);
		Acc -= V * Speed * (D.Drag / 100.f);
		if (HV > 1.f)
		{
			const FVector Right(-V.Y / HV, V.X / HV, 0.f);
			Acc += Right * (T.Curve * D.CurveK * Gravity * (HV / VMax));
		}
		V += Acc * Dt;
		const FVector Next = P + V * Dt;

		if (CageHit(P, Next))
		{
			Finish(ETDEnd::Basket, BasketBase + FVector(0.f, 0.f, 88.f));
			Out.LandIndex = Out.Points.Num() - 1;
			return;
		}

		FVector N;
		if (HitsObstacle(Next, N))
		{
			// Tree or pole: bounce off and lose most of the speed.
			if (FVector::DotProduct(V, N) < 0.f)
			{
				V = (V - 2.f * FVector::DotProduct(V, N) * N) * 0.28f;
			}
			Out.bHitTree = true;
			Out.Points.Add(P);
			continue;
		}
		const float G = GroundZ(FVector2D(Next));
		if (Next.Z <= G + 2.f)
		{
			P = FVector(Next.X, Next.Y, G + 3.f);
			Out.Points.Add(P);
			bLanded = true;
			break;
		}
		P = Next;
		Out.Points.Add(P);
		if (P.Z < -2000.f)
		{
			break;
		}
	}
	Out.LandIndex = Out.Points.Num() - 1;
	if (!bLanded)
	{
		P.Z = GroundZ(FVector2D(P)) + 3.f;
	}
	if (IsWater(FVector2D(P)))
	{
		Finish(ETDEnd::Water, FVector(P.X, P.Y, 3.f));
		return;
	}

	// ---- Ground: skid to a stop, following the slope ------------------------------------------------------------------
	FVector2D Dir = FVector2D(V.X, V.Y).GetSafeNormal();
	// How far the disc skids depends on what it lands on: sand stops it dead, rough grabs it.
	const ETDLie LandLie = LieAt(FVector2D(P));
	float Slide = FVector2D(V.X, V.Y).Size() * (LandLie == ETDLie::Sand ? 0.f : (LandLie == ETDLie::Rough ? 0.14f : 0.3f));
	const float Friction = LandLie == ETDLie::Rough ? 1400.f : 650.f;
	for (int32 Step = 0; Step < 240 && Slide > 0.f; ++Step)
	{
		Slide -= Friction * Dt;
		if (Slide <= 0.f)
		{
			break;
		}
		const FVector2D XY = FVector2D(P) + Dir * Slide * Dt;
		FVector Next(XY.X, XY.Y, GroundZ(XY) + 3.f);
		FVector N;
		if (HitsObstacle(Next + FVector(0.f, 0.f, 10.f), N))
		{
			break;  // skidded into a tree
		}
		P = Next;
		Out.Points.Add(P);
		if (IsWater(XY))
		{
			Finish(ETDEnd::Water, FVector(P.X, P.Y, 3.f));
			return;
		}
	}
	Finish(ETDEnd::Rest, P);
}

void ATDCourseActor::ShowPreview(const FTDFlight* Flight, bool bStrong)
{
	if (!PreviewDots)
	{
		return;
	}
	PreviewDots->ClearInstances();
	if (!Flight || Flight->Points.Num() < 2)
	{
		PreviewRing->SetVisibility(false);
		return;
	}
	// A dot every ~0.1 s of flight, then a ring where the disc comes to rest.
	const int32 Every = FMath::Max(1, FMath::RoundToInt(0.07f / Flight->Dt));
	int32 Count = 0;
	for (int32 i = Every; i <= Flight->LandIndex && Count < MaxPreviewDots; i += Every, ++Count)
	{
		PreviewDots->AddInstance(FTransform(FRotator::ZeroRotator, Flight->Points[i], FVector(bStrong ? 0.22f : 0.16f)), true);
	}
	PreviewMat->SetScalarParameterValue(TEXT("Opacity"), bStrong ? 0.95f : 0.6f);

	const FVector R = Flight->Rest;
	PreviewRing->SetVisibility(true);
	PreviewRing->SetWorldLocation(FVector(R.X, R.Y, (Flight->End == ETDEnd::Basket ? BasketBase.Z : R.Z) + 2.f));
	FLinearColor RC(1.f, 0.8f, 0.2f);
	if (Flight->End == ETDEnd::Basket) RC = FLinearColor(0.2f, 2.f, 0.4f);
	if (Flight->End == ETDEnd::Water) RC = FLinearColor(0.2f, 0.6f, 2.f);
	RingMat->SetVectorParameterValue(TEXT("Color"), RC);
}

void ATDCourseActor::Splash(const FVector& At)
{
	UStaticMeshComponent* C = TDAssets::AddMesh(this, RootComponent, TDAssets::Sphere(), TDAssets::MakeGlow(this, FLinearColor(0.7f, 0.9f, 1.f), 0.5f),
		FTransform(FRotator::ZeroRotator, At, FVector(0.6f, 0.6f, 0.25f)));
	C->SetUsingAbsoluteLocation(true);
	C->SetUsingAbsoluteScale(true);
	C->SetWorldLocation(At);
	C->SetCastShadow(false);
	Fx.Add({ C, 0.6f, 0.6f, FVector(0.6f, 0.6f, 0.25f) });
}

void ATDCourseActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	if (Flag)
	{
		Flag->SetRelativeRotation(FRotator(0.f, 8.f * FMath::Sin(Clock * 2.3f), 0.f));
	}
	for (int32 i = Fx.Num() - 1; i >= 0; --i)
	{
		FFx& F = Fx[i];
		F.Life -= DeltaSeconds;
		UStaticMeshComponent* C = F.C.Get();
		if (!C || F.Life <= 0.f)
		{
			if (C)
			{
				C->DestroyComponent();
			}
			Fx.RemoveAtSwap(i);
			continue;
		}
		const float A = 1.f - F.Life / F.MaxLife;
		C->SetWorldScale3D(F.BaseScale * FVector(1.f + 4.f * A, 1.f + 4.f * A, 1.f + 2.f * A));
	}
}
