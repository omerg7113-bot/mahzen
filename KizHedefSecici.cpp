#include "KizHedefSecici.h"

#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "FenerPili.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

namespace
{
	constexpr int32 EnFazlaAday = 32;

	// Yol bulunamazsa (kapalı kapı, kısmi yol) düz mesafeyi bu katsayıyla büyütüp tahmin ederiz.
	constexpr float YolYokDolambacKatsayisi = 1.5f;

	// Kapı menteşesi gibi navmesh dışı noktaları yola oturtmak için (kapı sesi ortalama 52 cm dışarıda).
	const FVector NavOturtmaGenisligi(200.f, 200.f, 150.f);
}

UKizHedefSecici::UKizHedefSecici()
{
	PrimaryComponentTick.bCanEverTick = true;

	TurAyarlari.SetNum(5);
	auto Ayarla = [this](EKizOlay T, float Carpan, float Menzil, bool bKat, float Omur)
	{
		FKizTurAyari& A = TurAyarlari[static_cast<int32>(T)];
		A.Carpan = Carpan;
		A.Menzil = Menzil;
		A.bKatlarArasi = bKat;
		A.OmurSn = Omur;
	};

	// Çarpanlar ve menziller kullanıcının karşılaştırma panolarından çıkarıldı.
	Ayarla(EKizOlay::Yok, 0.f, 0.f, false, 1.f);
	// Katlar arası duyulma moda bağlı (bModKatlarArasi): Takipçi kendi katı, Agresif iki kat.
	Ayarla(EKizOlay::Esya, 1.00f, 1800.f, true, 16.f);
	Ayarla(EKizOlay::Kapi, 1.20f, 2500.f, true, 16.f);
	Ayarla(EKizOlay::Muhur, 1.45f, 3200.f, true, 16.f);
	Ayarla(EKizOlay::Fener, 1.55f, 4000.f, false, 4.f);
}

const FKizTurAyari* UKizHedefSecici::Ayar(EKizOlay Tur) const
{
	const int32 I = static_cast<int32>(Tur);
	return TurAyarlari.IsValidIndex(I) ? &TurAyarlari[I] : nullptr;
}

int32 UKizHedefSecici::KatIndeksi(float Z) const
{
	return FMath::FloorToInt((Z - KatTabanZ) / KatYuksekligi);
}

bool UKizHedefSecici::AyniAdayMi(const FKizAday& A, const FKizAday& B) const
{
	if (A.Tur != B.Tur)
	{
		return false;
	}
	if ((A.Kaynak.IsValid() || B.Kaynak.IsValid()) && A.Kaynak != B.Kaynak)
	{
		return false;
	}
	return FVector::DistSquared(A.OlayKonumu, B.OlayKonumu) < FMath::Square(50.f);
}

float UKizHedefSecici::PilCarpani(float PilOrani) const
{
	const float Bos = 1.f - FMath::Clamp(PilOrani, 0.f, 1.f);
	return 1.f + PilKatsayisi * FMath::Pow(Bos, PilUssu);
}

float UKizHedefSecici::GrupCarpani(const FKizAday& Aday) const
{
	UWorld* Dunya = GetWorld();
	if (!Dunya)
	{
		return 1.f;
	}

	int32 Ek = 0;
	for (FConstPlayerControllerIterator It = Dunya->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		const APawn* Oyuncu = PC ? PC->GetPawn() : nullptr;
		if (!Oyuncu || Oyuncu == Aday.Kaynak.Get())
		{
			continue;
		}
		if (FVector::DistSquared(Oyuncu->GetActorLocation(), Aday.OlayKonumu) <= FMath::Square(GrupYaricapCm))
		{
			++Ek;
		}
	}

	return 1.f + FMath::Min(GrupBonusOyuncuBasina * Ek, GrupBonusEnFazla);
}

float UKizHedefSecici::AlismaCarpani(const FKizAday& Aday, double Simdi) const
{
	if (Aday.Tur == EKizOlay::Fener)
	{
		return 1.f;
	}

	for (const FAlismaKayit& K : AlismaKayitlari)
	{
		if (K.Kaynak == Aday.Kaynak && Simdi - K.Son <= AlismaSifirlamaSn
			&& FVector::DistSquared(K.Konum, Aday.OlayKonumu) <= FMath::Square(AlismaYaricapCm))
		{
			return FMath::Max(AlismaEnAzCarpan, 1.f - AlismaCezasiGidisBasina * K.Sayi);
		}
	}
	return 1.f;
}

float UKizHedefSecici::EtkinMesafeHesapla(const FKizAday& Aday, float YolCm, float GrupC) const
{
	const FKizTurAyari* Ay = Ayar(Aday.Tur);
	const float TurC = Ay ? Ay->Carpan : 1.f;
	const float FenerC = Aday.bFenerAcik ? 1.f : FenerKapaliCarpani;
	const float Payda = FMath::Max(TurC * PilCarpani(Aday.PilOrani) * FenerC * GrupC, KINDA_SMALL_NUMBER);
	return FMath::Max(YolCm, EnAzMesafeCm) / Payda;
}

float UKizHedefSecici::YolUzunluguHesapla(const FVector& Baslangic, const FVector& Hedef) const
{
	const float Duz = FVector::Dist(Baslangic, Hedef);

	UWorld* Dunya = GetWorld();
	UNavigationSystemV1* Nav = Dunya ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Dunya) : nullptr;
	if (!Nav)
	{
		return Duz * YolYokDolambacKatsayisi;
	}

	FVector Bitis = Hedef;
	FNavLocation Oturtulan;
	if (Nav->ProjectPointToNavigation(Hedef, Oturtulan, NavOturtmaGenisligi))
	{
		Bitis = Oturtulan.Location;
	}

	const UNavigationPath* Yol = UNavigationSystemV1::FindPathToLocationSynchronously(
		Dunya, Baslangic, Bitis, GetOwner());

	// Geçersiz yolda GetPathLength -1 döner; kısmi yol da hedefe varılamayacağı anlamına gelir.
	if (Yol && Yol->IsValid() && !Yol->IsPartial())
	{
		const float Uzunluk = Yol->GetPathLength();
		if (Uzunluk >= 0.f)
		{
			return Uzunluk;
		}
	}

	return Duz * YolYokDolambacKatsayisi;
}

void UKizHedefSecici::PilBilgisiOku(const AActor* Kaynak, float& PilOrani, bool& bFenerAcik) const
{
	PilOrani = 1.f;
	bFenerAcik = true;

	if (!Kaynak)
	{
		return;
	}

	const UFenerPili* Pil = Kaynak->FindComponentByClass<UFenerPili>();
	if (!Pil)
	{
		return;
	}

	PilOrani = Pil->KalanYuzde();
	bFenerAcik = Pil->FenerAcikMi() && PilOrani > 0.f;
}

FVector UKizHedefSecici::FenerAdimHedefi(const FVector& KizKonum, const FVector& KaynakKonum) const
{
	UWorld* Dunya = GetWorld();

	// Önce yürüme yolunda FenerAdimCm kadar ilerle; yol yoksa kaynağa doğru düz çizgide ilerle.
	if (Dunya)
	{
		const UNavigationPath* Yol = UNavigationSystemV1::FindPathToLocationSynchronously(
			Dunya, KizKonum, KaynakKonum, GetOwner());
		if (Yol && Yol->IsValid() && Yol->PathPoints.Num() >= 2)
		{
			float Kalan = FenerAdimCm;
			for (int32 i = 1; i < Yol->PathPoints.Num(); ++i)
			{
				const float Parca = FVector::Dist(Yol->PathPoints[i - 1], Yol->PathPoints[i]);
				if (Parca >= Kalan && Parca > KINDA_SMALL_NUMBER)
				{
					return FMath::Lerp(Yol->PathPoints[i - 1], Yol->PathPoints[i], Kalan / Parca);
				}
				Kalan -= Parca;
			}
			return Yol->PathPoints.Last();
		}
	}

	FVector Hedef = KizKonum + (KaynakKonum - KizKonum).GetSafeNormal2D() * FenerAdimCm;
	UNavigationSystemV1* Nav = Dunya ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Dunya) : nullptr;
	FNavLocation Oturtulan;
	if (Nav && Nav->ProjectPointToNavigation(Hedef, Oturtulan, NavOturtmaGenisligi))
	{
		Hedef = Oturtulan.Location;
	}
	return Hedef;
}

void UKizHedefSecici::OlayBildir(EKizOlay Tur, FVector Konum, AActor* Kaynak)
{
	AActor* Sahip = GetOwner();
	UWorld* Dunya = GetWorld();
	if (!Sahip || !Dunya || !Sahip->HasAuthority() || !bModDinler)
	{
		return;
	}

	// Fener içeriden algılanır; Yok olay değildir.
	if (Tur != EKizOlay::Esya && Tur != EKizOlay::Kapi && Tur != EKizOlay::Muhur)
	{
		UE_LOG(LogTemp, Warning, TEXT("KizHedefSecici: OlayBildir yalniz Esya/Kapi/Muhur kabul eder."));
		return;
	}

	const FKizTurAyari* Ay = Ayar(Tur);
	if (!Ay || Ay->Carpan <= 0.f)
	{
		return;
	}

	FKizAday Yeni;
	Yeni.Tur = Tur;
	Yeni.Konum = Konum;
	Yeni.OlayKonumu = Konum;
	Yeni.Kaynak = Kaynak;
	PilBilgisiOku(Kaynak, Yeni.PilOrani, Yeni.bFenerAcik);
	Yeni.DogusZamani = Dunya->GetTimeSeconds();
	Yeni.OmurSn = Ay->OmurSn;

	for (FKizAday& Var : Adaylar)
	{
		if (AyniAdayMi(Var, Yeni))
		{
			// Seçilmiş ve yürünen hedefin süresini yenilemeyiz; yalnız bekleyen aday tazelenir.
			if (!AyniAdayMi(Var, EnIyi))
			{
				Var = Yeni;
			}
			return;
		}
	}

	if (Adaylar.Num() >= EnFazlaAday)
	{
		Adaylar.RemoveAt(0);
	}
	Adaylar.Add(Yeni);
}

void UKizHedefSecici::ModAyarla(bool bDinler, bool bKatlarArasi)
{
	bModDinler = bDinler;
	bModKatlarArasi = bKatlarArasi;

	// Her mod değişiminde alışma sayaçları sıfırlanır: yeni modda kız "taze" başlar.
	FenerKayitlari.Empty();
	AlismaKayitlari.Empty();

	if (!bDinler)
	{
		Adaylar.Empty();
		Askida = FKizAday();
		SecimiGuncelle(FKizAday());
	}
}

void UKizHedefSecici::TakipAktifAyarla(bool bYeniTakip)
{
	if (bTakipte == bYeniTakip)
	{
		return;
	}
	bTakipte = bYeniTakip;

	if (bTakipte)
	{
		// Biri görüldü: fener alışma sayaçları sıfırlanır, kesilen olay hedefi unutulur.
		FenerKayitlari.Empty();
		AlismaKayitlari.Empty();
		Askida = FKizAday();
		SecimiGuncelle(FKizAday());
	}
}

void UKizHedefSecici::EnIyiAdayiTuket()
{
	// Kız bir olay yerine gidip döndü (vardı ya da vazgeçti): o oyuncunun oradaki sonraki olayı biraz daha az çeker.
	if (EnIyi.Tur == EKizOlay::Esya || EnIyi.Tur == EKizOlay::Kapi || EnIyi.Tur == EKizOlay::Muhur)
	{
		UWorld* Dunya = GetWorld();
		const double Simdi = Dunya ? Dunya->GetTimeSeconds() : 0.0;

		AlismaKayitlari.RemoveAll([&](const FAlismaKayit& K) { return Simdi - K.Son > AlismaSifirlamaSn; });

		FAlismaKayit* Var = nullptr;
		for (FAlismaKayit& K : AlismaKayitlari)
		{
			if (K.Kaynak == EnIyi.Kaynak
				&& FVector::DistSquared(K.Konum, EnIyi.OlayKonumu) <= FMath::Square(AlismaYaricapCm))
			{
				Var = &K;
				break;
			}
		}
		if (Var)
		{
			Var->Son = Simdi;
			Var->Sayi += 1;
		}
		else
		{
			if (AlismaKayitlari.Num() >= EnFazlaAday)
			{
				AlismaKayitlari.RemoveAt(0);
			}
			FAlismaKayit Yeni;
			Yeni.Kaynak = EnIyi.Kaynak;
			Yeni.Konum = EnIyi.OlayKonumu;
			Yeni.Son = Simdi;
			Yeni.Sayi = 1;
			AlismaKayitlari.Add(Yeni);
		}
	}

	for (int32 i = Adaylar.Num() - 1; i >= 0; --i)
	{
		if (AyniAdayMi(Adaylar[i], EnIyi))
		{
			Adaylar.RemoveAt(i);
		}
	}

	// Bir olay hedefi tüketildiyse kesilen olay da artık geçersiz; fener tüketildiyse ona geri dönülür.
	if (EnIyi.Tur != EKizOlay::Fener)
	{
		Askida = FKizAday();
	}

	SecimiGuncelle(FKizAday());
}

void UKizHedefSecici::FenerleriTara(double Simdi)
{
	UWorld* Dunya = GetWorld();
	AActor* Kiz = GetOwner();
	const FKizTurAyari* Ay = Ayar(EKizOlay::Fener);
	if (!Dunya || !Kiz || !Ay || Ay->Carpan <= 0.f)
	{
		return;
	}

	const FVector Hedef = Kiz->GetActorLocation();

	for (FConstPlayerControllerIterator It = Dunya->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		APawn* Oyuncu = PC ? PC->GetPawn() : nullptr;
		if (!Oyuncu)
		{
			continue;
		}

		const UFenerPili* Pil = Oyuncu->FindComponentByClass<UFenerPili>();
		if (!Pil || !Pil->FenerAcikMi())
		{
			continue;
		}

		const USpotLightComponent* Fener = Pil->FenerBileseni();
		if (!Fener)
		{
			continue;
		}

		if (FFenerKayit* Kayit = FenerKayitlari.Find(Oyuncu))
		{
			if (Simdi - Kayit->Son > FenerAlismaSifirlamaSn)
			{
				Kayit->Sayi = 0;
			}
			if (Simdi - Kayit->Son < FenerTekrarBeklemeSn || Kayit->Sayi >= FenerAlismaSayisi)
			{
				continue;
			}
		}

		const FVector Kaynak = Fener->GetComponentLocation();
		const FVector Yon = Hedef - Kaynak;
		const float Mesafe = Yon.Size();
		const float Menzil = FMath::Min(FenerIsikMenziliCm, Fener->AttenuationRadius);
		if (Mesafe < 1.f || Mesafe > Menzil)
		{
			continue;
		}

		const float Kosinus = FVector::DotProduct(Fener->GetForwardVector(), Yon / Mesafe);
		if (Kosinus < FMath::Cos(FMath::DegreesToRadians(Fener->OuterConeAngle)))
		{
			continue;
		}

		FCollisionQueryParams Parametre(SCENE_QUERY_STAT(KizFenerAlgisi), false);
		Parametre.AddIgnoredActor(Oyuncu);
		Parametre.AddIgnoredActor(Kiz);
		FHitResult Vurus;
		if (Dunya->LineTraceSingleByChannel(Vurus, Kaynak, Hedef, ECC_Visibility, Parametre))
		{
			continue;
		}

		FKizAday Yeni;
		Yeni.Tur = EKizOlay::Fener;
		Yeni.OlayKonumu = Oyuncu->GetActorLocation();
		Yeni.Konum = FenerAdimHedefi(Hedef, Yeni.OlayKonumu);
		Yeni.Kaynak = Oyuncu;
		Yeni.PilOrani = Pil->KalanYuzde();
		Yeni.bFenerAcik = true;
		Yeni.DogusZamani = Simdi;
		Yeni.OmurSn = Ay->OmurSn;

		if (Adaylar.Num() >= EnFazlaAday)
		{
			Adaylar.RemoveAt(0);
		}
		Adaylar.Add(Yeni);

		FFenerKayit& Kayit = FenerKayitlari.FindOrAdd(Oyuncu);
		Kayit.Son = Simdi;
		Kayit.Sayi += 1;
	}
}

FKizAday UKizHedefSecici::EnIyiSec()
{
	const AActor* Sahip = GetOwner();
	if (!Sahip)
	{
		return FKizAday();
	}

	const FVector KizKonum = Sahip->GetActorLocation();

	// 1) Kıza ışık isabet ettiyse tepki verir (yürüdüğü olayı keser). Mevcut fener hedefi korunur.
	int32 FenerIndeks = INDEX_NONE;
	float FenerDeger = TNumericLimits<float>::Max();
	for (int32 i = 0; i < Adaylar.Num(); ++i)
	{
		FKizAday& A = Adaylar[i];
		if (A.Tur != EKizOlay::Fener)
		{
			continue;
		}
		if (EnIyi.Tur == EKizOlay::Fener && AyniAdayMi(A, EnIyi))
		{
			return A;
		}

		A.YolUzunlugu = YolUzunluguHesapla(KizKonum, A.OlayKonumu);
		A.EtkinMesafe = EtkinMesafeHesapla(A, A.YolUzunlugu, 1.f);
		if (A.EtkinMesafe < FenerDeger)
		{
			FenerDeger = A.EtkinMesafe;
			FenerIndeks = i;
		}
	}
	if (FenerIndeks != INDEX_NONE)
	{
		return Adaylar[FenerIndeks];
	}

	// 2) Bağlı hedef: seçilmiş bir olay (ya da fenerle kesilen olay) varsa vazgeçilmez.
	const FKizAday& Bagli = (EnIyi.Tur == EKizOlay::Yok || EnIyi.Tur == EKizOlay::Fener) ? Askida : EnIyi;
	if (Bagli.Tur != EKizOlay::Yok)
	{
		for (const FKizAday& A : Adaylar)
		{
			if (AyniAdayMi(A, Bagli))
			{
				return A;
			}
		}
	}

	// 3) Yeni hedef: duyulabilen adaylar arasından etkin mesafesi en küçük olan.
	const int32 KizKat = KatIndeksi(KizKonum.Z);
	UWorld* Dunya = GetWorld();
	const double Simdi = Dunya ? Dunya->GetTimeSeconds() : 0.0;
	const float GrupEnFazla = 1.f + GrupBonusEnFazla;

	struct FAdayAlt
	{
		int32 Indeks;
		float AltSinir;
	};
	TArray<FAdayAlt, TInlineAllocator<8>> Liste;
	for (int32 i = 0; i < Adaylar.Num(); ++i)
	{
		const FKizAday& A = Adaylar[i];
		const FKizTurAyari* Ay = Ayar(A.Tur);
		if (A.Tur == EKizOlay::Fener || !Ay || Ay->Carpan <= 0.f)
		{
			continue;
		}
		if (!(bModKatlarArasi && Ay->bKatlarArasi) && KatIndeksi(A.OlayKonumu.Z) != KizKat)
		{
			continue;
		}

		const float Duz = FVector::Dist(KizKonum, A.OlayKonumu);
		if (Ay->Menzil > 0.f && Duz > Ay->Menzil)
		{
			continue;
		}

		// Alt sınır iyimser: yol düz mesafeden kısa olamaz, grup bonusu en çok ne ise o.
		Liste.Add({i, EtkinMesafeHesapla(A, Duz, GrupEnFazla)});
	}

	if (Liste.Num() == 0)
	{
		return FKizAday();
	}

	Liste.Sort([](const FAdayAlt& X, const FAdayAlt& Y) { return X.AltSinir < Y.AltSinir; });

	// Pahalı yol sorgusu yalnız kazanma şansı olanlar için: alt sınırı mevcut en iyiden büyük olanlar elenir.
	float EnIyiDeger = TNumericLimits<float>::Max();
	int32 EnIyiIndeks = INDEX_NONE;
	for (const FAdayAlt& L : Liste)
	{
		if (L.AltSinir >= EnIyiDeger)
		{
			break;
		}

		FKizAday& A = Adaylar[L.Indeks];
		const FKizTurAyari* Ay = Ayar(A.Tur);
		A.YolUzunlugu = YolUzunluguHesapla(KizKonum, A.OlayKonumu);
		if (Ay && Ay->Menzil > 0.f && A.YolUzunlugu > Ay->Menzil)
		{
			continue;
		}

		A.EtkinMesafe = EtkinMesafeHesapla(A, A.YolUzunlugu, GrupCarpani(A) * AlismaCarpani(A, Simdi));
		if (A.EtkinMesafe < EnIyiDeger)
		{
			EnIyiDeger = A.EtkinMesafe;
			EnIyiIndeks = L.Indeks;
		}
	}

	return EnIyiIndeks == INDEX_NONE ? FKizAday() : Adaylar[EnIyiIndeks];
}

void UKizHedefSecici::SecimiGuncelle(const FKizAday& Yeni)
{
	const bool bDegisti = Yeni.Tur != EnIyi.Tur || (Yeni.Tur != EKizOlay::Yok && !AyniAdayMi(Yeni, EnIyi));

	if (bDegisti)
	{
		UWorld* Dunya = GetWorld();
		const double Simdi = Dunya ? Dunya->GetTimeSeconds() : 0.0;

		if (Yeni.Tur == EKizOlay::Fener && EnIyi.Tur != EKizOlay::Fener)
		{
			// Fener bir olay yolunu kesti: o olayı bekletir, bekleme boyunca süresi dolmasın.
			if (EnIyi.Tur != EKizOlay::Yok)
			{
				Askida = EnIyi;
			}
			const FKizTurAyari* FA = Ayar(EKizOlay::Fener);
			const double Ek = FA ? FA->OmurSn : 4.0;
			for (FKizAday& A : Adaylar)
			{
				if (A.Tur != EKizOlay::Fener)
				{
					A.DogusZamani += Ek;
				}
			}
		}
		else if (Yeni.Tur != EKizOlay::Yok && Yeni.Tur != EKizOlay::Fener)
		{
			// Yeni (ya da geri dönülen) olay hedefi: yürüme süresine yetecek kadar ömür verilir.
			Askida = FKizAday();
			for (FKizAday& A : Adaylar)
			{
				if (AyniAdayMi(A, Yeni))
				{
					const float Gerekli = (Yeni.YolUzunlugu > 0.f ? Yeni.YolUzunlugu / MinYurumeHiziCm : 0.f) + VarisPayiSn;
					A.OmurSn = FMath::Max(A.OmurSn, Gerekli);
					A.DogusZamani = Simdi;
					break;
				}
			}
		}

		if (bLogla)
		{
			UE_LOG(LogTemp, Log, TEXT("KizHedefSecici: %s -> %s | kaynak %s | yol %.0f cm | pil %.0f%% | etkin %.0f"),
				*UEnum::GetValueAsString(EnIyi.Tur),
				*UEnum::GetValueAsString(Yeni.Tur),
				Yeni.Kaynak.IsValid() ? *Yeni.Kaynak->GetName() : TEXT("-"),
				Yeni.YolUzunlugu, Yeni.PilOrani * 100.f, Yeni.EtkinMesafe);
		}
	}

	EnIyi = Yeni;
}

void UKizHedefSecici::TickComponent(float DeltaSeconds, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	AActor* Sahip = GetOwner();
	UWorld* Dunya = GetWorld();
	if (!bAktif || !Sahip || !Dunya || !Sahip->HasAuthority())
	{
		return;
	}

	Birikim += DeltaSeconds;
	if (Birikim < KararAraligiSn)
	{
		return;
	}
	Birikim = 0.f;

	// Takip sürerken (ya da Hayalet'te) olay listesine bakılmaz; yol sorgusu da yapılmaz.
	if (bTakipte || !bModDinler)
	{
		return;
	}

	const double Simdi = Dunya->GetTimeSeconds();
	Adaylar.RemoveAll([Simdi](const FKizAday& A)
	{
		return Simdi - A.DogusZamani > A.OmurSn;
	});

	FenerleriTara(Simdi);
	SecimiGuncelle(EnIyiSec());
}
