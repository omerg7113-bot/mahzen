#include "CctvSistemi.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Scene.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

ACctvSistemi::ACctvSistemi()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Kok = CreateDefaultSubobject<USceneComponent>(TEXT("Kok"));
	SetRootComponent(Kok);

	Yakalayici = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Yakalayici"));
	Yakalayici->SetupAttachment(Kok);

	// Yakalayiciya bagli: kanal degisince onunla birlikte tasinir.
	IRIsik = CreateDefaultSubobject<USpotLightComponent>(TEXT("IRIsik"));
	IRIsik->SetupAttachment(Yakalayici);
	IRIsik->SetVisibility(false);
	IRIsik->SetCastShadows(false);   // bes kameralik golge butcesi yok
	// Ters-kare dusum KAPALI: acikken yakin duvar patlarken koridorun derini
	// kapkara kaliyordu (olculdu). Duz dusumle isik mesafeye yayiliyor.
	IRIsik->bUseInverseSquaredFalloff = false;
	IRIsik->LightFalloffExponent = 1.0f;

	OturmaKamerasi = CreateDefaultSubobject<UCameraComponent>(TEXT("OturmaKamerasi"));
	OturmaKamerasi->SetupAttachment(Kok);
	OturmaKamerasi->bUsePawnControlRotation = false;

	Tetik = CreateDefaultSubobject<UBoxComponent>(TEXT("Tetik"));
	Tetik->SetupAttachment(Kok);
	Tetik->SetBoxExtent(FVector(120.0f, 120.0f, 100.0f));
	Tetik->SetCollisionProfileName(TEXT("Trigger"));
	Tetik->SetGenerateOverlapEvents(true);
}

void ACctvSistemi::BeginPlay()
{
	Super::BeginPlay();

	KameralariTopla();
	YakalayiciyiKur();
	MateryaliKur();
	OturmaKamerasiniYerlestir();
	IRIsiginiAyarla(bIRIsigiAcik);

	if (Kameralar.Num() > 0)
	{
		AktifIndeks = 0;
		YakalayiciyiKameraya(AktifIndeks);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cctv: hic kamera bulunamadi (etiket oneki '%s')"),
			*KameraEtiketOneki.ToString());
	}

	if (Tetik)
	{
		Tetik->OnComponentBeginOverlap.AddDynamic(this, &ACctvSistemi::TetikGirdi);
		Tetik->OnComponentEndOverlap.AddDynamic(this, &ACctvSistemi::TetikCikti);
	}

	TuslariBagla();
	ParametreleriYaz();
}

void ACctvSistemi::EndPlay(const EEndPlayReason::Type Sebep)
{
	YakalamaZamanlayicisi(false);
	Super::EndPlay(Sebep);
}

// --- kurulum ---------------------------------------------------------------

void ACctvSistemi::KameralariTopla()
{
	// Elle doldurulduysa ona dokunma; sadece bos girisleri ayikla.
	Kameralar.RemoveAll([](const TObjectPtr<AActor>& A) { return A == nullptr; });
	if (Kameralar.Num() > 0)
	{
		return;
	}

	UWorld* Dunya = GetWorld();
	if (!Dunya)
	{
		return;
	}

	const FString Onek = KameraEtiketOneki.ToString();

	// Ham isaretci dizisinde topla ve sirala: TObjectPtr dizisini dogrudan
	// siralamak referanstan TObjectPtr kurmaya yol aciyor, o kurucu
	// kullanimdan kaldirildi (C4996).
	TArray<AActor*> Bulunanlar;
	for (TActorIterator<ACameraActor> It(Dunya); It; ++It)
	{
		ACameraActor* Kam = *It;
		if (!Kam)
		{
			continue;
		}
		for (const FName& Etiket : Kam->Tags)
		{
			if (Etiket.ToString().StartsWith(Onek))
			{
				Bulunanlar.Add(Kam);
				break;
			}
		}
	}

	// CCTV_CAM_01, _02 ... sirasi etiketin kendisinden gelsin; aktör
	// yineleyicisinin sirasi garanti degil.
	auto EtiketiAl = [&Onek](const AActor* A) -> FString
	{
		if (A)
		{
			for (const FName& T : A->Tags)
			{
				const FString S = T.ToString();
				if (S.StartsWith(Onek))
				{
					return S;
				}
			}
		}
		return FString();
	};

	Bulunanlar.Sort([&EtiketiAl](const AActor& A, const AActor& B)
	{
		return EtiketiAl(&A) < EtiketiAl(&B);
	});

	Kameralar.Reserve(Bulunanlar.Num());
	for (AActor* Kam : Bulunanlar)
	{
		Kameralar.Add(Kam);
	}

	UE_LOG(LogTemp, Log, TEXT("Cctv: %d kanal bulundu"), Kameralar.Num());
}

void ACctvSistemi::YakalayiciyiKur()
{
	if (!Yakalayici)
	{
		return;
	}

	// Render Target'i calisma zamaninda uret: varlik olarak tutmaya gerek yok,
	// cozunurluk degisince yeniden icerik derlemesi de istemiyoruz.
	// RGBA16f, RGBA8 degil: materyal karanlik pikselleri Kazanc ile buyuttugu
	// icin 8 bit nicelenme gorunur bant/gren uretiyordu.
	Hedef = UKismetRenderingLibrary::CreateRenderTarget2D(this,
		FMath::Max(64, CozunurlukX), FMath::Max(64, CozunurlukY), RTF_RGBA16f);
	if (Hedef)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, Hedef, FLinearColor::Black);
	}

	Yakalayici->TextureTarget = Hedef;
	Yakalayici->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

	// MANUEL yakalama: kare basina degil, zamanlayiciyla.
	Yakalayici->bCaptureEveryFrame = false;
	Yakalayici->bCaptureOnMovement = false;
	// TAA olmadigi icin kare gecmisi tutmanin faydasi yok, bellegi bosa yer.
	Yakalayici->bAlwaysPersistRenderingState = false;

	Yakalayici->MaxViewDistanceOverride = UzakKirpma;

	// Bileseni ebeveynden bagimsiz kil: kanal degisince dunya uzayinda
	// isinlayacagiz, ebeveyn dönüşümü karismasin.
	Yakalayici->SetAbsolute(true, true, false);

	// --- yakalamada KAPATILACAKLAR (Lumen/GI dahil) ---
	// AA: TemporalAA DENENDI VE GERI ALINDI. Manuel yakalamada (bCaptureEveryFrame
	// kapali) TAA'nin kare gecmisi birikmiyor, goruntu kayboluyor: olculdu,
	// ortalama luminans 0.00408 -> 0.00068, yani 6 kat karanlik, sadece en
	// parlak lamba hayatta kaldi. Ekranda yalnizca parazit goruntusu olusuyordu.
	//
	// TAA'siz kenar yumusatmanin yolu SUPER ORNEKLEME: yakalama cozunurlugunu
	// (CozunurlukX/Y) ekranda gorundugunden buyuk tut, materyalin bilineer
	// ornekleme kucultmesi gercek AA isi gorsun. Aktorde 1600x1200 ayarli.
	Yakalayici->ShowFlags.SetAntiAliasing(true);
	Yakalayici->ShowFlags.SetTemporalAA(false);
	Yakalayici->ShowFlags.SetMotionBlur(false);
	Yakalayici->ShowFlags.SetBloom(false);
	Yakalayici->ShowFlags.SetScreenSpaceReflections(false);
	Yakalayici->ShowFlags.SetAmbientOcclusion(false);
	Yakalayici->ShowFlags.SetGlobalIllumination(false);
	Yakalayici->ShowFlags.SetLensFlares(false);
	// EyeAdaptation ACIK olmak ZORUNDA: kapaliyken pozlama gecisi tamamen
	// atlaniyor ve AutoExposureBias hicbir sey yapmiyor (+2 ile +6 arasinda
	// olculdu, iki kare ayni cikti). Metering Manual oldugu icin zamanla
	// adapte olup nefes alma yapmaz.
	Yakalayici->ShowFlags.SetEyeAdaptation(true);
	Yakalayici->ShowFlags.SetVolumetricFog(false);
	Yakalayici->ShowFlags.SetDepthOfField(false);

	// Show flag'ler bazi yollari kesmiyor; post-process ile acikca kapat.
	FPostProcessSettings& PP = Yakalayici->PostProcessSettings;

	PP.bOverride_DynamicGlobalIlluminationMethod = true;
	PP.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;

	PP.bOverride_ReflectionMethod = true;
	PP.ReflectionMethod = EReflectionMethod::None;

	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.0f;

	PP.bOverride_MotionBlurAmount = true;
	PP.MotionBlurAmount = 0.0f;

	PP.bOverride_SceneFringeIntensity = true;
	PP.SceneFringeIntensity = 0.0f;

	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.0f;   // vinyet monitör materyalinde

	// Otomatik pozlama KAPALI kalmali: acik olsaydi kamera karanlik koridordan
	// aydinlik bolgeye bakinca goruntu nefes alirdi. Bias knob'u kaldirildi,
	// hicbir etkisi olmadigi olculdu; parlaklik materyaldeki Kazanc'tan geliyor.
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;

	// Renk yakalamada duz kalsin; yesile cevirmeyi materyal yapiyor.
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
}

void ACctvSistemi::IRIsiginiAyarla(bool bAcik)
{
	bIRIsigiAcik = bAcik;
	if (!IRIsik)
	{
		return;
	}
	IRIsik->SetIntensity(IRSiddeti);
	IRIsik->SetAttenuationRadius(IRMenzili);
	IRIsik->SetOuterConeAngle(IRAcisi);
	IRIsik->SetInnerConeAngle(10.0f);   // genis yumusak gecis, nokta huzme degil
	IRIsik->SetLightColor(IRRengi);
	// Yalnizca-kamerada modunda isik normalde SONUK durur, her yakalamada
	// bir anligina yanar (bkz. Yakala).
	IRIsik->SetVisibility(bAcik && !bIRYalnizcaKamerada);
}

void ACctvSistemi::MateryaliKur()
{
	if (!MonitorAktoru)
	{
		// Etiketten bul.
		if (UWorld* Dunya = GetWorld())
		{
			for (TActorIterator<AActor> It(Dunya); It; ++It)
			{
				if (*It && It->ActorHasTag(MonitorEtiketi))
				{
					MonitorAktoru = *It;
					break;
				}
			}
		}
	}

	if (!MonitorAktoru)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cctv: monitor aktoru bulunamadi ('%s')"), *MonitorEtiketi.ToString());
		return;
	}

	MonitorMesh = MonitorAktoru->FindComponentByClass<UStaticMeshComponent>();
	if (!MonitorMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cctv: monitor aktorunde StaticMeshComponent yok"));
		return;
	}

	if (!EkranMateryali)
	{
		// Faz 3 henuz yazilmadiysa en azindan ham goruntu basilsin diye
		// mevcut materyalden MID uretip dokuyu ona gecirmeyi DENEME:
		// parametresi olmayan materyale yazmak sessizce bir sey yapmaz.
		UE_LOG(LogTemp, Warning, TEXT("Cctv: EkranMateryali bos, CRT materyali (Faz 3) bekleniyor"));
		return;
	}

	EkranMid = UMaterialInstanceDynamic::Create(EkranMateryali, this);
	if (!EkranMid)
	{
		return;
	}

	if (Hedef)
	{
		EkranMid->SetTextureParameterValue(ParamGoruntu, Hedef);
	}

	const int32 Yuva = FMath::Clamp(EkranMateryalYuvasi, 0, FMath::Max(0, MonitorMesh->GetNumMaterials() - 1));
	MonitorMesh->SetMaterial(Yuva, EkranMid);
}

void ACctvSistemi::OturmaKamerasiniYerlestir()
{
	// Bileseni editörde elle tasidiysan dokunmuyoruz; sifirdaysa monitorun
	// onune, ekran yuksekligine kuruyoruz ki kutudan cikar gibi calissin.
	if (!OturmaKamerasi || !MonitorAktoru)
	{
		return;
	}
	if (!OturmaKamerasi->GetRelativeLocation().IsNearlyZero())
	{
		return;
	}

	const FVector Ileri = MonitorAktoru->GetActorForwardVector();
	FVector Merkez = MonitorAktoru->GetActorLocation();

	// Mesh pivotu geometriden kaymis olabilir (old_tv'de 34 birim); gercek
	// sinirin merkezini kullan.
	if (MonitorMesh)
	{
		FVector Orijin, Yaricap;
		MonitorAktoru->GetActorBounds(false, Orijin, Yaricap);
		Merkez = Orijin;
	}

	const FVector Konum = Merkez + Ileri * FMath::Max(10.0f, OturmaMesafesi);
	OturmaKamerasi->SetWorldLocation(Konum);
	OturmaKamerasi->SetWorldRotation((-Ileri).Rotation());
}

void ACctvSistemi::TuslariBagla()
{
	if (!bTuslariBagla)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	// Aktörün kendi InputComponent'i; MyCharacter'a hic dokunmuyoruz.
	EnableInput(PC);
	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(OturmaTusu, IE_Pressed, this, &ACctvSistemi::OturmayiDegistir);
	InputComponent->BindKey(SonrakiKanalTusu, IE_Pressed, this, &ACctvSistemi::TusSonraki);
	InputComponent->BindKey(OncekiKanalTusu, IE_Pressed, this, &ACctvSistemi::TusOnceki);
	InputComponent->BindKey(KalkmaTusu, IE_Pressed, this, &ACctvSistemi::MonitordenKalk);

	// Masaya yaklasana kadar girdi bize gelmesin: ok tuslari oyunun geri
	// kalaninda normal davransin.
	DisableInput(PC);
	bGirdiAcik = false;
}

void ACctvSistemi::GirdiAkisi(bool bAc)
{
	if (bAc == bGirdiAcik)
	{
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}
	if (bAc)
	{
		EnableInput(PC);
	}
	else
	{
		DisableInput(PC);
	}
	bGirdiAcik = bAc;
}

void ACctvSistemi::TetikGirdi(UPrimitiveComponent* /*Bilesen*/, AActor* Diger,
	UPrimitiveComponent* /*DigerBilesen*/, int32 /*Indeks*/, bool /*bSupurmeden*/, const FHitResult& /*Vurus*/)
{
	const APawn* Oyuncu = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Oyuncu || Diger != Oyuncu)
	{
		return;
	}
	bOyuncuYakinda = true;
	GirdiAkisi(true);
}

void ACctvSistemi::TetikCikti(UPrimitiveComponent* /*Bilesen*/, AActor* Diger,
	UPrimitiveComponent* /*DigerBilesen*/, int32 /*Indeks*/)
{
	const APawn* Oyuncu = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Oyuncu || Diger != Oyuncu)
	{
		return;
	}
	bOyuncuYakinda = false;
	// Oturuyorken tetikten cikmis sayilsa bile girdiyi kesme, yoksa kalkamaz.
	if (!bOturuyor)
	{
		GirdiAkisi(false);
	}
}

void ACctvSistemi::FeneriAyarla(bool bKapat)
{
	if (!bFeneriKapat)
	{
		return;
	}

	if (bKapat)
	{
		SondurulenFenerler.Reset();
		APawn* Oyuncu = UGameplayStatics::GetPlayerPawn(this, 0);
		if (!Oyuncu)
		{
			return;
		}
		// Fener oyuncu pawn'inda bir SpotLightComponent (MyCharacter da onu
		// boyle ayirt ediyor). Belirli bir Blueprint'e bagimli kalmamak icin
		// bilesen tipinden buluyoruz.
		TArray<USpotLightComponent*> Fenerler;
		Oyuncu->GetComponents<USpotLightComponent>(Fenerler);
		for (USpotLightComponent* Fener : Fenerler)
		{
			if (Fener && Fener->IsVisible())
			{
				Fener->SetVisibility(false);
				SondurulenFenerler.Add(Fener);
			}
		}
	}
	else
	{
		// Yalnizca bizim sondurduklerimizi geri yak.
		for (const TWeakObjectPtr<USpotLightComponent>& Fener : SondurulenFenerler)
		{
			if (Fener.IsValid())
			{
				Fener->SetVisibility(true);
			}
		}
		SondurulenFenerler.Reset();
	}
}

void ACctvSistemi::OturmayiDegistir()
{
	if (bOturuyor)
	{
		MonitordenKalk();
	}
	else if (bOyuncuYakinda)
	{
		MonitoreOtur();
	}
}

void ACctvSistemi::TusSonraki()
{
	if (bOturuyor)
	{
		SonrakiKanal();
	}
}

void ACctvSistemi::TusOnceki()
{
	if (bOturuyor)
	{
		OncekiKanal();
	}
}

// --- yakalama dongusu ------------------------------------------------------

void ACctvSistemi::YakalamaZamanlayicisi(bool bAc)
{
	if (UWorld* Dunya = GetWorld())
	{
		if (bAc)
		{
			if (!Dunya->GetTimerManager().IsTimerActive(YakalamaTimer))
			{
				const float Aralik = 1.0f / FMath::Max(1.0f, YakalamaFps);
				Dunya->GetTimerManager().SetTimer(YakalamaTimer, this, &ACctvSistemi::Yakala, Aralik, true);
			}
		}
		else
		{
			Dunya->GetTimerManager().ClearTimer(YakalamaTimer);
		}
	}
	bYakalamaAcik = bAc;
}

bool ACctvSistemi::YakalamaGerekliMi() const
{
	if (!bMonitorAcik || Kameralar.Num() == 0 || !Yakalayici || !Hedef)
	{
		return false;
	}
	if (bOturuyor)
	{
		return true;
	}

	// Oturmuyorsa yalnizca oyuncu monitorun yakinindayken yakala.
	const AActor* Ref = MonitorAktoru ? MonitorAktoru.Get() : this;
	if (const APawn* Oyuncu = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return FVector::Dist(Oyuncu->GetActorLocation(), Ref->GetActorLocation()) <= YakalamaMesafesi;
	}
	return false;
}

void ACctvSistemi::Yakala()
{
	// Senkron kaybi / kayit tekrari / kesinti sirasinda GORUNTU DONAR:
	// yeni kare yakalamiyoruz, Render Target'ta son kare kaliyor.
	if (SenkronKalan > 0.0f || DondurmaKalan > 0.0f || KesintiKalan > 0.0f)
	{
		return;
	}
	if (!Yakalayici || !Hedef)
	{
		return;
	}

	// Kizilotesi aydinlatma: yak -> yakala -> sondur. Render komutlari sirayla
	// islendigi icin yakalama isigi gorur, kare sonunda cizilen ana goruntu
	// gormez. Oyuncu karanlikta kalir, kamera her seyi gorur.
	const bool bAnlikIsik = bIRIsigiAcik && bIRYalnizcaKamerada && IRIsik;
	if (bAnlikIsik)
	{
		IRIsik->SetVisibility(true);
	}

	Yakalayici->CaptureScene();

	if (bAnlikIsik)
	{
		IRIsik->SetVisibility(false);
	}

	KanalYazisiniCiz();
}

void ACctvSistemi::KanalYazisiniCiz()
{
	if (!Hedef || !GEngine)
	{
		return;
	}
	if (!bKanalYazisi && !bRecGoster && !bZamanDamgasi)
	{
		return;
	}

	if (!OsdFontu)
	{
		// Monospace: CCTV yazisinin karakteri bundan geliyor.
		OsdFontu = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/DroidSansMono"));
	}
	UFont* YaziTipi = OsdFontu ? OsdFontu.Get() : GEngine->GetLargeFont();
	if (!YaziTipi)
	{
		return;
	}

	UCanvas* Tuval = nullptr;
	FVector2D Boyut = FVector2D::ZeroVector;
	FDrawToRenderTargetContext Baglam;

	// Temizlemeden cizer: az once yakalanan goruntunun USTUNE biner.
	// TEK Begin/End blogu, yakalama basina bir kez (saniyede ~8) -> bedeli yok.
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, Hedef, Tuval, Boyut, Baglam);
	if (Tuval)
	{
		const FLinearColor Golge(0.0f, 0.0f, 0.0f, 0.7f);
		const FVector2D GolgeKay(2.0f, 2.0f);

		auto Yaz = [&](const FString& Metin, const FVector2D& Konum, float Olcek, const FLinearColor& Renk)
		{
			Tuval->K2_DrawText(YaziTipi, Metin, Konum, FVector2D(Olcek, Olcek), Renk,
				0.0f, Golge, GolgeKay, false, false, false, FLinearColor::Black);
		};

		if (bKanalYazisi)
		{
			Yaz(KanalYazisiOneki + FString::FromInt(AktifIndeks + 1),
				KanalYazisiKonumu, KanalYazisiOlcegi, KanalYazisiRengi);
		}

		// REC: sag ust, yanip sonen. Yakalama zaten ~8 fps oldugu icin
		// yanip sonmesi de dogal olarak kekremsi cikiyor.
		if (bRecGoster)
		{
			const float Zaman = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
			const float Periyot = FMath::Max(0.2f, RecYanipSonmePeriyodu);
			if (FMath::Fmod(Zaman, Periyot) < Periyot * 0.55f)
			{
				const float X = FMath::Max(10.0f, Boyut.X - 150.0f);
				Yaz(TEXT("* REC"), FVector2D(X, KanalYazisiKonumu.Y), KanalYazisiOlcegi, RecRengi);
			}
		}

		// Zaman damgasi: sol alt. Sabit tarih + oyun ici saat.
		if (bZamanDamgasi)
		{
			FString Damga;
			if (SabitTarih.IsEmpty())
			{
				Damga = FDateTime::Now().ToString(TEXT("%Y-%m-%d  %H:%M:%S"));
			}
			else
			{
				const float Zaman = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
				const int32 Toplam = FMath::FloorToInt(Zaman);
				Damga = FString::Printf(TEXT("%s  %02d:%02d:%02d"), *SabitTarih,
					(Toplam / 3600) % 24, (Toplam / 60) % 60, Toplam % 60);
			}
			const float Y = FMath::Max(10.0f, Boyut.Y - 60.0f);
			Yaz(Damga, FVector2D(KanalYazisiKonumu.X, Y), ZamanDamgasiOlcegi, KanalYazisiRengi);
		}
	}
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Baglam);
}

void ACctvSistemi::YakalayiciyiKameraya(int32 Indeks)
{
	if (!Yakalayici || !Kameralar.IsValidIndex(Indeks))
	{
		return;
	}

	AActor* Kam = Kameralar[Indeks];
	if (!Kam)
	{
		return;
	}

	// Dunya uzayinda kur; ebeveyne dokunmuyoruz (bileseni yeniden
	// baglamak kalici olmuyor, bu projede bedeli odendi).
	Yakalayici->SetWorldLocationAndRotation(Kam->GetActorLocation(), Kam->GetActorRotation());

	// Kadraji kullanicinin editörde ayarladigi FOV ile ayni tut.
	if (const ACameraActor* KamAktor = Cast<ACameraActor>(Kam))
	{
		if (const UCameraComponent* KamBilesen = KamAktor->GetCameraComponent())
		{
			Yakalayici->FOVAngle = KamBilesen->FieldOfView;
		}
	}
}

// --- kanallar --------------------------------------------------------------

void ACctvSistemi::KanalSec(int32 Kanal)
{
	if (Kameralar.Num() == 0)
	{
		return;
	}

	// Dongusel: 5'ten sonra 1'e doner.
	const int32 Yeni = ((Kanal % Kameralar.Num()) + Kameralar.Num()) % Kameralar.Num();
	AktifIndeks = Yeni;

	YakalayiciyiKameraya(AktifIndeks);

	// Kanal degisimi = senkron kaybi: bu sure boyunca eski kare + parazit.
	SenkronKalan = FMath::Max(0.0f, SenkronKaybiSuresi);

	if (KanalSesi)
	{
		UGameplayStatics::PlaySound2D(this, KanalSesi, KanalSesSiddeti);
	}

	ParametreleriYaz();
}

void ACctvSistemi::SonrakiKanal()
{
	KanalSec(AktifIndeks + 1);
}

void ACctvSistemi::OncekiKanal()
{
	KanalSec(AktifIndeks - 1);
}

AActor* ACctvSistemi::AktifKameraAktoru() const
{
	return Kameralar.IsValidIndex(AktifIndeks) ? Kameralar[AktifIndeks].Get() : nullptr;
}

void ACctvSistemi::MonitoruAcKapa(bool bAcik)
{
	bMonitorAcik = bAcik;
	if (!bAcik)
	{
		// Monitör kapaliyken yakalama tamamen dursun: GPU'da bedeli sifir olsun.
		YakalamaZamanlayicisi(false);
		if (Hedef)
		{
			UKismetRenderingLibrary::ClearRenderTarget2D(this, Hedef, FLinearColor::Black);
		}
	}
	ParametreleriYaz();
}

// --- senaryolu numaralar ---------------------------------------------------

bool ACctvSistemi::KayitTekrariniOynat(float Sure)
{
	if (TekrarKullanildi >= MaksTekrar)
	{
		return false;
	}
	++TekrarKullanildi;
	DondurmaKalan = FMath::Max(0.1f, Sure);
	return true;
}

bool ACctvSistemi::KesintiBaslat(float Sure)
{
	if (TekrarKullanildi >= MaksTekrar)
	{
		return false;
	}
	++TekrarKullanildi;
	KesintiKalan = FMath::Max(0.1f, Sure);
	return true;
}

// --- oturma ----------------------------------------------------------------

void ACctvSistemi::MonitoreOtur()
{
	if (bOturuyor)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	bOturuyor = true;

	PC->SetViewTargetWithBlend(this, FMath::Max(0.0f, GecisSuresi), EViewTargetBlendFunction::VTBlend_Cubic);

	// Hareket/bakis kilitli ama girdi akisi acik kalsin ki kanal tuslari gelsin.
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);

	GirdiAkisi(true);
	FeneriAyarla(true);

	YakalamaZamanlayicisi(true);
	SenkronKalan = FMath::Max(0.0f, SenkronKaybiSuresi);
	ParametreleriYaz();
}

void ACctvSistemi::MonitordenKalk()
{
	if (!bOturuyor)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	bOturuyor = false;

	if (PC)
	{
		if (APawn* Oyuncu = PC->GetPawn())
		{
			PC->SetViewTargetWithBlend(Oyuncu, FMath::Max(0.0f, GecisSuresi), EViewTargetBlendFunction::VTBlend_Cubic);
		}
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
	FeneriAyarla(false);

	// Masanin basinda duruyorsa girdi acik kalsin: F ile tekrar oturabilsin.
	GirdiAkisi(bOyuncuYakinda);

	ParametreleriYaz();
}

// --- tick / parametreler ---------------------------------------------------

void ACctvSistemi::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SenkronKalan > 0.0f)  { SenkronKalan  = FMath::Max(0.0f, SenkronKalan  - DeltaTime); }
	if (DondurmaKalan > 0.0f) { DondurmaKalan = FMath::Max(0.0f, DondurmaKalan - DeltaTime); }
	if (KesintiKalan > 0.0f)  { KesintiKalan  = FMath::Max(0.0f, KesintiKalan  - DeltaTime); }

	// Yakalama yalnizca gerektiginde donsun (monitör kapali veya oyuncu uzak
	// oldugunda GPU'da hic is yapmasin).
	const bool bGerekli = YakalamaGerekliMi();
	if (bGerekli != bYakalamaAcik)
	{
		YakalamaZamanlayicisi(bGerekli);
	}

	ParametreleriYaz();
}

float ACctvSistemi::ParazitSiddeti() const
{
	if (!bMonitorAcik)
	{
		return 0.0f;
	}
	if (KesintiKalan > 0.0f)
	{
		return 1.0f;
	}
	if (SenkronKalan > 0.0f)
	{
		// Senkron kaybi sonuna dogru yumusakca taban parazite insin.
		const float Oran = SenkronKaybiSuresi > KINDA_SMALL_NUMBER ? (SenkronKalan / SenkronKaybiSuresi) : 0.0f;
		return FMath::Lerp(TabanParazit, SenkronParaziti, FMath::Clamp(Oran, 0.0f, 1.0f));
	}
	if (DondurmaKalan > 0.0f)
	{
		return FMath::Clamp(TabanParazit * 2.0f, 0.0f, 1.0f);
	}
	return TabanParazit;
}

void ACctvSistemi::ParametreleriYaz()
{
	if (!EkranMid)
	{
		return;
	}

	EkranMid->SetScalarParameterValue(ParamParazit, ParazitSiddeti());
	EkranMid->SetScalarParameterValue(ParamSenkron, SenkronKalan > 0.0f ? 1.0f : 0.0f);
	EkranMid->SetScalarParameterValue(ParamKanal, static_cast<float>(AktifIndeks + 1));
	EkranMid->SetScalarParameterValue(ParamAcik, bMonitorAcik ? 1.0f : 0.0f);
}

// --- dunya baglamli sarmalayicilar -----------------------------------------

ACctvSistemi* ACctvSistemi::Bul(UObject* DunyaBaglami)
{
	if (!DunyaBaglami)
	{
		return nullptr;
	}
	AActor* Bulunan = UGameplayStatics::GetActorOfClass(DunyaBaglami, ACctvSistemi::StaticClass());
	return Cast<ACctvSistemi>(Bulunan);
}

void ACctvSistemi::CctvOtur(UObject* DunyaBaglami)
{
	if (ACctvSistemi* S = Bul(DunyaBaglami)) { S->MonitoreOtur(); }
}

void ACctvSistemi::CctvKalk(UObject* DunyaBaglami)
{
	if (ACctvSistemi* S = Bul(DunyaBaglami)) { S->MonitordenKalk(); }
}

void ACctvSistemi::CctvSonrakiKanal(UObject* DunyaBaglami)
{
	if (ACctvSistemi* S = Bul(DunyaBaglami)) { S->SonrakiKanal(); }
}

void ACctvSistemi::CctvKayitTekrari(UObject* DunyaBaglami, float Sure)
{
	if (ACctvSistemi* S = Bul(DunyaBaglami)) { S->KayitTekrariniOynat(Sure); }
}

void ACctvSistemi::CctvKesinti(UObject* DunyaBaglami, float Sure)
{
	if (ACctvSistemi* S = Bul(DunyaBaglami)) { S->KesintiBaslat(Sure); }
}
