#!/usr/bin/perl
# Multiwave Assembler
#
use strict;
my $instructionsetfile = "rc3200.isf";

my %langtable = ( fileof1 => "Failed to open file: ", fileof2 => "",
	error => "Error", line => "Line",
	opunkn => "No such opcode.",
	addrunkn => "Invalid addressing mode.",
	unmatchlb => "found '[', expected ']' not found",
	unmatchrb => "']' without previous '['",
	duplabel => "Duplicate label on line"
);
my @regtable = ( {reg => '*', nam => 'intern'} );
my @littable = ( );
my %symtable;
my @allfile;
my $vpc = 0;
my $errors = 0;
# build-in instructions
my @optable = (
	{op => '.ORG', arc => 1, arf => '*', encode => 'M'},
	{op => '.EQU', arc => 1, arf => '*', encode => 'M'},
	{op => '.DAT', arc => -1, arf => '*', encode => 'M'}
);

print "Wave Asm - version 0.0.0\n";
LoadInstructions( $instructionsetfile );
print "<optable>\n";
foreach my $o (@optable) {
	print $o->{op} . " ";
}
print "<regtable>\n";
foreach my $o (@regtable) {
	print "$o->{reg} $o->{nam} $o->{encode} \n";
}
foreach(@ARGV) {
	if(/^--/) {
	} elsif (/^-/) {
	} else {
		@allfile = (); # clear file
		%symtable = ();
		Assemble( $_, ($_ . ".lst") );
	}
}

exit;

sub LoadInstructions {
	my($file) = @_;
	unless( open(ISF, "<", $file) ) {
		print STDERR $langtable{fileof1} . $file . $langtable{fileof2} . "\n";
		exit 1;
	}
	my (%att, @attribs, @creg, @mcr);
	while( <ISF> ) {
		s/\n//;
		if(/<HEAD>/ .. /<\/HEAD>/) {
		} elsif(/<REG>/ .. /<\/REG>/) {
			if(/^\W*#/ or /\</) {
			} elsif(/^\+/) {
				print "AttribSet: $_\n";
				s/^\+//;
				%att = ();
				@attribs = split(',', $_);
				for my $a (@attribs) {
					if($a =~ /^N"([^"]*)"/) {
						$att{nam} = $1;
					}
				}
			} else {
				@creg = split(':', $_);
				@mcr = split(',', ($creg[0]));
				foreach(@mcr) {
					print "REG: $_ " . $creg[1]  . "\n";
					push @regtable, ({reg => $_, nam => $att{nam}, encode => $creg[1]});
				}
			}
		} elsif(/<LIT>/ .. /<\/LIT>/) {
			if(/^\W*#/ or /\</) {
				next;
			} elsif(/^\+/) {
				print "AttribSet: $_\n";
				s/^\+//;
				%att = ();
				@attribs = split(',', $_);
				for my $a (@attribs) {
					if($a =~ /^N"([^"]*)"/) {
						$att{nam} = $1;
					} elsif($a =~ /^L([0-9]+)/) {
						$att{len} = $1;
					}
				}
				print "+NAME=$att{nam} +LEN=$att{len}\n";
			} else {
				my ($range, @encode) = split(':', $_);
				my ($rl,$ru) = split(',', $range);
				($rl,$ru) = ('*','*') if($range eq '*');
				for my $i (@encode) {
					$i = "L$att{len}*" if($i eq '*');
				}
				print "LIT: $att{nam} $range $att{len} " . join(':',@encode) . "\n";
				push @littable, ({rl => $rl, ru => $ru, nam => $att{nam}, encode => join(':',@encode)});

			}
		} elsif(/<OPCODE>/ .. /<\/OPCODE>/) {
			if(/^\W*#/ or /</) {
			} elsif(/^\W*$/) {
			} else {
				my @opparam = split(':', $_);
				my $opname = ($opparam[0]);
				$opname =~ s/"//g;
				my $fcode = $opparam[2];
				$fcode =~ s/"([^"]+)"/\1/;
				print "OPCODE: $opname $fcode\n";
				push @optable, {op => $opname, arc => $opparam[1], arf => $fcode, encode => $opparam[3]};
			}
		} elsif(/^[\t ]*#/) {
			print $_;
		}
	}
	close ISF;
}
sub DecodeSymbol {
	my ($sym, $itr) = @_;
	return ("null", undef) if(!defined($sym));
	return ("null", undef) if($sym eq "");
	my $v;
	my $itab = @littable;
	my $ci = 0;
	for my $r (@regtable) {
		if($r->{reg} eq $sym) {
			return ("reg", $r);
		}
	}
	my $r = $symtable{$sym};
	if($r->{val} ne '') {
		if($r->{val} eq '*') {
			for my $l (@littable) {
				if($ci >= $itr) {
					if($l->{rl} eq '*') {
						return ("lit", $l, $itab);
					}
				} else {
					$ci++;
				}
			}
		} else {
			for my $l (@littable) {
				if($ci >= $itr) {
					if($l->{rl} eq '*') {
						return ("lit", $l, $itab);
					} elsif(($l->{rl} <= $v) && ($l->{rh} >= $v)) {
						return ("lit", $l, $itab);
					}
				} else {
					$ci++;
				}
			}
		}
	}
	if($sym =~ /^(-*)([0-9]+)$/) {
		$v = oct($2);
		$v = -$v if(length($1) % 2 == 1);
		for my $r (@littable) {
			if($r->{rl} eq '*') {
				return ("lit", $r, $itab);
			} elsif(($r->{rl} <= $v) && ($r->{rh} >= $v)) {
				return ("lit", $r, $itab);
			}
		}
	}
	return ("null",undef);
}

sub DecodeSymbols {
	my ($allsym, $itr) = @_;
	my $format;
	my @format = ();
	my $maxitr = 0;
	my @decarg = split(",",$allsym);
	my @encode = ();
	for my $arg (@decarg) {
		print STDERR "DECARG: '$arg'\n";
		my @lfs = ();
		my $minus = "";
		my @wsa = split(" ",$arg);
		for my $elem (@wsa) {
			if($elem eq "[") {
				if($arg !~ /\[.*\]/) {
					print STDERR "$langtable{error}: $langtable{line} $.: $langtable{unmatchlb}\n";
				} else {
					push @lfs, "[";
				}
			} elsif($elem eq "]") {
				if($arg !~ /\[.*\]/) {
					print STDERR "$langtable{error}: $langtable{line} $.: $langtable{unmatchrb}\n";
				} else {
					push @lfs, "]";
				}
			} elsif($elem eq "+") {
				push @lfs, "+";
			} elsif($elem eq "-") {
				$minus = "-";
				push @lfs, "+";
			} elsif($elem =~ /^-*[0-9]+$/) {
				my ($type, $vec) = DecodeSymbol($minus . $elem);
				if($type eq "reg") {
					push @lfs, $vec->{nam};
					push @encode, $vec->{encode};
				} elsif($type eq "lit") {
					push @lfs, $vec->{nam};
					push @encode, $vec->{encode};
				} else {
					push @lfs, "#";
					push @encode, "*";
				}
				$minus = "";
			} else {
				my ($type, $vec, $i) = DecodeSymbol($minus . $elem, $itr);
				if($type eq "reg") {
					push @lfs, $vec->{nam};
					push @encode, $vec->{encode};
				} elsif($type eq "lit") {
					push @lfs, $vec->{nam};
					push @encode, $vec->{encode};
					$maxitr = $i;
				}
				$minus = "";
			}
		}
		$format = join('',@lfs);
		push @format, ($format);
	}
	$format = join(',',@format);
	return ($format,$maxitr,@encode);
}

sub Pass2 {
	print "Pass 2\n";
	my $flagpass = 0;
	for my $l (@allfile) {
		my ($label,$opname,$linearg) = ($l->{label},$l->{op},$l->{arg});
		print join("\t",($.,$label,$opname,$linearg)) . "\t" ;
		if($label ne '') {
			if($symtable{$label}->{type} eq "ll") {
				if($symtable{$label}->{val} != $vpc) {
					$symtable{$label}->{val} = $vpc;
					$flagpass++;
				}
			}
		}
		my $enc = undef;
		if($opname ne '') {
			my $found = 0;
			my $addrmode = 0;
			my $itr = 0;
			my $format; my $maxitr;
			my @encode;
			my $encode;
			do {
				$found = 0;
			($format, $maxitr, @encode) = DecodeSymbols($linearg, $itr);
			foreach my $i ( @optable ) {
				if(($i->{op}) eq ($opname)) {
					$enc = $i->{encode};
					$addrmode = 1 if($enc eq 'M');
					$found++;
					if($i->{arf} eq $format) {
						$addrmode = 1;
						last;
					}
				}
			}
			$itr++;
			print "ITR: $itr $found $addrmode\n";
			} while($found != 0 && $itr < $maxitr && $addrmode == 0);
			if($found == 0) {
				print STDERR "$langtable{error}: $langtable{line} $l->{lnum}:",
				" $opname - $langtable{opunkn}\n$l->{ltxt}\n";
				$errors++;
			} else {
				if($addrmode == 0) {
					print STDERR "$langtable{error}: $langtable{line} $l->{lnum}:",
					" $opname - $langtable{addrunkn}\n$l->{ltxt}\n";
					$errors++;
				}
			}
			$encode = join('||',@encode);
			print "$format \"$encode\" $enc\n";
		}
		#print "\n";
	}
}

sub GetLineNum {
	my ($ln) = @_;
	for my $i (@allfile) {
		if($i->{lnum} == $ln) {
			return $i;
		}
	}
	return undef;
}

sub Assemble {
	my ($file, $outfile) = @_;
	unless( open(ISF, "<", $file) ) {
		print STDERR $langtable{fileof1} . $file . $langtable{fileof2} . "\n";
		return;
	}
	unless( open(OSF, ">", $outfile) ) {
		print STDERR $langtable{fileof1} . $outfile . $langtable{fileof2} . "\n";
		close ISF;
		return;
	}
	$errors = 0;
	while(<ISF>) {
		# clean up the lines first
		s/\n//;
		my $prl = $_;
		$prl =~ s/;.*$//;
		#$prl =~ s/^\W*$//;
		if($prl =~ /^[ \t]*$/) { # blank line
			next
		}
		my ($label,$opname,@linearg) = split(/[ \t]+/, $prl);
		my $linearg = join(' ',@linearg);
		$linearg =~ s/\+/ + /g;
		if($label ne '') {
			$label =~ s/:$//;
			my $slnum;
			if($symtable{$label} == undef) {
				$symtable{$label} = { lnum => $., type => "ll", val => "*" };
			} else {
				$slnum = GetLineNum($symtable{$label}->{lnum});
				if($slnum == undef) {
					print STDERR "$langtable{error}: Internal Error!\n";
				} else {
					print STDERR "$langtable{error}: $langtable{line} $.; $slnum->{lnum}: $langtable{duplabel}\n$_\n$slnum->{ltxt}\n";
					$errors++;
				}
			}
		}
		my $enc = undef;
		if($opname ne '') {
			my $found = 0;
			foreach my $i ( @optable ) {
				if(($i->{op}) eq ($opname)) {
					$enc = $i->{encode};
					$found = 1;
					last;
				}
			}
			if($found == 0) {
				print STDERR "$langtable{error}: $langtable{line} $.: $opname - $langtable{opunkn}\n$_\n";
				$errors++;
			}
		}
		push @allfile, {lnum => $., ltxt => $_, label => $label, op => $opname, arg => $linearg};
		print join(':',($.,$label,$opname,$linearg,$enc)) ;
		print "\n";
	}
	$vpc = 0;
	$errors = 0;
	Pass2();
}
