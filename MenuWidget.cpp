// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuWidget.h"
#include "MyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"

void UMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_easy) Btn_easy->OnClicked.AddDynamic(this, &UMenuWidget::OnEasyClicked);
	if (Btn_normal) Btn_normal->OnClicked.AddDynamic(this, &UMenuWidget::OnNormalClicked);
	if (Btn_hard) Btn_hard->OnClicked.AddDynamic(this, &UMenuWidget::OnHardClicked);
	if (Btn_expert) Btn_expert->OnClicked.AddDynamic(this, &UMenuWidget::OnExpertClicked);
}

void UMenuWidget::OnEasyClicked()
{
	BaslatOyun(1);
}

void UMenuWidget::OnNormalClicked()
{
	BaslatOyun(2);
}

void UMenuWidget::OnHardClicked()
{
	BaslatOyun(3);
}

void UMenuWidget::OnExpertClicked()
{
	BaslatOyun(4);
}

void UMenuWidget::BaslatOyun(int32 Zorluk)
{
	if (kararma)
	{
		PlayAnimation(kararma);
	}

	// 1.5 saniye bekle, sonra gerçekten başlat
	FTimerDelegate Del;
	Del.BindLambda([this, Zorluk]()
	{
		OyunuGercekteBaslat(Zorluk);
	});

	GetWorld()->GetTimerManager().SetTimer(BaslatTimerHandle, Del, 1.5f, false);
}

void UMenuWidget::OyunuGercekteBaslat(int32 Zorluk)
{
	AActor* FoundActor = UGameplayStatics::GetActorOfClass(this, AMyCharacter::StaticClass());
	AMyCharacter* Karakter = Cast<AMyCharacter>(FoundActor);

	if (Karakter)
	{
		Karakter->zorlukdereceleri = Zorluk;
		Karakter->Sayacim();
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->bShowMouseCursor = false;
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
	}

	RemoveFromParent();
}