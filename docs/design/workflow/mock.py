from PIL import Image, ImageDraw, ImageFont
import sys
F="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"; FB="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
f=ImageFont.truetype(F,15); fs=ImageFont.truetype(F,13); fb=ImageFont.truetype(FB,15); ft=ImageFont.truetype(FB,18)
CODE={"xtb":"#7a4fa3","ORCA":"#1f6fb2","ECCE-QM":"#2b8a3e","G16":"#b35c00","CP2K":"#8a1c1c","QE":"#5c6b73","GMX":"#0b7285"}
STATE={"Complete":"#2b8a3e","Running":"#1f6fb2","Ready":"#868e96","Created":"#ced4da","Waiting":"#e8a400"}
def frame(W,H,title,sel=None):
    im=Image.new("RGB",(W,H),"white"); d=ImageDraw.Draw(im)
    d.rectangle([0,0,W,34],fill="#e9ecef"); d.text((12,8),title,font=fb,fill="black")
    # left organizer tree
    d.rectangle([0,35,260,H],fill="#f8f9fa"); d.line([260,35,260,H],fill="#adb5bd")
    y=50
    for t,ind in [("Projects",0),("Na-hydration",1),("Na(H2O)n workflow",2),("SN2 Cl- + CH3Br",2),("Examples",1)]:
        if t==sel: d.rectangle([4,y-3,256,y+19],fill="#d0ebff")
        d.text((14+18*ind,y),("▸ " if ind<2 else "◆ ")+t,font=f,fill="black"); y+=26
    # vertical toolbar of codes
    d.rectangle([W-70,35,W,H],fill="#f1f3f5"); d.line([W-70,35,W-70,H],fill="#adb5bd")
    y=50
    for c in ["xtb","ORCA","ECCE-QM","G16","CP2K","QE","GMX"]:
        d.rounded_rectangle([W-60,y,W-10,y+40],6,fill=CODE[c]); d.text((W-35,y+20),c[:4],font=fs,fill="white",anchor="mm"); y+=50
    d.text((W-35,y+5),"new step",font=fs,fill="#495057",anchor="mt")
    return im,d
def box(d,x,y,code,name,state,w=250,h=64,selected=False):
    d.rounded_rectangle([x,y,x+w,y+h],8,fill="white",outline="#1c7ed6" if selected else "#868e96",width=4 if selected else 2)
    d.rounded_rectangle([x,y,x+8,y+h],4,fill=CODE[code])
    d.text((x+16,y+8),code,font=fb,fill=CODE[code]); d.text((x+16,y+36),name,font=fs,fill="black")
    d.ellipse([x+w-22,y+10,x+w-10,y+22],fill=STATE[state]); d.text((x+w-28,y+16),state,font=fs,fill="#495057",anchor="rm")
    return (x,y,w,h)
def link(d,a,b,label=None,dash=False,col="#495057"):
    x1=a[0]+a[2]; y1=a[1]+a[3]//2; x2=b[0]; y2=b[1]+b[3]//2; xm=(x1+x2)//2
    pts=[(x1,y1),(xm,y1),(xm,y2),(x2,y2)]
    for p,q in zip(pts,pts[1:]):
        if dash:
            import math
            L=math.dist(p,q); n=int(L//10)
            for i in range(0,n,2):
                t0=i/n; t1=min(1,(i+1)/n)
                d.line([(p[0]+(q[0]-p[0])*t0,p[1]+(q[1]-p[1])*t0),(p[0]+(q[0]-p[0])*t1,p[1]+(q[1]-p[1])*t1)],fill=col,width=2)
        else: d.line([p,q],fill=col,width=2)
    d.polygon([(x2,y2),(x2-9,y2-5),(x2-9,y2+5)],fill=col)
    if label: d.text((xm+6,(y1+y2)//2-16 if y1!=y2 else y2-18),label,font=fs,fill="#495057")

# A: branching
W,H=1500,620
im,d=frame(W,H,"ECCE Organizer — Workflow: Na(H2O)n workflow","Na(H2O)n workflow")
d.text((285,48),"Na⁺ hydration: xtb finds cluster structures, DFT refines the best ones",font=f,fill="#343a40")
a=box(d,290,250,"xtb","Na(H2O)6 GFN2 opt","Complete")
b1=box(d,600,110,"ORCA","B3LYP-D3 opt, isomer 1","Complete")
b2=box(d,600,250,"ORCA","B3LYP-D3 opt, isomer 2","Running",selected=True)
b3=box(d,600,390,"xtb","GFN2 MD 300 K","Ready")
c1=box(d,930,110,"ORCA","Freq + IR","Ready")
c2=box(d,930,250,"ECCE-QM","Charges, MO diagram","Created")
link(d,a,b1,"geometry"); link(d,a,b2,"geometry"); link(d,a,b3,"geometry")
link(d,b1,c1,"geometry"); link(d,b2,c2,"geometry")
d.rectangle([285,530,1420,600],fill="#f8f9fa",outline="#dee2e6")
d.text((300,540),"Select a box, then a code on the right: the new step starts from that box's final geometry, charge and multiplicity.",font=fs,fill="black")
d.text((300,562),"Several boxes from one source = branches.  Launch Ready Steps runs every step whose source is Complete.",font=fs,fill="black")
im.save(sys.argv[1]+"/wf-a-branches.png")

# B: join
im,d=frame(W,H,"ECCE Organizer — Workflow: SN2 Cl- + CH3Br","SN2 Cl- + CH3Br")
d.text((285,48),"A join: a step with more than one source (transition-state search, reaction energy)",font=f,fill="#343a40")
r=box(d,290,120,"ORCA","Reactant complex opt","Complete")
p=box(d,290,330,"ORCA","Product complex opt","Complete")
ts=box(d,620,225,"ORCA","NEB-TS (reactant → product)","Running",w=270)
irc=box(d,980,130,"ORCA","IRC","Created")
e=box(d,980,400,"ECCE-QM","Reaction energy (ΔE, ΔG)","Created",w=260)
link(d,r,ts,"reactant"); link(d,p,ts,"product"); link(d,ts,irc,"geometry")
link(d,ts,e,"TS")
g="#868e96"
def dl(pts):
    import math
    for p0,q in zip(pts,pts[1:]):
        L=math.dist(p0,q); n=max(2,int(L//10))
        for i in range(0,n,2):
            t0=i/n; t1=min(1,(i+1)/n)
            d.line([(p0[0]+(q[0]-p0[0])*t0,p0[1]+(q[1]-p0[1])*t0),(p0[0]+(q[0]-p0[0])*t1,p0[1]+(q[1]-p0[1])*t1)],fill=g,width=2)
dl([(540,362),(540,470),(960,470),(960,445),(980,445)])
dl([(290+250,152),(575,152),(575,95),(950,95),(950,425),(980,425)])
d.polygon([(980,445),(971,440),(971,450)],fill=g); d.polygon([(980,425),(971,420),(971,430)],fill=g)
d.text((600,475),"reactant, product (dashed: second and third source)",font=fs,fill="#495057")
d.rectangle([285,530,1420,600],fill="#f8f9fa",outline="#dee2e6")
d.text((300,540),"Ctrl-click two or more boxes, then a code: the new step gets one named link per source (reactant, product, TS ...).",font=fs,fill="black")
d.text((300,562),"Lines to a join are drawn to each source; the tree stays laid out from the first source.",font=fs,fill="black")
im.save(sys.argv[1]+"/wf-b-join.png")

# C: new step dialog
im=Image.new("RGB",(640,430),"white"); d=ImageDraw.Draw(im)
d.rectangle([0,0,640,34],fill="#e9ecef"); d.text((12,8),"New ORCA step",font=fb,fill="black")
y=55
d.text((20,y),"Name:",font=f,fill="black"); d.rectangle([140,y-4,600,y+22],outline="#868e96"); d.text((148,y),"B3LYP-D3 opt, isomer 2",font=f,fill="black"); y+=45
d.text((20,y),"Starts from:",font=f,fill="black"); d.text((140,y),"xtb  Na(H2O)6 GFN2 opt   (Complete)",font=f,fill="#343a40"); y+=40
d.text((20,y),"Carry over:",font=fb,fill="black"); y+=30
for on,t in [(1,"Final geometry"),(1,"Charge and multiplicity"),(0,"Orbitals as the starting guess (where the two codes allow)"),(0,"Hessian (for a TS search)")]:
    d.rectangle([40,y,56,y+16],outline="#495057",width=2,fill="#1c7ed6" if on else "white")
    if on: d.text((43,y-3),"✓",font=f,fill="white")
    d.text((66,y-1),t,font=f,fill="black"); y+=30
y+=10; d.text((20,y),"Then the ORCA Calculation Editor opens with these filled in.",font=fs,fill="#495057")
d.rounded_rectangle([400,375,500,410],6,fill="#e9ecef",outline="#868e96"); d.text((450,392),"Cancel",font=f,fill="black",anchor="mm")
d.rounded_rectangle([515,375,620,410],6,fill="#1c7ed6"); d.text((567,392),"Create",font=fb,fill="white",anchor="mm")
im.save(sys.argv[1]+"/wf-c-new-step.png")
