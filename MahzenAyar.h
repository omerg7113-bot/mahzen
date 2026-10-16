#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameFramework/SaveGame.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MahzenAyar.generated.h"

/** Diske yazılan oyuncu ayarları ve ilerleme bayrakları. */
UCLASS()
class MYPROJECT2_API UMahzenKayit : public USaveGame
{
	GENERATED_BODY()

public:
	// İlk açılıştaki uyarı/kulaklık/gamma katmanı tamamlandı mı.
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") bool bIlkAcilisTamam = false;

	// Görüntü
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") float Gamma = 2.2f;
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") int32 CozunurlukX = 1920;
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") int32 CozunurlukY = 1080;
	// 0 = tam ekran, 1 = pencereli tam ekran, 2 = pencereli
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") int32 EkranModu = 1;

	// 0 = düşük, 1 = orta, 2 = yüksek, 3 = epik
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") int32 Kalite = 2;
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") bool bVSync = true;
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") bool bMotionBlur = true;

	// Ses
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") float AnaSes = 1.0f;
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") float MuzikSes = 1.0f;
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") float EfektSes = 1.0f;

	// Kontroller
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") float FareHassasiyeti = 1.0f;
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") bool bTersY = false;

	// Oyuncunun seçtiği tuşlar: eylem adı -> tuş. Bir eylem burada yoksa
	// UMahzenAyar::TusVarsayilan() döner. WASD ve fare değiştirilemez.
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") TMap<FName, FKey> Tuslar;

	// İlerleme: oyun en az bir kez başlatıldıysa menüde "Devam Et" yazar.
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") bool bOyunBaslatildi = false;
	UPROPERTY(BlueprintReadWrite, Category = "Mahzen") int32 ZorlukSeviyesi = 1;
};

/** Ayarları okuyan/yazan ve oyuna uygulayan yardımcılar. Widget'lardan çağrılır. */
UCLASS()
class MYPROJECT2_API UMahzenAyar : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Kayıt dosyasını getirir; yoksa varsayılanlarla oluşturur. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static UMahzenKayit* Kayit();

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void Kaydet();

	/** İlk açılış katmanı (uyarı + kulaklık + gamma) gösterilmeli mi. */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Ayar")
	static bool IlkAcilisMi();

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void IlkAcilisiTamamla();

	/** Menüde "Devam Et" mi yazsın (oyun daha önce başlatıldı mı). */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Ayar")
	static bool OyunBaslatildiMi();

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void OyunBaslatildiIsaretle();

	// --- görüntü ---
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void GammaAyarla(float Deger);

	UFUNCTION(BlueprintPure, Category = "Mahzen|Ayar")
	static float GammaOku();

	/** Ekranda listelenecek çözünürlükler ("1920 x 1080" biçiminde). */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static TArray<FString> CozunurlukListesi();

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void CozunurlukAyarla(int32 Genislik, int32 Yukseklik);

	/** "1920 x 1080" biçimindeki metni ayrıştırıp uygular (ComboBox için). */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void CozunurlukSecMetin(const FString& Metin);

	UFUNCTION(BlueprintPure, Category = "Mahzen|Ayar")
	static FString AktifCozunurlukMetni();

	/** Ekran modu seçenekleri (ComboBox doldurmak için). */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static TArray<FString> EkranModuListesi();

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void EkranModuSecMetin(const FString& Metin);

	UFUNCTION(BlueprintPure, Category = "Mahzen|Ayar")
	static FString AktifEkranModuMetni();

	// --- grafik kalitesi / v-sync / motion blur ---

	/** 0 = Düşük, 1 = Orta, 2 = Yüksek, 3 = Epik */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void KaliteAyarla(int32 Seviye);

	UFUNCTION(BlueprintPure, Category = "Mahzen|Ayar")
	static FString AktifKaliteMetni();

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void KaliteDegistir(int32 Yon, class UTextBlock* Hedef);

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void VSyncAyarla(bool bAcik);

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void MotionBlurAyarla(bool bAcik);

	// --- arayüzü mevcut değerlerle doldurmak için okuyucular ---
	UFUNCTION(BlueprintPure, Category = "Mahzen|Ayar")
	static float AnaSesOku();

	UFUNCTION(BlueprintPure, Category = "Mahzen|Ayar")
	static float EfektSesOku();

	/** 0 = tam ekran, 1 = pencereli tam ekran, 2 = pencereli */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void EkranModuAyarla(int32 Mod);

	/** Listede bir önceki/sonraki çözünürlüğe geçer, uygular ve metni hedefe yazar. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void CozunurlukDegistir(int32 Yon, class UTextBlock* Hedef);

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void EkranModuDegistir(int32 Yon, class UTextBlock* Hedef);

	/** Ayar ekranındaki kaydırıcı/metinleri kayıttaki değerlerle doldurur. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void AyarEkraniniDoldur(class UUserWidget* Ekran);

	/** Ana menüdeki oyna düğmesine "BASLA" ya da "DEVAM ET" yazar. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void AnaMenuyuDoldur(class UUserWidget* Menu);

	// --- ses ---
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void SesAyarla(float Ana, float Muzik, float Efekt);

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void AnaSesAyarla(float Deger);

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void MuzikSesAyarla(float Deger);

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void EfektSesAyarla(float Deger);

	UFUNCTION(BlueprintPure, Category = "Mahzen|Ayar")
	static float MuzikSesOku();

	/** Müzik/efekt ses sınıflarını miksere uygular. Dünya bağlamı gerekir. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void SesSiniflariniUygula(UObject* DunyaBaglami);

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void HassasiyetAyarla(float Deger);

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void TersYAyarla(bool bAcik);

	// --- kontroller ---
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void FareAyarla(float Hassasiyet, bool bTersY);

	/** Oyuncu bakış kodunun okuyacağı çarpanlar. */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Ayar")
	static float FareHassasiyeti();

	UFUNCTION(BlueprintPure, Category = "Mahzen|Ayar")
	static bool TersYMi();

	/** Kayıttaki her şeyi motora uygular (açılışta bir kez çağrılır). */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Ayar")
	static void TumAyarlariUygula(UObject* DunyaBaglami);

	// --- TUŞ ATAMALARI ---------------------------------------------------
	// Eylem adları sabit: Etkilesim, NotKapi, KasaAc, Muhurle, Envanter,
	// Fener, AnahtarBirak. Oyuncu her birine istediği harfi atayabilir;
	// aynı harf iki eyleme verilebilir (oyuncu hepsini E yapmak isterse diye).

	/** Ayarlar ekranının satırlarını kurarken kullandığı eylem listesi. */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Tus")
	static TArray<FName> TusEylemleri();

	/** Eylemin ekranda görünecek Türkçe adı. */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Tus")
	static FText TusEylemAdi(FName Eylem);

	/** Eylemin fabrika ayarı tuşu. */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Tus")
	static FKey TusVarsayilan(FName Eylem);

	/** Kayıtta seçim varsa onu, yoksa varsayılanı döner. */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Tus")
	static FKey TusOku(FName Eylem);

	/** Seçimi kaydeder. Oyun içindeki bağlamayı tazelemek çağıranın işi. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Tus")
	static void TusYaz(FName Eylem, FKey Tus);

	/**
	 * Eylemin o anki tuşunun ekranda gösterilecek harfi ("F", "Q"...).
	 * Oyun içi etkileşim ipuçları ("[F] Aç") bunu kullanır; oyuncu tuşu
	 * değiştirince ipucu da kendiliğinden değişir.
	 */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Tus")
	static FString TusHarfi(FName Eylem);

	/**
	 * Hazır ipucu metni: TusIpucuMetni("NotKapi", "Kapat") -> "[F] Kapat".
	 * Widget'ta tek düğümle SetText'e bağlanabilsin diye FText döner.
	 */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Tus")
	static FText TusIpucuMetni(FName Eylem, const FString& Metin);

	/** Açılır listede gösterilecek tuşlar (WASD hariç harfler + Tab/Boşluk). */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Tus")
	static TArray<FKey> TusSecenekleri();

	/** Tüm atamaları fabrika ayarına döndürür. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Tus")
	static void TuslariSifirla();

private:
	static UMahzenKayit* Onbellek;
	static const TCHAR* YuvaAdi;
};
