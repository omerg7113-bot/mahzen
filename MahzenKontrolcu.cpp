#include "MahzenKontrolcu.h"
#include "MahzenAyar.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AMahzenKontrolcu::AMahzenKontrolcu()
{
	// Duraklatılmışken de arayüz tıklamaları işlensin.
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AMahzenKontrolcu::BeginPlay()
{
	Super::BeginPlay();

	// Kayıttaki gamma/ses/kalite ayarlarını oyuna girer girmez uygula.
	UMahzenAyar::TumAyarlariUygula(this);
}

void AMahzenKontrolcu::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		// DefaultInput.ini: "MahzenDuraklat" = Escape + Gamepad_Special_Right
		InputComponent->BindAction(TEXT("MahzenDuraklat"), IE_Pressed, this,
			&AMahzenKontrolcu::DuraklatAcKapa).bExecuteWhenPaused = true;
	}
}

void AMahzenKontrolcu::AddYawInput(float Val)
{
	Super::AddYawInput(Val * UMahzenAyar::FareHassasiyeti());
}

void AMahzenKontrolcu::AddPitchInput(float Val)
{
	const float Yon = UMahzenAyar::TersYMi() ? -1.0f : 1.0f;
	Super::AddPitchInput(Val * UMahzenAyar::FareHassasiyeti() * Yon);
}

void AMahzenKontrolcu::DuraklatAcKapa()
{
	// Açıksa kapat ve oyuna dön.
	if (AcikDuraklat)
	{
		AcikDuraklat->RemoveFromParent();
		AcikDuraklat = nullptr;
		UGameplayStatics::SetGamePaused(this, false);
		bShowMouseCursor = false;
		FInputModeGameOnly Oyun;
		SetInputMode(Oyun);
		return;
	}

	if (!DuraklatWidget)
	{
		return;
	}

	AcikDuraklat = CreateWidget<UUserWidget>(this, DuraklatWidget);
	if (!AcikDuraklat)
	{
		return;
	}
	AcikDuraklat->AddToViewport(20);
	UGameplayStatics::SetGamePaused(this, true);

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI Mod;
	Mod.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mod.SetHideCursorDuringCapture(false);
	Mod.SetWidgetToFocus(AcikDuraklat->TakeWidget());
	SetInputMode(Mod);
	AcikDuraklat->SetKeyboardFocus();
}
