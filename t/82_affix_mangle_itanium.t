use strict;
use utf8;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use t::lib::nativecall;
use experimental 'signatures';
$|++;
#
my $lib = compile_test_lib('82_affix_mangle_itanium');

#system 'nm ' . $lib;
#
typedef 'MyClass' => Struct [ myNum => Int, myString => Str ];
#
subtest 'setup()' => sub {
    my $myclass = wrap( [ $lib, 'I' ] => 'setup' => [] => MyClass() )->();
    is $myclass->{myNum},    15,          '.myNum == 15';
    is $myclass->{myString}, 'Some text', '.myString eq "Some text"';
};
subtest 'setup(int i)' => sub {
    my $myclass = wrap( [ $lib, 'I' ] => 'setup' => [Int] => MyClass() )->(3);
    is $myclass->{myNum},    3,                     '.myNum == 3';
    is $myclass->{myString}, 'Some different text', '.myString eq "Some different text"';
};
#
done_testing;
