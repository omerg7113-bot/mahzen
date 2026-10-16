#include "FenerPili.h"

#include "Blueprint/UserWidget.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "FenerPiliWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

namespace
{
	// Telefon pili mantığı: dolu yeşil, azaldıkça turuncu, kritikte kırmızı.
	const FLinearColor RenkYesil(0.10f, 0.85f, 0.25f, 1.f);
	const FLinearColor RenkTuruncu(1.00f, 0.55f, 0.05f, 1.f);
	const FLinearColor RenkKirmizi(0.92f, 0.12f, 0.10f, 1.f);
}

UFenerPili::UFenerPili()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UFenerPili::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFenerPili, bFenerAcikSunucu);
	DOREPLIFETIME_CONDITION(UFenerPili, Yuzde, COND_OwnerOnly);
}

void UFenerPili::BeginPlay()
{
	Super::BeginPlay();

	AActor* Sahip = GetOwner();
	if (!Sahip)
	{
		return;
	}

	Fener = Sahip->FindComponentByClass<USpotLightComponent>();

	UE_LOG(LogTemp, Log, TEXT("FenerPili: fener %s, sure %.0f s, yetki %s"),
		Fener.IsValid() ? TEXT("bulundu") : TEXT("YOK"),
		ToplamSureSaniye,
		Sahip->HasAuthority() ? TEXT("evet") : TEXT("hayir"));
}

bool UFenerPili::YerelMi() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return !Pawn || Pawn->IsLocallyControlled();
}

void UFenerPili::GostergeKur()
{
	if (Gosterge.IsValid() || !GostergeSinifi || !YerelMi())
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	if (UFenerPiliWidget* W = CreateWidget<UFenerPiliWidget>(PC, GostergeSinifi))
	{
		W->AddToViewport();
		Gosterge = W;
		W->Guncelle(Yuzde, Renk());
	}
}

void UFenerPili::GostergeGuncelle()
{
	if (UFenerPiliWidget* W = Gosterge.Get())
	{
		W->Guncelle(Yuzde, Renk());
	}
}

FLinearColor UFenerPili::Renk() const
{
	if (Yuzde < KirmiziEsik)
	{
		return RenkKirmizi;
	}
	if (Yuzde < TuruncuEsik)
	{
		return RenkTuruncu;
	}
	return RenkYesil;
}

void UFenerPili::Doldur(float Miktar)
{
	AActor* Sahip = GetOwner();
	if (!Sahip || !Sahip->HasAuthority())
	{
		return;
	}

	Yuzde = FMath::Clamp(Yuzde + Miktar, 0.f, 1.f);
	GostergeGuncelle();
}

bool UFenerPili::FenerIsikAcikMi() const
{
	const USpotLightComponent* F = Fener.Get();
	return F && F->GetVisibleFlag();
}

bool UFenerPili::FenerAcikMi() const
{
	const AActor* Sahip = GetOwner();
	if (Sahip && Sahip->HasAuthority())
	{
		return bFenerAcikSunucu && Yuzde > 0.f;
	}
	return FenerIsikAcikMi();
}

void UFenerPili::FeneriKapat()
{
	if (USpotLightComponent* F = Fener.Get())
	{
		F->SetVisibility(false);
	}

	// Blueprint'teki aç/kapa zinciri `FenerAcik`i tersliyor; bayrağı da
	// düşürmezsek pil bitince atılan ilk tuş "kapat" sayılır ve ışık yanmaz.
	if (AActor* Sahip = GetOwner())
	{
		if (FBoolProperty* P = FindFProperty<FBoolProperty>(Sahip->GetClass(), TEXT("FenerAcik")))
		{
			P->SetPropertyValue_InContainer(Sahip, false);
		}
	}
}

void UFenerPili::ServerFenerDurumu_Implementation(bool bAcik)
{
	// Pili bitmiş oyuncunun feneri açık sayılmaz.
	bFenerAcikSunucu = bAcik && Yuzde > 0.f;

	// Sunucudaki kopyada ışık da yansın: kızın koni ve görüş hesabı bu ışığı kullanır.
	if (USpotLightComponent* F = Fener.Get())
	{
		F->SetVisibility(bFenerAcikSunucu);
	}
}

void UFenerPili::OnRep_Yuzde()
{
	GostergeGuncelle();

	// Sahip istemcide: pil sunucuda bitti, ışığı söndür.
	if (YerelMi() && Yuzde <= 0.f && FenerIsikAcikMi())
	{
		FeneriKapat();
	}
}

void UFenerPili::OnRep_FenerAcik()
{
	// Başkasının pawn'ında ışığı sunucunun gerçeğine göre aç/kapa. Yerel oyuncu kendi ışığını kendi yönetir.
	if (YerelMi())
	{
		return;
	}
	if (USpotLightComponent* F = Fener.Get())
	{
		F->SetVisibility(bFenerAcikSunucu);
	}
}

void UFenerPili::TickComponent(float DeltaSeconds, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	AActor* Sahip = GetOwner();
	if (!Sahip)
	{
		return;
	}

	const bool bYetki = Sahip->HasAuthority();
	const bool bYerel = YerelMi();

	// 1) Yerel oyuncu: göstergeyi kur, ışığın durumunu sunucuya bildir, pil bitmişse ışığı kapat.
	if (bYerel)
	{
		GostergeKur();

		if (Yuzde <= 0.f && FenerIsikAcikMi())
		{
			FeneriKapat();
		}

		const bool bIsikAcik = FenerIsikAcikMi();
		if (bIsikAcik != bBildirilen)
		{
			bBildirilen = bIsikAcik;
			if (bYetki)
			{
				bFenerAcikSunucu = bIsikAcik && Yuzde > 0.f;
			}
			else
			{
				ServerFenerDurumu(bIsikAcik);
			}
		}
	}

	// 2) Sunucu: pili tüket. Tek gerçek burada; sahibine yayılır.
	if (bYetki)
	{
		const float Onceki = Yuzde;

		if (Yuzde <= 0.f)
		{
			if (bFenerAcikSunucu)
			{
				bFenerAcikSunucu = false;
			}
			if (FenerIsikAcikMi())
			{
				FeneriKapat();
			}
		}
		else if (bFenerAcikSunucu && ToplamSureSaniye > KINDA_SMALL_NUMBER)
		{
			Yuzde = FMath::Max(0.f, Yuzde - DeltaSeconds / ToplamSureSaniye);
			if (Yuzde <= 0.f)
			{
				bFenerAcikSunucu = false;
				FeneriKapat();
			}
		}

		// Kapalıyken hiç değişmediği için gösterge de boşuna güncellenmez.
		if (bYerel && !FMath::IsNearlyEqual(Onceki, Yuzde, 1e-5f))
		{
			GostergeGuncelle();
		}
	}
}
