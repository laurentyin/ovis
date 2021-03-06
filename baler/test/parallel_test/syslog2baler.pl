#!/usr/bin/env perl
use strict;
use warnings;
use Socket;
use Pod::Usage;
use Getopt::Long;

my @abbr = qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);

my $host = "localhost"; # default
my $port = "55555"; # default
my $help;

GetOptions(
	"host=s" => \$host,
	"port=i" => \$port,
	"help" => \$help
);

pod2usage(1) if $help;

my $addr = inet_aton($host);
my $paddr = sockaddr_in($port, $addr);

my $proto = getprotobyname("tcp");

socket(SOCK, PF_INET, SOCK_STREAM, $proto) || die "socket: $!";
connect(SOCK, $paddr) || die "connect: $!";

my $line = <STDIN>;
chomp $line;
my $prefix;

die "Empty input ..." if (!$line);

if ($line =~ m/^</) {
	$prefix = "";
} elsif ($line =~ m/^\d/) {
	# new rsyslog format
	$prefix = "<1>1 ";
} else {
	# old rsyslog format
	$prefix = "<1>";
}

do {
	chomp $line;
	print SOCK "$prefix$line\n";
} while ($line = <STDIN>);

close(SOCK);

__END__

=head1 NAME

syslog2baler.pl - Pipe syslog (STDIN) to baler daemon for processing.

=head1 SYNOPSIS

syslog2baler.pl [options] < logfile

 Options:
	-host	The hostname of where balerd reside. (default: localhost)
	-port	The port number that balrrd listened to. (default: 55555)
	-help	Show this help message.
