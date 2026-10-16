#include "AnahtarDagitici.h"

#include "Engine/GameInstance.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "Math/RandomStream.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr int32 ODA_N = (int32)EMahzenOda::ODA_SAYISI;

	/**
	 * Grup numarası sırası (101..110 ve 111..120). Kasa anahtarı adlarından
	 * okundu: 101=Depo, 102=Kütüphane, 103=Lavabo, 104=Merdiven, 105=Mutfak,
	 * 106=Ofis, 107=Salon, 108=Tapınak, 109=Yatak Odası, 110=Çocuk Odası.
	 */
	const EMahzenOda GrupSirasi[10] = {
		EMahzenOda::Depo,
		EMahzenOda::Kutuphane,
		EMahzenOda::Lavabo,
		EMahzenOda::Merdiven,
		EMahzenOda::Mutfak,
		EMahzenOda::Ofis,
		EMahzenOda::Salon,
		EMahzenOda::Tapinak,
		EMahzenOda::YatakOdasi,
		EMahzenOda::CocukOdasi
	};

	int32 OdaGrupOfseti(EMahzenOda Oda)
	{
		for (int32 i = 0; i < 10; ++i)
		{
			if (GrupSirasi[i] == Oda) { return i; }
		}
		return INDEX_NONE;
	}

	struct FAnahtarKaydi
	{
		TWeakObjectPtr<AActor> Aktor;
		int32 Kat = 0;              // 0 = 1. kat, 1 = 2. kat
		EMahzenOda Oda = EMahzenOda::Koridor;
		FString Ad;
	};
}

AAnahtarDagitici::AAnahtarDagitici()
{
	PrimaryActorTick.bCanEverTick = false;
}

const TCHAR* AAnahtarDagitici::OdaAnahtarParcasi(EMahzenOda Oda)
{
	switch (Oda)
	{
	case EMahzenOda::CocukOdasi: return TEXT("çocuk odası");   // çocuk odası
	case EMahzenOda::YatakOdasi: return TEXT("yatak odası");        // yatak odası
	case EMahzenOda::Ofis:       return TEXT("ofis");
	case EMahzenOda::Lavabo:     return TEXT("lavabo");
	case EMahzenOda::Merdiven:   return TEXT("merdiven");
	case EMahzenOda::Mutfak:     return TEXT("mutfak");
	case EMahzenOda::Tapinak:    return TEXT("tapınak");            // tapınak
	case EMahzenOda::Kutuphane:  return TEXT("kütüphane");     // kütüphane
	case EMahzenOda::Salon:      return TEXT("salon");
	case EMahzenOda::Depo:       return TEXT("depo");
	default:                     return TEXT("");
	}
}

bool AAnahtarDagitici::AdiCozumle(const FString& Ad, int32& OutKat, EMahzenOda& OutOda)
{
	const FString Kucuk = Ad.ToLower();

	// Kasa anahtarları bu dağıtımın dışında: sistemleri aynı kalacak.
	if (Kucuk.Contains(TEXT("kasa")))
	{
		return false;
	}
	// Başlangıç ve koridor anahtarları kat öneki taşımaz, serbest alanda kalır.
	if (!Kucuk.StartsWith(TEXT("1. kat")) && !Kucuk.StartsWith(TEXT("2. kat")))
	{
		return false;
	}

	OutKat = Kucuk.StartsWith(TEXT("1. kat")) ? 0 : 1;

	// "yatak odası" ve "çocuk odası" önce denenmeli: "odası" ortak parçaları var.
	const EMahzenOda Sira[10] = {
		EMahzenOda::CocukOdasi, EMahzenOda::YatakOdasi, EMahzenOda::Kutuphane,
		EMahzenOda::Merdiven, EMahzenOda::Lavabo, EMahzenOda::Mutfak,
		EMahzenOda::Tapinak, EMahzenOda::Salon, EMahzenOda::Depo, EMahzenOda::Ofis
	};
	for (EMahzenOda Oda : Sira)
	{
		if (Kucuk.Contains(OdaAnahtarParcasi(Oda)))
		{
			OutOda = Oda;
			return true;
		}
	}
	return false;
}

void AAnahtarDagitici::ErisimHesapla(const bool bAnahtarVar[2][ODA_N], bool bOdaErisilir[2][ODA_N])
{
	for (int32 f = 0; f < 2; ++f)
	{
		for (int32 o = 0; o < ODA_N; ++o)
		{
			bOdaErisilir[f][o] = false;
		}
	}

	auto K = [&](int32 f, EMahzenOda o) { return bAnahtarVar[f][(int32)o]; };
	auto R = [&](int32 f, EMahzenOda o) -> bool& { return bOdaErisilir[f][(int32)o]; };

	// Sabit nokta: karşılıklı bağımlılık var (Mutfak <-> Tapınak <-> Kütüphane).
	for (int32 tur = 0; tur < 8; ++tur)
	{
		// 1. kat koridoru her zaman açık (oyun orada başlıyor).
		R(0, EMahzenOda::Koridor) = true;

		// 2. kat koridoruna çıkmak: 1. kat merdiven odasına gir, yukarı çık,
		// 2. kat merdiven odasından koridora ÇIK -> iki merdiven anahtarı da gerek.
		R(1, EMahzenOda::Koridor) = K(0, EMahzenOda::Merdiven) && K(1, EMahzenOda::Merdiven);

		for (int32 f = 0; f < 2; ++f)
		{
			const bool bKor = R(f, EMahzenOda::Koridor);

			// Yalnızca koridora açılanlar.
			R(f, EMahzenOda::CocukOdasi) = bKor && K(f, EMahzenOda::CocukOdasi);
			R(f, EMahzenOda::YatakOdasi) = bKor && K(f, EMahzenOda::YatakOdasi);
			R(f, EMahzenOda::Ofis)       = bKor && K(f, EMahzenOda::Ofis);
			R(f, EMahzenOda::Lavabo)     = bKor && K(f, EMahzenOda::Lavabo);
			R(f, EMahzenOda::Merdiven)   = bKor && K(f, EMahzenOda::Merdiven);
			R(f, EMahzenOda::Salon)      = bKor && K(f, EMahzenOda::Salon);

			// Depo yalnızca salondan; aradaki kapı depo anahtarıyla açılıyor.
			R(f, EMahzenOda::Depo) = R(f, EMahzenOda::Salon) && K(f, EMahzenOda::Depo);

			// Mutfak: koridordan ya da tapınaktan, ikisinde de mutfak anahtarı.
			R(f, EMahzenOda::Mutfak) =
				(bKor || R(f, EMahzenOda::Tapinak)) && K(f, EMahzenOda::Mutfak);

			// Tapınak: koridordan (tapınak), mutfaktan (mutfak) ya da
			// kütüphaneden (kütüphane).
			R(f, EMahzenOda::Tapinak) =
				(bKor && K(f, EMahzenOda::Tapinak)) ||
				(R(f, EMahzenOda::Mutfak) && K(f, EMahzenOda::Mutfak)) ||
				(R(f, EMahzenOda::Kutuphane) && K(f, EMahzenOda::Kutuphane));

			// Kütüphanenin tek kapısı tapınağa açılıyor.
			R(f, EMahzenOda::Kutuphane) = R(f, EMahzenOda::Tapinak) && K(f, EMahzenOda::Kutuphane);
		}
	}
}

void AAnahtarDagitici::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// GameInstance seviye değişiminde ölmüyor: ölüp yeniden başlayan oyuncu
	// bir önceki denemenin mühür sayısıyla devam ediyordu. Her seviye açılışında
	// sıfırla. (Kazanma kararı artık UMahzenHarita'da, ama HUD bunu okuyor.)
	if (UGameInstance* GI = GetGameInstance())
	{
		if (FIntProperty* Sayac = FindFProperty<FIntProperty>(GI->GetClass(), TEXT("MuhurlenmisOdaSayisi")))
		{
			Sayac->SetPropertyValue_InContainer(GI, 0);
		}
	}

	// BURADA çalışmak kritik: Unreal önce tüm aktörlerin
	// PostInitializeComponents'ini çağırıyor, BeginPlay'leri sonra. Yani
	// bm_key kendi ışınlanmasını yapmadan KatmanNo'sunu yazmış oluyoruz.
	if (bEtkin)
	{
		Dagit();
	}
}

void AAnahtarDagitici::Dagit()
{
	UWorld* Dunya = GetWorld();
	if (!Dunya)
	{
		return;
	}

	// --- 1) Anahtarları topla -------------------------------------------------
	TArray<FAnahtarKaydi> Anahtarlar;

	for (TActorIterator<AActor> It(Dunya); It; ++It)
	{
		AActor* A = *It;
		if (!A || !A->GetClass()->GetName().StartsWith(TEXT("bm_key")))
		{
			continue;
		}

		FStrProperty* P = FindFProperty<FStrProperty>(A->GetClass(), TEXT("anahtarısmı"));
		FIntProperty* KP = FindFProperty<FIntProperty>(A->GetClass(), TEXT("KatmanNo"));
		if (!P || !KP)
		{
			continue;
		}

		const FString Ad = P->GetPropertyValue_InContainer(A);
		FAnahtarKaydi Kayit;
		if (!AdiCozumle(Ad, Kayit.Kat, Kayit.Oda))
		{
			continue;   // kasa / başlangıç / koridor anahtarı: dokunma
		}
		Kayit.Aktor = A;
		Kayit.Ad = Ad;
		Anahtarlar.Add(Kayit);
	}

	if (Anahtarlar.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AnahtarDagitici: dagitilacak oda anahtari bulunamadi"));
		return;
	}

	// --- 2) Grup kapasitelerini say ------------------------------------------
	TMap<int32, int32> GrupKapasite;
	for (TActorIterator<ATargetPoint> It(Dunya); It; ++It)
	{
		for (const FName& Etiket : It->Tags)
		{
			const FString S = Etiket.ToString();
			if (S.StartsWith(TEXT("AnahtarKatman|")))
			{
				const int32 No = FCString::Atoi(*S.RightChop(14));
				GrupKapasite.FindOrAdd(No)++;
			}
		}
	}

	TMap<int32, int32> GrupKullanim;   // bu dağıtımda kaç oda anahtarı koyduk

	// --- 3) Yerleştirme: ileri erişilebilirlik -------------------------------
	FRandomStream Rastgele(Tohum != 0 ? Tohum : FMath::Rand());

	bool bAnahtarVar[2][ODA_N] = {};
	bool bOdaErisilir[2][ODA_N] = {};

	TArray<int32> Kalan;
	for (int32 i = 0; i < Anahtarlar.Num(); ++i) { Kalan.Add(i); }

	struct FSonuc { int32 Grup; FString Nereye; };
	TArray<FSonuc> Sonuclar;
	Sonuclar.SetNum(Anahtarlar.Num());

	while (Kalan.Num() > 0)
	{
		ErisimHesapla(bAnahtarVar, bOdaErisilir);

		// Şu an ulaşılabilen bölgeler: serbest alan + erişilebilir odalar.
		struct FBolge { int32 Grup; int32 Kat; FString Ad; };
		TArray<FBolge> Havuz;

		auto GrupUygun = [&](int32 Grup) -> bool
		{
			const int32* Kap = GrupKapasite.Find(Grup);
			if (!Kap) { return false; }
			const int32 Kullanilan = GrupKullanim.FindRef(Grup);
			return (*Kap - Kullanilan) > GrupYedekPayi;
		};

		if (GrupUygun(SerbestKatman))
		{
			Havuz.Add({ SerbestKatman, 0, TEXT("Giriş/Koridor") });
		}
		for (int32 f = 0; f < 2; ++f)
		{
			for (int32 i = 0; i < 10; ++i)
			{
				const EMahzenOda Oda = GrupSirasi[i];
				if (!bOdaErisilir[f][(int32)Oda]) { continue; }
				const int32 Grup = (f == 0 ? Kat1GrupBaslangic : Kat2GrupBaslangic) + i;
				if (GrupUygun(Grup))
				{
					Havuz.Add({ Grup, f, FString::Printf(TEXT("%d. Kat %s"), f + 1, OdaAnahtarParcasi(Oda)) });
				}
			}
		}

		if (Havuz.Num() == 0)
		{
			// Olmaması gereken durum: serbest alan her zaman erişilebilir.
			UE_LOG(LogTemp, Error, TEXT("AnahtarDagitici: yerlestirilecek bolge kalmadi (%d anahtar acikta)"), Kalan.Num());
			break;
		}

		// Anahtarları karıştırıp ilk YERLEŞTİRİLEBİLENİ seç: kat kısıtı olan
		// anahtar (1. kat merdiven) uygun bölge bulamazsa sıradakine geç.
		const int32 Basla = Rastgele.RandRange(0, Kalan.Num() - 1);
		int32 SecilenKalanIdx = INDEX_NONE;
		int32 SecilenBolge = INDEX_NONE;

		for (int32 k = 0; k < Kalan.Num(); ++k)
		{
			const int32 KalanIdx = (Basla + k) % Kalan.Num();
			const FAnahtarKaydi& Kayit = Anahtarlar[Kalan[KalanIdx]];

			// 1. kat merdiven anahtarı 1. KATTA kalmalı: yoksa yukarı çıkılamaz.
			const bool bKat1Zorunlu = (Kayit.Kat == 0 && Kayit.Oda == EMahzenOda::Merdiven);

			TArray<int32> Uygun;
			for (int32 b = 0; b < Havuz.Num(); ++b)
			{
				if (bKat1Zorunlu && Havuz[b].Kat != 0) { continue; }
				Uygun.Add(b);
			}
			if (Uygun.Num() > 0)
			{
				SecilenKalanIdx = KalanIdx;
				SecilenBolge = Uygun[Rastgele.RandRange(0, Uygun.Num() - 1)];
				break;
			}
		}

		if (SecilenKalanIdx == INDEX_NONE)
		{
			UE_LOG(LogTemp, Error, TEXT("AnahtarDagitici: kat kisiti yuzunden yerlestirilemedi"));
			break;
		}

		const int32 AnahtarIdx = Kalan[SecilenKalanIdx];
		const FBolge& B = Havuz[SecilenBolge];

		Sonuclar[AnahtarIdx] = { B.Grup, B.Ad };
		GrupKullanim.FindOrAdd(B.Grup)++;

		const FAnahtarKaydi& Kayit = Anahtarlar[AnahtarIdx];
		bAnahtarVar[Kayit.Kat][(int32)Kayit.Oda] = true;   // artık toplanabilir

		Kalan.RemoveAt(SecilenKalanIdx);
	}

	// --- 4) KatmanNo'ları yaz -------------------------------------------------
	int32 Yazilan = 0;
	for (int32 i = 0; i < Anahtarlar.Num(); ++i)
	{
		AActor* A = Anahtarlar[i].Aktor.Get();
		if (!A || Sonuclar[i].Grup == 0) { continue; }
		FIntProperty* KP = FindFProperty<FIntProperty>(A->GetClass(), TEXT("KatmanNo"));
		if (!KP) { continue; }
		KP->SetPropertyValue_InContainer(A, Sonuclar[i].Grup);
		++Yazilan;

		if (bLogla)
		{
			UE_LOG(LogTemp, Log, TEXT("AnahtarDagitici: %-30s -> %s (grup %d)"),
				*Anahtarlar[i].Ad, *Sonuclar[i].Nereye, Sonuclar[i].Grup);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AnahtarDagitici: %d/%d oda anahtari dagitildi"), Yazilan, Anahtarlar.Num());
}
