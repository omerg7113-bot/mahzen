#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MuhurPanosu.generated.h"

/**
 * Envanterdeki "Mühürlenen Odalar" sütununun metnini üretir.
 *
 * Neden C++: kural bir odanın KENDİ mührüne değil, o odaya açılan TÜM kapıların
 * mühürlenmesine bakıyor (salon depoya, tapınak mutfağa açılıyor). Bunu
 * Blueprint'te kurmak 10 oda × 2 kat için onlarca düğüm demek; burada tek
 * fonksiyon.
 *
 * Gerçeğin kaynağı seviyedeki kapılar: `bMuhurlenmis == true` olan her
 * `bp_kapı`nın `GerekliMuhurAdi` değeri "mühürlenmiş mühür" sayılır. Oyuncunun
 * envanterine değil dünyanın durumuna bakar, böylece mühür harcandıktan sonra
 * da liste doğru kalır.
 */
UCLASS()
class MYPROJECT2_API UMuhurPanosu : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Mühürlenen odaları kat kat, satır satır döndürür. Hiç yoksa boş string.
	 * Örnek:
	 *   1. Kat
	 *     Çocuk Odası
	 *     Salon
	 *   2. Kat
	 *     Lavabo
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mühür",
		meta = (WorldContext = "DunyaBaglami", DisplayName = "Mühürlenen Odalar Metni"))
	static FString MuhurlenenOdalarMetni(const UObject* DunyaBaglami);
};
