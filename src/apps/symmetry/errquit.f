      subroutine errquit(string, icode)
      implicit none
      character*(*) string
      integer icode
c     
c     error termination: callers carry on with unset data if this
c     returns, so stop with a non-zero status the C++ side can report
c
      write(0,*) string
      if (icode.gt.0.and.icode.lt.256) call exit(icode)
      call exit(1)
      end
c
c function upper returns upper case value of character
c
      function upper(c)
c
      character*1 c, upper
      if (ichar(c).ge.ichar('a').and.ichar(c).le.ichar('z')) then
        upper = char(ichar(c) - ichar('a') + ichar('A'))
      else
        upper = c
      endif
      return
      end
