#include "DegiskenYansitici.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

namespace
{
	// Değişken kayıtlarını ayıran kontrol karakteri (değer metinlerinde geçmez).
	const TCHAR KayitAyirac = 0x1E;
}

UDegiskenYansitici::UDegiskenYansitici()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	SetIsReplicatedByDefault(true);
}

void UDegiskenYansitici::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UDegiskenYansitici, Durum);
	DOREPLIFETIME_CONDITION(UDegiskenYansitici, DurumSahip, COND_OwnerOnly);
}

void UDegiskenYansitici::BeginPlay()
{
	Super::BeginPlay();
	PrimaryComponentTick.TickInterval = GuncellemeAraligiSn;
}

FString UDegiskenYansitici::Topla() const
{
	const AActor* Sahip = GetOwner();
	FString Sonuc;
	if (!Sahip)
	{
		return Sonuc;
	}

	for (const FName& Ad : Degiskenler)
	{
		const FProperty* P = Sahip->GetClass()->FindPropertyByName(Ad);
		if (!P)
		{
			continue;
		}

		FString Deger;
		P->ExportText_InContainer(0, Deger, Sahip, Sahip, const_cast<AActor*>(Sahip), PPF_None);

		Sonuc += Ad.ToString();
		Sonuc += TEXT("=");
		Sonuc += Deger;
		Sonuc.AppendChar(KayitAyirac);
	}
	return Sonuc;
}

void UDegiskenYansitici::Uygula(const FString& Metin)
{
	AActor* Sahip = GetOwner();
	if (!Sahip || Sahip->HasAuthority())
	{
		return;
	}

	TArray<FString> Kayitlar;
	Metin.ParseIntoArray(Kayitlar, *FString::Chr(KayitAyirac), true);

	for (const FString& Kayit : Kayitlar)
	{
		int32 EsitIndeks = INDEX_NONE;
		if (!Kayit.FindChar(TEXT('='), EsitIndeks))
		{
			continue;
		}

		const FName Ad(*Kayit.Left(EsitIndeks));
		const FString Deger = Kayit.Mid(EsitIndeks + 1);

		if (FProperty* P = Sahip->GetClass()->FindPropertyByName(Ad))
		{
			P->ImportText_InContainer(*Deger, Sahip, Sahip, PPF_None);
		}
	}
}

void UDegiskenYansitici::OnRep_Durum()
{
	Uygula(Durum);
}

void UDegiskenYansitici::OnRep_DurumSahip()
{
	Uygula(DurumSahip);
}

void UDegiskenYansitici::TickComponent(float DeltaSeconds, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	const AActor* Sahip = GetOwner();
	if (!Sahip || !Sahip->HasAuthority())
	{
		return;
	}

	const FString Yeni = Topla();
	FString& Hedef = bSadeceSahibine ? DurumSahip : Durum;
	if (Yeni != Hedef)
	{
		Hedef = Yeni;
	}
}
