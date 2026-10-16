#include "MahzenAyar.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "AudioDevice.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace MahzenSes
{
	// Ses sınıfları Python tarafında üretiliyor; burada yolla yükleniyor ki
	// varlık yoksa kod yine de sessizce çalışsın.
	static USoundMix* Mikser()
	{
		return LoadObject<USoundMix>(nullptr, TEXT("/Game/Mahzen/Audio/SMix_Mahzen.SMix_Mahzen"));
	}
	static USoundClass* Sinif(const TCHAR* Yol)
	{
		return LoadObject<USoundClass>(nullptr, Yol);
	}
}

UMahzenKayit* UMahzenAyar::Onbellek = nullptr;
const TCHAR* UMahzenAyar::YuvaAdi = TEXT("MahzenAyarlari");

UMahzenKayit* UMahzenAyar::Kayit()
{
	// ÖNEMLİ: Onbellek çıplak bir static işaretçi. UPROPERTY olmadığı için
	// çöp toplayıcı kayıt nesnesini topluyordu; sonraki erişim askıda kalan
	// işaretçiye gidip EXCEPTION_ACCESS_VIOLATION (0xffff...ffff) veriyordu.
	// Belirti: ayar kaydırıcısına dokunduktan bir süre sonra, GC çalışınca
	// editör çöküyordu. Çözüm: nesneyi kök kümeye al ve her erişimde
	// geçerliliğini doğrula.
	if (Onbellek && !IsValid(Onbellek))
	{
		Onbellek = nullptr;
	}
	if (Onbellek)
	{
		return Onbellek;
	}

	if (UGameplayStatics::DoesSaveGameExist(YuvaAdi, 0))
	{
		Onbellek = Cast<UMahzenKayit>(UGameplayStatics::LoadGameFromSlot(YuvaAdi, 0));
	}
	if (!Onbellek)
	{
		Onbellek = Cast<UMahzenKayit>(UGameplayStatics::CreateSaveGameObject(UMahzenKayit::StaticClass()));
	}
	if (Onbellek && !Onbellek->IsRooted())
	{
		Onbellek->AddToRoot();
	}
	return Onbellek;
}

void UMahzenAyar::Kaydet()
{
	if (UMahzenKayit* K = Kayit())
	{
		UGameplayStatics::SaveGameToSlot(K, YuvaAdi, 0);
	}
}

bool UMahzenAyar::IlkAcilisMi()
{
	UMahzenKayit* K = Kayit();
	return K ? !K->bIlkAcilisTamam : true;
}

void UMahzenAyar::IlkAcilisiTamamla()
{
	if (UMahzenKayit* K = Kayit())
	{
		K->bIlkAcilisTamam = true;
		Kaydet();
	}
}

bool UMahzenAyar::OyunBaslatildiMi()
{
	UMahzenKayit* K = Kayit();
	return K ? K->bOyunBaslatildi : false;
}

void UMahzenAyar::OyunBaslatildiIsaretle()
{
	if (UMahzenKayit* K = Kayit())
	{
		K->bOyunBaslatildi = true;
		Kaydet();
	}
}

void UMahzenAyar::GammaAyarla(float Deger)
{
	const float Kelepce = FMath::Clamp(Deger, 1.0f, 3.5f);
	if (GEngine)
	{
		GEngine->DisplayGamma = Kelepce;
	}
	if (UMahzenKayit* K = Kayit())
	{
		K->Gamma = Kelepce;
		Kaydet();
	}
}

float UMahzenAyar::GammaOku()
{
	UMahzenKayit* K = Kayit();
	return K ? K->Gamma : 2.2f;
}

TArray<FString> UMahzenAyar::CozunurlukListesi()
{
	TArray<FString> Liste;
	TArray<FIntPoint> Cozunurlukler;
	if (UKismetSystemLibrary::GetSupportedFullscreenResolutions(Cozunurlukler))
	{
		for (const FIntPoint& C : Cozunurlukler)
		{
			// çok küçük modları listeleme, menüde kalabalık yapıyor
			if (C.X >= 1280)
			{
				Liste.Add(FString::Printf(TEXT("%d x %d"), C.X, C.Y));
			}
		}
	}
	if (Liste.Num() == 0)
	{
		Liste.Add(TEXT("1920 x 1080"));
	}
	return Liste;
}

void UMahzenAyar::CozunurlukAyarla(int32 Genislik, int32 Yukseklik)
{
	if (UGameUserSettings* Ayar = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Ayar->SetScreenResolution(FIntPoint(Genislik, Yukseklik));
		Ayar->ApplyResolutionSettings(false);
		Ayar->SaveSettings();
	}
	if (UMahzenKayit* K = Kayit())
	{
		K->CozunurlukX = Genislik;
		K->CozunurlukY = Yukseklik;
		Kaydet();
	}
}

void UMahzenAyar::CozunurlukSecMetin(const FString& Metin)
{
	// "1920 x 1080" -> 1920, 1080
	FString Sol, Sag;
	if (Metin.Split(TEXT("x"), &Sol, &Sag))
	{
		const int32 G = FCString::Atoi(*Sol.TrimStartAndEnd());
		const int32 Y = FCString::Atoi(*Sag.TrimStartAndEnd());
		if (G > 0 && Y > 0)
		{
			CozunurlukAyarla(G, Y);
		}
	}
}

FString UMahzenAyar::AktifCozunurlukMetni()
{
	UMahzenKayit* K = Kayit();
	const int32 G = K ? K->CozunurlukX : 1920;
	const int32 Y = K ? K->CozunurlukY : 1080;
	return FString::Printf(TEXT("%d x %d"), G, Y);
}

TArray<FString> UMahzenAyar::EkranModuListesi()
{
	return { TEXT("Tam Ekran"), TEXT("Pencereli Tam Ekran"), TEXT("Pencereli") };
}

void UMahzenAyar::EkranModuSecMetin(const FString& Metin)
{
	int32 Mod = 1;
	if (Metin.Equals(TEXT("Tam Ekran"), ESearchCase::IgnoreCase)) Mod = 0;
	else if (Metin.Equals(TEXT("Pencereli"), ESearchCase::IgnoreCase)) Mod = 2;
	EkranModuAyarla(Mod);
}

FString UMahzenAyar::AktifEkranModuMetni()
{
	UMahzenKayit* K = Kayit();
	const int32 Mod = K ? K->EkranModu : 1;
	if (Mod == 0) return TEXT("Tam Ekran");
	if (Mod == 2) return TEXT("Pencereli");
	return TEXT("Pencereli Tam Ekran");
}

void UMahzenAyar::KaliteAyarla(int32 Seviye)
{
	const int32 S = FMath::Clamp(Seviye, 0, 3);
	if (UGameUserSettings* Ayar = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Ayar->SetOverallScalabilityLevel(S);
		Ayar->ApplySettings(false);
		Ayar->SaveSettings();
	}
	if (UMahzenKayit* K = Kayit())
	{
		K->Kalite = S;
		Kaydet();
	}
	// Kalite önayarı motion blur'u ezebiliyor; oyuncunun seçimini geri koy.
	MotionBlurAyarla(Kayit() ? Kayit()->bMotionBlur : true);
}

FString UMahzenAyar::AktifKaliteMetni()
{
	static const TCHAR* Adlar[] = { TEXT("Dusuk"), TEXT("Orta"), TEXT("Yuksek"), TEXT("Epik") };
	UMahzenKayit* K = Kayit();
	const int32 S = FMath::Clamp(K ? K->Kalite : 2, 0, 3);
	return Adlar[S];
}

void UMahzenAyar::KaliteDegistir(int32 Yon, UTextBlock* Hedef)
{
	UMahzenKayit* K = Kayit();
	const int32 Yeni = FMath::Clamp((K ? K->Kalite : 2) + (Yon > 0 ? 1 : -1), 0, 3);
	KaliteAyarla(Yeni);
	if (Hedef)
	{
		Hedef->SetText(FText::FromString(AktifKaliteMetni()));
	}
}

void UMahzenAyar::VSyncAyarla(bool bAcik)
{
	if (UGameUserSettings* Ayar = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Ayar->SetVSyncEnabled(bAcik);
		Ayar->ApplySettings(false);
		Ayar->SaveSettings();
	}
	if (UMahzenKayit* K = Kayit())
	{
		K->bVSync = bAcik;
		Kaydet();
	}
}

void UMahzenAyar::MotionBlurAyarla(bool bAcik)
{
	if (GEngine)
	{
		if (IConsoleVariable* CV = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality")))
		{
			CV->Set(bAcik ? 4 : 0, ECVF_SetByGameSetting);
		}
	}
	if (UMahzenKayit* K = Kayit())
	{
		K->bMotionBlur = bAcik;
		Kaydet();
	}
}

float UMahzenAyar::MuzikSesOku()
{
	UMahzenKayit* K = Kayit();
	return K ? K->MuzikSes : 1.0f;
}

void UMahzenAyar::SesSiniflariniUygula(UObject* DunyaBaglami)
{
	UMahzenKayit* K = Kayit();
	if (!K || !DunyaBaglami)
	{
		return;
	}

	USoundMix* Mikser = MahzenSes::Mikser();
	if (!Mikser)
	{
		return;
	}

	USoundClass* Muzik = MahzenSes::Sinif(TEXT("/Game/Mahzen/Audio/SC_Muzik.SC_Muzik"));
	USoundClass* Efekt = MahzenSes::Sinif(TEXT("/Game/Mahzen/Audio/SC_Efekt.SC_Efekt"));

	UGameplayStatics::PushSoundMixModifier(DunyaBaglami, Mikser);
	if (Muzik)
	{
		UGameplayStatics::SetSoundMixClassOverride(DunyaBaglami, Mikser, Muzik,
			FMath::Clamp(K->MuzikSes, 0.0f, 1.0f), 1.0f, 0.05f, true);
	}
	if (Efekt)
	{
		UGameplayStatics::SetSoundMixClassOverride(DunyaBaglami, Mikser, Efekt,
			FMath::Clamp(K->EfektSes, 0.0f, 1.0f), 1.0f, 0.05f, true);
	}
}

float UMahzenAyar::AnaSesOku()
{
	UMahzenKayit* K = Kayit();
	return K ? K->AnaSes : 1.0f;
}

float UMahzenAyar::EfektSesOku()
{
	UMahzenKayit* K = Kayit();
	return K ? K->EfektSes : 1.0f;
}

void UMahzenAyar::EkranModuAyarla(int32 Mod)
{
	EWindowMode::Type Hedef = EWindowMode::WindowedFullscreen;
	if (Mod == 0) Hedef = EWindowMode::Fullscreen;
	else if (Mod == 2) Hedef = EWindowMode::Windowed;

	if (UGameUserSettings* Ayar = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Ayar->SetFullscreenMode(Hedef);
		Ayar->ApplyResolutionSettings(false);
		Ayar->SaveSettings();
	}
	if (UMahzenKayit* K = Kayit())
	{
		K->EkranModu = Mod;
		Kaydet();
	}
}

void UMahzenAyar::CozunurlukDegistir(int32 Yon, UTextBlock* Hedef)
{
	TArray<FString> Liste = CozunurlukListesi();
	if (Liste.Num() == 0)
	{
		return;
	}
	int32 Sira = Liste.IndexOfByKey(AktifCozunurlukMetni());
	if (Sira == INDEX_NONE)
	{
		Sira = Liste.Num() - 1;
	}
	Sira = (Sira + Yon + Liste.Num()) % Liste.Num();
	CozunurlukSecMetin(Liste[Sira]);
	if (Hedef)
	{
		Hedef->SetText(FText::FromString(Liste[Sira]));
	}
}

void UMahzenAyar::EkranModuDegistir(int32 Yon, UTextBlock* Hedef)
{
	TArray<FString> Liste = EkranModuListesi();
	int32 Sira = Liste.IndexOfByKey(AktifEkranModuMetni());
	if (Sira == INDEX_NONE)
	{
		Sira = 1;
	}
	Sira = (Sira + Yon + Liste.Num()) % Liste.Num();
	EkranModuSecMetin(Liste[Sira]);
	if (Hedef)
	{
		Hedef->SetText(FText::FromString(Liste[Sira]));
	}
}

void UMahzenAyar::AyarEkraniniDoldur(UUserWidget* Ekran)
{
	UMahzenKayit* K = Kayit();
	if (!Ekran || !K)
	{
		return;
	}

	auto KaydiriciAyarla = [Ekran](const TCHAR* Ad, float Deger)
	{
		if (USlider* S = Cast<USlider>(Ekran->GetWidgetFromName(FName(Ad))))
		{
			S->SetValue(Deger);
		}
	};
	auto MetinAyarla = [Ekran](const TCHAR* Ad, const FString& Metin)
	{
		if (UTextBlock* T = Cast<UTextBlock>(Ekran->GetWidgetFromName(FName(Ad))))
		{
			T->SetText(FText::FromString(Metin));
		}
	};

	auto KutuAyarla = [Ekran](const TCHAR* Ad, bool bDeger)
	{
		if (UCheckBox* C = Cast<UCheckBox>(Ekran->GetWidgetFromName(FName(Ad))))
		{
			C->SetIsChecked(bDeger);
		}
	};

	KaydiriciAyarla(TEXT("Sld_Gamma"), K->Gamma);
	KaydiriciAyarla(TEXT("Sld_AnaSes"), K->AnaSes);
	KaydiriciAyarla(TEXT("Sld_MuzikSes"), K->MuzikSes);
	KaydiriciAyarla(TEXT("Sld_EfektSes"), K->EfektSes);
	KaydiriciAyarla(TEXT("Sld_Hassasiyet"), K->FareHassasiyeti);
	MetinAyarla(TEXT("Txt_Cozunurluk"), AktifCozunurlukMetni());
	MetinAyarla(TEXT("Txt_EkranModu"), AktifEkranModuMetni());
	MetinAyarla(TEXT("Txt_Kalite"), AktifKaliteMetni());

	KutuAyarla(TEXT("Chk_TersY"), K->bTersY);
	KutuAyarla(TEXT("Chk_VSync"), K->bVSync);
	KutuAyarla(TEXT("Chk_MotionBlur"), K->bMotionBlur);
}

void UMahzenAyar::AnaMenuyuDoldur(UUserWidget* Menu)
{
	if (!Menu)
	{
		return;
	}
	if (UTextBlock* T = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Txt_Oyna"))))
	{
		T->SetText(FText::FromString(OyunBaslatildiMi() ? TEXT("DEVAM ET") : TEXT("BASLA")));
	}
}

void UMahzenAyar::AnaSesAyarla(float Deger)
{
	UMahzenKayit* K = Kayit();
	SesAyarla(Deger, K ? K->MuzikSes : 1.0f, K ? K->EfektSes : 1.0f);
}

void UMahzenAyar::MuzikSesAyarla(float Deger)
{
	UMahzenKayit* K = Kayit();
	SesAyarla(K ? K->AnaSes : 1.0f, Deger, K ? K->EfektSes : 1.0f);
}

void UMahzenAyar::EfektSesAyarla(float Deger)
{
	UMahzenKayit* K = Kayit();
	SesAyarla(K ? K->AnaSes : 1.0f, K ? K->MuzikSes : 1.0f, Deger);
}

void UMahzenAyar::HassasiyetAyarla(float Deger)
{
	UMahzenKayit* K = Kayit();
	FareAyarla(Deger, K ? K->bTersY : false);
}

void UMahzenAyar::TersYAyarla(bool bAcik)
{
	UMahzenKayit* K = Kayit();
	FareAyarla(K ? K->FareHassasiyeti : 1.0f, bAcik);
}

void UMahzenAyar::SesAyarla(float Ana, float Muzik, float Efekt)
{
	const float A = FMath::Clamp(Ana, 0.0f, 1.0f);
	const float M = FMath::Clamp(Muzik, 0.0f, 1.0f);
	const float E = FMath::Clamp(Efekt, 0.0f, 1.0f);

	// Ana ses motorun birincil seviyesi; müzik ve efekt kendi SoundClass'ları
	// üstünden mikserle ayarlanıyor.
	if (GEngine)
	{
		if (FAudioDeviceHandle Cihaz = GEngine->GetMainAudioDevice())
		{
			Cihaz->SetTransientPrimaryVolume(A);
		}
	}

	if (UMahzenKayit* K = Kayit())
	{
		K->AnaSes = A;
		K->MuzikSes = M;
		K->EfektSes = E;
		Kaydet();
	}

	// Mikser dünya bağlamı ister; açık bir dünya varsa hemen uygula.
	if (GEngine && GEngine->GetWorldContexts().Num() > 0)
	{
		for (const FWorldContext& Baglam : GEngine->GetWorldContexts())
		{
			if (Baglam.World() && (Baglam.WorldType == EWorldType::Game || Baglam.WorldType == EWorldType::PIE))
			{
				SesSiniflariniUygula(Baglam.World());
				break;
			}
		}
	}
}

void UMahzenAyar::FareAyarla(float Hassasiyet, bool bTersY)
{
	if (UMahzenKayit* K = Kayit())
	{
		K->FareHassasiyeti = FMath::Clamp(Hassasiyet, 0.1f, 4.0f);
		K->bTersY = bTersY;
		Kaydet();
	}
}

float UMahzenAyar::FareHassasiyeti()
{
	UMahzenKayit* K = Kayit();
	return K ? K->FareHassasiyeti : 1.0f;
}

bool UMahzenAyar::TersYMi()
{
	UMahzenKayit* K = Kayit();
	return K ? K->bTersY : false;
}

void UMahzenAyar::TumAyarlariUygula(UObject* DunyaBaglami)
{
	UMahzenKayit* K = Kayit();
	if (!K)
	{
		return;
	}

	if (GEngine)
	{
		GEngine->DisplayGamma = FMath::Clamp(K->Gamma, 1.0f, 3.5f);
		if (FAudioDeviceHandle Cihaz = GEngine->GetMainAudioDevice())
		{
			Cihaz->SetTransientPrimaryVolume(FMath::Clamp(K->AnaSes, 0.0f, 1.0f));
		}
		if (IConsoleVariable* CV = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality")))
		{
			CV->Set(K->bMotionBlur ? 4 : 0, ECVF_SetByGameSetting);
		}
	}

	if (DunyaBaglami)
	{
		SesSiniflariniUygula(DunyaBaglami);
	}

	// Çözünürlük/ekran modu yalnızca paketlenmiş oyunda anlamlı; editörde
	// PIE penceresini zorlamamak için atlanıyor.
	if (DunyaBaglami && DunyaBaglami->GetWorld() && !GIsEditor)
	{
		if (UGameUserSettings* Ayar = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			Ayar->SetScreenResolution(FIntPoint(K->CozunurlukX, K->CozunurlukY));
			EWindowMode::Type Hedef = EWindowMode::WindowedFullscreen;
			if (K->EkranModu == 0) Hedef = EWindowMode::Fullscreen;
			else if (K->EkranModu == 2) Hedef = EWindowMode::Windowed;
			Ayar->SetFullscreenMode(Hedef);
			Ayar->SetOverallScalabilityLevel(FMath::Clamp(K->Kalite, 0, 3));
			Ayar->SetVSyncEnabled(K->bVSync);
			Ayar->ApplyResolutionSettings(false);
			Ayar->ApplySettings(false);
		}
	}
}

// ─── TUŞ ATAMALARI ───────────────────────────────────────────────────────────

namespace MahzenTus
{
	// Eylem adı -> fabrika ayarı tuşu. Karakter Blueprint'indeki eski
	// InputKey düğümleriyle birebir aynı: Q al/kullan, F not-kapı, T kasa,
	// E mühürle, I envanter, K fener, G anahtar bırak.
	struct FTanim { const TCHAR* Ad; FKey Tus; const TCHAR* Etiket; };

	static const FTanim Tanimlar[] = {
		{ TEXT("Etkilesim"),    EKeys::Q, TEXT("Al / Kullan") },
		{ TEXT("NotKapi"),      EKeys::F, TEXT("Not oku / Kapı aç") },
		{ TEXT("KasaAc"),       EKeys::T, TEXT("Kasayı aç") },
		{ TEXT("Muhurle"),      EKeys::E, TEXT("Odayı mühürle") },
		{ TEXT("Envanter"),     EKeys::I, TEXT("Envanter") },
		{ TEXT("Fener"),        EKeys::K, TEXT("Fener") },
		{ TEXT("AnahtarBirak"), EKeys::G, TEXT("Anahtar bırak") },
	};

	static const FTanim* Bul(FName Eylem)
	{
		for (const FTanim& T : Tanimlar)
		{
			if (Eylem == FName(T.Ad))
			{
				return &T;
			}
		}
		return nullptr;
	}
}

TArray<FName> UMahzenAyar::TusEylemleri()
{
	TArray<FName> Sonuc;
	for (const MahzenTus::FTanim& T : MahzenTus::Tanimlar)
	{
		Sonuc.Add(FName(T.Ad));
	}
	return Sonuc;
}

FText UMahzenAyar::TusEylemAdi(FName Eylem)
{
	const MahzenTus::FTanim* T = MahzenTus::Bul(Eylem);
	return T ? FText::FromString(T->Etiket) : FText::FromName(Eylem);
}

FKey UMahzenAyar::TusVarsayilan(FName Eylem)
{
	const MahzenTus::FTanim* T = MahzenTus::Bul(Eylem);
	return T ? T->Tus : EKeys::Invalid;
}

FKey UMahzenAyar::TusOku(FName Eylem)
{
	if (const UMahzenKayit* K = Kayit())
	{
		if (const FKey* Kayitli = K->Tuslar.Find(Eylem))
		{
			if (Kayitli->IsValid())
			{
				return *Kayitli;
			}
		}
	}
	return TusVarsayilan(Eylem);
}

void UMahzenAyar::TusYaz(FName Eylem, FKey Tus)
{
	if (!MahzenTus::Bul(Eylem) || !Tus.IsValid())
	{
		return;
	}
	if (UMahzenKayit* K = Kayit())
	{
		K->Tuslar.Add(Eylem, Tus);
		Kaydet();
	}
}

FString UMahzenAyar::TusHarfi(FName Eylem)
{
	const FKey Tus = TusOku(Eylem);
	return Tus.IsValid() ? Tus.GetDisplayName(false).ToString() : FString();
}

FText UMahzenAyar::TusIpucuMetni(FName Eylem, const FString& Metin)
{
	const FString Harf = TusHarfi(Eylem);
	if (Harf.IsEmpty())
	{
		return FText::FromString(Metin);
	}
	return FText::FromString(FString::Printf(TEXT("[%s] %s"), *Harf, *Metin));
}

TArray<FKey> UMahzenAyar::TusSecenekleri()
{
	// Kullanicinin istegi: liste yalnizca bu dort tus. Ayni tus birden fazla
	// eyleme atanabildigi icin 7 eylem 4 tusla da kurulabilir.
	// Not: bir eylemin o anki tusu bu listede yoksa (varsayilanlar T/I/G),
	// UMahzenArayuz::TuslariKur() onu listeye ekler ki secili deger bos
	// gorunmesin; oyuncu baska bir sey sectiginde listeden dusuyor.
	static const FKey Liste[] = {
		EKeys::Q, EKeys::E, EKeys::F, EKeys::K,
	};
	TArray<FKey> Sonuc;
	for (const FKey& Tus : Liste)
	{
		Sonuc.Add(Tus);
	}
	return Sonuc;
}

void UMahzenAyar::TuslariSifirla()
{
	if (UMahzenKayit* K = Kayit())
	{
		K->Tuslar.Empty();
		Kaydet();
	}
}
