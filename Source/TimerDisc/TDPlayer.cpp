#include "TDPlayer.h"

#include "TDAssets.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

ATDPlayer::ATDPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 90.f);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;

	USkeletalMeshComponent* Body = GetMesh();
	Body->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	Body->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
	Boom->SetupAttachment(RootComponent);
	Boom->TargetArmLength = 430.f;
	Boom->SocketOffset = FVector(0.f, 110.f, 130.f);
	Boom->bUsePawnControlRotation = true;
	Boom->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
	Camera->FieldOfView = 75.f;
}

void ATDPlayer::BeginPlay()
{
	Super::BeginPlay();
	// No physics walking: the game places the player directly.
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetCharacterMovement()->GravityScale = 0.f;
	if (USkeletalMesh* SK = TDAssets::Mannequin())
	{
		GetMesh()->SetSkeletalMesh(SK);
	}
	// Animations are played directly (the walk/jump blueprint would think we are falling, since we never walk).
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(TDAssets::IdleAnim(), true);
}

void ATDPlayer::PlayOnce(UAnimSequence* Seq, float Rate)
{
	if (!Seq)
	{
		return;
	}
	GetMesh()->PlayAnimation(Seq, false);
	GetMesh()->SetPlayRate(Rate);
	BackToIdle = Seq->GetPlayLength() / Rate;
}

void ATDPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (BackToIdle > 0.f)
	{
		BackToIdle -= DeltaSeconds;
		if (BackToIdle <= 0.f)
		{
			GetMesh()->PlayAnimation(TDAssets::IdleAnim(), true);
			GetMesh()->SetPlayRate(1.f);
		}
	}
}

void ATDPlayer::PlaceAt(const FVector& Ground, float Yaw)
{
	SetActorLocation(Ground + FVector(0.f, 0.f, 90.f), false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator(0.f, Yaw, 0.f));
}

FVector ATDPlayer::ReleasePoint(float Yaw) const
{
	return GetActorLocation() + FRotator(0.f, Yaw, 0.f).Vector() * 60.f + FRotator(0.f, Yaw + 90.f, 0.f).Vector() * 20.f + FVector(0.f, 0.f, 40.f);
}

void ATDPlayer::PlayThrow()
{
	PlayOnce(TDAssets::ThrowAnim(), 1.4f);
}

void ATDPlayer::PlayCheer()
{
	PlayOnce(TDAssets::CheerAnim(), 1.f);
}
