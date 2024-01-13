! http://www.math.utah.edu/software/c-with-fortran.html
! https://en.wikibooks.org/wiki/Fortran/Fortran_procedures_and_functions

function func(i) result(j)
    integer, intent (in), value :: i ! input
    integer                     :: j ! output

    j = i**2 + i**3
end function

recursive function fact(i) result(j)
    integer, intent (in) :: i
    integer :: j
    if (i==1) then
        j = 1
    else
        j = i * fact(i - 1)
    end if
end function fact

    ! fortran with by-value parameter
    function sum_v( x, y ) result(z)
        integer, intent (in), value :: x
        integer, intent (in), value :: y
        integer                     :: z

        z = x + y
    end function

    ! fortran with by-reference parameter
    function sum_r( x, y ) result(z)
        integer, intent (in) :: x
        integer, intent (in) :: y
        integer              :: z

        z = x + y
    end function

    ! fortran subroutine
    subroutine square_cube(i, isquare, icube)
        integer, intent (in)  :: i
        integer, intent (out) :: isquare, icube

        isquare = i**2
        icube   = i**3
    end subroutine

    !four forms for fortran functions
    function f1(i) result (j)
      !! result's variable:  separately specified
      !! result's data type: separately specified
      integer, intent (in) :: i
      integer              :: j
      j = i + 1
    end function

    integer function f2(i) result (j)
      !! result's variable:  separately specified
      !! result's data type: by prefix
      integer, intent (in) :: i
      j = i + 2
    end function

    integer function f3(i)
      !! result's variable:  by function name
      !! result's data type: by prefix
      integer, intent(in) :: i
      f3 = i + 3
    end function

    function f4(i)
      !! result's variable:  by function name
      !! result's data type: separately specified
      integer, intent (in) :: i
      integer              :: f4
      f4 = i + 4
    end function


    real function tester(a)
        real, intent (in), optional :: a
        if (present(a)) then
            tester = a
        else
            tester = 0.0
        end if
    end function




    integer function f_add(x, y)
        implicit none
        integer, intent(in), value :: x
        integer, intent(in), value :: y
        f_add = x + y
    end

    subroutine s_add(res, x, y)
        res = x + y
    end












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

    !write(0,*) '# In Fortran: ', (instr(i),i=1,len)
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





integer function sum_arr(arr, n)
  integer, intent(in), value :: n
  integer, dimension(n), intent(in) :: arr
  sum_arr = sum(arr)
end function sum_arr
