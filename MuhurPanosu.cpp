#include "MuhurPanosu.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"

namespace
{
	/** Listede görünme sırası: koridordan içeriye doğru. */
	const TCHAR* GOdaSirasi[] = {
		TEXT("Çocuk Odası"),
		TEXT("Yatak Odası"),
		TEXT("Ofis"),
		TEXT("Lavabo"),
		TEXT("Merdiven"),
		TEXT("Mutfak"),
		TEXT("Tapınak"),
		TEXT("Kütüphane"),
		TEXT("Salon"),
		TEXT("Depo")
	};

	/**
	 * Oda kendi mührünün yanında başka bir odanın mührünü de bekliyorsa onu
	 * döndürür. Sebep: o odaya ikinci bir kapı daha açılıyor.
	 *   Salon   -> Depo    (deponun tek girişi salon)
	 *   Tapınak -> Mutfak  (mutfağın ikinci kapısı tapınağa açılır)
	 * Kütüphane yalnızca tapınaktan girilir ama ayrı satır olarak yazılır;
	 * tapınağın yazması için gerekmez (kullanıcının kuralı).
	 */
	const TCHAR* EkKosul(const FString& Oda)
	{
		if (Oda == TEXT("Salon"))
		{
			return TEXT("Depo");
		}
		if (Oda == TEXT("Tapınak"))
		{
			return TEXT("Mutfak");
		}
		return nullptr;
	}

	FString MuhurAdi(int32 Kat, const FString& Oda)
	{
		// Haritadaki birebir kalıp: "1. Kat Yatak Odası mührü"
		return FString::Printf(TEXT("%d. Kat %s mührü"), Kat, *Oda);
	}
}

FString UMuhurPanosu::MuhurlenenOdalarMetni(const UObject* DunyaBaglami)
{
	UWorld* Dunya = GEngine
		? GEngine->GetWorldFromContextObject(DunyaBaglami, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!Dunya)
	{
		return FString();
	}

	// --- 1) Mühürlenmiş kapılardan mühür adlarını topla ----------------------
	TSet<FString> Muhurlu;

	for (TActorIterator<AActor> It(Dunya); It; ++It)
	{
		AActor* A = *It;
		if (!A || !A->GetClass()->GetName().StartsWith(TEXT("bp_kap")))
		{
			continue;
		}

		FStrProperty* MuhurP = FindFProperty<FStrProperty>(A->GetClass(), TEXT("GerekliMuhurAdi"));
		FBoolProperty* DurumP = FindFProperty<FBoolProperty>(A->GetClass(), TEXT("bMuhurlenmis"));
		if (!MuhurP || !DurumP)
		{
			continue;
		}

		if (!DurumP->GetPropertyValue_InContainer(A))
		{
			continue;
		}

		const FString Ad = MuhurP->GetPropertyValue_InContainer(A);
		if (!Ad.IsEmpty())
		{
			Muhurlu.Add(Ad);
		}
	}

	// --- 2) Kat kat yaz ------------------------------------------------------
	FString Metin;

	for (int32 Kat = 1; Kat <= 2; ++Kat)
	{
		TArray<FString> Yazilacak;

		for (const TCHAR* OdaRaw : GOdaSirasi)
		{
			const FString Oda(OdaRaw);

			if (!Muhurlu.Contains(MuhurAdi(Kat, Oda)))
			{
				continue;
			}

			if (const TCHAR* Ek = EkKosul(Oda))
			{
				if (!Muhurlu.Contains(MuhurAdi(Kat, FString(Ek))))
				{
					continue;   // odaya açılan diğer kapı hâlâ açık
				}
			}

			Yazilacak.Add(Oda);
		}

		if (Yazilacak.Num() == 0)
		{
			continue;
		}

		if (!Metin.IsEmpty())
		{
			Metin += TEXT("\n");
		}
		Metin += FString::Printf(TEXT("%d. Kat"), Kat);
		for (const FString& Oda : Yazilacak)
		{
			Metin += TEXT("\n  ") + Oda;
		}
	}

	return Metin;
}
