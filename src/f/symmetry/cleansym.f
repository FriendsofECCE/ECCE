c
c    This program finds the irreducible fragment that can be used
c    to generate a molecule given the symmetry point group.
c
      implicit real*8 (a-h,o-z)
      parameter(maxops=192,max_cent=2000)
      logical oprint
      dimension cdist(3),cang(3)
      double precision sym_ops(maxops*3,4)
      double precision coords(3,max_cent)
      double precision coords_new(3,max_cent)
      character*10 sym_molgnames(46)
      character*10 group_name
      data sym_molgnames/
     & 'C1','CS','CI','C2','C3',
     & 'C4','C5','C6','C7','C8',
     & 'D2','D3','D4','D5','D6',
     & 'C2V','C3V','C4V','C5V','C6V',
     & 'C2H','C3H','C4H','C5H','C6H',
     & 'D2H','D3H','D4H','D5H','D6H',
     & 'D8H','D2D','D3D','D4D','D5D',
     & 'D6D','S4','S6','S8','T',
     & 'TH','TD','O','OH','I',
     & 'IH'/
c
c  read in information from standard input.
c
      read(5,*) nata
      if (nata.gt.max_cent) call exit(2)
      read(5,*) threquiv
      read(5,100) group_name
  100 format(a10)
      do i = 1, nata
        read(5,*) coords(1,i),coords(2,i),coords(3,i)
      end do
c
c  parameters used in gensym and dosymops calls. itype = 0 for
c  molecules, itype = 1,2,3 for 1d,2d,3d periodic systems (not
c  currently implemented). For molecular systems, numset should
c  probably be 0 or 1 (probably 1).
c
      itype  = 0
      numset = 1
      do i = 1, 46
       if (group_name.eq.sym_molgnames(i)) numgrp = i
      end do
      oprint = .false.
c
c   lattice vectors & angles (cell constants)
c   values should have been read in and initialized properly for
c   each system dimension (not currently implemented)
c
      do 1000 i=1,3
        cdist(i)= 0.0d00
        cang(i) = 0.0d00
 1000 continue
c
c    generate symmetry operators for whatever group (crystals, surfaces
c    polymers or molecules) have been requested.
c
      call gensym(itype,numgrp,numset,sym_ops,nops,oprint,group_name)
c
c    find irreducible fragment
c
      call sym_avg(sym_ops,nops,coords,nata,
     &     itype,coords_new,cdist,cang,threquiv,ierr)
c
c
      write(6,300) nata
  300 format(i8)
      do i = 1, nata
        write(6,400) (coords_new(j,i),j=1,3)
  400   format(3e24.14)
      end do
c
c   We are enforcing error checking by having the
c   code exit with a non-zero status using the exit()
c   subroutine call. This is a non-standard call and may
c   conflict with the F90 EXIT statement if this code is
c   ever compiled under Fortran 90.
c
      if (ierr.ne.0) call exit(ierr)
      stop
      end
