module spnames
    implicit none

      character*10 spgnam(230),lyrnam(80),linenam(99),molname(46)
      integer*4 i
!
!--> names of the 3-dimensional space groups
!
      data (spgnam(i),i=1,95) /&
     '        P1','       P-1','        P2','      P2_1','        C2',&
     '        Pm','        Pc','        Cm','        Cc','      P2/m',&
     '    P2_1/m','      C2/m','      P2/c','    P2_1/c','      C2/c',&
     '      P222','    P222_1','  P2_12_12','P2_12_12_1','    C222_1',&
     '      C222','      F222','      I222','I2_12_12_1','      Pmm2',&
     '    Pmc2_1','      Pcc2','      Pma2','    Pca2_1','      Pnc2',&
     '    Pmn2_1','      Pba2','    Pna2_1','      Pnn2','      Cmm2',&
     '    Cmc2_1','      Ccc2','      Amm2','      Abm2','      Ama2',&
     '      Aba2','      Fmm2','      Fdd2','      Imm2','      Iba2',&
     '      Ima2','      Pmmm','      Pnnn','      Pccm','      Pban',&
     '      Pmma','      Pnna','      Pmna','      Pcca','      Pbam',&
     '      Pccn','      Pbcm','      Pnnm','      Pmmn','      Pbcn',&
     '      Pbca','      Pnma','      Cmcm','      Cmca','      Cmmm',&
     '      Cccm','      Cmma','      Ccca','      Fmmm','      Fddd',&
     '      Immm','      Ibam','      Ibca','      Imma','        P4',&
     '      P4_1','      P4_2','      P4_3','        I4','      I4_1',&
     '       P-4','       I-4','      P4/m','    P4_2/m','      P4/n',&
     '    P4_2/n','      I4/m','    I4_1/a','      P422','    P42_12',&
     '    P4_122','  P4_12_12','    P4_222','  P4_22_12','    P4_322'/
      data (spgnam(i),i=96,190) /&
     '  P4_32_12','      I422','    I4_122','      P4mm','      P4bm',&
     '    P4_2cm','    P4_2nm','      P4cc','      P4nc','    P4_2mc',&
     '    P4_2bc','      I4mm','      I4cm','    I4_1md','    I4_1cd',&
     '     P-42m','     P-42c','   P-42_1m','   P-42_1c','     P-4m2',&
     '     P-4c2','     P-4b2','     P-4n2','     I-4m2','     I-4c2',&
     '     I-42m','     I-42d','    P4/mmm','    P4/mcc','    P4/nbm',&
     '    P4/nnc','    P4/mbm','    P4/mnc','    P4/nmm','    P4/ncc',&
     '  P4_2/mmc','  P4_2/mcm','  P4_2/nbc','  P4_2/nnm','  P4_2/mbc',&
     '  P4_2/mnm','  P4_2/nmc','  P4_2/ncm','    I4/mmm','    I4/mcm',&
     '  I4_1/amd','  I4_1/acd','        P3','      P3_1','      P3_2',&
     '        R3','       P-3','       R-3','      P312','      P321',&
     '    P3_112','    P3_121','    P3_212','    P3_221','       R32',&
     '      P3m1','      P31m','      P3c1','      P31c','       R3m',&
     '       R3c','     P-31m','     P-31c','     P-3m1','     P-3c1',&
     '      R-3m','      R-3c','        P6','      P6_1','      P6_5',&
     '      P6_2','      P6_4','      P6_3','       P-6','      P6/m',&
     '    P6_3/m','      P622','    P6_122','    P6_522','    P6_222',&
     '    P6_422','    P6_322','      P6mm','      P6cc','    P6_3cm',&
     '    P6_3mc','     P-6m2','     P-6c2','     P-62m','     P-62c'/
      data (spgnam(i),i=191,230) /&
     '    P6/mmm','    P6/mcc','  P6_3/mcm','  P6_3/mmc','       P23',&
     '       F23','       I23','     P2_13','     I2_13','      Pm-3',&
     '      Pn-3','      Fm-3','      Fd-3','      Im-3','      Pa-3',&
     '      Ia-3','      P432','    P4_232','      F432','    F4_132',&
     '      I432','    P4_332','    P4_132','    I4_132','     P-43m',&
     '     F-43m','     I-43m','     P-43n','     F-43c','     I-43d',&
     '     Pm-3m','     Pn-3n','     Pm-3n','     Pn-3m','     Fm-3m',&
     '     Fm-3c','     Fd-3m','     Fd-3c','     Im-3m','     Ia-3d'/
!
!dgc---> names of the 80 layer groups
!
      data (lyrnam(i),i=1,80) /&
     '        P1','       P-1','      P112','      P11m','      P11a',&
     '    P112/m','    P112/a','      P211','    P2_111','      C211',&
     '      Pm11','      Pb11','      Cm11','    P2/m11','  P2_1/m11',&
     '    C2/m11','    P2/b11','  P2_1/b11','      P222','    P2_122',&
     '  P2_12_12','      C222','      Pmm2','      Pma2','      Pba2',&
     '      Cmm2','      P2mm','    P2_1am','    P2_1ma','      P2mb',&
     '    P2_1mn','      P2aa','    P2_1ab','      P2an','      C2mm',&
     '      C2mb','      Pmmm','      Pmam','      Pmma','      Pmmn',&
     '      Pbam','      Pmaa','      Pman','      Pbma','      Pbaa',&
     '      Pban','      Cmmm','      Cmma','        P4','       P-4',&
     '      P4/m','      P4/n','      P422','    P42_12','      P4mm',&
     '      P4bm','     P-42m','   P-42_1m','     P-4m2','     P-4b2',&
     '    P4/mmm','    P4/nbm','    P4/mbm','    P4/nmm','        P3',&
     '       P-3','      P312','      P321','      P3m1','      P31m',&
     '     P-31m','     P-3m1','        P6','       P-6','      P6/m',&
     '      P622','      P6mm','     P-6m2','     P-62m','    P6/mmm'/
!
!dgc--> names of the 1-dimensional line groups (z direction)
!
      data (linenam(i),i=1,99) /&
     '        P1','       P-1','      P112','    P112_1',&
     '      P121','      P111','      P11m','      P1m1',&
     '      P1c1','      Pm11','      Pc11','    P112/m',&
     '  P112_1/m','    P12/m1','    P12/c1','    P2/m11',&
     '    P2/c11','      P222','    P222_1','      Pmm2',&
     '    Pmc2_1','    Pcm2_1','      Pcc2','      Pm2m',&
     '      Pc2m','      P2mm','      P2cm','      Pmmm',&
     '      Pccm','      Pcmm','      Pmcm','        P4',&
     '      P4_1','      P4_2','      P4_3','       P-4',&
     '      P4/m','    P4_2/m','      P422','    P4_122',&
     '    P4_222','    P4_322','      P4mm','    P4_2cm',&
     '      P4cc','    P4_2mc','     P-42m','     P-42c',&
     '     P-4m2','     P-4c2','    P4/mmm','    P4/mcc',&
     '  P4_2/mmc','  P4_2/mcm','        P3','      P3_1',&
     '      P3_2','       P-3','      P312','    P3_112',&
     '    P3_212','      P321','    P3_121','    P3_221',&
     '      P3m1','      P3c1','      P31m','      P31c',&
     '     P-31m','     P-31c','     P-3m1','     P-3c1',&
     '        P6','      P6_1','      P6_5','      P6_2',&
     '      P6_4','      P6_3','       P-6','      P6/m',&
     '    P6_3/m','      P622','    P6_122','    P6_522',&
     '    P6_222','    P6_422','    P6_322','      P6mm',&
     '      P6cc','    P6_3cm','    P6_3mc','     P-6m2',&
     '     P-6c2','     P-62m','     P-62c','    P6/mmm',&
     '    P6/mcc','  P6_3/mcm','  P6_3/mmc'/
!
!dgc--> names of the molecular point groups
!
      data molname/&
     'C1        ','CS        ','CI        ','C2        ','C3        ',&
     'C4        ','C5        ','C6        ','C7        ','C8        ',&
     'D2        ','D3        ','D4        ','D5        ','D6        ',&
     'C2V       ','C3V       ','C4V       ','C5V       ','C6V       ',&
     'C2H       ','C3H       ','C4H       ','C5H       ','C6H       ',&
     'D2H       ','D3H       ','D4H       ','D5H       ','D6H       ',&
     'D8H       ','D2D       ','D3D       ','D4D       ','D5D       ',&
     'D6D       ',' S4       ',' S6       ',' S8       ','  T       ',&
     ' TH       ',' Td       ','  O       ',' OH       ','  I       ',&
     ' IH       '/

contains

!     Remove whitespace from the group_name
!     Experimentally, also remove _ characters.
!     The latter rule is based on a comparison of a small number of sample
!     car files vs the above name tables.
      subroutine hackname(group_name,hackedname)
      character*(*) group_name
      character*(*) hackedname
      integer i,j

      j=1
      do i=1,len(group_name)
         if (.NOT. (group_name(i:i) == ' ' .OR. group_name(i:i) == '_')) then
            hackedname(j:j) = group_name(i:i)
            j = j + 1
         endif
      enddo

      end subroutine

      subroutine getname(itype,numgrp,group_name)
      implicit real*8 (a-h,o-z)
      integer*4 itype,numgrp
      character*(*) group_name
!
!     determine name of requested group
!
      if(itype.eq.3) then 
         group_name = spgnam(numgrp)
      elseif(itype.eq.2) then
         group_name = lyrnam(numgrp)
      elseif(itype.eq.1) then
         group_name = linenam(numgrp)
      elseif(itype.eq.0) then
         group_name = molname(numgrp)
      endif

      return
      end subroutine

      subroutine getindices(itype,numgrp,group_name)
      integer*4 itype,numgrp
      character*(*) group_name
      character*(11) tmp
!
!     determine the type and index from the name
!     We look in 3d first, then 2d, then 1d, then point group
!

      itype = -1
      numgrp = -1 
!      character*10 spgnam(230),lyrnam(80),linenam(99),molname(46)
       do i=1,230
         tmp = '           '
         call hackname(spgnam(i), tmp)
         if (group_name .EQ. tmp) then
            itype = 3
            numgrp = i
         endif
      enddo

      if (itype == -1) then
         do i=1,80
            tmp = '           '
            call hackname(lyrnam(i), tmp)
            if (group_name .EQ. tmp) then
               itype = 2
               numgrp = i
            endif
         enddo
      endif

      if (itype == -1) then
         do i=1,99
            tmp = '           '
            call hackname(linenam(i), tmp)
            if (group_name .EQ. tmp) then
               itype = 1
               numgrp = i
            endif
         enddo
      endif

      if (itype == -1) then
         do i=1,46
            tmp = '           '
            call hackname(molname(i), tmp)
            if (group_name .EQ. tmp) then
               itype = 0
               numgrp = i
            endif
         enddo
      endif

      return 
      end subroutine

end module spnames
