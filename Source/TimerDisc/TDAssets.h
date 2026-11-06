#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class USkeletalMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UAnimSequence;
class UFont;
class UStaticMeshComponent;
class USoundBase;

/** Runtime asset lookups. Everything the game needs is loaded by path so the whole game can be built from code. */
namespace TDAssets
{
	UStaticMesh* Cube();
	UStaticMesh* Sphere();
	UStaticMesh* Cylinder();

	USkeletalMesh* Mannequin();
	UClass* MannequinAnimClass();
	UAnimSequence* ThrowAnim();
	UAnimSequence* CheerAnim();
	UAnimSequence* IdleAnim();

	UFont* Font();

	USoundBase* BreezeSound();
	USoundBase* ThrowSound();
	USoundBase* LandSound();
	USoundBase* SplashSound();
	USoundBase* ChainsSound();

	/** Lit, solid color material. */
	UMaterialInstanceDynamic* MakeColor(UObject* Outer, const FLinearColor& Color, float Roughness = 0.7f);
	/** Unlit glowing material (optionally translucent). */
	UMaterialInstanceDynamic* MakeGlow(UObject* Outer, const FLinearColor& Color, float Opacity = 1.f);

	/** Convenience: make a static mesh component on Owner, attached to Parent. */
	UStaticMeshComponent* AddMesh(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh, UMaterialInterface* Mat,
		const FTransform& Relative, FName Socket = NAME_None);
}
