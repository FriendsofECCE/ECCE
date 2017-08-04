      subroutine sym_avg(symops,nops,coords,nata,itype,
     &     coords_new,cdist,cang,threquiv,ierr)
c
c rak typed everything
c
      implicit none
c
c   subroutine takes structure, operates on it with all symmetry operations
c   in symops and takes the average structure (this is equivalent to projecting
c   structure onto the totally symmetric representation).
c
      double precision tol, threquiv
      integer maxops,max_cent
      parameter (maxops=192,max_cent=2000)
c
      integer nata
      integer nops, itype, ierr
      double precision symops(maxops*3,4),coords_new(3,max_cent),
     &    xnew(3,max_cent),rmin,rtmp,
     &    coords(3,max_cent),cdist(3),cang(3)
c
      integer i, j, k, iopnum, iop, isame
      double precision sum, dx, dy, dz, x, y, z
c
c     set tolerance
c
      tol = threquiv
c
c    apply all symmetry operations to list of original coordinates
c
      iopnum = 0
      do i = 1, nata
        do j = 1, 3 
          coords_new(j,i) = 0.0d00
        end do
      end do
c
      do iop=1,nops
        do j=1,3
          iopnum=iopnum+1
          do i = 1, nata
            sum=0.0d+00
            do k=1,3
              sum=sum+symops(iopnum,k)*coords(k,i)
            end do
            xnew(j,i)=sum
          end do
        end do
c
c    compare new set of coordinates to original coordinates to find
c    equivalent atoms
c
        ierr = 0
        do i = 1, nata
          x = coords(1,i)
          y = coords(2,i)
          z = coords(3,i)
          dx = abs(xnew(1,1)-x)
          dy = abs(xnew(2,1)-y)
          dz = abs(xnew(3,1)-z)
          rmin = sqrt(dx**2+dy**2+dz**2)
          isame = 1
          do j = 2, nata
            dx = abs(xnew(1,j)-x)
            dy = abs(xnew(2,j)-y)
            dz = abs(xnew(3,j)-z)
            rtmp = sqrt(dx**2+dy**2+dz**2)
            if (rtmp.lt.rmin) then
              rmin = rtmp
              isame = j
            endif
          end do
          coords_new(1,i) = coords_new(1,i) + xnew(1,isame)
          coords_new(2,i) = coords_new(2,i) + xnew(2,isame)
          coords_new(3,i) = coords_new(3,i) + xnew(3,isame)
  100   end do
      end do
c
c    finish evaluating averges
c
      do i = 1, nata
        do j = 1, 3
          coords_new(j,i) = (coords_new(j,i)+coords(j,i))
     +                    /dble(nops+1)
        end do
      end do
c
      return
      end
