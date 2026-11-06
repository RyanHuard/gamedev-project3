#include "TDDisc.h"

#include "TDAssets.h"
#include "TDTypes.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ATDDisc::ATDDisc()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Spin = CreateDefaultSubobject<USceneComponent>(TEXT("Spin"));
	Spin->SetupAttachment(RootComponent);

	Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
	Boom->SetupAttachment(RootComponent);
	Boom->SetUsingAbsoluteRotation(true);
	Boom->TargetArmLength = 650.f;
	Boom->bDoCollisionTest = false;
	Boom->bEnableCameraLag = true;
	Boom->CameraLagSpeed = 6.f;
	Boom->bEnableCameraRotationLag = true;
	Boom->CameraRotationLagSpeed = 4.f;
	Boom->SetWorldRotation(FRotator(-16.f, 0.f, 0.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
	Camera->FieldOfView = 75.f;
}

void ATDDisc::Clear()
{
	for (UStaticMeshComponent* C : Parts)
	{
		if (C)
		{
			C->DestroyComponent();
		}
	}
	Parts.Reset();
	if (HourPivot) HourPivot->DestroyComponent();
	if (MinutePivot) MinutePivot->DestroyComponent();
	HourPivot = MinutePivot = nullptr;
}

UStaticMeshComponent* ATDDisc::Add(USceneComponent* Parent, UStaticMesh* Mesh, const FLinearColor& Color, const FVector& Loc, const FVector& SizeCm,
	const FRotator& Rot, bool bGlow)
{
	UMaterialInstanceDynamic* Mat = bGlow ? TDAssets::MakeGlow(this, Color) : TDAssets::MakeColor(this, Color, 0.35f);
	UStaticMeshComponent* C = TDAssets::AddMesh(this, Parent, Mesh, Mat, FTransform(Rot, Loc, SizeCm / 100.f));
	Parts.Add(C);
	return C;
}

void ATDDisc::SetKind(int32 InKind)
{
	if (InKind == Kind)
	{
		return;
	}
	Kind = InKind;
	Clear();

	const FTDDiscDef& D = TDDiscs()[FMath::Clamp(Kind, 0, TDDiscs().Num() - 1)];
	const float R = D.RadiusCm;
	const FLinearColor Face(0.93f, 0.92f, 0.86f);
	const FLinearColor Ink(0.04f, 0.04f, 0.05f);

	// Body: a colored rim with a cream face on top.
	Add(Spin, TDAssets::Cylinder(), D.Rim, FVector(0.f, 0.f, 0.f), FVector(2.f * R + 3.f, 2.f * R + 3.f, 4.f));
	Add(Spin, TDAssets::Cylinder(), Face, FVector(0.f, 0.f, 1.2f), FVector(2.f * R - 2.f, 2.f * R - 2.f, 2.f));
	// Hour ticks
	for (int32 i = 0; i < 12; ++i)
	{
		const FVector Dir = FRotator(0.f, i * 30.f, 0.f).Vector();
		const bool bBig = i % 3 == 0;
		Add(Spin, TDAssets::Cube(), Ink, Dir * R * 0.8f + FVector(0.f, 0.f, 2.4f), FVector(bBig ? 0.22f * R : 0.12f * R, 1.2f, 0.4f), FRotator(0.f, i * 30.f, 0.f));
	}
	// Hands
	HourPivot = NewObject<USceneComponent>(this);
	HourPivot->SetupAttachment(Spin);
	HourPivot->SetRelativeLocation(FVector(0.f, 0.f, 2.8f));
	HourPivot->RegisterComponent();
	MinutePivot = NewObject<USceneComponent>(this);
	MinutePivot->SetupAttachment(Spin);
	MinutePivot->SetRelativeLocation(FVector(0.f, 0.f, 3.2f));
	MinutePivot->RegisterComponent();
	Add(HourPivot, TDAssets::Cube(), Ink, FVector(R * 0.25f, 0.f, 0.f), FVector(R * 0.5f, 2.2f, 0.5f));
	Add(MinutePivot, TDAssets::Cube(), Ink, FVector(R * 0.36f, 0.f, 0.f), FVector(R * 0.72f, 1.4f, 0.5f));
	Add(Spin, TDAssets::Cylinder(), D.Rim, FVector(0.f, 0.f, 3.6f), FVector(3.f, 3.f, 1.f));
	HourPivot->SetRelativeRotation(FRotator(0.f, -60.f, 0.f));   // ten past ten, the classic clock pose
	MinutePivot->SetRelativeRotation(FRotator(0.f, 60.f, 0.f));

	if (Kind == 0)
	{
		// Pocket watch: winding crown and a bow on the rim.
		Add(Spin, TDAssets::Cylinder(), D.Rim, FVector(R + 3.f, 0.f, 0.f), FVector(4.f, 4.f, 5.f), FRotator(90.f, 0.f, 0.f));
		Add(Spin, TDAssets::Cylinder(), D.Rim, FVector(R + 7.f, 0.f, 0.f), FVector(7.f, 7.f, 1.2f));
	}
	else if (Kind == 2)
	{
		// Alarm clock: two bells and a striker on the rim.
		for (float A : { -38.f, 38.f })
		{
			Add(Spin, TDAssets::Sphere(), FLinearColor(0.9f, 0.85f, 0.75f), FRotator(0.f, A, 0.f).Vector() * (R + 4.f) + FVector(0.f, 0.f, 1.f), FVector(10.f, 10.f, 6.f));
		}
		Add(Spin, TDAssets::Cube(), Ink, FVector(R + 4.f, 0.f, 1.f), FVector(4.f, 2.f, 2.f));
	}
}

void ATDDisc::SetFlying(bool bInFlying, float InCurve, float InYaw)
{
	bFlying = bInFlying;
	Curve = InCurve;
	CamYaw = InYaw;
	Boom->SetWorldRotation(FRotator(-16.f, CamYaw, 0.f));
	if (!bFlying)
	{
		Spin->SetRelativeRotation(FRotator::ZeroRotator);
	}
}

void ATDDisc::SetCameraYaw(float Yaw)
{
	CamYaw = Yaw;
}

void ATDDisc::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	if (bFlying)
	{
		// Spin fast, bank into the curve, and let the hands race around: time flies.
		Spin->AddLocalRotation(FRotator(0.f, 1400.f * DeltaSeconds, 0.f));
		SetActorRotation(FRotator(0.f, CamYaw, Curve * 18.f));
		if (MinutePivot) MinutePivot->AddLocalRotation(FRotator(0.f, 720.f * DeltaSeconds, 0.f));
		if (HourPivot) HourPivot->AddLocalRotation(FRotator(0.f, 60.f * DeltaSeconds, 0.f));
		Boom->SetWorldRotation(FRotator(-16.f, CamYaw, 0.f));
	}
}
