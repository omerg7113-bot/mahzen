#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "TusYonlendirici.generated.h"

/**
 * Oyuncunun seçtiği tuşları çalışma anında bağlar.
 *
 * Neden gerek var: karakter Blueprint'indeki `InputKey` düğümleri derleme
 * anında sabit tuşa kilitli, çalışma anında değiştirilemiyor. Bu bileşen
 * kayıttaki atamaları okuyup sahibin InputComponent'ine bağlar ve tuşa
 * basıldığında karakterdeki `Eylem_<EylemAdı>` custom event'ini çağırır.
 * Yani Blueprint'teki mantık aynı kalıyor, sadece tetikleyici veri oluyor.
 */
UCLASS(ClassGroup = (Mahzen), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UTusYonlendirici : public UActorComponent
{
	GENERATED_BODY()

public:
	UTusYonlendirici();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Kayıttaki tuşları sahibin InputComponent'ine (yeniden) bağlar.
	 * Ayarlar ekranında tuş değiştirilince çağrılır; önce kendi eklediği
	 * bağlamaları söker, sonra yenilerini kurar.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mahzen|Tus")
	void YenidenBagla();

	/** Bağlama kuruldu mu (InputComponent ancak possess sonrası var olur). */
	UFUNCTION(BlueprintPure, Category = "Mahzen|Tus")
	bool BagliMi() const { return bBagli; }

private:
	/** bZorunlu=false ise custom event yoksa uyarı basılmaz (bırakma olayları). */
	void EylemiCalistir(FName Eylem, bool bZorunlu);

	bool bBagli = false;

	// Bu bileşenin bağladığı tuşlar; yeniden bağlarken yalnızca bunlar sökülür.
	TArray<FKey> BagliTuslar;
};
