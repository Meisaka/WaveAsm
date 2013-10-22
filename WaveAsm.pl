#!/usr/bin/perl
# Multiwave Assembler
#
$instructionsetfile = "rc3200.isf";

%langtable = ( fileof1 => "Failed to open file: ", fileof2 => "",
	error => "Error", line => "Line",
	opunkn => "No such opcode.",
	addrunkn => "Invalid addressing mode."
);
@regtable = ( {reg => '*', nam => 'intern'} );
# build-in instructions
@optable = (
	{op => '.ORG', arc => 1, arf => '*', encode => 'M'},
	{op => '.EQU', arc => 1, arf => '*', encode => 'M'},
	{op => '.DAT', arc => -1, arf => '*', encode => 'M'}
);

LoadInstructions( $instructionsetfile );
print "<optable>\n";
foreach $o (@optable) {
	print $o->{op} . "\n";
}
print "<regtable>\n";
foreach $o (@regtable) {
	print "$o->{reg} $o->{nam} $o->{encode} \n";
}
foreach(@ARGV) {
	if(/^--/) {
	} elsif (/^-/) {
	} else {
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
	while( <ISF> ) {
		s/\n//;
		if(/<HEAD>/ .. /<\/HEAD>/) {
		} elsif(/<REG>/ .. /<\/REG>/) {
			if(/^\W*#/ or /\</) {
			my $attnam;
			} elsif(/^\+/) {
				print "AttribSet: $_\n";
				s/^\+//;
				$attnam = undef;
				@attribs = split(',', $_);
				for $a (@attribs) {
					if($a =~ /^N"([^"]*)"/) {
						$attnam = $1;
					}
				}
			} else {
				@creg = split(':', $_);
				@mcr = split(',', ($creg[0]));
				foreach(@mcr) {
					print "REG: $_ " . $creg[1]  . "\n";
					push @regtable, ({reg => $_, nam => $attnam, encode => $creg[1]});
				}
			}
		} elsif(/<LIT>/ .. /<\/LIT>/) {
		} elsif(/<OPCODE>/ .. /<\/OPCODE>/) {
			if(/^\W*#/ or /</) {
			} elsif(/^\W*$/) {
			} else {
				@opparam = split(':', $_);
				$opname = ($opparam[0]);
				$opname =~ s/"//g;
				$fcode = $opparam[2];
				$fcode =~ s/"([^"]+)"/\1/;
				print "OPCODE: $opname $fcode\n";
				$opset = {op => $opname, arc => $opparam[1], arf => $fcode, encode => $opparam[3]};
				push @optable, ($opset);
			}
		} elsif(/^[\t ]*#/) {
			print $_;
		}
	}
	close ISF;
}
sub DecodeSymbol {
	my ($sym) = @_;
	return ("null", undef) if(!defined($sym));
	return ("null", undef) if($sym eq "");
	my $found, $res;
	for $r (@regtable) {
		if($r->{reg} eq $sym) {
			return ("reg", $r);
		}
	}
	return ("null",undef);
}

sub DecodeSymbols {
	my ($allsym) = @_;
	my $format, @format = ();
	@decarg = split(",",$allsym);
	for $arg (@decarg) {
		print STDERR "DECARG: '$arg'\n";
		my @lfs = ();
		my $minus = "";
		@wsa = split(" ",$arg);
		for $elem (@wsa) {
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
				($type, $vec) = DecodeSymbol($minus . $elem);
				if($type eq "reg") {
					push @lfs, $vec->{nam};
				} else {
					push @lfs, "#";
				}
				$minus = "";
			} else {
				($type, $vec) = DecodeSymbol($minus . $elem);
				if($type eq "reg") {
					push @lfs, $vec->{nam};
				print "REG=$vec->{reg},";
				}
				print "DECSYM=$type\n";
				$minus = "";
			}
		}
		$format = join('',@lfs);
		push @format, ($format);
	}
	$format = join(',',@format);
	print "FORMAT: $format\n";
	return ($format,@decarg);
}

sub Assemble {
	my ($file, $outfile) = @_;
	my $enc, $opname, $label;
	unless( open(ISF, "<", $file) ) {
		print STDERR $langtable{fileof1} . $file . $langtable{fileof2} . "\n";
		return;
	}
	unless( open(OSF, ">", $outfile) ) {
		print STDERR $langtable{fileof1} . $outfile . $langtable{fileof2} . "\n";
		close ISF;
		return;
	}
	while(<ISF>) {
		# clean up the lines first
		$prl = $_;
		$prl =~ s/\n//;
		$prl =~ s/;.*$//;
		#$prl =~ s/^\W*$//;
		if($prl =~ /^[ \t]*$/) { # blank line
			next
		}
		($label,$opname,@line) = split(/[ \t]+/, $prl);
		$line = join(' ',@line);
		$line =~ s/\+/ + /g;
		if($label ne '') {
			$label =~ s/:$//;
		}
		$enc = undef;
		if($opname ne '') {
			my $found = 0;
			my $addrmode = 0;
			@decsym = DecodeSymbols($line);
			print "\{$decsym[0]\}";
			foreach $i ( @optable ) {
				if(($i->{op}) eq ($opname)) {
					$enc = $i->{encode};
					$found = 1;
					if($i->{arf} eq $decsym[0]) {
						$addrmode = 1;
						last;
					}
				}
			}
			if($found == 0) {
				print STDERR "$langtable{error}: $langtable{line} $.: $opname - $langtable{opunkn}\n$_\n";
			} else {
				print STDERR "$langtable{error}: $langtable{line} $.: $opname - $langtable{addrunkn}\n$_\n" if($addrmode == 0);
			}
		}
		print join(':',($.,$label,$opname,$line,$enc)) ;
		print "\n";
	}
}
