use strict;
use utf8;
use Test::More 0.98;
BEGIN { chdir '../' if !-d 't'; }
use lib '../lib', '../blib/arch', '../blib/lib', 'blib/arch', 'blib/lib', '../../', '.';
use Affix qw[:all];
use File::Spec;
use t::lib::nativecall;
use experimental 'signatures';
$|++;
#
compile_test_lib('64_affix_wchar_t');
sub check_string : Native('t/src/64_affix_wchar_t') : Signature([WStr]=>Int);
sub get_string : Native('t/src/64_affix_wchar_t') : Signature([]=>WStr);

#use Encode qw(decode encode);
#
subtest 'sv2ptr=>ptr2sv round trip' => sub {
    is check_string('時空'), 0, '[WStr]=>Int';

    #is get_string(),           '時空', '[]=>WStr';
};

#~ #
#~ use Data::Dump;
#~ #
#~ ddx '時空';
#~ warn sprintf '0x%X' x 6, unpack 'W*', '時空';
#~ warn hex '7a';
#~ #
#~ my $packed = pack( "U*", 0x6642 );
#~ print "$packed\n";    # 時
#~ my $unpacked = unpack( "U*", "時" );
#~ ddx $unpacked;
#~ ddx 0x42;
#~ ddx 0x66;
#~ printf "0x%X\n", $unpacked;    # 0x6642
#~ {
#~ my $foo = pack( "WWWW", 65, 66, 67, 68 );
#~ warn $foo;
#~ # foo eq "ABCD"
#~ $foo = pack( "W4", 65, 66, 67, 68 );
#~ warn $foo;
#~ # same thing
#~ $foo = pack( "W4", 0x24b6, 0x24b7, 0x24b8, 0x24b9 );
#~ warn $foo;
#~ # same thing with Unicode circled letters.
#~ $foo = pack( "U4", 0x24b6, 0x24b7, 0x24b8, 0x24b9 );
#~ warn $foo;
#~ ddx unpack 'U*', "時空";
#~ # same thing with Unicode circled letters.  You don't get the
#~ # UTF-8 bytes because the U at the start of the format caused
#~ # a switch to U0-mode, so the UTF-8 bytes get joined into
#~ # characters
#~ $foo = pack( "C0U4", 0x24b6, 0x24b7, 0x24b8, 0x24b9 );
#~ warn $foo;
#~ printf "%04x\n", $_ for unpack 'C0U*', "時空";
#~ # foo eq "\xe2\x92\xb6\xe2\x92\xb7\xe2\x92\xb8\xe2\x92\xb9"
#~ # This is the UTF-8 encoding of the string in the
#~ # previous example
#~ }
#
done_testing;
