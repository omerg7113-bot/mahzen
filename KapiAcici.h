#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KapiAcici.generated.h"

/**
 * Kızın, oyuncunun kilidini açtığı kapıları kendi başına açmasını sağlar.
 *
 * Neden gerekti: `bp_kapı.KapiyiKullan` ilk düğümde `Cast To
 * BP_FirstPersonCharacter` yapıyor, yani kapıyı YALNIZCA oyuncu açabiliyordu.
 * Kız kapalı kapıya dayanıp duruyordu (PIE'de ölçüldü: 11 dakikada 41 kapının
 * hiçbiri açılmadı, sıkışma sayacı 788 kez arttı, 2. kata hiç çıkamadı).
 * Çözüm: bp_kapı'ya kıza özel `KizAc` olayı eklendi (gövdesi kapının kendi
 * açılma dizisi: zaman çizelgesi + ses + NavDurumGuncelle + kıza ses konumu),
 * bu bileşen de yakındaki uygun kapılarda onu çağırıyor.
 *
 * Kurallar: kilidi açılmamış (bKilidiAcildi == false) ya da mühürlenmiş
 * (bMuhurlenmis == true) kapıya DOKUNMAZ — yani kız yalnızca oyuncunun açtığı
 * yerleri gezebilir, mühürlü odalara giremez.
 *
 * Kıza çalışma anında `UMahzenHaritaAltSistemi::OnWorldBeginPlay` içinde
 * eklenir; Blueprint'e dokunmak gerekmez.
 */
UCLASS(ClassGroup = (Mahzen), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UKapiAcici : public UActorComponent
{
	GENERATED_BODY()

public:
	UKapiAcici();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Normal halde bu yarıçaptaki uygun kapılar açılır (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|KapiAcici")
	float Yaricap = 180.f;

	/** Takılma algılandığında yarıçap buna genişler (kapı eşiğinde dayanma). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|KapiAcici")
	float TakilmaYaricapi = 340.f;

	/** Kaç saniyede bir bakılacak. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|KapiAcici")
	float Aralik = 0.25f;

	/** Bu süre boyunca TakilmaEsigi'nden az ilerlerse takılmış sayılır. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|KapiAcici")
	float TakilmaSuresi = 2.5f;

	/** Takılma eşiği (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|KapiAcici")
	float TakilmaEsigi = 50.f;

	/** Aynı kattaki kapılarla ilgilen: |dz| bundan büyükse atla. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|KapiAcici")
	float KatToleransi = 200.f;

	/** Ölçüm: bu oyunda kaç kapı açıldı. */
	UPROPERTY(BlueprintReadOnly, Category = "Mahzen|KapiAcici")
	int32 AcilanKapi = 0;

	/** Ölçüm: kaç kez takılma algılandı. */
	UPROPERTY(BlueprintReadOnly, Category = "Mahzen|KapiAcici")
	int32 TakilmaSayisi = 0;

	/** Ölçüm: en son açılan kapı. */
	UPROPERTY(BlueprintReadOnly, Category = "Mahzen|KapiAcici")
	FString SonKapi;

	/** Ölçüm: şu an takılmış mı. */
	UPROPERTY(BlueprintReadOnly, Category = "Mahzen|KapiAcici")
	bool bTakildi = false;

	/**
	 * Takılma kurtarması: yakında açılacak kapı yokken bu kadar saniye
	 * ilerlemezse kızın gezinti çıpası sıfırlanır (BP'deki `GezintiSayaci`
	 * eşiğin üstüne çekiliyor, böylece bir sonraki karar yeni bir hedef seçer).
	 * BP'nin kendi `SikismaSayaci` sayacı artıyordu ama hiçbir şey yapmıyordu.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|KapiAcici")
	bool bKurtarmaAcik = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|KapiAcici")
	float KurtarmaSuresi = 6.f;

	UPROPERTY(BlueprintReadOnly, Category = "Mahzen|KapiAcici")
	int32 KurtarmaSayisi = 0;

private:
	void KapilariTopla();
	bool KapiyiAc(AActor* Kapi);

	TArray<TWeakObjectPtr<AActor>> Kapilar;
	float Sayac = 0.f;
	FVector OlcumKonumu = FVector::ZeroVector;
	float DurgunlukSuresi = 0.f;
};
