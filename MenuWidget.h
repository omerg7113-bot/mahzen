// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "MenuWidget.generated.h"

UCLASS()
class MYPROJECT2_API UMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// UMG Designer'daki buton isimleriyle BİREBİR aynı olmalı (BindWidget)
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_easy;

	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_normal;

	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_hard;

	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_expert;

	// "kararma" animasyonu - UMG Designer'da tanımlı, burada isimle eşleştirilecek
	UPROPERTY(Transient, meta = (BindWidgetAnim), BlueprintReadOnly, Category = "Animations")
	class UWidgetAnimation* kararma;

	UFUNCTION()
	void OnEasyClicked();

	UFUNCTION()
	void OnNormalClicked();

	UFUNCTION()
	void OnHardClicked();

	UFUNCTION()
	void OnExpertClicked();

private:
	void BaslatOyun(int32 Zorluk);
	void OyunuGercekteBaslat(int32 Zorluk);

	FTimerHandle BaslatTimerHandle;
};
