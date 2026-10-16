// Fill out your copyright notice in the Description page of Project Settings.

#include "ModDuyurusuWidget.h"

void UModDuyurusuWidget::MetniAyarla(const FString& Metin)
{
	if (TextBlock_0)
	{
		TextBlock_0->SetText(FText::FromString(Metin));
	}
}
