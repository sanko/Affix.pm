package Affix::ABI::Fortran 1.0 {
    use strict;
    use warnings;
    use Affix qw[:types :flags];

    sub mangle ($;$$$) {
        my $affix = shift if ref $_[0];
        my ( $name, $args, $ret ) = @_;
        my @scope = split /::/, lc $name;
        return $name . '_' unless scalar @scope > 1;
        return sprintf '__%s_MOD_%s', shift @scope, shift @scope if @scope == 2;
    }
}
1;
__END__
=encoding utf-8

=head1 NAME

Affix::ABI::Fortran - Affix Support for Fortran

=head1 DESCRIPTION

Fortran. Computing's true survivor.

If you're interested in using code written in Fortran from Perl, you're in the right place. See the following sections
for a rundown.

=head1 Features

Affix::ABI::Fortran supports the following features right out of the box:

=over

=item intrinsic types as parameters to subroutines (by reference) and functions (by value)

=item L<name mangling|https://www.gnu.org/software/sather/docs-1.2/tutorial/fortran-names.html>

=back

=head2 Types



=head1 Building a Shared Library in Fortran

If you're building with the GNU Fortran compiler, you'd toss something like this into a terminal:

    gfortran -shared -fPIC -o ./project/out/vehicle.so ./project/compiled/truck.f90

Simply change the C<.so> extention for your platform and you're set.

To build a shared lib with Intel's Fortran compiler, you must specify the C<-shared> option on Unix, C<-dynamiclib> on
macOS, and C</libs:dll> on Windows.

    ifort -shared -fpic ./project/compiled/truck.f90 (Linux)
    ifort -dynamiclib   ./project/compiled/truck.f90 (macOS)
    ifort /libs:dll     ./project/compiled/truck.f90 (Win32)

See L<Intel's
docs|https://wcm-stg.intel.com/content/www/us/en/develop/documentation/fortran-compiler-oneapi-dev-guide-and-reference/top/compiler-reference/libraries/creating-shared-libraries.html>
for more.

=head1 Functions

In Fortran, C<function>s must return a single value mut may return an array of values. The following calls a function
to compute the sum of the square and the cube of an integer.

    ! fortran
    function func(i) result(j)
        integer, intent (in), value :: i ! input
        integer                     :: j ! output

        j = i**2 + i**3
    end function

    # perl
    use Affix;
    affix 'fortran_lib', 'func', [Int], Int;
    my $i = 3;
    printf 'sum of the square and cube of %d is %d', $i, func($i);

=head2 Calling recursive functions

Fortran 90 brought with it L<recursive functions and
subroutines|https://sites.esm.psu.edu/~ajm138/fortranexamples.html>. Here's a quick example on their use:

    ! fortran
    recursive function fact(i) result(j)
        integer, intent (in), value :: i
        integer :: j
        if (i==1) then
            j = 1
        else
            j = i * fact(i - 1)
        end if
    end function fact

    # perl
    use Affix;
    affix 'fortran_lib', 'fact', [Int], Int;
    warn '5! = ' . fact(5); # https://en.wikipedia.org/wiki/Factorial

=head2 Passing Arguments by Value vs. by Reference

By default, Fortran expects args to be passed by reference (as pointers) rather than by value. There are several ways
to change this but I'll present the one that involves the least amount of typing: the C<value> attribute. Be sure your
code correctly advises Affix which is correct.

    ! fortran with by-value parameters
    function sum_v( x, y ) result(z)
        integer, intent (in), value :: x
        integer, intent (in), value :: y
        integer                     :: z

        z = x + y
    end function

    # perl with by-value parameters
    affix( 'fortran_lib', 'sum_v', [ Int, Int ], Int );
    warn sum_v( 2, 5 );

Note the default by-reference expectations without C<value>:

    ! fortran with default, by-reference parameters
    function sum_v( x, y ) result(z)
        integer, intent (in) :: x
        integer, intent (in) :: y
        integer              :: z

        z = x + y
    end function

    # perl with by-reference parameters (pointers)
    affix( 'fortran_lib', 'sum_r', [ Pointer [Int], Pointer [Int] ], Int )
    warn sum_r( 2, 5 );

=head2 Different function result definitions

Functions can define the data type of their result in different forms: either as a separate variable or by the function
name.

    ! four forms for Fortran functions
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

    # Perl
    affix( 'fortran_lib', 'f1', [ Pointer [Int] ], Int ); # function f1(i) result (j)
    affix( 'fortran_lib', 'f2', [ Pointer [Int] ], Int ); # integer function f2(i) result (j)
    affix( 'fortran_lib', 'f3', [ Pointer [Int] ], Int ); # integer function f3(i)
    affix( 'fortran_lib', 'f4', [ Pointer [Int] ], Int ); # function f4(i)
    warn 'f1(0) == ' . f1(0);    # 1
    warn 'f2(0) == ' . f2(0);    # 2
    warn 'f3(0) == ' . f3(0);    # 3
    warn 'f4(0) == ' . f4(0);    # 4

Nothing changes on the Perl side. Just showing off how flexible Fortran is which is kinda neat.

=head2 Optional arguments

Arguments can be set C<optional>. The intrinsic function C<present> can be used to check if a specific parameter is
set.

    ! fortran
    real function tester(a)
        real, intent (in), optional :: a
        if (present(a)) then
            tester = a
        else
            tester = 0.0
        end if
    end function

    # perl
    affix 'fortran_lib', 'tester', [Varargs, Pointer[Float]], Float;

=head1 Subroutines

In Fortran, unlike a C<function>, a C<subroutine> B<does not> return a value but can modify many values and expects
them all to be passed by reference (pointers).

    ! fortran
    subroutine square_cube(i, isquare, icube)
        integer, intent (in)  :: i
        integer, intent (out) :: isquare, icube

        isquare = i**2
        icube   = i**3
    end subroutine

    # perl
    use Affix;
    affix 'fortran_add.so', 'square_cube', [ Pointer[Int], Pointer[Int], Pointer[Int] ], Void;
    square_cube( 4, \my $square, \my $cube ); # ask Affix to treat these pointers like lvalues
    printf 'i=%d,i^2=%d,i^3=%d', 4, $square, $cube; # i=4, i^2=16, i^3=64

=head1 See Also

https://gcc.gnu.org/fortran/

=head1 LICENSE

Copyright (C) Sanko Robinson.

This library is free software; you can redistribute it and/or modify it under the terms found in the Artistic License
2. Other copyrights, terms, and conditions may apply to data transmitted through this module.

=head1 AUTHOR

Sanko Robinson E<lt>sanko@cpan.orgE<gt>

=begin stopwords


=end stopwords

=cut
