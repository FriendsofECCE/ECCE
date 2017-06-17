      program main
c
c   coords: original coordinates and transformed coordinates
c
c   charge: real charges on nuclei
c
c   tags: atomic labels
c
c   ncenter: number of atoms
c
c   threquiv: tolerance value
c
c   groupname: symmetry groupname
c
      implicit none
      integer      rtdb
      integer      mxatom
      parameter    (mxatom=2000)
      integer      i
      integer      ncenter
      integer      nuc
      integer      nat
      logical      dbug 
      logical      some 
      double precision threquiv
      double precision coords
      double precision charge
      double precision c
      double precision zan
      double precision c3
      character*16 tags
      character*16 atmlab
      character*8  groupname
      common/hnd_molnuc/nuc(mxatom)
      common/hnd_mollab/atmlab(mxatom)
      common/hnd_molxyz/c(3,mxatom),zan(mxatom),nat
      dimension coords(3,mxatom)
      dimension charge(mxatom)
      dimension   tags(mxatom)
      dimension c3(3,mxatom)
      logical odone
c
c   read in information from standard input.
c
      read(5,*) ncenter
      if (ncenter.gt.mxatom) call exit(2)
      read(5,*) threquiv
      do i = 1, ncenter
        read(5,100) tags(i)
  100   format(a16)
        read(5,*) nuc(i),coords(1,i),coords(2,i),coords(3,i)
      end do
c
      dbug=.false.
      some=.false.
      some=some.or.dbug
c
      nat=ncenter
      do i=1,ncenter
         atmlab(i)=tags(i)
         charge(i) = dble(nuc(i))
         zan(i)=     charge(i) 
         c(1,i)=coords(1,i)
         c(2,i)=coords(2,i)
         c(3,i)=coords(3,i)
      enddo
c
      odone=.false.
c
c     ----- call auto_sym -----
c
      call hnd_autsym(odone,rtdb,threquiv,c3,groupname)
c
      do i=1,ncenter
         coords(1,i)=c(1,i)
         coords(2,i)=c(2,i)
         coords(3,i)=c(3,i)
      enddo
c
c   write out results to standard output
c
      write(6,200) groupname
  200 format(a8)
      do i=1, ncenter
        write(6,300) coords(1,i),coords(2,i),coords(3,i)
  300   format(3e24.14)
      end do
c
      end
      SUBROUTINE HND_AUTSYM(ODONE,RTDB,THREQUIV,c3,groupname)
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
      INTEGER     RTDB
      LOGICAL     ODONE
      PARAMETER   (MXATOM=2000)
      PARAMETER   (MXSYM =120)
      CHARACTER*8 ERRMSG,groupname
      CHARACTER*1 XYZNAM
      CHARACTER*1 ABCNAM
      LOGICAL     SOME
      LOGICAL     OUT
      LOGICAL     DBUG
      COMMON/HND_IOFILE/IR,IW
      COMMON/HND_MOLXYZ/C(3,MXATOM),ZAN(MXATOM),NAT
      COMMON/HND_SYMTRY/INVT(MXSYM),NT,NTMAX,NTWD,NOSYM
      DIMENSION   CI(3),AI(3,3)
      DIMENSION   C1(3,MXATOM)
      DIMENSION   C2(3,MXATOM)
      DIMENSION   C3(3,MXATOM)
      DIMENSION   AXS(3,3),EIG(3)
      DIMENSION   RT(3,3)
      DIMENSION   TR(3)
      DIMENSION   XYZNAM(3),ABCNAM(3)
      DIMENSION   ERRMSG(3)
      DATA ERRMSG /'PROGRAM ','STOP IN ','-AUTSYM-'/
      DATA ZERO   /0.0D+00/
      DATA ONE    /1.0D+00/
      DATA XYZNAM /'X','Y','Z'/
      DATA ABCNAM /'A','B','C'/
C
      DBUG=.FALSE.
      OUT =.FALSE. 
      OUT =OUT.OR.DBUG
      SOME=.FALSE.  
      SOME=SOME.OR.OUT
C
      NT=1
      NTMAX = 1                 ! Never seems to be set
      IF(NT.GT.1.OR.NTMAX.GT.1) THEN
         ODONE=.TRUE.
         RETURN
      ELSE
         ODONE=.FALSE.
      ENDIF
C
C     ----- AUTOMATIC DETECTION OF SYMMETRY -----
C
cbjp      IF(SOME) THEN
cbjp         WRITE(IW,9997)
cbjp         IF(OUT) THEN
cbjp            WRITE(IW,*) 'NT,NTMAX,NTWD,NOSYM = ',
cbjp     1                   NT,NTMAX,NTWD,NOSYM
cbjp         ENDIF
cbjp      ENDIF
C
C     ----- GET MOMENTS OF INERTIA -----                
C
      CALL HND_MOLINR(C,C1,NAT,CI,AI,TR)
C
C     ----- GET PRINCIPAL AXES -----
C
      CALL HND_MOLAXS(AI,AXS,EIG,3,3,3)
      IF(OUT) THEN
cbjp         WRITE(IW,9999)
         CALL HND_PRSQ(AXS,3,3,3)
      ENDIF
C
C     ----- IDENTIFY SYMMETRY OPERATIONS -----
C
      CALL HND_MOLOPS(C,C1,C2,C3,NAT,AXS,EIG,RT,TR,ODONE,THREQUIV,
     $     groupname)
C
C     ----- CREATE ATOM AND SHELL MAPPINGS -----
C
c$$$      CALL HND_MOLMAP(C,C3,NAT)
C
cbjp      IF(SOME) THEN
cbjp         IF(OUT) THEN
cbjp            WRITE(IW,*) 'NT,NTMAX,NTWD,NOSYM = ',
cbjp     1                   NT,NTMAX,NTWD,NOSYM
cbjp         ENDIF
cbjp         WRITE(IW,9996)
cbjp      ENDIF
C
C     ----- SYMMETRIZE CARTESIAN COORDINATES -----
C
c$$$      CALL HND_SYMATM
c
      do i = 1, nat
         c(1,i) = c3(1,i)
         c(2,i) = c3(2,i)
         c(3,i) = c3(3,i)
      enddo
C
      RETURN
 9999 FORMAT(/,1X,'PRINCIPAL AXES OF INERTIA',/,1X,25(1H-))
 9998 FORMAT(/,1X,'ANGLES ASSOCIATED WITH AXES',/,1X,27(1H-))
 9997 FORMAT(/,10X,7(1H-),                             
     1       /,10X,'autosym', 
     2       /,10X,7(1H-))
 9996 FORMAT(/)
      END
      SUBROUTINE HND_MOLOPS(C,C1,C2,C3,NAT,AXS,EIG,RT,TR,ODONE,
     $     THREQUIV,groupname)
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
C
C     ----- ROUTINE DETECTS MOLECULAR SYMMETRY OPERATIONS -----
C           CODE ADAPTED FROM A.M.CHAKA's ORIGINAL CODE.
c
c     RJH ... added parameter THREQUIV to provide user control
c     .       for noisy geometries, and groupname to return value
C
      LOGICAL     ODONE
      PARAMETER   (MXORDR=24)
      PARAMETER   (MXATOM=2000)
      CHARACTER*8 ERRMSG,groupname
      LOGICAL     MUCH
      LOGICAL     DBUG
      LOGICAL     OUT
      LOGICAL     SOME
      LOGICAL     ONLY
      LOGICAL     GOTX,GOTY,GOTZ
      LOGICAL     GOTONE
      LOGICAL     NOSYM
      LOGICAL     PROPER
      LOGICAL     PRPAXS
      LOGICAL     IMPROP
      LOGICAL     IMPAXS
      LOGICAL     SYMINV
      LOGICAL     INVERS
      LOGICAL     SYMC2X
      LOGICAL     SYMC2Y
      LOGICAL     MIRRYZ
      LOGICAL     MIRRZX
      LOGICAL     MIRRXY
      LOGICAL     MIRROR
      LOGICAL     ATOMIC
      LOGICAL     LINEAR
      LOGICAL     DEGNR2
      LOGICAL     DEGNR3
      LOGICAL     CUBIC
      LOGICAL     C2ROT
      LOGICAL     C2AXS
      LOGICAL     C4ROT
      LOGICAL     C4AXS
      LOGICAL     S4ROT
      LOGICAL     S4AXS
      LOGICAL     GRPOH
      LOGICAL     GRPTH
      LOGICAL     GRPTD
      LOGICAL     GRPT
      LOGICAL     GRPO
      INTEGER     AXORDR 
cbjp      COMPLEX*16  QX,QY,QZ
      DOUBLE PRECISION  QXR,QYR,QZR
      DOUBLE PRECISION  QXI,QYI,QZI
      COMPLEX*16  QZERO
cbjp      COMPLEX*16  QDUMX,QDUMY,QDUMZ
      DOUBLE PRECISION  QDUMXR,QDUMYR,QDUMZR
      DOUBLE PRECISION  QDUMXI,QDUMYI,QDUMZI
cbjp      COMPLEX*16  QDUMI,QDUMJ
      DOUBLE PRECISION  QDUMIR,QDUMJR,QSAVR,QSAVI
      DOUBLE PRECISION  QDUMII,QDUMJI
      COMMON/HND_IOFILE/IR,IW
      COMMON/HND_MOLNUC/NUC(MXATOM)
      DIMENSION IEQU(MXATOM)
      DIMENSION   QXR(MXORDR),  QYR(MXORDR),  QZR(MXORDR)
      DIMENSION   QXI(MXORDR),  QYI(MXORDR),  QZI(MXORDR)
      DIMENSION XMAG(MXORDR),YMAG(MXORDR),ZMAG(MXORDR)
      DIMENSION PROPER(3)
      DIMENSION IMPROP(3)
      DIMENSION AXORDR(3)
      DIMENSION NEWAXS(3)
      DIMENSION NUORDR(3)
      DIMENSION AXS(3,3),EIG(3)
      DIMENSION  RT(3,3), TR(3)
      DIMENSION PRM(3,3)
      DIMENSION  C(3,*)
      DIMENSION C1(3,*)
      DIMENSION C2(3,*)
      DIMENSION C3(3,*)
      DIMENSION CM(3),AM(3,3),TM(3)
      DIMENSION AXM(3,3)
      DIMENSION C2AXS(3)
      DIMENSION C4AXS(3)
      DIMENSION S4AXS(3)
      DIMENSION ERRMSG(3)
      double precision big,deter3,check_row
      external deter3
      EQUIVALENCE (DEGNR3,CUBIC)
      DATA ERRMSG /'PROGRAM ','STOP IN ','-MOLOPS-'/
      DATA ZERO   /0.0D+00/
      DATA ONE    /1.0D+00/
      DATA TWO    /2.0D+00/
      DATA THREE  /3.0D+00/
      DATA FOUR   /4.0D+00/
      DATA TENM01 /1.0D-01/
      DATA TENM02 /1.0D-02/
      DATA TENM04 /1.0D-04/
      DATA TENM05 /1.0D-05/
      DATA TENM09 /1.0D-09/
      DATA TEN06  /1.0D+06/
      DATA EPS    /1.01D+00/
C
      QZERO=cmplx(ZERO,ZERO)
         PI=    FOUR*ATAN(ONE)
      TWOPI=TWO*FOUR*ATAN(ONE)
      DEGREE=360.00D+00/TWOPI
c
****      THREQUIV = TENM04
C
      MUCH=.FALSE.
      DBUG=.FALSE. 
      DBUG=DBUG.OR.MUCH
      OUT =.FALSE.
      OUT =OUT.OR.DBUG
      SOME=.FALSE.
      SOME=SOME.OR.OUT
      ONLY=.TRUE. 
      ONLY=ONLY.OR.SOME
C
C     ----- INERTIA AXES AND DATA -----
C
      IF(OUT) THEN
cbjp         WRITE(IW,9986)
cbjp         WRITE(IW,9978) (I,EIG(I),I=1,3)
         CALL HND_PRSQ(AXS,3,3,3)
      ENDIF
C
C     ----- WE ARE INTERESTED IN RELATIVE ERROR -----
C
      DUM=(EIG(1)+EIG(2)+EIG(3))/THREE
      EIG(1)=EIG(1)/DUM
      EIG(2)=EIG(2)/DUM
      EIG(3)=EIG(3)/DUM
C
C     ----- CHECK FOR ATOM OR LINEAR MOLECULE -----
C
      NZER=0
      DO I=1,3
         IF(ABS(EIG(I)).LT.TENM04) NZER=NZER+1
      ENDDO
      IF(NZER.EQ.0) THEN
         ATOMIC=.FALSE.
         LINEAR=.FALSE.
      ELSEIF(NZER.EQ.1) THEN
         ATOMIC=.FALSE.
         LINEAR=.TRUE.
      ELSEIF(NZER.GT.1) THEN
         ATOMIC=.TRUE.
         LINEAR=.FALSE.
      ENDIF
cbjp      IF(SOME) THEN
cbjp         IF(ATOMIC) WRITE(IW,*) 'THIS IS AN ATOM'
cbjp         IF(LINEAR) WRITE(IW,*) 'THIS IS A LINEAR MOLECULE'
cbjp      ENDIF
      IF(ATOMIC) THEN
         RETURN
      ENDIF
C 
C     ----- CHECK FOR DEGENERACY -----
C
      DEGNR2=(ABS(EIG(2)-EIG(1)).LT.TENM04).OR.
     1       (ABS(EIG(2)-EIG(3)).LT.TENM04)
      DEGNR3=(ABS(EIG(2)-EIG(1)).LT.TENM04).AND.
     1       (ABS(EIG(2)-EIG(3)).LT.TENM04)
C
      IF(DEGNR3) THEN
C
C     ----- CUBIC GROUPS -----
C
         DEGNR2=.FALSE.
cbjp         IF(SOME) THEN
cbjp            WRITE(IW,*) '3-FOLD DEGENERATE MOMENTS ... CUBIC GROUP'
cbjp         ENDIF
         GRPTH=.FALSE.
         GRPOH=.FALSE.
         GRPTD=.FALSE.
         GRPT =.FALSE.
         GRPO =.FALSE.
C
C     ----- DETERMINE PRESENCE OF CENTER OF INVERSION -----
C                   Th AND Oh GROUPS
C
         SYMINV=.TRUE.
         DO IAT=1,NAT
            INVERS=.FALSE.
            DO JAT=1,NAT
               IF(NUC(JAT).EQ.NUC(IAT).AND..NOT.INVERS) THEN
                  DX=C1(1,IAT)+C1(1,JAT)
                  DY=C1(2,IAT)+C1(2,JAT)
                  DZ=C1(3,IAT)+C1(3,JAT)
                  INVERS=(ABS(DX).LT.THREQUIV).AND.
     1                   (ABS(DY).LT.THREQUIV).AND.
     2                   (ABS(DZ).LT.THREQUIV)
cbjp                  IF(DBUG) THEN
cbjp                     WRITE(IW,*) ' IAT,JAT,INVERS = ',
cbjp     1                             IAT,JAT,INVERS,DX,DY,DZ
cbjp                  ENDIF
               ENDIF
            ENDDO
            SYMINV=SYMINV.AND.INVERS
cbjp            IF(DBUG) THEN
cbjp               WRITE(IW,*) ' IAT,INVERS,SYMINV = ',IAT,INVERS,SYMINV
cbjp            ENDIF
         ENDDO
cbjp         IF(SOME) THEN
cbjp            WRITE(IW,9993) SYMINV
cbjp            IF(SYMINV) THEN
cbjp               WRITE(IW,*) ' Th OR Oh GROUP '
cbjp            ELSE
cbjp               WRITE(IW,*) ' T  OR Td OR O GROUP '
cbjp            ENDIF
cbjp         ENDIF
C
C     ----- DETERMINE ATOM EQUIVALENCIES -----
C
         DO IAT=1,NAT
            IEQU(IAT)=IAT
         ENDDO
         DO IAT=1,NAT
            IF(IEQU(IAT).EQ.IAT) THEN
               DUM=C1(1,IAT)**2+C1(2,IAT)**2+C1(3,IAT)**2
               DISTI=SQRT(DUM)
               IF(DISTI.GT.THREQUIV) THEN
                  DO JAT=1,NAT
                     IF(NUC(JAT).EQ.NUC(IAT).AND.JAT.NE.IAT) THEN
                        DUM=C1(1,JAT)**2+C1(2,JAT)**2+C1(3,JAT)**2
                        DISTJ=SQRT(DUM)
                        IF(ABS(DISTJ-DISTI).LT.THREQUIV) THEN
                           IEQU(JAT)=IAT
                        ENDIF
                     ENDIF
                  ENDDO
               ELSE
                  IEQU(IAT)=0
               ENDIF
            ENDIF
         ENDDO
cbjp         IF(OUT) THEN
cbjp           WRITE(IW,*) 'IEQU = ',(IEQU(JAT),JAT=1,NAT)
cbjp         ENDIF
C
C     ----- LOOK FOR 'GOOD' AXES BY DISTORTING THE MOLECULE -----
C           C2 AXES FOR -T - GROUP 
C           C2 AXES FOR -Th- GROUP 
C           S4 AXES FOR -Td- GROUP
C           C4 AXES FOR -O - GROUP
C           C4 AXES FOR -Oh- GROUP
C
         NAXM=0
         DO IAT=1,NAT
            IF(IEQU(IAT).EQ.IAT) THEN
               DO JAT=1,NAT
                  IF(JAT.NE.IAT.AND.IEQU(JAT).EQ.IAT) THEN
C
                     DO KAT=1,NAT
                        DO I=1,3
                           C2(I,KAT)=C1(I,KAT)
                        ENDDO
                     ENDDO
                     DO I=1,3
                        C2(I,IAT)=C1(I,IAT)*EPS
                        C2(I,JAT)=C1(I,JAT)*EPS
                     ENDDO
                     CALL HND_MOLINR(C2,C3,NAT,CM,AM,TM)
                     CALL HND_MOLAXS(AM,AXS,EIG,3,3,3)
                     IF(OUT) THEN
cbjp                        WRITE(IW,*) 'PRINCIPAL AXES OF ',
cbjp     1                              'DISTORTED MOLECULE ',
cbjp     2                              'IAT, JAT = ',IAT,JAT
                        CALL HND_PREV(AXS,EIG,3,3,3)
                     ENDIF
                     DO IIAT=1,NAT
                        DO J=1,3
                           DUM=ZERO
                           DO I=1,3
                              DUM=DUM+C1(I,IIAT)*AXS(I,J)
                           ENDDO
                           C2(J,IIAT)=DUM
                        ENDDO
                     ENDDO
cbjp                     IF(OUT) THEN
cbjp                        WRITE(IW,*) 'OLD COORDINATES'
cbjp                        DO IIAT=1,NAT
cbjp                           WRITE(IW,9989) IIAT,(C1(I,IIAT),I=1,3)
cbjp                        ENDDO
cbjp                        WRITE(IW,*) 'NEW COORDINATES'
cbjp                        DO IIAT=1,NAT
cbjp                           WRITE(IW,9989) IIAT,(C2(I,IIAT),I=1,3)
cbjp                        ENDDO
cbjp                     ENDIF
C
C     ----- CHECK THE AXES -----
C
                     DO IAXIS=1,3
                        IF(IAXIS.EQ.1) THEN
                           JAXIS=2
                           KAXIS=3
                        ELSEIF(IAXIS.EQ.2) THEN
                           JAXIS=3
                           KAXIS=1
                        ELSEIF(IAXIS.EQ.3) THEN
                           JAXIS=1
                           KAXIS=2
                        ENDIF
cbjp                        IF(DBUG) THEN
cbjp                           WRITE(IW,*) 'AXIS CHECK FOR -IAXIS- = ',IAXIS
cbjp                        ENDIF
                        C2AXS(IAXIS)=.TRUE.
                        C4AXS(IAXIS)=.TRUE.
                        S4AXS(IAXIS)=.TRUE.
                        DO IIAT=1,NAT
                           C2ROT=.FALSE.
                           JJC2 =0
                           DO JJAT=1,NAT
                              IF(.NOT.C2ROT) THEN
                                 D1=C2(IAXIS,IIAT)-C2(IAXIS,JJAT)                  
                                 D2=C2(JAXIS,IIAT)+C2(JAXIS,JJAT)                  
                                 D3=C2(KAXIS,IIAT)+C2(KAXIS,JJAT)                  
                                 C2ROT=(ABS(D1).LT.THREQUIV).AND.
     1                                 (ABS(D2).LT.THREQUIV).AND.
     2                                 (ABS(D3).LT.THREQUIV)
                                 IF(C2ROT) JJC2=JJAT
                              ENDIF
                           ENDDO
                           C4ROT=.FALSE.
                           JJC4 =0
                           DO JJAT=1,NAT
                              IF(.NOT.C4ROT) THEN
                                 D1=C2(IAXIS,IIAT)-C2(IAXIS,JJAT)
                                 D2=C2(JAXIS,IIAT)-C2(KAXIS,JJAT)
                                 D3=C2(KAXIS,IIAT)+C2(JAXIS,JJAT)
                                 C4ROT=(ABS(D1).LT.THREQUIV).AND.
     1                                 (ABS(D2).LT.THREQUIV).AND.
     2                                 (ABS(D3).LT.THREQUIV)
                                 IF(C4ROT) JJC4=JJAT
                              ENDIF
                           ENDDO
                           S4ROT=.FALSE.
                           JJS4 =0
                           DO JJAT=1,NAT
                              IF(.NOT.S4ROT) THEN
                                 D1=C2(IAXIS,IIAT)+C2(IAXIS,JJAT)
                                 D2=C2(JAXIS,IIAT)-C2(KAXIS,JJAT)
                                 D3=C2(KAXIS,IIAT)+C2(JAXIS,JJAT)
                                 S4ROT=(ABS(D1).LT.THREQUIV).AND.
     1                                 (ABS(D2).LT.THREQUIV).AND.
     2                                 (ABS(D3).LT.THREQUIV)
                                 IF(S4ROT) JJS4=JJAT
                              ENDIF
                           ENDDO
cbjp                           IF(DBUG) THEN
cbjp                              WRITE(IW,*) 'IIAT,JJC2,JJC4,JJS4 = ',
cbjp     1                                     IIAT,JJC2,JJC4,JJS4
cbjp                           ENDIF
                           C2AXS(IAXIS)=C2AXS(IAXIS).AND.C2ROT
                           C4AXS(IAXIS)=C4AXS(IAXIS).AND.C4ROT
                           S4AXS(IAXIS)=S4AXS(IAXIS).AND.S4ROT
                        ENDDO
                     ENDDO
cbjp                     IF(OUT) THEN
cbjp                        WRITE(IW,*) 'C2 AXES CHECK',(C2AXS(I),I=1,3)
cbjp                        WRITE(IW,*) 'C4 AXES CHECK',(C4AXS(I),I=1,3)
cbjp                        WRITE(IW,*) 'S4 AXES CHECK',(S4AXS(I),I=1,3)
cbjp                     ENDIF
C
C     ----- WE HAVE AXES THAT MAY BE C2 AND/OR C4 AND/OR S4 AXES -----
C           C2 AXES FOR -T - GROUP 
C           S4 AXES FOR -Td- GROUP
C           C4 AXES FOR -O - GROUP
C           C2 AXES FOR -Th- GROUP 
C           C4 AXES FOR -Oh- GROUP
C
                     IF(SYMINV) THEN
                        GRPOH=.FALSE.
                        DO IAXIS=1,3
                           GRPOH=GRPOH.OR.C4AXS(IAXIS)
                        ENDDO
                        GRPTH=.NOT.GRPOH
cbjp                        IF(OUT) THEN
cbjp                           WRITE(IW,*) 'TH , OH = ',GRPTH,GRPOH
cbjp                        ENDIF
                        GOTONE=.FALSE.
                        DO IAXIS=1,3
                           IF((GRPTH.AND.
     1                         C2AXS(IAXIS)                 ).OR. 
     2                        (GRPOH.AND.
     3                         C2AXS(IAXIS).AND.C4AXS(IAXIS))) THEN
                              NAXM=NAXM+1
c                             IF(NAXM.EQ.1) THEN
c                                GOTONE=.TRUE.
c                             ENDIF
c
c     check if you are adding an existing row
c
                                 gotone=.false.
                                 do j=1,3
                                    check_row=0d0
                                    do i=1,3
                                       check_row=check_row+
     +                                      abs(axs(i,iaxis)-axM(i,j))
                                    enddo
                                    if(check_row.lt.1d-2) gotone=.true.
                                 enddo
                              IF(NAXM.LE.3.and.(.not.gotone)) THEN
                                 DO I=1,3
                                    AXM(I,NAXM)=AXS(I,IAXIS)
                                 ENDDO
                                    else
                                       naxm=naxm-1
                                 endif
                              ENDIF
                        ENDDO
                     ELSE
                        GRPTD=.FALSE.
                        GRPO =.FALSE.
                        DO IAXIS=1,3
                           GRPTD=GRPTD.OR.S4AXS(IAXIS)
                           GRPO =GRPO .OR.C4AXS(IAXIS)
                        ENDDO
                        GRPT =.NOT.GRPTD.AND..NOT.GRPO
cbjp                        IF(OUT) THEN
cbjp                           WRITE(IW,*) 'T , TD , O = ',
cbjp     1                                  GRPT,GRPTD,GRPO
cbjp                        ENDIF
c                       GOTONE=.FALSE.
                        DO IAXIS=1,3
                           IF((GRPO .AND.
     1                         (C2AXS(IAXIS).AND.C4AXS(IAXIS))).OR.
     2                        (GRPTD.AND.
     3                         (C2AXS(IAXIS).AND.S4AXS(IAXIS))).OR.
     4                        (GRPT .AND.
     5                         (C2AXS(IAXIS)                 ))) THEN
                              NAXM=NAXM+1
c                             IF(NAXM.EQ.1) THEN
c                                GOTONE=.TRUE.
c                             ENDIF
                              IF(NAXM.LE.3) THEN
                                 DO I=1,3
                                    AXM(I,NAXM)=AXS(I,IAXIS)
                                 ENDDO
                              ENDIF
                           ENDIF
                        ENDDO
                     ENDIF
                     IF(OUT.AND.NAXM.GT.0) THEN
                        CALL HND_PRSQ(AXM,NAXM,3,3)
                     ENDIF
                     IF(NAXM.GE.2) GO TO 10
C
C     ----- CHECK VALIDITY OF SECOND AXIS -----
C
c                    IF(NAXM.EQ.2) THEN
c                       DUM =AXM(1,1)*AXM(1,2)+AXM(2,1)*AXM(2,2)+
c    1                       AXM(3,1)*AXM(3,2)
c                       DUMX=AXM(1,2)-DUM*AXM(1,1)
c                       DUMY=AXM(2,2)-DUM*AXM(2,1)
c                       DUMZ=AXM(3,2)-DUM*AXM(3,1)
c                       DUM =DUMX**2+DUMY**2+DUMZ**2
c                       DUM =SQRT(DUM)
c                       IF(DUM.GT.TENM02) THEN
c                       IF(DUM.GT.THREQUIV) THEN
c                          GO TO 10
c                       ELSE
c                          NAXM=1
c                       ENDIF
c                    ENDIF
C
                  ENDIF
               ENDDO
            ENDIF
         ENDDO
   10    CONTINUE
         DO IAXM=1,2
            DO I=1,3
               AXS(I,IAXM)=AXM(I,IAXM)
            ENDDO
         ENDDO
         AXS(1,3)=AXS(2,1)*AXS(3,2)-AXS(2,2)*AXS(3,1)
         AXS(2,3)=AXS(3,1)*AXS(1,2)-AXS(3,2)*AXS(1,1)
         AXS(3,3)=AXS(1,1)*AXS(2,2)-AXS(1,2)*AXS(2,1)
         if(abs(deter3(AXS)-1d0).gt.1d-2) then
            groupname='C1'
cbjp            write(IW,*) ' WARNING: autosym failed! '
            return
         endif
         IF(SOME) THEN
cbjp            WRITE(IW,*) ' NEW FRAME OF CUBIC GROUP '
            CALL HND_PRSQ(AXS,3,3,3)
         ENDIF
         IF(NAXM.LT.2) THEN
cbjp            WRITE(IW,*) ' AXIS DETECTION FAILED'
            RETURN
         ENDIF
      ENDIF
C
      IF(DEGNR2) THEN
C
C     ----- IF DEGENERACY, THEN MAKE ONE CHOICE AND -----
C           EXPRESS COORDINATES IN THAT NEW FRAME.
C
cbjp         IF(SOME) THEN
cbjp            WRITE(IW,*) '2-FOLD DEGENERATE MOMENTS OF INERTIA'
cbjp         ENDIF
         IF(ABS(EIG(2)-EIG(1)).LT.THREQUIV) KAXIS=3
         IF(ABS(EIG(2)-EIG(3)).LT.THREQUIV) KAXIS=1
cbjp         IF(OUT) THEN
cbjp            WRITE(IW,*) 'KAXIS = ',KAXIS
cbjp         ENDIF
         IF(KAXIS.NE.1.AND.KAXIS.NE.3) THEN
cbjp            WRITE(IW,*) 'SOMETHING WRONG WITH THRESHOLDS'
cbjp            WRITE(IW,*) 'KAXIS,DENGR2 = ',KAXIS,DEGNR2
            CALL HND_HNDERR(3,ERRMSG)
         ENDIF
         DO IAT=1,NAT
            DUM=C1(1,IAT)*C1(1,IAT)+C1(2,IAT)*C1(2,IAT)+    
     1                              C1(3,IAT)*C1(3,IAT)      
            DUM=SQRT(DUM)
            IF(DUM.GT.TENM05) THEN
cbjp               IF(OUT) THEN
cbjp                  WRITE(IW,*) 
cbjp     1         '-DUM- NEW FRAME DEFINED FROM ATOM -IAT- ',IAT,DUM
cbjp               ENDIF
               DIRX=C1(1,IAT)/DUM
               DIRY=C1(2,IAT)/DUM
               DIRZ=C1(3,IAT)/DUM
               TMP=DIRX*AXS(1,KAXIS)+DIRY*AXS(2,KAXIS)+
     1                               DIRZ*AXS(3,KAXIS)
               DIRX=DIRX-TMP*AXS(1,KAXIS)
               DIRY=DIRY-TMP*AXS(2,KAXIS)
               DIRZ=DIRZ-TMP*AXS(3,KAXIS)
               TMP=DIRX*DIRX+DIRY*DIRY+DIRZ*DIRZ
               TMP=SQRT(TMP)
               IF(TMP.GT.TENM05) THEN
cbjp                  IF(OUT) THEN
cbjp                     WRITE(IW,*) 
cbjp     1         '-TMP- NEW FRAME DEFINED FROM ATOM -IAT- ',IAT,TMP
cbjp                  ENDIF
                  JAXIS=2
                  AXS(1,JAXIS)=DIRX/TMP
                  AXS(2,JAXIS)=DIRY/TMP
                  AXS(3,JAXIS)=DIRZ/TMP
                  DIRX=AXS(2,JAXIS)*AXS(3,KAXIS)-
     1                 AXS(2,KAXIS)*AXS(3,JAXIS)
                  DIRY=AXS(3,JAXIS)*AXS(1,KAXIS)-
     1                 AXS(3,KAXIS)*AXS(1,JAXIS)
                  DIRZ=AXS(1,JAXIS)*AXS(2,KAXIS)-
     1                 AXS(1,KAXIS)*AXS(2,JAXIS)
                  IF(KAXIS.EQ.3) THEN
                     IAXIS=1
                     AXS(1,IAXIS)=DIRX
                     AXS(2,IAXIS)=DIRY
                     AXS(3,IAXIS)=DIRZ
                  ELSEIF(KAXIS.EQ.1) THEN
                     IAXIS=3
                     AXS(1,IAXIS)=-DIRX
                     AXS(2,IAXIS)=-DIRY
                     AXS(3,IAXIS)=-DIRZ
                  ENDIF
                  GO TO 20
               ENDIF
            ENDIF
         ENDDO
   20    CONTINUE
cbjp         IF(OUT) THEN
cbjp            WRITE(IW,*) 'NEW FRAME = '
cbjp            CALL HND_PRSQ(AXS,3,3,3)
cbjp         ENDIF
      ENDIF
C
C     ----- NO DEGENERACY -----
C
cbjp      IF(.NOT.DEGNR2.AND..NOT.DEGNR3) THEN
cbjp         IF(SOME) THEN
cbjp            WRITE(IW,*) 'MOMENTS OF INERTIA ARE NOT DEGENERATE'
cbjp         ENDIF
cbjp      ENDIF 
C
   30 CONTINUE
      IF(OUT) THEN
cbjp         WRITE(IW,*) '    FRAME = '
         CALL HND_PRSQ(AXS,3,3,3)
      ENDIF
C
C     ----- TRANSFORM COORDINATES TO PRINCIPAL AXES -----
C
      DO IAT=1,NAT
         DO J=1,3
            DUM=ZERO
            DO I=1,3
               DUM=DUM+C1(I,IAT)*AXS(I,J)
            ENDDO
            C2(J,IAT)=DUM
         ENDDO
      ENDDO
cbjp      IF(OUT) THEN
cbjp         WRITE(IW,9985)
cbjp         DO IAT=1,NAT
cbjp            WRITE(IW,9984) IAT,(C(I,IAT),I=1,3),(C1(I,IAT),I=1,3),
cbjp     1                                          (C2(I,IAT),I=1,3)
cbjp         ENDDO
cbjp      ENDIF
C
C     ----- DETERMINE ORDER OF PROPER/IMPROPER PRINCIPAL AXES -----  
C
      DO IORDR=1,MXORDR
cbjp         QX(IORDR)=QZERO 
cbjp         QY(IORDR)=QZERO 
cbjp         QZ(IORDR)=QZERO 
         QXR(IORDR) = ZERO
         QYR(IORDR) = ZERO
         QZR(IORDR) = ZERO
         QXI(IORDR) = ZERO
         QYI(IORDR) = ZERO
         QZI(IORDR) = ZERO
         DO IAT=1,NAT
cbjp            QDUMX=(CMPLX(C2(2,IAT),C2(3,IAT)))**IORDR
cbjp            QDUMY=(CMPLX(C2(1,IAT),C2(3,IAT)))**IORDR
cbjp            QDUMZ=(CMPLX(C2(1,IAT),C2(2,IAT)))**IORDR
            call powerz(C2(2,IAT),C2(3,IAT),IORDR,QDUMXR,QDUMXI)
            call powerz(C2(1,IAT),C2(3,IAT),IORDR,QDUMYR,QDUMYI)
            call powerz(C2(1,IAT),C2(2,IAT),IORDR,QDUMZR,QDUMZI)
cbjp            QX(IORDR)=QX(IORDR)+QDUMX
cbjp            QY(IORDR)=QY(IORDR)+QDUMY
cbjp            QZ(IORDR)=QZ(IORDR)+QDUMZ
            QXR(IORDR)=QXR(IORDR)+QDUMXR
            QXI(IORDR)=QXI(IORDR)+QDUMXI
            QYR(IORDR)=QYR(IORDR)+QDUMYR
            QYI(IORDR)=QYI(IORDR)+QDUMYI
            QZR(IORDR)=QZR(IORDR)+QDUMZR
            QZI(IORDR)=QZI(IORDR)+QDUMZI
cbjp            IF(MUCH) THEN
cbjp               WRITE(IW,9996) IORDR,IAT
cbjp               WRITE(IW,9995) QDUMX,QDUMY,QDUMZ
cbjp            ENDIF
         ENDDO
cbjp         XMAG(IORDR)=ABS(QX(IORDR))     
cbjp         YMAG(IORDR)=ABS(QY(IORDR))
cbjp         ZMAG(IORDR)=ABS(QZ(IORDR))
         XMAG(IORDR)=sqrt(QXR(IORDR)**2+QXI(IORDR)**2)     
         YMAG(IORDR)=sqrt(QYR(IORDR)**2+QYI(IORDR)**2)     
         ZMAG(IORDR)=sqrt(QZR(IORDR)**2+QZI(IORDR)**2)     
cbjp         IF(MUCH) THEN
cbjp            WRITE(IW,9998) QX(IORDR),QY(IORDR),QZ(IORDR)
cbjp         ENDIF
cbjp         IF(OUT) THEN
cbjp            WRITE(IW,9999) IORDR,XMAG(IORDR),YMAG(IORDR),ZMAG(IORDR)
cbjp         ENDIF
      ENDDO
C
      AXORDR(1)=1
      AXORDR(2)=1
      AXORDR(3)=1
      GOTX=.FALSE.
      GOTY=.FALSE.
      GOTZ=.FALSE.
      ethresh = 100.d+0 * threquiv
      DO IORDR=1,MXORDR
c        IF((XMAG(IORDR).GT.TENM01).AND.(.NOT.GOTX)) THEN
         IF((XMAG(IORDR).GT.ethresh).AND.(.NOT.GOTX)) THEN
            AXORDR(1)=IORDR
            GOTX=.TRUE.
         ENDIF
c        IF((YMAG(IORDR).GT.TENM01).AND.(.NOT.GOTY)) THEN
         IF((YMAG(IORDR).GT.ethresh).AND.(.NOT.GOTY)) THEN
            AXORDR(2)=IORDR
            GOTY=.TRUE.
         ENDIF
c        IF((ZMAG(IORDR).GT.TENM01).AND.(.NOT.GOTZ)) THEN
         IF((ZMAG(IORDR).GT.ethresh).AND.(.NOT.GOTZ)) THEN
            AXORDR(3)=IORDR
            GOTZ=.TRUE.
         ENDIF
c
c     correct for linear molecules with an infinite order axis.
c     it will be made a four fold axis
c
         if ((linear).and.(iordr.eq.mxordr)) then
cbjp           write(IW,*) 'ORDER OF PRIMARY AXIS IS BEING SET TO 4'
           if((xmag(iordr).le.ethresh).and.(.not.gotx)) then
             axordr(1)=4
             gotx=.true.
           endif
           if((ymag(iordr).le.ethresh).and.(.not.goty)) then
             axordr(2)=4
             goty=.true.
           endif
           if((zmag(iordr).le.ethresh).and.(.not.gotz)) then
             axordr(3)=4
             gotz=.true.
           endif
         endif
      ENDDO
cbjp      IF(OUT) THEN
cbjp         WRITE(IW,9997) (AXORDR(I),I=1,3)
cbjp      ENDIF
C
C     ----- PROPER OR IMPROPER ROTATION ?  -----
C
      DO IAXIS=1,3
         IF(IAXIS.EQ.1) THEN
            I=2
            J=3
            K=1
         ELSEIF(IAXIS.EQ.2) THEN
            I=3
            J=1
            K=2
         ELSEIF(IAXIS.EQ.3) THEN
            I=1
            J=2
            K=3
         ENDIF
         THETA=TWOPI/DBLE(AXORDR(IAXIS))
cbjp         IF(OUT) THEN
cbjp            WRITE(IW,9979) IAXIS,AXORDR(IAXIS)
cbjp         ENDIF
         PROPER(IAXIS)=.TRUE.
         DO IAT=1,NAT
            PRPAXS=.FALSE.
            DO JAT=1,NAT
               IF(NUC(JAT).EQ.NUC(IAT).AND..NOT.PRPAXS) THEN
cbjp                  QDUMI=CMPLX(C2(I,IAT),C2(J,IAT))
cbjp                  RR=ABS(QDUMI)
cbjp                  QDUMI=CMPLX(COS(THETA),SIN(THETA))*QDUMI
cbjp                  QDUMJ=CMPLX(C2(I,JAT),C2(J,JAT))
cbjp                  DD=ABS(QDUMI-QDUMJ)
                  QDUMIR=C2(I,IAT)
                  QDUMII=C2(J,IAT)
                  RR=sqrt(QDUMIR**2+QDUMII**2)
                  QSAVR = QDUMIR
                  QSAVI = QDUMII
                  QDUMIR = QSAVR*COS(THETA)-QSAVI*SIN(THETA)
                  QDUMII = QSAVI*COS(THETA)+QSAVR*SIN(THETA)
                  QDUMJR=C2(I,JAT)
                  QDUMJI=C2(J,JAT)
                  DD=sqrt((QDUMIR-QDUMJR)**2+(QDUMII-QDUMJI)**2)
                  DZ=C2(K,IAT)-C2(K,JAT)
                  PRPAXS=(ABS(DD).LT.THREQUIV).AND.
     1                   (ABS(DZ).LT.THREQUIV)
cbjp                  IF(DBUG) THEN
cbjp                     WRITE(IW,*) ' IAT,JAT,PRPAXS = ',
cbjp     1                             IAT,JAT,PRPAXS,RR,DD,DZ
cbjp                  ENDIF
               ENDIF
            ENDDO
            PROPER(IAXIS)=PROPER(IAXIS).AND.PRPAXS
cbjp            IF(DBUG) THEN
cbjp               WRITE(IW,*) ' IAT,PRPAXS,PROPER(IAXIS) = ',
cbjp     1                       IAT,PRPAXS,PROPER(IAXIS)
cbjp            ENDIF
         ENDDO
cbjp         IF(SOME) THEN
cbjp            WRITE(IW,9982) IAXIS,PROPER(IAXIS)
cbjp         ENDIF
         IMPROP(IAXIS)=.TRUE.
         DO IAT=1,NAT
            IMPAXS=.FALSE.
            DO JAT=1,NAT
               IF(NUC(JAT).EQ.NUC(IAT).AND..NOT.IMPAXS) THEN
cbjp                  QDUMI=CMPLX(C2(I,IAT),C2(J,IAT))
cbjp                  RR=ABS(QDUMI)
cbjp                  QDUMI=CMPLX(COS(THETA),SIN(THETA))*QDUMI
cbjp                  QDUMJ=CMPLX(C2(I,JAT),C2(J,JAT))
cbjp                  DD=ABS(QDUMI-QDUMJ)
                  QDUMIR = C2(I,IAT)
                  QDUMII = C2(J,IAT)
                  RR = sqrt(QDUMIR**2+QDUMII**2)
                  QSAVR = QDUMIR
                  QSAVI = QDUMII
                  QDUMIR = QSAVR*COS(THETA)-QSAVI*SIN(THETA)
                  QDUMII = QSAVI*COS(THETA)+QSAVR*SIN(THETA)
                  QDUMJR=C2(I,JAT)
                  QDUMJI=C2(J,JAT)
                  DD=sqrt((QDUMIR-QDUMJR)**2+(QDUMII-QDUMJI)**2)
                  DZ=C2(K,IAT)+C2(K,JAT)
                  IMPAXS=(ABS(DD).LT.THREQUIV).AND.
     1                   (ABS(DZ).LT.THREQUIV)
cbjp                  IF(DBUG) THEN
cbjp                     WRITE(IW,*) ' IAT,JAT,IMPAXS = ',
cbjp     1                             IAT,JAT,IMPAXS,RR,DD,DZ
cbjp                  ENDIF
               ENDIF
            ENDDO
            IMPROP(IAXIS)=IMPROP(IAXIS).AND.IMPAXS
cbjp            IF(DBUG) THEN
cbjp               WRITE(IW,*) ' IAT,IMPAXS,IMPROP(IAXIS) = ',
cbjp     1                       IAT,IMPAXS,IMPROP(IAXIS)
cbjp            ENDIF
         ENDDO
cbjp         IF(SOME) THEN
cbjp            WRITE(IW,9981) IAXIS,IMPROP(IAXIS)
cbjp         ENDIF
      ENDDO
C
c     do a check to fix if there is a high order axis that is not
c     proper and not improper
c
      if((.not.proper(1)).and.(.not.improp(1))) axordr(1)=1
      if((.not.proper(2)).and.(.not.improp(2))) axordr(2)=1
      if((.not.proper(3)).and.(.not.improp(3))) axordr(3)=1
c
c     IF((.NOT.PROPER(1)).AND.(.NOT.IMPROP(1)).AND.
c    1   (.NOT.PROPER(2)).AND.(.NOT.IMPROP(2)).AND.
c    2   (.NOT.PROPER(3)).AND.(.NOT.IMPROP(3))     ) THEN
c        WRITE(IW,*) 'AXES ARE NOT -PROPER- NOR -IMPROPER-.'
c        WRITE(IW,*) '-AUTO SYM- WILL STOP, THE CODE WILL CONTINUE.'
c        RETURN
c     ENDIF
C
C     ----- ALIGN AXIS OF HIGHEST ORDER ALONG -Z- -----
C
      IF((AXORDR(3).GT.AXORDR(1)).AND.
     1   (AXORDR(3).GT.AXORDR(2))     ) THEN
          KAXIS=3
      ELSEIF((AXORDR(2).GT.AXORDR(1)).AND.
     1       (AXORDR(2).GT.AXORDR(3))     ) THEN
          KAXIS=2
      ELSE
          KAXIS=1
      ENDIF
C
cbjp      IF(SOME) THEN
cbjp         WRITE(IW,9994) AXORDR(KAXIS)
cbjp      ENDIF
C
      IF(KAXIS.EQ.3) THEN
         NEWAXS(1)=1
         NEWAXS(2)=2
         NEWAXS(3)=3
      ELSEIF(KAXIS.EQ.2) THEN
         NEWAXS(1)=3
         NEWAXS(2)=1
         NEWAXS(3)=2
      ELSEIF(KAXIS.EQ.1) THEN
         NEWAXS(1)=2
         NEWAXS(2)=3
         NEWAXS(3)=1
      ENDIF
      DO K=1,3
         NUORDR(K)=AXORDR(NEWAXS(K))
      ENDDO
cbjp      IF(OUT) THEN
cbjp         WRITE(IW,9987) KAXIS
cbjp         WRITE(IW,9988) (NEWAXS(K),K=1,3)
cbjp         WRITE(IW,9980) (NUORDR(K),K=1,3)
cbjp      ENDIF
C
C     ----- ASSEMBLE PERMUTATION MATRIX -----
C
      DO J=1,3
         DO I=1,3
            PRM(I,J)=ZERO
         ENDDO
      ENDDO
      PRM(NEWAXS(1),1)=ONE
      PRM(NEWAXS(2),2)=ONE
      PRM(NEWAXS(3),3)=ONE
      IF(DBUG) THEN
cbjp         WRITE(IW,*) 'PERMUTATION MATRIX'
         CALL HND_PRSQ(PRM,3,3,3)
      ENDIF
C
C     ----- FIRST WAY OF GETTING -C3- FROM -C2- -----
C
      DO IAT=1,NAT
         DO J=1,3
            DUM=ZERO
            DO I=1,3   
               DUM=DUM+C2(I,IAT)*PRM(I,J) 
            ENDDO
            C3(J,IAT)=DUM
         ENDDO
      ENDDO
cbjp      IF(DBUG) THEN
cbjp         WRITE(IW,9990)
cbjp         DO IAT=1,NAT
cbjp            WRITE(IW,9989) IAT,(C3(I,IAT),I=1,3)
cbjp         ENDDO
cbjp      ENDIF
C
C     ----- SECOND WAY OF GETTING -C3- FROM -C2- -----
C
      DO IAT=1,NAT
         XX=C2(NEWAXS(1),IAT)
         YY=C2(NEWAXS(2),IAT)
         ZZ=C2(NEWAXS(3),IAT)
         C3(1,IAT)=XX
         C3(2,IAT)=YY
         C3(3,IAT)=ZZ
      ENDDO
cbjp      IF(DBUG) THEN
cbjp         WRITE(IW,9990)
cbjp         DO IAT=1,NAT
cbjp            WRITE(IW,9989) IAT,(C3(I,IAT),I=1,3)
cbjp         ENDDO
cbjp      ENDIF
C
C     ----- ASSEMBLE FINAL ROTATION MATRIX -----
C
      DO J=1,3
         DO I=1,3
            DUM=ZERO
            DO K=1,3
               DUM=DUM+AXS(I,K)*PRM(K,J)
            ENDDO
            RT(I,J)=DUM
         ENDDO
      ENDDO
C
C     ----- NOW GET -C3- FROM -C1- -----
C
      DO IAT=1,NAT
         DO J=1,3
            DUM=ZERO
            DO I=1,3
               DUM=DUM+C1(I,IAT)*RT(I,J)
            ENDDO
            C3(J,IAT)=DUM
         ENDDO
      ENDDO
cbjp      IF(OUT) THEN
cbjp         WRITE(IW,9990)
cbjp         DO IAT=1,NAT
cbjp            WRITE(IW,9989) IAT,(C3(I,IAT),I=1,3)
cbjp         ENDDO
cbjp      ENDIF
C
C     ----- C'2 AXES PERPENDICULAR TO PRINCIPAL AXIS -----
C
      SYMC2X=MOD(AXORDR(NEWAXS(1)),2).EQ.0.AND.
     1           PROPER(NEWAXS(1))
      SYMC2Y=MOD(AXORDR(NEWAXS(2)),2).EQ.0.AND.
     1           PROPER(NEWAXS(2))
c
c     Do more work if you don't find the C'2 axes right away
c
      if ((.not.symc2x).and.(.not.symc2y).and.
     1    (axordr(newaxs(3)).gt.1)) then
        fulltheta = TWOPI/DBLE(AXORDR(newaxs(3)))
        step = 0.5d+0 * (twopi / 360.d+0)
cgdb    do theta=0.d+0,fulltheta,step
        theta = 0.d+0
1000    if (theta.le.fulltheta) then
          if ((.not.symc2x).and.(.not.symc2y)) then
            do jat = 1, nat
              do iat = 1, 3
                c2(iat,jat) = c3(iat,jat)
              enddo
            enddo
c
            call rot_theta_z (theta,c2,nat)
c
            if(out) then
cbjp            write(6,*) 'New coordinates after theta of ',theta*degree
cbjp            do jat = 1, nat
cbjp              write(6,*) c2(1,jat), c2(2,jat), c2(3,jat)
cbjp            enddo
            endif
c

            call check_c2_perp(symc2x,symc2y,c2,nuc,
     1                         nat,threquiv)
          endif

          theta = theta + step
          goto 1000
        endif
cgdb    enddo
c
        if (symc2x.or.symc2y) then
          do jat = 1, nat
            do iat = 1, 3
              c3(iat,jat) = c2(iat,jat)
            enddo
          enddo
        endif
      endif
c
cbjp      IF(SOME) THEN
cbjp         WRITE(IW,9992) SYMC2X,SYMC2Y
cbjp      ENDIF
C
C     ----- CHECK : AXES OF ORDER HIGHER THAN 1 MUST  -----
C           BE EITHER PROPER OR IMPROPER. IF ALL AXES
C           ARE NEITHER PROPER NOR IMPROPER, THEN THE
C           ALGORITHM FAILED, AND IT SHOULD BE A 
C           - NO SYMMETRY - CASE .
C
      NOSYM=.FALSE.
      DO IAXIS=1,3
         NOSYM=NOSYM.AND.
     1        (.NOT.PROPER(IAXIS).AND..NOT.IMPROP(IAXIS))
      ENDDO
      IF(NOSYM) THEN
         AXORDR(KAXIS)=1
         DO J=1,3
            DO I=1,3
               RT(I,J)=ZERO
            ENDDO
            RT(J,J)=ONE
            TR(J  )=ZERO
         ENDDO
         DO IAT=1,NAT
            DO I=1,3
               C3(I,IAT)=C(I,IAT)
               C2(I,IAT)=C(I,IAT)
               C1(I,IAT)=C(I,IAT)
            ENDDO
         ENDDO
      ENDIF
C
C     ----- DETERMINE PRESENCE OF CENTER OF INVERSION -----
C
      SYMINV=.TRUE. 
      DO IAT=1,NAT
         INVERS=.FALSE.
         DO JAT=1,NAT
            IF(NUC(JAT).EQ.NUC(IAT).AND..NOT.INVERS) THEN
               DX=C3(1,IAT)+C3(1,JAT)
               DY=C3(2,IAT)+C3(2,JAT)
               DZ=C3(3,IAT)+C3(3,JAT)
               INVERS=(ABS(DX).LT.THREQUIV).AND.
     1                (ABS(DY).LT.THREQUIV).AND.
     2                (ABS(DZ).LT.THREQUIV)
cbjp               IF(DBUG) THEN
cbjp                  WRITE(IW,*) ' IAT,JAT,INVERS = ',
cbjp     1                          IAT,JAT,INVERS,DX,DY,DZ
cbjp               ENDIF
            ENDIF
         ENDDO
         SYMINV=SYMINV.AND.INVERS
cbjp         IF(DBUG) THEN
cbjp            WRITE(IW,*) ' IAT,INVERS,SYMINV = ',IAT,INVERS,SYMINV
cbjp         ENDIF
      ENDDO
cbjp      IF(SOME) THEN
cbjp         WRITE(IW,9993) SYMINV
cbjp      ENDIF
C
C     ----- MIRROR PLANES PERPENDICULAR TO PRINCIPAL AXES -----
C
      MIRRYZ=.TRUE. 
      DO IAT=1,NAT
         MIRROR=.FALSE.
         DO JAT=1,NAT
            IF(NUC(JAT).EQ.NUC(IAT).AND..NOT.MIRROR) THEN
               DX=C3(1,IAT)+C3(1,JAT)
               DY=C3(2,IAT)-C3(2,JAT)
               DZ=C3(3,IAT)-C3(3,JAT)
               MIRROR=(ABS(DX).LT.THREQUIV).AND.
     1                (ABS(DY).LT.THREQUIV).AND.
     2                (ABS(DZ).LT.THREQUIV)
cbjp               IF(DBUG) THEN
cbjp                  WRITE(IW,*) ' IAT,JAT,MIRRYZ = ',
cbjp     1                          IAT,JAT,MIRROR,DX,DY,DZ
cbjp               ENDIF
            ENDIF
         ENDDO
         MIRRYZ=MIRRYZ.AND.MIRROR
cbjp         IF(OUT) THEN
cbjp            WRITE(IW,*) ' IAT,MIRROR,MIRRYZ = ',IAT,MIRROR,MIRRYZ
cbjp         ENDIF
      ENDDO
      MIRRZX=.TRUE.
      DO IAT=1,NAT
         MIRROR=.FALSE.
         DO JAT=1,NAT
            IF(NUC(JAT).EQ.NUC(IAT).AND..NOT.MIRROR) THEN
               DX=C3(1,IAT)-C3(1,JAT)
               DY=C3(2,IAT)+C3(2,JAT)
               DZ=C3(3,IAT)-C3(3,JAT)
               MIRROR=(ABS(DX).LT.THREQUIV).AND.
     1                (ABS(DY).LT.THREQUIV).AND.
     2                (ABS(DZ).LT.THREQUIV)
cbjp               IF(DBUG) THEN
cbjp                  WRITE(IW,*) ' IAT,JAT,MIRRZX = ',
cbjp     1                          IAT,JAT,MIRROR,DX,DY,DZ
cbjp               ENDIF
            ENDIF
         ENDDO
         MIRRZX=MIRRZX.AND.MIRROR
cbjp         IF(OUT) THEN
cbjp            WRITE(IW,*) ' IAT,MIRROR,MIRRZX = ',IAT,MIRROR,MIRRZX
cbjp         ENDIF
      ENDDO
      MIRRXY=.TRUE.
      DO IAT=1,NAT
         MIRROR=.FALSE.
         DO JAT=1,NAT
            IF(NUC(JAT).EQ.NUC(IAT).AND..NOT.MIRROR) THEN
               DX=C3(1,IAT)-C3(1,JAT)
               DY=C3(2,IAT)-C3(2,JAT)
               DZ=C3(3,IAT)+C3(3,JAT)
               MIRROR=(ABS(DX).LT.THREQUIV).AND.
     1                (ABS(DY).LT.THREQUIV).AND.
     2                (ABS(DZ).LT.THREQUIV)
cbjp               IF(DBUG) THEN
cbjp                  WRITE(IW,*) ' IAT,JAT,MIRRXY = ',
cbjp     1                          IAT,JAT,MIRROR,DX,DY,DZ
cbjp               ENDIF
            ENDIF
         ENDDO
         MIRRXY=MIRRXY.AND.MIRROR
cbjp         IF(OUT) THEN
cbjp            WRITE(IW,*) ' IAT,MIRROR,MIRRXY = ',IAT,MIRROR,MIRRXY
cbjp         ENDIF
      ENDDO
cbjp      IF(SOME) THEN
cbjp         WRITE(IW,9991) MIRRYZ,MIRRZX,MIRRXY
cbjp      ENDIF         
c
c  Correct for the Cs being in the wrong mirror plane
c
      if ((axordr(kaxis).eq.1).and.(mirryz.or.mirrzx)) then
        if (mirryz) then ! switch z and x coordinates
          mirryz=.false.
          mirrxy=.true.
          do iat=1,nat
            temp=c3(1,iat)
            c3(1,iat)=c3(3,iat)
            c3(3,iat)=temp
          enddo
        else             ! switch z and y coordinates
          mirrzx=.false.
          mirrxy=.true.
          do iat=1,nat
            temp=c3(2,iat)
            c3(2,iat)=c3(3,iat)
            c3(3,iat)=temp
          enddo
        endif
      endif
C
C     ----- SET MOLECULAR POINT GROUP SYMBOL -----
C
      CALL HND_MOLSMB(MIRRYZ,MIRRZX,MIRRXY,
     1            SYMC2X,SYMC2Y,SYMINV,
     2            PROPER(NEWAXS(1)),PROPER(NEWAXS(2)),
     3            PROPER(NEWAXS(3)),
     4            CUBIC,GRPOH,GRPTH,GRPTD,GRPT,GRPO,
     5            AXORDR(KAXIS),TR,RT,ODONE,groupname)
C
c fix a few symmetries after the fact to go into nwchem order
c
cbjp      if (out) then
cbjp      write(6,*) 'before I muck with the geometry'
cbjp      DO IAT=1,NAT
cbjp        WRITE(IW,9989) IAT,(C3(I,IAT),I=1,3)
cbjp      ENDDO
cbjp      endif
      if ((groupname(1:3).eq.'C3V').or.(groupname(1:3).eq.'C6V').or.
     1  (groupname(1:2).eq.'D3').or.(groupname(1:2).eq.'D6')) then
        pi4 = atan(1d0)
        if (symc2x) then
          theta = pi4
        else if (symc2y) then
          theta = -pi4
        else if (mirrzx) then
          theta = pi4
        else if (mirryz) then
          theta = -pi4
        else
          theta = -1.d+0
        endif
        if (theta.eq.-1.d+0) call errquit
     1    ('hnd_molops: could not determine rotation angle',555)
        call rot_theta_z (theta,c3,nat)
      endif
c
      if ((groupname(1:3).eq.'C5V').and.mirryz) then
        pi2 = 2*atan(1d0)
        theta = pi2
        call rot_theta_z (theta,c3,nat)
      endif
c
      if (groupname(1:3).eq.'D4D') then
c       theta = tan(-0.38d+0/0.92d+0)    !+ 8.72663d-03
        theta = 22.5d+0/degree
        call rot_theta_z (theta,c3,nat)
      endif
c
      if (((groupname(1:3).eq.'D5H').or.(groupname(1:3).eq.'D5 '))
     1      .and.symc2x) then  ! used to be mirrzx
        pi2 = 2*atan(1.d+0)  ! pi divided by 2 (90 degrees)
        theta = pi2
        call rot_theta_z (theta,c3,nat)
      endif
c
      if ((groupname(1:3).eq.'D5D').and.symc2y) then
        pi2 = 2*atan(1.d+0)
        theta = -pi2
        call rot_theta_z (theta,c3,nat)
      endif
c
cbjp      IF(OUT) THEN
cbjp         WRITE(IW,9983)
cbjp         DO IAT=1,NAT
cbjp            WRITE(IW,9984) IAT,(C(I,IAT),I=1,3),(C1(I,IAT),I=1,3),
cbjp     1                                          (C3(I,IAT),I=1,3)
cbjp         ENDDO
cbjp      ENDIF
cbjp      IF(ONLY) THEN
cbjp         WRITE(IW,9977)
cbjp         DO IAT=1,NAT
cbjp            WRITE(IW,9984) IAT,(C3(I,IAT),I=1,3)
cbjp         ENDDO
cbjp      ENDIF
C
      RETURN
 9999 FORMAT(' IORDR= ',I3,' XMAG= ',G12.4,' YMAG= ',G12.4,
     1                                     ' ZMAG= ',G12.4)
 9998 FORMAT(' ABOUT X-AXIS = ',G12.4,' + i ',G12.4,/,
     1       ' ABOUT Y-AXIS = ',G12.4,' + i ',G12.4,/,
     2       ' ABOUT Z-AXIS = ',G12.4,' + i ',G12.4)
 9997 FORMAT(' X-AXIS ORDER = ',I3,' Y-AXIS ORDER = ',I3,
     1                             ' Z-AXIS ORDER = ',I3)
 9996 FORMAT(' IORDR,IAT = ',2I4)
 9995 FORMAT(2G12.4)
 9994 FORMAT(' HIGHEST ORDER OF PROPER/IMPROPER ROTATION = ',I2)
 9993 FORMAT(' INVERSION CENTER = ',L4)
 9992 FORMAT('           C2 -X- = ',L4,/,
     1       '           C2 -Y- = ',L4) 
 9991 FORMAT(' MIRROR PLANE -YZ-= ',L4,/,
     1       ' MIRROR PLANE -ZX-= ',L4,/,
     2       ' MIRROR PLANE -XY-= ',L4)
 9990 FORMAT(' COORDINATES AFTER ALIGNEMENT OF AXES ')
 9989 FORMAT(1X,I5,3F12.8)
 9988 FORMAT('           PERMUTATION IS = ',I1,1X,I1,1X,I1)
 9987 FORMAT(' AXIS OF HIGHEST ORDER IS = ',I1)
 9986 FORMAT(' INERTIA AXES AND DATA ')
 9985 FORMAT(/,17X,'ORIGINAL -XYZ-',22X,'MASS-CENTERED -XYZ-',
     1         17X,'ROTATED -XYZ-')
 9984 FORMAT(1X,I5,9F12.8)
 9983 FORMAT(/,17X,'ORIGINAL -XYZ-',22X,'MASS-CENTERED -XYZ-',
     1         17X,' FINAL  -XYZ-')
 9982 FORMAT('   PROPER PRINCIPAL AXIS ',I1,' = ',L4)
 9981 FORMAT(' IMPROPER PRINCIPAL AXIS ',I1,' = ',L4)
 9980 FORMAT(' ORDER OF PERMUTED AXES = ',3I3)
 9979 FORMAT(' PROPER/IMPROPER CHECK FOR -IAXIS- AND -ORDER- = ',2I3)
 9978 FORMAT(' I EIG = ',I5,F20.8)
 9977 FORMAT(' coordinates in local symmetry frame ',/)
      END
      SUBROUTINE HND_MOLSMB(MIRRYZ,MIRRZX,MIRRXY,
     1                     SYMC2X,SYMC2Y,SYMINV,
     2                     PROPRX,PROPRY,PROPRZ,
     3                     CUBIC,GRPOH,GRPTH,GRPTD,GRPT,GRPO,
     4                     AXORDR,TR,RT,ODONE,groupname)
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
      LOGICAL  ODONE
      LOGICAL  DBUG
      LOGICAL  OUT
      LOGICAL  SOME
      LOGICAL  PUNCH
      CHARACTER*8 ERRMSG
      CHARACTER*8 WRDSYM
      CHARACTER*8 WRDEND
      CHARACTER*8 GRPSAV
      CHARACTER*8 GROUP,groupname
      CHARACTER*8 BLANK
      CHARACTER*1 I2
      CHARACTER*1 B
      CHARACTER*1 C
      CHARACTER*1 S
      CHARACTER*1 D
      CHARACTER*1 N
      CHARACTER*1 V
      CHARACTER*1 H
      CHARACTER*2 C1
      CHARACTER*2 CI
      CHARACTER*2 CS
      CHARACTER*2 T 
      CHARACTER*2 TH
      CHARACTER*2 TD
      CHARACTER*2 O 
      CHARACTER*2 OH
      INTEGER  AXORDR
      LOGICAL  CUBIC
      LOGICAL  GRPOH
      LOGICAL  GRPTH
      LOGICAL  GRPTD
      LOGICAL  GRPT
      LOGICAL  GRPO
      LOGICAL  PROPRX
      LOGICAL  PROPRY
      LOGICAL  PROPRZ
      LOGICAL  SYMINV
      LOGICAL  SYMC2X
      LOGICAL  SYMC2Y
      LOGICAL  MIRRYZ
      LOGICAL  MIRRZX
      LOGICAL  MIRRXY
      COMMON/HND_IOFILE/IR,IW
      COMMON/HND_SYMMOL/COMPLX,IGROUP,NAXIS,LINEAR
      COMMON/HND_SYMNAM/GROUP
      DIMENSION TR(3),RT(3,3)
      DIMENSION ERRMSG(3)
      DATA ERRMSG /'PROGRAM ','STOP IN ','-MOLSMB-'/
      DATA BLANK  /'        '/
      DATA I2     /'2'/
      DATA B      /' '/
      DATA S      /'S'/
      DATA C      /'C'/
      DATA D      /'D'/
      DATA N      /'N'/
      DATA V      /'V'/
      DATA H      /'H'/
      DATA C1     /'C1'/
      DATA CI     /'CI'/
      DATA CS     /'CS'/
      DATA T      /'T '/
      DATA TH     /'TH'/
      DATA TD     /'TD'/
      DATA O      /'O '/
      DATA OH     /'OH'/
      DATA WRDSYM /' $SYM   '/
      DATA WRDEND /' $END   '/
C
      DBUG=.FALSE.
      OUT =.FALSE.
      OUT =OUT.OR.DBUG
      SOME=.TRUE. 
      SOME=SOME.OR.OUT
C
      PUNCH=.FALSE.
C
cbjp      IF(DBUG) THEN
cbjp         WRITE(IW,*) 'IN -MOLSMB- '
cbjp         WRITE(IW,*) 'MIRRYZ = ',MIRRYZ
cbjp         WRITE(IW,*) 'MIRRZX = ',MIRRZX
cbjp         WRITE(IW,*) 'MIRRXY = ',MIRRXY
cbjp         WRITE(IW,*) 'SYMC2X = ',SYMC2X
cbjp         WRITE(IW,*) 'SYMC2Y = ',SYMC2Y
cbjp         WRITE(IW,*) 'SYMINV = ',SYMINV
cbjp         WRITE(IW,*) 'PROPRX = ',PROPRX
cbjp         WRITE(IW,*) 'PROPRY = ',PROPRY
cbjp         WRITE(IW,*) 'PROPRZ = ',PROPRZ
cbjp         WRITE(IW,*) 'CUBIC  = ',CUBIC 
cbjp         WRITE(IW,*) 'GRPOH  = ',GRPOH 
cbjp         WRITE(IW,*) 'GRPTH  = ',GRPTH 
cbjp         WRITE(IW,*) 'GRPTD  = ',GRPTD 
cbjp         WRITE(IW,*) 'GRPT   = ',GRPO  
cbjp         WRITE(IW,*) 'GRPO   = ',GRPO  
cbjp         WRITE(IW,*) 'AXORDR = ',AXORDR
cbjp      ENDIF
C
      NAXSAV=NAXIS
      GRPSAV=GROUP
      IGRSAV=IGROUP
      LINSAV=LINEAR
      CMXSAV=COMPLX
C
C     ----- SET GROUP ORDER -----
C
      NAXIS=AXORDR
C
C     ----- SET GROUP SYMBOL -----
C
      GROUP=BLANK
      if (cubic) then
C
C     ----- CUBIC GROUPS -----
C
            IF(GRPOH) THEN
               GROUP(1:2)=OH
            ELSEIF(GRPTH) THEN
               GROUP(1:2)=TH
            ELSEIF(GRPTD) THEN
               GROUP(1:2)=TD
            ELSEIF(GRPT ) THEN
               GROUP(1:2)=T
            ELSEIF(GRPO ) THEN
               GROUP(1:2)=O
            ENDIF
      else
        IF(NAXIS.EQ.1) THEN
            GROUP(1:2)=C1(1:2)
            IF(SYMINV) THEN
               GROUP(1:2)=CI
            ENDIF
            IF(MIRRYZ.OR.MIRRZX.OR.MIRRXY) THEN
               GROUP(1:2)=CS
            ENDIF
        ELSE
            IF(MIRRYZ.OR.MIRRZX.OR.MIRRXY.OR.
     1         SYMC2X.OR.SYMC2Y.OR.
     2         PROPRX.OR.PROPRY.OR.PROPRZ.OR.
     3         (MOD(NAXIS,2).NE.0)           ) THEN
C
C     ----- C.. AND D.. GROUPS -----
C
               IF(SYMC2X.OR.SYMC2Y) THEN
C
C     ----- Dn , Dnh , Dnd -----
C
                  GROUP(1:1)=D
                  GROUP(2:2)=N
                  IF(MIRRXY) THEN
                     GROUP(3:3)=H
                  ELSE
                     IF(MIRRYZ.OR.MIRRZX.or.(.not.proprz)) THEN
                        GROUP(3:3)=D
                        NAXIS=NAXIS/2
                     ELSE
                        GROUP(3:3)=B
                     ENDIF
                  ENDIF
               ELSE
C
C     ----- Cn , Cnh , Cnv -----
C
                  GROUP(1:1)=C
                  GROUP(2:2)=N
                  IF(MIRRXY) THEN
                     GROUP(3:3)=H
                  ELSE
                     IF(MIRRYZ.OR.MIRRZX) THEN
                        GROUP(3:3)=V
                     ELSE
                        GROUP(3:3)=B
                     ENDIF
                  ENDIF
               ENDIF
            ELSE
C
C     ----- S2N GROUPS -----
C
               GROUP(1:1)=S
cbjp               write(GROUP(2:2),'(i1)') NAXIS
               if (NAXIS.eq.1) then
                 GROUP(2:2)='1'
               elseif(NAXIS.eq.2) then
                 GROUP(2:2)='2'
               elseif(NAXIS.eq.3) then
                 GROUP(2:2)='3'
               elseif(NAXIS.eq.4) then
                 GROUP(2:2)='4'
               elseif(NAXIS.eq.5) then
                 GROUP(2:2)='5'
               elseif(NAXIS.eq.6) then
                 GROUP(2:2)='6'
               elseif(NAXIS.eq.7) then
                 GROUP(2:2)='7'
               elseif(NAXIS.eq.8) then
                 GROUP(2:2)='8'
               elseif(NAXIS.eq.9) then
                 GROUP(2:2)='9'
               endif
cbjp
            ENDIF
        ENDIF
      ENDIF
C
      if (group(2:2).eq.'N') then
         groupname = ' '
cbjp         write(groupname,'(a1,i1,a6)') group(1:1),naxis,group(3:8)
         groupname(1:1) = group(1:1)
         if (naxis.eq.1) then
           groupname(2:2) = '1'
         elseif (naxis.eq.2) then
           groupname(2:2) = '2'
         elseif (naxis.eq.3) then
           groupname(2:2) = '3'
         elseif (naxis.eq.4) then
           groupname(2:2) = '4'
         elseif (naxis.eq.5) then
           groupname(2:2) = '5'
         elseif (naxis.eq.6) then
           groupname(2:2) = '6'
         elseif (naxis.eq.7) then
           groupname(2:2) = '7'
         elseif (naxis.eq.8) then
           groupname(2:2) = '8'
         elseif (naxis.eq.9) then
           groupname(2:2) = '9'
         endif
         groupname(3:8)=group(3:8)
      else
         groupname = group
      endif
c
c   This is an ugly cludge to get the ci point group which also shows
c   up with a S2!
c
      if (groupname(1:2).eq.'S2') then
         groupname(1:2)=CI
      endif
c
cbjp      IF(SOME) THEN
cbjp         WRITE(IW,9999) groupname
cbjp      ENDIF
cbjp      IF(OUT) THEN
cbjp         WRITE(IW,9995)  (TR(I  ),I=1,3),
cbjp     1                  ((RT(I,J),J=1,3),I=1,3)
cbjp      ENDIF
c
cbjp      IF(PUNCH) THEN
cbjp         WRITE(IW,9998) WRDSYM
cbjp         WRITE(IW,9997) GROUP(1:3),NAXIS
cbjp         WRITE(IW,9998) WRDEND
cbjp         WRITE(IW,*) 'TRANSLATION'
cbjp         WRITE(IW,9996) (TR(I),I=1,3)
cbjp         WRITE(IW,*) 'ROTATION   '
cbjp         WRITE(IW,9996) ((RT(I,J),J=1,3),I=1,3)
cbjp      ENDIF
C
C     ----- GET OPERATOR MATRICES -----
C
c     IF(GROUP(1:2).EQ.CS) THEN
c        XPT1=TR(1)
c        YPT1=TR(2)
c        ZPT1=TR(3)
c        IF(MIRRYZ) THEN
c           XPT2=XPT1+RT(1,2)
c           YPT2=YPT1+RT(2,2)
c           ZPT2=ZPT1+RT(3,2)
c           XPT3=XPT1+RT(1,3)
c           YPT3=YPT1+RT(2,3)
c           ZPT3=ZPT1+RT(3,3)
c        ELSEIF(MIRRZX) THEN
c           XPT2=XPT1+RT(1,3)
c           YPT2=YPT1+RT(2,3)
c           ZPT2=ZPT1+RT(3,3)
c           XPT3=XPT1+RT(1,1)
c           YPT3=YPT1+RT(2,1)
c           ZPT3=ZPT1+RT(3,1)
c        ELSEIF(MIRRXY) THEN
c           XPT2=XPT1+RT(1,1)
c           YPT2=YPT1+RT(2,1)
c           ZPT2=ZPT1+RT(3,1)
c           XPT3=XPT1+RT(1,2)
c           YPT3=YPT1+RT(2,2)
c           ZPT3=ZPT1+RT(3,2)
c        ENDIF
c     ELSE
c        IF(SYMC2X) THEN
c           XPT0=TR(1)
c           YPT0=TR(2)
c           ZPT0=TR(3)
c           XPT1=XPT0+RT(1,3)
c           YPT1=YPT0+RT(2,3)
c           ZPT1=ZPT0+RT(3,3)
c           XPT2=XPT0+RT(1,1)
c           YPT2=YPT0+RT(2,1)
c           ZPT2=ZPT0+RT(3,1)
c        ELSEIF(SYMC2Y) THEN
c           XPT0=TR(1)
c           YPT0=TR(2)
c           ZPT0=TR(3)
c           XPT1=XPT0+RT(1,3)
c           YPT1=YPT0+RT(2,3)
c           ZPT1=ZPT0+RT(3,3)
c           XPT2=XPT0+RT(1,2)
c           YPT2=YPT0+RT(2,2)
c           ZPT2=ZPT0+RT(3,2)
c        ELSEIF(MIRRZX) THEN
c           XPT0=TR(1)
c           YPT0=TR(2)
c           ZPT0=TR(3)
c           XPT1=XPT0+RT(1,3)
c           YPT1=YPT0+RT(2,3)
c           ZPT1=ZPT0+RT(3,3)
c           XPT2=XPT0+RT(1,1)
c           YPT2=YPT0+RT(2,1)
c           ZPT2=ZPT0+RT(3,1)
c        ELSEIF(MIRRYZ) THEN
c           XPT0=TR(1)
c           YPT0=TR(2)
c           ZPT0=TR(3)
c           XPT1=XPT0+RT(1,3)
c           YPT1=YPT0+RT(2,3)
c           ZPT1=ZPT0+RT(3,3)
c           XPT2=XPT0+RT(1,2)
c           YPT2=YPT0+RT(2,2)
c           ZPT2=ZPT0+RT(3,2)
c        ELSE
c           XPT0=TR(1)
c           YPT0=TR(2)
c           ZPT0=TR(3)
c           XPT1=XPT0+RT(1,3)
c           YPT1=YPT0+RT(2,3)
c           ZPT1=ZPT0+RT(3,3)
c           XPT2=XPT0+RT(1,1)
c           YPT2=YPT0+RT(2,1)
c           ZPT2=ZPT0+RT(3,1)
c        ENDIF
c     ENDIF
c     CALL HND_MOLGRP(XPT0,YPT0,ZPT0,XPT1,YPT1,ZPT1,
c    1                XPT2,YPT2,ZPT2,XPT3,YPT3,ZPT3)
C
      ODONE=.TRUE.
C
      RETURN
 9999 FORMAT(' ',a3,' symmetry detected')
 9998 FORMAT(A8)
 9997 FORMAT(1X,A3,2X,I5)
 9996 FORMAT(3F12.8)
 9995 FORMAT(
     1       ' Molecular frame translation',3F10.5,/,
     2       ' Molecular frame rotation   ',3F10.5,/,
     3       '                            ',3F10.5,/,
     4       '                            ',3F10.5)
      END
      SUBROUTINE HND_MOLAXS(A,VEC,EIG,NVEC,N,NDIM)
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
C
C     ----- ROUTINE TO SUBSTITUTE DIAGIV FOR DIAGONALIZATION -----
C           OF SYMMETRIC 3X3 MATRIX A  IN TRIANGULAR FORM
C
      CHARACTER*8 ERRMSG
      COMMON/HND_IOFILE/IR,IW
      DIMENSION A(3,3),VEC(3,3),EIG(3)
      DIMENSION AA(6),IA(3)
      DIMENSION ERRMSG(3)
      DATA ERRMSG /'PROGRAM ','STOP IN ','-MOLAXS-'/
      DATA ZERO   /0.0D+00/
      DATA ONE    /1.0D+00/
      DATA CONV   /1.0D-10/
      DATA MAXIT  /50/
C
      IF(N.EQ.3.AND.NDIM.EQ.3) GO TO 10
cbjp         WRITE(IW,9999)
         CALL HND_HNDERR(3,ERRMSG)
   10 CONTINUE
C
      DO 30 I=1,3
         DO 20 J=1,3
            VEC(J,I)=ZERO
   20    CONTINUE
         VEC(I,I)=ONE
   30 CONTINUE
      AA(1)=-A(1,1)
      AA(2)=-A(2,1)
      AA(3)=-A(2,2)
      AA(4)=-A(3,1)
      AA(5)=-A(3,2)
      AA(6)=-A(3,3)
      IA(1)=0
      IA(2)=1
      IA(3)=3

      CALL HND_DIAGIV(AA,VEC,EIG,IA,N,N,N)
      EIG(1)=-EIG(1)
      EIG(2)=-EIG(2)
      EIG(3)=-EIG(3)
C
C     ----- CHECK FOR RIGHT HANDEDNESS, CORRECT IF NOT -----
C
      TEST =   VEC(1,3)*( VEC(2,1)*VEC(3,2) - VEC(3,1)*VEC(2,2) )
     1       + VEC(2,3)*( VEC(3,1)*VEC(1,2) - VEC(1,1)*VEC(3,2) )
     2       + VEC(3,3)*( VEC(1,1)*VEC(2,2) - VEC(2,1)*VEC(1,2) )
      IF(TEST.GT.ZERO) RETURN
      IF( ABS(EIG(1)-EIG(2)).GT.CONV) GO TO 60
         T = EIG(1)
         EIG(1) = EIG(2)
         EIG(2) = T
         DO 50 I=1,3
            T = VEC(I,1)
            VEC(I,1) = VEC(I,2)
            VEC(I,2) = T
   50    CONTINUE
         RETURN
   60 IF( ABS(EIG(2)-EIG(3)).GT.CONV) GO TO 80
         T = EIG(2)
         EIG(2) = EIG(3)
         EIG(3) = T
         DO 70 I=1,3
            T = VEC(I,2)
            VEC(I,2) = VEC(I,3)
            VEC(I,3) = T
   70    CONTINUE
         RETURN
   80 DO 90 I=1,3
         VEC(I,3) = - VEC(I,3)
   90 CONTINUE
      RETURN
 9999 FORMAT(/,' -DIAAXS- DIAGONALIZATION ONLY SET UP FOR 3X3 MATRIX')
      END
      SUBROUTINE HND_MOLINR(C,C1,NAT,CI,AI,TR)
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
C
C     ----- CENTER AND MOMENTS OF INERTIA ----- 
C
      PARAMETER (MXATOM=2000)
      PARAMETER (ZERO=0.0D+00)
      LOGICAL OUT
      COMMON/HND_IOFILE/IR,IW
      DIMENSION AM(MXATOM)
      DIMENSION C(3,*),C1(3,*)
      DIMENSION CI(3),AI(3,3)
      DIMENSION TR(3)
C
      OUT=.FALSE.
C
C     -----                GET ATOMIC MASSES                -----
C     ----- CALCULATE CENTRE OF MASS AND MOMENTS OF INERTIA -----
C
      CALL HND_MOLAMS(AM,OUT)
C
      DO J=1,3
         CI(J)=ZERO
         DO I=1,3
            AI(J,I)=ZERO
         ENDDO
      ENDDO
      AMASS=ZERO
      DO IAT=1,NAT
         AMASS=AMASS+AM(IAT)
         DO I=1,3
            CI(I)=CI(I)+AM(IAT)*C(I,IAT)
         ENDDO
      ENDDO
      DO I=1,3
         CI(I)=CI(I)/AMASS
         TR(I)=CI(I)
      ENDDO    
      DO IAT=1,NAT
         DO I=1,3
            C1(I,IAT)=C(I,IAT)-TR(I) 
         ENDDO    
      ENDDO
C
      DO IAT=1,NAT
         WT=AM(IAT)
         X =C1(1,IAT)
         Y =C1(2,IAT)
         Z =C1(3,IAT)
         AI(1,1)=AI(1,1)+WT*(Y*Y+Z*Z)
         AI(2,1)=AI(2,1)-WT* X*Y
         AI(1,2)=AI(2,1)
         AI(3,1)=AI(3,1)-WT* X*Z
         AI(1,3)=AI(3,1)
         AI(2,2)=AI(2,2)+WT*(X*X+Z*Z)
         AI(3,2)=AI(3,2)-WT* Y*Z
         AI(2,3)=AI(3,2)
         AI(3,3)=AI(3,3)+WT*(X*X+Y*Y)
      ENDDO
      IF(OUT) THEN
cbjp         WRITE(IW,9999)
cbjp         WRITE(IW,9998) (CI(I),I=1,3)
cbjp         WRITE(IW,9997)
         CALL HND_PRSQ(AI,3,3,3)
      ENDIF
C
      RETURN
 9999 FORMAT(/,1X,'CENTER OF MASS',/,1X,14(1H-))
 9998 FORMAT(' X = ',F12.8,' Y = ',F12.8,' Z = ',F12.8)
 9997 FORMAT(/,1X,'MOMENTS OF INERTIA',/,1X,18(1H-))
      END
      SUBROUTINE HND_MOLAMS(BMASS,SOME)
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
      PARAMETER   (MXATOM=2000)
      LOGICAL     SOME
      CHARACTER*8 ERRMSG
      COMMON/HND_IOFILE/IR,IW
      COMMON/HND_MOLNUC/NUC(MXATOM)
      COMMON/HND_MOLXYZ/C(3,MXATOM),ZAN(MXATOM),NAT
      DIMENSION   AMASS(MXATOM)
      DIMENSION   BMASS(*)
      DIMENSION   AMS(105)
      DIMENSION   ERRMSG(3)
      DATA ERRMSG /'PROGRAM ','STOP IN ','-MOLAMS-'/
      DATA (AMS(I),I=1,54)  /
     1   1.007825D+00,  4.002600D+00,  7.016000D+00,  9.012180D+00,
     2  11.009310D+00, 12.000000D+00, 14.003070D+00, 15.994910D+00,
     3  18.998400D+00, 19.992440D+00, 22.989800D+00, 23.985040D+00,
     4  26.981530D+00, 27.976930D+00, 30.973760D+00, 31.972070D+00,
     5  34.968850D+00, 39.948000D+00, 38.963710D+00, 39.962590D+00,
     6  44.955920D+00, 47.900000D+00, 50.944000D+00, 51.940500D+00,
     7  54.938100D+00, 55.934900D+00, 58.933200D+00, 57.935300D+00,
     8  62.929800D+00, 63.929100D+00, 68.925700D+00, 73.921900D+00,
     9  74.921600D+00, 79.916500D+00, 78.918300D+00, 83.911500D+00,
     1  84.911700D+00, 87.905600D+00, 89.905400D+00, 89.904300D+00,
     2  92.906000D+00, 97.905500D+00, 97.000000D+00,101.903700D+00,
     3 102.904800D+00,105.903200D+00,106.904100D+00,113.903600D+00,
     4 114.904100D+00,119.902200D+00,120.903800D+00,129.906700D+00,
     5 126.904400D+00,131.904200D+00/
      DATA (AMS(I),I=55,105)  /
     1   132.9054D+00,137.9052D+00,138.9063D+00,139.9054D+00,
     2   140.9076D+00,141.9077D+00,144.9127D+00,151.9197D+00,
     3   152.9212D+00,157.9241D+00,158.9253D+00,163.9292D+00,
     4   164.9303D+00,165.9303D+00,168.9342D+00,173.9389D+00,
     5   174.9408D+00,179.9465D+00,180.9480D+00,183.9509D+00,
     6   186.9557D+00,191.9615D+00,192.9629D+00,194.9648D+00,
     7   196.9665D+00,201.9706D+00,204.9744D+00,207.9766D+00,
     8   208.9804D+00,208.9824D+00,209.9871D+00,222.0176D+00,
     9   223.0197D+00,226.0254D+00,227.0278D+00,232.0381D+00,
     1   231.0359D+00,238.0508D+00,237.0482D+00,244.0642D+00,
     2   243.0614D+00,247.0703D+00,247.0703D+00,251.0796D+00,
     3   252.0829D+00,257.0751D+00,258.0986D+00,259.1009D+00,
     4   260.1053D+00,  0.0000D+00,  0.0000D+00/
      DATA ZERO /0.0D+00/
C
      DO IAT=1,NAT
         NUCZ=NUC(IAT)
         IF(NUCZ.GT.103) THEN
cbjp            WRITE(IW,9996)
            CALL HND_HNDERR(3,ERRMSG)
         ENDIF
         IF(NUCZ.EQ.0) ZNUC=ZERO
         IF(NUCZ.GT.0) ZNUC=AMS(NUCZ)
         AMASS(IAT)=ZNUC
      ENDDO
      DO IAT=1,NAT
         BMASS(IAT)= ABS(AMASS(IAT))
      ENDDO
cbjp      IF(SOME) THEN
cbjp         WRITE(IW,9998)
cbjp         DO IAT=1,NAT
cbjp            WRITE(IW,9997) IAT,BMASS(IAT)
cbjp         ENDDO
cbjp      ENDIF
      RETURN
 9998 FORMAT(/,10X,21(1H-),/,10X,'ATOMIC WEIGHTS (A.U.)',
     1       /,10X,21(1H-),/)
 9997 FORMAT(I5,5X,F15.5)
 9996 FORMAT(' NO ATOMIC MASS TABULATED FOR  -NUCZ.GT.103-  . STOP')
      END
      SUBROUTINE HND_MOLGRP(XPT0,YPT0,ZPT0,XPT1,YPT1,ZPT1,         
     1                      XPT2,YPT2,ZPT2,XPT3,YPT3,ZPT3)
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
      PARAMETER     (MXSYM =120)
      PARAMETER     (MXSYMT=120*9)
      PARAMETER     (MXIODA=255)
      CHARACTER*8   ERRMSG
      LOGICAL       OUT
      LOGICAL       SOME
      LOGICAL       ALIGN
      CHARACTER*8   GRPCHR
      CHARACTER*8   DRCCHR
      CHARACTER*8   CHRGRP
      CHARACTER*8   CHRDIR
      CHARACTER*8   CHRBLK
      COMMON/HND_IOFILE/IR,IW
      COMMON/HND_DAFILE/IDAF,NAV,IODA(MXIODA)
      COMMON/HND_SYMTRY/INVT(MXSYM),NT,NTMAX,NTWD,NOSYM
      COMMON/HND_SYMTRF/XSMAL,YSMAL,ZSMAL,XNEW,YNEW,ZNEW,XP,YP,ZP
      COMMON/HND_FRAME/U1,U2,U3,V1,V2,V3,W1,W2,W3,X0,Y0,Z0
      COMMON/HND_SYMMOL/COMPLX,IGROUP,NAXIS,LINEAR
      COMMON/HND_SYMNAM/GROUP
      COMMON/HND_SYMMAT/T(MXSYMT)
      COMMON/HND_ORIENT/ALIGN
      DIMENSION     GRPCHR(19)
      DIMENSION     DRCCHR(2)
      DIMENSION     GRP(19)
      DIMENSION     DRC(2)
      DIMENSION     ERRMSG(3)
      EQUIVALENCE   (CHRGRP,GROUP )
      EQUIVALENCE   (CHRDIR,DIRECT)
      EQUIVALENCE   (CHRBLK,BLANK)
      EQUIVALENCE   (GRP(1),GRPCHR(1))
      EQUIVALENCE   (DRC(1),DRCCHR(1)) 
      DATA ERRMSG   /'PROGRAM ','STOP IN ','- PTGRP-'/
      DATA GRPCHR   /'C1      ','CS      ','CI      ',
     1               'CN      ','S2N     ','CNH     ',
     2               'CNV     ','DN      ','DNH     ',
     3               'DND     ','CINFV   ','DINFH   ',
     4               'T       ','TH      ','TD      ',
     5               'O       ','OH      ',
     6               'I       ','IH      '/
      DATA CHRBLK   /'        '/
      DATA DRCCHR   /'NORMAL  ','PARALLEL'/
      DATA TOL      /1.0D-10/
      DATA PI2      /6.28318530717958D+00/
      DATA ZERO     /0.0D+00/
      DATA PT5      /0.5D+00/
      DATA ONE      /1.0D+00/
      DATA THREE    /3.0D+00/
C
      OUT =.FALSE.
      SOME=.TRUE.
      SOME=SOME.OR.OUT
C
C     ----- GROUP INFO -----
C
      LINEAR=0
C
      IGROUP=20
      DO I=1,19
         IF(GROUP.EQ.GRP(I)) IGROUP=I
      ENDDO
      IF(NAXIS.LE.0) NAXIS=1
cbjp      IF(OUT) THEN
cbjp         WRITE(IW,9991) GROUP,NAXIS
cbjp         IF(LINEAR.NE.0) THEN
cbjp            WRITE(IW,9978)
cbjp         ENDIF
cbjp      ENDIF
      IF(IGROUP.GT.19) THEN
cbjp         WRITE(IW,9994)
         CALL HND_HNDERR(3,ERRMSG)
      ENDIF
      IF(IGROUP.EQ.18.OR.IGROUP.EQ.19) THEN
cbjp         WRITE(IW,9998)
         CALL HND_HNDERR(3,ERRMSG)
      ENDIF
      IF(IGROUP.LE.3                 ) GO TO 200
      IF(IGROUP.EQ.11.OR.IGROUP.EQ.12) GO TO 200
C
C     ----- DEFINE LOCAL FRAME
C     ----- READ IN PRINCIPAL AXIS   ( 1 CARD )
C     ----- READ IN X-LOCAL AXIS     ( 1 CARD )
C     ----- DEFAULT OPTION _ LOCAL FRAME IDENTICAL TO MASTER FRAME
C
      X0=XPT0
      Y0=YPT0
      Z0=ZPT0
      X1=XPT1
      Y1=YPT1
      Z1=ZPT1
C
      RHO= SQRT((X1-X0)**2+(Y1-Y0)**2+(Z1-Z0)**2)
      IF(RHO.LE.TOL) THEN
         X0=ZERO
         Y0=ZERO
         Z0=ZERO
         X1=ZERO
         Y1=ZERO
         Y2=ZERO
         Z2=ZERO
         Z1=ONE
         X2=ONE
         DIRECT=DRC(2)
         RHO=ONE
      ELSE
         X2=XPT2
         Y2=YPT2
         Z2=ZPT2
         DIRECT=BLANK
      ENDIF
      IF(DIRECT.NE.DRC(1)) DIRECT=DRC(2)
      ALIGN=X0.EQ.ZERO.AND.Y0.EQ.ZERO.AND.Z0.EQ.ZERO.AND.
     1      X1.EQ.ZERO.AND.Y1.EQ.ZERO               .AND.
     2                     Y2.EQ.ZERO.AND.Z2.EQ.ZERO.AND.
     3      DIRECT.EQ.DRC(2)
      W1=(X1-X0)/RHO
      W2=(Y1-Y0)/RHO
      W3=(Z1-Z0)/RHO
      WW=W1*W1+W2*W2+W3*W3
      X02=X2-X0
      Y02=Y2-Y0
      Z02=Z2-Z0
      RHO=(W1*X02+W2*Y02+W3*Z02)/WW
      DUM=RHO*W1
      X0=X0+DUM
      X02=X02-DUM
      DUM=RHO*W2
      Y0=Y0+DUM
      Y02=Y02-DUM
      DUM=RHO*W3
      Z0=Z0+DUM
      Z02=Z02-DUM
      UU=(X02*X02+Y02*Y02+Z02*Z02)
      U= SQRT(UU)
      U1=X02/U
      U2=Y02/U
      U3=Z02/U
      V3=W1*U2-W2*U1
      V2=W3*U1-W1*U3
      V1=W2*U3-W3*U2
      IF(DIRECT.NE.DRC(2)) THEN
         DUM=U1
         U1=V1
         V1=-DUM
         DUM=U2
         U2=V2
         V2=-DUM
         DUM=U3
         U3=V3
         V3=-DUM
      ENDIF
cbjp      IF(SOME) THEN
cbjp         WRITE(IW,9990) X0,Y0,Z0,U1,V1,W1,U2,V2,W2,U3,V3,W3
cbjp      ENDIF
      IF(IGROUP.GE.13) GO TO 200
C
C     ----- ROTATION ABOUT PRINCIPAL AXIS
C
      NN=0
      N=NAXIS
      ALPHA=ZERO
      ALPH=PI2/ DBLE(N)
   10 NN=NN+1
      IF(NN.GT.N) GO TO 20
      COSA=COS(ALPHA)
      SINA=SIN(ALPHA)
      I=9*(NN-1)
      T(I+1)=COSA
      T(I+5)=COSA
      T(I+2)=-SINA
      T(I+4)=SINA
      T(I+3)=ZERO
      T(I+6)=ZERO
      T(I+7)=ZERO
      T(I+8)=ZERO
      T(I+9)=ONE
      ALPHA=ALPHA+ALPH
      GO TO 10
C
C     ----- END OF GROUP 4
C
   20 NT=N
      II=9*NT
      IF(OUT) THEN
cbjp         WRITE(IW,9989)
         N1=1
         N2=NAXIS
         CALL HND_PRTSYM(N1,N2)
      ENDIF
      IF(IGROUP.EQ.4                ) GO TO 1000
      IF(IGROUP.EQ.5                ) GO TO 500
      IF(IGROUP.EQ.7                ) GO TO 115
      IF(IGROUP.NE.6.AND.IGROUP.NE.9) GO TO 55
C
C     ----- SIGMA-H PLANE  EQUATION (Z=0) IN LOCAL FRAME
C
      NN=0
   30 NN=NN+1
      IF(NN.GT.NT) GO TO 50
C
C     ----- GROUP 6 0R 9
C
      I=II+9*(NN-1)
      DO 40 J=1,8
   40 T(I+J)=T(I+J-II)
      T(I+9)=-T(I+9-II)
      GO TO 30
   50 NT=NT+NT
      II=9*NT
      IF(OUT) THEN
cbjp         WRITE(IW,9988)
         N1=N2+1
         N2=N2+NAXIS
         CALL HND_PRTSYM(N1,N2)
      ENDIF
C
C     ----- END OF GROUP 6
C
      IF(IGROUP.EQ.6) GO TO 1000
C
C     ----- ONE CP2 AXIS IS THE X-AXIS OF THE LOCAL FRAME
C     ----- GROUP 8 , 9 ,10
C
   55 CONTINUE
      NN=0
   60 NN=NN+1
      IF(NN.GT.NT) GO TO 70
      I=II+9*(NN-1)
      T(I+1)=T(I+1-II)
      T(I+2)=-T(I+2-II)
      T(I+3)=-T(I+3-II)
      T(I+4)=T(I+4-II)
      T(I+5)=-T(I+5-II)
      T(I+6)=-T(I+6-II)
      T(I+7)=T(I+7-II)
      T(I+8)=-T(I+8-II)
      T(I+9)=-T(I+9-II)
      GO TO 60
   70 NT=NT+NT
      II=9*NT
      IF(OUT) THEN
cbjp         WRITE(IW,9987)
         N1=N2+1
         N2=N2+NAXIS
         CALL HND_PRTSYM(N1,N2)
         IF(IGROUP.EQ.9) THEN
cbjp            WRITE(IW,9981)
            N1=N2+1
            N2=N2+NAXIS
            CALL HND_PRTSYM(N1,N2)
         ENDIF
      ENDIF
C
C     ----- END OF GROUP 8 AND 9
C
      IF(IGROUP.EQ.8.OR.IGROUP.EQ.9) GO TO 1000
C
C     ----- DND GROUP . EQUATION OF PLANE SIGMA-D IS _
C     ----- SIN(ALPH/4)*X-COS(ALPH/4)*Y=0
C     ----- THE X-AXIS IS THE CP2 AXIS.
C
C     ----- GROUP 10
C
      BETA=PT5*ALPH
      COSA=COS(BETA)
      SINA=SIN(BETA)
      NN=0
  100 NN=NN+1
      IF(NN.GT.NT) GO TO 110
      I=II+9*(NN-1)
      T(I+1)=COSA*T(I+1-II) + SINA*T(I+2-II)
      T(I+2)=SINA*T(I+1-II) - COSA*T(I+2-II)
      T(I+3)=     T(I+3-II)
      T(I+4)=COSA*T(I+4-II) + SINA*T(I+5-II)
      T(I+5)=SINA*T(I+4-II) - COSA*T(I+5-II)
      T(I+6)=     T(I+6-II)
      T(I+7)=COSA*T(I+7-II) + SINA*T(I+8-II)
      T(I+8)=SINA*T(I+7-II) - COSA*T(I+8-II)
      T(I+9)=     T(I+9-II)
      GO TO 100
  110 NT=NT+NT
      II=9*NT
      IF(OUT) THEN
cbjp         WRITE(IW,9986)
         N1=N2+1
         N2=N2+NAXIS
         CALL HND_PRTSYM(N1,N2)
cbjp         WRITE(IW,9980)
         N1=N2+1
         N2=N2+NAXIS
         CALL HND_PRTSYM(N1,N2)
      ENDIF
C
C     ----- END OF GROUP 10
C
      GO TO 1000
C
C     ----- GROUP 7
C     ----- SIGMA-V IS THE (X-Z) PLANE OF LOCAL FRAME
C
  115 CONTINUE
      NN=0
  120 NN=NN+1
      IF(NN.GT.NT) GO TO 130
      I=II+9*(NN-1)
      T(I+1)=T(I+1-II)
      T(I+2)=-T(I+2-II)
      T(I+3)=T(I+3-II)
      T(I+4)=T(I+4-II)
      T(I+5)=-T(I+5-II)
      T(I+6)=T(I+6-II)
      T(I+7)=T(I+7-II)
      T(I+8)=-T(I+8-II)
      T(I+9)=T(I+9-II)
      GO TO 120
  130 NT=NT+NT
      II=9*NT
C
C     ----- END OF GROUP 7
C
      IF(OUT) THEN
cbjp         WRITE(IW,9985)
         N1=N2+1
         N2=N2+NAXIS
         CALL HND_PRTSYM(N1,N2)
      ENDIF
      GO TO 1000
C
  200 CONTINUE
      T(1)=ONE
      T(5)=ONE
      T(9)=ONE
      T(2)=ZERO
      T(3)=ZERO
      T(4)=ZERO
      T(6)=ZERO
      T(7)=ZERO
      T(8)=ZERO
      IF(IGROUP.EQ.1                 ) GO TO 210
      IF(IGROUP.EQ.2                 ) GO TO 250
      IF(IGROUP.EQ.3                 ) GO TO 300
      IF(IGROUP.EQ.11.OR.IGROUP.EQ.12) GO TO 400
      GO TO 600
  210 NT=1
      ALIGN=.TRUE.
      X0=ZERO
      Y0=ZERO
      Z0=ZERO
      U1=ONE
      V2=ONE
      W3=ONE
      U2=ZERO
      U3=ZERO
      V1=ZERO
      V3=ZERO
      W1=ZERO
      W2=ZERO
      GO TO 1000
C
C     ----- CS SYMMETRY GROUP
C     ----- THE 3 POINTS 1,2,3 DEFINE SIGMA-H PLANE
C
  250 CONTINUE
C
      X1=XPT1
      Y1=YPT1
      Z1=ZPT1
      X2=XPT2
      Y2=YPT2
      Z2=ZPT2
      RHO=(X2-X1)**2+(Y2-Y1)**2+(Z2-Z1)**2
      IF(RHO.LE.TOL) THEN
C
C     ----- DEFAULT OPTION _ PLANE IS THE (X,Y) PLANE
C
         X1=ZERO
         Y1=ZERO
         Z1=ZERO
         Y2=ZERO
         Z2=ZERO
         X3=ZERO
         Z3=ZERO
         X2=ONE
         Y3=ONE
      ELSE
         X3=XPT3
         Y3=YPT3
         Z3=ZPT3
      ENDIF
      ALIGN=X1.EQ.ZERO.AND.Y1.EQ.ZERO.AND.Z1.EQ.ZERO.AND.
     1                     Y2.EQ.ZERO.AND.Z2.EQ.ZERO.AND.
     2      X3.EQ.ZERO.AND.               Z3.EQ.ZERO
      NT=2
      W1=(Y2-Y1)*(Z3-Z1)-(Y3-Y1)*(Z2-Z1)
      W2=(Z2-Z1)*(X3-X1)-(Z3-Z1)*(X2-X1)
      W3=(X2-X1)*(Y3-Y1)-(X3-X1)*(Y2-Y1)
      RHO= SQRT(W1*W1+W2*W2+W3*W3)
      W1=W1/RHO
      W2=W2/RHO
      W3=W3/RHO
      U1=X2-X1
      U2=Y2-Y1
      U3=Z2-Z1
      RHO= SQRT(U1*U1+U2*U2+U3*U3)
      U1=U1/RHO
      U2=U2/RHO
      U3=U3/RHO
      V1=W2*U3-W3*U2
      V2=W3*U1-W1*U3
      V3=W1*U2-W2*U1
      X0=X1
      Y0=Y1
      Z0=Z1
      T(10)=ONE
      T(14)=ONE
      T(18)=-ONE
      T(11)=ZERO
      T(12)=ZERO
      T(13)=ZERO
      T(15)=ZERO
      T(16)=ZERO
      T(17)=ZERO
cbjp      IF(OUT) THEN
cbjp         WRITE(IW,9983) W1,W2,W3
cbjp         WRITE(IW,9982) U1,V1,W1,U2,V2,W2,U3,V3,W3
cbjp      ENDIF
      GO TO 1000
C
C     ----- CI SYMMETRY GROUP
C     ----- CENTER OF INVERSION IS (X0,Y0,Z0)
C
  300 CONTINUE
      X0=XPT0
      Y0=YPT0
      Z0=ZPT0
C
      ALIGN=X0.EQ.ZERO.AND.Y0.EQ.ZERO.AND.Z0.EQ.ZERO
cbjp      IF(OUT) THEN
cbjp         WRITE(IW,9984) X0,Y0,Z0
cbjp      ENDIF
      T(10)=-ONE
      T(14)=-ONE
      T(18)=-ONE
      T(11)=ZERO
      T(12)=ZERO
      T(13)=ZERO
      T(15)=ZERO
      T(16)=ZERO
      T(17)=ZERO
      NT=2
      U1=ONE
      V2=ONE
      W3=ONE
      U2=ZERO
      U3=ZERO
      V1=ZERO
      V3=ZERO
      W1=ZERO
      W2=ZERO
      GO TO 1000
cbjp  400 WRITE(IW,9995)
  400 continue
      CALL HND_HNDERR(3,ERRMSG)
  500 NN=0
      BETA=PT5*ALPH
      COSB=COS(BETA)
      SINB=SIN(BETA)
  510 NN=NN+1
      IF(NN.GT.NT) GO TO 520
C
C     ----- S2N GROUP
C     ----- THE PLANE OF SYMMETRY FOR THE IMPROPER ROTATION
C     ----- IS THE (X,Y) PLANE OF THE LOCAL FRAME
C
      I=II+9*(NN-1)
      T(I+1)= COSB*T(I+1-II)+SINB*T(I+2-II)
      T(I+2)=-SINB*T(I+1-II)+COSB*T(I+2-II)
      T(I+3)=     -T(I+3-II)
      T(I+4)= COSB*T(I+4-II)+SINB*T(I+5-II)
      T(I+5)=-SINB*T(I+4-II)+COSB*T(I+5-II)
      T(I+6)=     -T(I+6-II)
      T(I+7)= COSB*T(I+7-II)+SINB*T(I+8-II)
      T(I+8)=-SINB*T(I+7-II)+COSB*T(I+8-II)
      T(I+9)=     -T(I+9-II)
      GO TO 510
  520 NT=NT+NT
      II=9*NT
      IF(OUT) THEN
cbjp         WRITE(IW,9979)
         N1=N2+1
         N2=N2+NAXIS
         CALL HND_PRTSYM(N1,N2)
      ENDIF
      GO TO 1000
C
C     ----- T GROUP AND OTHERS CONTAINING A SUBGROUP T _
C     ----- LOCAL X,Y,Z ARE THE C2 AXES
C
  600 DO 610 I=10,36
  610    T(I)=ZERO
      T(10)=ONE
      T(23)=ONE
      T(36)=ONE
      T(14)=-ONE
      T(18)=-ONE
      T(19)=-ONE
      T(27)=-ONE
      T(28)=-ONE
      T(32)=-ONE
      DO 620 II=5,12
         I=9*(II-1)
         J=9*(II-5)
         T(I+1)=T(J+7)
         T(I+2)=T(J+8)
         T(I+3)=T(J+9)
         T(I+4)=T(J+1)
         T(I+5)=T(J+2)
         T(I+6)=T(J+3)
         T(I+7)=T(J+4)
         T(I+8)=T(J+5)
  620    T(I+9)=T(J+6)
      NT=12
      IF(IGROUP.EQ.13) GO TO 1000
      IF(IGROUP.EQ.14) GO TO 650
      IF(IGROUP.EQ.15) GO TO 680
      GO TO 670
C
C     ----- TH GROUP
C     ----- EXPAND GROUP BY TAKING DIRECT PRODUCT WITH CI
C
  650 I=9*NT
      DO 660 J=1,I
  660    T(J+I)=-T(J)
      NT=NT+NT
      GO TO 1000
C
C     ----- OCTAHEDRAL GROUP IS DIRECT PRODUCT OF T AND A C4 ROTATION
C     ----- ABOUT Z AXIS
C
  670 SIGN=-ONE
      GO TO 685
C
C     ----- TD GROUP IS DIRECT PRODUCT OF T AND A REFLECTION IN A
C     ----- PLANE ( EQUATION OF THE PLANE   X=Y  )
C
  680 SIGN=ONE
  685 DO 690 II=13,24
         I=9*(II-1)
         J=9*(II-13)
         T(I+1)=T(J+4)*SIGN
         T(I+2)=T(J+5)*SIGN
         T(I+3)=T(J+6)*SIGN
         T(I+4)=T(J+1)
         T(I+5)=T(J+2)
         T(I+6)=T(J+3)
         T(I+7)=T(J+7)
         T(I+8)=T(J+8)
  690    T(I+9)=T(J+9)
      NT=24
      IF(IGROUP.NE.17) GO TO 1000
C
C     ----- OH GROUP IS DIRECT PRODUCT OF O AND CI
C
      I=9*NT
      DO 700 J=1,I
  700    T(J+I)=-T(J)
      NT=48
 1000 CONTINUE
C
C     ----- FIND THE INVERSE TRANSFORMATIONS
C
      DO 1002 ITR=1,NT
         NN=9*(ITR-1)
         DO 1001 IT=1,NT
            II=9*(IT-1)
            TEST= T(NN+1)*T(II+1)+T(NN+2)*T(II+4)+T(NN+3)*T(II+7)
     1           +T(NN+4)*T(II+2)+T(NN+5)*T(II+5)+T(NN+6)*T(II+8)
     1           +T(NN+7)*T(II+3)+T(NN+8)*T(II+6)+T(NN+9)*T(II+9)
     1           -THREE
            IF( ABS(TEST).GT.TOL) GO TO 1001
            INVT(ITR)=IT
            GO TO 1002
 1001       CONTINUE
 1002    CONTINUE
C
C     ----- GENERATE TRANSFORMATION MATRICES -----
C           FOR  P, D, F, G  BASIS FUNCTIONS
C
c$$$      CALL HND_MATSYM
C
      NTMAX=NT
      IF(NTMAX.EQ.1) THEN
         NOSYM=1
      ELSE
         NOSYM=0
      ENDIF
cbjp      IF(OUT) THEN
cbjp         WRITE(IW,*) 'NT,NTMAX,NOSYM = ',NT,NTMAX,NOSYM
cbjp      ENDIF
C
      RETURN
 9999 FORMAT(A5,2I5)
 9998 FORMAT(' ARE YOU KIDDING... YOU SHOULD GIVE UP...')
 9997 FORMAT(9F10.5)
 9996 FORMAT(3F10.5,A8)
 9995 FORMAT(' LINEAR MOLECULE , POINT GROUP IS CINFV OR DINFH ',/,
     1       ' PLEASE USE GROUP C4V OR D4H...')
 9994 FORMAT(' ILLEGAL POINT GROUP INPUT. STOP..')
 9991 FORMAT(/,' THE POINT GROUP OF THE MOLECULE IS ...',A8,/,
     1         ' THE ORDER OF THE PRINCIPAL AXIS IS ...',I5)
 9990 FORMAT(  ' the origin of the local frome is at',/,
     1         ' x = ',F10.5,' y = ',F10.5,' z = ',F10.5,/,
     2         ' director cosines of the new axes are',/,
     3         3(5X,F10.5),/,3(5X,F10.5),/,3(5X,F10.5))
 9989 FORMAT(' ROTATIONS ABOUT PRINCIPAL AXIS')
 9988 FORMAT(' SIGMA-H FOLLOWED BY ROTATIONS')
 9987 FORMAT(' C2 FOLLOWED BY ROTATIONS ')
 9986 FORMAT(' SIGMA-D FOLLOWED BY ROTATIONS')
 9985 FORMAT(' SIGMA-V FOLLOWED BY ROTATIONS')
 9984 FORMAT(/,10X,' CENTER OF SYMMETRY AT X = ',F10.5,' Y = ',F10.5,
     1             ' Z = ',F10.5)
 9983 FORMAT(/,' PLANE OF SYMMETRY DEFINED BY ITS NORMAL U = ',F10.5,
     1         ' V = ',F10.5,' W = ',F10.5)
 9982 FORMAT(/,10X,3F15.9,/,10X,3F15.9,/,10X,3F15.9)
 9981 FORMAT(' C2 FOLLOWED BY SIGMA-H FOLLOWED BY ROTATIONS')
 9980 FORMAT(' SIGMA-D FOLLOWED BY C2 FOLLOWED BY ROTATIONS')
 9979 FORMAT(' S2N ROTATION FOLLOWED BY ROTATIONS')
 9978 FORMAT(' THE MOLECULE IS LINEAR ')
      END
      SUBROUTINE HND_PRTSYM(N1,N2)
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
      PARAMETER (MXSYM =120)
      PARAMETER (MXSYMT=120*9)
      PARAMETER (MXIODA=255)
      COMMON/HND_IOFILE/IR,IW
      COMMON/HND_DAFILE/IDAF,NAV,IODA(MXIODA)
      COMMON/HND_SYMTRY/INVT(MXSYM),NT,NTMAX,NTWD,NOSYM
      COMMON/HND_SYMMAT/T(MXSYMT)
      DIMENSION NN(MXSYM)
      IMAX=N1-1
  100 IMIN=IMAX+1
      IMAX=IMAX+4
      IF(IMAX.GT.N2) IMAX=N2
      NJ=9*N1-8
      DO 200 J=1,3
      NI=0
      DO 150 I=IMIN,IMAX
      NN(I)=NJ+NI
  150 NI=NI+9
cbjp      WRITE(IW,1000) (T(NN(I)),T(NN(I)+1),T(NN(I)+2),I=IMIN,IMAX)
  200 NJ=NJ+3
cbjp      WRITE(IW,1001)
      IF(IMAX.LT.N2) GO TO 100
 1000 FORMAT(4X,4(3F10.5,2H '))
 1001 FORMAT(/)
      RETURN
      END
c
      subroutine rot_theta_z (theta,coords,ncenter)
c
c     rotate the xy coordinates around the z axis by angle theta
c
      implicit none
c
      integer ncenter, iat
      double precision theta, cosa, sina, temp
      double precision coords(3,ncenter)
c
      cosa = cos(theta)
      sina = sin(theta)
      do iat=1,ncenter
        temp = coords(1,iat)
        coords(1,iat)=cosa*coords(1,iat) - sina*coords(2,iat)
        coords(2,iat)=sina*temp + cosa*coords(2,iat)
      enddo

      end
c
      subroutine check_c2_perp(symc2x,symc2y,c2,nuc,
     1                         nat,threquiv)
c
c     check for a c2 axis perpendicular to the z axis
c
      implicit none
c
      integer nat, i, j, k, iat, jat, iaxis, nuc(nat)
      logical symc2x, symc2y, prpaxs, proper(2)
      logical some, dbug
      double precision theta, c2(3,nat)
      double precision pi, rr, dd, dz
      double precision threquiv
cbjp      COMPLEX*16  QDUMI, QDUMJ
      DOUBLE PRECISION  QDUMIR,QDUMJR,QSAVR,QSAVI
      DOUBLE PRECISION  QDUMII,QDUMJI
c
      some = .false.
      dbug = .false.
c
      PI=4.d+0*ATAN(1.d+0)
      THETA=PI
c
      do iaxis = 1, 2
        PROPER(IAXIS)=.TRUE.
       if (iaxis.eq.1) then
          i = 2
          j = 3
          k = 1
        else
          i = 3
          j = 1
          k = 2
        endif
        DO IAT=1,NAT
           PRPAXS=.FALSE.
           DO JAT=1,NAT
              IF(NUC(JAT).EQ.NUC(IAT).AND..NOT.PRPAXS) THEN
cbjp                 QDUMI=CMPLX(C2(I,IAT),C2(J,IAT))
cbjp                 RR=ABS(QDUMI)
cbjp                 QDUMI=CMPLX(COS(THETA),SIN(THETA))*QDUMI
cbjp                 QDUMJ=CMPLX(C2(I,JAT),C2(J,JAT))
cbjp                 DD=ABS(QDUMI-QDUMJ)
                 QDUMIR=C2(I,IAT)
                 QDUMII=C2(J,IAT)
                 RR=sqrt(QDUMIR**2+QDUMII**2)
                 QSAVR = QDUMIR
                 QSAVI = QDUMII
                 QDUMIR = QSAVR*COS(THETA)-QSAVI*SIN(THETA)
                 QDUMII = QSAVI*COS(THETA)+QSAVR*SIN(THETA)
                 QDUMJR=C2(I,JAT)
                 QDUMJI=C2(J,JAT)
                 DD=sqrt((QDUMIR-QDUMJR)**2+(QDUMII-QDUMJI)**2)
                 DZ=C2(K,IAT)-C2(K,JAT)
                 PRPAXS=(ABS(DD).LT.THREQUIV).AND.
     1                  (ABS(DZ).LT.THREQUIV)
cbjp                 IF(DBUG) THEN
cbjp                    WRITE(6,*) ' IAT,JAT,PRPAXS = ',
cbjp     1                            IAT,JAT,PRPAXS,RR,DD,DZ
cbjp                 ENDIF
              ENDIF
           ENDDO
           PROPER(IAXIS)=PROPER(IAXIS).AND.PRPAXS
cbjp           IF(DBUG) THEN
cbjp              WRITE(6,*) ' IAT,PRPAXS,PROPER(IAXIS) = ',
cbjp     1                      IAT,PRPAXS,PROPER(IAXIS)
cbjp           ENDIF
        ENDDO
cbjp        IF(SOME) THEN
cbjp           WRITE(6,9982) IAXIS,PROPER(IAXIS)
cbjp        ENDIF
      enddo
c
      symc2x = proper(1)
      symc2y = proper(2)
c
 9982 FORMAT('   PROPER PRINCIPAL AXIS ',I1,' = ',L4)
      end
cbjp
      subroutine powerz(a,b,iorder,x,y)
      real*8 a,b,x,y,xt,yt
      integer iorder
c
c  function to evaluate powers of complex number. This
c  is used to avoid function calls to Fortran routines with
c  no counterpart in C.
c
      if (iorder.eq.0) then
        x = 1.0d00
        y = 0.0d00
        return
      else
        x = a
        y = b
        do i = 2, iorder
          xt = x
          yt = y
          x = a*xt-b*yt
          y = b*xt+a*yt
        end do
        return
      endif
      end
