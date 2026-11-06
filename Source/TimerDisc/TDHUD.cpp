#include "TDHUD.h"

#include "TDAssets.h"
#include "TDGameMode.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "Rendering/SlateRenderer.h"
#include "TextureResource.h"

namespace
{
	const FLinearColor PanelBg(0.025f, 0.05f, 0.035f, 0.84f);
	const FLinearColor PanelSoft(0.025f, 0.05f, 0.035f, 0.62f);
	const FLinearColor Accent(1.f, 0.74f, 0.2f);
	const FLinearColor AccentDark(0.55f, 0.36f, 0.05f);
	const FLinearColor Cream(0.98f, 0.96f, 0.9f);
	const FLinearColor Muted(0.68f, 0.76f, 0.7f);
	const FLinearColor Good(0.42f, 0.95f, 0.5f);
	const FLinearColor Bad(1.f, 0.5f, 0.38f);
	const FLinearColor WaterBlue(0.45f, 0.78f, 1.f);
	const FLinearColor Ink(0.02f, 0.03f, 0.02f, 1.f);

	/** "20 min ahead", "On pace", "10 min behind": how the round compares with making sunset exactly. */
	FString PaceText(int32 StrokesOverPar)
	{
		const int32 Min = FMath::Abs(StrokesOverPar) * TD::MinutesPerStroke;
		if (StrokesOverPar == 0)
		{
			return TEXT("On pace");
		}
		return FString::Printf(TEXT("%d min %s"), Min, StrokesOverPar < 0 ? TEXT("ahead") : TEXT("behind"));
	}

	FLinearColor WithAlpha(FLinearColor C, float A)
	{
		C.A *= A;
		return C;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Drawing kit
// ---------------------------------------------------------------------------------------------------------------------

FSlateFontInfo ATDHUD::MakeFont(float SizePx, bool bBold, float /*Outline*/) const
{
	// Slate font sizes are points at 96 dpi; convert so SizePx is the cap-to-descender height at 1080p.
	return FSlateFontInfo(Font, FMath::Max(1, FMath::RoundToInt(SizePx * S * 0.75f)), bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular")));
}

FVector2D ATDHUD::Measure(const FString& Str, float SizePx, bool bBold)
{
	if (!FSlateApplication::IsInitialized())
	{
		return FVector2D(Str.Len() * SizePx * S * 0.55f, SizePx * S);
	}
	const TSharedRef<FSlateFontMeasure> M = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return M->Measure(Str, MakeFont(SizePx, bBold, 0.f), 1.f);
}

void ATDHUD::Text(const FString& Str, float X, float Y, float SizePx, const FLinearColor& Color, EAlign Align, bool bBold, float Outline)
{
	const FSlateFontInfo FI = MakeFont(SizePx, bBold, Outline);
	if (Align != EAlign::Left)
	{
		const float W = Measure(Str, SizePx, bBold).X;
		X -= Align == EAlign::Center ? W * 0.5f : W;
	}
	const FText T = FText::FromString(Str);
	if (Outline > 0.f)
	{
		// Thick outline: stamp the text in dark ink around a circle, then draw it on top.
		const float O = Outline * S;
		const FLinearColor OC(0.02f, 0.04f, 0.02f, Color.A * 0.9f);
		for (int32 i = 0; i < 12; ++i)
		{
			const float A = i * PI / 6.f;
			FCanvasTextItem Item(FVector2D(X + FMath::Cos(A) * O, Y + FMath::Sin(A) * O), T, FI, OC);
			Canvas->DrawItem(Item);
		}
	}
	else if (Color.GetLuminance() > 0.35f)
	{
		FCanvasTextItem Shadow(FVector2D(X + 1.5f * S, Y + 2.f * S), T, FI, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.55f));
		Canvas->DrawItem(Shadow);
	}
	FCanvasTextItem Item(FVector2D(X, Y), T, FI, Color);
	Canvas->DrawItem(Item);
}

void ATDHUD::Tris(TArray<FCanvasUVTri>& T)
{
	if (T.Num() == 0)
	{
		return;
	}
	FCanvasTriangleItem Item(T, GWhiteTexture);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void ATDHUD::AddQuad(TArray<FCanvasUVTri>& T, const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D, const FLinearColor& CA, const FLinearColor& CB)
{
	// A-B top edge (color CA), D-C bottom edge (color CB)
	FCanvasUVTri T1;
	T1.V0_Pos = A; T1.V1_Pos = B; T1.V2_Pos = C;
	T1.V0_Color = CA; T1.V1_Color = CA; T1.V2_Color = CB;
	FCanvasUVTri T2;
	T2.V0_Pos = A; T2.V1_Pos = C; T2.V2_Pos = D;
	T2.V0_Color = CA; T2.V1_Color = CB; T2.V2_Color = CB;
	T.Add(T1);
	T.Add(T2);
}

void ATDHUD::RoundRectV(float X, float Y, float W, float H, float R, const FLinearColor& Top, const FLinearColor& Bottom)
{
	R = FMath::Min(R, FMath::Min(W, H) * 0.5f);
	TArray<FCanvasUVTri> T;
	auto ColorAt = [&](float PY) { return FMath::Lerp(Top, Bottom, FMath::Clamp((PY - Y) / H, 0.f, 1.f)); };
	// Middle band (full width) and top / bottom bands between the corners.
	AddQuad(T, FVector2D(X, Y + R), FVector2D(X + W, Y + R), FVector2D(X + W, Y + H - R), FVector2D(X, Y + H - R), ColorAt(Y + R), ColorAt(Y + H - R));
	AddQuad(T, FVector2D(X + R, Y), FVector2D(X + W - R, Y), FVector2D(X + W - R, Y + R), FVector2D(X + R, Y + R), ColorAt(Y), ColorAt(Y + R));
	AddQuad(T, FVector2D(X + R, Y + H - R), FVector2D(X + W - R, Y + H - R), FVector2D(X + W - R, Y + H), FVector2D(X + R, Y + H), ColorAt(Y + H - R), ColorAt(Y + H));
	// Corner fans
	const FVector2D Centers[4] = { FVector2D(X + R, Y + R), FVector2D(X + W - R, Y + R), FVector2D(X + W - R, Y + H - R), FVector2D(X + R, Y + H - R) };
	const float Starts[4] = { PI, 1.5f * PI, 0.f, 0.5f * PI };
	constexpr int32 Seg = 7;
	for (int32 c = 0; c < 4; ++c)
	{
		for (int32 i = 0; i < Seg; ++i)
		{
			const float A0 = Starts[c] + (0.5f * PI) * i / Seg;
			const float A1 = Starts[c] + (0.5f * PI) * (i + 1) / Seg;
			FCanvasUVTri Tri;
			Tri.V0_Pos = Centers[c];
			Tri.V1_Pos = Centers[c] + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R;
			Tri.V2_Pos = Centers[c] + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * R;
			Tri.V0_Color = ColorAt(Tri.V0_Pos.Y);
			Tri.V1_Color = ColorAt(Tri.V1_Pos.Y);
			Tri.V2_Color = ColorAt(Tri.V2_Pos.Y);
			T.Add(Tri);
		}
	}
	Tris(T);
}

void ATDHUD::RoundRect(float X, float Y, float W, float H, float R, const FLinearColor& Color)
{
	RoundRectV(X, Y, W, H, R, Color, Color);
}

void ATDHUD::Circle(float CX, float CY, float R, const FLinearColor& Color)
{
	TArray<FCanvasUVTri> T;
	const int32 Seg = FMath::Clamp(FMath::RoundToInt(R * 0.8f), 16, 64);
	for (int32 i = 0; i < Seg; ++i)
	{
		const float A0 = 2.f * PI * i / Seg, A1 = 2.f * PI * (i + 1) / Seg;
		FCanvasUVTri Tri;
		Tri.V0_Pos = FVector2D(CX, CY);
		Tri.V1_Pos = FVector2D(CX + FMath::Cos(A0) * R, CY + FMath::Sin(A0) * R);
		Tri.V2_Pos = FVector2D(CX + FMath::Cos(A1) * R, CY + FMath::Sin(A1) * R);
		Tri.V0_Color = Tri.V1_Color = Tri.V2_Color = Color;
		T.Add(Tri);
	}
	Tris(T);
}

void ATDHUD::Ring(float CX, float CY, float R, float Thick, const FLinearColor& Color)
{
	TArray<FCanvasUVTri> T;
	const int32 Seg = FMath::Clamp(FMath::RoundToInt(R * 0.8f), 16, 64);
	for (int32 i = 0; i < Seg; ++i)
	{
		const float A0 = 2.f * PI * i / Seg, A1 = 2.f * PI * (i + 1) / Seg;
		const FVector2D D0(FMath::Cos(A0), FMath::Sin(A0)), D1(FMath::Cos(A1), FMath::Sin(A1));
		const FVector2D C(CX, CY);
		AddQuad(T, C + D0 * R, C + D1 * R, C + D1 * (R - Thick), C + D0 * (R - Thick), Color, Color);
	}
	Tris(T);
}

void ATDHUD::Line(float X1, float Y1, float X2, float Y2, float Thick, const FLinearColor& Color)
{
	const FVector2D A(X1, Y1), B(X2, Y2);
	const FVector2D N = FVector2D(-(B - A).Y, (B - A).X).GetSafeNormal() * Thick * 0.5f;
	TArray<FCanvasUVTri> T;
	AddQuad(T, A + N, B + N, B - N, A - N, Color, Color);
	Tris(T);
}

void ATDHUD::Pill(const FString& Label, float CX, float Y, float SizePx, const FLinearColor& Bg, const FLinearColor& Fg)
{
	const FVector2D Sz = Measure(Label, SizePx);
	const float PadX = SizePx * 0.9f * S, PadY = SizePx * 0.42f * S;
	const float W = Sz.X + PadX * 2.f, H = Sz.Y + PadY * 2.f;
	RoundRect(CX - W * 0.5f, Y, W, H, H * 0.5f, Bg);
	Text(Label, CX, Y + PadY, SizePx, Fg, EAlign::Center, true);
}

void ATDHUD::ClockFace(float CX, float CY, float R, const FLinearColor& Rim, float HourAngle, float MinuteAngle)
{
	Circle(CX, CY + R * 0.08f, R, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	Circle(CX, CY, R, Rim);
	Circle(CX, CY, R * 0.8f, FLinearColor(0.96f, 0.94f, 0.86f));
	for (int32 i = 0; i < 12; ++i)
	{
		const float A = i * PI / 6.f;
		const float In = (i % 3 == 0) ? 0.58f : 0.66f;
		Line(CX + FMath::Cos(A) * R * In, CY + FMath::Sin(A) * R * In, CX + FMath::Cos(A) * R * 0.74f, CY + FMath::Sin(A) * R * 0.74f,
			FMath::Max(1.f, R * 0.06f), Ink);
	}
	Line(CX, CY, CX + FMath::Cos(HourAngle) * R * 0.38f, CY + FMath::Sin(HourAngle) * R * 0.38f, R * 0.11f, Ink);
	Line(CX, CY, CX + FMath::Cos(MinuteAngle) * R * 0.6f, CY + FMath::Sin(MinuteAngle) * R * 0.6f, R * 0.07f, Ink);
	Circle(CX, CY, R * 0.08f, Rim);
}

void ATDHUD::ClockIcon(float CX, float CY, float R, const FLinearColor& Rim, float HandsAngle)
{
	ClockFace(CX, CY, R, Rim, HandsAngle * 0.083f - PI * 0.5f - 1.05f, HandsAngle - PI * 0.5f + 1.05f);
}

void ATDHUD::ClockAt(float CX, float CY, float R, const FLinearColor& Rim, float Minutes)
{
	// The hands show the actual time on the course.
	ClockFace(CX, CY, R, Rim, Minutes / 720.f * 2.f * PI - PI * 0.5f, FMath::Fmod(Minutes, 60.f) / 60.f * 2.f * PI - PI * 0.5f);
}

void ATDHUD::Badge(float CX, float CY, float R, const FString& Top, const FString& Number)
{
	Circle(CX, CY + 3.f * S, R, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	Circle(CX, CY, R, Accent);
	Ring(CX, CY, R, R * 0.07f, FLinearColor(1.f, 0.9f, 0.6f));
	if (!Top.IsEmpty())
	{
		const float TopSize = R * 0.28f / S;
		Text(Top, CX, CY - R * 0.62f, TopSize, Ink, EAlign::Center, true);
		Text(Number, CX, CY - R * 0.42f, R * 0.95f / S, Ink, EAlign::Center, true);
	}
	else
	{
		Text(Number, CX, CY - R * 0.62f, R * 1.1f / S, Ink, EAlign::Center, true);
	}
}

void ATDHUD::KeyCap(const FString& Key, float X, float Y, float H)
{
	const FVector2D Sz = Measure(Key, H * 0.5f / S);
	const float W = FMath::Max(H, Sz.X + H * 0.6f);
	RoundRect(X, Y + 3.f * S, W, H, 8.f * S, FLinearColor(0.f, 0.f, 0.f, 0.5f));
	RoundRectV(X, Y, W, H, 8.f * S, FLinearColor(0.98f, 0.96f, 0.9f), FLinearColor(0.82f, 0.8f, 0.74f));
	Text(Key, X + W * 0.5f, Y + H * 0.2f, H * 0.5f / S, Ink, EAlign::Center, true);
}

void ATDHUD::WorldPill(const FVector& World, const FString& Label)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const FVector Cam = PC->PlayerCameraManager->GetCameraLocation();
	if (FVector::DotProduct(World - Cam, PC->PlayerCameraManager->GetCameraRotation().Vector()) <= 0.f)
	{
		return;
	}
	const FVector P = Project(World, false);
	const FVector2D Sz = Measure(Label, 20.f);
	const float W = Sz.X + 30.f * S, H = Sz.Y + 14.f * S;
	RoundRect(P.X - W * 0.5f, P.Y - H - 10.f * S, W, H, H * 0.5f, PanelBg);
	// little pointer under the pill
	TArray<FCanvasUVTri> T;
	FCanvasUVTri Tri;
	Tri.V0_Pos = FVector2D(P.X - 8.f * S, P.Y - 10.f * S);
	Tri.V1_Pos = FVector2D(P.X + 8.f * S, P.Y - 10.f * S);
	Tri.V2_Pos = FVector2D(P.X, P.Y);
	Tri.V0_Color = Tri.V1_Color = Tri.V2_Color = PanelBg;
	T.Add(Tri);
	Tris(T);
	Text(Label, P.X, P.Y - H - 10.f * S + 7.f * S, 20.f, Accent, EAlign::Center, true);
}

// ---------------------------------------------------------------------------------------------------------------------

void ATDHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	if (!Font)
	{
		Font = TDAssets::Font();
	}
	S = Canvas->ClipY / 1080.f;
	Clock += (float)FApp::GetDeltaTime();
	ATDGameMode* GM = GetWorld()->GetAuthGameMode<ATDGameMode>();
	if (!GM)
	{
		return;
	}
	switch (GM->GetState())
	{
	case ETDState::Title: DrawTitle(GM); break;
	case ETDState::HoleIntro: DrawIntro(GM); break;
	case ETDState::Aim:
	case ETDState::Throwing:
	case ETDState::Flight: DrawPlay(GM); break;
	case ETDState::Landed: DrawPlay(GM); DrawLanded(GM); break;
	case ETDState::HoleDone: DrawHoleDone(GM); break;
	case ETDState::Final: DrawFinal(GM); break;
	}
}

void ATDHUD::DrawTitle(ATDGameMode* GM)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	// Dark wash from the left so the title reads over the course.
	{
		TArray<FCanvasUVTri> T;
		const FLinearColor L(0.01f, 0.03f, 0.02f, 0.9f), R(0.01f, 0.03f, 0.02f, 0.f);
		FCanvasUVTri A, B;
		A.V0_Pos = FVector2D(0, 0); A.V1_Pos = FVector2D(W * 0.62f, 0); A.V2_Pos = FVector2D(W * 0.62f, H);
		A.V0_Color = L; A.V1_Color = R; A.V2_Color = R;
		B.V0_Pos = FVector2D(0, 0); B.V1_Pos = FVector2D(W * 0.62f, H); B.V2_Pos = FVector2D(0, H);
		B.V0_Color = L; B.V1_Color = R; B.V2_Color = L;
		T.Add(A); T.Add(B);
		Tris(T);
	}

	const float X = 110.f * S;
	ClockAt(X + 70.f * S, 200.f * S, 70.f * S, FLinearColor(0.85f, 0.1f, 0.07f), GM->GetClockMinutes());
	Text(TEXT("TIMER DISC"), X + 170.f * S, 128.f * S, 128.f, Accent, EAlign::Left, true, 5.f);
	Text(FString::Printf(TEXT("Tee off %s   -   Sunset %s"), *TD::ClockText(TD::TeeOffMinutes), *TD::ClockText(TD::SunsetMinutes)), X, 290.f * S, 30.f, Cream, EAlign::Left, true);
	Text(FString::Printf(TEXT("9 holes. Each throw takes %d minutes. Finish before sunset."), TD::MinutesPerStroke), X, 334.f * S, 24.f, Cream, EAlign::Left, false);

	// Controls
	float Y = 400.f * S;
	struct FRow { const TCHAR* Key; const TCHAR* What; };
	const FRow Rows[] = {
		{ TEXT("MOUSE"), TEXT("Aim left / right, arc up / down") },
		{ TEXT("HOLD CLICK"), TEXT("Charge, release to throw") },
		{ TEXT("A   D"), TEXT("Curve left / right") },
		{ TEXT("1  2  3"), TEXT("Pocket Watch, Wall Clock, Alarm Clock") },
		{ TEXT("C"), TEXT("Hold to see the whole hole") },
	};
	for (const FRow& Rw : Rows)
	{
		KeyCap(Rw.Key, X, Y, 44.f * S);
		Text(Rw.What, X + 230.f * S, Y + 8.f * S, 26.f, Cream, EAlign::Left, false);
		Y += 64.f * S;
	}

	const float Pulse = 0.85f + 0.15f * FMath::Sin(Clock * 4.f);
	Pill(TEXT("CLICK TO START"), X + 230.f * S, H - 170.f * S, 34.f, WithAlpha(Accent, Pulse), Ink);
	Text(TEXT("ESC to quit"), X, H - 70.f * S, 20.f, Muted, EAlign::Left, false);
}

void ATDHUD::DrawIntro(ATDGameMode* GM)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const FTDHole& Hole = GM->GetHole();
	const float PW = 1100.f * S, PH = 190.f * S;
	const float PX = (W - PW) * 0.5f, PY = H - PH - 70.f * S;
	RoundRect(PX, PY + 6.f * S, PW, PH, 26.f * S, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	RoundRect(PX, PY, PW, PH, 26.f * S, PanelBg);

	Badge(PX + 115.f * S, PY + PH * 0.5f, 74.f * S, TEXT("HOLE"), FString::FromInt(GM->GetHoleIndex() + 1));
	const float TX = PX + 250.f * S;
	Text(Hole.Name.ToUpper(), TX, PY + 30.f * S, 58.f, Cream, EAlign::Left, true, 3.f);
	// Chips: par, distance
	float CX = TX;
	for (const FString& Chip : { FString::Printf(TEXT("PAR %d"), Hole.Par), FString::Printf(TEXT("%d yd"), FMath::RoundToInt(GM->GetDistanceToBasket())), TD::ClockText(FMath::RoundToInt(GM->GetClockMinutes())) })
	{
		const FVector2D Sz = Measure(Chip, 22.f);
		RoundRect(CX, PY + 108.f * S, Sz.X + 32.f * S, 40.f * S, 20.f * S, FLinearColor(1.f, 1.f, 1.f, 0.12f));
		Text(Chip, CX + 16.f * S, PY + 115.f * S, 22.f, Accent, EAlign::Left, true);
		CX += Sz.X + 46.f * S;
	}
	const float Pulse = 0.85f + 0.15f * FMath::Sin(Clock * 4.f);
	Pill(TEXT("CLICK TO PLAY"), PX + PW - 140.f * S, PY - 70.f * S, 26.f, WithAlpha(Accent, Pulse), Ink);
}

void ATDHUD::DrawPlay(ATDGameMode* GM)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const FTDHole& Hole = GM->GetHole();
	const ETDState State = GM->GetState();

	WorldPill(GM->GetBasketLabelPoint(), FString::Printf(TEXT("%d yd"), FMath::RoundToInt(GM->GetDistanceToBasket())));

	// ---- Hole card (top-left) ----
	const float CardX = 28.f * S, CardY = 28.f * S, CardW = 470.f * S, CardH = 126.f * S;
	RoundRect(CardX, CardY, CardW, CardH, 22.f * S, PanelBg);
	Badge(CardX + 64.f * S, CardY + CardH * 0.5f, 44.f * S, FString(), FString::FromInt(GM->GetHoleIndex() + 1));
	Text(Hole.Name.ToUpper(), CardX + 126.f * S, CardY + 18.f * S, 30.f, Cream, EAlign::Left, true);
	Text(FString::Printf(TEXT("Par %d   -   %d yd to go"), Hole.Par, FMath::RoundToInt(GM->GetDistanceToBasket())), CardX + 126.f * S, CardY + 58.f * S, 20.f, Muted, EAlign::Left, false);
	const FString ThrowStr = FString::Printf(TEXT("THROW %d"), GM->GetStrokes() + (State == ETDState::Aim ? 1 : 0));
	const FString LieStr = FString(TDLieName(GM->GetLie())).ToUpper();
	float ChipX = CardX + 126.f * S;
	for (const FString& Chip : { ThrowStr, LieStr })
	{
		const FVector2D Sz = Measure(Chip, 16.f);
		RoundRect(ChipX, CardY + 88.f * S, Sz.X + 22.f * S, 26.f * S, 13.f * S, FLinearColor(1.f, 1.f, 1.f, 0.12f));
		Text(Chip, ChipX + 11.f * S, CardY + 92.f * S, 16.f, Accent, EAlign::Left, true);
		ChipX += Sz.X + 32.f * S;
	}

	// ---- Clock (top-right) ----
	{
		const float Now = GM->GetClockMinutes();
		const int32 Over = GM->GetTotalStrokes() - GM->GetTotalPar(true);
		const bool bLate = Now > TD::SunsetMinutes;
		const float BW = 330.f * S;
		const float SX = W - 28.f * S - BW;
		RoundRect(SX, CardY, BW, CardH, 22.f * S, PanelBg);
		ClockAt(SX + 66.f * S, CardY + CardH * 0.5f, 44.f * S, FLinearColor(0.85f, 0.1f, 0.07f), Now);
		const float TX = SX + 128.f * S;
		Text(TD::ClockText(FMath::RoundToInt(Now)), TX, CardY + 12.f * S, 40.f, bLate ? Bad : Cream, EAlign::Left, true);
		Text(bLate ? FString(TEXT("After sunset")) : FString::Printf(TEXT("Sunset %s"), *TD::ClockText(TD::SunsetMinutes)), TX, CardY + 62.f * S, 18.f, Muted, EAlign::Left, false);
		Text(PaceText(Over), TX, CardY + 90.f * S, 18.f, Over < 0 ? Good : (Over > 0 ? Bad : Cream), EAlign::Left, true);
	}

	if (State == ETDState::Flight)
	{
		Pill(TEXT("SPACE  -  skip"), W * 0.5f, H - 90.f * S, 20.f, PanelSoft, Cream);
		return;
	}
	if (State != ETDState::Aim)
	{
		return;
	}
	if (GM->IsOverview())
	{
		Pill(TEXT("HOLE OVERVIEW  -  let go of C"), W * 0.5f, 40.f * S, 22.f, PanelBg, Cream);
		return;
	}

	// ---- Tip (top-center) ----
	if (!Hole.Tip.IsEmpty())
	{
		Pill(Hole.Tip, W * 0.5f, 180.f * S, 22.f, PanelSoft, Cream);
	}

	// ---- Disc tray (bottom-left) ----
	const TArray<FTDDiscDef>& Discs = TDDiscs();
	static const TCHAR* Range[] = { TEXT("~30 yd"), TEXT("~50 yd"), TEXT("~75 yd") };
	float X = 28.f * S;
	for (int32 i = 0; i < Discs.Num(); ++i)
	{
		const bool bSel = GM->GetDisc() == i;
		const float CW = 200.f * S, CH = 150.f * S;
		const float Y = H - CH - 28.f * S - (bSel ? 14.f * S : 0.f);
		if (bSel)
		{
			RoundRect(X - 4.f * S, Y - 4.f * S, CW + 8.f * S, CH + 8.f * S, 24.f * S, Accent);
		}
		RoundRect(X, Y, CW, CH, 20.f * S, bSel ? FLinearColor(0.05f, 0.1f, 0.07f, 0.95f) : PanelSoft);
		ClockIcon(X + CW * 0.5f, Y + 48.f * S, 32.f * S, Discs[i].Rim, bSel ? Clock * 3.f : 0.f);
		Text(Discs[i].Name, X + CW * 0.5f, Y + 88.f * S, 22.f, bSel ? Accent : Cream, EAlign::Center, true);
		Text(FString::Printf(TEXT("%s  %s"), *Discs[i].Role, Range[FMath::Min(i, 2)]), X + CW * 0.5f, Y + 118.f * S, 16.f, Muted, EAlign::Center, false);
		// number key badge
		Circle(X + 22.f * S, Y + 22.f * S, 15.f * S, bSel ? Accent : FLinearColor(1.f, 1.f, 1.f, 0.2f));
		Text(FString::FromInt(i + 1), X + 22.f * S, Y + 11.f * S, 18.f, bSel ? Ink : Cream, EAlign::Center, true);
		X += CW + 16.f * S;
	}

	// ---- Shot shape (bottom-center) ----
	{
		const int32 C = GM->GetCurveStep();
		const float BW = 330.f * S, BH = 150.f * S;
		const float BX = W * 0.5f - BW * 0.5f + 120.f * S, BY = H - BH - 28.f * S;
		RoundRect(BX, BY, BW, BH, 20.f * S, PanelSoft);
		Text(TEXT("CURVE"), BX + 24.f * S, BY + 16.f * S, 16.f, Muted, EAlign::Left, true);
		FString CurveText = TEXT("Straight");
		if (C != 0)
		{
			CurveText = FString::Printf(TEXT("%s %s"), FMath::Abs(C) == 2 ? TEXT("Hard") : TEXT("Soft"), C < 0 ? TEXT("left") : TEXT("right"));
		}
		Text(CurveText, BX + BW - 24.f * S, BY + 14.f * S, 20.f, Cream, EAlign::Right, true);
		for (int32 i = -2; i <= 2; ++i)
		{
			const bool bOn = (C < 0 && i < 0 && i >= C) || (C > 0 && i > 0 && i <= C) || (C == 0 && i == 0);
			const float DX = BX + BW * 0.5f + i * 52.f * S;
			RoundRect(DX - 20.f * S, BY + 52.f * S, 40.f * S, 14.f * S, 7.f * S, bOn ? Accent : FLinearColor(1.f, 1.f, 1.f, 0.16f));
		}
		Text(TEXT("A"), BX + 24.f * S, BY + 47.f * S, 18.f, Muted, EAlign::Left, true);
		Text(TEXT("D"), BX + BW - 24.f * S, BY + 47.f * S, 18.f, Muted, EAlign::Right, true);
		Text(TEXT("ARC"), BX + 24.f * S, BY + 92.f * S, 16.f, Muted, EAlign::Left, true);
		Text(FString::Printf(TEXT("%d deg"), FMath::RoundToInt(GM->GetArc())), BX + BW - 24.f * S, BY + 88.f * S, 20.f, Cream, EAlign::Right, true);
		const float Frac = GM->GetArc() / 40.f;
		RoundRect(BX + 24.f * S, BY + 120.f * S, BW - 48.f * S, 8.f * S, 4.f * S, FLinearColor(1.f, 1.f, 1.f, 0.16f));
		RoundRect(BX + 24.f * S, BY + 120.f * S, FMath::Max(8.f * S, (BW - 48.f * S) * Frac), 8.f * S, 4.f * S, Accent);
	}

	// ---- Power meter (bottom-right) ----
	{
		const float MW = 54.f * S, MH = 330.f * S;
		const float MX = W - 28.f * S - 110.f * S, MY = H - MH - 70.f * S;
		RoundRect(MX - 28.f * S, MY - 54.f * S, MW + 56.f * S, MH + 110.f * S, 22.f * S, PanelSoft);
		RoundRect(MX, MY, MW, MH, MW * 0.5f, FLinearColor(0.f, 0.f, 0.f, 0.45f));
		const float P = GM->IsCharging() ? GM->GetPower() : 0.f;
		if (P > 0.01f)
		{
			const float FH = FMath::Max(MW, MH * P);
			const FLinearColor Top = FLinearColor::LerpUsingHSV(FLinearColor(0.35f, 0.95f, 0.35f), FLinearColor(1.f, 0.35f, 0.15f), P);
			RoundRectV(MX + 4.f * S, MY + MH - FH + 4.f * S, MW - 8.f * S, FH - 8.f * S, (MW - 8.f * S) * 0.5f, Top, FLinearColor(0.35f, 0.95f, 0.35f));
		}
		for (int32 i = 1; i < 4; ++i)
		{
			RoundRect(MX + MW + 4.f * S, MY + MH * i / 4.f - 1.f * S, 10.f * S, 2.f * S, 1.f * S, FLinearColor(1.f, 1.f, 1.f, 0.4f));
		}
		Text(GM->IsCharging() ? FString::Printf(TEXT("%d%%"), FMath::RoundToInt(P * 100.f)) : FString(TEXT("POWER")), MX + MW * 0.5f, MY - 40.f * S, 20.f,
			GM->IsCharging() ? Cream : Muted, EAlign::Center, true);
		Text(GM->IsCharging() ? TEXT("RELEASE") : TEXT("HOLD CLICK"), MX + MW * 0.5f, MY + MH + 16.f * S, 15.f, Muted, EAlign::Center, true);
	}
}

void ATDHUD::DrawLanded(ATDGameMode* GM)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	FString Big, Small;
	FLinearColor Color = Cream;
	switch (GM->GetLanding())
	{
	case ETDLanding::Basket:
		Big = TEXT("IN THE BASKET");
		Small = ATDGameMode::ScoreName(GM->GetStrokes(), GM->GetHole().Par);
		Color = Good;
		break;
	case ETDLanding::Water:
		Big = TEXT("WATER");
		Small = FString::Printf(TEXT("+1 stroke, +%d min"), TD::MinutesPerStroke);
		Color = WaterBlue;
		break;
	default:
		Big = FString::Printf(TEXT("%d yd"), FMath::RoundToInt(GM->GetLastThrowDistance()));
		Small = GM->LastHitTree() ? FString(TEXT("Hit a tree")) : FString(TDLieName(GM->GetLandingLie()));
		break;
	}
	const float Pop = FMath::Min(1.f, GM->GetStateTime() * 6.f);
	Text(Big, W * 0.5f, H * 0.24f, 110.f * (0.8f + 0.2f * Pop), Color, EAlign::Center, true, 6.f);
	Pill(Small, W * 0.5f, H * 0.24f + 150.f * S, 26.f, PanelBg, Cream);
}

void ATDHUD::DrawScorecard(ATDGameMode* GM, float CY, bool bHighlightCurrent)
{
	const float W = Canvas->ClipX;
	const TArray<FTDHole>& Holes = GM->GetHoles();
	const TArray<int32>& Scores = GM->GetScores();
	const float Cell = 84.f * S, Label = 130.f * S, RowH = 48.f * S;
	const float Total = Label + Cell * (Holes.Num() + 1);
	const float X0 = (W - Total) * 0.5f, Y0 = CY;
	RoundRect(X0 - 20.f * S, Y0 - 16.f * S, Total + 40.f * S, RowH * 4.f + 28.f * S, 20.f * S, FLinearColor(0.97f, 0.95f, 0.88f, 0.96f));
	RoundRect(X0 - 20.f * S, Y0 - 16.f * S, Total + 40.f * S, RowH + 16.f * S, 20.f * S, FLinearColor(0.1f, 0.3f, 0.12f, 1.f));
	const FLinearColor CardInk(0.08f, 0.12f, 0.08f);
	const FLinearColor Red(0.8f, 0.15f, 0.1f), Blue(0.1f, 0.3f, 0.75f);

	Text(TEXT("HOLE"), X0, Y0 + 10.f * S, 20.f, Cream, EAlign::Left, true);
	Text(TEXT("PAR"), X0, Y0 + RowH + 10.f * S, 20.f, CardInk, EAlign::Left, true);
	Text(TEXT("SCORE"), X0, Y0 + RowH * 2.f + 10.f * S, 20.f, CardInk, EAlign::Left, true);
	Text(TEXT("FINISHED"), X0, Y0 + RowH * 3.f + 10.f * S, 20.f, CardInk, EAlign::Left, true);
	int32 ParSum = 0, ScoreSum = 0;
	for (int32 i = 0; i <= Holes.Num(); ++i)
	{
		const float X = X0 + Label + i * Cell + Cell * 0.5f;
		const bool bTot = i == Holes.Num();
		if (!bTot && bHighlightCurrent && i == GM->GetHoleIndex())
		{
			RoundRect(X - Cell * 0.5f + 6.f * S, Y0 + RowH - 4.f * S, Cell - 12.f * S, RowH * 3.f, 12.f * S, FLinearColor(1.f, 0.74f, 0.2f, 0.35f));
		}
		Text(bTot ? FString(TEXT("TOT")) : FString::FromInt(i + 1), X, Y0 + 10.f * S, 22.f, bTot ? Accent : Cream, EAlign::Center, true);
		const int32 Par = bTot ? ParSum : Holes[i].Par;
		if (!bTot) ParSum += Par;
		Text(FString::FromInt(bTot ? ParSum : Par), X, Y0 + RowH + 10.f * S, 22.f, CardInk, EAlign::Center, false);
		const int32 FinishedAt = TD::TeeOffMinutes + ScoreSum * TD::MinutesPerStroke;
		if (bTot)
		{
			Text(FString::FromInt(ScoreSum), X, Y0 + RowH * 2.f + 8.f * S, 26.f, CardInk, EAlign::Center, true);
			Text(TD::ClockShort(FinishedAt), X, Y0 + RowH * 3.f + 10.f * S, 20.f, FinishedAt > TD::SunsetMinutes ? Red : CardInk, EAlign::Center, true);
		}
		else if (Scores[i] >= 0)
		{
			const int32 D = Scores[i] - Par;
			const float SY = Y0 + RowH * 2.f + RowH * 0.5f;
			// Golf scorecard convention: circle = under par, square = over par.
			if (D < 0)
			{
				Ring(X, SY, 19.f * S, 2.5f * S, Red);
				if (D < -1) Ring(X, SY, 14.f * S, 2.f * S, Red);
			}
			else if (D > 0)
			{
				const float B = 18.f * S;
				Line(X - B, SY - B, X + B, SY - B, 2.5f * S, Blue);
				Line(X + B, SY - B, X + B, SY + B, 2.5f * S, Blue);
				Line(X + B, SY + B, X - B, SY + B, 2.5f * S, Blue);
				Line(X - B, SY + B, X - B, SY - B, 2.5f * S, Blue);
			}
			Text(FString::FromInt(Scores[i]), X, Y0 + RowH * 2.f + 9.f * S, 24.f, D < 0 ? Red : (D > 0 ? Blue : CardInk), EAlign::Center, true);
			ScoreSum += Scores[i];
			const int32 HoleDoneAt = TD::TeeOffMinutes + ScoreSum * TD::MinutesPerStroke;
			Text(TD::ClockShort(HoleDoneAt), X, Y0 + RowH * 3.f + 11.f * S, 19.f, HoleDoneAt > TD::SunsetMinutes ? Red : CardInk, EAlign::Center, false);
		}
	}
}

void ATDHUD::DrawHoleDone(ATDGameMode* GM)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	RoundRect(0, 0, W, H, 0.f, FLinearColor(0.f, 0.02f, 0.01f, 0.45f));
	const int32 Sc = GM->GetScores()[GM->GetHoleIndex()];
	const int32 Par = GM->GetHole().Par;
	Text(ATDGameMode::ScoreName(Sc, Par).ToUpper(), W * 0.5f, H * 0.12f, 120.f, Sc < Par ? Good : (Sc > Par ? Bad : Accent), EAlign::Center, true, 6.f);
	Pill(FString::Printf(TEXT("Hole %d  -  %s  -  %d throw%s on a par %d  -  %s"), GM->GetHoleIndex() + 1, *GM->GetHole().Name, Sc, Sc == 1 ? TEXT("") : TEXT("s"), Par, *TD::ClockText(GM->GetFinishMinutes())),
		W * 0.5f, H * 0.12f + 160.f * S, 26.f, PanelBg, Cream);
	DrawScorecard(GM, H * 0.47f, true);
	const bool bLast = GM->GetHoleIndex() + 1 >= GM->GetHoleCount();
	const float Pulse = 0.85f + 0.15f * FMath::Sin(Clock * 4.f);
	Pill(bLast ? FString(TEXT("CLICK TO CONTINUE")) : FString::Printf(TEXT("CLICK FOR HOLE %d"), GM->GetHoleIndex() + 2), W * 0.5f, H * 0.8f, 28.f,
		WithAlpha(Accent, Pulse), Ink);
}

void ATDHUD::DrawFinal(ATDGameMode* GM)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	RoundRect(0, 0, W, H, 0.f, FLinearColor(0.f, 0.02f, 0.01f, 0.6f));
	const int32 Finish = GM->GetFinishMinutes();
	const int32 Margin = TD::SunsetMinutes - Finish;
	const bool bBeat = Margin >= 0;
	Text(bBeat ? TEXT("BEAT SUNSET") : TEXT("MISSED SUNSET"), W * 0.5f, H * 0.09f, 100.f, bBeat ? Accent : Bad, EAlign::Center, true, 5.f);
	Text(FString::Printf(TEXT("Finished %s"), *TD::ClockText(Finish)), W * 0.5f, H * 0.09f + 135.f * S, 54.f, Cream, EAlign::Center, true, 3.f);
	const FString Gap = Margin == 0 ? FString(TEXT("Right at sunset"))
		: FString::Printf(TEXT("%d min %s sunset"), FMath::Abs(Margin), Margin > 0 ? TEXT("before") : TEXT("after"));
	Pill(FString::Printf(TEXT("%s   -   %d throws, par %d"), *Gap, GM->GetTotalStrokes(), GM->GetTotalPar(false)), W * 0.5f, H * 0.09f + 215.f * S, 24.f, PanelBg, Cream);
	DrawScorecard(GM, H * 0.47f, false);
	const float Pulse = 0.85f + 0.15f * FMath::Sin(Clock * 4.f);
	Pill(TEXT("CLICK TO PLAY AGAIN"), W * 0.5f, H * 0.82f, 28.f, WithAlpha(Accent, Pulse), Ink);
}
