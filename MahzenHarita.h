#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OyunSonu.h"
#include "Subsystems/WorldSubsystem.h"
#include "MahzenHarita.generated.h"

/** Izgara taramasında bulunan bir kapı ve birleştirdiği iki bölge. */
struct FMahzenKapiKaydi
{
	TWeakObjectPtr<AActor> Aktor;
	FString Anahtar;            // gerekenanahtar
	FString Muhur;              // GerekliMuhurAdi (boşsa giriş/koridor kapısı)
	int32 BolgeA = INDEX_NONE;
	int32 BolgeB = INDEX_NONE;
};

/** Yerde duran (henüz alınmamış) bir anahtar ya da mühür taşı. */
struct FMahzenEsya
{
	FString Ad;
	int32 Bolge = INDEX_NONE;
};

/**
 * Haritayı oyunun başında bir kez tarar ve odaları (bölgeleri) çıkarır.
 *
 * Yöntem: her katta 50 cm'lik bir ızgara kurulur, her hücrede oyuncu
 * kapsülü kadar bir küreyle çarpışma testi yapılır, komşu iki boş hücre
 * arasında ayrıca bir küre süpürmesi atılır (ince duvarlardan sızmayı
 * engeller). Kalan bağlı bileşenlerin her biri bir "bölge"dir. Tarama
 * seviye başında, bütün kapılar KAPALIYKEN yapıldığı için her oda kendi
 * bölgesi olarak çıkar; kapılar da bölgeler arası kenarlara dönüşür.
 *
 * Bölge adları hedef noktalarının `AnahtarKatman|101..120` etiketlerinden
 * gelir (AnahtarDagitici ile aynı sıra). Etiketsiz bölgeler koridordur.
 *
 * Buradan "oyun hâlâ bitirilebilir mi" sorusu klasik anahtar-kapı
 * erişilebilirlik problemine iner: `KilitlendiMi`.
 */
UCLASS()
class MYPROJECT2_API UMahzenHaritaAltSistemi : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Izgarayı ve bölgeleri kurar (bir kez; sonraki çağrılar hemen döner). */
	void Kur();

	bool Hazir() const { return bHazir; }

	/** Konumun düştüğü bölge; ızgara dışıysa INDEX_NONE. */
	int32 BolgeNo(const FVector& Konum) const;

	/** "1. Kat Ofis" / "2. kat koridoru" gibi okunur ad. */
	FString BolgeAdi(int32 Bolge) const;

	/**
	 * Oyun artık bitirilemez mi? Bitirilemiyorsa true döner, türü ve
	 * oyuncuya gösterilecek açıklamayı doldurur.
	 */
	bool KilitlendiMi(EOyunSonuTuru& OutTur, FString& OutSebep);

	/** Hata ayıklama: bölgeleri, kapıları ve eşyaları loga döker. */
	void Dok();

	/**
	 * Oyun HENÜZ BAŞLAMADAN bitirilemez durumdaysa true. Bu, oyuncunun hatası
	 * değil (anahtar dağıtımı o tohumda çözülemez çıkmış) — böyle bir oyunda
	 * ilk mühürde "çıkmaza girdin" ekranı basmak yanlış olur, bastırıyoruz.
	 */
	bool BaslangictaKilitliMi() const { return bBaslangictaKilitli; }

private:
	/** BeginPlay + 1 sn: taramayı yapar, kıza KapiAcici takar, başlangıç kontrolü. */
	void BaslangicKurulumu();

	void EsyalariTopla(TArray<FMahzenEsya>& Out) const;
	int32 HucreBolgesi(int32 Kat, int32 I, int32 J) const;
	int32 EnYakinBolge(const FVector& Konum, int32 AramaYaricapi) const;

	bool bHazir = false;
	bool bBaslangictaKilitli = false;

	float Adim = 50.0f;
	float ProbeYukseklik = 60.0f;
	FVector2D Kose = FVector2D::ZeroVector;   // ızgaranın (0,0) hücresinin dünya XY'si
	int32 NX = 0;
	int32 NY = 0;
	TArray<float> KatZ;                        // her katın zemin yüksekliği
	TArray<int32> Hucreler;                    // NK*NX*NY, INDEX_NONE = dolu

	TArray<FString> BolgeAdlari;               // bölge no -> ad
	TArray<FMahzenKapiKaydi> KapiListesi;

	FTimerHandle KurulumZamanlayici;
};

UCLASS()
class MYPROJECT2_API UMahzenHarita : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Bir oda mühürlendikten sonra çağrılır. Sırayla bakar:
	 *   1) bütün mühürler basıldıysa  -> ZAFER ekranı,
	 *   2) oyuncu kendini kilitlediyse -> KİLİTLİ KALDIN ekranı,
	 *   3) oyun bitirilemez hale geldiyse -> ÇIKMAZ ekranı (sebebiyle birlikte).
	 * Hiçbiri yoksa hiçbir şey yapmaz.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|OyunSonu", meta = (WorldContext = "DunyaBaglami"))
	static void MuhurSonrasiDegerlendir(const UObject* DunyaBaglami);

	/** Seviyedeki benzersiz mühür adı sayısı (kazanma eşiği). */
	UFUNCTION(BlueprintPure, Category = "Mahzen|OyunSonu", meta = (WorldContext = "DunyaBaglami"))
	static int32 ToplamMuhurSayisi(const UObject* DunyaBaglami);

	/** Şu an mühürlenmiş benzersiz mühür adı sayısı. */
	UFUNCTION(BlueprintPure, Category = "Mahzen|OyunSonu", meta = (WorldContext = "DunyaBaglami"))
	static int32 MuhurlenenSayisi(const UObject* DunyaBaglami);
};
