
// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "NavigationSystem.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/GameViewportClient.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ArrowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequenceBase.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "MahzenAyar.h"
#include "OyunSonu.h"

AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Yere düştükten sonra kan lekeleri ekranla birlikte yavaşça sönsün.
	if (bKarartiyor)
	{
		KarartmaGecen += DeltaTime;
		const float KararmaSure = FMath::Max(0.05f, KararmaSuresiToplam);
		const float KararmaAlfa = FMath::Clamp(KarartmaGecen / KararmaSure, 0.0f, 1.0f);

		if (KanWidgetOrnegi)
		{
			KanWidgetOrnegi->SetRenderOpacity(1.0f - KararmaAlfa);
		}

		if (KararmaAlfa >= 1.0f)
		{
			bKarartiyor = false;
		}
	}

	// Jumpscare sürüşü: yüz kameraya saldırır, sonra kamera sallanır.
	if (bJumpscareAktif)
	{
		JumpscareGecen += DeltaTime;

		// 1) Beliriş → kolların kavraması → ani çekme.
		const float Mesafe = MesafeHesapla(JumpscareGecen);

		// 2) Sallama: kamera BİLEŞENİNİ sars. Mesh bu bileşenin altında olduğu için
		// yüz ekranda sabit kalır, sarsılan dünya olur — bizi o sallıyormuş gibi.
		UCameraComponent* KorkuKam = KorkuKamera.Get();
		if (KorkuKam && bSallama)
		{
			const float Oran = FMath::Clamp(JumpscareGecen / FMath::Max(0.05f, JumpscareSuresi), 0.0f, 1.0f);
			const float Sonme = FMath::Lerp(1.0f, FMath::Clamp(1.0f - SallamaSonmeOrani, 0.0f, 1.0f), Oran);
			const float W = 2.0f * PI * SallamaFrekans * JumpscareGecen;

			// Üç eksende farklı frekans: düzenli salınım yerine çırpınma hissi.
			const FRotator SallamaRot(
				SallamaAci * 0.8f * FMath::Sin(W * 1.31f + 0.7f) * Sonme,
				SallamaAci * 0.6f * FMath::Sin(W * 0.87f + 2.1f) * Sonme,
				SallamaAci * FMath::Sin(W) * Sonme);

			const FVector SallamaKay(
				SallamaKayma * 0.4f * FMath::Sin(W * 1.70f) * Sonme,
				SallamaKayma * FMath::Sin(W * 1.13f + 1.3f) * Sonme,
				SallamaKayma * 0.7f * FMath::Sin(W * 1.47f) * Sonme);

			KorkuKam->SetRelativeRotation(KameraTabanRot + SallamaRot);
			KorkuKam->SetRelativeLocation(KameraTabanKonum + SallamaKay);
		}

		// 3) Bağırma hissi: yüzün kendi yüksek frekanslı titremesi (çene yok, titreme var).
		const float WT = 2.0f * PI * TitremeFrekans * JumpscareGecen;
		const FRotator Titreme(
			TitremeAci * 0.6f * FMath::Sin(WT * 1.09f),
			TitremeAci * FMath::Sin(WT),
			TitremeAci * 0.8f * FMath::Sin(WT * 0.91f));

		// 3a) Çekme anında ağzı açık mesh'e geç (tek kare, sarsıntı içinde görünmez).
		if (!bMeshDegisti && JumpscareGecen >= MeshDegisimAni)
		{
			bMeshDegisti = true;
			if (USkeletalMeshComponent* Korku = KorkuMesh.Get())
			{
				if (KorkuMeshVarligi)
				{
					Korku->SetSkeletalMeshAsset(KorkuMeshVarligi);
					Korku->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
				}
			}
		}

		// 3b) Ağzı aç ve bağırırken nabız gibi titret (Blender'da açılan morph target).
		if (USkeletalMeshComponent* Korku = bMeshDegisti ? KorkuMesh.Get() : nullptr)
		{
			const float AgizGecen = JumpscareGecen - AgizGecikmesi;
			const float Acilma = FMath::Clamp(AgizGecen / FMath::Max(0.01f, AgizAcilmaSuresi), 0.0f, 1.0f);
			const float Nabiz = 1.0f - AgizTitresimMiktari * 0.5f *
				(1.0f - FMath::Cos(2.0f * PI * AgizTitresimFrekans * JumpscareGecen));
			Korku->SetMorphTarget(AgizMorphAdi, FMath::Clamp(AgizAcikligi * Acilma * Nabiz, 0.0f, 1.0f));
		}

		KorkuMeshiYerlestir(Mesafe, Titreme);

		// Odak yüzle birlikte gelsin, yoksa yaklaşırken yüz bulanıklaşır.
		if (bBlurAktif && KorkuKam)
		{
			KorkuKam->PostProcessSettings.DepthOfFieldFocalDistance = Mesafe;
		}

		// 4) Işıklar kamerayla birlikte gelsin (bileşen kameraya bağlı ama
		// sallanma sırasında dünya konumunu her kare tazelemek daha güvenli).
		if (KorkuKam)
		{
			const FRotator KRot = KorkuKam->GetComponentRotation();
			const FVector Ileri = KRot.Vector();
			const FVector Yukari = FRotationMatrix(KRot).GetUnitAxis(EAxis::Z);
			if (KanIsigi)
			{
				KanIsigi->SetWorldLocation(KorkuKam->GetComponentLocation() + Ileri * KanIsikOn + Yukari * KanIsikDikey);
			}
			if (MorgIsigi)
			{
				MorgIsigi->SetWorldLocation(KorkuKam->GetComponentLocation() + Ileri * MorgIsikOn + Yukari * MorgIsikDikey);
			}
		}

		// 4b) Etkileşim ipucu ("[F] Aç: ...") ve diğer HUD widget'ları jumpscare'de
		// ekranda kalmasın. Her kare süpürülüyor çünkü ipucu widget'ı BPC_EtkilesimVurgu
		// tarafından tekrar tekrar yaratılabiliyor; tek seferlik kaldırma yetmez.
		{
			TArray<UUserWidget*> Widgetlar;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgetlar, UUserWidget::StaticClass(), false);
			for (UUserWidget* W : Widgetlar)
			{
				if (W && W != KanWidgetOrnegi && W->IsInViewport())
				{
					W->RemoveFromParent();
				}
			}
		}

		// 5) Çığlık, çekme anında patlasın.
		if (!bCiglikCaldi && CiglikSesi && JumpscareGecen >= CiglikGecikmesi)
		{
			bCiglikCaldi = true;
			UGameplayStatics::PlaySound2D(this, CiglikSesi, CiglikSesSiddeti);
		}
	}

	if (!bDusuyor)
	{
		return;
	}

	UCameraComponent* Kamera = DusenKamera.Get();
	if (!Kamera)
	{
		bDusuyor = false;
		OlumEkraniGoster();
		return;
	}

	DusmeGecen += DeltaTime;
	const float Sure = FMath::Max(0.05f, DusmeSuresi);
	const float Alfa = FMath::Clamp(DusmeGecen / Sure, 0.0f, 1.0f);

	// Hızlanarak devrilsin, yere yakın yumuşasın.
	const float Egri = FMath::InterpEaseInOut(0.0f, 1.0f, Alfa, 2.0f);

	FRotator YeniRot = DusmeBaslangicRot;
	YeniRot.Roll = DusmeBaslangicRot.Roll + DusmeRoll * Egri;
	YeniRot.Pitch = DusmeBaslangicRot.Pitch + DusmePitch * Egri;
	Kamera->SetRelativeRotation(YeniRot);

	FVector YeniKonum = DusmeBaslangicKonum;
	YeniKonum.Z = DusmeBaslangicKonum.Z - DusmeYuksekligi * Egri;
	YeniKonum.Y = DusmeBaslangicKonum.Y - DusmeYanKayma * Egri;
	Kamera->SetRelativeLocation(YeniKonum);

	if (Alfa >= 1.0f)
	{
		bDusuyor = false;
		OlumEkraniGoster();
	}
}

void AMyCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// ================= Bu davranışlar artık Blueprint'te (kızyurume) =================
// Sayacim / ModHesapla / agresif / takipci / hayalet / *HareketEt / MesafeKontrolEt
// fonksiyonlarının içi bilerek boşaltıldı; mantık Blueprint Event Graph'ına taşındı.
// Widget gösterme ve ölüm ekranı native tarafta kalmaya devam ediyor.

void AMyCharacter::Sayacim()
{
}

void AMyCharacter::ModHesapla(Ekaraktermodlari& secilenmod)
{
}

void AMyCharacter::agresif()
{
}

void AMyCharacter::AgresifHareketEt()
{
}

void AMyCharacter::takipci()
{
}

void AMyCharacter::TakipciHareketEt()
{
}

void AMyCharacter::hayalet()
{
}

void AMyCharacter::HayaletHareketEt()
{
}

// ================= WIDGET (Mod Yazısı) =================

void AMyCharacter::WidgetKaldir()
{
	if (AktifWidget)
	{
		AktifWidget->RemoveFromParent();
		AktifWidget = nullptr;
	}
}

void AMyCharacter::WidgetGoster(const FString& Metin)
{
	if (!KizYurume) return;

	if (!AktifWidget)
	{
		AktifWidget = CreateWidget<UUserWidget>(GetWorld(), KizYurume);
		if (AktifWidget)
		{
			AktifWidget->AddToViewport();
		}
	}

	if (AktifWidget)
	{
		if (UTextBlock* TextBlock = Cast<UTextBlock>(AktifWidget->GetWidgetFromName(TEXT("TextBlock_0"))))
		{
			TextBlock->SetText(FText::FromString(Metin));
		}
	}
}

// ================= JUMPSCARE =================
// Kızın aktörünü kameraya taşımak yerine oyuncunun kamerasında hazır duran
// korkunoktası rig'ini kullanır: mesh zaten kameranın çocuğu olduğu için
// sarsıntıyla birlikte hareket eder ve hareket bileşeni / AI ile çakışmaz.

USkeletalMeshComponent* AMyCharacter::KorkuMeshiBul(APawn* OyuncuPawn) const
{
	if (!OyuncuPawn) return nullptr;

	TArray<USkeletalMeshComponent*> Meshler;
	OyuncuPawn->GetComponents<USkeletalMeshComponent>(Meshler);

	// 1. tercih: bir ArrowComponent'e bağlı olan (korkunoktası rig'i).
	for (USkeletalMeshComponent* Bilesen : Meshler)
	{
		if (Bilesen && Bilesen->GetSkeletalMeshAsset() && Cast<UArrowComponent>(Bilesen->GetAttachParent()))
		{
			return Bilesen;
		}
	}

	// 2. tercih: ada göre.
	const FString Aranan = KorkuMeshAdi.ToString();
	for (USkeletalMeshComponent* Bilesen : Meshler)
	{
		if (Bilesen && Bilesen->GetSkeletalMeshAsset() && Bilesen->GetName().Contains(Aranan))
		{
			return Bilesen;
		}
	}

	return nullptr;
}

FVector AMyCharacter::KafaKemigiYerelKonum(USkeletalMeshComponent* Bilesen) const
{
	if (!Bilesen) return FVector::ZeroVector;

	USkeletalMesh* Varlik = Bilesen->GetSkeletalMeshAsset();
	if (!Varlik) return FVector::ZeroVector;

	const FReferenceSkeleton& RefIskelet = Varlik->GetRefSkeleton();
	int32 Index = RefIskelet.FindBoneIndex(KafaKemigiAdi);

	// Kafa kemiği yoksa mesh'in üst sınırına yaklaş.
	if (Index == INDEX_NONE)
	{
		return FVector(0.0f, 0.0f, Varlik->GetBounds().BoxExtent.Z * 1.6f);
	}

	// Kemikten köke yürüyerek bileşen-uzayı dönüşümünü biriktir.
	const TArray<FTransform>& Pozlar = RefIskelet.GetRefBonePose();
	FTransform Birikim = FTransform::Identity;
	while (Pozlar.IsValidIndex(Index))
	{
		Birikim = Birikim * Pozlar[Index];
		Index = RefIskelet.GetParentIndex(Index);
		if (Index == INDEX_NONE) break;
	}

	return Birikim.GetLocation();
}

float AMyCharacter::MesafeHesapla(float Gecen) const
{
	// Faz 1: kız burnumuzun dibinde belirir ve kolların kavradığı mesafeye oturur.
	const float Baslangic = FMath::Max(YuzBaslangicMesafesi, YuzKavramaMesafesi);
	const float YapismaAlfa = FMath::Clamp(Gecen / FMath::Max(0.01f, YapismaSuresi), 0.0f, 1.0f);
	const float Mesafe1 = FMath::Lerp(Baslangic, YuzKavramaMesafesi,
		FMath::InterpEaseOut(0.0f, 1.0f, YapismaAlfa, 2.0f));

	// Faz 2: kollar kenetlendikten sonra bizi kendine çeker — ivmelenerek.
	const float CekmeGecen = Gecen - CekmeGecikmesi;
	if (CekmeGecen <= 0.0f)
	{
		return Mesafe1;
	}

	const float CekmeAlfa = FMath::Clamp(CekmeGecen / FMath::Max(0.01f, CekmeSuresi), 0.0f, 1.0f);
	const float Egri = FMath::InterpEaseIn(0.0f, 1.0f, CekmeAlfa, FMath::Max(1.0f, CekmeIvmesi));
	const float Mesafe2 = FMath::Lerp(Mesafe1, YuzMesafesi - CekmeTasmasi, Egri);

	// Çarpma sonrası taşmayı geri yayla (sertçe çarpıp yerine oturma hissi).
	if (CekmeAlfa >= 1.0f)
	{
		const float Yay = FMath::Clamp((CekmeGecen - CekmeSuresi) / FMath::Max(0.01f, CekmeSuresi), 0.0f, 1.0f);
		return FMath::Lerp(YuzMesafesi - CekmeTasmasi, YuzMesafesi, FMath::InterpEaseOut(0.0f, 1.0f, Yay, 2.0f));
	}

	return Mesafe2;
}

UPointLightComponent* AMyCharacter::IsikYarat(USceneComponent* Ebeveyn, const FLinearColor& Renk,
	float Siddet, float Yaricap)
{
	UPointLightComponent* Isik = NewObject<UPointLightComponent>(Ebeveyn->GetOwner());
	if (!Isik) return nullptr;

	Isik->SetupAttachment(Ebeveyn);
	Isik->RegisterComponent();
	Isik->AttachToComponent(Ebeveyn, FAttachmentTransformRules::KeepRelativeTransform);
	Isik->SetLightColor(Renk);
	Isik->SetIntensity(Siddet);
	Isik->SetAttenuationRadius(Yaricap);
	Isik->SetCastShadows(true);
	Isik->SetMobility(EComponentMobility::Movable);
	Isik->SetVisibility(true);
	return Isik;
}

void AMyCharacter::JumpscareIsiklariniKaldir()
{
	if (KanIsigi)
	{
		KanIsigi->DestroyComponent();
		KanIsigi = nullptr;
	}
	if (MorgIsigi)
	{
		MorgIsigi->DestroyComponent();
		MorgIsigi = nullptr;
	}
}

void AMyCharacter::KorkuMeshiYerlestir(float Mesafe, const FRotator& EkRot)
{
	USkeletalMeshComponent* Korku = KorkuMesh.Get();
	UCameraComponent* Kamera = KorkuKamera.Get();
	if (!Korku || !Kamera) return;

	const FVector KamKonum = Kamera->GetComponentLocation();
	const FRotator KamRot = Kamera->GetComponentRotation();

	// Kameraya göre istenen duruş + titreme.
	const FRotator HedefRot = UKismetMathLibrary::ComposeRotators(
		FRotator(YuzPitch + EkRot.Pitch, YuzYaw + EkRot.Yaw, EkRot.Roll), KamRot);

	// Kafanın gitmesi gereken dünya noktası.
	const FVector HedefKafa = KamKonum
		+ KamRot.Vector() * Mesafe
		+ FRotationMatrix(KamRot).GetUnitAxis(EAxis::Z) * YuzYukseklikOfseti;

	// Kafanın bileşen-uzayı konumu: animasyon oynarken referans poz yanıltır
	// (uzanma animasyonu kökü de kaydırıyor), bu yüzden önce canlı poza bakılır.
	const FVector Olcek(OlcekCarpani);
	FVector KafaYerel = KafaKemigiYerelKonum(Korku);
	if (Korku->DoesSocketExist(KafaKemigiAdi))
	{
		KafaYerel = Korku->GetSocketTransform(KafaKemigiAdi, RTS_Component).GetLocation();
	}

	Korku->SetWorldScale3D(Olcek);
	Korku->SetWorldRotation(HedefRot);
	Korku->SetWorldLocation(HedefKafa - HedefRot.RotateVector(KafaYerel * Olcek));
}

void AMyCharacter::JumpscareBaslat()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	APawn* OyuncuPawn = PC->GetPawn();
	if (!OyuncuPawn) return;

	UCameraComponent* Kamera = OyuncuPawn->FindComponentByClass<UCameraComponent>();
	if (!Kamera) return;

	// 1) Gerçek kızı sahneden çek ve beynini durdur; iki kız görünmesin.
	SetActorHiddenInGame(true);
	if (UCharacterMovementComponent* Hareket = GetCharacterMovement())
	{
		Hareket->StopMovementImmediately();
		Hareket->DisableMovement();
	}
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
		if (AI->BrainComponent)
		{
			AI->BrainComponent->StopLogic(TEXT("Jumpscare"));
		}
	}

	// 2) Korku mesh'inin kafasını kadraja oturt.
	// NOT: Bileşeni kameraya yeniden bağlamak (AttachToComponent) kalıcı olmuyor —
	// bir kare sonra SCS'teki ebeveynine (korkunoktası) geri dönüyor. Bu yüzden
	// ebeveyne hiç dokunmuyoruz; hedefi doğrudan DÜNYA uzayında kuruyoruz.
	// korkunoktası zaten kameranın çocuğu olduğu için mesh kamerayı takip etmeye devam eder.
	if (USkeletalMeshComponent* Korku = KorkuMeshiBul(OyuncuPawn))
	{
		// Önce AĞZI KAPALI mesh (varsa): kız ağzı kapalı belirsin, ağız ancak
		// çekme anında açılsın. Yüzde kalıcı delik olduğu için kapalı ağız
		// morph'la değil, ayrı mesh'le sağlanıyor.
		if (KorkuMeshKapali)
		{
			Korku->SetSkeletalMeshAsset(KorkuMeshKapali);
		}
		else if (KorkuMeshVarligi)
		{
			Korku->SetSkeletalMeshAsset(KorkuMeshVarligi);
		}
		bMeshDegisti = (KorkuMeshKapali == nullptr);

		// Gizliyken/kadraj dışındayken de poz işlensin: kafa kemiğinin canlı konumunu
		// her kare okuyacağız, bayat poz yüzü yanlış yere oturtur.
		Korku->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

		// Kolların kameraya uzanma animasyonu.
		UAnimSequenceBase* Anim = KorkuAnimasyonu
			? KorkuAnimasyonu.Get()
			: Cast<UAnimSequenceBase>(Korku->AnimationData.AnimToPlay.Get());
		if (Anim)
		{
			Korku->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			Korku->SetAnimation(Anim);
			Korku->SetPlayRate(KorkuAnimHizi);
			Korku->SetPosition(FMath::Clamp(KorkuAnimBaslangici, 0.0f, Anim->GetPlayLength()));
			if (bKorkuAnimDondur)
			{
				Korku->Stop();
			}
			else
			{
				Korku->Play(false);
			}
		}

		KorkuMesh = Korku;
		KorkuKamera = Kamera;

		// Kamerayı bu andan sonra biz süreceğiz (sallama bileşen üzerinden olacak).
		// Kontrolcü dönüşü kapanırken görüntü zıplamasın diye dünya dönüşünü koruyoruz.
		const FRotator KamDunyaRot = Kamera->GetComponentRotation();
		Kamera->bUsePawnControlRotation = false;
		Kamera->SetWorldRotation(KamDunyaRot);
		KameraTabanRot = Kamera->GetRelativeRotation();
		KameraTabanKonum = Kamera->GetRelativeLocation();

		// İlk kare: kız doğrudan burnumuzun dibinde belirir.
		KorkuMeshiYerlestir(MesafeHesapla(0.0f), FRotator::ZeroRotator);

		Korku->SetHiddenInGame(false, true);
		Korku->SetVisibility(true, true);

		JumpscareGecen = 0.0f;
		bCiglikCaldi = false;
		bJumpscareAktif = true;
		bMeshDegisti = (KorkuMeshKapali == nullptr);

		// Çift ışık: çene altından kan kırmızısı anahtar + kamera hizasından
		// soğuk morg mavisi kontur (dişlerin sivriliğini parlatır).
		if (bJumpscareIsiklari)
		{
			JumpscareIsiklariniKaldir();
			KanIsigi = IsikYarat(Kamera, KanIsikRengi, KanIsikSiddeti, KanIsikYaricapi);
			MorgIsigi = IsikYarat(Kamera, MorgIsikRengi, MorgIsikSiddeti, MorgIsikYaricapi);
		}
	}

	// Kafamızı tutuyor: bakışı ve hareketi kilitle, kurtulamayalım.
	if (bBakisiKilitle)
	{
		PC->SetIgnoreLookInput(true);
		PC->SetIgnoreMoveInput(true);
	}

	// 2b) Yüzü aydınlat — koridor karanlık olduğunda jumpscare siyah bir lekeye dönüşüyor.
	if (bIsikTakviyesi)
	{
		TArray<UPointLightComponent*> Isiklar;
		OyuncuPawn->GetComponents<UPointLightComponent>(Isiklar);
		for (UPointLightComponent* Isik : Isiklar)
		{
			// Spot ışık (fener) hariç; sadece noktasal dolgu ışığını kullan.
			if (Isik && !Isik->IsA<USpotLightComponent>())
			{
				const FRotator KamRot2 = Kamera->GetComponentRotation();
				Isik->SetWorldLocation(Kamera->GetComponentLocation()
					+ KamRot2.Vector() * IsikOnMesafe
					+ FRotationMatrix(KamRot2).GetUnitAxis(EAxis::Z) * IsikDikeyOfset);
				Isik->SetIntensity(IsikYogunlugu);
				Isik->SetAttenuationRadius(IsikYaricapi);
				Isik->SetVisibility(true);
			}
		}
	}

	// 3) Kamera sarsıntısı.
	if (SarsintiSinifi)
	{
		PC->ClientStartCameraShake(SarsintiSinifi, SarsintiSiddeti);
	}

	// 4) Çığlık burada DEĞİL: Tick içinde CiglikGecikmesi dolunca, yani kollar
	// bizi kendine çektiği anda patlıyor.

	// 5) Arka planı bulanıklaştır; odak yüzün mesafesinde olduğu için yüz net kalır.
	if (bBlurAktif)
	{
		// MinFstop'a dokunulmuyor: 0'a sabitlemek diyafram simülasyonunu bozup
		// odak düzlemindeki yüzü de bulanıklaştırıyordu.
		FPostProcessSettings& PP = Kamera->PostProcessSettings;
		PP.bOverride_DepthOfFieldFocalDistance = true;
		PP.DepthOfFieldFocalDistance = YuzMesafesi;
		PP.bOverride_DepthOfFieldFstop = true;
		PP.DepthOfFieldFstop = BlurFStop;
		PP.bOverride_DepthOfFieldSensorWidth = true;
		PP.DepthOfFieldSensorWidth = BlurSensorGenisligi;
	}

	// 5b) Ambiyansı karart.
	if (!FMath::IsNearlyZero(PozlamaDusurme))
	{
		FPostProcessSettings& PP = Kamera->PostProcessSettings;
		PP.bOverride_AutoExposureBias = true;
		PP.AutoExposureBias = PozlamaDusurme;
	}

	// 5c) Kan lekelerini ekrana bas.
	if (KanWidget && !KanWidgetOrnegi)
	{
		KanWidgetOrnegi = CreateWidget<UUserWidget>(PC, KanWidget);
		if (KanWidgetOrnegi)
		{
			KanWidgetOrnegi->AddToViewport(5);
		}
	}

	// 6) Yüz ekranda kaldıktan sonra karartma + ölüm ekranı.
	GetWorldTimerManager().SetTimer(
		JumpscareTimerHandle, this, &AMyCharacter::JumpscareBitir,
		FMath::Max(0.05f, JumpscareSuresi), false);
}

void AMyCharacter::JumpscareBitir()
{
	// Yüz ekranda kaldıktan sonra karakter yere yığılır, sonra ölüm ekranı gelir.
	DusmeyeBasla();
}

void AMyCharacter::DusmeyeBasla()
{
	// Sallamayı bırak ve kamerayı sarsıntısız tabanına döndür; devrilme oradan başlasın.
	bJumpscareAktif = false;
	if (UCameraComponent* KorkuKam = KorkuKamera.Get())
	{
		KorkuKam->SetRelativeRotation(KameraTabanRot);
		KorkuKam->SetRelativeLocation(KameraTabanKonum);
	}

	if (!bDusmeAktif)
	{
		OlumEkraniGoster();
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	APawn* OyuncuPawn = PC ? PC->GetPawn() : nullptr;
	UCameraComponent* Kamera = OyuncuPawn ? OyuncuPawn->FindComponentByClass<UCameraComponent>() : nullptr;

	if (!Kamera)
	{
		OlumEkraniGoster();
		return;
	}

	// Kız bizimle birlikte devrilmesin: kamera zincirinden ayır, dünyada dik kalsın.
	if (USkeletalMeshComponent* Korku = KorkuMeshiBul(OyuncuPawn))
	{
		Korku->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	// Kontrolcü dönüşü her kare kamerayı ezmesin, yoksa devrilme görünmez.
	Kamera->bUsePawnControlRotation = false;

	DusenKamera = Kamera;
	DusmeBaslangicRot = Kamera->GetRelativeRotation();
	DusmeBaslangicKonum = Kamera->GetRelativeLocation();
	DusmeGecen = 0.0f;
	bDusuyor = true;
}

// ================= OYUN SONU (THE END) =================

void AMyCharacter::MesafeKontrolEt()
{
}

void AMyCharacter::OlumEkraniGoster()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	JumpscareIsiklariniKaldir();

	PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, KararmaSuresiToplam, FLinearColor::Black, false, true);

	// Kan lekeleri de ekranla birlikte sönsün (kamera fade'i UI'ı etkilemez).
	KarartmaGecen = 0.0f;
	bKarartiyor = true;

	GetWorldTimerManager().SetTimer(OlumEkraniTimerHandle, [this]()
	{
		bKarartiyor = false;
		if (KanWidgetOrnegi)
		{
			KanWidgetOrnegi->RemoveFromParent();
			KanWidgetOrnegi = nullptr;
		}

		// Dört bitiş senaryosunun ortak ekranı: GAME OVER -> YOU DIED.
		UOyunSonu::Goster(this, EOyunSonuTuru::Jumpscare, FString());
	}, KararmaSuresiToplam, false);
}
