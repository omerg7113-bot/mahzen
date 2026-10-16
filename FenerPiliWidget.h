#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FenerPiliWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;

/**
 * Sol altta duran pil göstergesi. Kendi tiki YOK — değer değiştikçe `UFenerPili`
 * çağırır (tek tik kaynağı, ana menüde de boşa çalışmaz).
 *
 * UMG Designer'daki widget adları aşağıdakilerle BİREBİR aynı olmalı.
 * Sadece `PilCubugu` zorunlu; diğerleri silinse de widget çalışır.
 */
UCLASS()
class MYPROJECT2_API UFenerPiliWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Doluluk (0..1) ve eşiklere göre belirlenen rengi uygular. */
	UFUNCTION(BlueprintCallable, Category = "Pil", meta = (DisplayName = "Pili Güncelle"))
	void Guncelle(float Yuzde, FLinearColor Renk);

	/** Pil kutusunun içindeki dolum çubuğu. */
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PilCubugu = nullptr;

	/** "%100" yazısı. */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* PilYazisi = nullptr;

	/** Pilin sağındaki küçük uç (simge görünümü için). */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* PilBasi = nullptr;
};
