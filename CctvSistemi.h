#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameFramework/Actor.h"
#include "CctvSistemi.generated.h"

class UBoxComponent;
class UCameraComponent;
class UFont;
class USpotLightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneCaptureComponent2D;
class USoundBase;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

/**
 * Güvenlik kamerası / kontrol odası sistemi.
 *
 * Mimari kararlar (Faz 0'da onaylandı):
 *  - 5 kamera arasında DÖNGÜSEL geçiş, tek monitör, TEK Render Target.
 *    Bu yüzden sahnede 5 yakalama bileşeni yok; TEK bir SceneCapture2D var ve
 *    kanal değişince aktif kameranın dünya dönüşümüne ışınlanıyor. 16 GB RAM
 *    sınırında en pahalı kalem yakalama olurdu, beşe bölmek anlamsız.
 *  - Yakalama MANUEL: bCaptureEveryFrame kapalı, saniyede YakalamaFps kez
 *    CaptureScene() çağrılıyor. Düşük kare hızı hem bedava hem de CRT hissi.
 *  - Lumen/GI, AA, bloom, motion blur, SSR, volumetrik sis yakalamada KAPALI.
 *  - Yeşil tint, parazit, tarama çizgisi, vinyet BURADA DEĞİL monitörün
 *    materyalinde hesaplanıyor; buradan yalnızca skaler parametre geçiyor.
 *
 * Kameralar etikete (Tags) göre bulunuyor, label'a göre DEĞİL: label yalnızca
 * editörde var, paketlenmiş oyunda yok.
 */
UCLASS()
class MYPROJECT2_API ACctvSistemi : public AActor
{
	GENERATED_BODY()

public:
	ACctvSistemi();

	// --- KAMERALAR ---

	/** Bu önekle başlayan Tag'i olan CameraActor'lar kanal olarak kullanılır. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Kameralar")
	FName KameraEtiketOneki = TEXT("CCTV_CAM_");

	/** Elle sıralamak istersen doldur; BOŞ bırakılırsa etikete göre otomatik bulunur. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Cctv|Kameralar")
	TArray<TObjectPtr<AActor>> Kameralar;

	// --- YAKALAMA ---

	/** Saniyedeki yakalama sayısı. 8-12 arası CCTV hissi verir, GPU'yu de kurtarır. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yakalama", meta = (ClampMin = "1.0", ClampMax = "60.0"))
	float YakalamaFps = 10.0f;

	// 4:3 kalmalı: monitörün ekran yüzeyi 198x143 (en/boy 1.385).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Yakalama", meta = (ClampMin = "64", ClampMax = "2048"))
	int32 CozunurlukX = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Yakalama", meta = (ClampMin = "64", ClampMax = "2048"))
	int32 CozunurlukY = 768;

	/**
	 * Kameranın kendi kızılötesi aydınlatması.
	 *
	 * Gerçek gece görüş kameralarının net görmesinin sebebi budur: sahneyi
	 * kendileri aydınlatır. Kazancı yükselterek aydınlatmanın tavanı var,
	 * çünkü kazanç yakalamanın gürültüsünü de büyütüyor.
	 *
	 * DİKKAT: Bu ışık oyuncuya da görünür, koridoru gerçekten aydınlatır.
	 * O yüzden varsayılan KAPALI — açmak bir tasarım kararı.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|IR")
	bool bIRIsigiAcik = true;

	/**
	 * Işık yalnızca yakalama anında yanar: kamera aydınlık görür, oyuncu
	 * karanlık. Gerçek kızılötesinin yaptığı budur.
	 *
	 * Render komutları sıraya girdiği için "yak → CaptureScene → söndür"
	 * dizisinde yakalama ışığı görür, kare sonunda çizilen ana görüntü görmez.
	 * Eğer oyunda koridor saniyede 8 kez yanıp sönüyorsa bu numara o donanımda
	 * tutmamış demektir; bunu false yap, ışık sürekli yanar (oyuncu da görür).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|IR")
	bool bIRYalnizcaKamerada = true;

	/**
	 * Ters-kare düşüm KAPALI olduğu için bu değer birimsiz bir parlaklık.
	 * Ölçüldü (ham yakalamada kapkara piksel oranı): 25 → %32, 60 → %21,
	 * 140 → %6. Ters-kare açık olsaydı yakın duvar patlar, derinlik kararırdı.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|IR")
	float IRSiddeti = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|IR")
	float IRMenzili = 2600.0f;

	/** Koninin dış açısı (derece). Kameranın görüş açısından biraz geniş tut. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|IR")
	float IRAcisi = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|IR")
	FLinearColor IRRengi = FLinearColor(0.75f, 0.85f, 1.0f, 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Cctv|IR")
	void IRIsiginiAyarla(bool bAcik);

	// NOT: Burada bir "PozlamaArtisi" özelliği vardı, KALDIRILDI. Ölçüldü:
	// AutoExposureBias +3 ile +10 arasında ortalama luminans 0.00408 -> 0.00376,
	// yani hiçbir etkisi yoktu (EyeAdaptation açıldıktan sonra bile). Parlaklık
	// monitör materyalindeki Kazanc + Reinhard eğrisinden geliyor.

	/** Yakalamada uzak kırpma. Kısma sırasında ikinci en etkili kalem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yakalama")
	float UzakKirpma = 4000.0f;

	/**
	 * Oyuncu monitörden bu kadar uzaksa yakalama tamamen durur.
	 * Oturmuşken mesafe kontrolü atlanır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yakalama")
	float YakalamaMesafesi = 700.0f;

	// --- MONITOR ---

	/** Ekranı taşıyan aktör (CCTV_Monitor). Boşsa Tag'den bulunur. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Cctv|Monitor")
	TObjectPtr<AActor> MonitorAktoru;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Monitor")
	FName MonitorEtiketi = TEXT("CCTV_Monitor");

	/** old_tv mesh'inde ekran yüzeyi 2. yuva ('tv'): 198x143, en/boy 4:3. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Monitor")
	int32 EkranMateryalYuvasi = 2;

	/** Faz 3'te yazılacak CRT materyali. Boşsa görüntü ham Render Target olarak basılır. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Monitor")
	TObjectPtr<UMaterialInterface> EkranMateryali;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Monitor")
	bool bMonitorAcik = true;

	// --- EKRANDAKİ KANAL YAZISI ---
	//
	// Yazı materyale değil, yakalamadan SONRA render target'ın üstüne canvas
	// ile basılıyor. Böylece tarama çizgisi/parazit/vinyet yazının da üstünden
	// geçiyor: görüntüye gömülmüş duruyor, ekrana yapıştırılmış gibi değil.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yazi")
	bool bKanalYazisi = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yazi")
	FString KanalYazisiOneki = TEXT("CAM ");

	/** Render target piksel cinsinden sol üst köşeden uzaklık. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yazi")
	FVector2D KanalYazisiKonumu = FVector2D(40.0f, 28.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yazi")
	float KanalYazisiOlcegi = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yazi")
	FLinearColor KanalYazisiRengi = FLinearColor(0.85f, 0.95f, 0.85f, 1.0f);

	/**
	 * OSD fontu. Boşsa DroidSansMono yüklenir — CCTV yazısının karakteri
	 * monospace olmasından gelir. Gerçek bir VCR fontun varsa (TTF) içe alıp
	 * buraya ata, başka hiçbir şey değişmesin.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Yazi")
	TObjectPtr<UFont> OsdFontu;

	// --- REC göstergesi ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yazi")
	bool bRecGoster = true;

	/** Saniye. Yanıp sönme döngüsünün tamamı; yarısında yanar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yazi")
	float RecYanipSonmePeriyodu = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yazi")
	FLinearColor RecRengi = FLinearColor(0.95f, 0.25f, 0.2f, 1.0f);

	// --- Zaman damgası ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yazi")
	bool bZamanDamgasi = true;

	/**
	 * Gerçek sistem saati yerine sabit bir tarih kullan (korku için daha iyi:
	 * oyuncunun kendi saatini görmesi büyüyü bozuyor). Boş bırakılırsa
	 * sistem saati basılır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yazi")
	FString SabitTarih = TEXT("1998-11-03");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Yazi")
	float ZamanDamgasiOlcegi = 2.2f;

	// Materyal parametre adları (Faz 3'teki materyalle aynı olmalı).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Monitor|Parametre") FName ParamGoruntu = TEXT("Goruntu");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Monitor|Parametre") FName ParamParazit = TEXT("Parazit");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Monitor|Parametre") FName ParamSenkron = TEXT("Senkron");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Monitor|Parametre") FName ParamKanal   = TEXT("Kanal");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Monitor|Parametre") FName ParamAcik    = TEXT("Acik");

	// --- PARAZIT / SENKRON ---

	/** Her zaman duran taban parazit. "Parazit kuralı" bununla öğretiliyor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Parazit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TabanParazit = 0.12f;

	/** Kanal değişiminde senkron kaybı süresi; bu sürede yeni kare YAKALANMAZ. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Parazit")
	float SenkronKaybiSuresi = 0.35f;

	/** Senkron kaybı anındaki parazit şiddeti. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Parazit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SenkronParaziti = 1.0f;

	// --- SENARYOLU NUMARALAR ---

	/** "Kayıt tekrarı" ve "kesinti" en fazla bu kadar kez kullanılabilir (tasarım: 2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Senaryo")
	int32 MaksTekrar = 2;

	// --- SES ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Ses")
	TObjectPtr<USoundBase> KanalSesi;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Ses")
	float KanalSesSiddeti = 0.45f;

	// --- OTURMA ---

	/** Oturunca kameranın monitöre geçiş süresi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Oturma")
	float GecisSuresi = 0.45f;

	/** C++ tarafında tuş bağlanması. Kapatıp Blueprint'ten çağırmak da mümkün. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Oturma")
	bool bTuslariBagla = true;

	// Varsayılanlar bilinçli olarak çakışmayan tuşlar: E oyunun etkileşim
	// tuşu, ESC duraklatma menüsü. İkisine de girmiyoruz; editörden
	// değiştirmek derleme gerektirmez.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Oturma") FKey OturmaTusu       = EKeys::F;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Oturma") FKey SonrakiKanalTusu = EKeys::Right;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Oturma") FKey OncekiKanalTusu  = EKeys::Left;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cctv|Oturma") FKey KalkmaTusu       = EKeys::X;

	/** Oyuncu masanın başında mı (etkileşim ipucu göstermek için). */
	UFUNCTION(BlueprintPure, Category = "Cctv|Oturma") bool OyuncuYakindaMi() const { return bOyuncuYakinda; }

	/** Oturuyorsa kaldırır, ayaktaysa oturtur. Oturma tuşu buna bağlı. */
	UFUNCTION(BlueprintCallable, Category = "Cctv|Oturma") void OturmayiDegistir();

	/** Oturma kamerası ekrandan bu kadar önde durur (bileşen elle taşınmadıysa). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Oturma")
	float OturmaMesafesi = 70.0f;

	/**
	 * Monitöre oturunca oyuncunun el feneri sönsün.
	 *
	 * Amaç: ekranda gördüğün görüntünün oyun dünyasından bağımsız durması.
	 * Fener açıkken masayı ve monitörün kasasını aydınlatıyor, ekran da oyunun
	 * içindeki bir nesne gibi görünüyor; sönünce yalnızca ekranın kendi ışığı
	 * kalıyor. Kalkınca YALNIZCA bizim söndürdüklerimiz geri yakılır — oyuncu
	 * zaten kapalıyken oturduysa kapalı kalır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cctv|Oturma")
	bool bFeneriKapat = true;

	// --- EYLEMLER ---

	UFUNCTION(BlueprintCallable, Category = "Cctv") void SonrakiKanal();
	UFUNCTION(BlueprintCallable, Category = "Cctv") void OncekiKanal();
	UFUNCTION(BlueprintCallable, Category = "Cctv") void KanalSec(int32 Kanal);

	UFUNCTION(BlueprintCallable, Category = "Cctv") void MonitoruAcKapa(bool bAcik);

	/** Görüntüyü Sure saniye boyunca DONDURUR: "kayıt tekrarı" hissi. */
	UFUNCTION(BlueprintCallable, Category = "Cctv|Senaryo") bool KayitTekrariniOynat(float Sure);

	/** Sinyal kesintisi: yakalama durur, parazit tam açılır. */
	UFUNCTION(BlueprintCallable, Category = "Cctv|Senaryo") bool KesintiBaslat(float Sure);

	UFUNCTION(BlueprintCallable, Category = "Cctv|Oturma") void MonitoreOtur();
	UFUNCTION(BlueprintCallable, Category = "Cctv|Oturma") void MonitordenKalk();

	UFUNCTION(BlueprintPure, Category = "Cctv") int32 AktifKanal() const { return AktifIndeks; }
	UFUNCTION(BlueprintPure, Category = "Cctv") int32 KanalSayisi() const { return Kameralar.Num(); }
	UFUNCTION(BlueprintPure, Category = "Cctv") float ParazitSiddeti() const;
	UFUNCTION(BlueprintPure, Category = "Cctv") bool OturuyorMu() const { return bOturuyor; }
	UFUNCTION(BlueprintPure, Category = "Cctv") bool YakalamaAcikMi() const { return bYakalamaAcik; }

	/** Aktif kanalda hangi kamera var (tetikleyici/HUD için). */
	UFUNCTION(BlueprintPure, Category = "Cctv") AActor* AktifKameraAktoru() const;

	// Blueprint'ten tek düğümle çağrılabilsin diye dünya bağlamı alan sarmalayıcılar
	// (menüde işleyen yöntem: MenuAcici::MenuOyunaBasla ile aynı kalıp).
	UFUNCTION(BlueprintCallable, Category = "Cctv", meta = (WorldContext = "DunyaBaglami"))
	static void CctvOtur(UObject* DunyaBaglami);

	UFUNCTION(BlueprintCallable, Category = "Cctv", meta = (WorldContext = "DunyaBaglami"))
	static void CctvKalk(UObject* DunyaBaglami);

	UFUNCTION(BlueprintCallable, Category = "Cctv", meta = (WorldContext = "DunyaBaglami"))
	static void CctvSonrakiKanal(UObject* DunyaBaglami);

	UFUNCTION(BlueprintCallable, Category = "Cctv|Senaryo", meta = (WorldContext = "DunyaBaglami"))
	static void CctvKayitTekrari(UObject* DunyaBaglami, float Sure);

	UFUNCTION(BlueprintCallable, Category = "Cctv|Senaryo", meta = (WorldContext = "DunyaBaglami"))
	static void CctvKesinti(UObject* DunyaBaglami, float Sure);

	static ACctvSistemi* Bul(UObject* DunyaBaglami);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Sebep) override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, Category = "Cctv")
	TObjectPtr<USceneCaptureComponent2D> Yakalayici;

	/** Oturunca oyuncunun görüş noktası. Editörden monitörün önüne ayarla. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cctv")
	TObjectPtr<UCameraComponent> OturmaKamerasi;

	/** Oyuncu yakınlığı (etkileşim ipucu için; oturmayı zorunlu kılmaz). */
	UPROPERTY(VisibleAnywhere, Category = "Cctv")
	TObjectPtr<UBoxComponent> Tetik;

	/** Yakalayıcıya bağlı, onunla birlikte aktif kameraya ışınlanır. */
	UPROPERTY(VisibleAnywhere, Category = "Cctv")
	TObjectPtr<USpotLightComponent> IRIsik;

private:
	UFUNCTION()
	void TetikGirdi(UPrimitiveComponent* Bilesen, AActor* Diger, UPrimitiveComponent* DigerBilesen,
		int32 Indeks, bool bSupurmeden, const FHitResult& Vurus);

	UFUNCTION()
	void TetikCikti(UPrimitiveComponent* Bilesen, AActor* Diger, UPrimitiveComponent* DigerBilesen, int32 Indeks);

	/** Tuşa basıldığında yalnızca oturuyorken kanal değiştirsin. */
	void TusSonraki();
	void TusOnceki();

	void GirdiAkisi(bool bAc);

	void KanalYazisiniCiz();
	void FeneriAyarla(bool bKapat);

	void KameralariTopla();
	void OturmaKamerasiniYerlestir();
	void YakalayiciyiKur();
	void YakalayiciyiKameraya(int32 Indeks);
	void YakalamaZamanlayicisi(bool bAc);
	void Yakala();
	void MateryaliKur();
	void ParametreleriYaz();
	bool YakalamaGerekliMi() const;
	void TuslariBagla();

	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> Hedef;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> EkranMid;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> MonitorMesh;

	FTimerHandle YakalamaTimer;

	/** Oturunca söndürdüğümüz fenerler; kalkınca sadece bunlar geri yakılır. */
	TArray<TWeakObjectPtr<USpotLightComponent>> SondurulenFenerler;

	int32 AktifIndeks = 0;
	bool bOturuyor = false;
	bool bYakalamaAcik = false;
	bool bOyuncuYakinda = false;
	bool bGirdiAcik = false;

	/** > 0 iken yeni kare yakalanmaz (senkron kaybı / tekrar / kesinti). */
	float SenkronKalan = 0.0f;
	float DondurmaKalan = 0.0f;
	float KesintiKalan = 0.0f;
	int32 TekrarKullanildi = 0;
};
