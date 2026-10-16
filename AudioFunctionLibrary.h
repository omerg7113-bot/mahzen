// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AudioFunctionLibrary.generated.h"

UCLASS()
class MYPROJECT2_API UAudioFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlayFootstepSound(UObject* WorldContextObject, FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlayDoorOpenSound(UObject* WorldContextObject, FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlayDoorCloseSound(UObject* WorldContextObject, FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlayKeyPickupSound(UObject* WorldContextObject, FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlayKeyDropSound(UObject* WorldContextObject, FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlayGenericPickupSound(UObject* WorldContextObject, FVector Location);

	// --- Aşağıdaki sesler konum pini istemez: WorldContext (çağıran Actor) kendisi
	// konum kaynağıdır, böylece Blueprint'te tek pinsiz düğüm olarak durur. ---

	// Kasa açılması: ağır kilit dili + gıcırdayan kapak (iki katman, gecikmeli).
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlayKasaAcilmaSesi(UObject* WorldContextObject);

	// Oda mühürlenmesi: kilit çevirme + ağır sürgü.
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlayMuhurlemeSesi(UObject* WorldContextObject);

	// Envanter açılışı / kapanışı (2D arayüz sesi).
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlayEnvanterAcilmaSesi(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlayEnvanterKapanmaSesi(UObject* WorldContextObject);

	// Fener düğmesi. bAcik=true açılış tıkı, false kapanış tıkı (daha boğuk).
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlayFenerSesi(UObject* WorldContextObject, bool bAcik);

private:
	static void PlayRandomFromPool(UObject* WorldContextObject, const TArray<FString>& AssetPaths, FVector Location, float PitchMin, float PitchMax, float Volume);

	// Tek varlık yolunu 3B (konumlu) ya da 2B olarak çalar.
	static void Cal(UObject* WorldContextObject, const FString& AssetPath, float Volume, float Pitch, bool b2D, FVector Location);

	// Aynısını Gecikme saniye sonra çalar; katmanlı sesler için.
	static void GecikmeliCal(UObject* WorldContextObject, const FString& AssetPath, float Gecikme, float Volume, float Pitch, bool b2D, FVector Location);

	// WorldContext bir Actor ise konumu, değilse sıfır vektör.
	static FVector BaglamKonumu(UObject* WorldContextObject);
};
