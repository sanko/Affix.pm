requires 'perl', '5.030000';
requires 'File::Spec';
requires 'Config';
requires 'XSLoader';
requires 'Sub::Util';
requires 'Text::ParseWords';
#
on 'test' => sub {
    requires 'Test::More' => 0.98;
};
on 'configure' => sub {
    requires 'Archive::Tar';
    requires 'CPAN::Meta';
    requires 'ExtUtils::Config'  => 0.003;
    requires 'ExtUtils::Helpers' => 0.020;
    requires 'ExtUtils::Install';
    requires 'ExtUtils::InstallPaths' => 0.002;
    requires 'File::Basename';
    requires 'File::Find';
    requires 'File::Path';
    requires 'File::Spec::Functions';
    requires 'Getopt::Long' => 2.36;
    requires 'HTTP::Tiny';
    requires 'IO::Socket::SSL' => 1.42;
    requires 'IO::Uncompress::Unzip';
    requires 'JSON::PP' => 2;
    requires 'Module::Build::Tiny';
    requires 'Module::Load::Conditional';
    requires 'Net::SSLeay' => 1.49;
    requires 'Path::Tiny';
};
