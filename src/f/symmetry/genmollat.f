
c
c    This program generates a full molecule given an irreducible
c    fragment of the molecule. If additional atoms beyond the
c    irreducible fragment are included, then genmol will generate
c    the full molecule with no error.
c
c    It is intended to be used with lattice systems and space groups.
c    The coordinates must be supplied in fractional coords
c
      use spnames

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
      double precision fx, fy, fz, xthresh, ythresh, zthresh
      integer iasym_order(max_cent)
      character*16 tags(max_cent),tags_new(max_cent)
      character*10 sym_molgnames(46)
      character*10 group_name
      character*1 upper
c
c  read in information from standard input.
c
      read(5,*) nata
      if (nata.gt.max_cent) call exit(2)
      read(5,*) threquiv
      read(5,100) group_name
  100 format(a10)
      read(5,*) cdist(1), cdist(2), cdist(3),
     &            cang(1), cang(2), cang(3)
      do i = 1, nata
        read(5,200) tags(i)
  200   format(a16)
        read(5,*) charge(i),coords(1,i),
     &            coords(2,i),coords(3,i)
      end do
c      do i = 1, 10
c        group_name(i:i) = upper(group_name(i:i))
c      end do
c
c  parameters used in gensym and dosymops calls. itype = 0 for
c  molecules, itype = 1,2,3 for 1d,2d,3d periodic systems (not
c  currently implemented). For molecular systems, numset should
c  probably be 0 or 1 (probably 1).
c
      itype  = -1
      numset = 1
      numgrp = -1
      call getindices(itype,numgrp,group_name)
      if (itype == -1 .OR. numgrp == -1) then
         print *,"Could not match group name: ", group_name
         call exit(-1)
      endif
      oprint = .false.
c
c    masses not needed at this point. Just assign 1 to all masses
c
      do i = 1, nata
        mass(i) = 1.0d00
      end do
c
c    generate symmetry operators for whatever group (crystals, surfaces
c    polymers or molecules) have been requested.
c
      call gensym(itype,numgrp,numset,sym_ops,nops,oprint,group_name)
c
c    generate molecule from irreducible fragment
c
c      write(6,'(a,3f12.6)') 'a b c: ',(cdist(i),i=1,3)
c      write(6,'(a,3f12.6)') 'alpha beta gamma: ',(cang(i),i=1,3)
c      do i = 1, nata
c        write(6,'(a,3f12.6)') 'coords: ',(coords(j,i),j=1,3)
c      end do
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
        if (cdist(1).gt.0.0d00) then
          xthresh = threquiv/cdist(1)
        else
          xthresh = threquiv
        endif
        if (cdist(2).gt.0.0d00) then
          ythresh = threquiv/cdist(2)
        else
          ythresh = threquiv
        endif
        if (cdist(3).gt.0.0d00) then
          zthresh = threquiv/cdist(3)
        else
          zthresh = threquiv
        endif
c
        do i = 1, nata
          x = coords(1,i)
          y = coords(2,i)
          z = coords(3,i)
          iat = 1
  800     xnew = coords_new(1,iat)
          ynew = coords_new(2,iat)
          znew = coords_new(3,iat)
          if (dabs(x-xnew).le.xthresh.and.dabs(y-ynew).le.ythresh
     &       .and.dabs(z-znew).le.zthresh
     &       .and.charge(i).eq.charge_new(iat)) then
            do j = iat, nareal-1
              coords_new(1,j) = coords_new(1,j+1)
              coords_new(2,j) = coords_new(2,j+1)
              coords_new(3,j) = coords_new(3,j+1)
              tags_new(j) = tags_new(j+1)
              charge_new(j) = charge_new(j+1)
            end do
            nareal = nareal - 1
            goto 900
          else
            iat = iat + 1 
            if (iat.le.nareal) goto 800
          endif
  900   end do
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
c    scan list for duplicate atoms
c
      if (cdist(1).gt.0.0d00) then
        xthresh = threquiv/cdist(1)
      else
        xthresh = threquiv
      endif
      if (cdist(2).gt.0.0d00) then
        ythresh = threquiv/cdist(2)
      else
        ythresh = threquiv
      endif
      if (cdist(3).gt.0.0d00) then
        zthresh = threquiv/cdist(3)
      else
        zthresh = threquiv
      endif
      iat = 0
      do i = 1, nareal
        x = coords_new(1,i)
        y = coords_new(2,i)
        z = coords_new(3,i)
        ichk = 1
        do j = 1, iat
          xx = coords_new(1,j)
          yy = coords_new(2,j)
          zz = coords_new(3,j)
          fx = x - xx - anint(x-xx)
          fy = y - yy - anint(y-yy)
          fz = z - zz - anint(z-zz)
          if (dabs(fx).le.xthresh.and.dabs(fy).le.ythresh.and.
     &        dabs(fz).le.zthresh.and.
     &        charge_new(i).eq.charge_new(j)) then
            ichk = 0
          endif
        end do
        if (ichk.eq.1) then
          iat = iat + 1
          coords_new(1,iat) = coords_new(1,i)
          coords_new(2,iat) = coords_new(2,i)
          coords_new(3,iat) = coords_new(3,i)
          tags_new(iat) = tags_new(i)
          charge_new(iat) = charge_new(i)
        endif
      end do
      nareal = iat
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
