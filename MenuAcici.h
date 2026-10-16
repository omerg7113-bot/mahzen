#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "MenuAcici.generated.h"

class UAudioComponent;
class USoundBase;

/**
 * Ana menü haritasının yöneticisi.
 *  - menü widget'ını ekrana basar, girdiyi arayüze alır, imleci açar
 *  - menü kamerasını yavaşça salındırır (sahne "canlı" dursun)
 *  - seçilen ışıkları kırpıştırır (koridordaki bozuk lamba hissi)
 *  - ambiyans sesini başlatır
 *  - "Oyna" basıldığında karartma + geçiş sesiyle oyun haritasını açar
 */
UCLASS()
class MYPROJECT2_API AMenuAcici : public AActor
{
	GENERATED_BODY()

public:
	AMenuAcici();

	virtual void Tick(float DeltaTime) override;

	// --- MENÜ ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	TSubclassOf<UUserWidget> MenuWidget;

	// İlk açılışta gösterilen katman (uyarı → kulaklık → gamma).
	// Boşsa doğrudan ana menü açılır.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	TSubclassOf<UUserWidget> KalibrasyonWidget;

	// Ayarlar ekranı. Menüdeki AYARLAR düğmesi bunu açar.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	TSubclassOf<UUserWidget> AyarWidget;

	/**
	 * Ana menü widget'ını gizler/gösterir. Ayarlar ekranı açılırken arkadaki
	 * DEVAM ET / AYARLAR / ÇIKIŞ düğmeleri görünmesin diye gizlenir, GERİ'ye
	 * basılınca geri gelir. Menü widget'ı yoksa (oyun içi duraklatma ekranı)
	 * hiçbir şey yapmaz.
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu", meta = (WorldContext = "DunyaBaglami"))
	static void MenuWidgetGoster(UObject* DunyaBaglami, bool bGoster);

	// Menü widget'ından tek düğümle çağrılır (sınıf pini gerektirmez).
	UFUNCTION(BlueprintCallable, Category = "Menu", meta = (WorldContext = "DunyaBaglami"))
	static void MenuAyarlariAc(UObject* DunyaBaglami);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	bool bOyunuDuraklat = false;

	/**
	 * Menü haritası oyun haritasının birebir kopyası olduğu için içinde
	 * anahtarlar, kapılar, kolyeler gibi oynanış aktörleri de var. Bunlar
	 * Event Tick'te Get Player Character çağırıyor; menüde GameModeBase
	 * kullanıldığından oyuncu karakteri yok ve her karede "Accessed None"
	 * hatası basılıyordu — dakikada on binlerce satır, bellek tükeniyor,
	 * editör çöküyordu. Bu bayrak açıkken menüdeki tüm Blueprint aktörlerinin
	 * tiki kapatılır: sahne saf dekor olur.
	 *
	 * Python'dan PrimaryActorTick.bStartWithTickEnabled yazılamadığı için
	 * (struct kopya dönüyor) bu iş çalışma zamanında burada yapılıyor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	bool bOynanisAktorleriniSustur = true;

	// Kalibrasyon ekranı "Onayla"ya basınca bunu çağırır: katmanı kapatır,
	// ana menüyü açar.
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void MenuyuGoster();

	UFUNCTION(BlueprintCallable, Category = "Menu", meta = (WorldContext = "DunyaBaglami"))
	static void MenuMenuyuGoster(UObject* DunyaBaglami);

	// --- KAMERA SALINIMI ---

	// Menü kamerası (CameraActor). Boşsa salınım yapılmaz.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Kamera")
	TObjectPtr<AActor> MenuKamera = nullptr;

	// Konum salınımının genliği (birim).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Kamera")
	float SalinimGenligi = 6.0f;

	// Açı salınımının genliği (derece).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Kamera")
	float SalinimAcisi = 0.8f;

	// Salınım hızı (tur/saniye). Küçük = ağır, huzursuz.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Kamera")
	float SalinimHizi = 0.08f;

	// --- IŞIK KIRPIŞMASI ---

	// Kırpışacak ışık aktörleri (PointLight/SpotLight).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Isik")
	TArray<TObjectPtr<AActor>> KirpisanIsiklar;

	// Kırpışmanın en düşük parlaklık oranı.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Isik")
	float KirpismaAlt = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Isik")
	float KirpismaHizi = 7.0f;

	// Ara ara tamamen sönüp yanma olasılığı (0 = hiç, 1 = sürekli).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Isik")
	float SonmeSikligi = 0.12f;

	// --- SES ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Ses")
	TObjectPtr<USoundBase> AmbiyansSesi = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Ses")
	float AmbiyansSiddeti = 0.6f;

	// Ambiyans ve bas, menü siyahtan açılırken bu sürede sıfırdan yükselir.
	// Böylece ilk saniyelerde yalnızca derinden gelen basınç duyulur.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Ses")
	float SesYukselmeSuresi = 4.0f;

	// Derinden vuran boğuk bas (döngülü).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Ses")
	TObjectPtr<USoundBase> BasSesi = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Ses")
	float BasSiddeti = 0.35f;

	// Ara ara çalan su damlası / ahşap gıcırtısı.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Ses")
	TObjectPtr<USoundBase> DamlaSesi = nullptr;

	// Doluysa her seferinde bu listeden rastgele biri çalar (DamlaSesi yerine).
	// Birkaç gıcırtı + bir damla koy, sahne "canlı dekor" gibi dursun.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Ses")
	TArray<TObjectPtr<USoundBase>> DamlaSesleri;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Ses")
	float DamlaSiddeti = 0.5f;

	// Damlalar arası en az / en çok bekleme (saniye).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Ses")
	float DamlaAraligiAz = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Ses")
	float DamlaAraligiCok = 11.0f;

	// --- OYUNA GEÇİŞ ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Gecis")
	FName OyunHaritasi = TEXT("FirstPersonMap");

	// Karartma süresi; bu süre dolunca harita açılır.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Gecis")
	float GecisSuresi = 1.8f;

	// Karartma başlarken çalan ses (nefes / ağır adımlar).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Gecis")
	TObjectPtr<USoundBase> GecisSesi = nullptr;

	// Menüdeki "Oyna" düğmesi bunu çağırır: karartır, ses çalar, haritayı açar.
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OyunaBasla();

	// Widget'tan tek düğümle çağrılabilsin diye statik sarmalayıcı: sahnedeki
	// MenuAcici'yi kendisi bulur. (Blueprint'te sınıf pinine değer yazmak
	// Python API'siyle mümkün değil, bu yüzden GetActorOfClass kullanılmıyor.)
	UFUNCTION(BlueprintCallable, Category = "Menu", meta = (WorldContext = "DunyaBaglami"))
	static void MenuOyunaBasla(UObject* DunyaBaglami);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> AcikMenu = nullptr;

	UPROPERTY()
	TObjectPtr<UAudioComponent> AmbiyansBileseni = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> AcikKalibrasyon = nullptr;

	UPROPERTY()
	TObjectPtr<UAudioComponent> BasBileseni = nullptr;

	FVector KameraTabanKonum = FVector::ZeroVector;
	FRotator KameraTabanRot = FRotator::ZeroRotator;
	TArray<float> IsikTabanSiddet;
	float Gecen = 0.0f;
	bool bGecisBasladi = false;
	FTimerHandle GecisTimer;
	FTimerHandle DamlaTimer;
	FTimerHandle GirdiTimer;

	void HaritayiAc();
	void WidgetiAc(TSubclassOf<UUserWidget> Sinif, TObjectPtr<UUserWidget>& Hedef);
	void DamlaCal();
	void GirdiyiArayuzeAl();
	void OynanisAktorleriniSustur();
};
