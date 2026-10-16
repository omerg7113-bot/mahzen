#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KizHedefSecici.h"
#include "EtkilesimSunucu.generated.h"

/**
 * Oyuncunun (pawn) dünyadaki nesnelerle etkileşimini sunucuya taşır.
 *
 * Çok oyunculuda kapıyı açmak, mührü kapatmak gibi işler SUNUCUDA çalışmalı:
 * anahtar kontrolü, kapı durumu, kızın duyması hep sunucudadır. Oyuncunun tuşu
 * ise kendi bilgisayarında çalışır. Bu bileşen o köprüdür:
 *
 *  1) İstemci `KapiyiKullan(Kapi)` çağırır → `ServerOlayCagir` RPC'si sunucuya gider.
 *  2) Sunucu kapının aynı adlı Blueprint olayını (custom event) çalıştırır
 *     (Blueprint mantığı değişmez; sunucuda, etkileşen pawn'ı bilerek çalışır).
 *  3) Sunucu aynı olayı istemcilere de yayar (ses/animasyon her ekranda olsun).
 *  4) Sunucu olayı kızın hedef seçicisine bildirir (`UKizHedefSecici::OlayBildir`).
 *
 * Tek oyunculuda (yetki zaten yerelde) RPC atlanır, olay doğrudan çalışır.
 *
 * Güvenlik: yalnız `IzinliOlaylar` listesindeki olay adları çağrılır ve hedef,
 * pawn'a `MaksMenzilCm` içinde olmalıdır; istemci keyfi fonksiyon çağıramaz.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "Etkilesim Sunucu"))
class MYPROJECT2_API UEtkilesimSunucu : public UActorComponent
{
	GENERATED_BODY()

public:
	UEtkilesimSunucu();

	/** Hedef pawn'a bu mesafeden (cm) uzaksa sunucu isteği reddeder. 0 = sınırsız. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etkilesim", meta = (ClampMin = "0.0"))
	float MaksMenzilCm = 800.f;

	/** Sunucuda çalıştırılmasına izin verilen olay (custom event) adları. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etkilesim")
	TArray<FName> IzinliOlaylar;

	/** Kapının `KapiyiKullan` olayını sunucuda çalıştırır; kıza "kapı" olayı bildirir. */
	UFUNCTION(BlueprintCallable, Category = "Etkilesim", meta = (DisplayName = "Kapiyi Kullan"))
	void KapiyiKullan(AActor* Kapi);

	/** Kapının `OdayiMuhurle` olayını sunucuda çalıştırır; kıza "mühür" olayı bildirir. */
	UFUNCTION(BlueprintCallable, Category = "Etkilesim", meta = (DisplayName = "Odayi Muhurle"))
	void OdayiMuhurle(AActor* Kapi);

	/** Herhangi izinli olayı sunucuda çalıştırır. KizOlayi Yok ise kıza haber verilmez. */
	UFUNCTION(BlueprintCallable, Category = "Etkilesim", meta = (DisplayName = "Olay Cagir"))
	void OlayCagir(AActor* Hedef, FName OlayAdi, EKizOlay KizOlayi);

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerOlayCagir(AActor* Hedef, FName OlayAdi, EKizOlay KizOlayi);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOlayCagir(AActor* Hedef, FName OlayAdi);

private:
	void SunucudaYurut(AActor* Hedef, FName OlayAdi, EKizOlay KizOlayi);
	bool YerelCagir(AActor* Hedef, FName OlayAdi) const;
};
