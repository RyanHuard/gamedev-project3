#include "TDAssets.h"

#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Font.h"
#include "Animation/AnimSequence.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	template <typename T>
	T* LoadCached(const TCHAR* Path)
	{
		// Loaded objects are rooted so the cache never dangles.
		static TMap<FString, TWeakObjectPtr<UObject>> Cache;
		if (TWeakObjectPtr<UObject>* Found = Cache.Find(Path))
		{
			if (Found->IsValid())
			{
				return Cast<T>(Found->Get());
			}
		}
		T* Obj = LoadObject<T>(nullptr, Path);
		if (Obj)
		{
			Obj->AddToRoot();
			Cache.Add(Path, Obj);
		}
		return Obj;
	}

	UMaterialInterface* ColorBase()
	{
		if (UMaterialInterface* M = LoadCached<UMaterialInterface>(TEXT("/Game/TimerDisc/Materials/M_TDColor.M_TDColor")))
		{
			return M;
		}
		return LoadCached<UMaterialInterface>(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	UMaterialInterface* GlowBase(bool bTranslucent)
	{
		const TCHAR* Path = bTranslucent
			? TEXT("/Game/TimerDisc/Materials/M_TDGlowTranslucent.M_TDGlowTranslucent")
			: TEXT("/Game/TimerDisc/Materials/M_TDGlow.M_TDGlow");
		if (UMaterialInterface* M = LoadCached<UMaterialInterface>(Path))
		{
			return M;
		}
		return ColorBase();
	}
}

namespace TDAssets
{
	UStaticMesh* Cube() { return LoadCached<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube.Cube")); }
	UStaticMesh* Sphere() { return LoadCached<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere.Sphere")); }
	UStaticMesh* Cylinder() { return LoadCached<UStaticMesh>(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); }

	USkeletalMesh* Mannequin()
	{
		return LoadCached<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	}

	UClass* MannequinAnimClass()
	{
		return LoadCached<UClass>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
	}

	UAnimSequence* ThrowAnim()
	{
		return LoadCached<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
	}

	UAnimSequence* CheerAnim()
	{
		return LoadCached<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump.MM_Jump"));
	}

	UAnimSequence* IdleAnim()
	{
		return LoadCached<UAnimSequence>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	}

	USoundBase* BreezeSound() { return LoadCached<USoundBase>(TEXT("/Game/TimerDisc/Audio/S_Breeze.S_Breeze")); }
	USoundBase* ThrowSound() { return LoadCached<USoundBase>(TEXT("/Game/TimerDisc/Audio/S_Throw.S_Throw")); }
	USoundBase* LandSound() { return LoadCached<USoundBase>(TEXT("/Game/TimerDisc/Audio/S_Land.S_Land")); }
	USoundBase* SplashSound() { return LoadCached<USoundBase>(TEXT("/Game/TimerDisc/Audio/S_Splash.S_Splash")); }
	USoundBase* ChainsSound() { return LoadCached<USoundBase>(TEXT("/Game/TimerDisc/Audio/S_Chains.S_Chains")); }

	UFont* Font()
	{
		if (UFont* F = LoadCached<UFont>(TEXT("/Engine/EngineFonts/Roboto.Roboto")))
		{
			return F;
		}
		return LoadCached<UFont>(TEXT("/Engine/EngineFonts/RobotoDistanceField.RobotoDistanceField"));
	}

	UMaterialInstanceDynamic* MakeColor(UObject* Outer, const FLinearColor& Color, float Roughness)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ColorBase(), Outer);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		return MID;
	}

	UMaterialInstanceDynamic* MakeGlow(UObject* Outer, const FLinearColor& Color, float Opacity)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(GlowBase(Opacity < 0.999f), Outer);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
		return MID;
	}

	UStaticMeshComponent* AddMesh(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh, UMaterialInterface* Mat,
		const FTransform& Relative, FName Socket)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
		C->SetStaticMesh(Mesh);
		if (Mat)
		{
			C->SetMaterial(0, Mat);
		}
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		C->SetGenerateOverlapEvents(false);
		C->SetupAttachment(Parent, Socket);
		C->SetRelativeTransform(Relative);
		C->RegisterComponent();
		Owner->AddInstanceComponent(C);
		return C;
	}
}
