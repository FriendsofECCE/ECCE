from PIL import Image, ImageDraw, ImageFont
import sys
R="/home/andy/tmp/ecce/ECCE/help/src/img/"
org=Image.open(R+"organizer-project.png").convert("RGB")
bld=Image.open(R+"builder-water.png").convert("RGB")
# white viewer background (Andy's rule)
px=bld.load()
for y in range(87,979):
    for x in range(0,1027):
        r,g,b=px[x,y]
        if r<25 and g<25 and b<25: px[x,y]=(255,255,255)
F=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",15)
FB=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",15)
FS=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",13)
BG=(246,245,244); LINE=(200,198,195); ACT=(255,255,255); ACC=(53,132,228)
def tabs(d,x0,y0,h,items,active):
    d.rectangle([x0,y0,1500,y0+h],fill=(232,230,227)); d.line([x0,y0+h,1500,y0+h],fill=LINE)
    x=x0+6
    for i,(icon,t) in enumerate(items):
        w=int(d.textlength(t,font=F))+58
        on=i==active
        d.rounded_rectangle([x,y0+4,x+w,y0+h+2],radius=6,fill=ACT if on else (238,236,233),outline=LINE)
        if on: d.rectangle([x+1,y0+4,x+w-1,y0+7],fill=ACC)
        d.ellipse([x+10,y0+h//2-5,x+20,y0+h//2+5],fill=icon)
        d.text((x+28,y0+h//2),t,font=FB if on else F,fill=(30,30,30),anchor="lm")
        if i>0: d.text((x+w-14,y0+h//2),"×",font=F,fill=(110,110,110),anchor="mm")
        x+=w+4
    d.text((x+14,y0+h//2),"+",font=FB,fill=(90,90,90),anchor="mm")
items=[((52,101,164),"Organizer"),((204,0,0),"water-opt — Builder"),((78,154,6),"water-opt — ORCA Editor"),((117,80,123),"water-opt — Launcher")]
def caption(d,y,text):
    d.rectangle([0,y,1500,1100],fill=(255,255,255)); d.text((16,y+14),text,font=FS,fill=(60,60,60))

# A: browser-style tabs under the toolbar; the active tab's menus and toolbar
A=Image.new("RGB",(1500,1100),BG); d=ImageDraw.Draw(A)
A.paste(bld.crop((0,0,1500,68)),(0,0))
tabs(d,0,68,34,items,1)
A.paste(bld.crop((0,68,1500,1064)),(0,103))
caption(d,1062,"A  Tabs across the window. Menu bar and toolbar belong to the open tab. The Organizer is the first tab. Drag a tab out (or right-click > Open in Window) for a separate window.")
A.save("tabs-A.png")

# B: Organizer tree always on the left; tabs on the right
B=Image.new("RGB",(1500,1100),BG); d=ImageDraw.Draw(B)
B.paste(bld.crop((0,0,1500,68)),(0,0))
d.rectangle([0,68,250,1060],fill=(255,255,255)); tree=org.crop((0,67,250,686))
B.paste(tree,(0,68)); d.line([251,68,251,1060],fill=LINE,width=3)
tabs(d,254,68,34,items[1:],0)
content=bld.crop((0,68,1500,1064)).resize((1246,int(996*1246/1500)))
B.paste(content,(254,103))
d.rectangle([254,103+content.size[1],1500,1060],fill=BG)
caption(d,1062,"B  The Organizer's tree stays on the left. Each tool opens as a tab on the right; its menus and toolbar replace the bar above. A tab can still be dragged out into a window.")
B.save("tabs-B.png")
