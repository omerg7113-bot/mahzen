#!/usr/bin/env python3
"""
yt_indie_horror_scout.py'nin API ANAHTARI GEREKTIRMEYEN surumu.

Google API yerine yt-dlp ile calisir: YouTube aramasini ve kanal sayfalarini
dogrudan okur. Kota yok, anahtar yok. Karsiliginda daha yavastir, YouTube
hiz siniri uygulayabilir ve YouTube arayuzunu degistirdiginde yt-dlp
guncellenene kadar bozulabilir. Guvenilirlik onceligin ise API surumunu kullan.

Bu betik YouTube'a dogrudan baglanir; Claude Code bulut oturumunda ag politikasi
youtube.com'u engelledigi icin ORADA CALISMAZ. Kendi makinende calistir.

Kurulum:
    pip install -U yt-dlp

Kullanim:
    python3 tools/yt_scout_nokey.py --target 100 --out kanallar.csv
"""

import argparse
import csv
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
# Anahtar kelimeler, odak etiketleme ve abone bandi mantigi API surumuyle ortak
# tutuluyor; iki betigin kriterleri birbirinden kaymasin diye oradan aliniyor.
from yt_indie_horror_scout import (  # noqa: E402
    DEFAULT_QUERIES,
    classify_channel,
    focus_label,
    horror_ratio,
    sub_bucket,
)

try:
    from yt_dlp import YoutubeDL
except ImportError:
    raise SystemExit("yt-dlp kurulu degil.  Kur:  pip install -U yt-dlp\n")


BASE_OPTS = {
    "quiet": True,
    "no_warnings": True,
    "skip_download": True,
    "extract_flat": "in_playlist",
    "ignoreerrors": True,
}


def search_channels(queries, per_query, verbose):
    """Arama sonuclarindan benzersiz kanal kimlikleri topla."""
    found = {}
    with YoutubeDL(BASE_OPTS) as ydl:
        for i, q in enumerate(queries, 1):
            try:
                info = ydl.extract_info(f"ytsearch{per_query}:{q}", download=False)
            except Exception as e:                      # noqa: BLE001
                print(f"  ! {q!r} arama hatasi: {e}", file=sys.stderr)
                continue
            for e in (info or {}).get("entries") or []:
                if not e:
                    continue
                cid = e.get("channel_id")
                if cid:
                    found.setdefault(cid, e.get("channel") or "")
            if verbose:
                print(f"  [{i}/{len(queries)}] {q!r} -> toplam {len(found)} kanal",
                      file=sys.stderr)
    return found


def channel_detail(ydl, channel_id, video_limit):
    """Kanalin abone sayisi, handle'i ve son video basliklari."""
    url = f"https://www.youtube.com/channel/{channel_id}/videos"
    opts = dict(BASE_OPTS, playlistend=video_limit)
    with YoutubeDL(opts) as y:
        try:
            info = y.extract_info(url, download=False)
        except Exception:                               # noqa: BLE001
            return None
    if not info:
        return None

    handle = info.get("uploader_id") or ""
    if not handle.startswith("@"):
        uploader_url = info.get("uploader_url") or ""
        if "/@" in uploader_url:
            handle = "@" + uploader_url.rsplit("/@", 1)[1]
        else:
            handle = ""

    titles = [e.get("title", "") for e in (info.get("entries") or []) if e]
    return {
        "name": info.get("channel") or info.get("title") or "",
        "handle": handle,
        "subs": info.get("channel_follower_count"),
        "titles": titles,
        "description": info.get("description") or "",
    }


def main():
    ap = argparse.ArgumentParser(
        description="Indie korku YouTuber avcisi (yt-dlp, anahtarsiz)")
    ap.add_argument("--min-subs", type=int, default=5000)
    ap.add_argument("--max-subs", type=int, default=30000)
    ap.add_argument("--target", type=int, default=100)
    ap.add_argument("--per-query", type=int, default=40,
                    help="Sorgu basina taranacak video sayisi")
    ap.add_argument("--video-limit", type=int, default=50,
                    help="Korku orani icin bakilacak son video sayisi")
    ap.add_argument("--horror-ratio", type=float, default=0.45)
    ap.add_argument("--sleep", type=float, default=1.0,
                    help="Kanallar arasi bekleme (saniye) — hiz siniri icin")
    ap.add_argument("--keep-kinds", default="oynayici,belirsiz",
                    help="Hangi kanal turleri listeye girsin "
                         "(oynayici,belirsiz,gelistirici,film,baska_oyun)")
    ap.add_argument("--report", default="",
                    help="Elenenleri gerekceleriyle bu CSV'ye yaz")
    ap.add_argument("--exact-subs", action="store_true")
    ap.add_argument("--out", default="indie_horror_kanallar.csv")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    verbose = not args.quiet

    if verbose:
        print("1/3  Korku oyunu aramalari taraniyor...", file=sys.stderr)
    candidates = search_channels(DEFAULT_QUERIES, args.per_query, verbose)
    if verbose:
        print(f"     {len(candidates)} aday kanal bulundu.", file=sys.stderr)
        print("2/3  Kanal sayfalari okunuyor (abone + handle + son videolar)...",
              file=sys.stderr)

    keep_kinds = {k.strip() for k in args.keep_kinds.split(",") if k.strip()}
    rejected = []
    rows = []
    seen = set()
    with YoutubeDL(BASE_OPTS) as ydl:
        for n, cid in enumerate(candidates, 1):
            if len(rows) >= args.target:
                break
            d = channel_detail(ydl, cid, args.video_limit)
            time.sleep(args.sleep)
            if not d or not d["handle"] or d["subs"] is None:
                continue
            if not (args.min_subs <= d["subs"] <= args.max_subs):
                continue
            if d["handle"].lower() in seen:
                continue
            ratio = horror_ratio(d["titles"])
            if ratio < args.horror_ratio:
                rejected.append([d["name"], d["handle"], d["subs"],
                                 "korku_orani_dusuk", f"%{ratio*100:.0f}"])
                continue

            kind, why = classify_channel(d["name"], d["handle"], d["titles"],
                                         d["description"])
            if kind not in keep_kinds:
                rejected.append([d["name"], d["handle"], d["subs"], kind, why])
                if verbose:
                    print(f"     - {d['name'][:40]:<40} {d['handle']:<24} "
                          f"ELENDI: {kind} ({why})", file=sys.stderr)
                continue

            rows.append([
                0,
                d["name"].strip(),
                d["handle"],
                str(d["subs"]) if args.exact_subs else sub_bucket(d["subs"]),
                "Belirtilmemis",   # yt-dlp kanal ulkesini guvenilir vermiyor
                focus_label(d["titles"], d["description"]),
            ])
            seen.add(d["handle"].lower())
            if verbose:
                print(f"     + {d['name'][:40]:<40} {d['handle']:<24} "
                      f"{d['subs']:>7,}  korku {ratio:.0%}  "
                      f"[{len(rows)}/{args.target}]", file=sys.stderr)

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
    if len(rows) < args.target:
        print(f"Hedefe ({args.target}) ulasilamadi. Deneyebileceklerin:\n"
              f"  --per-query 60         her sorguda daha cok video tara\n"
              f"  --horror-ratio 0.35    odak esigini gevset\n"
              f"  --max-subs 50000       bandi genislet", file=sys.stderr)


if __name__ == "__main__":
    main()
