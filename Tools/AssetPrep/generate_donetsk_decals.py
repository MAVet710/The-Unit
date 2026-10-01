from pathlib import Path
import json
import hashlib
import math

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "ExternalAssets" / "DonetskDecals"
OUT.mkdir(parents=True, exist_ok=True)
SIZE = 1024
SEED = 710

SPECS = {
    "CrackedPlaster": ("facade_damage", (170, 166, 154), (180, 180)),
    "ChippedPaint": ("facade_damage", (126, 115, 101), (220, 180)),
    "ExposedSubstrate": ("facade_damage", (104, 91, 78), (260, 220)),
    "Soot": ("water_grime", (45, 43, 40), (240, 360)),
    "RainStreaks": ("water_grime", (72, 77, 74), (220, 420)),
    "RustDrips": ("rust", (128, 57, 28), (180, 360)),
    "PatchedAsphalt": ("road_wear", (50, 52, 52), (320, 220)),
    "AsphaltCracks": ("road_wear", (33, 34, 34), (300, 300)),
    "TireWear": ("road_wear", (44, 44, 42), (420, 180)),
    "CurbGrime": ("water_grime", (68, 66, 60), (360, 140)),
    "UtilityMarking": ("signage_remnants", (180, 149, 65), (180, 180)),
    "FadedSignage": ("signage_remnants", (146, 58, 48), (360, 180)),
    "WaterStain": ("water_grime", (73, 77, 70), (260, 260)),
    "ImpactScorch": ("mission_impact", (36, 33, 31), (220, 220)),
}


def noise(seed, scale=1.0):
    rng = np.random.default_rng(seed)
    arr = rng.random((SIZE, SIZE), dtype=np.float32)
    img = Image.fromarray(np.uint8(arr * 255), "L")
    blur = max(1.0, 26.0 * scale)
    return np.asarray(img.filter(ImageFilter.GaussianBlur(blur)), dtype=np.float32) / 255.0


def blank():
    return Image.new("L", (SIZE, SIZE), 0)


def crack_mask(seed, branches=26, width=(2, 7)):
    rng = np.random.default_rng(seed)
    img = blank()
    draw = ImageDraw.Draw(img)
    for _ in range(branches):
        x = int(rng.integers(160, SIZE - 160))
        y = int(rng.integers(160, SIZE - 160))
        pts = [(x, y)]
        angle = float(rng.uniform(0, math.tau))
        length = int(rng.integers(90, 340))
        steps = int(rng.integers(4, 9))
        for step in range(steps):
            angle += float(rng.normal(0, 0.35))
            d = length / steps
            x += int(math.cos(angle) * d)
            y += int(math.sin(angle) * d)
            pts.append((x, y))
        draw.line(pts, fill=int(rng.integers(145, 240)), width=int(rng.integers(width[0], width[1] + 1)))
    return img.filter(ImageFilter.GaussianBlur(0.7))


def irregular_blob(seed, threshold=0.57, blur=12):
    n = noise(seed, 1.0)
    m = np.clip((n - threshold) / max(0.001, 1.0 - threshold), 0, 1)
    img = Image.fromarray(np.uint8(m * 255), "L")
    return img.filter(ImageFilter.GaussianBlur(blur))


def vertical_streaks(seed, count=34, max_width=20):
    rng = np.random.default_rng(seed)
    img = blank()
    draw = ImageDraw.Draw(img)
    for _ in range(count):
        x = int(rng.integers(50, SIZE - 50))
        y0 = int(rng.integers(20, SIZE // 2))
        y1 = int(rng.integers(SIZE // 2, SIZE - 20))
        width = int(rng.integers(2, max_width))
        strength = int(rng.integers(55, 190))
        draw.line([(x, y0), (x + int(rng.normal(0, 12)), y1)], fill=strength, width=width)
        if rng.random() < 0.65:
            r = width * 2
            draw.ellipse((x-r, y0-r, x+r, y0+r), fill=min(255, strength + 25))
    return img.filter(ImageFilter.GaussianBlur(6))


def radial_mask(seed, radius=250):
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:SIZE, 0:SIZE]
    cx = SIZE / 2 + rng.uniform(-40, 40)
    cy = SIZE / 2 + rng.uniform(-40, 40)
    dist = np.sqrt((xx-cx)**2 + (yy-cy)**2)
    base = np.clip(1.0 - dist / radius, 0, 1)
    base *= np.clip(0.55 + noise(seed + 99, 0.5), 0, 1)
    return Image.fromarray(np.uint8(np.clip(base, 0, 1) * 255), "L").filter(ImageFilter.GaussianBlur(8))


def make_mask(name, seed):
    rng = np.random.default_rng(seed)
    if name == "CrackedPlaster":
        a = crack_mask(seed, 34, (2, 8))
        b = irregular_blob(seed + 1, 0.61, 9)
        return Image.fromarray(np.maximum(np.asarray(a), np.asarray(b) // 2).astype(np.uint8), "L")
    if name == "ChippedPaint":
        return irregular_blob(seed, 0.54, 8)
    if name == "ExposedSubstrate":
        return irregular_blob(seed, 0.50, 16)
    if name == "Soot":
        return radial_mask(seed, 330).filter(ImageFilter.GaussianBlur(18))
    if name == "RainStreaks":
        return vertical_streaks(seed, 46, 15)
    if name == "RustDrips":
        return vertical_streaks(seed, 28, 18)
    if name == "PatchedAsphalt":
        img = blank(); draw = ImageDraw.Draw(img)
        poly = [(170,230),(820,190),(870,720),(210,780)]
        draw.polygon(poly, fill=225)
        return img.filter(ImageFilter.GaussianBlur(7))
    if name == "AsphaltCracks":
        return crack_mask(seed, 45, (2, 6))
    if name == "TireWear":
        img = blank(); draw = ImageDraw.Draw(img)
        for off in (-85, 85):
            pts=[]
            for x in range(90, 940, 35):
                y = int(500 + off + 38*math.sin(x/150.0) + rng.normal(0,4))
                pts.append((x,y))
            draw.line(pts, fill=170, width=42)
        return img.filter(ImageFilter.GaussianBlur(9))
    if name == "CurbGrime":
        yy=np.arange(SIZE,dtype=np.float32)[:,None]
        band=np.exp(-((yy-650.0)/155.0)**2)
        n=noise(seed,0.6)
        arr=np.clip(band*(0.4+0.9*n),0,1)
        return Image.fromarray(np.uint8(arr*255),"L").filter(ImageFilter.GaussianBlur(5))
    if name == "UtilityMarking":
        img=blank(); draw=ImageDraw.Draw(img)
        draw.line([(230,760),(790,240)], fill=215, width=42)
        draw.line([(550,250),(790,240),(780,480)], fill=215, width=42)
        draw.ellipse((390,410,590,610), outline=185, width=28)
        return img.filter(ImageFilter.GaussianBlur(2))
    if name == "FadedSignage":
        img=blank(); draw=ImageDraw.Draw(img)
        draw.rounded_rectangle((120,270,900,730), radius=35, fill=165)
        draw.rectangle((170,320,850,680), fill=70)
        draw.line([(220,590),(800,400)], fill=150, width=34)
        return img.filter(ImageFilter.GaussianBlur(5))
    if name == "WaterStain":
        return irregular_blob(seed,0.50,30)
    if name == "ImpactScorch":
        core=radial_mask(seed,245)
        cracks=crack_mask(seed+4,18,(2,5))
        return Image.fromarray(np.maximum(np.asarray(core),np.asarray(cracks)).astype(np.uint8),"L")
    raise KeyError(name)
def normal_from_mask(mask):
    h = np.asarray(mask.filter(ImageFilter.GaussianBlur(2.0)), dtype=np.float32) / 255.0
    gy, gx = np.gradient(h)
    strength = 3.2
    nx, ny, nz = -gx*strength, -gy*strength, np.ones_like(h)
    norm = np.sqrt(nx*nx + ny*ny + nz*nz)
    normal = np.stack((nx/norm, ny/norm, nz/norm), axis=-1) * 0.5 + 0.5
    return Image.fromarray(np.uint8(np.clip(normal,0,1)*255), "RGB")


def write_decal(name, category, color, physical_size, seed):
    rng = np.random.default_rng(seed)
    mask = make_mask(name, seed)
    alpha = np.asarray(mask, dtype=np.float32) / 255.0
    macro = noise(seed + 200, 0.55)
    rgb = np.empty((SIZE, SIZE, 3), dtype=np.float32)
    base = np.array(color, dtype=np.float32) / 255.0
    for channel in range(3):
        rgb[..., channel] = np.clip(
            base[channel] * (0.82 + 0.28 * macro) + rng.normal(0, 0.012, (SIZE, SIZE)),
            0, 1
        )
    # Keep transparent regions neutral so edge filtering does not create bright halos.
    neutral = np.array([0.5,0.5,0.5], dtype=np.float32)
    rgb = rgb * alpha[...,None] + neutral * (1-alpha[...,None])
    rough = np.clip(
        0.44 + 0.46 * alpha + (macro - 0.5) * 0.13,
        0.20, 0.98
    )
    if name in {"RainStreaks","WaterStain"}:
        rough = np.clip(rough - 0.28 * alpha, 0.10, 0.92)
    if name in {"RustDrips","Soot","ImpactScorch"}:
        rough = np.clip(rough + 0.05 * alpha, 0.18, 0.98)

    Image.fromarray(np.uint8(rgb*255), "RGB").save(OUT/f"{name}_BaseColor.png")
    normal_from_mask(mask).save(OUT/f"{name}_Normal.png")
    Image.fromarray(np.uint8(rough*255), "L").save(OUT/f"{name}_Roughness.png")
    mask.save(OUT/f"{name}_Opacity.png")

    files = [
        OUT/f"{name}_BaseColor.png",
        OUT/f"{name}_Normal.png",
        OUT/f"{name}_Roughness.png",
        OUT/f"{name}_Opacity.png",
    ]
    return {
        "name": name,
        "instance_name": f"MI_Donetsk_Decal_{name}",
        "category": category,
        "source_url": None,
        "license": "Project-owned original",
        "approval_state": "approved",
        "resolution": SIZE,
        "physical_size_cm": {"x": physical_size[0], "y": physical_size[1]},
        "local_files": [f.relative_to(ROOT).as_posix() for f in files],
        "sha256": {f.stem.split("_")[-1]: hashlib.sha256(f.read_bytes()).hexdigest() for f in files},
        "provenance": "Original deterministic procedural decal authored for The Unit; no reference-game texture data.",
    }


def main():
    entries=[]
    for idx,(name,(category,color,physical_size)) in enumerate(SPECS.items()):
        entries.append(write_decal(name,category,color,physical_size,SEED+idx*37))
    ledger={
        "schema":"the-unit/donetsk-decal-source-ledger/v1",
        "version":1,
        "generator":"Tools/AssetPrep/generate_donetsk_decals.py",
        "decals":entries,
    }
    ledger_path=ROOT/"Tools"/"Reference"/"donetsk_decal_source_ledger.json"
    ledger_path.parent.mkdir(parents=True,exist_ok=True)
    ledger_path.write_text(json.dumps(ledger,indent=2)+"\n",encoding="utf-8")
    print(f"Generated {len(entries)} Donetsk decal sets in {OUT}")
    print(f"Ledger: {ledger_path}")


if __name__ == "__main__":
    main()
