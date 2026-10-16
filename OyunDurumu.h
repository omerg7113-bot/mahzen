#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OyunDurumu.generated.h"

/**
 * Haritanın hangi kipte açıldığı. Menü ayrı bir haritada değil, oyun
 * haritasının İÇİNDE çalışıyor: FirstPersonMap menü kipinde açıldığında
 * MenuAcici devreye girer (widget + menü kamerası + ambiyans + oyun duraklı),
 * oynanış kipinde MenuAcici hiçbir şey yapmaz.
 */
UENUM(BlueprintType)
enum class EOyunModu : uint8
{
	Menu     UMETA(DisplayName = "Ana menü"),
	Oynanis  UMETA(DisplayName = "Oynanış")
};

/** Bir kapının kayıtlı hali (aktör adıyla eşleşir). */
USTRUCT()
struct FKayitliKapi
{
	GENERATED_BODY()

	UPROPERTY() FString Ad;
	UPROPERTY() bool bKilidiAcildi = false;
	UPROPERTY() bool bMuhurlenmis = false;
	UPROPERTY() bool bAcik = false;
};

/** Bir kasanın kayıtlı hali. */
USTRUCT()
struct FKayitliKasa
{
	GENERATED_BODY()

	UPROPERTY() FString Ad;
	UPROPERTY() bool bAcildiMi = false;
};

/**
 * Bir eşyanın (anahtar / mühür taşı) kayıtlı hali.
 * Konum da saklanıyor: anahtarlar her oyun başında AnahtarDagitici tarafından
 * yeniden dağıtılıyor, kayıt yüklenirken o rastgele dağıtımın üzerine
 * kaydedilen yerleşim yazılmalı — yoksa envanterdeki anahtarlarla kapıların
 * istediği anahtarlar birbirini tutmaz.
 */
USTRUCT()
struct FKayitliEsya
{
	GENERATED_BODY()

	UPROPERTY() FString Ad;         // aktör adı
	UPROPERTY() FString Isim;       // anahtarısmı
	UPROPERTY() FVector Konum = FVector::ZeroVector;
	UPROPERTY() bool bVar = true;   // false ise toplanmış (yüklenince yok edilir)
};

/** P ile duraklatıp ana menüye dönüldüğünde alınan oyun anlık görüntüsü. */
UCLASS()
class MYPROJECT2_API UOyunDurumKayit : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() FString Harita;
	UPROPERTY() FDateTime Zaman;

	// --- oyuncu
	UPROPERTY() FVector OyuncuKonum = FVector::ZeroVector;
	UPROPERTY() FRotator OyuncuBakis = FRotator::ZeroRotator;
	UPROPERTY() TArray<FString> Canta;

	// --- kız
	UPROPERTY() FVector KizKonum = FVector::ZeroVector;
	UPROPERTY() FString KizMod;
	UPROPERTY() int32 KizKalanSure = 0;
	UPROPERTY() int32 Zorluk = -1;

	// --- dünya
	UPROPERTY() TArray<FKayitliKapi> Kapilar;
	UPROPERTY() TArray<FKayitliKasa> Kasalar;
	UPROPERTY() TArray<FKayitliEsya> Esyalar;
	UPROPERTY() int32 MuhurlenmisOdaSayisi = 0;
};

/**
 * Oyunu bıraktığı yerden sürdürme.
 *
 * Akış: P -> duraklatma menüsü -> "ANA MENÜYE DÖN" anlık görüntüyü kaydeder ve
 * menüye döner. Ana menüdeki "DEVAM ET" haritayı açar; harita açılışında
 * (UMahzenHaritaAltSistemi::BaslangicKurulumu, BeginPlay + 1 sn — anahtar
 * dağıtımı bittikten SONRA) kayıt dünyaya uygulanır. "YENİ OYUN" kaydı siler.
 */
UCLASS()
class MYPROJECT2_API UOyunDurumu : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Kayıtlı oyun var mı (ana menüde DEVAM ET'i göstermek için). */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Kayit")
	static bool VarMi();

	/** Dünyanın o anki halini yuvaya yazar. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Kayit", meta = (WorldContext = "DunyaBaglami"))
	static bool Kaydet(const UObject* DunyaBaglami);

	/** Kayıtlı haritayı açar ve açılışta kaydın uygulanmasını işaretler. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Kayit", meta = (WorldContext = "DunyaBaglami"))
	static void DevamEt(const UObject* DunyaBaglami);

	/** Yeni oyun: kaydı siler. */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Kayit")
	static void Temizle();

	/** Harita açılışında kayıt uygulanacak mı. */
	static bool BeklemedeMi();

	/** Sonraki harita açılışının kipi (seviye değişimini aşar). */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Kayit")
	static void ModAyarla(EOyunModu Yeni);

	/** Şu anki kip. İlk açılış MENÜ'dür. */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Kayit")
	static EOyunModu Mod();

	/** Kaydı dünyaya uygular (harita yüklendikten ~1 sn sonra çağrılır). */
	static void Uygula(UWorld* Dunya);
};
