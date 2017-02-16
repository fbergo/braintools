#!/usr/bin/perl

my @defs, @makedefs;

my $cflags;
my $libs;
my $lazyc = 0;

my $quiet = 0;

# utility
sub checking;
sub checking_for;
sub result;
sub presult;
sub set_flush;
sub define;
sub makedefine;
sub bdefine;
sub verconv;
sub commit;

# actual tests
sub check_platform;
sub check_sdl;
sub check_nasm;

# output control
sub clang;

sub checking {
    $_ = shift @_;
    print "checking $_..." if $quiet == 0;
}

sub checking_for {
    $_ = shift @_;
    print "checking for $_..." if $quiet == 0;
}

sub result {
    $_ = shift @_;
    print "$_\n" if $quiet == 0;
}

sub presult {
    $_ = shift @_;
    print "$_ " if $quiet == 0;
}

sub define {
    $_ = shift @_;
    push @defs, $_;
}

sub makedefine {
    $_ = shift @_;
    push @makedefs, $_;
}

sub bdefine {
	$_ = shift @_;
    	push @defs, $_;
    	push @makedefs, "$_ = yes";
}	

sub set_flush {
    select(STDOUT);
    $| = 1;
}

sub greet {
    print "--- welcome to the iconfig tool - proudly non-GNU - die m4, die\n" if $quiet == 0;
}

sub verconv {
    $_ = shift @_;
    if (/(\d+)\.(\d+)\.(\d+)/) {
	$_ = $3 + ($2 * 1000) + ($1 * 1000000);
	return $_;
    }
    return 0;
}

sub commit {
    my $i;

    print "writing iconfig.minc (Makefile include)..." if $quiet==0;
    open(MKINC,">iconfig.minc") or die("cannot write iconfig.minc");

    print MKINC "### Makefile include generated by iconfig\n";
    print MKINC "ICONFIG = true\n";
    if ($lazyc != 0) {
	print MKINC "CC = gcc\n";
    }
    print MKINC "CFLAGS += $cflags\n";
    print MKINC "LIBS   += $libs\n\n";
    for (@makedefs) {
	print MKINC "$_\n";
    }
    print MKINC "\n";
    if ($lazyc != 0) {
	print MKINC ".c.o: \$<\n";
	print MKINC "\t\$(CC) \$(CFLAGS) -c \$< -o \$\@\n\n";
    }
    print MKINC ".iconfig-clean:\n";
    print MKINC "\trm -f iconfig.h iconfig.minc\n\n";
    if ($lazyc != 0) {
	print MKINC ".mid-clean: .iconfig-clean\n";
	print MKINC "\trm -f *.o *~\n\n";
    }
    print MKINC "### end of include\n";

    close(MKINC);
    print "\n" if $quiet == 0;

    print "writing iconfig.h..." if $quiet == 0;
    open(ICH,">iconfig.h") or die("cannot write iconfig.h");

    print ICH "#ifndef ICONFIG_H\n";
    print ICH "#define ICONFIG_H 1\n\n";
    for (@defs) {
	if (/\s/) {
	    print ICH "#define $_\n";
	} else {
	    print ICH "#define $_    1\n";
	}
    }
    print ICH "\n#endif\n";

    close(ICH);
    print "\n" if $quiet == 0;
}

sub check_platform {
    my $os, $arch, $res, $cpuinfo;
    
    checking("operating system");
    $os = `uname -s`;
    chomp($os);

    bdefine("OS_LINUX")     if ($os eq "Linux");
    bdefine("OS_BSD")       if ($os =~ /.+BSD/);
    bdefine("OS_FREEBSD")   if ($os eq "FreeBSD");
    bdefine("OS_OPENBSD")   if ($os eq "OpenBSD");
    bdefine("OS_NETBSD")    if ($os eq "NetBSD");
    bdefine("OS_DARWIN")    if ($os eq "Darwin");
    bdefine("OS_SOLARIS")   if ($os eq "SunOS");

    result($os);

    checking("architecture");
    $arch = `uname -m`;
    chomp($arch);

    $res = "unknown";
    if ($arch =~ /^i[3-9]86/) {
	bdefine("ARCH_I386");
	$res = "Intel, i386";
    }
    if ($arch =~ /^i[5-9]86/) {
	bdefine("ARCH_I586");
	$res = "Intel, Pentium (P5)";
    }
    if ($arch =~ /^i[6-9]86/) {
	bdefine("ARCH_I686");
	$res = "Intel, Pentium Pro (P6)";
    }
    if ($arch =~ /^sun.*/) {
	bdefine("ARCH_SUN");
	$res = "Sun Sparc";
    }
    if ($arch =~ /^sun4.*/) {
	bdefine("ARCH_SUN4");
	$res = "Sun Sparc, sun4";
    }
    if ($arch =~ /^sun5.*/) {
	bdefine("ARCH_SUN5");
	$res = "Sun Sparc, sun5";
    }
    if ($arch =~ /^x86.64*/) {
	bdefine("ARCH_AMD64");
	$res = "AMD64, x86_64";
    }
    result($res);
    
    if ($os eq "Linux") {
	checking_for("additional CPU features");
	result;
	$cpuinfo = `grep ^flags /proc/cpuinfo`;
	chomp($cpuinfo);

	if ($cpuinfo =~ /\sfpu/) {
	    define("CPU_FPU");
	    presult("+FP");
	} else {
	    presult("-FP");
	}

	if ($cpuinfo =~ /\scmov/) {
	    define("CPU_CMOV");
	    presult("+CMOV");
	} else {
	    presult("-CMOV");
	}

	if ($cpuinfo =~ /\smmx/) {
	    define("CPU_MMX");
	    presult("+MMX");
	} else {
	    presult("-MMX");
	}

	if ($cpuinfo =~ /\smmxext/) {
	    define("CPU_MMX2");
	    presult("+MMX2");
	} else {
	    presult("-MMX2");
	}

	if ($cpuinfo =~ /\s3dnow/) {
	    define("CPU_3DNOW");
	    presult("+3DNOW");
	} else {
	    presult("-3DNOW");
	}

	if ($cpuinfo =~ /\s3dnowext/) {
	    define("CPU_3DNOW2");
	    presult("+3DNOW2");
	} else {
	    presult("-3DNOW2");
	}

	if ($cpuinfo =~ /\ssse/) {
	    define("CPU_SSE");
	    presult("+SSE");
	} else {
	    presult("-SSE");
	}

	result;
	# must check sse2's string later
    }
    return 1;
}

sub check_sdl {
    my $minver = shift @_;
    my $cmin;
    my $ver, $cver, $x;

    if ($minver eq '') {
	checking_for("library: SDL");
    } else {
	checking_for("library: SDL >= $minver");
    }
    $ver = `sdl-config --version`;
    chomp($ver);

    if ($ver eq '') {
	result('not found');
	return 0;
    } else {
	if ($minver eq '') {
	    result("yes, version $ver");
	} else {
	    $cver = verconv($ver);
	    $cmin = verconv($minver);
	    if ($cver >= $cmin) {
		result("yes ($ver)");
	    } else {
		result("no, $ver is too old.");
		return 0;
	    }
	}
    }

    define("LIB_SDL");
    $x = `sdl-config --cflags`;
    chomp($x);
    $cflags = $cflags.' '.$x;

    $x = `sdl-config --libs`;
    chomp($x);
    $libs = $libs.' '.$x;

    return 1;
}

sub check_nasm {
    my $x;

    checking_for('NASM assembler');
    $x = `nasm -v 2>/dev/null`;
    if (! $x =~ /NASM\s+version/) {
	$x = `nasm -r 2>/dev/null`;
    }
    if (! $x =~ /NASM\s+version/) {
	result("no");
	return 0;
    } else {
	result("yes");
	define("PROG_NASM");
	makedefine("NASM = nasm");
	makedefine("NASM_FLAGS =");
	return 1;
    }
}

sub clang {
    $lazyc = 1;
    $cflags = '-Wall -O2 '.$cflags;
}

for (@ARGV) {
    if ($_ eq "--quiet") {
	$quiet = 1;
    }
    if ($_ eq "web") {
	define("AUDIENCE 2");
	print "BUILDING FOR AUDIENCE: WEB\n";
    }
    if ($_ eq "unicamp") {
	define("AUDIENCE 1");
	print "BUILDING FOR AUDIENCE: UNICAMP\n";
    }
    if ($_ eq "devel") {
	define("AUDIENCE 0");
	print "BUILDING FOR AUDIENCE: DEVELOPMENT (full)\n";
    }
}

set_flush;
check_platform;
commit;
