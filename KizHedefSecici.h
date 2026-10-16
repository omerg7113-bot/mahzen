#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KizHedefSecici.generated.h"

/** Kızın görüşte kimse yokken tepki verebileceği olay türleri. */
UENUM(BlueprintType)
enum class EKizOlay : uint8
{
	Yok   UMETA(DisplayName = "Yok (Devriye)"),
	Esya  UMETA(DisplayName = "Eşya Bırakma"),
	Kapi  UMETA(DisplayName = "Kapı Açma/Kapama"),
	Muhur UMETA(DisplayName = "Mühür Kapanması"),
	Fener UMETA(DisplayName = "El Feneri Algısı")
};

/** Bir hedef adayı: bir olay ya da kıza isabet eden bir fener ışığı. */
USTRUCT(BlueprintType)
struct FKizAday
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Kiz")
	EKizOlay Tur = EKizOlay::Yok;

	/** Kızın yürüyeceği nokta. Fenerde, kaynağa doğru birkaç adımlık ara noktadır. */
	UPROPERTY(BlueprintReadOnly, Category = "Kiz")
	FVector Konum = FVector::ZeroVector;

	/** Olayın gerçekten olduğu yer (mesafe buradan ölçülür). Fenerde ışığı tutan oyuncunun konumu. */
	UPROPERTY(BlueprintReadOnly, Category = "Kiz")
	FVector OlayKonumu = FVector::ZeroVector;

	/** Olayı doğuran oyuncu / aktör. Fenerde kızın dönüp bakacağı kişi. */
	UPROPERTY(BlueprintReadOnly, Category = "Kiz")
	TWeakObjectPtr<AActor> Kaynak;

	/** Olayı yapan oyuncunun pili, 0..1. */
	UPROPERTY(BlueprintReadOnly, Category = "Kiz")
	float PilOrani = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Kiz")
	bool bFenerAcik = true;

	/** Kızdan olaya yürüyerek yol (cm). Eksi = henüz hesaplanmadı. */
	UPROPERTY(BlueprintReadOnly, Category = "Kiz")
	float YolUzunlugu = -1.f;

	/** Yol / (tür × pil × fener × grup çarpanı). Küçük olan kazanır. */
	UPROPERTY(BlueprintReadOnly, Category = "Kiz")
	float EtkinMesafe = -1.f;

	double DogusZamani = 0.0;
	float OmurSn = 0.f;
};

/** Bir olay türünün çarpanı ve duyulma kuralları. */
USTRUCT(BlueprintType)
struct FKizTurAyari
{
	GENERATED_BODY()

	/** Büyük çarpan = aynı mesafede daha çekici. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kiz", meta = (ClampMin = "0.0"))
	float Carpan = 1.f;

	/** Duyma menzili: kızdan olaya yürüyerek yol (cm). 0 = sınırsız. Pil menzili uzatmaz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kiz", meta = (ClampMin = "0.0"))
	float Menzil = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kiz")
	bool bKatlarArasi = false;

	/** Aday kaç saniye geçerli kalır (hedef seçilince yürüme süresine göre uzar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kiz", meta = (ClampMin = "0.1"))
	float OmurSn = 16.f;
};

/** Bir oyuncunun fenerine verilen tepkilerin kaydı (alışma için). */
struct FFenerKayit
{
	double Son = -1.0e9;
	int32 Sayi = 0;
};

/** Bir oyuncunun aynı yerdeki olayına kızın boşuna gidişleri (alışma için). */
struct FAlismaKayit
{
	TWeakObjectPtr<AActor> Kaynak;
	FVector Konum = FVector::ZeroVector;
	double Son = 0.0;
	int32 Sayi = 0;
};

/**
 * Kıza eklenir. Yalnız sunucuda çalışır. Görüşte kimse yokken (Takip bitince)
 * olayları ve fener ışıklarını aday olarak toplar.
 *
 * KURAL: görüşe biri girmedikçe ya da kıza ışık tutulmadıkça, seçilen olay
 * yerine MUTLAKA gidilir. Yani:
 *   1) Kıza ışık isabet ettiyse (fener adayı) kız ona tepki verir; bu, yürüdüğü
 *      olayı keser, ama bitince kesilen olaya geri döner.
 *   2) Aksi halde, daha önce seçilmiş bir olay hedefi varsa ondan vazgeçilmez;
 *      başka bir olay onu bozmaz.
 *   3) Aksi halde adaylar arasından "etkin mesafe"si en küçük olan seçilir:
 *        etkin mesafe = yol / (tür × pil × fener × grup çarpanı)
 *        pil çarpanı  = 1 + PilKatsayisi × (1 − pil)^PilUssu
 *
 * Fener, "dönüp bak + kaynağa birkaç adım at" tepkisidir: aday konumu kaynağa
 * `FenerAdimCm` kadar yaklaşan ara noktadır; `Kaynak` kızın bakacağı oyuncudur.
 * Aynı oyuncunun feneri kız birini bulamadan `FenerAlismaSayisi` kez tepki
 * aldırdıysa, sayaç sıfırlanana kadar yok sayılır.
 *
 * Takip (görüş / son görülen konum) sürerken `TakipAktifAyarla(true)` çağrılır;
 * o sırada seçim yapılmaz, yol sorgusu bile çalışmaz.
 *
 * ADIM 1: bileşen yalnız seçer ve değişimi loga yazar (`bLogla`). Kızı
 * yürütmez; Blueprint henüz `EnIyiAday`ı okumuyor.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "Kiz Hedef Secici"))
class MYPROJECT2_API UKizHedefSecici : public UActorComponent
{
	GENERATED_BODY()

public:
	UKizHedefSecici();

	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secici")
	bool bAktif = true;

	/** Seçim değişince loga yazar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secici")
	bool bLogla = true;

	/** Mod tablosu: Hayalet'te false (hiç tetik almaz). Blueprint mod değişince `ModAyarla` çağırır. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Secici|Mod")
	bool bModDinler = true;

	/** Takipçi: false (yalnız kendi katı). Agresif: true (iki kat). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Secici|Mod")
	bool bModKatlarArasi = false;

	UPROPERTY(EditAnywhere, Category = "Secici", meta = (ClampMin = "0.05"))
	float KararAraligiSn = 0.2f;

	/** Çok yakın olaylarda bölme patlamasın diye en küçük mesafe (cm). */
	UPROPERTY(EditAnywhere, Category = "Secici", meta = (ClampMin = "1.0"))
	float EnAzMesafeCm = 100.f;

	/** Hedef seçilince olayın ömrü en az: yol / bu hız + VarisPayiSn (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Secici", meta = (ClampMin = "10.0"))
	float MinYurumeHiziCm = 140.f;

	UPROPERTY(EditAnywhere, Category = "Secici", meta = (ClampMin = "0.0"))
	float VarisPayiSn = 4.f;

	/** Pil çarpanı = 1 + PilKatsayisi × (1 − pil)^PilUssu. Pil %0 iken 1 + PilKatsayisi. */
	UPROPERTY(EditAnywhere, Category = "Secici|Pil", meta = (ClampMin = "0.0"))
	float PilKatsayisi = 1.6f;

	UPROPERTY(EditAnywhere, Category = "Secici|Pil", meta = (ClampMin = "0.1"))
	float PilUssu = 4.5f;

	/** Fener kapalı (ya da pili bitmiş) oyuncunun olayı bu çarpanla biraz daha az çekici. */
	UPROPERTY(EditAnywhere, Category = "Secici|Pil", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float FenerKapaliCarpani = 0.92f;

	/** Olayın yakınındaki her EK oyuncu çekiciliği bu kadar artırır (0.1 = %10). */
	UPROPERTY(EditAnywhere, Category = "Secici|Grup", meta = (ClampMin = "0.0"))
	float GrupBonusOyuncuBasina = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Secici|Grup", meta = (ClampMin = "0.0"))
	float GrupBonusEnFazla = 0.3f;

	/** "Yakınında" sayılma yarıçapı (cm). */
	UPROPERTY(EditAnywhere, Category = "Secici|Grup", meta = (ClampMin = "0.0"))
	float GrupYaricapCm = 500.f;

	/** Kız bir olaya gidip döndüyse, aynı oyuncunun aynı yerdeki sonraki olayı her gidiş için bu kadar daha az çekici olur (0.08 = %8). */
	UPROPERTY(EditAnywhere, Category = "Secici|Alisma", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float AlismaCezasiGidisBasina = 0.08f;

	/** Alışma çarpanının inebileceği en düşük değer (0.75 = en fazla %25 azalma). */
	UPROPERTY(EditAnywhere, Category = "Secici|Alisma", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float AlismaEnAzCarpan = 0.75f;

	/** Son gidişten bu kadar süre sonra alışma unutulur. */
	UPROPERTY(EditAnywhere, Category = "Secici|Alisma", meta = (ClampMin = "0.0"))
	float AlismaSifirlamaSn = 60.f;

	/** "Aynı yer" sayılma yarıçapı (cm). */
	UPROPERTY(EditAnywhere, Category = "Secici|Alisma", meta = (ClampMin = "0.0"))
	float AlismaYaricapCm = 300.f;

	/** Işığın kıza ulaşabileceği en uzak mesafe (cm); ışığın kendi menziliyle sınırlanır. */
	UPROPERTY(EditAnywhere, Category = "Secici|Fener", meta = (ClampMin = "0.0"))
	float FenerIsikMenziliCm = 3500.f;

	/** Kızın ışığın kaynağına doğru atacağı adım uzunluğu (cm). */
	UPROPERTY(EditAnywhere, Category = "Secici|Fener", meta = (ClampMin = "0.0"))
	float FenerAdimCm = 500.f;

	/** Aynı oyuncunun iki tepkisi arasında en az geçecek süre. */
	UPROPERTY(EditAnywhere, Category = "Secici|Fener", meta = (ClampMin = "0.0"))
	float FenerTekrarBeklemeSn = 8.f;

	/** Kız kimseyi bulamadan bir oyuncunun feneri en fazla bu kadar tepki alır (sonra yok sayılır). */
	UPROPERTY(EditAnywhere, Category = "Secici|Fener", meta = (ClampMin = "1"))
	int32 FenerAlismaSayisi = 3;

	/** Son tepkiden bu kadar süre sonra alışma sayacı sıfırlanır. */
	UPROPERTY(EditAnywhere, Category = "Secici|Fener", meta = (ClampMin = "0.0"))
	float FenerAlismaSifirlamaSn = 60.f;

	/** Kat indeksi = floor((z - KatTabanZ) / KatYuksekligi). */
	UPROPERTY(EditAnywhere, Category = "Secici|Kat")
	float KatTabanZ = 310.f;

	UPROPERTY(EditAnywhere, Category = "Secici|Kat", meta = (ClampMin = "1.0"))
	float KatYuksekligi = 293.f;

	/** İndeks = EKizOlay değeri (Yok, Eşya, Kapı, Mühür, Fener). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secici")
	TArray<FKizTurAyari> TurAyarlari;

	/** Eşya / kapı / mühür olayı bildirir (Blueprint'ten, yalnız sunucuda). Kaynak = olayı yapan oyuncu. */
	UFUNCTION(BlueprintCallable, Category = "Secici", meta = (DisplayName = "Olay Bildir"))
	void OlayBildir(EKizOlay Tur, FVector Konum, AActor* Kaynak);

	/** Hayalet: (false, false). Takipçi: (true, false). Agresif: (true, true). Dinlemeyen modda adaylar silinir. */
	UFUNCTION(BlueprintCallable, Category = "Secici", meta = (DisplayName = "Mod Ayarla"))
	void ModAyarla(bool bDinler, bool bKatlarArasi);

	/** Görüş ya da son-görülen-konum takibi sürerken true yapılır. Başlarken fener sayaçları sıfırlanır. */
	UFUNCTION(BlueprintCallable, Category = "Secici", meta = (DisplayName = "Takip Aktif Ayarla"))
	void TakipAktifAyarla(bool bTakipte);

	/** Seçilen hedefe varılınca / vazgeçilince adayı listeden siler. Fener bitince kesilen olaya döner. */
	UFUNCTION(BlueprintCallable, Category = "Secici", meta = (DisplayName = "En Iyi Adayi Tuket"))
	void EnIyiAdayiTuket();

	UFUNCTION(BlueprintPure, Category = "Secici", meta = (DisplayName = "En Iyi Aday"))
	FKizAday EnIyiAday() const { return EnIyi; }

	/** Devriyeden farklı bir hedef var mı. */
	UFUNCTION(BlueprintPure, Category = "Secici", meta = (DisplayName = "Hedef Var mi"))
	bool HedefVarMi() const { return EnIyi.Tur != EKizOlay::Yok; }

	/** Pil çarpanı (1 = etkisiz). pil: 0..1. */
	UFUNCTION(BlueprintPure, Category = "Secici", meta = (DisplayName = "Pil Carpani"))
	float PilCarpani(float PilOrani) const;

private:
	const FKizTurAyari* Ayar(EKizOlay Tur) const;
	int32 KatIndeksi(float Z) const;
	bool AyniAdayMi(const FKizAday& A, const FKizAday& B) const;
	float EtkinMesafeHesapla(const FKizAday& Aday, float YolCm, float GrupC) const;
	float GrupCarpani(const FKizAday& Aday) const;
	float AlismaCarpani(const FKizAday& Aday, double Simdi) const;
	float YolUzunluguHesapla(const FVector& Baslangic, const FVector& Hedef) const;
	void PilBilgisiOku(const AActor* Kaynak, float& PilOrani, bool& bFenerAcik) const;
	FVector FenerAdimHedefi(const FVector& KizKonum, const FVector& KaynakKonum) const;

	void FenerleriTara(double Simdi);
	FKizAday EnIyiSec();
	void SecimiGuncelle(const FKizAday& Yeni);

	TArray<FKizAday> Adaylar;
	FKizAday EnIyi;

	/** Fener tepkisi tarafından kesilen olay hedefi; fener bitince buna dönülür. */
	FKizAday Askida;

	TMap<TWeakObjectPtr<AActor>, FFenerKayit> FenerKayitlari;
	TArray<FAlismaKayit> AlismaKayitlari;

	float Birikim = 0.f;
	bool bTakipte = false;
};
