import sys
exec(open(sys.argv[2]).read().split("# A: branching")[0])
from PIL import ImageFilter
FM="/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
fm=ImageFont.truetype(FM,13); fx=ImageFont.truetype(F,12); fxb=ImageFont.truetype(FB,12)
def ell(t,font,w,d):
    while d.textlength(t,font=font)>w and len(t)>4: t=t[:-2]
    return t if d.textlength(t,font=font)<=w else t
def card(im,d,x,y,code,name,state,theory,E,extra=None,w=280,sel=False):
    h=96 if extra is None else 116
    d.rounded_rectangle([x,y,x+w,y+h],10,fill="white",outline="#1c7ed6" if sel else "#ced4da",width=3 if sel else 1)
    d.rounded_rectangle([x,y,x+7,y+h],4,fill=CODE[code])
    d.text((x+16,y+8),code,font=fb,fill=CODE[code])
    d.ellipse([x+w-20,y+11,x+w-10,y+21],fill=STATE[state]); d.text((x+w-26,y+16),state,font=fx,fill="#868e96",anchor="rm")
    nm=name if d.textlength(name,font=f)<w-30 else ell(name,f,w-45,d)+"…"
    d.text((x+16,y+32),nm,font=f,fill="black")
    d.line([x+16,y+56,x+w-12,y+56],fill="#e9ecef",width=1)
    d.text((x+16,y+62),theory,font=fx,fill="#495057")
    d.text((x+16,y+78),E,font=fm,fill="#212529")
    if extra:
        cx=x+16
        for t in extra:
            tw=d.textlength(t,font=fx)+14
            d.rounded_rectangle([cx,y+96,cx+tw,y+112],8,fill="#f1f3f5"); d.text((cx+7,y+104),t,font=fx,fill="#495057",anchor="lm"); cx+=tw+6
    return (x,y,w,h)
def arrow(d,a,b,label):
    x1=a[0]+a[2]; y1=a[1]+44; x2=b[0]; y2=b[1]+44; xm=(x1+x2)//2
    d.line([(x1,y1),(xm,y1),(xm,y2),(x2,y2)],fill="#868e96",width=2); d.polygon([(x2,y2),(x2-9,y2-5),(x2-9,y2+5)],fill="#868e96")
    d.text(((xm+x2)//2 if y1!=y2 else (x1+x2)//2,y2-7),label,font=fx,fill="#868e96",anchor="mb")
def popover(im,x,y,title,rows,w=330):
    h=40+len(rows)*21+10
    sh=Image.new("RGBA",im.size,(0,0,0,0)); sd=ImageDraw.Draw(sh); sd.rounded_rectangle([x+4,y+6,x+w+4,y+h+6],12,fill=(0,0,0,70))
    sh=sh.filter(ImageFilter.GaussianBlur(6)); im.paste(sh,(0,0),sh); d=ImageDraw.Draw(im)
    d.rounded_rectangle([x,y,x+w,y+h],12,fill="white",outline="#dee2e6")
    d.text((x+14,y+12),title,font=fb,fill="black")
    yy=y+40
    for k,v in rows:
        d.text((x+14,yy),k,font=fx,fill="#868e96"); d.text((x+w-14,yy),v,font=fm,fill="#212529",anchor="ra"); yy+=21
    return d
rowsfull=[("Method","r2SCAN-3c (CPCM water)"),("E","-620.743348 Eh"),("H","-620.512600 Eh"),("G","-620.559800 Eh"),("S","415 J/(mol K)"),("Modes","57, 0 imaginary"),("CPU time","0.2 CPU-h")]
data=[("6_0",-30.413509,-620.740199),("5_1",-30.417359,-620.740504),("4_2",-30.420171,-620.743348)]
# V1: in-box cards with chips
W,H=1500,560
im,d=frame(W,H,"Variant 1 — key data inside the box: method, E, and small tags","Na(H2O)n workflow")
for i,(n,ex,eo) in enumerate(data):
    y=60+i*150
    a=card(im,d,290,y,"xtb","na_%s_xtb"%n,"Complete","GFN2-xTB","E = %.6f Eh"%ex,["0.01 CPU-h"])
    b=card(im,d,720,y,"ORCA","na_%s_orca_with_a_rather_long_name"%n if i==1 else "na_%s_orca"%n,"Complete","r2SCAN-3c (CPCM water)","E = %.6f Eh"%eo,["G = -620.5598","57 modes, 0 imag","0.2 CPU-h"],sel=(i==2),w=345)
    arrow(d,a,b,"geometry + vibration")
im.save(sys.argv[1]+"/wf-v1-inbox.png")
# V2: compact boxes + hover popover
im,d=frame(W,H,"Variant 2 — compact boxes; the mouse over a box shows a card with everything","Na(H2O)n workflow")
for i,(n,ex,eo) in enumerate(data):
    y=60+i*130
    a=card(im,d,290,y,"xtb","na_%s_xtb"%n,"Complete","GFN2-xTB","E = %.6f Eh"%ex)
    b=card(im,d,720,y,"ORCA","na_%s_orca"%n,"Complete","r2SCAN-3c (CPCM water)","E = %.6f Eh"%eo,sel=(i==2))
    arrow(d,a,b,"geometry + vibration")
d=popover(im,1000,290,"na_4_2_orca",rowsfull)
d.polygon([(1000,330),(990,338),(1000,346)],fill="white")
im.save(sys.argv[1]+"/wf-v2-hover.png")
# V3: compact boxes + selection card docked on the right
W2=1500
im,d=frame(W2,H,"Variant 3 — compact boxes; the selected box's card is shown beside the diagram","Na(H2O)n workflow")
for i,(n,ex,eo) in enumerate(data):
    y=60+i*130
    a=card(im,d,290,y,"xtb","na_%s_xtb"%n,"Complete","GFN2-xTB","E = %.6f Eh"%ex,w=250)
    b=card(im,d,640,y,"ORCA","na_%s_orca"%n,"Complete","r2SCAN-3c (CPCM water)","E = %.6f Eh"%eo,sel=(i==2),w=270)
    arrow(d,a,b,"geom + vib")
d=popover(im,1040,60,"na_4_2_orca  ·  Complete",rowsfull+[("Started from","na_4_2_xtb"),("Machine","nerdegg, 4 cores")],w=340)
im.save(sys.argv[1]+"/wf-v3-sidecard.png")
