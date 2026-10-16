#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OyunSonu.generated.h"

/**
 * Oyunun bitebileceği dört yol. Hepsi aynı ekranı kullanır: önce siyah perde,
 * sonra 1.5 saniyede beliren "GAME OVER", o sönünce türe özel ikinci başlık ve
 * (varsa) oyuncunun neden kaybettiğini anlatan açıklama.
 */
UENUM(BlueprintType)
enum class EOyunSonuTuru : uint8
{
	/** Kız yakaladı: GAME OVER -> YOU DIED */
	Jumpscare         UMETA(DisplayName = "Kıza yakalandı"),

	/** Bütün odalar mühürlendi: GAME OVER -> YOU WIN */
	Zafer             UMETA(DisplayName = "Bütün odalar mühürlendi"),

	/** Oyuncu bulunduğu odanın bütün çıkışlarını mühürledi ve anahtarı yok. */
	KendiniKilitledi  UMETA(DisplayName = "Odada kilitli kaldı"),

	/** Gereken bir anahtar/mühür ulaşılamaz oldu: oyun artık bitirilemez. */
	Cikmaz            UMETA(DisplayName = "Oyun bitirilemez (soft lock)")
};

UCLASS()
class MYPROJECT2_API UOyunSonu : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Oyunu bitirir: girdiyi kapatır, oyunu duraklatır ve oyun sonu ekranını açar.
	 * `Sebep` boş bırakılabilir; doluysa ikinci başlığın altında satır satır yazılır.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|OyunSonu", meta = (WorldContext = "DunyaBaglami"))
	static void Goster(const UObject* DunyaBaglami, EOyunSonuTuru Tur, const FString& Sebep);

	/** Ekran zaten açıksa true; ikinci bir bitiş çağrısı yoksayılır. */
	UFUNCTION(BlueprintPure, Category = "Mahzen|OyunSonu")
	static bool Aktif();

	/** Seviye değişiminde/çıkışta ekranı temizler. */
	static void Kapat();
};
