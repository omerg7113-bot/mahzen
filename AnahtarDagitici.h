#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AnahtarDagitici.generated.h"

/** Haritadaki odalar. Sıra önemsiz; graf aşağıda kurulur. */
UENUM(BlueprintType)
enum class EMahzenOda : uint8
{
	Koridor     UMETA(DisplayName = "Koridor"),
	CocukOdasi  UMETA(DisplayName = "Çocuk Odası"),
	YatakOdasi  UMETA(DisplayName = "Yatak Odası"),
	Ofis        UMETA(DisplayName = "Ofis"),
	Lavabo      UMETA(DisplayName = "Lavabo"),
	Merdiven    UMETA(DisplayName = "Merdiven"),
	Mutfak      UMETA(DisplayName = "Mutfak"),
	Tapinak     UMETA(DisplayName = "Tapınak"),
	Kutuphane   UMETA(DisplayName = "Kütüphane"),
	Salon       UMETA(DisplayName = "Salon"),
	Depo        UMETA(DisplayName = "Depo"),
	ODA_SAYISI  UMETA(Hidden)
};

/**
 * Oda anahtarlarını SOFTLOCK ÜRETMEYECEK şekilde rastgele dağıtır.
 *
 * Neden gerekli: mevcut sistemde oda anahtarları `AnahtarKatman|2` (1. kat) ve
 * `|3` (2. kat) gruplarına doğuyor; bu gruplar X 608..2527, Y 874..3567 ile
 * KATIN TAMAMINI kapsıyor, yani kilitli odaların içini de. Dolayısıyla
 * "depo anahtarı salonda, salon anahtarı depoda" durumu üretilebiliyor ve
 * oyun bitirilemez hale geliyor.
 *
 * Çözüm: anahtarlar yalnızca KİMLİĞİ BİLİNEN bölgelere konur — serbest alan
 * (`AnahtarKatman|1`, giriş/koridor) ve 20 odaya özel grup (`|101..|120`).
 * Yerleştirme sırası "ileri erişilebilirlik" kuralını izler:
 *
 *   Her anahtar, YALNIZCA o ana kadar yerleştirilmiş anahtarlarla ulaşılabilen
 *   bir bölgeye konur.
 *
 * Bu bir değişmez (invariant): oyuncu anahtarları yerleştirme sırasıyla
 * toplayabilir, dolayısıyla zincir hiçbir noktada kopmaz. Kilitlenme
 * matematiksel olarak imkânsız hale gelir; rastgelelik korunur.
 *
 * Çalışma anı: `PostInitializeComponents`. Unreal önce TÜM aktörlerin
 * PostInitializeComponents'ini çağırıp BeginPlay'leri sonra dağıttığı için,
 * `bm_key` kendi ışınlanmasını yapmadan önce `KatmanNo`'su yazılmış olur.
 * Böylece Blueprint'e dokunmaya ve BeginPlay sırasına bel bağlamaya gerek kalmaz.
 */
UCLASS()
class MYPROJECT2_API AAnahtarDagitici : public AActor
{
	GENERATED_BODY()

public:
	AAnahtarDagitici();

	/** Kapatılırsa dağıtım hiç çalışmaz, anahtarlar editördeki KatmanNo'suyla doğar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mahzen|Dagitim")
	bool bEtkin = true;

	/**
	 * 0 = her oyunda farklı. Sıfırdan farklıysa sabit tohum: aynı dağılım
	 * tekrar üretilir (hata ayıklama ve "şu dağılımı bir daha oynat" için).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Dagitim")
	int32 Tohum = 0;

	/** Serbest alan (giriş + koridor) anahtar noktalarının katman numarası. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mahzen|Dagitim")
	int32 SerbestKatman = 1;

	/** 1. kat oda gruplarının başlangıcı: 101..110 sırası OdaSirasi ile eşleşir. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mahzen|Dagitim")
	int32 Kat1GrupBaslangic = 101;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mahzen|Dagitim")
	int32 Kat2GrupBaslangic = 111;

	/**
	 * Bir oda grubunda bu kadar nokta boş kalmadıysa oraya oda anahtarı koyma.
	 * Kasa anahtarları aynı gruplardan nokta tükettiği için pay bırakıyoruz
	 * (bazı gruplarda yalnızca 3 nokta var).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Dagitim")
	int32 GrupYedekPayi = 1;

	/** Logda dağılımı yazdır. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Dagitim")
	bool bLogla = true;

	/** Dağıtımı elle tetikle (editörde test için). Normalde otomatik çalışır. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Dagitim")
	void Dagit();

protected:
	virtual void PostInitializeComponents() override;

private:
	/** Oda adlarının anahtar/mühür metinlerindeki karşılığı. 101..110 sırası. */
	static const TCHAR* OdaAnahtarParcasi(EMahzenOda Oda);

	/** "1. Kat ofis anahtarı" gibi bir addan (kat, oda) çıkarır. */
	static bool AdiCozumle(const FString& Ad, int32& OutKat, EMahzenOda& OutOda);

	/** Erişilebilirlik sabit noktası: eldeki anahtarlarla hangi odalara girilir. */
	static void ErisimHesapla(const bool bAnahtarVar[2][(uint8)EMahzenOda::ODA_SAYISI],
		bool bOdaErisilir[2][(uint8)EMahzenOda::ODA_SAYISI]);
};
