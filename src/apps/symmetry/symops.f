cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
c  Program symops:
c     Given a molecular point group name, write out the matrix
c     representations of every operation in that group.
c
c  Why this exists.  gensym already turns a group name into exactly
c  these matrices -- spgen holds the generators, gensym multiplies them
c  out -- and cleansym, genmol, genmollat and getfrag all use them to
c  build and clean structures.  Nothing ever made them available to the
c  rest of ECCE, which needs them to decide how a set of atomic
c  orbitals transforms.  So this is a wrapper, not a new algorithm:
c  the operations it prints are the same ones the structure tools have
c  always used, and cannot drift from them.
c
c  Input, on standard input:
c     the group name, uppercase, one line
c
c  Output, on standard output:
c     the number of operations, then that many 3x4 matrices, one row
c     per line.  The leading 3x3 is the rotation; the fourth column is
c     the translation, which is zero for every molecular point group
c     and is written anyway so the format matches what gensym holds.
c
c     THE IDENTITY IS WRITTEN FIRST, and is not one of the operations
c     gensym returns -- it hands back the h-1 others, which is what the
c     structure tools need and is not a complete group.  Checked rather
c     than assumed: for all of C1, Cs, C2v, C3v, C4v, C2h, D2h, D3h,
c     D4h, Td and Oh, gensym returns exactly one fewer operation than
c     the group order.  Adding it here means the count printed below
c     equals the group order, so it can be checked against the
c     independently entered character tables -- two unrelated sources
c     for the same number.
c
c  Exit status 2 means the group name was not recognised.
c
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      program symops_main
      implicit none

      integer maxops
      parameter (maxops=192)

      double precision sym_ops(maxops*3,4)
      integer nops, itype, numset, numgrp, i, j, k
      logical oprint
      character*10 group_name
      character*10 sym_molgnames(46)

c     The same 46 names, in the same order, that cleansym uses: the
c     index into this list IS the group number gensym expects.
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

      read(5,100,end=900) group_name
  100 format(a10)

      itype  = 0
      numset = 1
      numgrp = 0
      do i = 1, 46
        if (group_name.eq.sym_molgnames(i)) numgrp = i
      end do
      if (numgrp.eq.0) call exit(2)

      oprint = .false.
      call gensym(itype,numgrp,numset,sym_ops,nops,oprint,group_name)

      write(6,200) nops+1
  200 format(i8)

c     The identity, which gensym does not return.
      write(6,300) 1.0d0, 0.0d0, 0.0d0, 0.0d0
      write(6,300) 0.0d0, 1.0d0, 0.0d0, 0.0d0
      write(6,300) 0.0d0, 0.0d0, 1.0d0, 0.0d0

      do i = 1, nops
        do j = 1, 3
          write(6,300) (sym_ops((i-1)*3+j,k), k=1,4)
        end do
      end do
  300 format(4e24.14)

      call exit(0)

  900 call exit(2)
      end
