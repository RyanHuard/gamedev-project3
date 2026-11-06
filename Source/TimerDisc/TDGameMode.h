#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDCourseActor.h"
#include "TDTypes.h"
#include "TDGameMode.generated.h"

class ATDPlayer;
class ATDDisc;
class ACameraActor;
class UAudioComponent;
class USoundBase;

UENUM()
enum class ETDState : uint8
{
	Title,
	HoleIntro,
	Aim,
	Throwing,  // wind-up animation
	Flight,
	Landed,
	HoleDone,
	Final
};

UENUM()
enum class ETDLanding : uint8
{
	None,
	Basket,
	Water,
	Rest
};

UCLASS()
class ATDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATDGameMode();
	void StartPlay() override;
	void Tick(float DeltaSeconds) override;

	// ---- Input ----
	void OnConfirm();
	void OnBack();
	void OnAimDelta(float DYaw, float DArc);
	void OnSelectDisc(int32 Index);
	void OnCycleDisc(int32 Dir);
	void OnCurve(int32 Dir);
	void OnChargeStart();
	void OnChargeRelease();
	void OnChargeCancel();
	void OnResetAim();
	void SetOverview(bool bOn);

	// ---- HUD ----
	ETDState GetState() const { return State; }
	float GetStateTime() const { return StateTime; }
	const FTDHole& GetHole() const { return Holes[HoleIndex]; }
	int32 GetHoleIndex() const { return HoleIndex; }
	int32 GetHoleCount() const { return Holes.Num(); }
	const TArray<FTDHole>& GetHoles() const { return Holes; }
	int32 GetStrokes() const { return Strokes; }
	const TArray<int32>& GetScores() const { return Scores; }
	int32 GetDisc() const { return DiscKind; }
	int32 GetCurveStep() const { return CurveStep; }
	float GetArc() const { return Arc; }
	bool IsCharging() const { return bCharging; }
	float GetPower() const { return Power; }
	float GetDistanceToBasket() const;
	ETDLanding GetLanding() const { return Landing; }
	bool LastHitTree() const { return bLastHitTree; }
	float GetLastThrowDistance() const { return LastThrowDistance; }
	FVector GetBasketLabelPoint() const;
	bool IsOverview() const { return bOverview; }
	int32 GetTotalStrokes() const;
	int32 GetTotalPar(bool bOnlyPlayed) const;
	int32 GetRoundStrokes() const;
	float GetClockMinutes() const { return ClockMinutes; }
	int32 GetFinishMinutes() const { return TD::TeeOffMinutes + GetTotalStrokes() * TD::MinutesPerStroke; }
	ETDLie GetLie() const { return Course->LieAt(FVector2D(Lie)); }
	ETDLie GetLandingLie() const { return Course->LieAt(FVector2D(Flight.Rest)); }

	static FString ScoreName(int32 Strokes, int32 Par);

private:
	TArray<FTDHole> Holes;
	ETDState State = ETDState::Title;
	float StateTime = 0.f;
	int32 HoleIndex = 0;
	int32 Strokes = 0;
	TArray<int32> Scores;
	float ClockMinutes = TD::TeeOffMinutes;

	UPROPERTY() TObjectPtr<ATDCourseActor> Course;
	UPROPERTY() TObjectPtr<ATDPlayer> Player;
	UPROPERTY() TObjectPtr<ATDDisc> Disc;
	UPROPERTY() TObjectPtr<ACameraActor> Overview;
	UPROPERTY() TObjectPtr<UAudioComponent> Ambience;
	bool bAmbienceOn = false;
	bool bLandSoundPlayed = false;

	// Aim
	float AimYaw = 0.f;
	float Arc = 12.f;
	int32 CurveStep = 0;   // -2..2
	int32 DiscKind = 1;
	bool bCharging = false;
	float ChargeTime = 0.f;
	float Power = 0.f;
	bool bOverview = false;

	// Lie
	FVector Lie = FVector::ZeroVector;
	FVector PrevLie = FVector::ZeroVector;

	// Flight
	FTDFlight Flight;
	float FlightTime = 0.f;
	float FlightYaw = 0.f;
	ETDLanding Landing = ETDLanding::None;
	bool bLastHitTree = false;
	float LastThrowDistance = 0.f;

	void SetState(ETDState S);
	void Sfx(USoundBase* Sound, float Volume, float PitchSpread = 0.f);
	void UpdateAmbience();
	void StartHole(int32 Index);
	void BeginAimFrom(const FVector& Ground);
	FTDThrow MakeThrow(float InPower) const;
	void Throw();
	void UpdateAim(float Dt);
	void UpdateFlight(float Dt);
	void FinishLanding();
	void PositionOverview();
	void AttachDiscToHand();
	void ResetClock();

};
