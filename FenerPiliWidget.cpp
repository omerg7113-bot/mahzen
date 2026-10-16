#include "FenerPiliWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UFenerPiliWidget::Guncelle(float Yuzde, FLinearColor Renk)
{
	const float Doluluk = FMath::Clamp(Yuzde, 0.f, 1.f);

	if (PilCubugu)
	{
		PilCubugu->SetPercent(Doluluk);
		PilCubugu->SetFillColorAndOpacity(Renk);
	}

	if (PilYazisi)
	{
		// Yüzde yukarı yuvarlanmıyor: 0.4% kalmışken "%1" yazması yanıltıcı olur.
		const int32 Tam = FMath::FloorToInt(Doluluk * 100.f + 0.5f);
		// Retro sayaç görünümü: üç hane sıfır dolgulu, yazı genişliği hiç oynamıyor.
		PilYazisi->SetText(FText::FromString(FString::Printf(TEXT("%03d%%"), Tam)));
		PilYazisi->SetColorAndOpacity(FSlateColor(Renk));
	}

	if (PilBasi)
	{
		PilBasi->SetColorAndOpacity(Renk);
	}
}
