#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MahzenKontrolcu.generated.h"

class UUserWidget;

/**
 * Oyuncu kontrolcüsü.
 *
 * Oyuncu karakteri BP_FirstPersonCharacter (motorun ACharacter'ı) olduğu için
 * bakış süzgeci ve duraklatma karaktere değil kontrolcüye konuldu:
 *  - APawn::AddControllerYawInput zaten kontrolcünün AddYawInput'una gider,
 *    dolayısıyla fare hassasiyeti ve ters-Y her yoldan gelen girdide çalışır.
 *  - ESC duraklatma ekranını açar/kapatır.
 */
UCLASS()
class MYPROJECT2_API AMahzenKontrolcu : public APlayerController
{
	GENERATED_BODY()

public:
	AMahzenKontrolcu();

	virtual void AddYawInput(float Val) override;
	virtual void AddPitchInput(float Val) override;

	// ESC ile açılan duraklatma ekranı.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahzen|Duraklat")
	TSubclassOf<UUserWidget> DuraklatWidget;

	UFUNCTION(BlueprintCallable, Category = "Mahzen|Duraklat")
	void DuraklatAcKapa();

	UFUNCTION(BlueprintPure, Category = "Mahzen|Duraklat")
	bool DuraklatmaAcikMi() const { return AcikDuraklat != nullptr; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> AcikDuraklat = nullptr;
};
