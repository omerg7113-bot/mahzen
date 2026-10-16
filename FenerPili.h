#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FenerPili.generated.h"

class USpotLightComponent;
class UFenerPiliWidget;

/**
 * El fenerinin pili. Telefon şarjı mantığı: dolu başlar, fener AÇIKKEN yavaş
 * yavaş azalır, KAPALIYKEN hiç azalmaz. Bitince fener zorla kapanır ve bir daha
 * açılmaz (pil dolana kadar).
 *
 * Fenerin açık/kapalı olduğunu `QuantumSpotLight`in görünürlüğünden okur —
 * Blueprint'te aç/kapa zinciri zaten sadece `Set Visibility` yapıyor
 * (`FenerAcik` bayrağı + Set Visibility). Pil bitince o bayrağı da reflection
 * ile false'a çeker, yoksa oyuncunun bir sonraki tuşu "kapat" sayılırdı.
 *
 * ÇOK OYUNCULU: ışığı aç/kapa tuşu yalnız sahibi olan istemcide çalışır. Bu
 * bileşen o istemcinin ışık durumunu sunucuya bildirir (`ServerFenerDurumu`),
 * sunucu pili TÜKETİR (tek gerçek burada) ve herkese yayar:
 *  - `bFenerAcikSunucu`: herkese; diğer oyuncuların ekranında bu pawn'ın feneri
 *    görünsün diye ışığı açıp kapatır.
 *  - `Yuzde`: yalnız sahibine; HUD ve "pil bitti, ışığı kapat" için.
 * Tek oyunculu oyunda hepsi yerelde çalışır, davranış eskisiyle aynıdır.
 * Kızın hedef seçicisi (`UKizHedefSecici`) sunucuda `FenerAcikMi()` ve
 * `KalanYuzde()` ile her oyuncunun fenerini okur.
 *
 * Ekrandaki gösterge `GostergeSinifi` doluysa YALNIZ yerel oyuncunun
 * pawn'ında oluşturulup viewport'a eklenir; Blueprint'te hiçbir bağlantı
 * gerekmez.
 *
 * Kurulum: bu bileşeni `BP_FirstPersonCharacter`a ekle, `GostergeSinifi`ni
 * `WBP_FenerPili` yap. Fener bileşeni ayrıca aranmaz, sahibin ilk
 * SpotLightComponent'i kullanılır.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "Fener Pili"))
class MYPROJECT2_API UFenerPili : public UActorComponent
{
	GENERATED_BODY()

public:
	UFenerPili();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Dolu pil kaç saniye ışık verir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pil", meta = (ClampMin = "1.0"))
	float ToplamSureSaniye = 120.f;

	/** Bu yüzdenin ALTINDA gösterge turuncuya döner. */
	UPROPERTY(EditAnywhere, Category = "Pil", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TuruncuEsik = 0.30f;

	/** Bu yüzdenin ALTINDA gösterge kırmızıya döner. */
	UPROPERTY(EditAnywhere, Category = "Pil", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float KirmiziEsik = 0.15f;

	/** Ekrana basılacak gösterge (WBP_FenerPili). Boşsa arayüz oluşturulmaz. */
	UPROPERTY(EditAnywhere, Category = "Pil")
	TSubclassOf<UFenerPiliWidget> GostergeSinifi;

	/** Kalan pil, 0..1 arası. Sunucu yazar, sahibine yayılır. */
	UPROPERTY(ReplicatedUsing = OnRep_Yuzde, VisibleAnywhere, BlueprintReadOnly, Category = "Pil")
	float Yuzde = 1.f;

	UFUNCTION(BlueprintPure, Category = "Pil", meta = (DisplayName = "Kalan Pil Yüzdesi"))
	float KalanYuzde() const { return Yuzde; }

	UFUNCTION(BlueprintPure, Category = "Pil", meta = (DisplayName = "Pil Bitti mi"))
	bool Bitti() const { return Yuzde <= 0.f; }

	/** Pili doldurur. 1 = tam dolu. Yalnız sunucuda etkilidir. (İleride pil toplama eşyası için.) */
	UFUNCTION(BlueprintCallable, Category = "Pil", meta = (DisplayName = "Pili Doldur"))
	void Doldur(float Miktar = 1.f);

	/** Eşiklere göre yeşil / turuncu / kırmızı. */
	UFUNCTION(BlueprintPure, Category = "Pil", meta = (DisplayName = "Pil Rengi"))
	FLinearColor Renk() const;

	/**
	 * Fener açık mı. Sunucuda sahibin bildirdiği gerçek durum, istemcide ışığın
	 * görünürlüğü. Kızın seçicisi bunu sunucuda okur.
	 */
	bool FenerAcikMi() const;

	/** Fenerin ışık bileşeni (BeginPlay'den önce null olabilir). */
	const USpotLightComponent* FenerBileseni() const { return Fener.Get(); }

protected:
	/** Sahibin ışığı açıp kapattığı yerden sunucuya bildirilir. */
	UFUNCTION(Server, Reliable)
	void ServerFenerDurumu(bool bAcik);

	UFUNCTION()
	void OnRep_Yuzde();

	UFUNCTION()
	void OnRep_FenerAcik();

	/** Sunucudaki gerçek fener durumu; herkese yayılır. */
	UPROPERTY(ReplicatedUsing = OnRep_FenerAcik)
	bool bFenerAcikSunucu = false;

private:
	/** Işık bileşeninin görünürlük bayrağı. */
	bool FenerIsikAcikMi() const;

	/** Yerel oyuncunun pawn'ı mı (tek oyunculuda hep true). */
	bool YerelMi() const;

	/** Işığı söndürür ve sahibin `FenerAcik` bayrağını da false yapar. */
	void FeneriKapat();

	/** Yerel oyuncuysa ve henüz kurulmadıysa göstergeyi oluşturur. */
	void GostergeKur();

	void GostergeGuncelle();

	TWeakObjectPtr<USpotLightComponent> Fener;
	TWeakObjectPtr<UFenerPiliWidget> Gosterge;

	/** Sunucuya en son bildirilen ışık durumu (yalnız yerel pawn'da anlamlı). */
	bool bBildirilen = false;
};
