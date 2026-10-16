// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "ModDuyurusuWidget.generated.h"

UCLASS()
class MYPROJECT2_API UModDuyurusuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void MetniAyarla(const FString& Metin);

protected:
	// Widget Designer'daki Text Block'un adıyla BİREBİR aynı olmalı
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TextBlock_0;
};