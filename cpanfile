requires 'Capture::Tiny';
requires 'Carp';
requires 'Path::Tiny';
requires 'Storable';
requires 'XSLoader';
requires 'perl', 'v5.40.0';
on configure => sub {
	requires => 'Dist::Build', '0.025';
	requires => 'Dist::Build::XS', '0.025';
    requires 'perl', 'v5.40.0';
};

on test => sub {
    requires 'Capture::Tiny';
    requires 'Data::Dump';
    requires 'Data::Printer';
    requires 'IPC::Cmd';
    requires 'TAP::Harness::Env';
    requires 'Test2::Plugin::UTF8';
    requires 'Test2::Tools::Compare';
    requires 'Test2::V0';
    recommends 'Benchmark';
    recommends 'FFI::Platypus', '2';
    recommends 'Inline::C';
};
