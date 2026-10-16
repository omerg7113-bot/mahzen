// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "MyCharacter.generated.h"

class UAnimSequenceBase;
class UCameraComponent;
class UPointLightComponent;
class UCameraShakeBase;
class USkeletalMesh;
class USkeletalMeshComponent;
class USoundBase;

UENUM(BlueprintType)
enum class Ekaraktermodlari : uint8
{
	NewEnumerator0 UMETA(DisplayName = "Agresif Mod"),
	NewEnumerator1 UMETA(DisplayName = "Takipçi Mod"),
	NewEnumerator2 UMETA(DisplayName = "Hayalet Mod")
};

UCLASS()
class MYPROJECT2_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMyCharacter();

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	UUserWidget* AktifWidget = nullptr;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// --- DEĞİŞKENLER ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
	int32 kalansure;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
	Ekaraktermodlari guncelmod;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
	Ekaraktermodlari BeklenenMod;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
	FString GelecekModAdi;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
	int32 zorlukdereceleri;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> KizYurume;

	// --- OYUN SONU ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver")
	float OlumMesafesi = 100.0f;

	// Yere düştükten sonra ekranın yavaşça kararma süresi.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver")
	float KararmaSuresiToplam = 2.5f;

	// Boşsa native "THE END / YOU DIED" ekranı çizilir; doluysa bu widget kullanılır.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver")
	TSubclassOf<UUserWidget> OlumEkraniWidget;

	// --- JUMPSCARE ---

	// Kameraya yapışacak yüzün kameradan uzaklığı (birim). PIE'de 130 doğrulandı;
	// 45 gibi değerlerde ekranı saç kaplıyor çünkü karakter ~3.5 m boyunda.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float YuzMesafesi = 130.0f;

	// Yüzün dikey kaydırması; 0 = tam kamera hizası.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float YuzYukseklikOfseti = 0.0f;

	// Mesh'in kameraya dönük olmasını sağlayan yaw. PIE'de 0 doğrulandı (180 sırtını dönüyor).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float YuzYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float YuzPitch = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float OlcekCarpani = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	FName KafaKemigiAdi = TEXT("head");

	// korkunoktası altındaki mesh bileşeninin adı (Arrow'a bağlı olan otomatik bulunur).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	FName KorkuMeshAdi = TEXT("SkeletalMesh");

	// Jumpscare'in ilk anında gösterilen AĞZI KAPALI mesh (yüzünde delik ve diş yok).
	// Doluysa kız önce bununla belirir, çekme anında KorkuMeshVarligi'na geçilir.
	// Yüzde kalıcı bir ağız deliği olduğu için "kapalı ağız" tek mesh'le mümkün değil.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	TObjectPtr<USkeletalMesh> KorkuMeshKapali = nullptr;

	// Kaç saniye sonra ağzı açık mesh'e geçilsin (çekme anıyla aynı tutulmalı).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float MeshDegisimAni = 0.45f;

	// Doluysa jumpscare anında rig'deki mesh bununla değiştirilir.
	// Mevcut Tripo modelinde modellenmiş bir yüz yok; düzgün yüzlü bir karakter
	// bulunduğunda buraya atamak yeterli, kod değişmesi gerekmez.
	// Yeni mesh'in kafa kemiği farklı adlanıyorsa KafaKemigiAdi'ni de güncelle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	TObjectPtr<USkeletalMesh> KorkuMeshVarligi = nullptr;

	// Jumpscare başladıktan kaç saniye sonra karakter yere düşmeye başlasın.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float JumpscareSuresi = 1.5f;

	// --- KOL ANİMASYONU (kameraya uzanıp tutma) ---

	// Jumpscare anında korku rig'inde oynayacak animasyon (AS_Uzanma_Doll gibi).
	// Boşsa bileşenin kendi AnimationData'sındaki animasyon kullanılır.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Animasyon")
	TObjectPtr<UAnimSequenceBase> KorkuAnimasyonu = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Animasyon")
	float KorkuAnimHizi = 1.0f;

	// Animasyonun hangi saniyesinden başlasın (kollar en açık olduğu kareye atlamak için).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Animasyon")
	float KorkuAnimBaslangici = 0.0f;

	// true = animasyon o karede dondurulur (poz olarak kullanılır).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Animasyon")
	bool bKorkuAnimDondur = false;

	// --- YAPIŞMA (yüzün kameraya saldırması) ---

	// Kız bu mesafede belirir (burnumuzun dibi). Kafa kemiği referans alınır;
	// yüzün yüzeyi bundan ~26 birim daha yakındır, yakın kırpma düzlemi 10 birim.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Yapisma")
	float YuzBaslangicMesafesi = 85.0f;

	// Belirdikten sonra kolların kavradığı mesafeye oturma süresi.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Yapisma")
	float YapismaSuresi = 0.10f;

	// Kollar kameraya kenetlenirken beklediği mesafe.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Yapisma")
	float YuzKavramaMesafesi = 72.0f;

	// Kavradıktan kaç saniye sonra bizi kendine çeksin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Yapisma")
	float CekmeGecikmesi = 0.16f;

	// Çekmenin süresi. Kısa = daha sert hamle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Yapisma")
	float CekmeSuresi = 0.13f;

	// Çekmenin ivmelenmesi (1 = doğrusal, büyük = sona doğru patlar).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Yapisma")
	float CekmeIvmesi = 3.0f;

	// Çekme sonunda bu kadar daha yaklaşıp geri yaylanır (çarpma hissi).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Yapisma")
	float CekmeTasmasi = 6.0f;

	// --- SALLAMA (kollarıyla kamerayı tutup sarsması) ---

	// Kamera BİLEŞENİNİ sallar. Mesh kameranın çocuğu olduğu için yüz ekranda sabit
	// kalır, sarsılan dünya olur — yani bizi o sallıyormuş gibi görünür.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Sallama")
	bool bSallama = true;

	// Açısal genlik (derece).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Sallama")
	float SallamaAci = 7.0f;

	// Konumsal genlik (birim).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Sallama")
	float SallamaKayma = 6.0f;

	// Saniyedeki sallama sayısı.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Sallama")
	float SallamaFrekans = 6.0f;

	// 0 = hiç sönmez, 1 = süre sonunda tamamen durur.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Sallama")
	float SallamaSonmeOrani = 0.3f;

	// Bağırma hissi: yüzün kendi yüksek frekanslı titremesi (derece).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Sallama")
	float TitremeAci = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Sallama")
	float TitremeFrekans = 22.0f;

	// Jumpscare boyunca oyuncunun bakış/hareket girdisini kilitle (kafasını tutuyor).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Sallama")
	bool bBakisiKilitle = true;

	// --- BAĞIRMA (ağız morph target'ı) ---

	// Blender'da açılan ağzın morph target adı. Mesh'te yoksa sessizce atlanır.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Bagirma")
	FName AgizMorphAdi = TEXT("AgizAcik");

	// 1.0 = tam açık (dudaklarda ~4 cm). 0 = kapalı.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Bagirma")
	float AgizAcikligi = 1.0f;

	// Ağzın açılma süresi (saniye).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Bagirma")
	float AgizAcilmaSuresi = 0.10f;

	// Ağız bu saniyede açılmaya başlar; çekme anıyla aynı olsun diye ayrı tutuldu.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Bagirma")
	float AgizGecikmesi = 0.16f;

	// Çığlık bu saniyede çalar (0 = jumpscare başlar başlamaz).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Bagirma")
	float CiglikGecikmesi = 0.14f;

	// Bağırırken ağzın nabız gibi titremesi (0 = sabit açık).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Bagirma")
	float AgizTitresimMiktari = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Bagirma")
	float AgizTitresimFrekans = 9.0f;

	// Sahneyi karart (otomatik pozlama kaydırması; negatif = daha karanlık).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float PozlamaDusurme = -1.3f;

	// --- YERE DÜŞME ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Dusme")
	bool bDusmeAktif = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Dusme")
	float DusmeSuresi = 1.2f;

	// Negatif = sola devrilme.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Dusme")
	float DusmeRoll = -82.0f;

	// Negatif = yere doğru bakma.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Dusme")
	float DusmePitch = -28.0f;

	// Kameranın ne kadar aşağı ineceği.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Dusme")
	float DusmeYuksekligi = 75.0f;

	// Sola savrulma miktarı.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Dusme")
	float DusmeYanKayma = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	TSubclassOf<UCameraShakeBase> SarsintiSinifi;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float SarsintiSiddeti = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	USoundBase* CiglikSesi = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float CiglikSesSiddeti = 1.0f;

	// Jumpscare anında ekrana basılan kan lekeleri; kararma sırasında birlikte söner.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	TSubclassOf<UUserWidget> KanWidget;

	// Karanlık koridorda yüz siyah bir lekeye dönüşmesin diye dolgu ışığı.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	bool bIsikTakviyesi = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float IsikYogunlugu = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float IsikYaricapi = 1200.0f;

	// Dolgu ışığının kameradan ileri mesafesi.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float IsikOnMesafe = 55.0f;

	// Negatif = alttan aydınlatma; ağız boşluğunu ve göz çukurlarını belirginleştirir.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float IsikDikeyOfset = -22.0f;

	// --- JUMPSCARE IŞIKLARI (sadece jumpscare anında yaratılır, sonunda silinir) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Isik")
	bool bJumpscareIsiklari = true;

	// Çene altından vuran koyu kan kırmızısı anahtar ışık.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Isik")
	FLinearColor KanIsikRengi = FLinearColor(0.60f, 0.025f, 0.015f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Isik")
	float KanIsikSiddeti = 9000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Isik")
	float KanIsikYaricapi = 400.0f;

	// Kameradan ileri / dikey ofset (negatif dikey = çene altı).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Isik")
	float KanIsikOn = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Isik")
	float KanIsikDikey = -34.0f;

	// Kamera hizasından dişlerin sivriliğini parlatan soğuk morg mavisi kontur ışığı.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Isik")
	FLinearColor MorgIsikRengi = FLinearColor(0.35f, 0.58f, 0.95f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Isik")
	float MorgIsikSiddeti = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Isik")
	float MorgIsikYaricapi = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Isik")
	float MorgIsikOn = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare|Isik")
	float MorgIsikDikey = 8.0f;

	// Arka planı bulanıklaştırma (alan derinliği). Yüz net kalır.
	// DİKKAT: 0.4/150 gibi agresif değerler yüzü de bulanıklaştırıyor; yumuşak tutuldu.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	bool bBlurAktif = true;

	// Küçük değer = daha güçlü bulanıklık.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float BlurFStop = 2.0f;

	// Büyük değer = daha güçlü bulanıklık.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumpscare")
	float BlurSensorGenisligi = 70.0f;

	// Yakalanma anında çağrılır: yüzü kadraja oturtur, sarsar, çığlık atar, arkayı blurlar.
	UFUNCTION(BlueprintCallable, Category = "Jumpscare")
	void JumpscareBaslat();

	// --- ANA FONKSİYONLAR ---
	UFUNCTION(BlueprintCallable, Category = "Functions")
	void Sayacim();

	UFUNCTION(BlueprintCallable, Category = "Functions")
	void ModHesapla(Ekaraktermodlari& secilenmod);

	UFUNCTION(BlueprintCallable, Category = "Functions")
	void agresif();

	UFUNCTION(BlueprintCallable, Category = "Functions")
	void takipci();

	UFUNCTION(BlueprintCallable, Category = "Functions")
	void hayalet();

private:
	FTimerHandle SayacTimerHandle;
	FTimerHandle AgresifTimerHandle;
	FTimerHandle TakipciTimerHandle;
	FTimerHandle HayaletTimerHandle;
	FTimerHandle JumpscareTimerHandle;
	FTimerHandle OlumEkraniTimerHandle;

	UPROPERTY()
	UUserWidget* KanWidgetOrnegi = nullptr;

	// Yere düşme ve sonrasındaki kararma (Tick ile sürülür).
	bool bDusuyor = false;
	float DusmeGecen = 0.0f;
	bool bKarartiyor = false;
	float KarartmaGecen = 0.0f;
	TWeakObjectPtr<UCameraComponent> DusenKamera;
	FRotator DusmeBaslangicRot = FRotator::ZeroRotator;
	FVector DusmeBaslangicKonum = FVector::ZeroVector;
	void DusmeyeBasla();

	// Jumpscare sürüşü (yaklaşma + sallama, Tick ile).
	bool bJumpscareAktif = false;
	float JumpscareGecen = 0.0f;
	TWeakObjectPtr<UCameraComponent> KorkuKamera;
	TWeakObjectPtr<USkeletalMeshComponent> KorkuMesh;
	FRotator KameraTabanRot = FRotator::ZeroRotator;
	FVector KameraTabanKonum = FVector::ZeroVector;
	bool bCiglikCaldi = false;
	bool bMeshDegisti = false;

	// Jumpscare anında yaratılan iki ışık (bitince yok edilir).
	UPROPERTY()
	TObjectPtr<UPointLightComponent> KanIsigi = nullptr;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> MorgIsigi = nullptr;

	UPointLightComponent* IsikYarat(USceneComponent* Ebeveyn, const FLinearColor& Renk,
		float Siddet, float Yaricap);
	void JumpscareIsiklariniKaldir();

	// Jumpscare'in o anki mesafesi (beliriş → kavrama → çekme).
	float MesafeHesapla(float Gecen) const;

	// Korku mesh'ini kafası verilen mesafede kameraya bakacak şekilde dünyada kurar.
	void KorkuMeshiYerlestir(float Mesafe, const FRotator& EkRot);

	// korkunoktası rig'indeki mesh'i bulur (önce Arrow'a bağlı olan, sonra ada göre).
	USkeletalMeshComponent* KorkuMeshiBul(APawn* OyuncuPawn) const;

	// Kafa kemiğinin referans pozdaki bileşen-uzayı konumu.
	FVector KafaKemigiYerelKonum(USkeletalMeshComponent* Bilesen) const;

	void JumpscareBitir();

	bool bOyunBitti = false;

	void AgresifHareketEt();
	void TakipciHareketEt();
	void HayaletHareketEt();

	void WidgetGoster(const FString& Metin);
	void WidgetKaldir();

	void MesafeKontrolEt();
	void OlumEkraniGoster();
};