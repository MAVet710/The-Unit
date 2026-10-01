from pathlib import Path
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "ExternalAssets" / "GeneratedDonetsk"
TEX = OUT / "Textures"
OUT.mkdir(parents=True, exist_ok=True)
TEX.mkdir(parents=True, exist_ok=True)

SEED = 710
rng = np.random.default_rng(SEED)

def multiscale_noise(size, seed, octaves=(8, 32, 128, 512)):
    local = np.random.default_rng(seed)
    acc = np.zeros((size, size), dtype=np.float32)
    weight = 0.0
    for idx, grid in enumerate(octaves):
        small = local.random((max(2, size // grid), max(2, size // grid)), dtype=np.float32)
        im = Image.fromarray(np.uint8(small * 255)).resize((size, size), Image.Resampling.BICUBIC)
        arr = np.asarray(im, dtype=np.float32) / 255.0
        w = 1.0 / (idx + 1)
        acc += arr * w
        weight += w
    return np.clip(acc / weight, 0, 1)

def save_surface(name, rgb, roughness, metallic=0.0, grit=0.12, size=1024, seed=1):
    n = multiscale_noise(size, seed)
    fine = np.random.default_rng(seed + 1000).normal(0, 1, (size, size)).astype(np.float32)
    fine = np.clip(fine * grit, -0.22, 0.22)
    tone = np.clip((n - 0.5) * 0.30 + fine, -0.34, 0.34)

    base = np.empty((size, size, 3), dtype=np.float32)
    for ch, c in enumerate(rgb):
        base[..., ch] = np.clip(c / 255.0 * (1.0 + tone), 0, 1)
    Image.fromarray(np.uint8(base * 255)).save(TEX / f"{name}_BaseColor.png")

    rough = np.clip(roughness + (n - 0.5) * 0.16 + fine * 0.10, 0.03, 1.0)
    Image.fromarray(np.uint8(rough * 255)).save(TEX / f"{name}_Roughness.png")

    metal = np.full((size, size), metallic, dtype=np.float32)
    if metallic > 0.05:
        metal = np.clip(metal - (n > 0.66) * 0.15, 0, 1)
    Image.fromarray(np.uint8(metal * 255)).save(TEX / f"{name}_Metallic.png")

    height = np.clip(n * 0.72 + fine * 0.20, 0, 1)
    gy, gx = np.gradient(height)
    strength = 2.0
    nx = -gx * strength
    ny = -gy * strength
    nz = np.ones_like(nx)
    norm = np.sqrt(nx * nx + ny * ny + nz * nz)
    normal = np.stack(((nx / norm) * 0.5 + 0.5, (ny / norm) * 0.5 + 0.5, (nz / norm) * 0.5 + 0.5), axis=-1)
    Image.fromarray(np.uint8(np.clip(normal, 0, 1) * 255)).save(TEX / f"{name}_Normal.png")


def save_brick_surface(size=1024, seed=710):
    local = np.random.default_rng(seed)
    height = np.zeros((size, size), dtype=np.float32)
    base = np.zeros((size, size, 3), dtype=np.float32)
    brick_h = 96
    brick_w = 220
    mortar = 11
    brick_colors = np.array([
        [0.34, 0.105, 0.055],
        [0.42, 0.135, 0.070],
        [0.30, 0.085, 0.045],
        [0.48, 0.170, 0.090],
    ], dtype=np.float32)
    mortar_color = np.array([0.34, 0.31, 0.27], dtype=np.float32)
    base[:] = mortar_color
    height[:] = 0.18

    for row, y0 in enumerate(range(-brick_h, size + brick_h, brick_h)):
        offset = -(brick_w // 2) if row % 2 else 0
        for x0 in range(offset - brick_w, size + brick_w, brick_w):
            y1 = max(0, y0 + mortar)
            y2 = min(size, y0 + brick_h - mortar)
            x1 = max(0, x0 + mortar)
            x2 = min(size, x0 + brick_w - mortar)
            if y1 >= y2 or x1 >= x2:
                continue
            color = brick_colors[local.integers(0, len(brick_colors))].copy()
            color *= float(local.uniform(0.88, 1.10))
            patch = local.normal(
                0.0, 0.035, size=(y2 - y1, x2 - x1, 1)
            ).astype(np.float32)
            base[y1:y2, x1:x2] = np.clip(color + patch, 0, 1)
            surface = 0.72 + local.normal(
                0.0, 0.055, size=(y2 - y1, x2 - x1)
            ).astype(np.float32)
            height[y1:y2, x1:x2] = np.clip(surface, 0, 1)

    yy, xx = np.mgrid[0:size, 0:size]
    broad = (
        np.sin(xx / 93.0)
        + np.sin(yy / 131.0)
        + np.sin((xx + yy) / 177.0)
    ) / 3.0
    base = np.clip(base * (0.94 + 0.09 * broad[..., None]), 0, 1)

    for _ in range(34):
        cx, cy = local.integers(0, size, 2)
        radius = int(local.integers(18, 85))
        dist = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
        mask = np.clip(1.0 - dist / radius, 0, 1)[..., None]
        tint = (
            np.array([0.12, 0.10, 0.08])
            if local.random() < 0.7
            else np.array([0.60, 0.55, 0.45])
        )
        strength = float(local.uniform(0.025, 0.09))
        base = base * (1 - mask * strength) + tint * mask * strength

    height_image = Image.fromarray(
        np.uint8(np.clip(height, 0, 1) * 255), "L"
    ).filter(ImageFilter.GaussianBlur(1.0))
    h = np.asarray(height_image, dtype=np.float32) / 255.0
    gy, gx = np.gradient(h)
    strength = 7.5
    nx = -gx * strength
    ny = -gy * strength
    nz = np.ones_like(h)
    norm = np.sqrt(nx * nx + ny * ny + nz * nz)
    normal = np.stack((nx / norm, ny / norm, nz / norm), axis=-1)
    normal = normal * 0.5 + 0.5
    rough = np.clip(
        0.62 + (1 - h) * 0.20 + local.normal(0, 0.035, (size, size)),
        0.42,
        0.93,
    )
    metal = np.zeros((size, size), dtype=np.float32)

    Image.fromarray(np.uint8(np.clip(base, 0, 1) * 255), "RGB").save(
        TEX / "Brick_BaseColor.png"
    )
    Image.fromarray(np.uint8(rough * 255), "L").save(
        TEX / "Brick_Roughness.png"
    )
    Image.fromarray(np.uint8(metal * 255), "L").save(
        TEX / "Brick_Metallic.png"
    )
    Image.fromarray(np.uint8(np.clip(normal, 0, 1) * 255), "RGB").save(
        TEX / "Brick_Normal.png"
    )


SURFACES = {
    "Asphalt": ((40, 43, 44), .91, 0.0, .13, 2048),
    "Paving": ((118, 111, 101), .84, 0.0, .10, 2048),
    "UrbanGrass": ((61, 82, 42), .96, 0.0, .15, 1024),
    "DrySoil": ((92, 68, 45), .95, 0.0, .15, 1024),
    "RustSteel": ((103, 48, 27), .72, .58, .18, 1024),
    "Concrete": ((113, 113, 109), .89, 0.0, .12, 2048),
    "CoalStone": ((45, 47, 49), .91, 0.0, .10, 1024),
    "DarkGlass": ((31, 42, 48), .17, .08, .02, 1024),
    "PanelWarm": ((139, 129, 115), .84, 0.0, .08, 1024),
    "PanelCool": ((116, 121, 123), .86, 0.0, .08, 1024),
    "PinkStucco": ((158, 99, 86), .88, 0.0, .12, 1024),
    "MetalDark": ((52, 54, 55), .42, .72, .08, 1024),
    "StalinkaSand": ((157, 139, 105), .86, 0.0, .10, 1024),
    "TreeBark": ((72, 47, 29), .94, 0.0, .18, 1024),
    "TreeLeaves": ((53, 89, 39), .83, 0.0, .18, 1024),
    "WhitePlaster": ((205, 202, 190), .83, 0.0, .08, 1024),
    "RoadPaint": ((188, 187, 175), .68, 0.0, .07, 1024),
    "Rubber": ((30, 30, 29), .82, 0.0, .09, 1024),
    "VehiclePaint": ((74, 82, 88), .38, .58, .06, 1024),
    "Galvanized": ((115, 119, 119), .48, .74, .08, 1024),
    "Wood": ((105, 71, 41), .83, 0.0, .14, 1024),
    "DirtyWater": ((55, 63, 61), .12, .02, .03, 1024),
}
for idx, (name, args) in enumerate(SURFACES.items()):
    save_surface(name, *args, seed=SEED + idx * 17)
save_brick_surface(seed=SEED)

MTL = "\n".join(
    f"newmtl {name}\nKd {rgb[0]/255:.4f} {rgb[1]/255:.4f} {rgb[2]/255:.4f}\nNs 20"
    for name, (rgb, *_rest) in SURFACES.items()
)
(OUT / "DonetskEnvironmentMaterials.mtl").write_text(MTL + "\n", encoding="utf-8")

class Obj:
    def __init__(self, name):
        self.name=name; self.v=[]; self.parts=[]
    def box(self,c,s,mat):
        cx,cy,cz=c; sx,sy,sz=s
        start=len(self.v)+1
        self.v += [(cx+x*sx/2,cy+y*sy/2,cz+z*sz/2) for x,y,z in
                   [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
        fs=[(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(4,0,3,7)]
        self.parts.append((mat,[tuple(start+i for i in f) for f in fs]))
    def cyl(self,c,r,h,mat,n=16):
        cx,cy,cz=c; start=len(self.v)+1
        for z in (cz-h/2,cz+h/2):
            for i in range(n):
                a=2*math.pi*i/n
                self.v.append((cx+r*math.cos(a),cy+r*math.sin(a),z))
        fs=[tuple(start+i for i in range(n-1,-1,-1)),tuple(start+n+i for i in range(n))]
        for i in range(n):
            j=(i+1)%n
            fs.append((start+i,start+j,start+n+j,start+n+i))
        self.parts.append((mat,fs))
    def write(self):
        lines=["mtllib DonetskEnvironmentMaterials.mtl",f"o {self.name}"]
        lines += [f"v {x:.4f} {y:.4f} {z:.4f}" for x,y,z in self.v]
        for mat,fs in self.parts:
            lines.append(f"usemtl {mat}")
            lines += ["f "+" ".join(map(str,f)) for f in fs]
        (OUT/f"{self.name}.obj").write_text("\n".join(lines)+"\n",encoding="utf-8")

def lamp():
    o=Obj("SM_Donetsk_Prop_LampPost"); o.cyl((0,0,325),9,650,"Galvanized",12); o.box((0,0,655),(18,18,20),"MetalDark"); o.box((45,0,670),(100,24,20),"MetalDark"); o.write()
def dash():
    o=Obj("SM_Donetsk_Prop_RoadDash"); o.box((0,0,1.5),(18,220,3),"RoadPaint"); o.write()
def barrier():
    o=Obj("SM_Donetsk_Prop_ConcreteBarrier"); o.box((0,0,42),(200,52,84),"Concrete"); o.box((0,0,92),(170,40,18),"Concrete"); o.write()
def sedan():
    o=Obj("SM_Donetsk_Prop_Sedan"); o.box((0,0,42),(420,172,76),"VehiclePaint"); o.box((5,0,98),(225,160,75),"VehiclePaint"); o.box((5,-82,99),(178,5,50),"DarkGlass"); o.box((5,82,99),(178,5,50),"DarkGlass")
    for x in (-130,130):
        for y in (-78,78): o.cyl((x,y,35),31,24,"Rubber",16)
    o.box((212,0,50),(12,120,24),"MetalDark"); o.box((-212,0,50),(12,120,24),"MetalDark"); o.write()
def dumpster():
    o=Obj("SM_Donetsk_Prop_Dumpster"); o.box((0,0,65),(190,105,130),"Galvanized"); o.box((0,0,135),(198,112,12),"MetalDark"); o.write()
def shelter():
    o=Obj("SM_Donetsk_Prop_BusShelter"); o.box((0,0,120),(360,12,240),"DarkGlass"); o.box((-175,72,120),(12,145,240),"DarkGlass"); o.box((0,72,247),(390,165,14),"Galvanized"); o.box((0,22,44),(240,42,16),"Wood")
    for x in (-165,165): o.box((x,66,120),(10,10,240),"MetalDark")
    o.write()
def cabinet():
    o=Obj("SM_Donetsk_Prop_UtilityCabinet"); o.box((0,0,70),(100,62,140),"Galvanized"); o.box((0,-32,72),(82,4,110),"MetalDark"); o.write()
def kiosk():
    o=Obj("SM_Donetsk_Prop_Kiosk"); o.box((0,0,125),(250,210,250),"PanelCool"); o.box((0,-108,142),(150,6,90),"DarkGlass"); o.box((0,-120,92),(170,28,10),"Galvanized"); o.box((0,0,258),(275,235,16),"MetalDark"); o.write()
def rubble():
    o=Obj("SM_Donetsk_Prop_RubblePile")
    local=np.random.default_rng(409)
    for i in range(18):
        x=float(local.uniform(-140,140)); y=float(local.uniform(-100,100)); z=float(local.uniform(12,45))
        sx=float(local.uniform(35,105)); sy=float(local.uniform(28,80)); sz=float(local.uniform(18,55))
        o.box((x,y,z),(sx,sy,sz),"Concrete" if i%3 else "RustSteel")
    o.write()
def pallet():
    o=Obj("SM_Donetsk_Prop_Pallet")
    for y in (-42,0,42): o.box((0,y,8),(120,18,16),"Wood")
    for x in (-48,0,48): o.box((x,0,19),(18,95,16),"Wood")
    o.write()
def sign():
    o=Obj("SM_Donetsk_Prop_TrafficSign"); o.cyl((0,0,120),4,240,"Galvanized",10); o.box((0,0,220),(65,5,65),"RoadPaint"); o.write()
def bench():
    o=Obj("SM_Donetsk_Prop_Bench"); o.box((0,0,47),(180,42,12),"Wood"); o.box((0,18,92),(180,10,82),"Wood")
    for x in (-72,72): o.box((x,0,25),(10,10,50),"MetalDark")
    o.write()
def planter():
    o=Obj("SM_Donetsk_Prop_Planter"); o.box((0,0,28),(95,95,56),"Concrete"); o.box((0,0,57),(74,74,12),"DrySoil"); o.write()
def crate():
    o=Obj("SM_Donetsk_Prop_Crate"); o.box((0,0,42),(90,70,84),"Wood"); o.box((0,-36,42),(96,6,12),"MetalDark"); o.write()
def manhole():
    o=Obj("SM_Donetsk_Prop_Manhole"); o.cyl((0,0,2),42,4,"MetalDark",28); o.write()
def puddle():
    o=Obj("SM_Donetsk_Prop_Puddle"); o.box((0,0,0.8),(180,105,1.6),"DirtyWater"); o.write()

for fn in (lamp,dash,barrier,sedan,dumpster,shelter,cabinet,kiosk,rubble,pallet,sign,bench,planter,crate,manhole,puddle):
    fn()

print(f"Generated {len(SURFACES) + 1} PBR material sets and 16 Donetsk environment meshes in {OUT}")
