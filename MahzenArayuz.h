#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Ticker.h"
#include "MahzenArayuz.generated.h"

class UButton;
class USoundBase;
class UMaterialInterface;

/**
 * Menü/ayar/duraklatma ekranlarının ortak taban sınıfı.
 *
 * Neden taban sınıf: Blueprint grafiğinde bir ÜYE fonksiyonu çağırdığında
 * hedef (self) pini kendiliğinden dolar. Python'dan sınıf/nesne pinine değer
 * yazılamadığı için statik fonksiyon + pin bağlama yolu tıkanıyordu; üye
 * fonksiyon bu sorunu tamamen ortadan kaldırıyor.
 *
 * Verdikleri:
 *  - her düğmeye otomatik: üzerine gelince yumuşak büyüme, kayan sis/pas
 *    dokusu, boğuk kalp atışı; tıklayınca ağır metal sesi
 *  - sekmeli ekran: panelleri yumuşak görünürlük geçişiyle değiştirir
 *  - menü eylemleri (devam / ayarlar / ana menü / çıkış)
 */
UCLASS()
class MYPROJECT2_API UMahzenArayuz : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- CİLA ---

	// Fare düğmenin üzerine geldiğinde çalar (derin kalp atışı).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Cila")
	TObjectPtr<USoundBase> UzerineGelmeSesi = nullptr;

	// Üzerine gelme sesinin şiddeti. Düşük tutulur, sık duyulan bir ses.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Cila")
	float UzerineSesSiddeti = 0.30f;

	// Perde. 1'in altı sesi kalınlaştırıp yavaşlatır — nabız ağırlaşır.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Cila")
	float UzerineSesPerdesi = 0.72f;

	// İki üzerine-gelme sesi arasındaki EN AZ süre (saniye). Fare düğmeler
	// arasında gezinirken sesin makineli gibi tekrarlamasını engeller.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Cila")
	float UzerineSesAraligi = 1.60f;

	// Tıklayınca çalar (ağır metal).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Cila")
	TObjectPtr<USoundBase> TiklamaSesi = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Cila")
	float TiklamaSesSiddeti = 0.55f;

	// Üzerine gelince düğmenin arkasında beliren kayan sis/pas materyali.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Cila")
	TObjectPtr<UMaterialInterface> UzerineGelmeDokusu = nullptr;

	// Üzerine gelince ulaşılan ölçek (1.0 = büyüme yok).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Cila")
	float UzerineBuyume = 1.10f;

	// Üzerine gelince düğme yazısının döneceği renk. Çerçevesiz tasarımda
	// geri bildirimi bu veriyor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Cila")
	FLinearColor UzerineMetinRengi = FLinearColor(0.86f, 0.11f, 0.07f, 1.0f);

	// Büyüme/küçülmenin yumuşaklığı. Büyük = hızlı.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Cila")
	float BuyumeHizi = 11.0f;

	// Basılıyken üzerine-gelme büyümesinin ne kadarı korunsun (0 = tam
	// orijinal boyut, 1 = hover boyutu). Hafif bir bastırma hissi için.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Cila")
	float BasmaOrani = 0.35f;

	// --- SEKMELER ---

	// Sekme içerik panellerinin widget adları (sırayla: Grafik, Ses, Kontroller).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Sekme")
	TArray<FName> SekmePaneller;

	// Sekme düğmelerinin widget adları; aktif olan vurgulanır.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Sekme")
	TArray<FName> SekmeDugmeleri;

	// Panel belirme süresi (saniye). Küt geçiş olmasın diye kısa bir fade.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Sekme")
	float SekmeGecisSuresi = 0.18f;

	// Açılışta gösterilecek sekme.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Sekme")
	int32 BaslangicSekmesi = 0;

	// Açıkken herhangi bir tuş/tıklama bir sonraki panele geçirir
	// (kalibrasyon: uyarı → kulaklık → gamma). Son panelde hiçbir şey yapmaz.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Sekme")
	bool bTusIleIlerle = false;

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Sekme")
	void SekmeSec(int32 Sekme);

	/** Bir sonraki panele geçer; son paneldeyse false döner. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Sekme")
	bool SonrakiSekme();

	// Sekme düğmelerine tek düğümle bağlanabilsin diye sarmalayıcılar.
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Sekme") void SekmeGrafik();
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Sekme") void SekmeSes();
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Sekme") void SekmeKontrol();

	// --- MENÜ EYLEMLERİ ---

	// Ayar ekranındaki denetimleri kayıttaki değerlerle doldurur.
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Menu")
	void EkraniDoldur();

	// Ana menüdeki oyna düğmesine BAŞLA / DEVAM ET yazar, sürümü basar.
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Menu")
	void MenuyuDoldur();

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Menu")
	void Kapat();

	// Duraklatma ekranından oyuna dön.
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Menu")
	void Devam();

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Menu")
	void AyarlariAc();

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Menu")
	void AnaMenuyeDon();

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Menu")
	void OyundanCik();

	/**
	 * P tuşu: duraklatma menüsünü açar, açıksa oyuna döndürür.
	 * Widget `/Game/Mahzen/UI/W_Duraklat`'tan yüklenir; `bDuraklatmaEkrani`
	 * açık geldiği için "DEVAM ET" duraklatmayı kaldırır, "ANA MENÜYE DÖN"
	 * önce oyunu kaydeder (bkz. UOyunDurumu).
	 */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Menu", meta = (WorldContext = "DunyaBaglami"))
	static void DuraklatmaAcKapa(const UObject* DunyaBaglami);

	/** Duraklatma menüsü şu an ekranda mı. */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Menu")
	static bool DuraklatmaAcikMi();

	/** Ana menü: kaydı silip oyunu baştan başlatır. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Menu")
	void YeniOyun();

	/** Ana menü: kayıtlı oyunu bıraktığı yerden açar. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Menu")
	void KayittanDevam();

	/** Oynanış haritası (yeni oyun bunu açar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Menu")
	FName OyunHaritasi = TEXT("FirstPersonMap");

	// AYARLAR düğmesinin açacağı ekran.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Menu")
	TSubclassOf<UUserWidget> AyarWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Menu")
	FName AnaMenuHaritasi = TEXT("AnaMenu");

	// Bu ekran kapanınca oyun duraklatması kalksın mı (duraklatma menüsü için).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Menu")
	bool bDuraklatmaEkrani = false;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometri, const FKeyEvent& Olay) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometri, const FPointerEvent& Olay) override;

private:
	struct FDugmeDurum
	{
		TWeakObjectPtr<UButton> Dugme;
		TWeakObjectPtr<class UTextBlock> Yazi;
		FLinearColor TabanRenk = FLinearColor::White;
		float Olcek = 1.0f;
		bool bUzerindeydi = false;
	};

	TArray<FDugmeDurum> Dugmeler;
	TArray<TWeakObjectPtr<UWidget>> Paneller;
	int32 AktifSekme = -1;
	float SekmeGecen = 1.0f;

	/**
	 * Kare başına iş çekirdek tiklayıcıya bağlı, widget tikine DEĞİL.
	 *
	 * Sebep: UUserWidget::TickFrequency'nin UE 5.6'daki tek makul değeri
	 * "Auto" ve bu modda motor, Blueprint'inde Event Tick bulunmayan
	 * widget'ların tikini kapatıyor. Saf C++ NativeTick geçersiz kılması
	 * yetmiyordu; panel geçişi RenderOpacity 0'da donuyor, düğme animasyonu
	 * hiç çalışmıyordu. Çekirdek tiklayıcı ayrıca oyun duraklatılmışken de
	 * çalışır — duraklatma menüsü için şart.
	 */
	FTSTicker::FDelegateHandle TikTutamaci;
	bool Tikla(float DeltaTime);
	bool bIlkTikYazildi = false;

	// Son üzerine-gelme sesinin zamanı (gerçek zaman, duraklatmadan etkilenmez).
	double SonUzerineSesi = -1000.0;

	void DugmeleriTopla();
	void SekmeVurgusu();

	/**
	 * Eylem düğmelerini ADINA göre bağlar: Btn_Devam, Btn_AnaMenu, Btn_Ayarlar,
	 * Btn_Cikis, Btn_YeniOyun, Btn_DevamEt. Böylece MCP ile kurulan widget'ların
	 * grafiğine hiç dokunmak gerekmiyor (Python'dan pin/temsilci bağlamak zor).
	 */
	void EylemDugmeleriniBagla();

	// --- TUŞ ATAMA SATIRLARI ---
	// Kontroller sekmesindeki `Lbl_Tus0..6` etiketleri ve `Cmb_Tus0..6`
	// açılır listelerini kurar: etiketi yazar, listeyi doldurur, kayıttaki
	// seçimi işaretler ve değişimi dinler. Widget yoksa sessizce atlar.
	void TuslariKur();

	UFUNCTION()
	void TusSecimiDegisti(FString Secilen, ESelectInfo::Type Tip);

	/** Oyun içindeyse oyuncunun tuş yönlendiricisini tazeler. */
	void TuslariYenidenBagla();

	// Kaydıraçların yanındaki yüzde yazılarını günceller. Ekran açılırken
	// değerler C++'tan programatik set edildiği için OnValueChanged
	// tetiklenmiyor; o yüzden tikten okunuyor. Değişmedikçe yazıya dokunulmaz.
	void KaydiracDegerleriniYaz();
	int32 SonYuzdeler[5] = { -1, -1, -1, -1, -1 };

	UFUNCTION()
	void DugmeTiklandi();
};
