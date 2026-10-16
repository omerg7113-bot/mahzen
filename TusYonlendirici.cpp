#include "TusYonlendirici.h"
#include "MahzenAyar.h"
#include "MahzenArayuz.h"
#include "OyunDurumu.h"
#include "Components/InputComponent.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogTusYonlendirici, Log, All);

UTusYonlendirici::UTusYonlendirici()
{
	// InputComponent possess olmadan var olmuyor; ilk tiklarda yakalanacak.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
}

void UTusYonlendirici::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bBagli)
	{
		YenidenBagla();
	}
	else
	{
		// Bağlama kurulduktan sonra tike gerek yok.
		PrimaryComponentTick.SetTickFunctionEnable(false);
	}
}

void UTusYonlendirici::YenidenBagla()
{
	APawn* Sahip = Cast<APawn>(GetOwner());
	if (!Sahip)
	{
		return;
	}

	UInputComponent* IC = Sahip->InputComponent;
	if (!IC)
	{
		return;   // henüz possess edilmemiş
	}

	// Önce bu bileşenin daha önce eklediği bağlamaları sök. Sahibin kendi
	// bağlamaları (Jump, PrimaryAction) BagliTuslar'da olmadığı için korunur.
	for (int32 i = IC->KeyBindings.Num() - 1; i >= 0; --i)
	{
		if (BagliTuslar.Contains(IC->KeyBindings[i].Chord.Key))
		{
			IC->KeyBindings.RemoveAt(i);
		}
	}
	BagliTuslar.Reset();

	int32 Sayi = 0;
	for (const FName Eylem : UMahzenAyar::TusEylemleri())
	{
		const FKey Tus = UMahzenAyar::TusOku(Eylem);
		if (!Tus.IsValid())
		{
			continue;
		}

		FInputKeyBinding Bagla(FInputChord(Tus, false, false, false, false), IE_Pressed);
		// Girdi TÜKETİLMEMELİ: oyuncu aynı harfi birden fazla eyleme atayabilsin
		// (örn. hepsini E yapmak isterse hepsi birden tetiklenir).
		Bagla.bConsumeInput = false;
		Bagla.KeyDelegate.GetDelegateForManualSet().BindLambda(
			[this, Eylem]()
			{
				EylemiCalistir(Eylem, true);
			});
		IC->KeyBindings.Add(Bagla);

		// Bırakma da bağlanıyor: anahtar bırakma gibi basılı tutma mekanikleri
		// `Eylem_<Ad>_Birak` custom event'ini kullanır. Olmayan eylemler için
		// sessizce atlanır.
		FInputKeyBinding Birak(FInputChord(Tus, false, false, false, false), IE_Released);
		Birak.bConsumeInput = false;
		Birak.KeyDelegate.GetDelegateForManualSet().BindLambda(
			[this, Eylem]()
			{
				EylemiCalistir(FName(*(Eylem.ToString() + TEXT("_Birak"))), false);
			});
		IC->KeyBindings.Add(Birak);

		BagliTuslar.AddUnique(Tus);
		++Sayi;
	}

	// --- DURAKLATMA: S ---------------------------------------------------
	// Sabit tuş (ayarlardan değiştirilen listede yok). `bExecuteWhenPaused`
	// şart: oyun duraklatılmışken de çalışsın ki P ikinci kez basıldığında
	// menü kapanıp oyuna dönülebilsin.
	{
		FInputKeyBinding Duraklat(FInputChord(EKeys::S, false, false, false, false), IE_Pressed);
		Duraklat.bConsumeInput = false;
		Duraklat.bExecuteWhenPaused = true;
		Duraklat.KeyDelegate.GetDelegateForManualSet().BindLambda(
			[this]()
			{
				UMahzenArayuz::DuraklatmaAcKapa(GetOwner());
			});
		IC->KeyBindings.Add(Duraklat);
		BagliTuslar.AddUnique(EKeys::S);
	}

	bBagli = Sayi > 0;
	UE_LOG(LogTusYonlendirici, Log, TEXT("Tus baglama: %d eylem, %d tus (+S duraklatma)"),
		Sayi, BagliTuslar.Num());
}

void UTusYonlendirici::EylemiCalistir(FName Eylem, bool bZorunlu)
{
	AActor* Sahip = GetOwner();
	if (!Sahip)
	{
		return;
	}

	// Harita MENÜ kipinde açıldıysa oynanış tuşları (al/kapı/kasa/mühürle/
	// envanter/fener) çalışmasın — menünün arkasında oyun oynanmasın.
	if (UOyunDurumu::Mod() == EOyunModu::Menu)
	{
		return;
	}

	// Karakter Blueprint'indeki custom event: Eylem_Etkilesim, Eylem_Fener ...
	const FName FnAd(*FString::Printf(TEXT("Eylem_%s"), *Eylem.ToString()));
	if (UFunction* Fn = Sahip->FindFunction(FnAd))
	{
		Sahip->ProcessEvent(Fn, nullptr);
	}
	else if (bZorunlu)
	{
		UE_LOG(LogTusYonlendirici, Warning,
			TEXT("%s custom event'i karakter Blueprint'inde yok, eylem calismadi."),
			*FnAd.ToString());
	}
}

