exec(open(__file__.replace("tabs_c.py","tabs_ab.py")).read().split("# A:")[0])
SS=3  # supersample for smooth curves
TB=(222,221,218)
TW=250; H=42; R=10
def titlebar(img,active):
    W=1500
    big=Image.new("RGB",(W*SS,(H+2)*SS),TB); d=ImageDraw.Draw(big)
    s=lambda v:int(v*SS)
    x0=8
    for i,(icon,t) in enumerate(items):
        x=x0+i*TW; on=i==active
        if on:
            # filled tab, rounded top, flares into the bar below
            d.rounded_rectangle([s(x),s(7),s(x+TW),s(H+R)],radius=s(R),fill=BG)
            d.rectangle([s(x-R),s(H-R),s(x+TW+R),s(H+2)],fill=BG)
            d.pieslice([s(x-2*R),s(H-2*R),s(x),s(H)],0,90,fill=TB)
            d.pieslice([s(x+TW),s(H-2*R),s(x+TW+2*R),s(H)],90,180,fill=TB)
        elif i+1!=active and i!=len(items)-1:
            d.line([s(x+TW),s(16),s(x+TW),s(H-8)],fill=(170,168,165),width=s(1))
    img.paste(big.resize((W,H+2),Image.LANCZOS),(0,0))
    d=ImageDraw.Draw(img)
    for i,(icon,t) in enumerate(items):
        x=x0+i*TW; on=i==active
        d.ellipse([x+14,H//2-1,x+24,H//2+9],fill=icon)
        # title clipped with a fade before the close button
        lab=Image.new("RGB",(TW-70,24),BG if on else TB); ld=ImageDraw.Draw(lab)
        ld.text((0,12),t,font=F,fill=(30,30,30),anchor="lm")
        fadew=30
        for k in range(fadew):
            a=k/fadew; xx=lab.size[0]-fadew+k
            for yy in range(lab.size[1]):
                r,g,b=lab.getpixel((xx,yy)); c=BG if on else TB
                lab.putpixel((xx,yy),tuple(int(r*(1-a)+c[j]*a) for j,_ in enumerate(c)))
        img.paste(lab,(x+32,H//2-8))
        if i>0: d.text((x+TW-20,H//2+4),"×",font=F,fill=(90,90,90),anchor="mm")
    xp=x0+len(items)*TW
    d.text((xp+20,H//2+4),"+",font=FB,fill=(80,80,80),anchor="mm")
    for k,sym in enumerate(["–","□","×"]):
        d.text((1500-110+k*38,H//2+3),sym,font=FB,fill=(60,60,60),anchor="mm")
    return H+2
C=Image.new("RGB",(1500,1100),BG); h=titlebar(C,1); d=ImageDraw.Draw(C)
C.paste(bld.crop((0,0,1500,1100-h-40)),(0,h))
caption(d,1062,"C  Tabs at the top of the window, Chrome style; the open tab joins the bar below. Each tab has its own menus and toolbar: here the Builder's.")
C.save("tabs-C1.png")
C=Image.new("RGB",(1500,1100),BG); h=titlebar(C,0); d=ImageDraw.Draw(C)
o=org.resize((1500,int(800*1500/1280)))
C.paste(o.crop((0,0,1500,min(o.size[1],1100-h-40))),(0,h))
caption(d,1062,"C  The same window with the Organizer tab open: its own menus and toolbar. Drag a tab out for a separate window.")
C.save("tabs-C2.png")
