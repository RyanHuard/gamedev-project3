#include "TDTypes.h"

const TArray<FTDDiscDef>& TDDiscs()
{
	static TArray<FTDDiscDef> Discs = []
	{
		TArray<FTDDiscDef> D;
		FTDDiscDef Putter;
		Putter.Name = TEXT("Pocket Watch");
		Putter.Role = TEXT("Putter");
		Putter.MaxSpeed = 17.f;
		Putter.Glide = 0.55f;
		Putter.Drag = 0.006f;
		Putter.CurveK = 0.6f;
		Putter.RadiusCm = 13.f;
		Putter.Rim = FLinearColor(0.95f, 0.7f, 0.2f);
		D.Add(Putter);

		FTDDiscDef Mid;
		Mid.Name = TEXT("Wall Clock");
		Mid.Role = TEXT("Midrange");
		Mid.MaxSpeed = 23.f;
		Mid.Glide = 0.6f;
		Mid.Drag = 0.005f;
		Mid.CurveK = 0.7f;
		Mid.RadiusCm = 16.f;
		Mid.Rim = FLinearColor(0.08f, 0.1f, 0.14f);
		D.Add(Mid);

		FTDDiscDef Driver;
		Driver.Name = TEXT("Alarm Clock");
		Driver.Role = TEXT("Driver");
		Driver.MaxSpeed = 31.f;
		Driver.Glide = 0.62f;
		Driver.Drag = 0.0045f;
		Driver.CurveK = 0.8f;
		Driver.RadiusCm = 16.f;
		Driver.Rim = FLinearColor(0.85f, 0.08f, 0.06f);
		D.Add(Driver);
		return D;
	}();
	return Discs;
}

const TCHAR* TDLieName(ETDLie Lie)
{
	switch (Lie)
	{
	case ETDLie::Tee: return TEXT("Tee box");
	case ETDLie::Fairway: return TEXT("Fairway");
	case ETDLie::Rough: return TEXT("Rough");
	case ETDLie::Sand: return TEXT("Bunker");
	case ETDLie::Green: return TEXT("Green");
	case ETDLie::Water: return TEXT("Water");
	}
	return TEXT("");
}

namespace
{
	FTDTree Tree(float X, float Y, float Scale = 1.f) { return FTDTree{ FVector2D(X, Y), Scale }; }
	FTDBunker Bunker(float X, float Y, float RX, float RY) { return FTDBunker{ FVector2D(X, Y), FVector2D(RX, RY) }; }
	FVector2D P(float X, float Y) { return FVector2D(X, Y); }
}

// Nine holes laid out like a real golf course: tee box, a mown fairway lined with trees, bunkers, and a green.
// Meters. Each hole is built around its own origin; the tee is at (0, 0) and the hole plays toward +X.
TArray<FTDHole> TDBuildCourse()
{
	TArray<FTDHole> C;

	{
		FTDHole H;
		H.Name = TEXT("First Throw");
		H.Tip = TEXT("Hold left click, release to throw");
		H.Par = 3;
		H.Basket = P(46.f, 0.f);
		H.Fairway = { P(0.f, 0.f), P(46.f, 0.f) };
		H.FairwayWidth = 20.f;
		H.Bunkers = { Bunker(39.f, -10.f, 4.f, 2.5f), Bunker(53.f, 9.f, 3.5f, 3.f) };
		C.Add(H);
	}
	{
		FTDHole H;
		H.Name = TEXT("The Old Oak");
		H.Tip = TEXT("A / D: curve");
		H.Par = 3;
		H.Basket = P(56.f, 24.f);
		H.Fairway = { P(0.f, 0.f), P(32.f, 0.f), P(56.f, 24.f) };
		H.FairwayWidth = 20.f;
		H.Trees = { Tree(30.f, 12.f, 1.7f), Tree(24.f, 17.f, 1.2f), Tree(38.f, 20.f, 1.1f) };
		H.Bunkers = { Bunker(62.f, 31.f, 4.f, 3.f) };
		C.Add(H);
	}
	{
		FTDHole H;
		H.Name = TEXT("The Long Way");
		H.Tip = TEXT("1 / 2 / 3: change disc");
		H.Par = 4;
		H.Basket = P(190.f, 0.f);
		H.Fairway = { P(0.f, 0.f), P(190.f, 0.f) };
		H.FairwayWidth = 24.f;
		H.Bunkers = { Bunker(72.f, 13.f, 6.f, 3.f), Bunker(98.f, -12.f, 5.f, 3.f), Bunker(150.f, 13.f, 6.f, 3.f), Bunker(183.f, 10.f, 4.f, 3.f) };
		C.Add(H);
	}
	{
		FTDHole H;
		H.Name = TEXT("Over the Water");
		H.Tip = TEXT("Water: +1 stroke, +10 min");
		H.Par = 3;
		H.Basket = P(72.f, 0.f);
		H.Fairway = { P(0.f, 0.f), P(72.f, 0.f) };
		H.FairwayWidth = 22.f;
		H.Ponds = { FTDPond{ P(36.f, 0.f), P(13.f, 22.f) } };
		H.Bunkers = { Bunker(80.f, -8.f, 4.f, 3.f) };
		C.Add(H);
	}
	{
		FTDHole H;
		H.Name = TEXT("Dogleg Left");
		H.Par = 4;
		H.Basket = P(135.f, -62.f);
		H.Fairway = { P(0.f, 0.f), P(90.f, 0.f), P(135.f, -62.f) };
		H.FairwayWidth = 22.f;
		H.Trees = { Tree(68.f, -15.f, 1.3f), Tree(74.f, -21.f, 1.3f), Tree(81.f, -26.f, 1.3f), Tree(87.f, -32.f, 1.3f),
			Tree(66.f, -24.f, 1.2f), Tree(74.f, -31.f, 1.2f), Tree(82.f, -38.f, 1.2f) };
		H.Bunkers = { Bunker(104.f, 9.f, 6.f, 3.5f), Bunker(142.f, -54.f, 3.5f, 3.f) };
		C.Add(H);
	}
	{
		FTDHole H;
		H.Name = TEXT("Up the Hill");
		H.Tip = TEXT("Mouse up / down: arc");
		H.Par = 3;
		H.Basket = P(64.f, 0.f);
		H.Fairway = { P(0.f, 0.f), P(64.f, 0.f) };
		H.FairwayWidth = 22.f;
		H.Hills = { FTDHill{ P(64.f, 0.f), 24.f, 7.f } };
		H.Bunkers = { Bunker(40.f, -12.f, 4.f, 3.f), Bunker(42.f, 12.f, 4.f, 3.f) };
		C.Add(H);
	}
	{
		FTDHole H;
		H.Name = TEXT("Down the Hill");
		H.Par = 3;
		H.Basket = P(56.f, 0.f);
		H.Fairway = { P(0.f, 0.f), P(56.f, 0.f) };
		H.FairwayWidth = 22.f;
		H.Hills = { FTDHill{ P(0.f, 0.f), 22.f, 8.f } };
		H.Ponds = { FTDPond{ P(77.f, 0.f), P(10.f, 24.f) } };
		H.Bunkers = { Bunker(50.f, -11.f, 4.f, 2.5f) };
		C.Add(H);
	}
	{
		FTDHole H;
		H.Name = TEXT("The Chute");
		H.Par = 4;
		H.Basket = P(150.f, 0.f);
		H.Fairway = { P(0.f, 0.f), P(75.f, 5.f), P(150.f, 0.f) };
		H.FairwayWidth = 13.f;
		H.TreeLineGap = 2.f;
		H.Bunkers = { Bunker(156.f, 7.f, 3.f, 2.5f) };
		C.Add(H);
	}
	{
		FTDHole H;
		H.Name = TEXT("Island Green");
		H.Par = 3;
		H.Basket = P(64.f, 0.f);
		H.Fairway = { P(0.f, 0.f), P(30.f, 0.f) };
		H.FairwayWidth = 22.f;
		H.GreenRadius = 6.f;
		H.Ponds = { FTDPond{ P(58.f, 0.f), P(27.f, 26.f), 6.5f, P(64.f, 0.f) } };
		C.Add(H);
	}
	return C;
}
