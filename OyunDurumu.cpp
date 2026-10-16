#include "OyunDurumu.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MyCharacter.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* GYuva = TEXT("OyunDurumu");

	/** Harita açılışında kayıt uygulanacak mı (seviye değişimini aşması gerekiyor). */
	bool GUygulaBekliyor = false;

	/** Harita hangi kipte açılacak. İlk açılış = menü. */
	EOyunModu GMod = EOyunModu::Menu;

	bool KapiMi(const AActor* A)
	{
		// Sınıf adı "bp_kapı_C"; Türkçe harfi aramamak için önekle bak.
		return A && A->GetClass()->GetName().StartsWith(TEXT("bp_kap"));
	}

	bool KasaMi(const AActor* A)
	{
		return A && A->GetClass()->GetName().StartsWith(TEXT("Bp_Kasa"));
	}

	bool EsyaMi(const AActor* A)
	{
		if (!A)
		{
			return false;
		}
		const FString Ad = A->GetClass()->GetName();
		return Ad.StartsWith(TEXT("bm_key")) || Ad.StartsWith(TEXT("neckale"))
			|| Ad.StartsWith(TEXT("kasaanahtar"));
	}

	bool BoolOku(const UObject* O, const TCHAR* Ad, bool bVars = false)
	{
		if (!O)
		{
			return bVars;
		}
		if (const FBoolProperty* P = FindFProperty<FBoolProperty>(O->GetClass(), Ad))
		{
			return P->GetPropertyValue_InContainer(O);
		}
		return bVars;
	}

	void BoolYaz(UObject* O, const TCHAR* Ad, bool bDeger)
	{
		if (!O)
		{
			return;
		}
		if (const FBoolProperty* P = FindFProperty<FBoolProperty>(O->GetClass(), Ad))
		{
			P->SetPropertyValue_InContainer(O, bDeger);
		}
	}

	FString StrOku(const UObject* O, const TCHAR* Ad)
	{
		if (!O)
		{
			return FString();
		}
		if (const FStrProperty* P = FindFProperty<FStrProperty>(O->GetClass(), Ad))
		{
			return P->GetPropertyValue_InContainer(O);
		}
		return FString();
	}

	void StrYaz(UObject* O, const TCHAR* Ad, const FString& Deger)
	{
		if (!O)
		{
			return;
		}
		if (const FStrProperty* P = FindFProperty<FStrProperty>(O->GetClass(), Ad))
		{
			P->SetPropertyValue_InContainer(O, Deger);
		}
	}

	int32 IntOku(const UObject* O, const TCHAR* Ad, int32 Vars = 0)
	{
		if (!O)
		{
			return Vars;
		}
		if (const FIntProperty* P = FindFProperty<FIntProperty>(O->GetClass(), Ad))
		{
			return P->GetPropertyValue_InContainer(O);
		}
		return Vars;
	}

	void IntYaz(UObject* O, const TCHAR* Ad, int32 Deger)
	{
		if (!O)
		{
			return;
		}
		if (const FIntProperty* P = FindFProperty<FIntProperty>(O->GetClass(), Ad))
		{
			P->SetPropertyValue_InContainer(O, Deger);
		}
	}

	/** Oyuncunun `anahtarcantası` dizisini okur/yazar (yansıma ile). */
	TArray<FString> CantaOku(UObject* O)
	{
		TArray<FString> Sonuc;
		if (!O)
		{
			return Sonuc;
		}
		if (const FArrayProperty* Dizi = FindFProperty<FArrayProperty>(O->GetClass(), TEXT("anahtarcantası")))
		{
			if (const FStrProperty* Eleman = CastField<FStrProperty>(Dizi->Inner))
			{
				FScriptArrayHelper Yardim(Dizi, Dizi->ContainerPtrToValuePtr<void>(O));
				for (int32 i = 0; i < Yardim.Num(); ++i)
				{
					Sonuc.Add(Eleman->GetPropertyValue(Yardim.GetRawPtr(i)));
				}
			}
		}
		return Sonuc;
	}

	void CantaYaz(UObject* O, const TArray<FString>& Degerler)
	{
		if (!O)
		{
			return;
		}
		if (const FArrayProperty* Dizi = FindFProperty<FArrayProperty>(O->GetClass(), TEXT("anahtarcantası")))
		{
			if (const FStrProperty* Eleman = CastField<FStrProperty>(Dizi->Inner))
			{
				FScriptArrayHelper Yardim(Dizi, Dizi->ContainerPtrToValuePtr<void>(O));
				Yardim.Resize(Degerler.Num());
				for (int32 i = 0; i < Degerler.Num(); ++i)
				{
					Eleman->SetPropertyValue(Yardim.GetRawPtr(i), Degerler[i]);
				}
			}
		}
	}

	void NavTazele(AActor* Kapi)
	{
		if (!Kapi)
		{
			return;
		}
		if (UFunction* Fn = Kapi->FindFunction(TEXT("NavDurumGuncelle")))
		{
			Kapi->ProcessEvent(Fn, nullptr);
		}
	}
}

bool UOyunDurumu::VarMi()
{
	return UGameplayStatics::DoesSaveGameExist(GYuva, 0);
}

void UOyunDurumu::Temizle()
{
	if (UGameplayStatics::DoesSaveGameExist(GYuva, 0))
	{
		UGameplayStatics::DeleteGameInSlot(GYuva, 0);
	}
	GUygulaBekliyor = false;
	UE_LOG(LogTemp, Log, TEXT("OyunDurumu: kayit silindi (yeni oyun)"));
}

bool UOyunDurumu::BeklemedeMi()
{
	return GUygulaBekliyor;
}

void UOyunDurumu::ModAyarla(EOyunModu Yeni)
{
	GMod = Yeni;
	UE_LOG(LogTemp, Log, TEXT("OyunDurumu: kip -> %s"),
		Yeni == EOyunModu::Menu ? TEXT("MENU") : TEXT("OYNANIS"));
}

EOyunModu UOyunDurumu::Mod()
{
	return GMod;
}

bool UOyunDurumu::Kaydet(const UObject* DunyaBaglami)
{
	UWorld* W = GEngine ? GEngine->GetWorldFromContextObject(DunyaBaglami, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!W)
	{
		return false;
	}

	UOyunDurumKayit* K = Cast<UOyunDurumKayit>(
		UGameplayStatics::CreateSaveGameObject(UOyunDurumKayit::StaticClass()));
	if (!K)
	{
		return false;
	}

	// PIE'de dünya adı "UEDPIE_0_FirstPersonMap" olabiliyor; öneki atıyoruz.
	FString Harita = W->GetName();
	int32 Yer = INDEX_NONE;
	if (Harita.StartsWith(TEXT("UEDPIE")) && Harita.FindChar(TEXT('_'), Yer))
	{
		Harita = Harita.RightChop(Yer + 1);
		if (Harita.FindChar(TEXT('_'), Yer))
		{
			Harita = Harita.RightChop(Yer + 1);
		}
	}
	K->Harita = Harita;
	K->Zaman = FDateTime::Now();

	// --- oyuncu
	if (APawn* P = UGameplayStatics::GetPlayerPawn(W, 0))
	{
		K->OyuncuKonum = P->GetActorLocation();
		if (APlayerController* PC = Cast<APlayerController>(P->GetController()))
		{
			K->OyuncuBakis = PC->GetControlRotation();
		}
		K->Canta = CantaOku(P);
	}

	// --- kız
	for (TActorIterator<AMyCharacter> It(W); It; ++It)
	{
		K->KizKonum = It->GetActorLocation();
		K->KizMod = StrOku(*It, TEXT("GuncelModAdi"));
		K->KizKalanSure = IntOku(*It, TEXT("kalansure"));
		K->Zorluk = IntOku(*It, TEXT("UygulananZorluk"), -1);
		break;
	}

	// --- dünya
	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* A = *It;
		if (KapiMi(A))
		{
			FKayitliKapi Kayit;
			Kayit.Ad = A->GetName();
			Kayit.bKilidiAcildi = BoolOku(A, TEXT("bKilidiAcildi"));
			Kayit.bMuhurlenmis = BoolOku(A, TEXT("bMuhurlenmis"));
			Kayit.bAcik = BoolOku(A, TEXT("bAcik"));
			K->Kapilar.Add(Kayit);
		}
		else if (KasaMi(A))
		{
			FKayitliKasa Kayit;
			Kayit.Ad = A->GetName();
			Kayit.bAcildiMi = BoolOku(A, TEXT("bAcildiMi"));
			K->Kasalar.Add(Kayit);
		}
		else if (EsyaMi(A))
		{
			FKayitliEsya Kayit;
			Kayit.Ad = A->GetName();
			Kayit.Isim = StrOku(A, TEXT("anahtarısmı"));
			Kayit.Konum = A->GetActorLocation();
			Kayit.bVar = true;
			K->Esyalar.Add(Kayit);
		}
	}

	if (UGameInstance* GI = W->GetGameInstance())
	{
		K->MuhurlenmisOdaSayisi = IntOku(GI, TEXT("MuhurlenmisOdaSayisi"));
	}

	const bool bTamam = UGameplayStatics::SaveGameToSlot(K, GYuva, 0);
	UE_LOG(LogTemp, Log,
		TEXT("OyunDurumu: kaydedildi=%d harita=%s kapi=%d kasa=%d esya=%d canta=%d muhur=%d"),
		bTamam ? 1 : 0, *K->Harita, K->Kapilar.Num(), K->Kasalar.Num(),
		K->Esyalar.Num(), K->Canta.Num(), K->MuhurlenmisOdaSayisi);
	return bTamam;
}

void UOyunDurumu::DevamEt(const UObject* DunyaBaglami)
{
	UWorld* W = GEngine ? GEngine->GetWorldFromContextObject(DunyaBaglami, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!W)
	{
		return;
	}

	UOyunDurumKayit* K = Cast<UOyunDurumKayit>(UGameplayStatics::LoadGameFromSlot(GYuva, 0));
	if (!K)
	{
		UE_LOG(LogTemp, Warning, TEXT("OyunDurumu: kayit yok, devam edilemiyor"));
		return;
	}

	GUygulaBekliyor = true;
	GMod = EOyunModu::Oynanis;   // kayıttan devam = oynanış kipi
	const FName Harita(*(K->Harita.IsEmpty() ? FString(TEXT("FirstPersonMap")) : K->Harita));
	UGameplayStatics::SetGamePaused(W, false);
	UGameplayStatics::OpenLevel(W, Harita);
}

void UOyunDurumu::Uygula(UWorld* W)
{
	GUygulaBekliyor = false;
	if (!W)
	{
		return;
	}

	UOyunDurumKayit* K = Cast<UOyunDurumKayit>(UGameplayStatics::LoadGameFromSlot(GYuva, 0));
	if (!K)
	{
		return;
	}

	// --- kapılar
	TMap<FString, const FKayitliKapi*> KapiHarita;
	for (const FKayitliKapi& Kp : K->Kapilar)
	{
		KapiHarita.Add(Kp.Ad, &Kp);
	}
	TMap<FString, const FKayitliKasa*> KasaHarita;
	for (const FKayitliKasa& Ks : K->Kasalar)
	{
		KasaHarita.Add(Ks.Ad, &Ks);
	}
	TMap<FString, const FKayitliEsya*> EsyaHarita;
	for (const FKayitliEsya& Es : K->Esyalar)
	{
		EsyaHarita.Add(Es.Ad, &Es);
	}

	int32 KapiSayi = 0, KasaSayi = 0, EsyaSayi = 0, SilinenEsya = 0;
	TArray<AActor*> Silinecek;

	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* A = *It;
		const FString Ad = A->GetName();

		if (KapiMi(A))
		{
			if (const FKayitliKapi** Bulunan = KapiHarita.Find(Ad))
			{
				const FKayitliKapi* Kp = *Bulunan;
				BoolYaz(A, TEXT("bKilidiAcildi"), Kp->bKilidiAcildi);
				BoolYaz(A, TEXT("bMuhurlenmis"), Kp->bMuhurlenmis);
				BoolYaz(A, TEXT("bAcik"), Kp->bAcik);
				NavTazele(A);   // navmesh kapının yeni haline uysun
				++KapiSayi;
			}
		}
		else if (KasaMi(A))
		{
			if (const FKayitliKasa** Bulunan = KasaHarita.Find(Ad))
			{
				BoolYaz(A, TEXT("bAcildiMi"), (*Bulunan)->bAcildiMi);
				++KasaSayi;
			}
		}
		else if (EsyaMi(A))
		{
			if (const FKayitliEsya** Bulunan = EsyaHarita.Find(Ad))
			{
				const FKayitliEsya* Es = *Bulunan;
				// Anahtar dağıtımı yeniden rastgele yaptı; kaydı geri yaz.
				StrYaz(A, TEXT("anahtarısmı"), Es->Isim);
				A->SetActorLocation(Es->Konum, false, nullptr, ETeleportType::TeleportPhysics);
				++EsyaSayi;
			}
			else
			{
				// Kayıtta yok = oyuncu toplamıştı.
				Silinecek.Add(A);
			}
		}
	}

	for (AActor* A : Silinecek)
	{
		A->Destroy();
		++SilinenEsya;
	}

	// --- oyuncu
	if (APawn* P = UGameplayStatics::GetPlayerPawn(W, 0))
	{
		P->SetActorLocation(K->OyuncuKonum, false, nullptr, ETeleportType::TeleportPhysics);
		if (APlayerController* PC = Cast<APlayerController>(P->GetController()))
		{
			PC->SetControlRotation(K->OyuncuBakis);
		}
		CantaYaz(P, K->Canta);
	}

	// --- kız
	for (TActorIterator<AMyCharacter> It(W); It; ++It)
	{
		It->SetActorLocation(K->KizKonum, false, nullptr, ETeleportType::TeleportPhysics);
		if (!K->KizMod.IsEmpty())
		{
			StrYaz(*It, TEXT("GuncelModAdi"), K->KizMod);
		}
		IntYaz(*It, TEXT("kalansure"), K->KizKalanSure);
		break;
	}

	if (UGameInstance* GI = W->GetGameInstance())
	{
		IntYaz(GI, TEXT("MuhurlenmisOdaSayisi"), K->MuhurlenmisOdaSayisi);
	}

	UE_LOG(LogTemp, Log,
		TEXT("OyunDurumu: kayit uygulandi — kapi=%d kasa=%d esya=%d (silinen %d), canta=%d, muhur=%d"),
		KapiSayi, KasaSayi, EsyaSayi, SilinenEsya, K->Canta.Num(), K->MuhurlenmisOdaSayisi);
}
