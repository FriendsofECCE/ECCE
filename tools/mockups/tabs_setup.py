exec(open("mkc2.py").read().split("C=Image.new")[0])
items[:]=[((52,101,164),"Organizer"),((204,0,0),"water-opt — Structure"),((78,154,6),"water-opt — Set-up")]
cal=Image.open("/home/andy/tmp/ecce/ECCE/help/src/img/calced-water.png").convert("RGB")
W,Hh=1366,768
def frame(detail_title, body_fn, active_btn):
    img=Image.new("RGB",(W,Hh),BG)
    big=Image.new("RGB",(1500,1100),BG); h=titlebar(big,2)
    img.paste(big.crop((0,0,W,h)),(0,0))
    d=ImageDraw.Draw(img)
    # left column: real editor, minus its own menu (File Help) and status area
    left=cal.crop((0,24,514,596))
    img.paste(left,(0,h+4))
    # highlight the active details button
    boxes={"theory":(12,418-24,151,451-24),"runtype":(265,418-24,413,451-24)}
    if active_btn in boxes:
        x0,y0,x1,y1=boxes[active_btn]
        d.rounded_rectangle([x0,y0+h+4,x1,y1+h+4],radius=5,outline=ACC,width=3)
    d.line([518,h+4,518,Hh],fill=LINE,width=2)
    # detail area
    X0=526; Y0=h+10
    d.rectangle([X0,Y0,W-10,Hh-10],fill=(255,255,255),outline=LINE)
    d.text((X0+14,Y0+12),detail_title,font=FB,fill=(30,30,30))
    d.line([X0+10,Y0+38,W-20,Y0+38],fill=LINE)
    body_fn(d,X0+14,Y0+52)
    return img
def form(rows):
    def f(d,x,y):
        for i,(lab,val) in enumerate(rows):
            yy=y+i*34
            d.text((x,yy+8),lab,font=F,fill=(40,40,40))
            d.rounded_rectangle([x+300,yy,x+520,yy+28],radius=4,fill=(250,250,250),outline=LINE)
            d.text((x+310,yy+14),val,font=F,fill=(30,30,30),anchor="lm")
    return f
theory=form([("SCF integrals","Semi-Direct"),("SCF convergence algorithm","Quadratic"),("Convergence gradient (Hartree)","0.0001"),("Maximum SCF iterations","20"),("Level shift","0.5"),("Damping","0"),("Initial guess","Atomic"),("Memory per core (GB)","2")])
runtype=form([("Optimisation algorithm","NWChem Driver"),("Search for","Minimum"),("Geometry convergence","Default"),("Maximum steps","20"),("Trust radius (bohr)","0.3"),("Hessian update","BFGS"),("Print every step","yes")])
def inputfile(d,x,y):
    lines=["echo","start water-opt","title \"water-opt\"","charge 0","geometry units angstroms","  O   0.000000  0.000000  0.000000","  H   0.766044  0.642788  0.000000","  H  -0.766044  0.642788  0.000000","end","basis \"ao basis\" cartesian","  * library 6-31G*","end","scf","  rhf; singlet","  maxiter 20","  thresh 1.0e-4","end","driver","  maxiter 20","end","task scf optimize"]
    M=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",14)
    for i,l in enumerate(lines): d.text((x,y+i*21),l,font=M,fill=(30,30,30))
frame("Theory details  (RHF)",theory,"theory").save("setup-theory.png")
frame("Input file as it will be written  (updates as you change settings)",inputfile,None).save("setup-input.png")
