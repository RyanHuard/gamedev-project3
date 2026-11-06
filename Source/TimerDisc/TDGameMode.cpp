#include "TDGameMode.h"

#include "TDAssets.h"
#include "TDDisc.h"
#include "TDHUD.h"
#include "TDPlayer.h"
#include "TDPlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	constexpr float MeterPeriod = 1.1f;  // seconds for the power meter to fill (then it falls back)
	constexpr int32 MaxStrokes = 10;
	constexpr float ArcMin = 0.f, ArcMax = 40.f;
	constexpr float BreezeVolume = 0.08f;
}

ATDGameMode::ATDGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ATDPlayerController::StaticClass();
	HUDClass = ATDHUD::StaticClass();
}

void ATDGameMode::StartPlay()
{
	Super::StartPlay();
	Holes = TDBuildCourse();
	Scores.Init(-1, Holes.Num());

	UWorld* World = GetWorld();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Course = World->SpawnActor<ATDCourseActor>(ATDCourseActor::StaticClass(), FTransform::Identity, P);
	Player = World->SpawnActor<ATDPlayer>(ATDPlayer::StaticClass(), FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, P);
	Disc = World->SpawnActor<ATDDisc>(ATDDisc::StaticClass(), FTransform::Identity, P);
	Overview = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, P);
	Overview->GetCameraComponent()->FieldOfView = 70.f;
	Overview->GetCameraComponent()->bConstrainAspectRatio = false;
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		PC->Possess(Player);
	}

	StartHole(0);
	SetState(ETDState::Title);
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		PC->SetViewTarget(Overview);
	}
}

void ATDGameMode::SetState(ETDState S)
{
	State = S;
	StateTime = 0.f;
	UpdateAmbience();
}

void ATDGameMode::Sfx(USoundBase* Sound, float Volume, float PitchSpread)
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, Volume, 1.f + FMath::FRandRange(-PitchSpread, PitchSpread));
	}
}

void ATDGameMode::UpdateAmbience()
{
	// The breeze plays while you are out on a hole, and fades away on the title and final screens.
	const bool bWant = State != ETDState::Title && State != ETDState::Final;
	if (bWant == bAmbienceOn)
	{
		return;
	}
	bAmbienceOn = bWant;
	if (!Ambience)
	{
		if (USoundBase* Breeze = TDAssets::BreezeSound())
		{
			Ambience = UGameplayStatics::CreateSound2D(this, Breeze, BreezeVolume, 1.f, 0.f, nullptr, false, false);
		}
	}
	if (Ambience)
	{
		if (bWant)
		{
			Ambience->FadeIn(2.f, 1.f);
		}
		else
		{
			Ambience->FadeOut(1.5f, 0.f);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Holes and lies
// ---------------------------------------------------------------------------------------------------------------------

void ATDGameMode::StartHole(int32 Index)
{
	HoleIndex = Index;
	Strokes = 0;
	Landing = ETDLanding::None;
	Course->BuildHole(Holes[HoleIndex], HoleIndex + 1);
	DiscKind = 1;
	BeginAimFrom(Course->GetTee());
	// From the tee, aim down the fairway (on a dogleg the basket is behind the trees).
	AimYaw = Course->GetTeeYaw();
	Player->PlaceAt(Lie, AimYaw);
	if (AController* C = Player->GetController())
	{
		C->SetControlRotation(FRotator(-10.f, AimYaw, 0.f));
	}
	PositionOverview();
}

void ATDGameMode::BeginAimFrom(const FVector& Ground)
{
	Lie = Ground;
	const FVector Basket = Course->GetBasketBase();
	AimYaw = (Basket - Lie).Rotation().Yaw;
	CurveStep = 0;
	Arc = 12.f;
	bCharging = false;
	Power = 0.f;
	Player->PlaceAt(Lie, AimYaw);
	if (AController* C = Player->GetController())
	{
		C->SetControlRotation(FRotator(-10.f, AimYaw, 0.f));
	}
	AttachDiscToHand();
}

void ATDGameMode::AttachDiscToHand()
{
	Disc->SetKind(DiscKind);
	Disc->SetFlying(false);
	Disc->AttachToComponent(Player->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("hand_r"));
	Disc->SetActorRelativeLocation(FVector(0.f, 8.f, 0.f));
	Disc->SetActorRelativeRotation(FRotator(0.f, 0.f, 90.f));
}

void ATDGameMode::PositionOverview()
{
	// Look at the hole from behind the current lie, high enough to see the basket.
	const FVector A = (State == ETDState::Aim || State == ETDState::Title || State == ETDState::HoleIntro) ? Lie : Course->GetTee();
	const FVector B = Course->GetBasketBase();
	const FVector Mid = (A + B) * 0.5f;
	const float Span = FMath::Max(FVector::Dist2D(A, B), 2500.f);
	const FVector Back = (A - B).GetSafeNormal2D();
	const FVector CamLoc = Mid + Back * Span * 0.75f + FVector(0.f, 0.f, Span * 0.65f);
	Overview->SetActorLocation(CamLoc);
	Overview->SetActorRotation((Mid - CamLoc).Rotation());
}

int32 ATDGameMode::GetTotalStrokes() const
{
	int32 T = 0;
	for (int32 S : Scores)
	{
		T += FMath::Max(0, S);
	}
	return T;
}

int32 ATDGameMode::GetTotalPar(bool bOnlyPlayed) const
{
	int32 T = 0;
	for (int32 i = 0; i < Holes.Num(); ++i)
	{
		if (!bOnlyPlayed || Scores[i] >= 0)
		{
			T += Holes[i].Par;
		}
	}
	return T;
}

FString ATDGameMode::ScoreName(int32 InStrokes, int32 Par)
{
	if (InStrokes == 1)
	{
		return TEXT("Ace");
	}
	switch (InStrokes - Par)
	{
	case -3: return TEXT("Albatross");
	case -2: return TEXT("Eagle");
	case -1: return TEXT("Birdie");
	case 0: return TEXT("Par");
	case 1: return TEXT("Bogey");
	case 2: return TEXT("Double Bogey");
	case 3: return TEXT("Triple Bogey");
	default: return FString::Printf(TEXT("%+d"), InStrokes - Par);
	}
}

float ATDGameMode::GetDistanceToBasket() const
{
	return FVector::Dist2D(Lie, Course->GetBasketBase()) / TD::M;
}

FVector ATDGameMode::GetBasketLabelPoint() const
{
	return Course->GetFlagTop() + FVector(0.f, 0.f, 60.f);
}

// ---------------------------------------------------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------------------------------------------------

void ATDGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	StateTime += Dt;

	switch (State)
	{
	case ETDState::Aim:
		PlayTime += Dt;
		UpdateAim(Dt);
		break;
	case ETDState::Throwing:
		PlayTime += Dt;
		if (StateTime >= 0.32f)
		{
			// Let go of the disc.
			Disc->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			Disc->SetActorLocation(Flight.Points[0]);
			Disc->SetActorScale3D(FVector(1.f));
			Disc->SetFlying(true, CurveStep / 2.f, AimYaw);
			FlightYaw = AimYaw;
			FlightTime = 0.f;
			bLandSoundPlayed = false;
			Sfx(TDAssets::ThrowSound(), 0.45f + 0.45f * Power, 0.06f);
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				PC->SetViewTargetWithBlend(Disc, 0.45f, VTBlend_EaseInOut, 2.f);
			}
			SetState(ETDState::Flight);
		}
		break;
	case ETDState::Flight:
		PlayTime += Dt;
		UpdateFlight(Dt);
		break;
	case ETDState::Landed:
		if (StateTime > (Landing == ETDLanding::Basket ? 2.2f : 1.5f))
		{
			FinishLanding();
		}
		break;
	default:
		break;
	}
}

FTDThrow ATDGameMode::MakeThrow(float InPower) const
{
	FTDThrow T;
	T.Start = Player->ReleasePoint(AimYaw);
	T.Yaw = AimYaw;
	T.Arc = Arc;
	T.Power = InPower;
	T.Curve = CurveStep / 2.f;
	T.Disc = DiscKind;
	return T;
}

void ATDGameMode::UpdateAim(float Dt)
{
	Player->SetActorRotation(FRotator(0.f, AimYaw, 0.f));
	if (bCharging)
	{
		ChargeTime += Dt;
		const float Phase = FMath::Fmod(ChargeTime / MeterPeriod, 2.f);
		Power = 0.04f + 0.96f * (Phase < 1.f ? Phase : 2.f - Phase);
	}
	// Preview: while charging it follows the meter; otherwise it shows a full-power throw.
	FTDFlight Preview;
	Course->Simulate(MakeThrow(bCharging ? Power : 1.f), Preview);
	Course->ShowPreview(&Preview, bCharging);
}

void ATDGameMode::Throw()
{
	Course->Simulate(MakeThrow(Power), Flight);
	Course->ShowPreview(nullptr, false);
	PrevLie = Lie;
	++Strokes;
	bLastHitTree = Flight.bHitTree;
	Player->PlayThrow();
	SetOverview(false);
	SetState(ETDState::Throwing);
}

void ATDGameMode::UpdateFlight(float Dt)
{
	FlightTime += Dt;
	const float FIndex = FlightTime / Flight.Dt;
	const int32 I = FMath::FloorToInt(FIndex);
	if (I >= Flight.Points.Num() - 1)
	{
		Disc->SetActorLocation(Flight.Points.Last());
		Disc->SetFlying(false);
		Disc->SetActorRotation(FRotator::ZeroRotator);
		LastThrowDistance = FVector::Dist2D(PrevLie, Flight.Rest) / TD::M;
		switch (Flight.End)
		{
		case ETDEnd::Basket:
			Landing = ETDLanding::Basket;
			Player->PlayCheer();
			Sfx(TDAssets::ChainsSound(), 1.f);
			break;
		case ETDEnd::Water:
			Landing = ETDLanding::Water;
			Course->Splash(Flight.Rest);
			Disc->SetActorHiddenInGame(true);
			Sfx(TDAssets::SplashSound(), 0.9f);
			break;
		default:
			Landing = ETDLanding::Rest;
			if (!bLandSoundPlayed)
			{
				bLandSoundPlayed = true;
				Sfx(TDAssets::LandSound(), 0.7f, 0.05f);
			}
			break;
		}
		SetState(ETDState::Landed);
		return;
	}
	const FVector A = Flight.Points[I];
	const FVector B = Flight.Points[I + 1];
	Disc->SetActorLocation(FMath::Lerp(A, B, FIndex - I));
	if (I >= Flight.LandIndex)
	{
		if (!bLandSoundPlayed && Flight.End == ETDEnd::Rest)
		{
			bLandSoundPlayed = true;
			Sfx(TDAssets::LandSound(), 0.7f, 0.05f);
		}
		Disc->SetFlying(false);
		Disc->SetActorRotation(FRotator(0.f, FlightYaw, 0.f));
	}
	else if (FVector::Dist2D(A, B) > 0.5f)
	{
		// Camera swings gently to follow the curve.
		FlightYaw = FMath::FixedTurn(FlightYaw, (B - A).Rotation().Yaw, 60.f * Dt);
		Disc->SetCameraYaw(FlightYaw);
	}
}

void ATDGameMode::FinishLanding()
{
	Disc->SetActorHiddenInGame(false);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();

	if (Landing == ETDLanding::Basket)
	{
		Scores[HoleIndex] = Strokes;
		SetState(ETDState::HoleDone);
		return;
	}
	if (Landing == ETDLanding::Water)
	{
		++Strokes;  // penalty stroke; throw again from where you were
		BeginAimFrom(PrevLie);
	}
	else
	{
		BeginAimFrom(Flight.Rest - FVector(0.f, 0.f, 3.f));
	}
	if (Strokes >= MaxStrokes)
	{
		Scores[HoleIndex] = MaxStrokes;  // pick up and move on
		SetState(ETDState::HoleDone);
		return;
	}
	if (PC)
	{
		PC->SetViewTargetWithBlend(Player, 0.6f, VTBlend_EaseInOut, 2.f);
	}
	Landing = ETDLanding::None;
	SetState(ETDState::Aim);
}

// ---------------------------------------------------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------------------------------------------------

void ATDGameMode::OnConfirm()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	switch (State)
	{
	case ETDState::Title:
		Scores.Init(-1, Holes.Num());
		PlayTime = 0.f;
		StartHole(0);
		SetState(ETDState::HoleIntro);
		break;
	case ETDState::HoleIntro:
		if (StateTime > 0.3f)
		{
			if (PC) PC->SetViewTargetWithBlend(Player, 0.8f, VTBlend_EaseInOut, 2.f);
			SetState(ETDState::Aim);
		}
		break;
	case ETDState::Flight:
		// Skip ahead to where the disc ends up.
		FlightTime = Flight.Points.Num() * Flight.Dt;
		break;
	case ETDState::Landed:
		if (StateTime > 0.4f)
		{
			FinishLanding();
		}
		break;
	case ETDState::HoleDone:
		if (StateTime > 0.5f)
		{
			if (HoleIndex + 1 < Holes.Num())
			{
				StartHole(HoleIndex + 1);
				if (PC) PC->SetViewTarget(Overview);
				SetState(ETDState::HoleIntro);
			}
			else
			{
				SetState(ETDState::Final);
			}
		}
		break;
	case ETDState::Final:
		if (StateTime > 1.f)
		{
			StartHole(0);
			if (PC) PC->SetViewTarget(Overview);
			SetState(ETDState::Title);
		}
		break;
	default:
		break;
	}
}

void ATDGameMode::OnBack()
{
	if (State == ETDState::Title)
	{
		UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
		return;
	}
	StartHole(0);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetViewTarget(Overview);
	}
	SetState(ETDState::Title);
}

void ATDGameMode::OnAimDelta(float DYaw, float DArc)
{
	if (State != ETDState::Aim)
	{
		return;
	}
	AimYaw = FRotator::NormalizeAxis(AimYaw + DYaw);
	Arc = FMath::Clamp(Arc + DArc, ArcMin, ArcMax);
	if (AController* C = Player->GetController())
	{
		C->SetControlRotation(FRotator(-10.f, AimYaw, 0.f));
	}
}

void ATDGameMode::OnSelectDisc(int32 Index)
{
	if (State != ETDState::Aim || bCharging)
	{
		return;
	}
	DiscKind = FMath::Clamp(Index, 0, TDDiscs().Num() - 1);
	Disc->SetKind(DiscKind);
}

void ATDGameMode::OnCycleDisc(int32 Dir)
{
	OnSelectDisc((DiscKind + Dir + TDDiscs().Num()) % TDDiscs().Num());
}

void ATDGameMode::OnCurve(int32 Dir)
{
	if (State == ETDState::Aim)
	{
		CurveStep = FMath::Clamp(CurveStep + Dir, -2, 2);
	}
}

void ATDGameMode::OnChargeStart()
{
	if (State == ETDState::Aim && !bCharging)
	{
		bCharging = true;
		ChargeTime = 0.f;
		Power = 0.04f;
	}
}

void ATDGameMode::OnChargeRelease()
{
	if (State == ETDState::Aim && bCharging)
	{
		bCharging = false;
		Throw();
	}
}

void ATDGameMode::OnChargeCancel()
{
	bCharging = false;
	Power = 0.f;
}

void ATDGameMode::OnResetAim()
{
	if (State == ETDState::Aim && !bCharging)
	{
		BeginAimFrom(Lie);
	}
}

void ATDGameMode::SetOverview(bool bOn)
{
	if (bOn == bOverview || (bOn && State != ETDState::Aim))
	{
		return;
	}
	bOverview = bOn;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (bOn)
		{
			PositionOverview();
			PC->SetViewTargetWithBlend(Overview, 0.35f, VTBlend_EaseInOut, 2.f);
		}
		else if (State == ETDState::Aim)
		{
			PC->SetViewTargetWithBlend(Player, 0.35f, VTBlend_EaseInOut, 2.f);
		}
	}
}
