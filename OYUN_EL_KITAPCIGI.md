# MAHZEN
## OYUNCU EL KİTAPÇIĞI

> *"Aşağıda hava yok. Aşağıda sadece bekleyiş var."*

---

### İÇİNDEKİLER

| # | Bölüm |
|---|-------|
| I | [Mahzen'e İniş](#i--mahzene-İniş) |
| II | [Temel Hayatta Kalma Kuralları](#ii--temel-hayatta-kalma-kurallari) |
| III | [Kontroller & Etkileşim Rehberi](#iii--kontroller--etkİleŞİm-rehberİ) |
| IV | [Varlık Dosyası](#iv--varlik-dosyasi) |
| V | [İleri Düzey Taktikler](#v--İlerİ-düzey-taktİkler) |
| VI | [Hızlı Başvuru Kartı](#vi--hizli-baŞvuru-karti) |

---

# I — MAHZEN'E İNİŞ

Merdivenin son basamağında taş, ayakkabının altında başka türlü ses çıkarır. Yukarıda bıraktığın dünyanın sesi — rüzgâr, uzaktan bir köpek, kendi nefesin — hepsi o basamakta kesilir. Mahzen sesi yutmaz; **saklar.** Sonra, uygun gördüğü anda, sana geri verir.

Elinde bir fener var. Pili, sabrından uzun değil.

Yirmi oda. Her birinin loş bir köşesinde, kapağı yüzyıllardır kapalı bir kasa. Her kasanın içinde, bu yapıyı ayakta tutan şeyin bir parçası: bir **mühür.** Yirmi mühür. Hepsini bulup işlediğinde mahzen kapanır — ve sen yukarıda olursun.

Yanında yirmi iki anahtar dolaşıyor bu koridorlarda. Kimisi bir rafın arkasında, kimisi bir cesedin avucunda, kimisi tam göz hizasında, seni bekler gibi. Yirmi kasa için yirmi iki anahtar. Bu, matematiğin sana verdiği tek hediye: **burada kilitlenip kalamazsın.** Yol her zaman vardır.

Ama yolda bir şey var.

Ona *Kız* diyorlar. Çünkü ilk duyduğunda bir çocuğun koridorda yürüdüğünü sanırsın. Işığı sevmez, sesi sever, sabrı seninkinden derindir. Üç halde dolaşır: bazen seni bilmeden süzülür, bazen izini sürer, bazen de — koşar.

Ve mahzenin en eski, en acımasız, en merhametli kuralı şudur:

> ### Seni ancak ona baktığın sürece öldürebilir.
> **Bakmazsan öldüremez.**

Gözlerini kaç, sırtını dön, yürü. Korkunun doğru yönü aşağı değil, **öteye** doğrudur.

Fenerini kapat. Dinle.

Ve in.

---

# II — TEMEL HAYATTA KALMA KURALLARI

## 2.1 — Mahzenin Yapısı

Mahzen rastgele değil; **saat gibi kurulmuş bir sistemdir.** Her oyunda dağılım değişir, ama sayılar asla değişmez.

| Unsur | Adet | İşlevi |
|:--|:--:|:--|
| **Oda** | 20 | Mahzenin fiziksel haritası. Katlara dağılmıştır, kapılarla bağlanır. |
| **Kasa** | 20 | Her odada tam olarak **bir** kasa bulunur. Kilitlidir. İçinde bir mühür vardır. |
| **Mühür** | 20 | Kazanma koşulu. Kasadan alınır, bir mühür yuvasına işlenir. |
| **Anahtar** | 22 | Kasaları açar. Odalara serbest biçimde saçılmıştır — kasa içinde **değil.** |

**Bire bir eşleşme kuralı:** 1 oda = 1 kasa = 1 mühür. Bir odayı temizlediysen o odada yapacak işin kalmamıştır. Mühürü aldığın odayı zihninde **kapat.**

## 2.2 — Kazanma Koşulu

**20 mührün 20'sini de işle ve çıkış merdivenine ulaş.**

Eksik mühürle merdiven açılmaz. On dokuz mühür ile sıfır mühür arasında, mahzen açısından hiçbir fark yoktur.

## 2.3 — Çıkmaz Sokak Yoktur: Kilitlenme Garantisi

Oyuncuların en büyük korkusu şudur: *"Son kasanın anahtarı o kasanın içinde kalırsa ne olur? Oyun kilitlenir mi?"*

**Kilitlenmez. Kilitlenemez.** Bu, bir tasarım tercihi değil — mahzenin kendi matematiğidir. Dört garanti üzerine kuruludur:

> ### Garanti 1 — Arz, talebi daima aşar
> 22 anahtar, 20 kasa. Kaynak fazlası: **+2.**
> Her kasayı açmak için bir anahtar gerekir; elinde her zaman gereğinden iki fazla anahtar vardır.

> ### Garanti 2 — Döngüsel bağımlılık imkânsızdır
> **Hiçbir anahtar, kasa içine yerleştirilmez.** Anahtarlar yalnızca odaların açık alanlarına dağılır.
> Bu yüzden "A kasasını açmak için B kasasının içindeki anahtar gerekir" türünden bir zincir **hiç oluşmaz.** Bağımlılık grafiği döngüsüzdür; her kasa, elindeki herhangi bir uygun anahtarla, herhangi bir sırayla açılabilir.

> ### Garanti 3 — Anahtarlar yok olmaz
> Kullanılan anahtar, kasayı açtıktan sonra zaten gereksizdir — o kasa bir daha kilitlenmez.
> **Fırlatılan anahtar (G) kaybolmaz:** düştüğü yerde durur, geri alınabilir. Paniğinde düşürdüğün anahtar da öyle. Mahzen senden hiçbir kaynağı kalıcı olarak geri almaz.

> ### Garanti 4 — Mühürler birbirine bağımlı değildir
> 20 mühür, 20 ayrı kasadan çıkar. Birini işlemek diğerini etkilemez, sırası serbesttir.
> Yanlış sırada mühürlemek diye bir hata yoktur — yalnızca **zamanlaması kötü** mühürleme vardır (bkz. Bölüm 5.1).

### Sonuç — Değişmez (invariant)

> **Oyunun her anında, elindeki anahtarlarla açılabilecek en az bir kasa vardır.**
> İlerleme her zaman mümkündür. Bu mahzende başarısızlığın tek bir yolu vardır: **ölmek.** Kilitlenmek yoktur.

### +2 fazlalık ne işe yarar?

Fazla iki anahtar, **hata toleransındır.** Şunlar için harcanabilir:

- Dikkat dağıtmak için fırlatılan anahtarlar (G tuşu — bkz. 5.2)
- Varlıktan kaçarken yanlış odada bıraktıkların
- Karanlıkta düşürüp bir daha bulamadıkların

İki anahtar; iki hata hakkı. Üçüncüsünde mahzen hâlâ çözülebilir — ama bedavadan değil, koşturarak.

## 2.4 — Üç Değişmez Davranış Kuralı

1. **Sessizlik, karanlıktan değerlidir.** Varlık seni görmekten çok **duyar.** Fener riski görünürlüktür; koşmak riski ölümdür.
2. **Her odayı bir kez temizle, bir daha dönme.** Geri dönüş, Varlığın zaten taradığı bir alana girmek demektir.
3. **Elin doluysa iş yapmaya başlamazsın.** Mühür işlemek, kasa açmak — hepsi seni saniyeler boyunca savunmasız bırakır. Önce Varlığın nerede olduğunu bil, sonra eğil.

---

# III — KONTROLLER & ETKİLEŞİM REHBERİ

## 3.1 — Tuş Dizilimi

| Tuş | Eylem | Ne yapar? | Risk |
|:--:|:--|:--|:--|
| **E** | **Etkileşim** | Kapı açar/kapatır, kasayı açar, yerden anahtar/eşya alır. Her şeyin başladığı tuş. | Kasa açmak **gürültülüdür.** |
| **F** | **Fener** | Feneri açar/kapatır. | Açık fener, Varlığın görüş menzilinde **bayrak** gibidir. |
| **T** | **Mühür İşle** | Elindeki mührü, karşısındaki mühür yuvasına işler. Birkaç saniyelik bir animasyon başlatır. | Animasyon boyunca **hareket edemezsin.** En savunmasız anın. |
| **K** | **Kilitle / Kilit Aç** | Kapandıktan sonra kapıyı kilitler. | Kilitli kapı **Takipçi'yi geciktirir, Agresif'i durdurmaz.** |
| **Q** | **Çömel / Sessiz Hareket** | Yürüyüş sesini en alt kademeye indirir. Hız düşer. | Yavaşsın. Agresif modda çömelmek **intihardır.** |
| **I** | **Envanter** | Anahtarlarını ve mühürlerini listeler, aktif eşyayı seçer. | Envanter açıkken dışarıdaki dünya durmaz. |
| **G** | **Anahtar Fırlat** | Seçili anahtarı bakış yönüne fırlatır. Çarptığı yerde **yüksek bir ses** üretir. | Bir anahtarı ve bir saniyeni harcar. Karşılığında bir rota kazandırır. |

## 3.2 — Eşik Mantığı: Kapılarda Durma Kuralı

Mahzenin en çok oyuncu öldüren kuralı bu — ve en az anlaşılanı.

> ### **Bir kapı eşiğinde asla durmazsın.**

Kapı aralığı bir geçittir, bir mevzi değil. Eşikte durduğun an üç şey birden olur:

1. **İki odadan birden görünürsün.** Varlığın hangi odada olduğunu bilmesen de, o seni iki koridorun birinden yakalar.
2. **Sesin iki odaya birden taşar.** Eşik, ses yalıtımının olmadığı tek noktadır. Çömelsen bile iki odaya duyurursun.
3. **Kapı kapanmaz, kilitlenmez.** Gövden kanadı engeller. `E` çalışmaz, `K` çalışmaz. Arkanı kapatamazsın — ve Varlık bunu bilir.

### Doğru geçiş prosedürü

```
1. Eşiğin ÖNÜNDE dur. Dinle. (uğultuyu oku — bkz. 5.3)
2. E ile kapıyı aç.
3. TEK HAREKETTE, duraksamadan karşı odaya TAMAMEN geç.
4. Dön, E ile kapıyı kapat.
5. Gerekiyorsa K ile kilitle.
6. ŞİMDİ işine başla.
```

**Asla:** eşikte envanter açmak (`I`), eşikte mühür işlemek (`T`), eşikte anahtar fırlatmak (`G`), eşikte "bir saniye bakıp dönmek".

Eşikte geçirdiğin her saniye, Varlığa bedava verilmiş bir saniyedir.

## 3.3 — Fenerle Etkileşim

Fener, Mahzen'de bir araç değil; bir **para birimidir.** Harcadıkça görürsün, harcadıkça görülürsün.

**Fenerin işleyişi:**

- **Açık fener (F):** Odayı ve kasa kilidini okuyabilmeni sağlar. Uzaktan görünür; Varlığın görüş alanında ışık, hareketten daha güçlü bir sinyaldir.
- **Kapalı fener:** Siluetler, kapı çerçeveleri ve mühür yuvalarının soluk parıltısı görünmeye devam eder. **Karanlıkta yürümek öğrenilebilir bir beceridir** ve bu oyunun temel becerisidir.
- **Kasa kilidini okumak** fener gerektirir. Anahtar denemesi karanlıkta yapılamaz.

**Fener disiplini — üç kural:**

1. **Koridorda kapalı, odada kısa süreli açık.** Hareket ederken fenere ihtiyacın yok; iş yaparken var.
2. **Feneri Varlığın üzerine tutmak, ona bakmak sayılır.** Işığı onun üstüne çevirdiğin an, "Bakmazsan öldüremez" kuralının koruması kalkar. Işık, bakıştır.
3. **Fenerin açılma/kapanma klik sesi duyulur.** Varlık çok yakınken feneri açıp kapatmak, sessiz durmaktan daha risklidir. Zaten karanlıktaysan, karanlıkta kal.

> **Altın kural:** Fener, Varlığın en az **bir kat** uzakta olduğunu bildiğin anlarda açılır. Başka hiçbir zaman.

---

# IV — VARLIK DOSYASI
### Kayıt Adı: *"Kız"*

> *"Yürüyüşü bir çocuğun, sabrı bir taşın."*

Varlık, mahzende tek bir birey olarak dolaşır. Çoğalmaz, ikiye bölünmez, ışınlanmaz. **Her zaman belirli bir odada, belirli bir kattadır** ve oraya yürüyerek gitmiştir. Bu, onun en büyük zayıflığıdır: **izlenebilir.**

Üç halden birinde bulunur. Haller birbirine **merdiven gibi** bağlıdır — tırmanır ve iner.

## 4.1 — HAL 1: HAYALET (Pasif)

**Varlık senin varlığını bilmiyor.**

| Özellik | Davranış |
|:--|:--|
| **Hareket** | Yavaş, amaçsız süzülme. Odadan odaya rastgele dolaşır. |
| **Kapılar** | Kapalı kapıları açar, ama kilitli kapıyı **zorlamaz** — döner, başka yol arar. |
| **Algı** | Ses menzili **en geniş**, tepkisi **en yavaş.** Bir ses duyduğunda hemen gelmez; o yöne doğru **ilgisini kaydırır.** |
| **Ölümcüllük** | Temas kurmaz. Bu halde seni öldürmeye kalkışmaz — ama seni **görürse** anında Hal 3'e tırmanır. |
| **Senin işin** | Mahzenin bütün ağır işi bu halde yapılır: kasa açmak, mühür işlemek, kat değiştirmek. |

**Hayalet modu senin çalışma saatindir.** Varlığı bu halde tuttuğun her dakika, kazanılmış bir mühürdür.

## 4.2 — HAL 2: TAKİPÇİ (Şüpheli)

**Varlık bir şey duydu. Nerede olduğunu bilmiyor; ama nereden geldiğini biliyor.**

| Özellik | Davranış |
|:--|:--|
| **Tetikleyici** | Duyulan bir gürültü: koşma, kasa açma, kapı çarpması, fırlatılan anahtar, mühür animasyonu. |
| **Hareket** | Hızlı ve **amaçlı** yürüyüş — koşmaz. Sesin geldiği noktaya doğrudan ilerler. |
| **Hedefi** | Senin son bilinen **ses konumun.** Kendin değil. Sen o noktadan uzaklaştıysan, boş bir odaya varır. |
| **Kapılar** | Kapıları açar. **Kilitli kapıda belirgin biçimde gecikir** — kilit, saniye kazandırır. |
| **İniş yolu** | Sesin kaynağına varır, kısa bir süre tarar, hiçbir şey bulamazsa **Hayalet'e geri döner.** |
| **Senin işin** | Sessizliğe çekilmek. Mühür işlemeyi **derhal** bırak; `T` animasyonu bu halde ölüm fermanıdır. |

> **Takipçi, bir tehdit değil, bir uyarıdır.** Doğru okunduğunda kazanılmış bir avantaja dönüşür: Varlığın tam olarak nereye gittiğini bilirsin — çünkü oraya onu sen gönderdin.

## 4.3 — HAL 3: AGRESİF (Avcı)

**Varlık seni gördü. Artık arama yok.**

| Özellik | Davranış |
|:--|:--|
| **Tetikleyici** | Doğrudan görüş teması. Fener ışığını üzerine tutmak da buna dahildir. |
| **Hareket** | **Koşar.** Senden hızlıdır. Düz koridorda kaçarak kurtulamazsın. |
| **Kapılar** | Açar, çarpar, **kilitli kapıyı zorlar.** Kilit onu durdurmaz — yalnızca birkaç saniye alır. |
| **Hedefi** | Doğrudan sen. Tahmini rotayla keser, köşeyi döner. |
| **Ölümcüllük** | **Tam.** Temas mesafesinde ve sen ona bakıyorsan, öldürür. |
| **İniş yolu** | Görüşü kaybettiği an **donar** (bkz. 4.6), sonra Takipçi'ye iner. Bir süre hiçbir şey bulamazsa Hayalet'e döner. |
| **Senin işin** | Bölüm 4.7'deki protokol. Tartışmasız, istisnasız. |

### Hal Merdiveni

```
        ses duyuldu              görüş teması
HAYALET ───────────►  TAKİPÇİ  ───────────►  AGRESİF
   ▲                     ▲                      │
   │   arama boşa çıktı  │                      │ görüş kaybı
   └─────────────────────┴──────────────────────┘
                              (+2 sn donma)
```

**Tek yönlü kısayol yoktur:** Hayalet'ten doğrudan Agresif'e **ancak görüş temasıyla** çıkılır. Ses, Varlığı asla tek adımda Agresif yapmaz. Bu yüzden gürültü düzeltilebilir bir hatadır; **göz teması değildir.**

## 4.4 — Ses Algılama Menzilleri

Varlık dünyayı kulaklarıyla haritalar. Çıkardığın her ses bir **şiddet kademesine** sahiptir ve bu kademe, sesin kaç oda öteye taşındığını belirler.

| Eylem | Ses kademesi | Taşınma menzili |
|:--|:--:|:--|
| Durmak / çömelerek beklemek | **0 — Sessiz** | Duyulmaz |
| `Q` Çömelerek hareket | **1 — Fısıltı** | Yalnızca **aynı oda** |
| Normal yürüyüş | **2 — Orta** | Aynı oda + **komşu odalar** |
| Koşmak | **3 — Yüksek** | **Katın geniş bir bölümü** |
| Kapıyı çarpmak / `E` kasa açmak | **3 — Yüksek (anlık darbe)** | Katın geniş bir bölümü |
| `T` Mühür işleme animasyonu | **2–3 — Sürekli** | Komşu odalar; **animasyon boyunca kesintisiz yayılır** |
| `G` Fırlatılan anahtarın çarpması | **3 — Yüksek (noktasal)** | Çarptığı noktanın etrafında geniş alan |

**Kritik ayrım:** Darbe sesleri (kasa, kapı, anahtar) **tek seferliktir** — bir kez duyulur, Varlık o noktaya gider. Sürekli sesler (mühür animasyonu, koşma) **seni takip eder** — Varlık senin güncel konumunu adım adım günceller. Bu yüzden koşarak kaçmak, kaçmanın en kötü biçimidir.

## 4.5 — Kat Farkı Kuralı

Mahzen çok katlıdır ve **katlar ses yalıtımıdır.** Bu, elindeki en güçlü savunma mekaniğidir.

> ### Her kat farkı, duyulan sesin şiddetini **bir kademe** düşürür.

| Senin sesin | Aynı kat | 1 kat fark | 2+ kat fark |
|:--|:--:|:--:|:--:|
| Koşma (3) | **3 — net duyulur** | 2 — duyulur | 1 — belirsiz |
| Yürüme (2) | **2 — duyulur** | 1 — belirsiz | **0 — duyulmaz** |
| Çömelme (1) | 1 — belirsiz | **0 — duyulmaz** | **0 — duyulmaz** |

**Doğrudan sonuçlar:**

1. **İki kat fark, pratikte dokunulmazlıktır.** Varlık iki kat yukarıdaysa, normal yürüyüşle çalışabilirsin. Mühürleri bu pencerede işle.
2. **Varlık ışınlanmaz; merdiven kullanır.** Kat değiştirmesi zaman alır ve bu süre senin **bedava kazandığın** süredir. Üst katta bir gürültü duyduysan ve sen alt kattaysan, saniyelerin var — ama dakikaların yok.
3. **Agresif mod kat farkını yok etmez, kısaltır.** Koşarak merdiveni çıkar. Kat farkı seni gizler, ama sonsuza kadar saklamaz.
4. **Mühürleme kararı kat farkına göre verilir.** Varlık aynı kattaysa `T` tuşuna basmazsın. Nokta.

## 4.6 — Kritik 2 Saniye: Görüş Kaybı Donması

Mahzenin sana verdiği en küçük ve en değerli armağan budur.

> ### Varlık seninle görüş temasını kaybettiği anda, kaybettiği noktada **yaklaşık 2 saniye donar.**

Bu iki saniye boyunca:

- **Hareket etmez.** Olduğu yerde kalır.
- **Tarar.** Son gördüğü yönü ve çevresini kontrol eder.
- **Yeniden kilitlenmeye hazırdır.** Donma bitince, en son bildiği yöne doğru hücuma devam eder.

### Bu 2 saniye nasıl harcanır?

| ✅ Doğru kullanım | ❌ Ölümcül kullanım |
|:--|:--|
| Köşeyi dönüp **görüş hattından tamamen** çıkmak | Köşeden geri bakıp "gitti mi?" diye kontrol etmek |
| Bir kapıdan geçip `E` ile kapatmak | Donmanın bittiğini sanıp aynı yöne geri dönmek |
| Merdivene atlayıp **kat değiştirmek** | Donma süresince hareketsiz beklemek |
| Rotanı 90° değiştirip yeni bir koridora girmek | Bu iki saniyede envanter (`I`) açmak |

> ### ⚠ En sık yapılan ölümcül hata
> **Donma süresi içinde onun görüş alanında hareket etmek, görüşü anında yeniden kurar.** Donma bir kalkan değil; bir **mesafe penceresidir.** Görüş hattından çık, sonra hareket et. Sırayı karıştırma.

**Zihinsel sayaç:** Görüşü kestiğin an içinden "bir — iki" say. Bu sayı bitmeden **ikinci bir köşenin arkasında** olmalısın. Tek köşe yeterli değildir; tek köşe, geri döndüğünde seni hâlâ aynı hatta bırakır.

## 4.7 — EN HAYATİ KURAL: "Bakmazsan Öldüremez"

Bu kitapçıkta tek bir cümle hatırlayacaksan, bu olmalı.

> # 🔴 Varlık, yalnızca sen ona bakarken öldürebilir.

**Kuralın tam tanımı:**

- Varlık, **senin bakış koninin içinde** (ekranında görünür durumda) olduğu sürece öldürme eylemini başlatabilir.
- Öldürme eylemi başlamışken **sırtını dönersen, eylem iptal olur.**
- Bakış teması kopmuşken Varlık sana **dokunamaz.** Yanından geçebilir, arkanda durabilir, soluğunu ensende duyabilirsin — ama öldüremez.
- **Fener ışığını üzerine tutmak bakmaktır.** Işık da temas kurar. Karanlıkta ona bakmamak yetmez; ışığınla da bakmayacaksın.

### Temas Protokolü — Varlığı gördüğün an

```
1. BAKMA.        Kamerayı derhal çevir. Ona bakmak, ölüm iznini imzalamaktır.
2. FENERİ KAPAT. F. Işık, bakıştır.
3. YÜRÜ.         Sırtın ona dönük, uzaklaşan yöne. Donmak, kaçmanın zıddıdır.
4. KÖŞE AL.      Görüş hattını kes → 2 saniyelik donma başlar.
5. İKİNCİ KÖŞE.  Tek köşe yeterli değil. İkinci engeli koy.
6. KAT DEĞİŞTİR. Merdiven varsa kullan. Kat farkı, Agresif'i Takipçi'ye indirir.
7. ÇÖMEL (Q) ve SUS. Uğultu zayıflayıp boğuklaşana kadar hiçbir şey yapma.
```

### Panik anında asla yapılmayacaklar

- ❌ **Ona bakarak geri geri yürümek.** Hayatta kalma içgüdüsünün en ölümcül biçimi. Bakış teması korunuyor demektir.
- ❌ **"Nerede acaba" diye dönüp kontrol etmek.** Merakın bedeli, tek bir karedir.
- ❌ **Fener açıp onu bulmaya çalışmak.** Onu bulduğun an o seni bulur.
- ❌ **Düz bir koridorda koşmak.** Senden hızlıdır ve koşma sesi konumunu sürekli yayınlar.
- ❌ **Eşikte durup kapıyı kapatmaya çalışmak.** Kapı kapanmaz (bkz. 3.2) ve sen hâlâ görüş hattındasın.

> **Mahzenin merhameti budur:** Seni görmek öldürmez. **Seni görmek, senin onu görmene izin verir** — ve ölüm o ikinci bakıştadır. Gözlerini kaçır, yaşa.

---

# V — İLERİ DÜZEY TAKTİKLER

## 5.1 — Mühürleme Stratejisi

Mühür işlemek (`T`) oyunun hem tek kazanma yolu, hem de tek **gönüllü savunmasızlık** anıdır. Animasyon boyunca hareket edemezsin ve sürekli ses yayarsın. Bir mühür, güvenli bir anda işlenmek üzere beklemeyi hak eder.

### Mühürleme Penceresi — üç koşul

Üçü birden sağlanmadan `T` tuşuna basmazsın:

1. **Varlık en az bir kat uzakta.** (Tercihen iki.) Uğultu boğuk ve zayıf olmalı.
2. **Varlık Hayalet veya sakinleşmiş Takipçi halinde.** Agresif halde mühür işlemek intihardır.
3. **Arkandaki kapı kapalı ve kilitli.** (`E` → `K`) Animasyon sırasında kaçamazsın; kilit senin yerine zaman kazanır.

### Sıralama Doktrini

| Aşama | Yaklaşım | Mantık |
|:--|:--|:--|
| **Açılış** | Varlığın başlangıç uğultusunun **en uzak** olduğu kata git. | En sessiz katta en çok iş çıkarılır. |
| **Orta oyun** | Katları **tek tek bitir**, katlar arasında zıplama. | Her kat geçişi bir risk ve bir gürültüdür. Kat içinde kalmak mesafeyi öngörülebilir tutar. |
| **Toplama** | Mühürleri bulduğun anda işleme; **2–3 mührü biriktirip** sakin bir pencerede sırayla işle. | Her `T` bir gürültü darbesidir. Tek bir sakin pencerede üç mühür işlemek, üç ayrı riskten iyidir. |
| **Final** | Çıkış merdivenine **en yakın** mühürleri en sona bırak. | Son mühür işlendiğinde mahzen tepki verir ve Varlık kalıcı olarak saldırgan hale geçer. O anda çıkışa yakın olmak istersin. |

> ### ⚠ Son Mühür Uyarısı
> **20. mühür işlendikten sonra sinsilik dönemi biter.** Mahzen kapanmaya başlar, uğultu yükselir ve Varlık seni arayan değil, **kesmeye çalışan** bir şeye dönüşür.
> Son mühre basmadan önce: çıkış rotanı ezberle, kapıları önceden aç, feneri kapat ve **koşacağın hattı önceden seç.** Son mühür bir kutlama değil, bir **start tabancasıdır.**

### Mühürleme sırasında ses duyarsan

Animasyon kesilebilir. **Kes.** Yarım mühür, yarım ölümden iyidir — mührü kaybetmezsin, envanterinde kalır. Çekil, sessizliğe gir, dön ve yeniden başla. Mahzen seni acele etmeye zorlamaz; yalnızca cezalandırır.

## 5.2 — Anahtar Fırlatarak Dikkat Dağıtma (`G`)

Bu, Mahzen'in tek **saldırı** mekaniğidir — Varlığa zarar vermez, ama onu **sen yönetirsin.**

### Nasıl çalışır?

1. `I` ile fırlatmak istediğin anahtarı seç.
2. Göndermek istediğin yöne bak.
3. `G` ile fırlat.
4. Anahtar çarptığı yerde **Kademe 3 (Yüksek)** bir noktasal ses üretir.
5. Varlık, o sesi duyarsa **Takipçi** haline geçer ve **çarpma noktasına** gider — sana değil.
6. Anahtar düştüğü yerde kalır; **kaybolmaz,** sonra geri alınabilir.

### Taktik Kullanımlar

| Durum | Uygulama |
|:--|:--|
| **Kat tuzağı (en güçlü kullanım)** | Merdivenden **alt kata** bir anahtar fırlat, sonra **üst kata** çık. Varlık aşağı iner; sen iki kat fark kazanır ve pratikte dokunulmaz olursun (bkz. 4.5). |
| **Koridor temizleme** | Geçmen gereken koridorun **ötesine** fırlat. Varlık koridoru senin geldiğin yöne değil, sesin gittiği yöne doğru boşaltır. |
| **Mühürleme penceresi açma** | Mühür işlemeden hemen önce, mühür odasından **uzak bir yöne** fırlat. Animasyon süresi boyunca Varlık yanlış hedefi tarar. |
| **Agresif modu kırmak** | Köşeyi dönüp görüşü kestikten (2 sn donma) sonra, **geldiğin yöne** fırlat. Varlık inerken, en son bildiği yönü ses tarafından onaylanmış sanır. |
| **Çıkış hazırlığı** | Son mühre basmadan önce, çıkış rotasının **tersine** bir anahtar bırak. Son mührün gürültüsüyle yarışacak bir sahte hedef. |

### Kurallar ve Sınırlar

- **Bütçen +2'dir.** 22 anahtar, 20 kasa: iki anahtar güvenle harcanabilir. Daha fazlası, sonradan yerden toplamayı gerektirir — mümkündür ama zahmetlidir.
- **Kilitli bir odaya fırlatma.** Anahtar kaybolmaz, ama geri alman için o odayı açman gerekir. Boşa risk.
- **Varlık sesi duyamayacak kadar uzaktaysa, fırlatma bedavaya gider.** İki kat ötedeki Varlık, fırlattığın anahtarı duymaz. Dikkat dağıtma, onu **duyabildiğinde** işe yarar.
- **Agresif modda, görüş hattı açıkken fırlatma işe yaramaz.** Seni görüyorsa ses onu ikna etmez. Önce görüşü kes, sonra yanılt.

## 5.3 — Ses ve Uğultu Takibi

Varlık sessiz değildir. Yanında bir **uğultu** taşır — bu, mahzenin sana kurduğu radardır. Uğultuyu okumayı öğrenmek, bu oyunu öğrenmektir.

### Uğultu Okuma Tablosu

| Duyduğun | Anlamı | Yapman gereken |
|:--|:--|:--|
| **Zayıf, boğuk, tok uğultu** | Varlık **farklı bir katta,** uzakta. | En güvenli pencere. Mühürle, kasa aç, kat temizle. |
| **Zayıf ama net/tiz uğultu** | Varlık **aynı katta,** ama birkaç oda ötede. | Çömel (`Q`). Gürültülü iş yapma. Mesafeyi koru. |
| **Güçlenen uğultu** | Yaklaşıyor. Muhtemelen Takipçi. | İşi bırak, ters yöne çekil, kapıyı kapat ve kilitle. |
| **Güçlü ve net uğultu** | **Komşu oda veya aynı oda.** | Hareket etme, fener kapalı, çömel. Kapıya bakma. |
| **Uğultunun aniden kesilmesi** | **En tehlikeli sinyal.** Donma anı veya pusu. | Dur. Hiçbir şey yapma. Uğultu geri dönene kadar bekle. |
| **Uğultuyla birlikte koşma sesi** | **Agresif.** Seni görmüş olabilir. | Temas Protokolü (4.7). Derhal. |

### Üç İleri Düzey Dinleme Tekniği

**1 — Tonla kat ayırt etme.** Uğultunun şiddeti mesafeyi, **tonu** katı söyler. Boğuk ve tok ses = kat farkı var; net ve tiz ses = aynı kat. İki zayıf uğultu aynı yüksekliğe sahip olabilir ama farklı tehditlerdir. **Tonu dinle, şiddeti değil.**

**2 — Sessizliğin okunması.** Mahzen hiçbir zaman tam sessiz değildir. Uğultunun kesilmesi, Varlığın gittiği anlamına **gelmez** — donduğu ya da beklediği anlamına gelir. Sessizliğe güvenerek `T` tuşuna basan oyuncu sayısı, gürültüde basanlardan fazladır.

**3 — Kendi sesini bütçelemek.** Varlığın seni duyması kadar, **senin onu duyabilmen** de hayatidir. Koşarken kendi ayak sesin uğultuyu bastırır — yani koşmak seni hem duyulabilir hem de **sağır** yapar. Her koşunun sonunda durup çömel ve **bir kez dinle.** Bu üç saniye, bilgi karşılığında ödenen en kârlı bedeldir.

---

# VI — HIZLI BAŞVURU KARTI

## Sayılar

```
20 oda  •  20 kasa  •  20 mühür  •  22 anahtar  •  +2 hata toleransı
KİLİTLENME İMKÂNSIZDIR — ilerleme her zaman mümkündür.
```

## Tuşlar

```
E  Etkileşim (kapı / kasa / eşya al)     F  Fener aç-kapat
T  Mühür işle (hareketsiz + gürültülü)   K  Kapıyı kilitle
Q  Çömel / sessiz hareket                I  Envanter
G  Anahtar fırlat (dikkat dağıt)
```

## Varlık Halleri

```
HAYALET   → Bilmiyor.   ÇALIŞMA ZAMANI.
TAKİPÇİ   → Duydu.      İŞİ BIRAK, SESSİZCE ÇEKİL.
AGRESİF   → Gördü.      BAKMA. FENER KAPALI. İKİ KÖŞE. KAT DEĞİŞTİR.
```

## Beş Değişmez Kural

1. **Bakmazsan öldüremez.** Temasta ilk hareket, kamerayı çevirmektir.
2. **Eşikte durulmaz.** Geç, kapat, sonra iş yap.
3. **Mühür, kat farkıyla işlenir.** Varlık aynı kattaysa `T` yok.
4. **Görüş kaybından sonra 2 saniyen var.** İkinci köşeye ulaş; geri bakma.
5. **Tonu dinle, şiddeti değil.** Boğuk uğultu = farklı kat = güvenli pencere.

## Ölüm Nedenleri Kontrol Listesi

> Öldüysen, bunlardan biri olmuştur:
> - Ona baktın (ya da fenerini üstüne tuttun).
> - Eşikte durdun.
> - Varlık aynı kattayken mühür işledin.
> - Görüş kaybından sonra geri dönüp kontrol ettin.
> - Düz koridorda koştun.
> - Kesilen uğultuyu "gitti" diye okudun.

---

> *Yirmi mühür. Yirmi iki anahtar. Bir merdiven.*
> *Matematik seni kurtarır; merakın öldürür.*
>
> **Fenerini kapat. Dinle. Ve bakma.**

---

<sub>**Kitapçık Künyesi** — Bu el kitapçığı Mahzen'in kural setinden (oda/kasa/mühür/anahtar dağılımı, tuş dizilimi, eşik mantığı, Varlık halleri, ses ve kat farkı kuralları, görüş kaybı donması ve temas kuralı) derlenmiştir. Oyun dengesi güncellendiğinde bu belgedeki menzil kademeleri, donma süresi ve sıralama doktrini yeniden gözden geçirilmelidir.</sub>
