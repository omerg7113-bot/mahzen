// Fill out your copyright notice in the Description page of Project Settings.

#include "AudioFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAudioFunctionLibrary, Log, All);

void UAudioFunctionLibrary::PlayRandomFromPool(UObject* WorldContextObject, const TArray<FString>& AssetPaths, FVector Location, float PitchMin, float PitchMax, float Volume)
{
	if (AssetPaths.Num() == 0)
	{
		return;
	}

	const int32 Index = FMath::RandRange(0, AssetPaths.Num() - 1);
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *AssetPaths[Index]);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::RandRange(PitchMin, PitchMax);
	UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Sound, Location, Volume, Pitch);
}

void UAudioFunctionLibrary::PlayFootstepSound(UObject* WorldContextObject, FVector Location)
{
	const TArray<FString> Pool = {
		TEXT("/Game/Audio/SFX/footstep_wood_000.footstep_wood_000"),
		TEXT("/Game/Audio/SFX/footstep_wood_001.footstep_wood_001"),
		TEXT("/Game/Audio/SFX/footstep_wood_002.footstep_wood_002"),
		TEXT("/Game/Audio/SFX/footstep_wood_003.footstep_wood_003"),
		TEXT("/Game/Audio/SFX/footstep_wood_004.footstep_wood_004"),
	};
	PlayRandomFromPool(WorldContextObject, Pool, Location, 0.92f, 1.08f, 0.55f);
}

void UAudioFunctionLibrary::PlayDoorOpenSound(UObject* WorldContextObject, FVector Location)
{
	const TArray<FString> Pool = {
		TEXT("/Game/Audio/SFX/doorOpen_1.doorOpen_1"),
		TEXT("/Game/Audio/SFX/doorOpen_2.doorOpen_2"),
	};
	PlayRandomFromPool(WorldContextObject, Pool, Location, 0.97f, 1.03f, 0.45f);
}

void UAudioFunctionLibrary::PlayDoorCloseSound(UObject* WorldContextObject, FVector Location)
{
	const TArray<FString> Pool = {
		TEXT("/Game/Audio/SFX/doorClose_1.doorClose_1"),
		TEXT("/Game/Audio/SFX/doorClose_2.doorClose_2"),
	};
	PlayRandomFromPool(WorldContextObject, Pool, Location, 0.97f, 1.03f, 0.45f);
}

void UAudioFunctionLibrary::PlayKeyPickupSound(UObject* WorldContextObject, FVector Location)
{
	const TArray<FString> Pool = {
		TEXT("/Game/Audio/SFX/metalClick.metalClick"),
		TEXT("/Game/Audio/SFX/handleCoins.handleCoins"),
		TEXT("/Game/Audio/SFX/handleCoins2.handleCoins2"),
	};
	PlayRandomFromPool(WorldContextObject, Pool, Location, 0.95f, 1.05f, 1.0f);
}

void UAudioFunctionLibrary::PlayKeyDropSound(UObject* WorldContextObject, FVector Location)
{
	const TArray<FString> Pool = {
		TEXT("/Game/Audio/SFX/impactMetal_light_000.impactMetal_light_000"),
		TEXT("/Game/Audio/SFX/impactMetal_light_001.impactMetal_light_001"),
		TEXT("/Game/Audio/SFX/impactMetal_medium_000.impactMetal_medium_000"),
		TEXT("/Game/Audio/SFX/impactMetal_medium_001.impactMetal_medium_001"),
	};
	PlayRandomFromPool(WorldContextObject, Pool, Location, 0.95f, 1.05f, 1.0f);
}

void UAudioFunctionLibrary::PlayGenericPickupSound(UObject* WorldContextObject, FVector Location)
{
	const TArray<FString> Pool = {
		TEXT("/Game/840213__anskukisu64__picking-up-keys__1_.840213__anskukisu64__picking-up-keys__1_"),
	};

	if (Pool.Num() == 0)
	{
		return;
	}

	const int32 Index = FMath::RandRange(0, Pool.Num() - 1);
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Pool[Index]);
	if (!Sound)
	{
		return;
	}

	// Played as a 2D UI-style cue (not 3D-attenuated) so it's always clearly audible, regardless of distance.
	UGameplayStatics::PlaySound2D(WorldContextObject, Sound, 1.3f, 1.0f);
}

// ─── Tek varlık / katmanlı çalma yardımcıları ────────────────────────────────

FVector UAudioFunctionLibrary::BaglamKonumu(UObject* WorldContextObject)
{
	if (const AActor* Sahip = Cast<AActor>(WorldContextObject))
	{
		return Sahip->GetActorLocation();
	}
	return FVector::ZeroVector;
}

void UAudioFunctionLibrary::Cal(UObject* WorldContextObject, const FString& AssetPath, float Volume, float Pitch, bool b2D, FVector Location)
{
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *AssetPath);
	if (!Sound)
	{
		UE_LOG(LogAudioFunctionLibrary, Warning, TEXT("Ses bulunamadi: %s"), *AssetPath);
		return;
	}

	if (b2D)
	{
		UGameplayStatics::PlaySound2D(WorldContextObject, Sound, Volume, Pitch);
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Sound, Location, Volume, Pitch);
	}
}

void UAudioFunctionLibrary::GecikmeliCal(UObject* WorldContextObject, const FString& AssetPath, float Gecikme, float Volume, float Pitch, bool b2D, FVector Location)
{
	UWorld* Dunya = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!Dunya || Gecikme <= 0.0f)
	{
		// Zamanlayıcı kurulamıyorsa ses hiç kaybolmasın, hemen çalsın.
		Cal(WorldContextObject, AssetPath, Volume, Pitch, b2D, Location);
		return;
	}

	// Handle yerel olabilir; zamanlayıcı yine kurulur (handle yalnızca iptal içindir).
	FTimerHandle Handle;
	Dunya->GetTimerManager().SetTimer(Handle,
		FTimerDelegate::CreateWeakLambda(WorldContextObject,
			[WorldContextObject, AssetPath, Volume, Pitch, b2D, Location]()
			{
				Cal(WorldContextObject, AssetPath, Volume, Pitch, b2D, Location);
			}),
		Gecikme, false);
}

// ─── Oyun olaylarının sesleri ────────────────────────────────────────────────

void UAudioFunctionLibrary::PlayKasaAcilmaSesi(UObject* WorldContextObject)
{
	const FVector Konum = BaglamKonumu(WorldContextObject);
	// 1) ağır kilit dili, 2) biraz sonra gıcırdayarak açılan kapak
	Cal(WorldContextObject, TEXT("/Game/Audio/SFX/metalLatch.metalLatch"), 1.0f, 0.78f, false, Konum);
	GecikmeliCal(WorldContextObject, TEXT("/Game/Audio/SFX/doorOpen_2.doorOpen_2"), 0.22f, 0.8f, 0.80f, false, Konum);
}

void UAudioFunctionLibrary::PlayMuhurlemeSesi(UObject* WorldContextObject)
{
	const FVector Konum = BaglamKonumu(WorldContextObject);
	// 1) kilidin çevrilmesi, 2) üstüne oturan ağır sürgü
	Cal(WorldContextObject, TEXT("/Game/ResidentHorrorV1/Audio/WavFiles/WAV_door-lock-43124.WAV_door-lock-43124"), 1.1f, 0.92f, false, Konum);
	GecikmeliCal(WorldContextObject, TEXT("/Game/Audio/SFX/metalLatch.metalLatch"), 0.30f, 0.9f, 0.72f, false, Konum);
}

void UAudioFunctionLibrary::PlayEnvanterAcilmaSesi(UObject* WorldContextObject)
{
	const TArray<FString> Havuz = {
		TEXT("/Game/assetler/Notlar/Sesler/bookFlip1.bookFlip1"),
		TEXT("/Game/assetler/Notlar/Sesler/bookFlip2.bookFlip2"),
	};
	const int32 Index = FMath::RandRange(0, Havuz.Num() - 1);
	Cal(WorldContextObject, Havuz[Index], 0.8f, FMath::RandRange(0.96f, 1.04f), true, FVector::ZeroVector);
}

void UAudioFunctionLibrary::PlayEnvanterKapanmaSesi(UObject* WorldContextObject)
{
	Cal(WorldContextObject, TEXT("/Game/assetler/Notlar/Sesler/bookFlip3.bookFlip3"), 0.8f, 0.90f, true, FVector::ZeroVector);
}

void UAudioFunctionLibrary::PlayFenerSesi(UObject* WorldContextObject, bool bAcik)
{
	// Aynı tık; açılış tiz ve net, kapanış boğuk. Elimizde olduğu için 2D.
	Cal(WorldContextObject, TEXT("/Game/Audio/SFX/metalClick.metalClick"),
		bAcik ? 0.7f : 0.6f, bAcik ? 1.15f : 0.85f, true, FVector::ZeroVector);
}
