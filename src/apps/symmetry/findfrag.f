      subroutine findfrag(symops,nops,coords,tags,nata,itype,
     &     tags_new,coords_new,nareal,nat,cdist,cang,iasym_order,
     $     charge,charge_new,threquiv)
c
c rak typed everything
c
      implicit none
c
c   subroutines finds an irreducible fragment given the symmetry
c   operators and a list of coordinates in the correct orientation
c
      double precision tol, threquiv
      integer maxops,max_cent
      parameter (maxops=192,max_cent=2000)
c
      integer nata, nat
      integer nops, itype, nareal
      integer iused(max_cent)
      character*16 tags(max_cent),tags_new(max_cent)
      double precision symops(maxops*3,4),coords_new(3,max_cent),
     &    xnew(maxops,3),
     &    coords(3,max_cent),cdist(3),cang(3)
      integer iasym_order(max_cent)
      double precision charge(max_cent), charge_new(max_cent)
c
      integer i, j, k, ll, iopnum, iop, ichk
      double precision sum, dx, dy, dz, x, y, z, r, rchk
c
c     set tolerance
c
      tol = threquiv
c
c symops*coordinates
c
      do i = 1, nata
        iused(i) = 0
      end do
c
c    find atom closest to the origin
c
      ichk = 1
      rchk = sqrt(coords(1,1)**2+coords(2,1)**2+coords(3,1)**2)
      do i = 2, nata
        r = sqrt(coords(1,i)**2+coords(2,i)**2+coords(3,i)**2)
        if (r.lt.rchk) then
          ichk = i
          rchk = r
        endif
      end do
c
c   put atom closest to origin in list of irreducible atoms
c
      iused(ichk) = 1
      nareal = 1
      do ll = 1, 3
        coords_new(ll,nareal)=coords(ll,ichk)
      end do
      tags_new(nareal)=tags(ichk)
      charge_new(nareal)=charge(ichk)
      iasym_order(nareal)=ichk
c
c    generate all symmetry-equivalent atoms and see which atoms
c    in original list can be eliminated
c
  100 iopnum = 0
      do iop=1,nops
        do j=1,3
          iopnum=iopnum+1
          sum=0.0d+00
          do k=1,3
            sum=sum+symops(iopnum,k)*coords(k,ichk)
          end do
          xnew(iop,j)=sum+symops(iopnum,4)
        end do
      end do
c
c  compare symmetry generated atoms with atoms in original list
c  and eliminate atoms in original list that are duplicates
c
      do i= 1, nata
        if (iused(i).eq.0) then
          x = coords(1,i)
          y = coords(2,i)
          z = coords(3,i)
          do ll = 1, nops
            dx = dabs(x - xnew(ll,1))
            dy = dabs(y - xnew(ll,2))
            dz = dabs(z - xnew(ll,3))
            if (dx.lt.tol.and.dy.lt.tol.and.dz.lt.tol
     &         .and.tags(i).eq.tags(ichk)) iused(i)=1
          end do
        endif
      end do
c
c   find atom in original list that is closest to the atoms in the
c   irreducible fragment. If no atoms left in original list, then
c   reduction is done.
c
      ichk = 0
      rchk = -1.0d00
      do ll = 1, nareal
        x = coords_new(1,ll)
        y = coords_new(2,ll)
        z = coords_new(3,ll)
        do i = 1, nata
          if (iused(i).eq.0) then 
            dx=coords(1,i)-x
            dy=coords(2,i)-y
            dz=coords(3,i)-z
            r = sqrt(dx**2+dy**2+dz**2)
            if (rchk.lt.0.0d00) then
              rchk = r
              ichk = i
            else
              if (r.lt.rchk) then
                rchk = r
                ichk = i
              endif
            endif
          endif
        end do
      end do
c
      if (ichk.eq.0) then
        continue
      else
        nareal = nareal + 1
        iused(ichk) = 1
        do ll = 1, 3
          coords_new(ll,nareal)=coords(ll,ichk)
        end do
        tags_new(nareal)=tags(ichk)
        charge_new(nareal)=charge(ichk)
        iasym_order(nareal)=ichk
        goto 100
      endif
c
c    if irreducible fragment is the same size as original
c    molecule, replace reduced list with original list
c
      if (nareal.eq.nata) then
        do i = 1, nata
          do ll = 1, 3
            coords_new(ll,i) = coords(ll,i)
          end do
          tags_new(i) = tags(i)
          charge_new(i) = charge(i)
        end do
      endif
      return
      end
