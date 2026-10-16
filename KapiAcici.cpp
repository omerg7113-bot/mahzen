#include "KapiAcici.h"

#include "AIController.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "UObject/UnrealType.h"

namespace
{
	/**
	 * Kat değiştirme dürtüsünün durumu. Üye değişken EKLEMİYORUZ: sınıf
	 * yerleşimi değişirse Live Coding yamalayamıyor, editörü kapatıp tam
	 * derleme gerekiyor. Bileşen başına durum bu dosya-statik haritada.
	 */
	struct FKatDurumu
	{
		double SonDurtu = 0.0;
		double SonKatZamani = 0.0;
		int32 SonKat = -1;
		int32 Durtu = 0;
	};
	TMap<TWeakObjectPtr<const UActorComponent>, FKatDurumu> GKatDurumlari;

	constexpr float GKatSiniri = 550.f;      // bu z'nin ustu = 2. kat
	constexpr float GDurtuBekleme = 45.f;    // ayni katta bu kadar saniye kalirsa
	constexpr float GDurtuAralik = 20.f;     // en sik bu kadar bir dene
	constexpr float GUzakOyuncu = 1500.f;    // oyuncu bundan uzaksa (kovalamayi bozma)

	/** bp_kapı Blueprint'indeki bool değişkenini yansıma ile okur. */
	bool BoolOzellik(const AActor* A, const TCHAR* Ad, bool bVarsayilan = false)
	{
		if (!A)
		{
			return bVarsayilan;
		}
		if (const FBoolProperty* P = FindFProperty<FBoolProperty>(A->GetClass(), Ad))
		{
			return P->GetPropertyValue_InContainer(A);
		}
		return bVarsayilan;
	}

	bool KapiMi(const AActor* A)
	{
		// Sınıf adı "bp_kapı_C" — Türkçe harfi aramamak için önekle bak.
		return A && A->GetClass()->GetName().StartsWith(TEXT("bp_kap"));
	}
}

UKapiAcici::UKapiAcici()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UKapiAcici::BeginPlay()
{
	Super::BeginPlay();

	KapilariTopla();

	if (const AActor* Sahip = GetOwner())
	{
		OlcumKonumu = Sahip->GetActorLocation();
	}

	UE_LOG(LogTemp, Log, TEXT("KapiAcici: %s icin %d kapi izleniyor"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"), Kapilar.Num());
}

void UKapiAcici::KapilariTopla()
{
	Kapilar.Reset();
	UWorld* W = GetWorld();
	if (!W)
	{
		return;
	}
	for (TActorIterator<AActor> It(W); It; ++It)
	{
		if (KapiMi(*It))
		{
			Kapilar.Add(*It);
		}
	}
}

bool UKapiAcici::KapiyiAc(AActor* Kapi)
{
	if (!Kapi)
	{
		return false;
	}
	// Kapı animasyonu, sesi ve navmesh güncellemesi Blueprint tarafında.
	UFunction* Fonk = Kapi->FindFunction(TEXT("KizAc"));
	if (!Fonk)
	{
		return false;
	}
	Kapi->ProcessEvent(Fonk, nullptr);
	++AcilanKapi;
	SonKapi = Kapi->GetName();
	UE_LOG(LogTemp, Log, TEXT("KapiAcici: kapi acildi (%s), toplam %d"), *SonKapi, AcilanKapi);
	return true;
}

void UKapiAcici::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Sahip = GetOwner();
	if (!Sahip)
	{
		return;
	}

	Sayac += DeltaTime;
	if (Sayac < Aralik)
	{
		return;
	}
	const float Gecen = Sayac;
	Sayac = 0.f;

	const FVector Konum = Sahip->GetActorLocation();

	// --- takılma takibi: bir süre boyunca ilerlemediyse yarıçapı genişlet ----
	if (FVector::Dist2D(Konum, OlcumKonumu) < TakilmaEsigi)
	{
		DurgunlukSuresi += Gecen;
		if (!bTakildi && DurgunlukSuresi >= TakilmaSuresi)
		{
			bTakildi = true;
			++TakilmaSayisi;
		}
	}
	else
	{
		OlcumKonumu = Konum;
		DurgunlukSuresi = 0.f;
		bTakildi = false;
	}

	// --- kat değiştirme dürtüsü ---------------------------------------------
	// Gezinti mantığı rastgele nokta örnekliyor; 2. kat noktaları nadiren
	// denk geliyordu (üstüne çatının navmesh'i örneklemeleri kirletiyordu) ve
	// kız saatlerce aynı katta kalıyordu. Oyuncu uzakta ve peşinde ses tetiği
	// yokken, bir süredir kat değiştirmediyse DİĞER kattan ulaşılabilir bir
	// noktaya hedef veriyoruz; merdiveni kendisi kullanıyor.
	{
		UWorld* W = GetWorld();
		const double Simdi = W ? W->GetTimeSeconds() : 0.0;
		FKatDurumu& KD = GKatDurumlari.FindOrAdd(this);
		const int32 Kat = (Konum.Z > GKatSiniri) ? 1 : 0;
		if (KD.SonKat != Kat)
		{
			KD.SonKat = Kat;
			KD.SonKatZamani = Simdi;
		}

		const bool bTetikVar = BoolOzellik(Sahip, TEXT("TetikVar"));
		const APawn* Oyuncu = W ? UGameplayStatics::GetPlayerPawn(W, 0) : nullptr;
		const float OyuncuMesafe = Oyuncu ? FVector::Dist(Oyuncu->GetActorLocation(), Konum) : 0.f;

		if (!bTetikVar && OyuncuMesafe > GUzakOyuncu
			&& (Simdi - KD.SonKatZamani) > GDurtuBekleme
			&& (Simdi - KD.SonDurtu) > GDurtuAralik)
		{
			KD.SonDurtu = Simdi;
			if (APawn* SahipPawn2 = Cast<APawn>(Sahip))
			{
				if (AAIController* AI2 = Cast<AAIController>(SahipPawn2->GetController()))
				{
					UNavigationSystemV1* Nav2 = UNavigationSystemV1::GetCurrent(W);
					FNavLocation Hedef2;
					for (int32 Deneme = 0; Deneme < 24 && Nav2; ++Deneme)
					{
						if (!Nav2->GetRandomReachablePointInRadius(Konum, 6000.f, Hedef2))
						{
							continue;
						}
						const int32 HedefKat = (Hedef2.Location.Z > GKatSiniri) ? 1 : 0;
						if (HedefKat != Kat)
						{
							AI2->StopMovement();
							AI2->MoveToLocation(Hedef2.Location, 60.f, true, true, false, true, nullptr, true);
							++KD.Durtu;
							UE_LOG(LogTemp, Log,
								TEXT("KapiAcici: kat durtusu #%d — kat %d -> %d, hedef (%.0f, %.0f, %.0f)"),
								KD.Durtu, Kat + 1, HedefKat + 1,
								Hedef2.Location.X, Hedef2.Location.Y, Hedef2.Location.Z);
							break;
						}
					}
				}
			}
		}
	}

	const float Menzil = bTakildi ? TakilmaYaricapi : Yaricap;

	// --- en yakın uygun kapıyı aç -------------------------------------------
	AActor* EnYakin = nullptr;
	float EnYakinMesafe = TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<AActor>& Zayif : Kapilar)
	{
		AActor* Kapi = Zayif.Get();
		if (!Kapi)
		{
			continue;
		}
		const FVector KapiKonum = Kapi->GetActorLocation();
		if (FMath::Abs(KapiKonum.Z - Konum.Z) > KatToleransi)
		{
			continue;   // başka kat
		}
		const float Mesafe = FVector::Dist2D(KapiKonum, Konum);
		if (Mesafe > Menzil || Mesafe >= EnYakinMesafe)
		{
			continue;
		}
		// Oyuncu kilidini açmış olmalı, mühürlü olmamalı, kapalı olmalı.
		if (!BoolOzellik(Kapi, TEXT("bKilidiAcildi")) ||
			BoolOzellik(Kapi, TEXT("bMuhurlenmis")) ||
			BoolOzellik(Kapi, TEXT("bAcik")))
		{
			continue;
		}
		EnYakin = Kapi;
		EnYakinMesafe = Mesafe;
	}

	if (EnYakin && KapiyiAc(EnYakin))
	{
		// Kapı açıldı: takılma durumunu sıfırla, bir sonraki ölçüm temiz başlasın.
		OlcumKonumu = Konum;
		DurgunlukSuresi = 0.f;
		bTakildi = false;
		return;
	}

	// --- kurtarma: açılacak kapı yok ve hâlâ ilerlemiyorsa ------------------
	if (bKurtarmaAcik && DurgunlukSuresi >= KurtarmaSuresi)
	{
		// 1) BP'nin gezinti çıpasını sıfırla (Hayalet/gezinti modunda işe yarar).
		if (FIntProperty* GezintiOzellik = FindFProperty<FIntProperty>(Sahip->GetClass(), TEXT("GezintiSayaci")))
		{
			// BP kuralı: GezintiSayaci >= 3 olunca yeni gezinti çıpası seçilir.
			GezintiOzellik->SetPropertyValue_InContainer(Sahip, 99);
		}

		// 2) MODDAN BAĞIMSIZ gerçek kurtarma: AIController'a doğrudan yeni hedef
		//    ver. Kovalama modlarında (Agresif/Takipçi) BP'nin gezinti mantığı hiç
		//    çalışmadığı için yalnızca çıpa sıfırlamak etkisizdi — PIE'de 37
		//    saniyelik duruşlar böyle oluşuyordu (merdiven sahanlığı).
		FString Nereye = TEXT("hedef bulunamadi");
		if (APawn* SahipPawn = Cast<APawn>(Sahip))
		{
			if (AAIController* AI = Cast<AAIController>(SahipPawn->GetController()))
			{
				UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
				FNavLocation Hedef;
				bool bBulundu = false;
				const float Yaricaplar[3] = { 700.f, 1600.f, 3200.f };
				for (float R : Yaricaplar)
				{
					if (Nav && Nav->GetRandomReachablePointInRadius(Konum, R, Hedef)
						&& FVector::Dist2D(Hedef.Location, Konum) > 250.f)
					{
						bBulundu = true;
						break;
					}
				}
				AI->StopMovement();
				if (bBulundu)
				{
					AI->MoveToLocation(Hedef.Location, 50.f, true, true, false, true, nullptr, true);
					Nereye = FString::Printf(TEXT("(%.0f, %.0f) %.0f birim"),
						Hedef.Location.X, Hedef.Location.Y, FVector::Dist2D(Hedef.Location, Konum));
				}
			}
		}

		++KurtarmaSayisi;
		UE_LOG(LogTemp, Log, TEXT("KapiAcici: kurtarma #%d — %.0f s durgun (%.0f, %.0f, %.0f) -> %s"),
			KurtarmaSayisi, DurgunlukSuresi, Konum.X, Konum.Y, Konum.Z, *Nereye);

		OlcumKonumu = Konum;
		DurgunlukSuresi = 0.f;
		bTakildi = false;
	}
}
