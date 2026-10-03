#!/usr/bin/env python3
"""
Indie korku oyunu YouTuber avcisi — Mahzen tanitim listesi uretir.

YouTube Data API v3 kullanir. Her satir API'den gelir; hicbir veri tahmin degildir.

Ne yapar:
  1. Korku oyunu anahtar kelimeleriyle SON videolari arar (aktif kanallari yakalar).
  2. Bulunan kanallarin abone sayisini, handle'ini ve ulkesini ceker.
  3. Abone bandina gore filtreler (varsayilan 5.000-30.000).
  4. Kanalin SON 50 videosunun basliklarina bakip "korku odak orani" hesaplar;
     esigin altinda kalan genel oyun kanallarini eler. Asil kalite filtresi budur.
  5. Istenen kolonlarda CSV yazar.

Kullanim:
  export YT_API_KEY="..."
  python3 tools/yt_indie_horror_scout.py --target 100 --out kanallar.csv

Kota: Ucretsiz katman gunde 10.000 birim. search.list = 100 birim/cagri,
digerleri 1 birim. Varsayilan ayarlar ~4.000-5.000 birim harcar.
Yetmezse ertesi gun --resume ile ayni dosyaya ekleme yapabilirsin.
"""

import argparse
import csv
import json
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timedelta, timezone

API = "https://www.googleapis.com/youtube/v3"

# --- Arama sorgulari -------------------------------------------------------
# Genis tutuldu: tur adlari + platform adlari + yaygin alt tur etiketleri.
DEFAULT_QUERIES = [
    "indie horror game gameplay",
    "indie horror no commentary",
    "itch.io horror game",
    "free indie horror game",
    "short horror game playthrough",
    "psx horror game",
    "ps1 style horror game",
    "retro horror game indie",
    "analog horror game",
    "found footage horror game",
    "haunted ps1 demo disc",
    "dread xp horror game",
    "puppet combo gameplay",
    "chilla's art gameplay",
    "steam indie horror game",
    "horror game demo gameplay",
    "new indie horror games",
    "scary indie game playthrough",
    "liminal horror game",
    "backrooms horror game",
    "first person horror indie",
    "survival horror indie game",
    "psychological horror indie game",
    "horror game let's play indie",
    "unity horror game indie",
    "lo-fi horror game",
    "vhs horror game",
    "creepy indie game gameplay",
    "horror oyunu indie",          # TR
    "korku oyunu bagimsiz",        # TR
    "juego de terror indie",       # ES
    "jeu d'horreur indé",          # FR
    "indie horror spiel",          # DE
    "jogo de terror indie",        # PT
    "インディー ホラーゲーム",        # JA
]

# --- Korku odak tespiti ----------------------------------------------------
HORROR_WORDS = [
    "horror", "scary", "creepy", "terror", "haunted", "haunting", "nightmare",
    "ghost", "demon", "cursed", "eerie", "dread", "spooky", "fear", "macabre",
    "paranormal", "slasher", "gore", "psx horror", "ps1 horror", "analog horror",
    "found footage", "backrooms", "liminal", "korku", "dehset", "dehşet",
    "terror", "miedo", "gruselspiel", "ホラー", "怖い",
]

FOCUS_GROUPS = {
    "Retro/PSX korku": ["psx", "ps1", "ps2", "retro horror", "lo-fi", "vhs",
                        "low poly", "demo disc", "haunted ps1"],
    "Analog/found footage korku": ["analog horror", "found footage", "vhs tape",
                                   "backrooms", "liminal"],
    "itch.io kisa korku": ["itch.io", "itch io", "short horror", "free horror",
                           "demo"],
    "Sistemik/co-op korku": ["phasmophobia", "co-op horror", "coop horror",
                             "multiplayer horror", "lethal company", "devour"],
}

TOKEN = re.compile(r"[a-z0-9']+")

# --- Kanal turu tespiti ----------------------------------------------------
# Korku filtresi "bu kanal korku mu?" sorusunu cozer ama "bu kanal OYNUYOR mu?"
# sorusunu cozmez. Gelistirici studyolari, kisa film kanallari ve baska bir
# oyuna adanmis kanallar da korku kelimelerinden gecer. Outreach icin ise
# yaramazlar: oyununu oynamazlar.

DEV_NAME_WORDS = [
    "studio", "studios", "interactive", "softworks", "entertainment",
    "games inc", "game studio", "_dev", "dev ", " dev", "devteam", "team",
]
DEV_TITLE_WORDS = [
    "devlog", "dev log", "development", "announcement trailer", "teaser",
    "wishlist", "out now on steam", "early access", "patch notes",
    "kickstarter", "our game", "my game", "release date", "coming soon",
    "demo is out", "now available",
]
FILM_WORDS = [
    "short film", "kisa film", "animation", "animated", "cgi", "sfm",
    "movie", "cinematic",
]
PLAYER_WORDS = [
    "gameplay", "playthrough", "let's play", "lets play", "no commentary",
    "full game", "walkthrough", "part ", "ep.", "episode", "first time",
    "reaction", "played", "ending explained", "all endings", "oynuyorum",
    "bolum",
]
# Kanal adinda/handle'inda baska bir oyunun adi varsa odak orada demektir.
OTHER_GAME_WORDS = [
    "minecraft", "roblox", "fortnite", "gta", "fifa", "valorant", "league of",
    "among us", "terraria", "sims",
]

STOPWORDS = {
    "the", "a", "an", "of", "in", "on", "and", "or", "to", "is", "it", "my",
    "i", "we", "you", "this", "that", "with", "for", "new", "game", "games",
    "part", "ep", "episode", "full", "video", "ve", "bir", "bu",
}


def _repetition(titles):
    """Basliklarin kacinda ayni anlamli kelime geciyor (0-1).

    Gelistirici kanallari tek bir oyunun adini her baslikta tekrarlar;
    oynayici kanallari her videoda baska bir oyundan soz eder.
    """
    if len(titles) < 5:
        return 0.0
    doc_freq = {}
    for t in titles:
        for w in set(TOKEN.findall(t.lower())):
            if w in STOPWORDS or len(w) < 4 or w in HORROR_WORDS:
                continue
            doc_freq[w] = doc_freq.get(w, 0) + 1
    if not doc_freq:
        return 0.0
    return max(doc_freq.values()) / len(titles)


def classify_channel(name, handle, titles, description):
    """('oynayici'|'gelistirici'|'film'|'baska_oyun'|'belirsiz', gerekce)."""
    ident = f"{name} {handle}".lower()
    blob = " ".join(titles).lower()
    desc = (description or "").lower()

    for w in OTHER_GAME_WORDS:
        if w in ident:
            return "baska_oyun", f"ad/handle '{w}' iceriyor"

    name_dev = [w for w in DEV_NAME_WORDS if w in ident]
    title_dev = sum(blob.count(w) for w in DEV_TITLE_WORDS)
    desc_dev = sum(desc.count(w) for w in DEV_TITLE_WORDS)
    rep = _repetition(titles)
    player_hits = sum(blob.count(w) for w in PLAYER_WORDS)
    film_hits = sum(blob.count(w) for w in FILM_WORDS) +         sum(1 for w in ("film", "films") if w in ident) * 3

    if name_dev and player_hits < 3:
        return "gelistirici", f"ad/handle '{name_dev[0]}' + oynayici sinyali zayif"
    if rep >= 0.6 and player_hits < 5:
        return "gelistirici", f"basliklarin %{rep*100:.0f}'inde ayni oyun adi"
    if (title_dev + desc_dev) >= 4 and player_hits < 4:
        return "gelistirici", "devlog/trailer/wishlist dili baskin"
    if film_hits >= 4 and player_hits < 3:
        return "film", "kisa film/animasyon sinyali baskin"
    if player_hits >= 3:
        return "oynayici", f"{player_hits} oynayici sinyali"
    return "belirsiz", "sinyal yetersiz — elle bak"




def api_get(endpoint, params, key, quota_box, cost):
    """Tek bir API cagrisi. Kota sayacini artirir, hatayi anlasilir sekilde yukseltir."""
    params = dict(params)
    params["key"] = key
    url = f"{API}/{endpoint}?" + urllib.parse.urlencode(params)
    req = urllib.request.Request(url, headers={"Accept": "application/json"})
    for attempt in range(4):
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                quota_box["used"] += cost
                return json.load(resp)
        except urllib.error.HTTPError as e:
            body = e.read().decode("utf-8", "replace")
            if e.code == 403 and "quotaExceeded" in body:
                raise SystemExit(
                    "\nKOTA BITTI. Bugunluk 10.000 birim doldu.\n"
                    "Yarin --resume ile ayni cikti dosyasina devam edebilirsin.\n"
                )
            if e.code in (500, 503) and attempt < 3:
                time.sleep(2 ** attempt)
                continue
            raise SystemExit(f"\nAPI hatasi {e.code} ({endpoint}): {body[:400]}\n")
        except urllib.error.URLError as e:
            if attempt < 3:
                time.sleep(2 ** attempt)
                continue
            raise SystemExit(f"\nAga ulasilamadi: {e}\n")
    raise SystemExit("\nAPI cagrisi tekrar tekrar basarisiz oldu.\n")


def horror_ratio(titles):
    """Baslik listesinin yuzde kaci korku kelimesi iceriyor."""
    if not titles:
        return 0.0
    hits = 0
    for t in titles:
        low = t.lower()
        if any(w in low for w in HORROR_WORDS):
            hits += 1
    return hits / len(titles)


def focus_label(titles, description):
    """Baskin alt turu etiketle."""
    blob = (" ".join(titles) + " " + description).lower()
    scores = {}
    for label, words in FOCUS_GROUPS.items():
        scores[label] = sum(blob.count(w) for w in words)
    best = max(scores, key=scores.get)
    if scores[best] == 0:
        return "Genel indie korku"
    runner = sorted(scores.values(), reverse=True)
    if len(runner) > 1 and runner[1] > 0 and runner[1] >= runner[0] * 0.6:
        second = [k for k, v in scores.items() if v == runner[1] and k != best]
        if second:
            return f"{best} + {second[0]}"
    return best


def sub_bucket(n):
    if n < 5000:
        return "<5K"
    if n < 10000:
        return "5K-10K"
    if n < 20000:
        return "10K-20K"
    if n < 30000:
        return "20K-30K"
    if n < 50000:
        return "30K-50K"
    if n < 100000:
        return "50K-100K"
    return "100K+"


def collect_channel_ids(key, quota, queries, months, per_query, verbose):
    """Son N ayda korku videosu yuklemis kanallarin kimliklerini topla."""
    after = (datetime.now(timezone.utc) - timedelta(days=30 * months)).strftime(
        "%Y-%m-%dT%H:%M:%SZ"
    )
    found = {}
    for i, q in enumerate(queries, 1):
        page, got = None, 0
        while got < per_query:
            params = {
                "part": "snippet",
                "type": "video",
                "q": q,
                "maxResults": min(50, per_query - got),
                "order": "date",
                "publishedAfter": after,
                "videoCategoryId": "20",  # Gaming
            }
            if page:
                params["pageToken"] = page
            data = api_get("search", params, key, quota, 100)
            items = data.get("items", [])
            for it in items:
                sn = it.get("snippet", {})
                cid = sn.get("channelId")
                if cid:
                    found.setdefault(cid, sn.get("channelTitle", ""))
            got += len(items)
            page = data.get("nextPageToken")
            if not page or not items:
                break
        if verbose:
            print(f"  [{i}/{len(queries)}] {q!r} -> toplam {len(found)} kanal "
                  f"(kota {quota['used']})", file=sys.stderr)
    return found


def fetch_channels(key, quota, channel_ids):
    """channels.list ile 50'serli gruplar halinde detay cek."""
    out = []
    ids = list(channel_ids)
    for i in range(0, len(ids), 50):
        chunk = ids[i:i + 50]
        data = api_get(
            "channels",
            {"part": "snippet,statistics,contentDetails", "id": ",".join(chunk),
             "maxResults": 50},
            key, quota, 1,
        )
        out.extend(data.get("items", []))
    return out


def recent_titles(key, quota, uploads_playlist, limit=50):
    data = api_get(
        "playlistItems",
        {"part": "snippet", "playlistId": uploads_playlist, "maxResults": limit},
        key, quota, 1,
    )
    return [it["snippet"]["title"] for it in data.get("items", [])]


def main():
    ap = argparse.ArgumentParser(description="Indie korku YouTuber avcisi")
    ap.add_argument("--min-subs", type=int, default=5000)
    ap.add_argument("--max-subs", type=int, default=30000)
    ap.add_argument("--target", type=int, default=100, help="Kac kanal toplanacak")
    ap.add_argument("--months", type=int, default=6,
                    help="Son kac ayda video yuklemis olsun (aktiflik filtresi)")
    ap.add_argument("--per-query", type=int, default=50,
                    help="Sorgu basina taranacak video sayisi (50 = 100 birim)")
    ap.add_argument("--horror-ratio", type=float, default=0.45,
                    help="Son videolarin en az bu orani korku olmali (0-1)")
    ap.add_argument("--keep-kinds", default="oynayici,belirsiz",
                    help="Hangi kanal turleri listeye girsin "
                         "(oynayici,belirsiz,gelistirici,film,baska_oyun)")
    ap.add_argument("--report", default="",
                    help="Elenenleri gerekceleriyle bu CSV'ye yaz")
    ap.add_argument("--exact-subs", action="store_true",
                    help="Bant yerine tam abone sayisini yaz")
    ap.add_argument("--out", default="indie_horror_kanallar.csv")
    ap.add_argument("--resume", action="store_true",
                    help="Cikti dosyasi varsa uzerine yazma, eksikleri tamamla")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    key = os.environ.get("YT_API_KEY")
    if not key:
        raise SystemExit(
            "YT_API_KEY tanimli degil.\n"
            "  https://console.cloud.google.com -> yeni proje -> "
            "'YouTube Data API v3'u etkinlestir -> Credentials -> API key\n"
            "  export YT_API_KEY=\"...\"\n"
        )

    verbose = not args.quiet
    quota = {"used": 0}

    keep_kinds = {k.strip() for k in args.keep_kinds.split(",") if k.strip()}
    rejected = []
    seen_handles = set()
    rows = []
    if args.resume and os.path.exists(args.out):
        with open(args.out, newline="", encoding="utf-8") as f:
            for r in csv.DictReader(f):
                rows.append([r[k] for k in
                             ["No", "Kanal_Adi", "Handle", "Tahmini_Abone_Araligi",
                              "Dil_Ulke", "Odak_Turu"]])
                seen_handles.add(r["Handle"].lower())
        if verbose:
            print(f"Devam: {len(rows)} kanal zaten var.", file=sys.stderr)

    if verbose:
        print(f"1/3  Son {args.months} ayin korku videolari taraniyor...",
              file=sys.stderr)
    candidates = collect_channel_ids(key, quota, DEFAULT_QUERIES, args.months,
                                     args.per_query, verbose)
    if verbose:
        print(f"     {len(candidates)} aday kanal bulundu.", file=sys.stderr)
        print("2/3  Abone sayilari ve handle'lar cekiliyor...", file=sys.stderr)

    channels = fetch_channels(key, quota, candidates.keys())

    in_band = []
    for ch in channels:
        stats = ch.get("statistics", {})
        if stats.get("hiddenSubscriberCount"):
            continue
        try:
            subs = int(stats.get("subscriberCount", "0"))
        except ValueError:
            continue
        if not (args.min_subs <= subs <= args.max_subs):
            continue
        in_band.append((subs, ch))
    in_band.sort(key=lambda x: -x[0])

    if verbose:
        print(f"     {len(in_band)} kanal {args.min_subs:,}-{args.max_subs:,} "
              f"bandinda.", file=sys.stderr)
        print(f"3/3  Korku odak orani olculuyor (esik {args.horror_ratio:.0%})...",
              file=sys.stderr)

    for subs, ch in in_band:
        if len(rows) >= args.target:
            break
        sn = ch.get("snippet", {})
        handle = sn.get("customUrl", "")
        if not handle:
            continue
        if not handle.startswith("@"):
            handle = "@" + handle
        if handle.lower() in seen_handles:
            continue

        uploads = (ch.get("contentDetails", {})
                     .get("relatedPlaylists", {})
                     .get("uploads"))
        if not uploads:
            continue
        titles = recent_titles(key, quota, uploads)
        ratio = horror_ratio(titles)
        if ratio < args.horror_ratio:
            rejected.append([sn.get("title", ""), handle, subs,
                             "korku_orani_dusuk", f"%{ratio*100:.0f}"])
            continue

        kind, why = classify_channel(sn.get("title", ""), handle, titles,
                                     sn.get("description", ""))
        if kind not in keep_kinds:
            rejected.append([sn.get("title", ""), handle, subs, kind, why])
            if verbose:
                print(f"     - {sn.get('title','')[:40]:<40} {handle:<24} "
                      f"ELENDI: {kind} ({why})", file=sys.stderr)
            continue

        country = sn.get("country", "")
        lang = (sn.get("defaultLanguage") or "").upper()[:2]
        dil_ulke = "/".join([p for p in (lang, country) if p]) or "Belirtilmemis"

        rows.append([
            0,
            sn.get("title", "").strip(),
            handle,
            str(subs) if args.exact_subs else sub_bucket(subs),
            dil_ulke,
            focus_label(titles, sn.get("description", "")),
        ])
        seen_handles.add(handle.lower())
        if verbose:
            print(f"     + {sn.get('title','')[:40]:<40} {handle:<24} "
                  f"{subs:>7,}  korku {ratio:.0%}", file=sys.stderr)

    for i, r in enumerate(rows, 1):
        r[0] = i

    with open(args.out, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["No", "Kanal_Adi", "Handle", "Tahmini_Abone_Araligi",
                    "Dil_Ulke", "Odak_Turu"])
        w.writerows(rows)

    if args.report and rejected:
        with open(args.report, "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f)
            w.writerow(["Kanal_Adi", "Handle", "Abone", "Eleme_Nedeni", "Detay"])
            w.writerows(rejected)
        print(f"{len(rejected)} elenen kanal -> {args.report}", file=sys.stderr)

    print(f"\n{len(rows)} kanal yazildi -> {args.out}", file=sys.stderr)
    print(f"Harcanan kota: ~{quota['used']} / 10.000 birim", file=sys.stderr)
    if len(rows) < args.target:
        print(f"Hedefe ({args.target}) ulasilamadi. Deneyebilecegin ayarlar:\n"
              f"  --months 12            daha genis zaman araligi\n"
              f"  --horror-ratio 0.35    odak esigini gevset\n"
              f"  --max-subs 50000       bandi genislet\n"
              f"  --resume               yarin kalan kotayla devam et",
              file=sys.stderr)


if __name__ == "__main__":
    main()
