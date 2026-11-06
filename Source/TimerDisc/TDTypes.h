#pragma once

#include "CoreMinimal.h"

/** Course data is authored in meters; Unreal works in centimeters. */
namespace TD
{
	constexpr float M = 100.f;
	inline FVector W(float X, float Y, float Z = 0.f) { return FVector(X * M, Y * M, Z * M); }
}

/** A disc in the bag. The discs are clocks: a pocket watch putter, a wall clock midrange, an alarm clock driver. */
struct FTDDiscDef
{
	FString Name;
	FString Role;          // "Putter", "Midrange", "Driver"
	float MaxSpeed = 20.f; // m/s at full power
	float Glide = 0.6f;    // lift at full speed, as a fraction of gravity
	float Drag = 0.005f;   // quadratic drag
	float CurveK = 0.6f;   // sideways push at full curve, as a fraction of gravity
	float RadiusCm = 15.f;
	FLinearColor Rim = FLinearColor::White;
};

/** What the disc is sitting on. */
enum class ETDLie : uint8
{
	Tee,
	Fairway,
	Rough,
	Sand,
	Green,
	Water
};

struct FTDTree
{
	FVector2D Pos = FVector2D::ZeroVector;
	float Scale = 1.f;
};

struct FTDPond
{
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D Radii = FVector2D(10.f, 10.f);
	float IslandRadius = 0.f;                  // > 0: a dry island (putting green) in the middle
	FVector2D IslandCenter = FVector2D::ZeroVector;
};

struct FTDHill
{
	FVector2D Center = FVector2D::ZeroVector;
	float Radius = 20.f;
	float Height = 5.f;
};

struct FTDBunker
{
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D Radii = FVector2D(4.f, 3.f);
};

struct FTDHole
{
	FString Name;
	FString Tip;      // short control reminder, on the hole where that control first matters (usually empty)
	int32 Par = 3;
	FVector2D Tee = FVector2D::ZeroVector;
	FVector2D Basket = FVector2D(40.f, 0.f);

	// Golf-course shape: a mown fairway along a center line from the tee to the green.
	TArray<FVector2D> Fairway;
	float FairwayWidth = 22.f;
	float GreenRadius = 10.f;
	float TreeLineGap = 6.f;    // how far outside the fairway edge the tree lines start
	bool bTreeLined = true;

	TArray<FTDTree> Trees;      // extra hand-placed trees
	TArray<FTDPond> Ponds;
	TArray<FTDHill> Hills;
	TArray<FTDBunker> Bunkers;
};

const TArray<FTDDiscDef>& TDDiscs();
TArray<FTDHole> TDBuildCourse();
const TCHAR* TDLieName(ETDLie Lie);
