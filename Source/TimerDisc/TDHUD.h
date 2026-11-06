#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Fonts/SlateFontInfo.h"
#include "TDHUD.generated.h"

class ATDGameMode;
class UFont;
struct FCanvasUVTri;

UCLASS()
class ATDHUD : public AHUD
{
	GENERATED_BODY()

public:
	void DrawHUD() override;

private:
	UPROPERTY() TObjectPtr<UFont> Font;
	float S = 1.f;     // UI scale (1080p reference)
	float Clock = 0.f;

	enum class EAlign : uint8 { Left, Center, Right };

	// ---- Drawing kit ----
	FSlateFontInfo MakeFont(float SizePx, bool bBold, float Outline) const;
	FVector2D Measure(const FString& Text, float SizePx, bool bBold = true);
	void Text(const FString& Text, float X, float Y, float SizePx, const FLinearColor& Color, EAlign Align = EAlign::Left, bool bBold = true, float Outline = 0.f);
	void Tris(TArray<FCanvasUVTri>& T);
	void AddQuad(TArray<FCanvasUVTri>& T, const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D, const FLinearColor& CA, const FLinearColor& CB);
	void RoundRect(float X, float Y, float W, float H, float R, const FLinearColor& Color);
	void RoundRectV(float X, float Y, float W, float H, float R, const FLinearColor& Top, const FLinearColor& Bottom);
	void Circle(float CX, float CY, float R, const FLinearColor& Color);
	void Ring(float CX, float CY, float R, float Thick, const FLinearColor& Color);
	void Line(float X1, float Y1, float X2, float Y2, float Thick, const FLinearColor& Color);
	void Pill(const FString& Label, float CX, float Y, float SizePx, const FLinearColor& Bg, const FLinearColor& Fg);
	void ClockFace(float CX, float CY, float R, const FLinearColor& Rim, float HourAngle, float MinuteAngle);
	void ClockIcon(float CX, float CY, float R, const FLinearColor& Rim, float HandsAngle);
	void ClockAt(float CX, float CY, float R, const FLinearColor& Rim, float Minutes);
	void Badge(float CX, float CY, float R, const FString& Top, const FString& Number);
	void KeyCap(const FString& Key, float X, float Y, float H);
	void WorldPill(const FVector& World, const FString& Label);

	// ---- Screens ----
	void DrawTitle(ATDGameMode* GM);
	void DrawIntro(ATDGameMode* GM);
	void DrawPlay(ATDGameMode* GM);
	void DrawLanded(ATDGameMode* GM);
	void DrawScorecard(ATDGameMode* GM, float CY, bool bHighlightCurrent);
	void DrawHoleDone(ATDGameMode* GM);
	void DrawFinal(ATDGameMode* GM);
};
