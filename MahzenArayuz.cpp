#include "MahzenArayuz.h"
#include "MahzenAyar.h"
#include "OyunDurumu.h"
#include "OyunSonu.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "TusYonlendirici.h"
#include "MenuAcici.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** P ile açılan duraklatma ekranı; aynı anda yalnızca bir tane. */
	TWeakObjectPtr<UMahzenArayuz> GDuraklatmaEkrani;

	const TCHAR* GDuraklatmaWidgetYolu = TEXT("/Game/Mahzen/UI/W_Duraklat.W_Duraklat_C");
}

void UMahzenArayuz::NativeConstruct()
{
	Super::NativeConstruct();

	// Tasarımcı önizlemesinde ses çalmak, temsilci bağlamak ve stil ezmek
	// editörü gereksiz yere riske atıyor; önizlemede hiçbir şey yapma.
	if (IsDesignTime())
	{
		return;
	}

	DugmeleriTopla();
	EylemDugmeleriniBagla();
	TuslariKur();

	// Sekme panellerini çöz, başlangıç sekmesini aç.
	Paneller.Reset();
	for (const FName& Ad : SekmePaneller)
	{
		Paneller.Add(GetWidgetFromName(Ad));
	}
	if (Paneller.Num() > 0)
	{
		SekmeSec(FMath::Clamp(BaslangicSekmesi, 0, Paneller.Num() - 1));
	}

	// Kare başına iş: widget tiki güvenilmez olduğu için çekirdek tiklayıcı.
	if (!TikTutamaci.IsValid())
	{
		TikTutamaci = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UMahzenArayuz::Tikla), 0.0f);
	}
}

void UMahzenArayuz::NativeDestruct()
{
	if (TikTutamaci.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TikTutamaci);
		TikTutamaci.Reset();
	}
	Super::NativeDestruct();
}

void UMahzenArayuz::DugmeleriTopla()
{
	Dugmeler.Reset();
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* W)
	{
		UButton* D = Cast<UButton>(W);
		if (!D)
		{
			return;
		}

		FDugmeDurum Durum;
		Durum.Dugme = D;
		// Çerçevesiz tasarımda geri bildirimi yazının rengi veriyor; taban
		// rengi burada saklanıyor ki hover'dan çıkınca geri dönebilelim.
		if (D->GetChildrenCount() > 0)
		{
			if (UTextBlock* T = Cast<UTextBlock>(D->GetChildAt(0)))
			{
				Durum.Yazi = T;
				Durum.TabanRenk = T->GetColorAndOpacity().GetSpecifiedColor();
			}
		}
		Dugmeler.Add(Durum);

		// Üzerine gelince arkada kayan sis/pas dokusu belirsin.
		if (UzerineGelmeDokusu)
		{
			FButtonStyle Stil = D->GetStyle();
			Stil.Hovered.SetResourceObject(UzerineGelmeDokusu);
			Stil.Hovered.DrawAs = ESlateBrushDrawType::Image;
			Stil.Hovered.TintColor = FSlateColor(FLinearColor::White);
			Stil.Pressed.SetResourceObject(UzerineGelmeDokusu);
			Stil.Pressed.DrawAs = ESlateBrushDrawType::Image;
			Stil.Pressed.TintColor = FSlateColor(FLinearColor(0.75f, 0.72f, 0.70f, 1.0f));
			D->SetStyle(Stil);
		}

		// Tıklama sesi; Blueprint'in kendi OnClicked bağı da ayrıca çalışır
		// (çok yayınlı temsilci, ikisi birden tetiklenir).
		if (TiklamaSesi)
		{
			D->OnClicked.AddDynamic(this, &UMahzenArayuz::DugmeTiklandi);
		}

		// Büyüme merkezden olsun, köşeden değil.
		D->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	});
}

void UMahzenArayuz::DugmeTiklandi()
{
	if (TiklamaSesi)
	{
		UGameplayStatics::PlaySound2D(this, TiklamaSesi, TiklamaSesSiddeti);
	}
}

bool UMahzenArayuz::Tikla(float DeltaTime)
{
	if (IsDesignTime())
	{
		return true;
	}

	if (!bIlkTikYazildi)
	{
		bIlkTikYazildi = true;
		UE_LOG(LogTemp, Log, TEXT("Arayuz tiklayicisi calisiyor: %s (%d dugme, %d panel)"),
			*GetClass()->GetName(), Dugmeler.Num(), Paneller.Num());
	}

	const float DT = FMath::Clamp(DeltaTime, 0.0f, 0.1f);

	KaydiracDegerleriniYaz();

	// --- düğmeler: yumuşak büyüme + üzerine gelme sesi ---
	for (FDugmeDurum& Durum : Dugmeler)
	{
		UButton* D = Durum.Dugme.Get();
		if (!D)
		{
			continue;
		}

		const bool bUzerinde = D->IsHovered();
		if (bUzerinde && !Durum.bUzerindeydi && UzerineGelmeSesi)
		{
			// Fare düğmeler arasında gezinirken ses makineli gibi tekrarlamasın:
			// iki çalma arasında en az UzerineSesAraligi saniye olsun.
			const double Simdi = FPlatformTime::Seconds();
			if (Simdi - SonUzerineSesi >= UzerineSesAraligi)
			{
				SonUzerineSesi = Simdi;
				UGameplayStatics::PlaySound2D(this, UzerineGelmeSesi,
					UzerineSesSiddeti, UzerineSesPerdesi);
			}
		}
		Durum.bUzerindeydi = bUzerinde;

		// Basılıyken tam büyümeye gitmeyip aradaki bir noktaya düşsün:
		// parmağın altında ezilme hissi veriyor.
		const float Hedef = D->IsPressed()
			? 1.0f + (UzerineBuyume - 1.0f) * BasmaOrani
			: (bUzerinde ? UzerineBuyume : 1.0f);
		Durum.Olcek = FMath::FInterpTo(Durum.Olcek, Hedef, DT, BuyumeHizi);
		D->SetRenderScale(FVector2D(Durum.Olcek, Durum.Olcek));

		// Ölçekle aynı eğride yazıyı da kırmızıya çevir: tek bir kalp atışı.
		if (UTextBlock* T = Durum.Yazi.Get())
		{
			const float Aralik = FMath::Max(KINDA_SMALL_NUMBER, UzerineBuyume - 1.0f);
			const float Oran = FMath::Clamp((Durum.Olcek - 1.0f) / Aralik, 0.0f, 1.0f);
			T->SetColorAndOpacity(FSlateColor(
				FMath::Lerp(Durum.TabanRenk, UzerineMetinRengi, Oran)));
		}
	}

	// --- sekme geçişi: yeni panel yumuşakça belirsin ---
	if (SekmeGecen < SekmeGecisSuresi && Paneller.IsValidIndex(AktifSekme))
	{
		SekmeGecen += DT;
		const float Oran = FMath::Clamp(SekmeGecen / FMath::Max(0.01f, SekmeGecisSuresi), 0.0f, 1.0f);
		if (UWidget* Panel = Paneller[AktifSekme].Get())
		{
			Panel->SetRenderOpacity(Oran);
		}
	}

	return true;  // tiklamaya devam
}

void UMahzenArayuz::SekmeSec(int32 Sekme)
{
	if (!Paneller.IsValidIndex(Sekme))
	{
		return;
	}

	AktifSekme = Sekme;
	SekmeGecen = 0.0f;

	for (int32 i = 0; i < Paneller.Num(); ++i)
	{
		UWidget* Panel = Paneller[i].Get();
		if (!Panel)
		{
			continue;
		}
		if (i == Sekme)
		{
			Panel->SetRenderOpacity(0.0f);
			Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Panel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	SekmeVurgusu();
}

void UMahzenArayuz::SekmeVurgusu()
{
	for (int32 i = 0; i < SekmeDugmeleri.Num(); ++i)
	{
		UButton* D = Cast<UButton>(GetWidgetFromName(SekmeDugmeleri[i]));
		if (!D)
		{
			continue;
		}
		const bool bAktif = (i == AktifSekme);

		// Sekme yazısı aktifken açılsın. Çerçeve çizilmediği için ayrımı
		// yalnızca renk veriyor; Tick'teki hover geçişi bu rengi taban alsın
		// diye saklanan taban renk de güncelleniyor.
		const FLinearColor Renk = bAktif ? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)
			: FLinearColor(0.48f, 0.46f, 0.43f, 1.0f);
		if (D->GetChildrenCount() > 0)
		{
			if (UTextBlock* T = Cast<UTextBlock>(D->GetChildAt(0)))
			{
				T->SetColorAndOpacity(FSlateColor(Renk));
				for (FDugmeDurum& Durum : Dugmeler)
				{
					if (Durum.Dugme.Get() == D)
					{
						Durum.TabanRenk = Renk;
						break;
					}
				}
			}
		}
	}
}

bool UMahzenArayuz::SonrakiSekme()
{
	if (!Paneller.IsValidIndex(AktifSekme + 1))
	{
		return false;
	}
	SekmeSec(AktifSekme + 1);
	return true;
}

FReply UMahzenArayuz::NativeOnKeyDown(const FGeometry& Geometri, const FKeyEvent& Olay)
{
	if (bTusIleIlerle && SonrakiSekme())
	{
		if (TiklamaSesi)
		{
			UGameplayStatics::PlaySound2D(this, TiklamaSesi, TiklamaSesSiddeti);
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometri, Olay);
}

FReply UMahzenArayuz::NativeOnMouseButtonDown(const FGeometry& Geometri, const FPointerEvent& Olay)
{
	if (bTusIleIlerle && SonrakiSekme())
	{
		if (TiklamaSesi)
		{
			UGameplayStatics::PlaySound2D(this, TiklamaSesi, TiklamaSesSiddeti);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(Geometri, Olay);
}

void UMahzenArayuz::KaydiracDegerleriniYaz()
{
	// Kaydıraç adı -> yanındaki yüzde yazısının adı. Widget yoksa atlanır,
	// yani bu yazılar tasarımda olmasa da ekran çalışmaya devam eder.
	static const TCHAR* Ciftler[][2] = {
		{ TEXT("Sld_Gamma"),      TEXT("Txt_GammaDeger") },
		{ TEXT("Sld_AnaSes"),     TEXT("Txt_AnaSesDeger") },
		{ TEXT("Sld_MuzikSes"),   TEXT("Txt_MuzikSesDeger") },
		{ TEXT("Sld_EfektSes"),   TEXT("Txt_EfektSesDeger") },
		{ TEXT("Sld_Hassasiyet"), TEXT("Txt_HassasiyetDeger") },
	};
	static_assert(UE_ARRAY_COUNT(Ciftler) == UE_ARRAY_COUNT(SonYuzdeler),
		"SonYuzdeler dizisi Ciftler ile ayni boyda olmali.");

	constexpr int32 Sayi = UE_ARRAY_COUNT(Ciftler);
	for (int32 i = 0; i < Sayi; ++i)
	{
		USlider* Kaydirac = Cast<USlider>(GetWidgetFromName(FName(Ciftler[i][0])));
		UTextBlock* Yazi = Cast<UTextBlock>(GetWidgetFromName(FName(Ciftler[i][1])));
		if (!Kaydirac || !Yazi)
		{
			continue;
		}

		const int32 Yuzde = FMath::RoundToInt(Kaydirac->GetValue() * 100.0f);
		if (Yuzde == SonYuzdeler[i])
		{
			continue;   // her karede FText üretip Slate'i kirletme
		}
		SonYuzdeler[i] = Yuzde;
		Yazi->SetText(FText::FromString(FString::Printf(TEXT("%%%d"), Yuzde)));
	}
}

void UMahzenArayuz::SekmeGrafik() { SekmeSec(0); }
void UMahzenArayuz::SekmeSes() { SekmeSec(1); }
void UMahzenArayuz::SekmeKontrol() { SekmeSec(2); }

void UMahzenArayuz::EkraniDoldur()
{
	UMahzenAyar::AyarEkraniniDoldur(this);
}

void UMahzenArayuz::MenuyuDoldur()
{
	UMahzenAyar::AnaMenuyuDoldur(this);

	if (UTextBlock* T = Cast<UTextBlock>(GetWidgetFromName(TEXT("Surum"))))
	{
		FString Surum;
		GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"),
			TEXT("ProjectVersion"), Surum, GGameIni);
		if (Surum.IsEmpty())
		{
			Surum = TEXT("0.1.0");
		}
		T->SetText(FText::FromString(FString::Printf(TEXT("MAHZEN  v%s"), *Surum)));
	}
}

void UMahzenArayuz::Kapat()
{
	// Ayarlar kapanırken arkadaki ana menü düğmeleri geri gelsin. Oyun
	// haritasındayken (duraklatma ekranı) MenuAcici olmadığı için no-op.
	AMenuAcici::MenuWidgetGoster(this, true);
	RemoveFromParent();
}

void UMahzenArayuz::Devam()
{
	APlayerController* PC = GetOwningPlayer();
	if (GDuraklatmaEkrani.Get() == this)
	{
		GDuraklatmaEkrani.Reset();
	}
	RemoveFromParent();

	if (bDuraklatmaEkrani)
	{
		UGameplayStatics::SetGamePaused(this, false);
		if (PC)
		{
			PC->bShowMouseCursor = false;
			FInputModeGameOnly Mod;
			PC->SetInputMode(Mod);
		}
	}
}

void UMahzenArayuz::AyarlariAc()
{
	if (!AyarWidget)
	{
		return;
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UUserWidget* Ekran = CreateWidget<UUserWidget>(PC, AyarWidget))
		{
			Ekran->AddToViewport(10);
		}
	}
}

void UMahzenArayuz::AnaMenuyeDon()
{
	// Duraklatma menüsünden dönüyorsak oyunu bıraktığı yerden sürdürebilmek
	// için anlık görüntüyü al. Ana menüdeki "DEVAM ET" bunu yükler.
	if (bDuraklatmaEkrani)
	{
		UOyunDurumu::Kaydet(this);
	}
	// Ana menü artık ayrı bir harita değil: AYNI oyun haritası menü kipinde
	// yeniden açılıyor (bütün oyun öncesi ekranlar FirstPersonMap'te).
	UOyunDurumu::ModAyarla(EOyunModu::Menu);
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, OyunHaritasi);
}

// ─── DURAKLATMA MENÜSÜ ────────────────────────────────────────────────────────

void UMahzenArayuz::DuraklatmaAcKapa(const UObject* DunyaBaglami)
{
	UWorld* W = GEngine ? GEngine->GetWorldFromContextObject(DunyaBaglami, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!W || !W->IsGameWorld())
	{
		return;
	}

	// Oyun sonu ekranı açıkken ya da harita MENÜ kipindeyken (ana menü zaten
	// ekranda) P hiçbir şey yapmasın.
	if (UOyunSonu::Aktif() || UOyunDurumu::Mod() == EOyunModu::Menu)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0);
	if (!PC)
	{
		return;
	}

	// Zaten açıksa: P ikinci kez basıldığında oyuna dön.
	if (GDuraklatmaEkrani.IsValid())
	{
		UMahzenArayuz* Acik = GDuraklatmaEkrani.Get();
		GDuraklatmaEkrani.Reset();
		Acik->Devam();
		return;
	}

	UClass* Sinif = LoadClass<UUserWidget>(nullptr, GDuraklatmaWidgetYolu);
	if (!Sinif)
	{
		UE_LOG(LogTemp, Warning, TEXT("DuraklatmaAcKapa: %s yuklenemedi"), GDuraklatmaWidgetYolu);
		return;
	}

	UMahzenArayuz* Ekran = Cast<UMahzenArayuz>(CreateWidget<UUserWidget>(PC, Sinif));
	if (!Ekran)
	{
		return;
	}

	Ekran->bDuraklatmaEkrani = true;
	Ekran->AddToViewport(20);
	GDuraklatmaEkrani = Ekran;

	UGameplayStatics::SetGamePaused(W, true);
	PC->bShowMouseCursor = true;
	FInputModeGameAndUI Mod;
	Mod.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mod.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mod);
}

bool UMahzenArayuz::DuraklatmaAcikMi()
{
	return GDuraklatmaEkrani.IsValid();
}

void UMahzenArayuz::YeniOyun()
{
	UOyunDurumu::Temizle();
	UMahzenAyar::OyunBaslatildiIsaretle();
	UOyunDurumu::ModAyarla(EOyunModu::Oynanis);

	// Menü sahnedeyse onun karartma + geçiş sesiyle gir (daha iyi duruyor);
	// yoksa doğrudan haritayı aç.
	for (TActorIterator<AMenuAcici> It(GetWorld()); It; ++It)
	{
		It->OyunaBasla();
		return;
	}
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, OyunHaritasi);
}

void UMahzenArayuz::KayittanDevam()
{
	if (!UOyunDurumu::VarMi())
	{
		// Kayıt yoksa düğme hiç görünmemeli; yine de yeni oyun gibi davran.
		YeniOyun();
		return;
	}
	UMahzenAyar::OyunBaslatildiIsaretle();
	UOyunDurumu::DevamEt(this);
}

void UMahzenArayuz::EylemDugmeleriniBagla()
{
	struct FEslesme { const TCHAR* Ad; void (UMahzenArayuz::*Fn)(); };
	static const FEslesme Tablo[] = {
		{ TEXT("Btn_Devam"),    &UMahzenArayuz::Devam },
		{ TEXT("Btn_AnaMenu"),  &UMahzenArayuz::AnaMenuyeDon },
		{ TEXT("Btn_Ayarlar"),  &UMahzenArayuz::AyarlariAc },
		{ TEXT("Btn_Cikis"),    &UMahzenArayuz::OyundanCik },
		{ TEXT("Btn_YeniOyun"), &UMahzenArayuz::YeniOyun },
		{ TEXT("Btn_DevamEt"),  &UMahzenArayuz::KayittanDevam },
	};

	for (const FEslesme& E : Tablo)
	{
		UButton* Dugme = Cast<UButton>(GetWidgetFromName(FName(E.Ad)));
		if (!Dugme)
		{
			continue;
		}
		// Kayıt yoksa "DEVAM ET" gizlenir.
		if (FCString::Strcmp(E.Ad, TEXT("Btn_DevamEt")) == 0 && !UOyunDurumu::VarMi())
		{
			Dugme->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		if (E.Fn == &UMahzenArayuz::Devam)             { Dugme->OnClicked.AddDynamic(this, &UMahzenArayuz::Devam); }
		else if (E.Fn == &UMahzenArayuz::AnaMenuyeDon) { Dugme->OnClicked.AddDynamic(this, &UMahzenArayuz::AnaMenuyeDon); }
		else if (E.Fn == &UMahzenArayuz::AyarlariAc)   { Dugme->OnClicked.AddDynamic(this, &UMahzenArayuz::AyarlariAc); }
		else if (E.Fn == &UMahzenArayuz::OyundanCik)   { Dugme->OnClicked.AddDynamic(this, &UMahzenArayuz::OyundanCik); }
		else if (E.Fn == &UMahzenArayuz::YeniOyun)     { Dugme->OnClicked.AddDynamic(this, &UMahzenArayuz::YeniOyun); }
		else if (E.Fn == &UMahzenArayuz::KayittanDevam){ Dugme->OnClicked.AddDynamic(this, &UMahzenArayuz::KayittanDevam); }
	}
}

void UMahzenArayuz::OyundanCik()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

// ─── TUŞ ATAMA SATIRLARI ─────────────────────────────────────────────────────

void UMahzenArayuz::TuslariKur()
{
	const TArray<FName> Eylemler = UMahzenAyar::TusEylemleri();
	const TArray<FKey> Secenekler = UMahzenAyar::TusSecenekleri();

	for (int32 i = 0; i < Eylemler.Num(); ++i)
	{
		const FName Eylem = Eylemler[i];

		if (UTextBlock* Etiket = Cast<UTextBlock>(
			GetWidgetFromName(FName(*FString::Printf(TEXT("Lbl_Tus%d"), i)))))
		{
			Etiket->SetText(UMahzenAyar::TusEylemAdi(Eylem));
		}

		UComboBoxString* Liste = Cast<UComboBoxString>(
			GetWidgetFromName(FName(*FString::Printf(TEXT("Cmb_Tus%d"), i))));
		if (!Liste)
		{
			continue;
		}

		// Listeyi her kuruluşta sıfırdan doldur; tasarımda elle girilmiş
		// seçenekler varsa onlar da temizlenir.
		Liste->ClearOptions();
		for (const FKey& Tus : Secenekler)
		{
			Liste->AddOption(Tus.GetDisplayName(false).ToString());
		}

		const FKey Mevcut = UMahzenAyar::TusOku(Eylem);
		const FString MevcutAd = Mevcut.GetDisplayName(false).ToString();
		if (Liste->FindOptionIndex(MevcutAd) == INDEX_NONE)
		{
			Liste->AddOption(MevcutAd);   // varsayılan listede yoksa da görünsün
		}
		Liste->SetSelectedOption(MevcutAd);

		// Aynı delegeyi iki kez eklememek için önce temizle.
		Liste->OnSelectionChanged.Clear();
		Liste->OnSelectionChanged.AddDynamic(this, &UMahzenArayuz::TusSecimiDegisti);
	}
}

void UMahzenArayuz::TusSecimiDegisti(FString Secilen, ESelectInfo::Type Tip)
{
	if (Tip == ESelectInfo::Direct)
	{
		return;   // SetSelectedOption'ın kendi tetiklemesi; kullanıcı seçimi değil
	}

	// Delege hangi satırdan geldiğini söylemiyor; bütün satırları tarayıp
	// kayıttakinden farklı olanı yazıyoruz.
	const TArray<FName> Eylemler = UMahzenAyar::TusEylemleri();
	const TArray<FKey> Secenekler = UMahzenAyar::TusSecenekleri();
	bool bDegisti = false;

	for (int32 i = 0; i < Eylemler.Num(); ++i)
	{
		UComboBoxString* Liste = Cast<UComboBoxString>(
			GetWidgetFromName(FName(*FString::Printf(TEXT("Cmb_Tus%d"), i))));
		if (!Liste)
		{
			continue;
		}

		const FString SeciliAd = Liste->GetSelectedOption();
		const FKey Kayitli = UMahzenAyar::TusOku(Eylemler[i]);
		if (SeciliAd.IsEmpty() || SeciliAd == Kayitli.GetDisplayName(false).ToString())
		{
			continue;
		}

		for (const FKey& Tus : Secenekler)
		{
			if (Tus.GetDisplayName(false).ToString() == SeciliAd)
			{
				UMahzenAyar::TusYaz(Eylemler[i], Tus);
				bDegisti = true;
				break;
			}
		}
	}

	if (bDegisti)
	{
		if (TiklamaSesi)
		{
			UGameplayStatics::PlaySound2D(this, TiklamaSesi, TiklamaSesSiddeti);
		}
		TuslariYenidenBagla();
	}
}

void UMahzenArayuz::TuslariYenidenBagla()
{
	// Menüde oyuncu pawn'ı yok; oyun içi duraklatma menüsünde var.
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (UTusYonlendirici* Yonlendirici = Pawn->FindComponentByClass<UTusYonlendirici>())
			{
				Yonlendirici->YenidenBagla();
			}
		}
	}
}
