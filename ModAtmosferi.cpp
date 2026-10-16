#include "ModAtmosferi.h"

#include "Camera/CameraComponent.h"
#include "Components/LightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Light.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

namespace
{
	// Güneşin (DirectionalLight) mod başına MUTLAK yoğunluğu — çarpan değil, lux.
	// Haritadaki güneşin kendi değeri 10; artık modlar o değeri ölçeklemek yerine
	// doğrudan aşağıdaki hedefe gidiyor. Hayalet'in hedefi `HayaletGunesYogunlugu`
	// alanından okunur (varsayılan 2).
	constexpr float GunesYogunlukAgresif = 0.0f;
	constexpr float GunesYogunlukTakipci = 1.0f;
}

AModAtmosferi::AModAtmosferi()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;   // kamera güncellendikten sonra
}

void AModAtmosferi::BeginPlay()
{
	Super::BeginPlay();

	UWorld* Dunya = GetWorld();
	if (!Dunya)
	{
		return;
	}

	// --- 1) Güneşi ve kırmızı ışıkları topla --------------------------------
	for (TActorIterator<ALight> It(Dunya); It; ++It)
	{
		ALight* IsikAktoru = *It;
		if (!IsikAktoru)
		{
			continue;
		}

		ULightComponent* Bilesen = IsikAktoru->GetLightComponent();
		if (!Bilesen)
		{
			continue;
		}

		if (IsikAktoru->IsA<ADirectionalLight>())
		{
			Gunes = Bilesen;
			GunesTabanYogunluk = Bilesen->Intensity;
			continue;
		}

		const FColor Renk = Bilesen->LightColor;
		const bool bEtiketli = IsikAktoru->ActorHasTag(TEXT("KirmiziLamba"));
		const bool bRenkKirmizi =
			Renk.R >= KirmiziEnAzR && Renk.G <= KirmiziEnCokGB && Renk.B <= KirmiziEnCokGB;

		if (bEtiketli || bRenkKirmizi)
		{
			FIsikKaydi Kayit;
			Kayit.Isik = Bilesen;
			Kayit.TabanYogunluk = Bilesen->Intensity;
			KirmiziIsiklar.Add(Kayit);
		}
	}

	// Oyun Takipçi yoğunluğundan başlasın; yoksa ilk saniyede güneş taban
	// değerinden (10) hedefe doğru gözle görülür şekilde kısılıyor.
	if (GunesTabanYogunluk > KINDA_SMALL_NUMBER)
	{
		GunesCarpaniSuanki = GunesYogunlukTakipci / GunesTabanYogunluk;
		if (ULightComponent* G = Gunes.Get())
		{
			G->SetIntensity(GunesYogunlukTakipci);
		}
	}

	// --- 2) Kızı ve oyuncunun kamerasını bul --------------------------------
	for (TActorIterator<AActor> It(Dunya); It; ++It)
	{
		AActor* A = *It;
		if (A && A->GetClass()->GetName().StartsWith(TEXT("kızyurume")))
		{
			Kiz = A;
			break;
		}
	}

	if (APawn* Oyuncu = UGameplayStatics::GetPlayerPawn(Dunya, 0))
	{
		if (UCameraComponent* Kamera = Oyuncu->FindComponentByClass<UCameraComponent>())
		{
			OyuncuKamera = Kamera;
			KameraTabanKonum = Kamera->GetRelativeLocation();
		}

		// El feneri: oyuncunun üzerindeki spot ışık (QuantumSpotLight).
		if (USpotLightComponent* Spot = Oyuncu->FindComponentByClass<USpotLightComponent>())
		{
			Fener = Spot;
			FenerTabanYogunluk = Spot->Intensity;
			FenerTabanIcKoni = Spot->InnerConeAngle;
			FenerTabanDisKoni = Spot->OuterConeAngle;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ModAtmosferi: %d kirmizi isik, gunes %s, kiz %s"),
		KirmiziIsiklar.Num(),
		Gunes.IsValid() ? TEXT("bulundu") : TEXT("YOK"),
		Kiz.IsValid() ? TEXT("bulundu") : TEXT("YOK"));
}

FString AModAtmosferi::ModuOku() const
{
	AActor* KizAktoru = Kiz.Get();
	if (!KizAktoru)
	{
		return FString();
	}

	FStrProperty* P = FindFProperty<FStrProperty>(KizAktoru->GetClass(), TEXT("GuncelModAdi"));
	return P ? P->GetPropertyValue_InContainer(KizAktoru) : FString();
}

void AModAtmosferi::KameraOfsetiUygula(const FVector& Ofset)
{
	USceneComponent* Kamera = OyuncuKamera.Get();
	if (!Kamera)
	{
		return;
	}

	if (Ofset.IsNearlyZero())
	{
		if (bKameraOynandi)
		{
			Kamera->SetRelativeLocation(KameraTabanKonum);
			bKameraOynandi = false;
		}
		return;
	}

	Kamera->SetRelativeLocation(KameraTabanKonum + Ofset);
	bKameraOynandi = true;
}

void AModAtmosferi::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Zaman += DeltaSeconds;

	const FString Mod = ModuOku();
	const bool bHayalet = (!Mod.IsEmpty() && Mod.Equals(AdHayalet, ESearchCase::IgnoreCase));
	const bool bAgresif = (!Mod.IsEmpty() && Mod.Equals(AdAgresif, ESearchCase::IgnoreCase));

	// --- Güneş: mutlak yoğunluk hedefi (Agresif 0, Hayalet 2, Takipçi 1) ----
	// Çarpan üzerinden ilerliyoruz ki geçişin yumuşaklığı aynı kalsın; hedef
	// çarpanı mutlak hedefi güneşin taban değerine bölerek buluyoruz.
	const float HedefYogunluk = bAgresif ? GunesYogunlukAgresif
		: (bHayalet ? HayaletGunesYogunlugu : GunesYogunlukTakipci);
	const float HedefCarpan = (GunesTabanYogunluk > KINDA_SMALL_NUMBER)
		? (HedefYogunluk / GunesTabanYogunluk)
		: 0.0f;
	GunesCarpaniSuanki = FMath::FInterpTo(GunesCarpaniSuanki, HedefCarpan, DeltaSeconds, GecisHizi);

	if (ULightComponent* G = Gunes.Get())
	{
		G->SetIntensity(GunesTabanYogunluk * GunesCarpaniSuanki);
	}

	// --- Agresif şiddeti: moda girerken/çıkarken yumuşak ---------------------
	// Aşağıdaki üç etki de bununla ölçeklenir, böylece mod geçişi ani olmaz.
	AgresifSiddet = FMath::FInterpTo(AgresifSiddet, bAgresif ? 1.0f : 0.0f, DeltaSeconds, GecisHizi);

	// --- Kırmızı ışıklar: Agresif'te hızlı yanıp sönsün ---------------------
	// Kare dalga kullanılıyor; sinüs "nabız" gibi yumuşak kalıyordu, istenen sert çakma.
	float KirmiziCarpan = 1.0f;
	if (AgresifSiddet > KINDA_SMALL_NUMBER)
	{
		const float Faz = FMath::Frac(Zaman * AgresifFrekans);
		const float Hedef = (Faz < 0.5f) ? AgresifParlakCarpan : AgresifSonukCarpan;
		KirmiziCarpan = FMath::Lerp(1.0f, Hedef, AgresifSiddet);
	}

	for (const FIsikKaydi& Kayit : KirmiziIsiklar)
	{
		if (ULightComponent* I = Kayit.Isik.Get())
		{
			I->SetIntensity(Kayit.TabanYogunluk * KirmiziCarpan);
		}
	}

	// --- El feneri: Agresif'te biraz kısılsın ve koni biraz daralsın --------
	if (USpotLightComponent* F = Fener.Get())
	{
		const float ParlaklikCarpan = FMath::Lerp(1.0f, AgresifFenerParlaklik, AgresifSiddet);
		const float KoniCarpan = FMath::Lerp(1.0f, AgresifFenerKoni, AgresifSiddet);

		F->SetIntensity(FenerTabanYogunluk * ParlaklikCarpan);
		F->SetInnerConeAngle(FenerTabanIcKoni * KoniCarpan);
		F->SetOuterConeAngle(FenerTabanDisKoni * KoniCarpan);
	}

	// --- Ekran titremesi: yalnızca Agresif ---------------------------------
	// El feneri titremeye katılmaz. Fener zinciri kameranın altında
	// (FlashlightLag -> QuantumFlashlight -> QuantumSpotLight), yani kamerayı
	// oynatınca fener de oynuyordu. Çözüm: kameranın hemen altındaki halkanın
	// dünya konumunu titremeden ÖNCE al, kamerayı oynattıktan sonra geri koy.
	// Böylece spring arm'ın gecikme hesabı da sarsılmamış konumdan yürür ve
	// bir sonraki karede zincir yine sarsılmamış halde başlar.
	const bool bTitremeVar = (AgresifSiddet > KINDA_SMALL_NUMBER && TitremeGenligi > 0.f);

	USceneComponent* FenerKok = nullptr;
	if (USceneComponent* Kam = OyuncuKamera.Get())
	{
		if (bTitremeVar || bKameraOynandi)
		{
			for (USceneComponent* B = Fener.Get(); B; B = B->GetAttachParent())
			{
				if (B->GetAttachParent() == Kam)
				{
					FenerKok = B;
					break;
				}
			}
		}
	}
	const FVector FenerDunyaOnce = FenerKok ? FenerKok->GetComponentLocation() : FVector::ZeroVector;

	if (bTitremeVar)
	{
		// İki farklı frekans, tekrar eden bir ritim duyulmasın diye.
		const float X = FMath::Sin(Zaman * TitremeHizi) * FMath::Sin(Zaman * TitremeHizi * 0.37f);
		const float Y = FMath::Sin(Zaman * TitremeHizi * 1.31f + 1.7f);
		const float Z = FMath::Sin(Zaman * TitremeHizi * 0.83f + 3.1f);
		KameraOfsetiUygula(FVector(X, Y, Z) * TitremeGenligi * AgresifSiddet);
	}
	else
	{
		KameraOfsetiUygula(FVector::ZeroVector);
	}

	if (FenerKok)
	{
		FenerKok->SetWorldLocation(FenerDunyaOnce);
	}
}
