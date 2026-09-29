from pathlib import Path
import math
out=Path(r"C:\Users\ndasi\OneDrive\Documents\GitHub\The-Unit\ExternalAssets\GeneratedDonetsk")
out.mkdir(parents=True,exist_ok=True)

mtl='''newmtl PinkStucco
Kd 0.58 0.32 0.27
Ks 0.08 0.08 0.08
Ns 18
newmtl CoalStone
Kd 0.10 0.105 0.115
Ks 0.04 0.04 0.04
Ns 8
newmtl WhitePlaster
Kd 0.86 0.84 0.78
Ks 0.08 0.08 0.08
Ns 22
newmtl DarkGlass
Kd 0.055 0.085 0.105
Ks 0.30 0.30 0.32
Ns 90
newmtl PanelWarm
Kd 0.46 0.43 0.39
Ks 0.04 0.04 0.04
Ns 10
newmtl PanelCool
Kd 0.40 0.42 0.43
Ks 0.04 0.04 0.04
Ns 10
newmtl StalinkaSand
Kd 0.59 0.52 0.40
Ks 0.06 0.06 0.06
Ns 14
newmtl MetalDark
Kd 0.14 0.15 0.16
Ks 0.25 0.25 0.25
Ns 55
newmtl Concrete
Kd 0.34 0.34 0.33
Ks 0.03 0.03 0.03
Ns 8
'''
(out/"DonetskMaterials.mtl").write_text(mtl,encoding="utf-8")

class Obj:
    def __init__(self,name):
        self.name=name; self.v=[]; self.parts=[]
    def box(self,c,s,mat):
        cx,cy,cz=c; sx,sy,sz=s
        verts=[(cx+x*sx/2,cy+y*sy/2,cz+z*sz/2) for x,y,z in
               [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
        start=len(self.v)+1; self.v+=verts
        faces=[(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(4,0,3,7)]
        self.parts.append((mat,[tuple(start+i for i in f) for f in faces]))
    def cyl(self,c,r,h,mat,n=16):
        cx,cy,cz=c; start=len(self.v)+1
        for z in (cz-h/2,cz+h/2):
            for i in range(n):
                a=2*math.pi*i/n; self.v.append((cx+r*math.cos(a),cy+r*math.sin(a),z))
        fs=[]
        fs.append(tuple(start+i for i in range(n-1,-1,-1)))
        fs.append(tuple(start+n+i for i in range(n)))
        for i in range(n):
            j=(i+1)%n; fs.append((start+i,start+j,start+n+j,start+n+i))
        self.parts.append((mat,fs))
    def write(self,path):
        lines=[f"mtllib DonetskMaterials.mtl",f"o {self.name}"]
        lines += [f"v {x:.4f} {y:.4f} {z:.4f}" for x,y,z in self.v]
        grouped={}
        order=[]
        for mat,faces in self.parts:
            if mat not in grouped:
                grouped[mat]=[]
                order.append(mat)
            grouped[mat].extend(faces)
        for mat in order:
            lines.append(f"usemtl {mat}")
            lines += ["f "+" ".join(map(str,f)) for f in grouped[mat]]
        path.write_text("\n".join(lines)+"\n",encoding="utf-8")

def facade(name,floors,bays,bayw,depth,floorh,kind,balconies=True,raised=False):
    o=Obj(name); width=bays*bayw; H=floors*floorh
    mat="PanelWarm" if kind=="khrush" else "PanelCool" if kind=="brezhnev" else "StalinkaSand"
    o.box((0,0,H/2),(width,depth,H),mat)
    o.box((0,-depth/2-10,55),(width+20,20,110),"CoalStone")
    front=-depth/2-18
    for fl in range(floors):
        z=floorh*(fl+.54)
        wh=floorh*(.50 if fl==0 and raised else .58)
        for bay in range(bays):
            x=-width/2+bayw*(bay+.5)
            entrance=(fl==0 and bay in ({bays//3,2*bays//3} if bays>=10 else {bays//2}))
            if entrance:
                o.box((x,front-3,floorh*.42),(bayw*.42,16,floorh*.72),"MetalDark")
            else:
                o.box((x,front-3,z),(bayw*.48,16,wh),"DarkGlass")
            if balconies and fl>0 and bay%2==1:
                slabz=floorh*fl+floorh*.28
                o.box((x,front-72,slabz),(bayw*.72,120,22),"Concrete")
                for k in range(-3,4):
                    o.box((x+k*bayw*.10,front-130,slabz+55),(8,8,110),"MetalDark")
                o.box((x,front-132,slabz+105),(bayw*.72,8,8),"MetalDark")
    if kind=="brezhnev":
        for x in (-width*.32,width*.32):
            o.box((x,-depth/2-35,H*.5),(bayw*.6,70,H*.92),"PanelCool")
    if kind=="stalinka":
        o.box((0,front-5,H+45),(width+50,30,90),"WhitePlaster")
        for x in (-width/2+bayw*.5,width/2-bayw*.5):
            o.box((x,front-32,H*.52),(45,35,H*.8),"WhitePlaster")
    o.write(out/(name+".obj"))

def artema60():
    o=Obj("SM_Artema60_Production")
    W,D=5200,1900; gf,uf=360,340; H=gf+3*uf
    o.box((0,0,H/2),(W,D,H),"PinkStucco")
    front=-D/2-18
    # coal-dark plinth quads
    pieces=17; pw=W/pieces
    for i in range(pieces):
        x=-W/2+pw*(i+.5)
        o.box((x,front-8,52),(pw-12,30,104),"CoalStone")
    # straight facade windows, preserving the calibrated tower/projection zones
    tower=640; radius=430; bays=10; usable=W-2*radius-tower; bw=usable/bays
    floorbase=0
    for fl in range(4):
        fh=gf if fl==0 else uf
        z=floorbase+fh*.54; wh=fh*(.52 if fl==0 else .60)
        x0=-W/2+tower+bw*.5
        for bay in range(bays):
            x=x0+bw*bay
            o.box((x,front-10,z),(max(140,bw*.50),20,wh),"DarkGlass")
        floorbase+=fh
        if fl<3:o.box((0,front-22,floorbase),(W+40,34,32),"WhitePlaster")
    # stair tower
    tx=-W/2+tower/2; th=H+170
    o.box((tx,-D/2-80,th/2),(tower,520,th),"PinkStucco")
    for i in range(3):
        o.box((tx,-D/2-351,310+i*390),(tower*.40,20,300),"DarkGlass")
    # rounded street projection: cylinder is embedded halfway into main mass
    px=W/2-radius-180; py=-D/2
    o.cyl((px,py,H/2),radius,H,"PinkStucco",24)
    for idx,a in enumerate([205,237.5,270,302.5,335]):
        rad=math.radians(a)
        x=px+math.cos(rad)*(radius+28); y=py+math.sin(rad)*(radius+28)
        o.cyl((x,y,730),24,1080,"WhitePlaster",12)
    # balconies and real post rhythm
    for fl in range(1,4):
        z=gf+uf*fl-40
        o.box((px,front-radius*.72,z),(radius*1.84,170,24),"Concrete")
        y=front-radius*.91
        for k in range(-5,6):
            o.box((px+k*radius*.16,y,z+52),(9,9,104),"WhitePlaster")
        o.box((px,y,z+102),(radius*1.84,10,10),"WhitePlaster")
    # parapet with breaks/pedestals instead of one flat gray bar
    o.box((0,front-18,H+52),(W+50,42,104),"WhitePlaster")
    for k in range(-6,7):
        o.box((k*W/13,front-38,H+118),(34,55,130),"WhitePlaster")
    # documented facade cartouche marker
    o.box((px,front-radius*.97,H*.72),(280,18,210),"WhitePlaster")
    # street entrance and canopy
    o.box((-250,front-20,150),(260,30,300),"MetalDark")
    o.box((-250,front-120,330),(420,210,26),"Concrete")
    # side facade windows to stop silhouette reading as a blank box
    for side in (-1,1):
        x=side*(W/2+10)
        for fl in range(4):
            z=(gf*.55 if fl==0 else gf+uf*(fl-.47))
            for j in range(4):
                y=-D*.35+j*(D*.23)
                o.box((x,y,z),(20,180,190),"DarkGlass")
    o.write(out/"SM_Artema60_Production.obj")

artema60()
facade("SM_Donetsk_Khrush_5F_16",5,16,315,1150,280,"khrush",True,False)
facade("SM_Donetsk_Khrush_5F_14",5,14,315,1150,280,"khrush",True,False)
facade("SM_Donetsk_Khrush_5F_12",5,12,315,1150,280,"khrush",False,False)
facade("SM_Donetsk_Brezhnev_9F_14",9,14,340,1450,285,"brezhnev",True,True)
facade("SM_Donetsk_Brezhnev_9F_10",9,10,340,1450,285,"brezhnev",True,True)
facade("SM_Donetsk_Stalinka_5F_12",5,12,390,1500,330,"stalinka",False,True)
facade("SM_Donetsk_Stalinka_5F_10",5,10,390,1500,330,"stalinka",True,True)
print("generated",len(list(out.glob("*.obj"))),"OBJ meshes at",out)
