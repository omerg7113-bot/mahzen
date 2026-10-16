#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DegiskenYansitici.generated.h"

/**
 * Sahibi olan aktörün Blueprint değişkenlerini sunucudan istemcilere yansıtır.
 *
 * Neden: Blueprint değişkenlerinin "replike" bayrağı editör betiğinden
 * (MCP/Python) ayarlanamıyor; bu bileşen aynı işi yansıma (reflection) ile
 * yapar. Sunucu `Degiskenler` listesindeki değişkenleri metne çevirip
 * `Durum`a yazar (değişince), istemciler OnRep'te aynı metni değişkenlere geri
 * yazar. Böylece Blueprint mantığı (örn. kapının Tick'i `bAcik`a bakarak
 * animasyon yapar) hiç değişmeden çalışır.
 *
 * Kullanım:
 *  - Kapı: sahibi `bp_kapı`; Degiskenler = bAcik, bKilidiAcildi, bMuhurlenmis.
 *    Aktör replike olmalı.
 *  - Oyuncu: sahibi `BP_FirstPersonCharacter`; Degiskenler = anahtarcantası,
 *    ToplananMuhurSayisi; `bSadeceSahibine` = true (envanter başkasına gitmez).
 *
 * Yazma yalnızca sunucuda olur; istemcide değişen değer bir sonraki sunucu
 * güncellemesiyle düzeltilir. Dizi ve sayı gibi her tür Blueprint değişkeni
 * `ExportText` ile taşınır; çok büyük diziler için uygun değildir.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "Degisken Yansitici"))
class MYPROJECT2_API UDegiskenYansitici : public UActorComponent
{
	GENERATED_BODY()

public:
	UDegiskenYansitici();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Yansıtılacak değişkenlerin adları (sahibin Blueprint sınıfında). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yansitici")
	TArray<FName> Degiskenler;

	/** True ise değerler yalnız sahibi olan istemciye gider (envanter gibi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yansitici")
	bool bSadeceSahibine = false;

	/** Sunucunun değişiklik kontrol aralığı (sn). */
	UPROPERTY(EditAnywhere, Category = "Yansitici", meta = (ClampMin = "0.02"))
	float GuncellemeAraligiSn = 0.1f;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Durum)
	FString Durum;

	UPROPERTY(ReplicatedUsing = OnRep_DurumSahip)
	FString DurumSahip;

	UFUNCTION()
	void OnRep_Durum();

	UFUNCTION()
	void OnRep_DurumSahip();

private:
	FString Topla() const;
	void Uygula(const FString& Metin);
};
