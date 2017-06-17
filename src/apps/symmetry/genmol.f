c
c    This program generates a full molecule given an irreducible
c    fragment of the molecule. If additional atoms beyond the
c    irreducible fragment are included, then genmol will generate
c    the full molecule with no error.
c
      implicit real*8 (a-h,o-z)
      parameter(maxops=192,max_cent=2000)
      logical oprint
      dimension cdist(3),cang(3)
      double precision charge_new(max_cent)
      double precision sym_ops(maxops*3,4)
      double precision coords(3,max_cent)
      double precision charge(max_cent)
      double precision coords_new(3,max_cent)
      double precision mass(max_cent),mass_new(max_cent)
      integer iasym_order(max_cent)
      character*16 tags(max_cent),tags_new(max_cent)
      character*10 sym_molgnames(46)
      character*10 group_name
      character*1 upper
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
        read(5,200) tags(i)
  200   format(a16)
        read(5,*) charge(i),coords(1,i),
     &            coords(2,i),coords(3,i)
      end do
      do i = 1, 10
        group_name(i:i) = upper(group_name(i:i))
      end do
c
c  parameters used in gensym and dosymops calls. itype = 0 for
c  molecules, itype = 1,2,3 for 1d,2d,3d periodic systems (not
c  currently implemented). For molecular systems, numset should
c  probably be 0 or 1 (probably 1).
c
      itype  = 0
      numset = 1
      numgrp = 0
      do i = 1, 46
       if (group_name.eq.sym_molgnames(i)) numgrp = i
      end do
c      if (numgrp.eq.0) call exit(3)
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
c    masses not need at this point. Just assign 1 to all masses
c
      do i = 1, nata
        mass(i) = 1.0d00
      end do
c
c    generate symmetry operators for whatever group (crystals, surfaces
c    polymers or molecules) have been requested.
c
      if (numgrp == 0) then
        call gensym(-1,-1,numset,sym_ops,nops,oprint,group_name)
      else
        call gensym(itype,numgrp,numset,sym_ops,nops,oprint,group_name)
      endif
c
c    generate molecule from irreducible fragment
c
      call dosymops(sym_ops,nops,coords,tags,nata,
     &     itype,tags_new,coords_new,nareal,nata*(nops+1),
     $     cdist,cang,iasym_order, 
     $     charge,charge_new,mass,mass_new,threquiv,ierr)
c
c   We are enforcing error checking by having the
c   code exit with a non-zero status using the exit()
c   subroutine call. This is a non-standard call and may
c   conflict with the F90 EXIT statement if this code is
c   ever compiled under Fortran 90.
c
      if (ierr.ne.0) call exit(ierr)
c
c   restructure list of generated atoms so that original atoms
c   are at the front of the new list
c
      nsav = nareal
      if (nata.lt.nareal) then
c
c eliminate original atoms from new lists
c
        do i = 1, nata
          x = coords(1,i)
          y = coords(2,i)
          z = coords(3,i)
          iat = 1
  800     xnew = coords_new(1,iat)
          ynew = coords_new(2,iat)
          znew = coords_new(3,iat)
          if (dabs(x-xnew).le.threquiv.and.dabs(y-ynew).le.threquiv
     &       .and.dabs(z-znew).le.threquiv
     &       .and.charge(i).eq.charge_new(iat)) then
            do j = iat, nareal-1
              coords_new(1,j) = coords_new(1,j+1)
              coords_new(2,j) = coords_new(2,j+1)
              coords_new(3,j) = coords_new(3,j+1)
              tags_new(j) = tags_new(j+1)
              charge_new(j) = charge_new(j+1)
            end do
            nareal = nareal - 1
          else
            iat = iat + 1 
            if (iat.le.nareal) goto 800
          endif
        end do
c
c   push generated list of atoms up to create space for original
c   atoms
c
        do i = nareal, 1, -1
          coords_new(1,i+nata) = coords_new(1,i)
          coords_new(2,i+nata) = coords_new(2,i)
          coords_new(3,i+nata) = coords_new(3,i)
          tags_new(i+nata) = tags_new(i)
          charge_new(i+nata) = charge_new(i)
        end do
c
c   add original atoms to front of new list
c
        do i = 1, nata
          coords_new(1,i) = coords(1,i)
          coords_new(2,i) = coords(2,i)
          coords_new(3,i) = coords(3,i)
          tags_new(i) = tags(i)
          charge_new(i) = charge(i)
        end do
        nareal = nareal + nata
      endif
c
      write(6,300) nareal
      write(6,*)
  300 format(i8)
      do i = 1, nareal
        write(6,400) tags_new(i),(coords_new(j,i),j=1,3)
  400   format(a16,3e24.14)
      end do
      stop
      end
