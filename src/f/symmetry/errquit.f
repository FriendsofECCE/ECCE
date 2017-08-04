      subroutine errquit(string, icode)
      implicit none
      character*(*) string
      integer icode
c     
c     error termination 
c     
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
