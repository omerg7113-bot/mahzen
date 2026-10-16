#include "MahzenHarita.h"

#include "Engine/Engine.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "KapiAcici.h"
#include "MyCharacter.h"
#include "OyunDurumu.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
	/** AnahtarDagitici ile aynı sıra: grup 101..110 (1. kat), 111..120 (2. kat). */
	const TCHAR* GOdaAdlari[10] = {
		TEXT("Depo"), TEXT("Kütüphane"), TEXT("Lavabo"), TEXT("Merdiven"), TEXT("Mutfak"),
		TEXT("Ofis"), TEXT("Salon"), TEXT("Tapınak"), TEXT("Yatak Odası"), TEXT("Çocuk Odası")
	};

	bool KapiMi(const AActor* A)
	{
		return A && A->GetClass()->GetName().StartsWith(TEXT("bp_kap"));
	}

	bool EsyaMi(const AActor* A)
	{
		if (!A)
		{
			return false;
		}
		const FString S = A->GetClass()->GetName();
		return S.StartsWith(TEXT("bm_key")) || S.StartsWith(TEXT("neckale"));
	}

	FString StrOzellik(const AActor* A, const TCHAR* Ad)
	{
		if (!A)
		{
			return FString();
		}
		if (FStrProperty* P = FindFProperty<FStrProperty>(A->GetClass(), Ad))
		{
			return P->GetPropertyValue_InContainer(A);
		}
		return FString();
	}

	bool BoolOzellik(const AActor* A, const TCHAR* Ad, bool Varsayilan = false)
	{
		if (!A)
		{
			return Varsayilan;
		}
		if (FBoolProperty* P = FindFProperty<FBoolProperty>(A->GetClass(), Ad))
		{
			return P->GetPropertyValue_InContainer(A);
		}
		return Varsayilan;
	}

	/** Karşılaştırma için: baştaki/sondaki boşluklar atılır, küçük harfe inilir. */
	FString Sadelestir(const FString& Metin)
	{
		return Metin.TrimStartAndEnd().ToLower();
	}

	FString ListeyeCevir(const TArray<FString>& Ogeler)
	{
		FString Sonuc;
		for (int32 i = 0; i < Ogeler.Num(); ++i)
		{
			if (i > 0)
			{
				Sonuc += (i == Ogeler.Num() - 1) ? TEXT(" ve ") : TEXT(", ");
			}
			Sonuc += FString::Printf(TEXT("\"%s\""), *Ogeler[i]);
		}
		return Sonuc;
	}

	/** Oyuncunun envanteri (anahtarcantası dizisi), sadeleştirilmiş adlarla. */
	void EnvanteriOku(APawn* Oyuncu, TSet<FString>& Out)
	{
		if (!Oyuncu)
		{
			return;
		}
		FArrayProperty* Dizi = FindFProperty<FArrayProperty>(Oyuncu->GetClass(), TEXT("anahtarcantası"));
		FStrProperty* Ic = Dizi ? CastField<FStrProperty>(Dizi->Inner) : nullptr;
		if (!Dizi || !Ic)
		{
			return;
		}
		FScriptArrayHelper Yardimci(Dizi, Dizi->ContainerPtrToValuePtr<void>(Oyuncu));
		for (int32 i = 0; i < Yardimci.Num(); ++i)
		{
			Out.Add(Sadelestir(Ic->GetPropertyValue(Yardimci.GetRawPtr(i))));
		}
	}
}

// ===================== kurulum =====================

void UMahzenHaritaAltSistemi::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	// 1 saniye gecikme: bütün aktörler yerleşsin, ama oyuncu daha hiçbir kapıyı
	// açmamış olsun — tarama kapılar KAPALIYKEN yapılmalı.
	InWorld.GetTimerManager().SetTimer(
		KurulumZamanlayici, this, &UMahzenHaritaAltSistemi::BaslangicKurulumu, 1.0f, false);
}

void UMahzenHaritaAltSistemi::BaslangicKurulumu()
{
	Kur();

	UWorld* W = GetWorld();
	if (!W)
	{
		return;
	}

	// Ana menüden "DEVAM ET" ile girildiyse kaydı şimdi uygula: anahtar
	// dağıtımı (PostInitializeComponents) ve bm_key'lerin kendi ışınlanması
	// (BeginPlay) bitmiş oluyor, yani üzerine yazmak güvenli.
	if (UOyunDurumu::BeklemedeMi())
	{
		UOyunDurumu::Uygula(W);
	}

	// --- kıza kapı açma yeteneği tak ----------------------------------------
	// bp_kapı'yı yalnızca oyuncu açabiliyor (KapiyiKullan ilk düğümde
	// BP_FirstPersonCharacter'a cast ediyor). Kız kapalı kapıya dayanıp
	// kalıyordu; bu bileşen oyuncunun kilidini açtığı kapıları ona açtırıyor.
	int32 Takilan = 0;
	for (TActorIterator<AMyCharacter> It(W); It; ++It)
	{
		AMyCharacter* Kiz = *It;
		if (!Kiz || Kiz->FindComponentByClass<UKapiAcici>())
		{
			continue;
		}
		UKapiAcici* Acici = NewObject<UKapiAcici>(Kiz, UKapiAcici::StaticClass(), TEXT("KapiAcici"));
		if (Acici)
		{
			Acici->RegisterComponent();
			Kiz->AddInstanceComponent(Acici);
			++Takilan;
		}
	}
	if (Takilan > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("MahzenHarita: %d karaktere KapiAcici takildi"), Takilan);
	}

	// --- oyun daha başlamadan bitirilemez mi? -------------------------------
	// Anahtar dağıtımı bazı tohumlarda çözülemez çıkıyor. Bunu burada bir kez
	// ölçüp işaretliyoruz; sonra ilk mühürde sahte "çıkmaza girdin" basmıyoruz.
	EOyunSonuTuru Tur = EOyunSonuTuru::Cikmaz;
	FString Sebep;
	if (KilitlendiMi(Tur, Sebep) && Tur == EOyunSonuTuru::Cikmaz)
	{
		bBaslangictaKilitli = true;
		UE_LOG(LogTemp, Error,
			TEXT("MahzenHarita: BU TOHUM BASLANGICTA BITIRILEMEZ (anahtar dagitimi). Cikmaz ekrani bastirilacak.\n%s"),
			*Sebep);
	}
}

void UMahzenHaritaAltSistemi::Kur()
{
	if (bHazir)
	{
		return;
	}

	UWorld* W = GetWorld();
	if (!W || !W->IsGameWorld())
	{
		return;
	}

	const double T0 = FPlatformTime::Seconds();

	// --- 1) kapıları ve hedef noktalarını topla ------------------------------
	TArray<AActor*> Kapilar;
	TArray<TPair<int32, FVector>> Noktalar;

	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* A = *It;
		if (!A)
		{
			continue;
		}
		if (KapiMi(A))
		{
			Kapilar.Add(A);
			continue;
		}
		if (const ATargetPoint* TP = Cast<ATargetPoint>(A))
		{
			const FVector L = TP->GetActorLocation();
			if (FMath::Abs(L.Z) > 10000.0f)
			{
				continue;   // bm_key tarafından tüketilip uzağa taşınmış nokta
			}
			for (const FName& Etiket : TP->Tags)
			{
				const FString S = Etiket.ToString();
				if (S.StartsWith(TEXT("AnahtarKatman|")))
				{
					const int32 G = FCString::Atoi(*S.RightChop(14));
					if (G == 1 || (G >= 101 && G <= 120))
					{
						Noktalar.Add(TPair<int32, FVector>(G, L));
					}
				}
			}
		}
	}

	if (Kapilar.Num() == 0)
	{
		return;   // ana menü haritası gibi kapısız seviyelerde iş yok
	}

	// --- 2) sınırlar ve katlar ----------------------------------------------
	FVector2D EnAz(FLT_MAX, FLT_MAX);
	FVector2D EnCok(-FLT_MAX, -FLT_MAX);
	TArray<float> Zler;

	auto SinirEkle = [&EnAz, &EnCok](const FVector& L)
	{
		EnAz.X = FMath::Min(EnAz.X, L.X);
		EnAz.Y = FMath::Min(EnAz.Y, L.Y);
		EnCok.X = FMath::Max(EnCok.X, L.X);
		EnCok.Y = FMath::Max(EnCok.Y, L.Y);
	};

	for (const AActor* A : Kapilar)
	{
		const FVector L = A->GetActorLocation();
		SinirEkle(L);
		Zler.Add(L.Z);
	}
	for (const TPair<int32, FVector>& N : Noktalar)
	{
		SinirEkle(N.Value);
	}

	EnAz -= FVector2D(400.0f, 400.0f);
	EnCok += FVector2D(400.0f, 400.0f);

	Zler.Sort();
	KatZ.Reset();
	{
		float Toplam = Zler[0];
		int32 Adet = 1;
		for (int32 i = 1; i <= Zler.Num(); ++i)
		{
			const bool bYeniKume = (i == Zler.Num()) || (Zler[i] - Zler[i - 1] > 150.0f);
			if (bYeniKume)
			{
				KatZ.Add(Toplam / Adet);
				if (i < Zler.Num())
				{
					Toplam = Zler[i];
					Adet = 1;
				}
			}
			else
			{
				Toplam += Zler[i];
				++Adet;
			}
		}
	}

	Kose = EnAz;
	NX = FMath::CeilToInt((EnCok.X - EnAz.X) / Adim) + 1;
	NY = FMath::CeilToInt((EnCok.Y - EnAz.Y) / Adim) + 1;
	const int32 NK = KatZ.Num();

	// --- 3) hücre doluluk testi ---------------------------------------------
	FCollisionQueryParams Sorgu(SCENE_QUERY_STAT(MahzenIzgara), false);
	const FCollisionShape HucreKuresi = FCollisionShape::MakeSphere(28.0f);
	const FCollisionShape KenarKuresi = FCollisionShape::MakeSphere(26.0f);
	const FName PawnProfili(TEXT("Pawn"));

	const int32 ToplamHucre = NK * NX * NY;
	TArray<uint8> Bos;
	Bos.Init(0, ToplamHucre);

	auto Dizin = [this](int32 K, int32 I, int32 J) { return (K * NX + I) * NY + J; };
	auto Merkez = [this](int32 K, int32 I, int32 J)
	{
		return FVector(Kose.X + I * Adim, Kose.Y + J * Adim, KatZ[K] + ProbeYukseklik);
	};

	for (int32 K = 0; K < NK; ++K)
	{
		for (int32 I = 0; I < NX; ++I)
		{
			for (int32 J = 0; J < NY; ++J)
			{
				const FVector P = Merkez(K, I, J);
				const bool bDolu = W->OverlapBlockingTestByProfile(
					P, FQuat::Identity, PawnProfili, HucreKuresi, Sorgu);
				Bos[Dizin(K, I, J)] = bDolu ? 0 : 1;
			}
		}
	}

	// --- 4) bağlı bileşenler (bölgeler) --------------------------------------
	Hucreler.Init(INDEX_NONE, ToplamHucre);
	TArray<int32> BolgeKat;
	TArray<int32> Yigin;

	auto KomsuGecilir = [&](int32 K, int32 I1, int32 J1, int32 I2, int32 J2)
	{
		if (I2 < 0 || J2 < 0 || I2 >= NX || J2 >= NY)
		{
			return false;
		}
		if (!Bos[Dizin(K, I2, J2)])
		{
			return false;
		}
		return !W->SweepTestByProfile(Merkez(K, I1, J1), Merkez(K, I2, J2),
			FQuat::Identity, PawnProfili, KenarKuresi, Sorgu);
	};

	int32 BolgeSayisi = 0;
	for (int32 K = 0; K < NK; ++K)
	{
		for (int32 I = 0; I < NX; ++I)
		{
			for (int32 J = 0; J < NY; ++J)
			{
				const int32 D = Dizin(K, I, J);
				if (!Bos[D] || Hucreler[D] != INDEX_NONE)
				{
					continue;
				}

				const int32 Bolge = BolgeSayisi++;
				BolgeKat.Add(K);
				Hucreler[D] = Bolge;
				Yigin.Reset();
				Yigin.Add(D);

				while (Yigin.Num() > 0)
				{
					const int32 Su = Yigin.Pop(EAllowShrinking::No);
					const int32 SuJ = Su % NY;
					const int32 SuI = (Su / NY) % NX;

					const int32 DI[4] = { 1, -1, 0, 0 };
					const int32 DJ[4] = { 0, 0, 1, -1 };
					for (int32 n = 0; n < 4; ++n)
					{
						const int32 I2 = SuI + DI[n];
						const int32 J2 = SuJ + DJ[n];
						if (!KomsuGecilir(K, SuI, SuJ, I2, J2))
						{
							continue;
						}
						const int32 D2 = Dizin(K, I2, J2);
						if (Hucreler[D2] == INDEX_NONE)
						{
							Hucreler[D2] = Bolge;
							Yigin.Add(D2);
						}
					}
				}
			}
		}
	}

	// --- 5) bölge adları ------------------------------------------------------
	BolgeAdlari.Reset();
	BolgeAdlari.SetNum(BolgeSayisi);

	TMap<int32, TMap<int32, int32>> Oylar;   // bölge -> (grup -> adet)
	for (const TPair<int32, FVector>& N : Noktalar)
	{
		const int32 B = EnYakinBolge(N.Value, 3);
		if (B != INDEX_NONE)
		{
			Oylar.FindOrAdd(B).FindOrAdd(N.Key)++;
		}
	}

	for (int32 B = 0; B < BolgeSayisi; ++B)
	{
		int32 EnIyiGrup = INDEX_NONE;
		int32 EnIyiAdet = 0;
		if (const TMap<int32, int32>* Kutu = Oylar.Find(B))
		{
			for (const TPair<int32, int32>& O : *Kutu)
			{
				if (O.Value > EnIyiAdet)
				{
					EnIyiAdet = O.Value;
					EnIyiGrup = O.Key;
				}
			}
		}

		if (EnIyiGrup >= 101 && EnIyiGrup <= 120)
		{
			const int32 Kat = (EnIyiGrup <= 110) ? 1 : 2;
			BolgeAdlari[B] = FString::Printf(TEXT("%d. Kat %s"), Kat, GOdaAdlari[(EnIyiGrup - 101) % 10]);
		}
		else
		{
			BolgeAdlari[B] = FString::Printf(TEXT("%d. kat koridoru"), BolgeKat[B] + 1);
		}
	}

	// --- 6) kapıların bağladığı bölgeler -------------------------------------
	KapiListesi.Reset();
	for (AActor* A : Kapilar)
	{
		FMahzenKapiKaydi Kayit;
		Kayit.Aktor = A;
		Kayit.Anahtar = StrOzellik(A, TEXT("gerekenanahtar"));
		Kayit.Muhur = StrOzellik(A, TEXT("GerekliMuhurAdi"));

		// Kapı pivotu menteşede; açıklığın ortası ileri yönde ~52 cm.
		const FVector Orta = A->GetActorLocation() + A->GetActorForwardVector() * 52.0f;
		const FVector Sag = A->GetActorRightVector();
		const float Mesafeler[4] = { 80.0f, 110.0f, 140.0f, 170.0f };
		for (float M : Mesafeler)
		{
			if (Kayit.BolgeA == INDEX_NONE)
			{
				Kayit.BolgeA = EnYakinBolge(Orta + Sag * M, 2);
			}
			if (Kayit.BolgeB == INDEX_NONE)
			{
				Kayit.BolgeB = EnYakinBolge(Orta - Sag * M, 2);
			}
		}

		KapiListesi.Add(Kayit);
	}

	// Merdiven bağı: ızgara kat kat kurulduğu için merdiven boşluğu kenar
	// üretmiyor. Katlar arası geçiş merdiven odalarının içinden olduğundan
	// "k. Kat Merdiven" ile "(k+1). Kat Merdiven" bölgelerini anahtarsız bir
	// kenarla birleştiriyoruz.
	// Ada bakmak güvenilmez: 1. katta merdiven koridorun İÇİNDE kaldığı için o
	// bölge "1. kat koridoru" adını alıyor, "1. Kat Merdiven" hiç oluşmuyor ve
	// bağ kurulamıyordu (PIE'de ölçüldü: bütün 1. kat ulaşılamaz görünüp ilk
	// mühürde sahte game over geliyordu). Bu yüzden önce merdiven hedef
	// NOKTALARININ düştüğü bölgeye bakıyoruz (grup 101+3 = 104, 111+3 = 114),
	// ada ancak yedek olarak düşüyoruz.
	const int32 MerdivenIndeksi = 3;   // GOdaAdlari icindeki sira
	for (int32 Kat = 1; Kat < KatZ.Num(); ++Kat)
	{
		const int32 AltGrup = 101 + MerdivenIndeksi + (Kat - 1) * 10;
		const int32 UstGrup = AltGrup + 10;
		int32 AltB = INDEX_NONE;
		int32 UstB = INDEX_NONE;
		for (const TPair<int32, FVector>& N : Noktalar)
		{
			if (N.Key == AltGrup && AltB == INDEX_NONE)
			{
				AltB = EnYakinBolge(N.Value, 3);
			}
			else if (N.Key == UstGrup && UstB == INDEX_NONE)
			{
				UstB = EnYakinBolge(N.Value, 3);
			}
		}

		const FString Alt = FString::Printf(TEXT("%d. Kat Merdiven"), Kat);
		const FString Ust = FString::Printf(TEXT("%d. Kat Merdiven"), Kat + 1);
		if (AltB == INDEX_NONE)
		{
			AltB = BolgeAdlari.IndexOfByKey(Alt);
		}
		if (UstB == INDEX_NONE)
		{
			UstB = BolgeAdlari.IndexOfByKey(Ust);
		}

		if (AltB != INDEX_NONE && UstB != INDEX_NONE && AltB != UstB)
		{
			FMahzenKapiKaydi Bag;   // Aktor boş = serbest geçiş
			Bag.BolgeA = AltB;
			Bag.BolgeB = UstB;
			KapiListesi.Add(Bag);
			UE_LOG(LogTemp, Log, TEXT("MahzenHarita: merdiven bagi %s <-> %s"),
				*BolgeAdlari[AltB], *BolgeAdlari[UstB]);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("MahzenHarita: merdiven bagi kurulamadi (kat %d-%d, grup %d/%d, bolge %d/%d)"),
				Kat, Kat + 1, AltGrup, UstGrup, AltB, UstB);
		}
	}

	bHazir = true;

	UE_LOG(LogTemp, Log,
		TEXT("MahzenHarita: %d kat, %dx%d izgara, %d bolge, %d kapi (%.0f ms)"),
		NK, NX, NY, BolgeSayisi, KapiListesi.Num(), (FPlatformTime::Seconds() - T0) * 1000.0);
}

// ===================== sorgular =====================

int32 UMahzenHaritaAltSistemi::HucreBolgesi(int32 Kat, int32 I, int32 J) const
{
	if (Kat < 0 || Kat >= KatZ.Num() || I < 0 || J < 0 || I >= NX || J >= NY)
	{
		return INDEX_NONE;
	}
	return Hucreler[(Kat * NX + I) * NY + J];
}

int32 UMahzenHaritaAltSistemi::EnYakinBolge(const FVector& Konum, int32 AramaYaricapi) const
{
	if (KatZ.Num() == 0 || Hucreler.Num() == 0)
	{
		return INDEX_NONE;
	}

	int32 Kat = 0;
	float EnIyiFark = FLT_MAX;
	for (int32 K = 0; K < KatZ.Num(); ++K)
	{
		const float Fark = FMath::Abs(Konum.Z - KatZ[K]);
		if (Fark < EnIyiFark)
		{
			EnIyiFark = Fark;
			Kat = K;
		}
	}

	const int32 I0 = FMath::RoundToInt((Konum.X - Kose.X) / Adim);
	const int32 J0 = FMath::RoundToInt((Konum.Y - Kose.Y) / Adim);

	int32 Sonuc = INDEX_NONE;
	int32 EnIyiUzaklik = MAX_int32;
	for (int32 DI = -AramaYaricapi; DI <= AramaYaricapi; ++DI)
	{
		for (int32 DJ = -AramaYaricapi; DJ <= AramaYaricapi; ++DJ)
		{
			const int32 B = HucreBolgesi(Kat, I0 + DI, J0 + DJ);
			if (B == INDEX_NONE)
			{
				continue;
			}
			const int32 U = DI * DI + DJ * DJ;
			if (U < EnIyiUzaklik)
			{
				EnIyiUzaklik = U;
				Sonuc = B;
			}
		}
	}
	return Sonuc;
}

int32 UMahzenHaritaAltSistemi::BolgeNo(const FVector& Konum) const
{
	return EnYakinBolge(Konum, 3);
}

FString UMahzenHaritaAltSistemi::BolgeAdi(int32 Bolge) const
{
	return BolgeAdlari.IsValidIndex(Bolge) ? BolgeAdlari[Bolge] : FString(TEXT("bilinmeyen yer"));
}

void UMahzenHaritaAltSistemi::EsyalariTopla(TArray<FMahzenEsya>& Out) const
{
	UWorld* W = GetWorld();
	if (!W)
	{
		return;
	}

	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* A = *It;
		if (!EsyaMi(A))
		{
			continue;
		}
		const FString Ad = StrOzellik(A, TEXT("anahtarısmı"));
		if (Ad.IsEmpty())
		{
			continue;
		}
		FMahzenEsya Esya;
		Esya.Ad = Ad;
		Esya.Bolge = BolgeNo(A->GetActorLocation());
		Out.Add(Esya);
	}
}

// ===================== kilit analizi =====================

bool UMahzenHaritaAltSistemi::KilitlendiMi(EOyunSonuTuru& OutTur, FString& OutSebep)
{
	UWorld* W = GetWorld();
	if (!W || !bHazir)
	{
		return false;
	}

	APawn* Oyuncu = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Oyuncu)
	{
		return false;
	}

	const int32 Baslangic = BolgeNo(Oyuncu->GetActorLocation());
	if (Baslangic == INDEX_NONE)
	{
		return false;   // oyuncuyu ızgarada bulamadıysak karar verme
	}

	// --- emniyet kontrolleri -------------------------------------------------
	// Yanlış bir "oyun bitti" kararı, kaçırılmış bir soft lock'tan çok daha kötü.
	// Harita taraması şüpheliyse hiç karar verme.

	// 1) Oyuncunun bölgesi hiçbir kapıya komşu değilse ızgara onu yanlış yere
	//    oturtmuş demektir (mühürleme anında oyuncu her zaman bir kapının yanında).
	bool bBolgeTanidik = false;
	for (const FMahzenKapiKaydi& K : KapiListesi)
	{
		if (K.BolgeA == Baslangic || K.BolgeB == Baslangic)
		{
			bBolgeTanidik = true;
			break;
		}
	}
	if (!bBolgeTanidik)
	{
		UE_LOG(LogTemp, Warning, TEXT("MahzenHarita: oyuncunun bolgesi (%d) hicbir kapiya komsu degil, kilit kontrolu atlandi"), Baslangic);
		return false;
	}

	// 2) Adlandırılmış oda sayısı, seviyedeki benzersiz mühür sayısının altındaysa
	//    tarama odaları parçalamış olabilir; bu da sahte kilit üretir.
	{
		int32 OdaBolgesi = 0;
		for (const FString& Ad : BolgeAdlari)
		{
			if (Ad.Contains(TEXT(". Kat ")))
			{
				++OdaBolgesi;
			}
		}
		TSet<FString> Muhurler;
		for (const FMahzenKapiKaydi& K : KapiListesi)
		{
			if (!K.Muhur.IsEmpty())
			{
				Muhurler.Add(K.Muhur);
			}
		}
		if (OdaBolgesi < Muhurler.Num())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("MahzenHarita: %d oda bolgesi / %d muhur — tarama eksik, kilit kontrolu atlandi"),
				OdaBolgesi, Muhurler.Num());
			return false;
		}
	}

	TSet<FString> Envanter;
	EnvanteriOku(Oyuncu, Envanter);

	TArray<FMahzenEsya> Esyalar;
	EsyalariTopla(Esyalar);

	// --- erişilebilirlik sabit noktası ---------------------------------------
	TSet<int32> Ulasilabilir;
	Ulasilabilir.Add(Baslangic);

	bool bDegisti = true;
	while (bDegisti)
	{
		bDegisti = false;

		for (const FMahzenEsya& E : Esyalar)
		{
			if (E.Bolge != INDEX_NONE && Ulasilabilir.Contains(E.Bolge))
			{
				const FString N = Sadelestir(E.Ad);
				if (!Envanter.Contains(N))
				{
					Envanter.Add(N);
					bDegisti = true;
				}
			}
		}

		for (const FMahzenKapiKaydi& K : KapiListesi)
		{
			if (K.BolgeA == INDEX_NONE || K.BolgeB == INDEX_NONE || K.BolgeA == K.BolgeB)
			{
				continue;
			}

			AActor* A = K.Aktor.Get();

			// Aktörsüz kayıt = merdiven bağı, her zaman geçilir.
			// Mühürlü kapı yalnızca oda anahtarıyla açılır (mührü sökmek için de
			// aynı anahtar gerekiyor); mühürsüz ve kilidi açılmış kapı serbesttir.
			bool bGecilir = (A == nullptr);
			if (!bGecilir)
			{
				const bool bAcikKalmis = !BoolOzellik(A, TEXT("bMuhurlenmis")) && BoolOzellik(A, TEXT("bKilidiAcildi"));
				bGecilir = K.Anahtar.IsEmpty() || bAcikKalmis || Envanter.Contains(Sadelestir(K.Anahtar));
			}
			if (!bGecilir)
			{
				continue;
			}

			const bool bA = Ulasilabilir.Contains(K.BolgeA);
			const bool bB = Ulasilabilir.Contains(K.BolgeB);
			if (bA && !bB)
			{
				Ulasilabilir.Add(K.BolgeB);
				bDegisti = true;
			}
			else if (bB && !bA)
			{
				Ulasilabilir.Add(K.BolgeA);
				bDegisti = true;
			}
		}
	}

	// --- kalan mühürler bitirilebilir mi? ------------------------------------
	TSet<FString> AcikMuhurler;
	TMap<FString, bool> KapiUlasilir;

	for (const FMahzenKapiKaydi& K : KapiListesi)
	{
		if (K.Muhur.IsEmpty())
		{
			continue;
		}
		AActor* A = K.Aktor.Get();
		if (!A || BoolOzellik(A, TEXT("bMuhurlenmis")))
		{
			continue;
		}
		AcikMuhurler.Add(K.Muhur);
		bool& U = KapiUlasilir.FindOrAdd(K.Muhur);
		U = U || Ulasilabilir.Contains(K.BolgeA) || Ulasilabilir.Contains(K.BolgeB);
	}

	auto EsyaNerede = [&](const FString& Ad) -> FString
	{
		const FString N = Sadelestir(Ad);
		for (const FMahzenEsya& E : Esyalar)
		{
			if (Sadelestir(E.Ad) == N)
			{
				return BolgeAdi(E.Bolge);
			}
		}
		return FString();
	};

	TArray<FString> Satirlar;
	for (const FString& M : AcikMuhurler)
	{
		const bool bEsyaVar = Envanter.Contains(Sadelestir(M));
		const bool bKapiVar = KapiUlasilir.FindRef(M);
		if (bEsyaVar && bKapiVar)
		{
			continue;
		}

		FString Neden;
		if (!bEsyaVar)
		{
			const FString Yer = EsyaNerede(M);
			Neden = Yer.IsEmpty()
				? FString(TEXT("mühür taşına artık ulaşamıyorsun"))
				: FString::Printf(TEXT("mühür taşı %s içinde kaldı"), *Yer);
		}
		else
		{
			Neden = TEXT("kapısına artık ulaşamıyorsun");
		}
		Satirlar.Add(FString::Printf(TEXT("%s — %s"), *M, *Neden));
	}

	if (Satirlar.Num() == 0)
	{
		return false;   // oyun hâlâ bitirilebilir
	}
	Satirlar.Sort();

	// --- arkada kalan anahtarlar ---------------------------------------------
	TArray<FString> KalanAnahtarlar;
	for (const FMahzenEsya& E : Esyalar)
	{
		if (E.Bolge == INDEX_NONE || Ulasilabilir.Contains(E.Bolge))
		{
			continue;
		}
		if (Envanter.Contains(Sadelestir(E.Ad)))
		{
			continue;
		}
		if (!E.Ad.Contains(TEXT("anahtar")))
		{
			continue;
		}
		KalanAnahtarlar.Add(FString::Printf(TEXT("%s → %s"), *E.Ad, *BolgeAdi(E.Bolge)));
	}
	KalanAnahtarlar.Sort();

	const FString BaslangicAdi = BolgeAdi(Baslangic);
	const bool bHapis = (Ulasilabilir.Num() <= 1) && !BaslangicAdi.Contains(TEXT("koridor"));

	FString Metin;

	if (bHapis)
	{
		OutTur = EOyunSonuTuru::KendiniKilitledi;

		TArray<FString> Eksik;
		for (const FMahzenKapiKaydi& K : KapiListesi)
		{
			if (K.BolgeA != Baslangic && K.BolgeB != Baslangic)
			{
				continue;
			}
			if (K.Anahtar.IsEmpty() || Envanter.Contains(Sadelestir(K.Anahtar)))
			{
				continue;
			}
			Eksik.AddUnique(K.Anahtar);
		}
		Eksik.Sort();

		Metin = FString::Printf(TEXT("%s içinde kilitli kaldın.\n"), *BaslangicAdi);
		Metin += Eksik.Num() > 0
			? FString::Printf(TEXT("Bütün çıkışları mühürledin ve %s envanterinde yok."), *ListeyeCevir(Eksik))
			: FString(TEXT("Bütün çıkışları mühürledin."));
	}
	else
	{
		OutTur = EOyunSonuTuru::Cikmaz;
		Metin = TEXT("Oyun artık bitirilemez:\n");

		const int32 Gosterilecek = FMath::Min(Satirlar.Num(), 4);
		for (int32 i = 0; i < Gosterilecek; ++i)
		{
			Metin += FString::Printf(TEXT("\n%s"), *Satirlar[i]);
		}
		if (Satirlar.Num() > Gosterilecek)
		{
			Metin += FString::Printf(TEXT("\n... ve %d mühür daha"), Satirlar.Num() - Gosterilecek);
		}
	}

	if (KalanAnahtarlar.Num() > 0)
	{
		Metin += TEXT("\n\nUlaşamadığın eşyalar:");
		const int32 Gosterilecek = FMath::Min(KalanAnahtarlar.Num(), 3);
		for (int32 i = 0; i < Gosterilecek; ++i)
		{
			Metin += FString::Printf(TEXT("\n%s"), *KalanAnahtarlar[i]);
		}
		if (KalanAnahtarlar.Num() > Gosterilecek)
		{
			Metin += FString::Printf(TEXT("\n... ve %d eşya daha"), KalanAnahtarlar.Num() - Gosterilecek);
		}
	}

	OutSebep = Metin;
	return true;
}

void UMahzenHaritaAltSistemi::Dok()
{
	Kur();
	if (!bHazir)
	{
		UE_LOG(LogTemp, Warning, TEXT("MahzenHarita: harita hazir degil"));
		return;
	}

	UWorld* W = GetWorld();
	APawn* Oyuncu = W ? UGameplayStatics::GetPlayerPawn(W, 0) : nullptr;
	if (Oyuncu)
	{
		UE_LOG(LogTemp, Log, TEXT("MahzenHarita: oyuncu -> %s"), *BolgeAdi(BolgeNo(Oyuncu->GetActorLocation())));
	}

	for (const FMahzenKapiKaydi& K : KapiListesi)
	{
		AActor* A = K.Aktor.Get();
		UE_LOG(LogTemp, Log, TEXT("  kapi %-16s %-28s muhur=%-24s %s <-> %s%s"),
			A ? *A->GetName() : TEXT("?"),
			*K.Anahtar, K.Muhur.IsEmpty() ? TEXT("-") : *K.Muhur,
			*BolgeAdi(K.BolgeA), *BolgeAdi(K.BolgeB),
			BoolOzellik(A, TEXT("bMuhurlenmis")) ? TEXT("  [MUHURLU]") : TEXT(""));
	}

	TArray<FMahzenEsya> Esyalar;
	EsyalariTopla(Esyalar);
	for (const FMahzenEsya& E : Esyalar)
	{
		UE_LOG(LogTemp, Log, TEXT("  esya %-30s -> %s"), *E.Ad, *BolgeAdi(E.Bolge));
	}

	EOyunSonuTuru Tur;
	FString Sebep;
	if (KilitlendiMi(Tur, Sebep))
	{
		UE_LOG(LogTemp, Warning, TEXT("MahzenHarita: KILITLI (tur=%d)\n%s"), (int32)Tur, *Sebep);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("MahzenHarita: oyun bitirilebilir durumda"));
	}
}

// ===================== blueprint kütüphanesi =====================

namespace
{
	void MuhurDurumu(UWorld* W, TSet<FString>& Tum, TSet<FString>& Muhurlu)
	{
		for (TActorIterator<AActor> It(W); It; ++It)
		{
			AActor* A = *It;
			if (!KapiMi(A))
			{
				continue;
			}
			const FString Ad = StrOzellik(A, TEXT("GerekliMuhurAdi"));
			if (Ad.IsEmpty())
			{
				continue;
			}
			Tum.Add(Ad);
			if (BoolOzellik(A, TEXT("bMuhurlenmis")))
			{
				Muhurlu.Add(Ad);
			}
		}
	}

	UWorld* DunyaBul(const UObject* Baglam)
	{
		return GEngine ? GEngine->GetWorldFromContextObject(Baglam, EGetWorldErrorMode::ReturnNull) : nullptr;
	}
}

void UMahzenHarita::MuhurSonrasiDegerlendir(const UObject* DunyaBaglami)
{
	UWorld* W = DunyaBul(DunyaBaglami);
	if (!W || UOyunSonu::Aktif())
	{
		return;
	}

	TSet<FString> Tum, Muhurlu;
	MuhurDurumu(W, Tum, Muhurlu);

	if (Tum.Num() > 0 && Muhurlu.Num() >= Tum.Num())
	{
		UOyunSonu::Goster(W, EOyunSonuTuru::Zafer,
			FString::Printf(TEXT("%d odanın hepsini mühürledin. Ev sustu."), Tum.Num()));
		return;
	}

	UMahzenHaritaAltSistemi* Harita = W->GetSubsystem<UMahzenHaritaAltSistemi>();
	if (!Harita)
	{
		return;
	}
	Harita->Kur();

	EOyunSonuTuru Tur = EOyunSonuTuru::Cikmaz;
	FString Sebep;
	if (Harita->KilitlendiMi(Tur, Sebep))
	{
		// Oyuncunun kendini bir odaya kilitlemesi onun kararı, her zaman gösterilir.
		// Ama "çıkmaz" daha oyun başlamadan doğmuşsa suç oyuncuda değil: dağıtım o
		// tohumda çözülemez çıkmış. Böyle bir oyunu ilk mühürde bitirmek yanlış.
		if (Tur == EOyunSonuTuru::Cikmaz && Harita->BaslangictaKilitliMi())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("MahzenHarita: cikmaz ekrani BASTIRILDI (tohum zaten baslangicta bitirilemezdi).\n%s"),
				*Sebep);
			return;
		}
		UOyunSonu::Goster(W, Tur, Sebep);
	}
}

int32 UMahzenHarita::ToplamMuhurSayisi(const UObject* DunyaBaglami)
{
	UWorld* W = DunyaBul(DunyaBaglami);
	if (!W)
	{
		return 0;
	}
	TSet<FString> Tum, Muhurlu;
	MuhurDurumu(W, Tum, Muhurlu);
	return Tum.Num();
}

int32 UMahzenHarita::MuhurlenenSayisi(const UObject* DunyaBaglami)
{
	UWorld* W = DunyaBul(DunyaBaglami);
	if (!W)
	{
		return 0;
	}
	TSet<FString> Tum, Muhurlu;
	MuhurDurumu(W, Tum, Muhurlu);
	return Muhurlu.Num();
}

// ===================== konsol komutları =====================

static FAutoConsoleCommandWithWorld GMahzenHaritaKomutu(
	TEXT("mahzen.harita"),
	TEXT("Bolge haritasini, kapilari, esyalari ve kilit durumunu loga doker."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* W)
	{
		if (!W)
		{
			return;
		}
		if (UMahzenHaritaAltSistemi* H = W->GetSubsystem<UMahzenHaritaAltSistemi>())
		{
			H->Dok();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GMahzenSonuKomutu(
	TEXT("mahzen.sonu"),
	TEXT("Oyun sonu ekranini test eder: 0=jumpscare 1=zafer 2=kilitli kaldin 3=cikmaz"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Argumanlar, UWorld* W)
	{
		if (!W)
		{
			return;
		}
		const int32 No = Argumanlar.Num() > 0 ? FCString::Atoi(*Argumanlar[0]) : 0;
		const EOyunSonuTuru Tur = static_cast<EOyunSonuTuru>(static_cast<uint8>(FMath::Clamp(No, 0, 3)));
		FString Sebep;
		if (Tur == EOyunSonuTuru::KendiniKilitledi)
		{
			Sebep = TEXT("1. Kat Ofis içinde kilitli kaldın.\nBütün çıkışları mühürledin ve \"1. Kat ofis anahtarı\" envanterinde yok.");
		}
		else if (Tur == EOyunSonuTuru::Cikmaz)
		{
			Sebep = TEXT("Oyun artık bitirilemez:\n\n2. Kat Mutfak mührü — mühür taşı 1. Kat Ofis içinde kaldı\n\nUlaşamadığın eşyalar:\n2. Kat mutfak anahtarı → 1. Kat Ofis");
		}
		else if (Tur == EOyunSonuTuru::Zafer)
		{
			Sebep = TEXT("20 odanın hepsini mühürledin. Ev sustu.");
		}
		UOyunSonu::Goster(W, Tur, Sebep);
	}));
