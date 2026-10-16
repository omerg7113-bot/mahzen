#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ModAtmosferi.generated.h"

class ULightComponent;
class UDirectionalLightComponent;
class USceneComponent;

/**
 * Kızın moduna göre sahnenin atmosferini değiştirir. Mod duyurusu widget'ının
 * yerini alır: oyuncu hangi modda olduğunu yazıdan değil ortamdan anlar.
 *
 *   Hayalet : güneş (DirectionalLight) belirgin şekilde parlar, ortalık aydınlanır
 *   Takipçi : her şey normal (sahnenin kendi değerleri)
 *   Agresif : bütün kırmızı ışıklar hızlı hızlı yanıp söner + ekranda hafif titreme
 *
 * Modu `kızyurume`nin `GuncelModAdi` değişkeninden reflection ile okur, yani
 * Blueprint tarafında hiçbir bağlantı gerekmez; sahneye bir tane koymak yeter.
 *
 * Kırmızı ışıklar renge göre otomatik bulunur (R yüksek, G ve B düşük); istersen
 * aktöre `KirmiziLamba` etiketi vererek de dahil edebilirsin.
 */
UCLASS()
class MYPROJECT2_API AModAtmosferi : public AActor
{
	GENERATED_BODY()

public:
	AModAtmosferi();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Mod adları: kızyurume'deki Ad_* değişkenleriyle BİREBİR aynı olmalı ---
	UPROPERTY(EditAnywhere, Category = "Mod")
	FString AdHayalet = TEXT("Hayalet");

	UPROPERTY(EditAnywhere, Category = "Mod")
	FString AdAgresif = TEXT("Agresif");

	/**
	 * Hayalet modunda güneşin (DirectionalLight) yoğunluğu — ÇARPAN DEĞİL,
	 * doğrudan Intensity değeri. Takipçi 1, Agresif 0 (bunlar .cpp'de sabit).
	 */
	UPROPERTY(EditAnywhere, Category = "Hayalet", meta = (ClampMin = "0.0"))
	float HayaletGunesYogunlugu = 2.0f;

	/** Hayalet/normal geçişinin yumuşaklığı (saniyede yaklaşma oranı). */
	UPROPERTY(EditAnywhere, Category = "Hayalet", meta = (ClampMin = "0.1"))
	float GecisHizi = 2.5f;

	/** Agresif modda kırmızı ışıkların saniyedeki yanıp sönme sayısı. */
	UPROPERTY(EditAnywhere, Category = "Agresif", meta = (ClampMin = "0.5"))
	float AgresifFrekans = 9.0f;

	/** Sönük anındaki çarpan (0 = tamamen söner). */
	UPROPERTY(EditAnywhere, Category = "Agresif", meta = (ClampMin = "0.0"))
	float AgresifSonukCarpan = 0.05f;

	/** Parlak anındaki çarpan. */
	UPROPERTY(EditAnywhere, Category = "Agresif", meta = (ClampMin = "1.0"))
	float AgresifParlakCarpan = 3.0f;

	/** Ekran titremesinin genliği (santimetre). 0 = titreme yok. */
	UPROPERTY(EditAnywhere, Category = "Agresif", meta = (ClampMin = "0.0"))
	float TitremeGenligi = 1.6f;

	/** Ekran titremesinin hızı. */
	UPROPERTY(EditAnywhere, Category = "Agresif", meta = (ClampMin = "0.1"))
	float TitremeHizi = 26.0f;

	/** Agresif modda el fenerinin parlaklık çarpanı (1 = değişmez). */
	UPROPERTY(EditAnywhere, Category = "Agresif", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AgresifFenerParlaklik = 0.75f;

	/** Agresif modda el fenerinin koni açısı çarpanı (1 = değişmez). */
	UPROPERTY(EditAnywhere, Category = "Agresif", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float AgresifFenerKoni = 0.85f;

	/** Bir ışığın "kırmızı" sayılması için gereken en düşük R ve en yüksek G/B. */
	UPROPERTY(EditAnywhere, Category = "Kırmızı ışıklar")
	int32 KirmiziEnAzR = 180;

	UPROPERTY(EditAnywhere, Category = "Kırmızı ışıklar")
	int32 KirmiziEnCokGB = 100;

private:
	struct FIsikKaydi
	{
		TWeakObjectPtr<ULightComponent> Isik;
		float TabanYogunluk = 0.f;
	};

	/** Kızın `GuncelModAdi` değerini reflection ile okur; bulunamazsa boş döner. */
	FString ModuOku() const;

	void KameraOfsetiUygula(const FVector& Ofset);

	TArray<FIsikKaydi> KirmiziIsiklar;

	TWeakObjectPtr<ULightComponent> Gunes;
	float GunesTabanYogunluk = 0.f;
	/** Güneşin o anki çarpanı; modlar arası yumuşak geçsin diye tutuluyor. */
	float GunesCarpaniSuanki = 1.0f;

	TWeakObjectPtr<AActor> Kiz;
	TWeakObjectPtr<USceneComponent> OyuncuKamera;
	FVector KameraTabanKonum = FVector::ZeroVector;
	bool bKameraOynandi = false;

	/** El feneri ve oyun başındaki değerleri. */
	TWeakObjectPtr<class USpotLightComponent> Fener;
	float FenerTabanYogunluk = 0.f;
	float FenerTabanIcKoni = 0.f;
	float FenerTabanDisKoni = 0.f;

	/**
	 * Agresif modun 0..1 arası şiddeti. Moda girerken 1'e, çıkarken 0'a doğru
	 * yumuşak gider; kırmızı çakma, fener kısılması ve titreme hep bununla
	 * ölçeklenir, böylece mod geçişlerinde ani sıçrama olmaz.
	 */
	float AgresifSiddet = 0.f;

	float Zaman = 0.f;
};
