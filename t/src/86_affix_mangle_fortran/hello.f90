! http://www.math.utah.edu/software/c-with-fortran.html

integer(kind=c_int) function getString(instr) bind(C,name='fstringlen')
    use, intrinsic :: iso_c_binding
    character(kind=c_char), dimension(*), intent(IN) :: instr
    integer :: len
    integer :: i

    len=0
    do
        if (instr(len+1) == C_NULL_CHAR) exit
        len = len + 1
    end do

    write(0,*) '# In Fortran: ', (instr(i),i=1,len)
    getstring = len
end function getString

module math
  implicit none
contains
    function add( x, y ) result( r ) !bind(C,name="add")  ! can use a different name
        use iso_c_binding, only: c_int
        integer(c_int), value :: x
        integer(c_int), value :: y
        integer(c_int) :: r

        r = x +y
    end function
end module math
