#include "MenuAcici.h"
#include "MahzenAyar.h"
#include "MyCharacter.h"
#include "OyunDurumu.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

AMenuAcici::AMenuAcici()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMenuAcici::BeginPlay()
{
	Super::BeginPlay();

	// Menü artık ayrı bir haritada değil, OYUN HARİTASININ İÇİNDE. Harita
	// oynanış kipinde açıldıysa bu aktör hiçbir şey yapmaz: ne widget, ne
	// kamera, ne ambiyans — ve oynanış aktörlerini susturmaz.
	if (UOyunDurumu::Mod() == EOyunModu::Oynanis)
	{
		SetActorTickEnabled(false);
		return;
	}

	// Menü kipinde oyuncunun kamerası yerine menü kamerasından bakılır ve
	// oyuncu girdisi kilitlenir.
	//
	// Neden SetGamePaused DEĞİL: oyunu duraklatmak zamanlayıcıları da
	// durduruyor, "YENİ OYUN" karartma geçişi (GecisTimer) hiç tetiklenmiyor
	// ve menü kilitli kalıyordu. Oynanış aktörlerinin tiki ayrıca
	// susturulduğu için sahne yine donuk duruyor.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (MenuKamera)
		{
			PC->SetViewTargetWithBlend(MenuKamera, 0.0f);
		}
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	// Kopya haritadaki oynanış aktörleri menüde kod çalıştırmasın.
	if (bOynanisAktorleriniSustur)
	{
		OynanisAktorleriniSustur();
	}

	// Kızı SERT dondur. Yalnızca aktör tikini kapatmak yetmiyor: AIController
	// yol takibini sürdürüyor, CharacterMovement ve C++ bileşenleri (KapiAcici)
	// kendi tikleriyle çalışıyor — menü kamerasının önünden yürüyüp geçiyordu.
	for (TActorIterator<AMyCharacter> It(GetWorld()); It; ++It)
	{
		AMyCharacter* Kiz = *It;
		if (!Kiz)
		{
			continue;
		}
		if (AController* C = Kiz->GetController())
		{
			if (AAIController* AI = Cast<AAIController>(C))
			{
				AI->StopMovement();
			}
			C->SetActorTickEnabled(false);
		}
		if (UCharacterMovementComponent* Hareket = Kiz->GetCharacterMovement())
		{
			Hareket->StopMovementImmediately();
			Hareket->DisableMovement();
		}
		for (UActorComponent* Bilesen : Kiz->GetComponents())
		{
			if (Bilesen)
			{
				Bilesen->SetComponentTickEnabled(false);
			}
		}
		Kiz->SetActorTickEnabled(false);
		Kiz->SetActorHiddenInGame(true);
		Kiz->SetActorEnableCollision(false);
		UE_LOG(LogTemp, Log, TEXT("Menu: kiz donduruldu (%s)"), *Kiz->GetName());
	}

	// Kayıttaki gamma/ses/çözünürlük ayarlarını uygula.
	UMahzenAyar::TumAyarlariUygula(this);

	// İlk açılışta uyarı/kulaklık/gamma katmanı, sonrasında doğrudan menü.
	if (UMahzenAyar::IlkAcilisMi() && KalibrasyonWidget)
	{
		WidgetiAc(KalibrasyonWidget, AcikKalibrasyon);
	}
	else
	{
		WidgetiAc(MenuWidget, AcikMenu);
	}

	// Kamera salınımı için başlangıç duruşunu sakla.
	if (MenuKamera)
	{
		KameraTabanKonum = MenuKamera->GetActorLocation();
		KameraTabanRot = MenuKamera->GetActorRotation();
	}

	// Kırpışacak ışıkların taban şiddetleri.
	IsikTabanSiddet.Reset();
	for (AActor* Isik : KirpisanIsiklar)
	{
		float Taban = 0.0f;
		if (Isik)
		{
			if (ULightComponent* LC = Isik->FindComponentByClass<ULightComponent>())
			{
				Taban = LC->Intensity;
			}
		}
		IsikTabanSiddet.Add(Taban);
	}

	if (AmbiyansSesi)
	{
		AmbiyansBileseni = UGameplayStatics::SpawnSound2D(this, AmbiyansSesi, AmbiyansSiddeti, 1.0f, 0.0f, nullptr, true, false);
		// Karartma açılırken ses de sıfırdan gelsin; ilk saniyelerde yalnızca
		// alt frekanslar duyulur, tiz hiçbir şey araya girmez.
		if (AmbiyansBileseni && SesYukselmeSuresi > 0.0f)
		{
			AmbiyansBileseni->FadeIn(SesYukselmeSuresi, 1.0f);
		}
	}
	if (BasSesi)
	{
		BasBileseni = UGameplayStatics::SpawnSound2D(this, BasSesi, BasSiddeti, 1.0f, 0.0f, nullptr, true, false);
		if (BasBileseni && SesYukselmeSuresi > 0.0f)
		{
			BasBileseni->FadeIn(SesYukselmeSuresi, 1.0f);
		}
	}
	if (DamlaSesi || DamlaSesleri.Num() > 0)
	{
		// ilk damla biraz gecikmeli gelsin
		GetWorldTimerManager().SetTimer(DamlaTimer, this, &AMenuAcici::DamlaCal,
			FMath::FRandRange(1.0f, 3.0f), false);
	}

	if (bOyunuDuraklat)
	{
		UGameplayStatics::SetGamePaused(this, true);
	}
}

void AMenuAcici::OynanisAktorleriniSustur()
{
	UWorld* Dunya = GetWorld();
	if (!Dunya)
	{
		return;
	}

	int32 Susturulan = 0;
	for (TActorIterator<AActor> It(Dunya); It; ++It)
	{
		AActor* A = *It;
		if (!A || A == this)
		{
			continue;
		}
		// Yalnızca Blueprint'ten türeyen aktörler; yerel aktörler (StaticMesh,
		// ışık, fog) zaten Tick'te kod çalıştırmıyor.
		if (!A->GetClass()->IsChildOf(AActor::StaticClass()) ||
			!A->GetClass()->ClassGeneratedBy)
		{
			continue;
		}

		A->SetActorTickEnabled(false);

		// Bileşen tikleri de sussun (zamanlayıcı benzeri bileşenler için).
		for (UActorComponent* Bilesen : A->GetComponents())
		{
			if (Bilesen && Bilesen->GetClass()->ClassGeneratedBy)
			{
				Bilesen->SetComponentTickEnabled(false);
			}
		}
		++Susturulan;
	}

	UE_LOG(LogTemp, Log, TEXT("Menu: %d oynanis aktorunun tiki kapatildi"), Susturulan);
}

void AMenuAcici::WidgetiAc(TSubclassOf<UUserWidget> Sinif, TObjectPtr<UUserWidget>& Hedef)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !Sinif)
	{
		return;
	}

	Hedef = CreateWidget<UUserWidget>(PC, Sinif);
	if (!Hedef)
	{
		return;
	}
	Hedef->AddToViewport(0);
	GirdiyiArayuzeAl();

	// Oyuncu kontrolcüsü varsayılan pawn'ı sahiplendiğinde girdi modunu
	// kendi üstüne alabiliyor; kısa bir gecikmeyle bir kez daha uygula.
	GetWorldTimerManager().SetTimer(GirdiTimer, this, &AMenuAcici::GirdiyiArayuzeAl, 0.25f, false);
}

void AMenuAcici::GirdiyiArayuzeAl()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	UUserWidget* Odak = AcikKalibrasyon ? AcikKalibrasyon.Get() : AcikMenu.Get();

	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;

	// UIOnly bazı kurulumlarda fare yakalamasıyla çakışıyor; GameAndUI hem
	// imleci açık tutuyor hem de tıklamaları arayüze geçiriyor.
	FInputModeGameAndUI Mod;
	Mod.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mod.SetHideCursorDuringCapture(false);
	if (Odak)
	{
		Mod.SetWidgetToFocus(Odak->TakeWidget());
	}
	PC->SetInputMode(Mod);

	if (Odak)
	{
		Odak->SetKeyboardFocus();
	}
}

void AMenuAcici::MenuyuGoster()
{
	if (AcikKalibrasyon)
	{
		AcikKalibrasyon->RemoveFromParent();
		AcikKalibrasyon = nullptr;
	}
	UMahzenAyar::IlkAcilisiTamamla();
	if (!AcikMenu)
	{
		WidgetiAc(MenuWidget, AcikMenu);
	}
}

void AMenuAcici::MenuMenuyuGoster(UObject* DunyaBaglami)
{
	if (!DunyaBaglami)
	{
		return;
	}
	if (AActor* Bulunan = UGameplayStatics::GetActorOfClass(DunyaBaglami, AMenuAcici::StaticClass()))
	{
		if (AMenuAcici* Menu = Cast<AMenuAcici>(Bulunan))
		{
			Menu->MenuyuGoster();
		}
	}
}

void AMenuAcici::MenuAyarlariAc(UObject* DunyaBaglami)
{
	if (!DunyaBaglami)
	{
		return;
	}
	AActor* Bulunan = UGameplayStatics::GetActorOfClass(DunyaBaglami, AMenuAcici::StaticClass());
	AMenuAcici* Menu = Cast<AMenuAcici>(Bulunan);
	if (!Menu || !Menu->AyarWidget)
	{
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(Menu, 0);
	if (!PC)
	{
		return;
	}
	if (UUserWidget* Ekran = CreateWidget<UUserWidget>(PC, Menu->AyarWidget))
	{
		// menünün üstünde dursun
		Ekran->AddToViewport(5);
		// Arkadaki menü düğmeleri ayarların altından görünmesin.
		MenuWidgetGoster(Menu, false);
	}
}

void AMenuAcici::MenuWidgetGoster(UObject* DunyaBaglami, bool bGoster)
{
	if (!DunyaBaglami)
	{
		return;
	}

	AMenuAcici* Menu = Cast<AMenuAcici>(
		UGameplayStatics::GetActorOfClass(DunyaBaglami, AMenuAcici::StaticClass()));
	if (!Menu || !Menu->MenuWidget)
	{
		return;   // oyun haritasındayız (duraklatma ekranı): menü widget'ı yok
	}

	TArray<UUserWidget*> Bulunanlar;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(DunyaBaglami, Bulunanlar, Menu->MenuWidget, false);
	for (UUserWidget* W : Bulunanlar)
	{
		if (W)
		{
			W->SetVisibility(bGoster ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}

void AMenuAcici::DamlaCal()
{
	if (!bGecisBasladi)
	{
		// Liste doluysa her seferinde farklı bir gıcırtı/damla; değilse tek ses.
		USoundBase* Ses = DamlaSesi;
		if (DamlaSesleri.Num() > 0)
		{
			USoundBase* Secilen = DamlaSesleri[FMath::RandRange(0, DamlaSesleri.Num() - 1)].Get();
			if (Secilen)
			{
				Ses = Secilen;
			}
		}
		if (Ses)
		{
			UGameplayStatics::PlaySound2D(this, Ses, DamlaSiddeti,
				FMath::FRandRange(0.92f, 1.08f));
		}
	}
	const float Sonraki = FMath::FRandRange(FMath::Max(0.5f, DamlaAraligiAz),
		FMath::Max(1.0f, DamlaAraligiCok));
	GetWorldTimerManager().SetTimer(DamlaTimer, this, &AMenuAcici::DamlaCal, Sonraki, false);
}

void AMenuAcici::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Gecen += DeltaTime;

	// --- kamera salınımı: üç farklı frekans, tekrar ettiği belli olmasın ---
	if (MenuKamera && !bGecisBasladi)
	{
		const float W = 2.0f * PI * SalinimHizi * Gecen;
		const FVector Kay(
			FMath::Sin(W * 1.00f) * SalinimGenligi,
			FMath::Sin(W * 0.73f + 1.3f) * SalinimGenligi,
			FMath::Sin(W * 0.41f + 2.1f) * SalinimGenligi * 0.5f);
		MenuKamera->SetActorLocation(KameraTabanKonum + Kay);

		const FRotator AciKay(
			FMath::Sin(W * 0.57f + 0.4f) * SalinimAcisi,
			FMath::Sin(W * 0.83f + 2.6f) * SalinimAcisi,
			0.0f);
		MenuKamera->SetActorRotation(KameraTabanRot + AciKay);
	}

	// --- ışık kırpışması: hızlı gürültü + ara ara tam sönme ---
	for (int32 i = 0; i < KirpisanIsiklar.Num(); ++i)
	{
		AActor* Isik = KirpisanIsiklar[i];
		if (!Isik || !IsikTabanSiddet.IsValidIndex(i) || IsikTabanSiddet[i] <= 0.0f)
		{
			continue;
		}
		ULightComponent* LC = Isik->FindComponentByClass<ULightComponent>();
		if (!LC)
		{
			continue;
		}

		const float T = Gecen * KirpismaHizi + i * 7.3f;
		// iki sinüs + yüksek frekanslı titreşim = düzensiz kırpışma
		float Oran = 0.5f + 0.5f * (0.6f * FMath::Sin(T) + 0.4f * FMath::Sin(T * 2.7f + 1.1f));
		Oran = FMath::Lerp(KirpismaAlt, 1.0f, Oran);

		// ara ara tamamen sönsün
		const float Sonme = FMath::Frac(FMath::Sin((Gecen * 1.7f + i * 3.1f)) * 43758.5453f);
		if (Sonme < SonmeSikligi)
		{
			Oran *= 0.08f;
		}

		LC->SetIntensity(IsikTabanSiddet[i] * Oran);
	}
}

void AMenuAcici::OyunaBasla()
{
	if (bGecisBasladi)
	{
		return;
	}
	bGecisBasladi = true;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

	// Menüyü kaldır, ekranı karart.
	if (AcikMenu)
	{
		AcikMenu->RemoveFromParent();
		AcikMenu = nullptr;
	}
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FMath::Max(0.1f, GecisSuresi),
			FLinearColor::Black, false, true);
	}

	// Ambiyans sussun, yerine nefes/adım sesi gelsin.
	if (AmbiyansBileseni)
	{
		AmbiyansBileseni->FadeOut(FMath::Max(0.1f, GecisSuresi * 0.6f), 0.0f);
	}
	if (BasBileseni)
	{
		BasBileseni->FadeOut(FMath::Max(0.1f, GecisSuresi * 0.6f), 0.0f);
	}
	GetWorldTimerManager().ClearTimer(DamlaTimer);

	// Menüde "Devam Et" yazabilmek için oyunun başlatıldığını işaretle.
	UMahzenAyar::OyunBaslatildiIsaretle();
	if (GecisSesi)
	{
		UGameplayStatics::PlaySound2D(this, GecisSesi);
	}

	GetWorldTimerManager().SetTimer(GecisTimer, this, &AMenuAcici::HaritayiAc,
		FMath::Max(0.1f, GecisSuresi), false);
}

void AMenuAcici::MenuOyunaBasla(UObject* DunyaBaglami)
{
	if (!DunyaBaglami)
	{
		return;
	}
	if (AActor* Bulunan = UGameplayStatics::GetActorOfClass(DunyaBaglami, AMenuAcici::StaticClass()))
	{
		if (AMenuAcici* Menu = Cast<AMenuAcici>(Bulunan))
		{
			Menu->OyunaBasla();
		}
	}
}

void AMenuAcici::HaritayiAc()
{
	// Menü oyun haritasının içinde olduğu için bu AYNI haritayı yeniden açıyor;
	// kip oynanışa çevrildiği için bu kez MenuAcici sessiz kalır, oyun başlar.
	UOyunDurumu::ModAyarla(EOyunModu::Oynanis);
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, OyunHaritasi);
}
