c
c    This program finds the irreducible fragment that can be used
c    to generate a molecule given the symmetry point group.
c
      use spnames

      implicit real*8 (a-h,o-z)
      integer*4 itype
      integer*4 numgrp

      character*(20) group_name

      itype = 3
      numgrp = 2
      call getname(itype,numgrp,group_name)
      print *,"groupname",group_name

      call getindices(itype,numgrp,'P112')
      print *,"indices",itype,numgrp

      stop
      end
