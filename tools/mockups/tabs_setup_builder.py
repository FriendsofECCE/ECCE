exec(open("mksetup.py").read().split("frame(\"Theory")[0])
items[:]=[((52,101,164),"Organizer"),((204,0,0),"water-opt — Set-up"),((117,80,123),"water-opt — Results")]
img=Image.new("RGB",(W,Hh),BG)
big=Image.new("RGB",(1500,1100),BG); h=titlebar(big,1)
img.paste(big.crop((0,0,W,h)),(0,0)); d=ImageDraw.Draw(img)
# one menu bar + toolbar for the tab: builder's, trimmed
img.paste(bld.crop((0,0,W,68)),(0,h))
top=h+68
left=cal.crop((0,24,514,596)); img.paste(left,(0,top+2))
d.line([518,top,518,Hh],fill=LINE,width=2)
# builder: viewer + narrowed panel column, scaled to fit
X0=522; avail=W-X0
viewer=bld.crop((0,68,1027,1015)); panels=bld.crop((1071,68,1500,1015))
vw=avail-300; sc=vw/1027
v=viewer.resize((vw,int(947*sc))); p=panels.resize((300,int(947*300/429)))
img.paste(v,(X0,top+2)); img.paste(p.crop((0,0,300,Hh-top)),(X0+vw,top+2))
d.rectangle([0,Hh-26,W,Hh],fill=(255,255,255))
d.text((10,Hh-20),"Set-up tab: the Calculation Editor on the left, the Builder on the right (viewer ~560 px, panels narrowed to 300 px). Results open in their own tab.",font=FS,fill=(60,60,60))
img.save("setup-editor-builder.png")
