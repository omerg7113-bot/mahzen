#include "OyunSonu.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** Başlıklar için projedeki sıkışık büyük harf fontu (Staatliches). */
	const TCHAR* GBaslikFontYolu =
		TEXT("/Game/ResidentHorrorV1/Demo/Fonts/F_Staatliches-Regular_Font.F_Staatliches-Regular_Font");

	/** Ana menü haritası; MahzenArayuz ile aynı ad. */
	const TCHAR* GAnaMenuHaritasi = TEXT("AnaMenu");

	// --- zaman çizelgesi (saniye, gerçek zaman; oyun duraklatılmış olsa da akar)
	constexpr float PERDE_SURE      = 0.80f;   // siyah perde
	constexpr float BASLIK_BAS      = 0.60f;   // "GAME OVER" belirmeye başlar
	constexpr float BASLIK_SURE     = 1.50f;   // kullanıcı isteği: 0 -> 1 tam 1.5 sn
	constexpr float BASLIK_SON_BAS  = 3.40f;   // sönmeye başlar
	constexpr float BASLIK_SON_SURE = 0.90f;
	constexpr float IKINCI_BAS      = 4.40f;   // YOU DIED / YOU WIN / KİLİTLİ KALDIN
	constexpr float IKINCI_SURE     = 1.20f;
	constexpr float SEBEP_BAS       = 5.40f;
	constexpr float SEBEP_SURE      = 1.00f;
	constexpr float DUGME_BAS       = 6.30f;
	constexpr float DUGME_SURE      = 0.80f;

	float Gecis(float Zaman, float Baslangic, float Sure)
	{
		return FMath::Clamp((Zaman - Baslangic) / FMath::Max(Sure, 0.01f), 0.0f, 1.0f);
	}

	bool SadeceAscii(const FString& Metin)
	{
		for (const TCHAR C : Metin)
		{
			if (C > 126)
			{
				return false;
			}
		}
		return true;
	}

	/**
	 * Staatliches yalnızca Latin harfleri taşıyor; Türkçe başlıklar (İ, ı, ü...)
	 * motorun varsayılan fontuyla çizilir, yoksa kutu görünürdü.
	 */
	FSlateFontInfo BaslikFontu(const FString& Metin, float Boyut)
	{
		if (SadeceAscii(Metin))
		{
			if (UFont* Font = LoadObject<UFont>(nullptr, GBaslikFontYolu))
			{
				return FSlateFontInfo(Font, Boyut);
			}
		}
		return FCoreStyle::GetDefaultFontStyle("Bold", Boyut);
	}

	/** "GAME OVER" -> "G A M E   O V E R": harf arası boşluk gotik duruşu verir. */
	FString Araliklandir(const FString& Metin)
	{
		FString Sonuc;
		for (int32 i = 0; i < Metin.Len(); ++i)
		{
			if (i > 0)
			{
				Sonuc.AppendChar(TEXT(' '));
			}
			Sonuc.AppendChar(Metin[i]);
		}
		return Sonuc;
	}

	struct FSonuMetni
	{
		FString Ikinci;
		FLinearColor Renk;
	};

	FSonuMetni SonuMetniOlustur(EOyunSonuTuru Tur)
	{
		switch (Tur)
		{
		case EOyunSonuTuru::Zafer:
			return { TEXT("YOU WIN"), FLinearColor(0.78f, 0.70f, 0.42f, 1.0f) };
		case EOyunSonuTuru::KendiniKilitledi:
			return { TEXT("KİLİTLİ KALDIN"), FLinearColor(0.72f, 0.54f, 0.22f, 1.0f) };
		case EOyunSonuTuru::Cikmaz:
			return { TEXT("ÇIKMAZA GİRDİN"), FLinearColor(0.68f, 0.46f, 0.20f, 1.0f) };
		case EOyunSonuTuru::Jumpscare:
		default:
			return { TEXT("YOU DIED"), FLinearColor(0.62f, 0.06f, 0.06f, 1.0f) };
		}
	}
}

/** Oyun sonu ekranı. Slate, oyun duraklatılsa da tik aldığı için animasyon akar. */
class SOyunSonuEkrani : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOyunSonuEkrani) {}
		SLATE_ARGUMENT(EOyunSonuTuru, Tur)
		SLATE_ARGUMENT(FString, Sebep)
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, Dunya)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	FReply TekrarDene();
	FReply AnaMenu();
	void AyrilVeTemizle();

	TSharedRef<SWidget> DugmeYap(const FString& Etiket, FOnClicked Tiklama);

	EOyunSonuTuru Tur = EOyunSonuTuru::Jumpscare;
	FString Sebep;
	TWeakObjectPtr<UWorld> Dunya;

	float Zaman = 0.0f;
	float APerde = 0.0f;
	float ABaslik = 0.0f;
	float AIkinci = 0.0f;
	float ASebep = 0.0f;
	float ADugme = 0.0f;
};

void SOyunSonuEkrani::Construct(const FArguments& InArgs)
{
	Tur = InArgs._Tur;
	Sebep = InArgs._Sebep;
	Dunya = InArgs._Dunya;

	static FSlateColorBrush DolguFircasi(FLinearColor::White);

	const FSonuMetni Metin = SonuMetniOlustur(Tur);
	const FString BirinciMetin = Araliklandir(TEXT("GAME OVER"));

	ChildSlot
	[
		SNew(SOverlay)

		// 1) siyah perde
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SImage)
			.Image(&DolguFircasi)
			.ColorAndOpacity_Lambda([this]()
			{
				return FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, APerde));
			})
		]

		// 2) yazılar
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)

			// GAME OVER
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(FMargin(0, 0, 0, 10))
			[
				SNew(STextBlock)
				.Text(FText::FromString(BirinciMetin))
				.Justification(ETextJustify::Center)
				.Font(BaslikFontu(BirinciMetin, 86))
				.ColorAndOpacity_Lambda([this]()
				{
					return FSlateColor(FLinearColor(0.74f, 0.71f, 0.66f, ABaslik));
				})
			]

			// YOU DIED / YOU WIN / KİLİTLİ KALDIN / ÇIKMAZA GİRDİN
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(FMargin(0, 0, 0, 26))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Araliklandir(Metin.Ikinci)))
				.Justification(ETextJustify::Center)
				.Font(BaslikFontu(Metin.Ikinci, 54))
				.ColorAndOpacity_Lambda([this, Metin]()
				{
					FLinearColor R = Metin.Renk;
					R.A = AIkinci;
					return FSlateColor(R);
				})
			]

			// neden bitti
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(FMargin(0, 0, 0, 34))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Sebep))
				.Justification(ETextJustify::Center)
				.AutoWrapText(true)
				.WrapTextAt(900.0f)
				.LineHeightPercentage(1.35f)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 21))
				.Visibility(Sebep.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible)
				.ColorAndOpacity_Lambda([this]()
				{
					return FSlateColor(FLinearColor(0.60f, 0.58f, 0.55f, ASebep));
				})
			]

			// düğmeler
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(0, 0, 40, 0))
				[
					DugmeYap(TEXT("TEKRAR DENE"), FOnClicked::CreateSP(this, &SOyunSonuEkrani::TekrarDene))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					DugmeYap(TEXT("ANA MENÜ"), FOnClicked::CreateSP(this, &SOyunSonuEkrani::AnaMenu))
				]
			]
		]
	];
}

TSharedRef<SWidget> SOyunSonuEkrani::DugmeYap(const FString& Etiket, FOnClicked Tiklama)
{
	static FButtonStyle Stil = FButtonStyle()
		.SetNormal(FSlateNoResource())
		.SetHovered(FSlateNoResource())
		.SetPressed(FSlateNoResource())
		.SetDisabled(FSlateNoResource())
		.SetNormalPadding(FMargin(0))
		.SetPressedPadding(FMargin(0));

	TSharedRef<SButton> Dugme = SNew(SButton)
		.ButtonStyle(&Stil)
		.ContentPadding(FMargin(18, 8))
		.OnClicked(Tiklama)
		.Visibility_Lambda([this]()
		{
			return ADugme > 0.05f ? EVisibility::Visible : EVisibility::Hidden;
		});

	TWeakPtr<SButton> Zayif = Dugme;

	Dugme->SetContent(
		SNew(STextBlock)
		.Text(FText::FromString(Etiket))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 22))
		.ColorAndOpacity_Lambda([this, Zayif]()
		{
			const TSharedPtr<SButton> D = Zayif.Pin();
			const bool bUstunde = D.IsValid() && D->IsHovered();
			const FLinearColor Taban = bUstunde
				? FLinearColor(0.92f, 0.88f, 0.82f, 1.0f)
				: FLinearColor(0.48f, 0.46f, 0.43f, 1.0f);
			return FSlateColor(FLinearColor(Taban.R, Taban.G, Taban.B, ADugme));
		})
	);

	return Dugme;
}

void SOyunSonuEkrani::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	Zaman += InDeltaTime;

	APerde  = Gecis(Zaman, 0.0f, PERDE_SURE);
	ABaslik = FMath::Min(Gecis(Zaman, BASLIK_BAS, BASLIK_SURE),
	                     1.0f - Gecis(Zaman, BASLIK_SON_BAS, BASLIK_SON_SURE));
	AIkinci = Gecis(Zaman, IKINCI_BAS, IKINCI_SURE);
	ASebep  = Gecis(Zaman, SEBEP_BAS, SEBEP_SURE);
	ADugme  = Gecis(Zaman, DUGME_BAS, DUGME_SURE);
}

void SOyunSonuEkrani::AyrilVeTemizle()
{
	UOyunSonu::Kapat();
}

FReply SOyunSonuEkrani::TekrarDene()
{
	UWorld* W = Dunya.Get();
	const FName Harita = W ? FName(*W->GetName()) : NAME_None;
	AyrilVeTemizle();
	if (W)
	{
		UGameplayStatics::SetGamePaused(W, false);
		UGameplayStatics::OpenLevel(W, Harita);
	}
	return FReply::Handled();
}

FReply SOyunSonuEkrani::AnaMenu()
{
	UWorld* W = Dunya.Get();
	AyrilVeTemizle();
	if (W)
	{
		UGameplayStatics::SetGamePaused(W, false);
		UGameplayStatics::OpenLevel(W, FName(GAnaMenuHaritasi));
	}
	return FReply::Handled();
}

// ===================== kütüphane =====================

namespace
{
	TSharedPtr<SOyunSonuEkrani> GEkran;
	TWeakObjectPtr<UWorld> GEkranDunyasi;
}

bool UOyunSonu::Aktif()
{
	return GEkran.IsValid();
}

void UOyunSonu::Kapat()
{
	if (GEkran.IsValid())
	{
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(GEkran.ToSharedRef());
		}
		GEkran.Reset();
	}
	GEkranDunyasi.Reset();
}

void UOyunSonu::Goster(const UObject* DunyaBaglami, EOyunSonuTuru Tur, const FString& Sebep)
{
	UWorld* Dunya = GEngine
		? GEngine->GetWorldFromContextObject(DunyaBaglami, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!Dunya)
	{
		return;
	}

	// Önceki seviyeden kalan ekran varsa temizle; aynı seviyede ikinci çağrıyı yoksay.
	if (GEkran.IsValid())
	{
		if (GEkranDunyasi.Get() == Dunya)
		{
			return;
		}
		Kapat();
	}

	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("OyunSonu: tur=%d sebep=%s"), (int32)Tur, *Sebep.Replace(TEXT("\n"), TEXT(" | ")));

	GEkranDunyasi = Dunya;
	GEkran = SNew(SOyunSonuEkrani)
		.Tur(Tur)
		.Sebep(Sebep)
		.Dunya(Dunya);

	GEngine->GameViewport->AddViewportWidgetContent(GEkran.ToSharedRef(), 1000);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(Dunya, 0))
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->DisableInput(PC);
		}
		PC->SetIgnoreLookInput(true);
		PC->SetIgnoreMoveInput(true);
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}

	UGameplayStatics::SetGamePaused(Dunya, true);
}
