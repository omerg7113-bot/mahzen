#include "EtkilesimSunucu.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Misc/OutputDeviceNull.h"

UEtkilesimSunucu::UEtkilesimSunucu()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	IzinliOlaylar = {FName(TEXT("KapiyiKullan")), FName(TEXT("OdayiMuhurle"))};
}

void UEtkilesimSunucu::KapiyiKullan(AActor* Kapi)
{
	OlayCagir(Kapi, FName(TEXT("KapiyiKullan")), EKizOlay::Kapi);
}

void UEtkilesimSunucu::OdayiMuhurle(AActor* Kapi)
{
	OlayCagir(Kapi, FName(TEXT("OdayiMuhurle")), EKizOlay::Muhur);
}

void UEtkilesimSunucu::OlayCagir(AActor* Hedef, FName OlayAdi, EKizOlay KizOlayi)
{
	AActor* Sahip = GetOwner();
	if (!Sahip || !Hedef)
	{
		return;
	}

	if (Sahip->HasAuthority())
	{
		SunucudaYurut(Hedef, OlayAdi, KizOlayi);
	}
	else
	{
		ServerOlayCagir(Hedef, OlayAdi, KizOlayi);
	}
}

bool UEtkilesimSunucu::ServerOlayCagir_Validate(AActor* Hedef, FName OlayAdi, EKizOlay KizOlayi)
{
	return Hedef != nullptr && IzinliOlaylar.Contains(OlayAdi);
}

void UEtkilesimSunucu::ServerOlayCagir_Implementation(AActor* Hedef, FName OlayAdi, EKizOlay KizOlayi)
{
	SunucudaYurut(Hedef, OlayAdi, KizOlayi);
}

void UEtkilesimSunucu::SunucudaYurut(AActor* Hedef, FName OlayAdi, EKizOlay KizOlayi)
{
	AActor* Sahip = GetOwner();
	UWorld* Dunya = GetWorld();
	if (!Sahip || !Hedef || !Dunya || !IzinliOlaylar.Contains(OlayAdi))
	{
		return;
	}

	if (MaksMenzilCm > 0.f
		&& FVector::Dist(Sahip->GetActorLocation(), Hedef->GetActorLocation()) > MaksMenzilCm)
	{
		UE_LOG(LogTemp, Warning, TEXT("EtkilesimSunucu: %s cok uzak, %s reddedildi."),
			*Hedef->GetName(), *OlayAdi.ToString());
		return;
	}

	if (!YerelCagir(Hedef, OlayAdi))
	{
		UE_LOG(LogTemp, Warning, TEXT("EtkilesimSunucu: %s uzerinde %s olayi bulunamadi."),
			*Hedef->GetName(), *OlayAdi.ToString());
		return;
	}

	MulticastOlayCagir(Hedef, OlayAdi);

	if (KizOlayi != EKizOlay::Yok)
	{
		for (TActorIterator<AActor> It(Dunya); It; ++It)
		{
			if (UKizHedefSecici* Secici = It->FindComponentByClass<UKizHedefSecici>())
			{
				Secici->OlayBildir(KizOlayi, Hedef->GetActorLocation(), Sahip);
			}
		}
	}
}

void UEtkilesimSunucu::MulticastOlayCagir_Implementation(AActor* Hedef, FName OlayAdi)
{
	// Sunucu olayı zaten çalıştırdı; yalnız istemciler yerelde çalıştırır (ses, animasyon).
	const AActor* Sahip = GetOwner();
	if (!Hedef || (Sahip && Sahip->HasAuthority()) || !IzinliOlaylar.Contains(OlayAdi))
	{
		return;
	}
	YerelCagir(Hedef, OlayAdi);
}

bool UEtkilesimSunucu::YerelCagir(AActor* Hedef, FName OlayAdi) const
{
	if (!Hedef || !Hedef->FindFunction(OlayAdi))
	{
		return false;
	}

	FOutputDeviceNull Cikti;
	return Hedef->CallFunctionByNameWithArguments(*OlayAdi.ToString(), Cikti, nullptr, true);
}
