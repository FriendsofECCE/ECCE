      SUBROUTINE HND_HNDERR(LERR,ERRMSG)
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
      CHARACTER*8  ERRMSG
      DIMENSION    ERRMSG(LERR)
c
      iw = 6
C
cbjp      WRITE(IW,9999)
cbjp      WRITE(IW,9998) ERRMSG
cbjp      WRITE(IW,9997)
C
      CALL ERRQUIT(' CALLS IT QUIT FROM HND_HNDERR ',0)
cbjp 9999 FORMAT(/,1X,72(1H-),/,31X,11HJOB STOPPED)
cbjp 9998 FORMAT(/,1X,9A8)
cbjp 9997 FORMAT(/,1X,72(1H-))
      END
      SUBROUTINE HND_PRSQ(V,M,N,NDIM)
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
C
C     ----- PRINT OUT A SQUARE MATRIX -----
C
***      COMMON/HND_LISTNG/LIST
      DIMENSION V(NDIM,1)
      DIMENSION IC(5),C(5)
      DATA VTOL  /1.5D-01/
      DATA ICMAX /5/
c
      IW = 6
C
      LIST=1
      IF(LIST.EQ.0) MAX=10
      IF(LIST.EQ.1) MAX=7
      IF(LIST.EQ.2) MAX=7
C
      IF(LIST.EQ.0.AND.N.GT.0) GO TO 200
C
      IMAX = 0
  100 IMIN = IMAX+1
      IMAX = IMAX+MAX
      IF (IMAX .GT. M) IMAX = M
cbjp      WRITE (IW,9008)
cbjp      IF(LIST.EQ.0) WRITE (IW,9028) (I,I = IMIN,IMAX)
cbjp      IF(LIST.EQ.1) WRITE (IW,9128) (I,I = IMIN,IMAX)
cbjp      IF(LIST.EQ.2) WRITE (IW,9228) (I,I = IMIN,IMAX)
cbjp      WRITE (IW,9008)
cbjp      DO 120 J = 1,N
cbjp      IF(LIST.EQ.0) WRITE (IW,9048) J,(V(J,I),I = IMIN,IMAX)
cbjp      IF(LIST.EQ.1) WRITE (IW,9148) J,(V(J,I),I = IMIN,IMAX)
cbjp      IF(LIST.EQ.2) WRITE (IW,9248) J,(V(J,I),I = IMIN,IMAX)
cbjp  120 CONTINUE
      IF (IMAX .LT. M) GO TO 100
      RETURN
C
  200 CONTINUE
C
      DO 240 J=1,M
cbjp      WRITE(IW,9328) J
C
      II=0
      DO 220 I=1,N
      IF( ABS(V(I,J)).LT.VTOL) GO TO 220
      II=II+1
      IC (II)=I
      C  (II)=V(I,J)
      IF(II.LT.ICMAX) GO TO 220
cbjp      WRITE(IW,9348) (IC(IDUM),C(IDUM),IDUM=1,II)
      II=0
  220 CONTINUE
      IF(II.EQ.0) GO TO 240
cbjp      WRITE(IW,9348) (IC(IDUM),C(IDUM),IDUM=1,II)
  240 CONTINUE
C
      RETURN
cbjp 9008 FORMAT(/)
cbjp 9028 FORMAT(6X,10(4X,I3,4X))
cbjp 9048 FORMAT(I5,1X,10F11.5)
cbjp 9128 FORMAT(6X,7(6X,I3,6X))
cbjp 9148 FORMAT(I5,1X,7F15.10)
cbjp 9228 FORMAT(6X,7(6X,I3,6X))
cbjp 9248 FORMAT(I5,1X,7E15.8)
cbjp 9328 FORMAT(/,' VEC =',I5,/,1X,10(1H-))
 9348 FORMAT(5(I5,F11.5))
      END
      SUBROUTINE HND_PREV(V,E,M,N,NDIM)
      IMPLICIT REAL*8 (A-H,O-Z)
C
C     ----- PRINT OUT E AND V-MATRICES
C
***      COMMON/HND_LISTNG/LIST
      DIMENSION V(NDIM,1),E(1)
      DIMENSION IC(5),C(5)
      DATA VTOL  /1.5D-01/
      DATA ICMAX /5/
C
      IW = 6
      LIST=1
      IF(LIST.EQ.0) MAX=10
      IF(LIST.EQ.1) MAX=7
      IF(LIST.EQ.2) MAX=7
C
      IF(LIST.EQ.0.AND.N.GT.0) GO TO 200
C
      IMAX = 0
  100 IMIN = IMAX+1
      IMAX = IMAX+MAX
      IF (IMAX .GT. M) IMAX = M
cbjp      WRITE (IW,9008)
cbjp      IF(LIST.EQ.0) WRITE (IW,9028) (I,I = IMIN,IMAX)
cbjp      IF(LIST.EQ.1) WRITE (IW,9128) (I,I = IMIN,IMAX)
cbjp      IF(LIST.EQ.2) WRITE (IW,9228) (I,I = IMIN,IMAX)
cbjp      WRITE (IW,9008)
cbjp      IF(LIST.EQ.0) WRITE (IW,9068) (E(I),I = IMIN,IMAX)
cbjp      IF(LIST.EQ.1) WRITE (IW,9168) (E(I),I = IMIN,IMAX)
cbjp      IF(LIST.EQ.2) WRITE (IW,9268) (E(I),I = IMIN,IMAX)
cbjp      WRITE (IW,9008)
cbjp      DO 120 J = 1,N
cbjp      IF(LIST.EQ.0) WRITE (IW,9048) J,(V(J,I),I = IMIN,IMAX)
cbjp      IF(LIST.EQ.1) WRITE (IW,9148) J,(V(J,I),I = IMIN,IMAX)
cbjp      IF(LIST.EQ.2) WRITE (IW,9248) J,(V(J,I),I = IMIN,IMAX)
cbjp  120 CONTINUE
      IF (IMAX .LT. M) GO TO 100
      RETURN
C
  200 CONTINUE
C
      DO 240 J=1,M
cbjp      WRITE(IW,9328) J,E(J)
C
      II=0
      DO 220 I=1,N
      IF( ABS(V(I,J)).LT.VTOL) GO TO 220
      II=II+1
      IC (II)=I
      C  (II)=V(I,J)
      IF(II.LT.ICMAX) GO TO 220
cbjp      WRITE(IW,9348) (IC(IDUM),C(IDUM),IDUM=1,II)
      II=0
  220 CONTINUE
      IF(II.EQ.0) GO TO 240
cbjp      WRITE(IW,9348) (IC(IDUM),C(IDUM),IDUM=1,II)
  240 CONTINUE
C
      RETURN
 9008 FORMAT(/)
 9028 FORMAT(15X,10(4X,I3,4X))
 9048 FORMAT(I5,1X,10F11.5)
 9068 FORMAT(15X,10F11.5)
 9128 FORMAT(15X,7(6X,I3,6X))
 9148 FORMAT(10X,I5,7F15.10)
 9168 FORMAT(15X,7F15.10)
 9228 FORMAT(15X,7(6X,I3,6X))
 9248 FORMAT(I5,1X,7E15.8)
 9268 FORMAT(15X,7E15.8)
 9328 FORMAT(/,' VEC =',I5,' EIG/OCC =',F11.5,/,1X,31(1H-))
 9348 FORMAT(5(I5,F11.5))
      END
      SUBROUTINE HND_DIAGIV(A,VEC,EIG,IA,NVEC,N,NDIM)
      IMPLICIT REAL*8 (A-H,O-Z)
      real*8 w(12),gamma(15),beta(18),betasq(21),p(18),q(18)
      integer iposv(9),ivpos(12),iord(15)
C
C     ----- GENERAL CALLING  ROUTINE FOR -----
C               -GIVEIS- OR -GIVDIA-
C
      DIMENSION A(1),VEC(NDIM,1),EIG(1),IA(1)
c
c     ----- call -----
c
      CALL HND_GIVDIA(A,VEC,EIG,IA,N,NDIM,
     1     w,gamma,beta,betasq,p,q,iposv,ivpos,iord)
c
      RETURN
      END
      SUBROUTINE HND_GIVDIA(A,VEC,EIG,IA,N,NDIM,
     1           w,gamma,beta,betasq,p,q,iposv,ivpos,iord)
      IMPLICIT REAL*8 (A-H,O-Z)
C        -----  A GIVENS HOUSHOLDER MATRIX DIAGONALIZATION  -----
C        -----  ROUTINE SAME AS EIGEN BUT WORKS WITH A      -----
C        -----  LINEAR ARRAY.                               -----
      DIMENSION A(1),VEC(NDIM,1),EIG(1),IA(1)
      DIMENSION W(*),GAMMA(*),BETA(*),BETASQ(*)
      DIMENSION P(*),Q(*),IPOSV(*),IVPOS(*),IORD(*)
      DATA ZERO,PT5,ONE,TWO /0.0D+00,0.5D+00,1.0D+00,2.0D+00/
      DATA RHOSQ /1.0D-22/
C
      IF(N.EQ.0) GO TO 560
      N1=N-1
      N2=N-2
      GAMMA(1)=A(1)
C     ----- convert F90 deprecated arithmetic if to equivalent logic -----
C      IF(N2) 280,270,120
      IF(N2.LT.0) THEN
        GO TO 280
      ELSEIF(N2.EQ.0) THEN
        GO TO 270
      ENDIF
      DO 260 NR=1,N2
      IK=IA(NR+1)+NR
      B=A(IK)
      S=ZERO
      DO 130 I=NR,N2
      IJ=IA(I+2)+NR
  130 S=S+A(IJ)**2
C
C        -----  PREPARE FOR POSSIBLE BYPASS OF TRANSFORMATION ----
C
      A(IK)=ZERO
      IF(S.LE.ZERO) GO TO 250
      S=S+B*B
      SGN=+ONE
      IF(B.GE.ZERO) GO TO 160
      SGN=-ONE
  160 SQRTS= SQRT(S)
      D=SGN/(SQRTS+SQRTS)
      TEMP= SQRT(PT5+B*D)
      W(NR)=TEMP
      A(IK)=TEMP
      D=D/TEMP
      B=-SGN*SQRTS
C
C        -----  -D- IS FACTOR OF PROPORTIONALITY. NOW       -----
C        -----  COMPUTE AND SAVE -W- VECTOR. EXTRA SINGLY   -----
C        -----  SUBSCRIPTED -W- VECTOR FOR SPEED.           -----
C
      DO 170 I=NR,N2
      IJ=IA(I+2)+NR
      TEMP=D*A(IJ)
      W(I+1)=TEMP
  170 A(IJ)=TEMP
C
C        -----  PREMULTIPLY VECTOR -W- BY MATRIX -A- TO     -----
C        -----  OBTAIN -P- VECTOR. SIMULTANEOUSLY ACCUMULATE ----
C        -----  DOT PRODUCT -WP- -- SCALR -K-.              -----
C
      WTAW=ZERO
      DO 220 I=NR,N1
      SUM=ZERO
      II=IA(I+1)
      DO 180 J=NR,I
      IJ=II+J+1
  180 SUM=SUM+A(IJ)*W(J)
      I1=I+1
      IF(N1.LT.I1) GO TO 210
      DO 200 J=I1,N1
      IJ=IA(J+1)+I+1
  200 SUM=SUM+A(IJ)*W(J)
  210 P(I)=SUM
  220 WTAW=WTAW+SUM*W(I)
      DO 230 I=NR,N1
  230 Q(I)=P(I)-WTAW*W(I)
C
C        -----  NOW FORM -PAP- MATRIX, REQUIRED PART        -----
C
      DO 240 J=NR,N1
      QJ=Q(J)
      WJ=W(J)
      JJ=J+1
      DO 240 I=J,N1
      IJ=IA(I+1)+JJ
  240 A(IJ)=A(IJ)-TWO*(W(I)*QJ+WJ*Q(I))
  250 BETA(NR)=B
      BETASQ(NR)=B*B
      IL=IK+1
  260 GAMMA(NR+1)=A(IL)
  270 IJ=IA(N)+N-1
      B=A(IJ)
      BETA(N-1)=B
      BETASQ(N-1)=B*B
      IJ=IJ+1
      GAMMA(N)=A(IJ)
  280 BETASQ(N)=ZERO
C
C        -----  ADJOIN AN IDENTYTY MATRIX TO BE POST-       -----
C        -----  MULTIPLIED BY ROTATIONS                     -----
C
      DO 300 I=1,N
      DO 299 J=1,N
  299 VEC(I,J)=ZERO
  300 VEC(I,I)=ONE
      M=N
      SUM=ZERO
      NPAS=1
      GO TO 400
  310 SUM=SUM+SHIFT
      COSA=ONE
      G=GAMMA(1)-SHIFT
      PP=G
      PPBS=PP*PP+BETASQ(1)
      PPBR= SQRT(PPBS)
      DO 370 J=1,M
      COSAP=COSA
      IF(PPBS.NE.ZERO) GO TO 320
      SINA=ZERO
      SINA2=ZERO
      COSA=ONE
      GO TO 350
  320 SINA=BETA(J)/PPBR
      SINA2=BETASQ(J)/PPBS
      COSA=PP/PPBR
C
C        -----  POSTMULTIPLY IDENTITY BY -P- TRANSPOSE .    -----
C
      NT=J+NPAS
      IF(NT.LT.N) GO TO 330
      NT=N
  330 CONTINUE
      DO 340 I=1,NT
      TEMP=COSA*VEC(I,J)+SINA*VEC(I,J+1)
      VEC(I,J+1)=-SINA*VEC(I,J)+COSA*VEC(I,J+1)
  340 VEC(I,J)=TEMP
  350 DIA=GAMMA(J+1)-SHIFT
      U=SINA2*(G+DIA)
      GAMMA(J)=G+U
      G=DIA-U
      PP=DIA*COSA-SINA*COSAP*BETA(J)
      IF(J.NE.M) GO TO 360
      BETA(J)=SINA*PP
      BETASQ(J)=SINA2*PP*PP
      GO TO 380
  360 PPBS=PP*PP+BETASQ(J+1)
      PPBR= SQRT(PPBS)
      BETA(J)=SINA*PPBR
  370 BETASQ(J)=SINA2*PPBS
  380 GAMMA(M+1)=G
C
C        -----  TEST FOR CONVERGENCE OF LAST DIAGONAL ELEMENT ----
C
      NPAS=NPAS+1
      IF(BETASQ(M).GT.RHOSQ) GO TO 410
  390 EIG(M+1)=GAMMA(M+1)+SUM
  400 BETA(M)=ZERO
      BETASQ(M)=ZERO
      M=M-1
      IF(M.EQ.0) GO TO 430
      IF(BETASQ(M).LE.RHOSQ) GO TO 390
C
C        -----  TAKE ROOT OF CORMER 2 BY 2 NEAREST TO       -----
C        -----  LOWER DIAGONAL IN VALUE AS ESTIMATE OF      -----
C        -----  EIGENVALUE TO USE FOR SHIFT                 -----
C
  410 A2=GAMMA(M+1)
      R2=PT5*A2
      R1=PT5*GAMMA(M)
      R12=R1+R2
      DIF=R1-R2
      TEMP= SQRT(DIF*DIF+BETASQ(M))
      R1=R12+TEMP
      R2=R12-TEMP
      DIF=  ABS(A2-R1)-  ABS(A2-R2)
      IF(DIF.LT.ZERO) GO TO 420
      SHIFT=R2
      GO TO 310
  420 SHIFT=R1
      GO TO 310
  430 EIG(1)=GAMMA(1)+SUM
      DO 440 J=1,N
      IPOSV(J)=J
      IVPOS(J)=J
  440  IORD(J)=J
      M=N
      GO TO 470
  450 DO 460 J=1,M
      IF(EIG(J).LE.EIG(J+1)) GO TO 460
      TEMP=EIG(J)
      EIG(J)=EIG(J+1)
      EIG(J+1)=TEMP
      ITEMP=IORD(J)
      IORD(J)=IORD(J+1)
      IORD(J+1)=ITEMP
  460 CONTINUE
  470 M=M-1
      IF(M.NE.0) GO TO 450
      IF(N1.EQ.0) GO TO 500
      DO 490 L=1,N1
      NV=IORD(L)
      NP=IPOSV(NV)
      IF(NP.EQ.L) GO TO 490
      LV=IVPOS(L)
      IVPOS(NP)=LV
      IPOSV(LV)=NP
      DO 480 I=1,N
      TEMP=VEC(I,L)
      VEC(I,L)=VEC(I,NP)
  480 VEC(I,NP)=TEMP
  490 CONTINUE
C
C        -----  BACK TRANSFORM THE VECTORS OF THE TRIPLE    -----
C        -----  DIAGONAL MATRIX.                            -----
C
  500 DO 550 NRR=1,N
      K=N1
  510 K=K-1
      IF(K.LE.0) GO TO 550
      SUM=ZERO
      DO 520 I=K,N1
      IJ=IA(I+1)+K
  520 SUM=SUM+VEC(I+1,NRR)*A(IJ)
      SUM=SUM+SUM
      DO 530 I=K,N1
      IJ=IA(I+1)+K
  530 VEC(I+1,NRR)=VEC(I+1,NRR)-SUM*A(IJ)
      GO TO 510
  550 CONTINUE
  560 CONTINUE
      RETURN
      END
