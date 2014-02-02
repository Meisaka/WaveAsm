#!/usr/bin/perl
# Multiwave Assembler
#
use strict;
my $instructionsetfile = "tr3200.isf";

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
my @incltable = ( );
my @keywtable = ( );
my %symtable;
my @allfile;
my $vpc = 0;
my $vinstrend = 0;
my $errors = 0;
my $flagpass = 0;
my $verbose = 2;
my $binformat = 'f';
my @asmqueue = ( );
# CPU specs (these are replaced in loaded file)
my %cputable = (ISN => "NONE", FILEW => 8, CPUW => 8, CPUM => 8, CPUE => "LE", FILEE => "LE", ALIGN => 0);

my @optable = (
);
# build-in macros
my @macrotable = (
	{op => '.ORG', arc => 1, arf => '*', encode => 'M'},
	{op => '.EQU', arc => 1, arf => '*', encode => 'M'},
	{op => '.DAT', arc => -1, arf => '*', encode => 'M'},
	{op => '.DB', arc => -1, arf => '*', encode => 'M'},
	{op => '.DW', arc => -1, arf => '*', encode => 'M'},
	{op => '.DD', arc => -1, arf => '*', encode => 'M'}
);

print STDERR "Wave Asm - version 0.3.0\n";
foreach(@ARGV) {
	if(/^--(.*)/) {
		my $flags = $1;
		if($flags =~ /^A=(.*)$/) {
			print STDERR "Use ISF $1\n";
			$instructionsetfile = $1;
			if(not ($instructionsetfile =~ /\.isf$/)) {
				$instructionsetfile .= '.isf';
			}
		}
	} elsif (/^-(.*)/) {
		my $flags = $1;
		if($flags =~ /([Vv]*)/) {
			$verbose += length($1);
		} elsif($flags eq 'ff') {
			$binformat = 'f';
		} elsif($flags eq 'fs') {
			$binformat = 's';
		} elsif($flags eq 'fE') {
			$binformat = 'E';
		}
	} else {
		push @asmqueue, $_;
	}
}

# load ISF
LoadInstructions( $instructionsetfile );
print STDERR "<optable>\n" if($verbose > 2);
foreach my $o (@optable) {
	print STDERR $o->{op} . " " if($verbose > 2);
}
print STDERR "\n<regtable>\n" if($verbose > 2);
foreach my $o (@regtable) {
	print STDERR "$o->{reg} $o->{nam} $o->{encode} " if($verbose > 2);
}

# assemble files
foreach(@asmqueue) {
	@allfile = (); # clear file
	%symtable = ();
	Assemble( $_, ($_ . ".lst") , ($_ . ".bin"));
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
			if(/^\W*#/ or /\</) {
			} elsif(/:/) {
				my ($k, $v) = split(':', $_);
				$cputable{$k} = $v;
			}
		} elsif(/<KEYWORD>/ .. /<\/KEYWORD>/) {
			if(/^\W*#/ or /\</) {
			} elsif(/^\+/) {
				print STDERR "AttribSet: $_\n" if($verbose > 4);
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
				if($creg[1] == undef) {
					push @keywtable, ({keyw => $creg[0], nam => $att{nam}, encode => ''});
				} else {
					push @keywtable, ({keyw => $creg[0], nam => $att{nam}, encode => $creg[1]});
				}
				print STDERR "KEYWORD: $creg[0] $creg[1] \n" if($verbose > 4);
			}
		} elsif(/<REG>/ .. /<\/REG>/) {
			if(/^\W*#/ or /\</) {
			} elsif(/^\+/) {
				print STDERR "AttribSet: $_\n" if($verbose > 4);
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
					push @regtable, ({reg => $_, nam => $att{nam}, encode => $creg[1]});
					print STDERR "REG: $_ " . $creg[1]  . "\n" if($verbose > 4);
				}
			}
		} elsif(/<LIT>/ .. /<\/LIT>/) {
			if(/^\W*#/ or /\</) {
				next;
			} elsif(/^\+/) {
				print STDERR "AttribSet: $_\n" if($verbose > 4);
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
				print STDERR "+NAME=$att{nam} +LEN=$att{len}\n" if($verbose > 4);
			} else {
				my ($range, @encode) = split(':', $_);
				my ($rl,$ru) = split(',', $range);
				($rl,$ru) = ('*','*') if($range eq '*');
				for my $i (@encode) {
					$i = "L$att{len}*" if($i eq '*');
				}
				push @littable, ({rl => $rl, ru => $ru, nam => $att{nam}, encode => join(':',@encode)});
				print STDERR "LIT: $att{nam} $range $att{len} " . join(':',@encode) . "\n" if($verbose > 4);

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
				push @optable, {op => $opname, arc => $opparam[1], arf => $fcode, encode => $opparam[3]};
				print STDERR "OPCODE: $opname $fcode\n" if($verbose > 4);
			}
		} elsif(/^[\t ]*#/) {
			print STDERR $_ if($verbose > 5);
		}
	}
	close ISF;
}

sub DecodeValue {
	my ($sym, $tkval, $nvc) = @_;
	my $v;
	my @v;
	if ($tkval == 10) { # Ascii (escaped) literal
		@v = split('',$sym);
		if($v[0] eq '\\') {
			$v = $v[1];
			$v =~ tr/nrt0/\x0A\x0D\x09\0/; #translate lf cr tab nul, others as is.
		} else {
			$v = $v[0];
		}
		$v = ord($v);
		return ("val",$v);
	} elsif($tkval == 11) { # String literal (TODO)
		$v = $sym;
		$v =~ s/\\r/\r/g;
		$v =~ s/\\n/\n/g;
		$v =~ s/\\t/\t/g;
		$v =~ s/\\0/\0/g;
		$v =~ s/\\(.)/\1/g;
		@v = split('',$v);
		print STDERR "[STR:\"$v\"]" if($verbose > 5);
		$v = "";
		foreach my $c (@v) {
			$v .= "+". ord($c);
		}
		return ("str",$v);
	} elsif($tkval == 6) { # Hexadecimal 0x...
		$v = oct("0x$sym");
		$v = -$v if(length($nvc) % 2 == 1);
		return ("val",$v);
	} elsif($tkval == 8) { # Binary 0b...
		$v = oct("0b$sym");
		$v = -$v if(length($nvc) % 2 == 1);
		return ("val",$v);
	} elsif($tkval == 4) { # Hexadecimal ...h
		$v = oct("0x$sym");
		$v = -$v if(length($nvc) % 2 == 1);
		return ("val",$v);
	} elsif($tkval == 7) { # octal 0... just in case ;)
		$v = oct($sym);
		$v = -$v if(length($nvc) % 2 == 1);
		return ("val",$v);
	} elsif($tkval == 5) { # decimal
		$v = ($sym) + 0;
		$v = -$v if(length($nvc) % 2 == 1);
		return ("val",$v);
	}
	print STDERR "[$sym]UNDEF|" if($verbose > 4);
	return ("null",undef);
}

sub DecodeSymbol {
	my ($sym, $minus, $tknum, $itr, $rel) = @_;
	return ("null", undef) if(!defined($sym));
	return ("null", undef) if($sym eq "");
	my $v;
	my $vt;
	my $itab = @littable;
	my $ci = 0;
	my $r = $symtable{$sym};
	if($r->{val} ne '') {
		if($r->{val} eq '*') {
			if($itr == -1) {
				return ("cpc", undef, $itab, $r->{val});
			}
			for my $l (@littable) {
				if($ci >= $itr) {
					if($l->{rl} eq '*') {
						return ("lit", $l, $itab, $r->{val});
					}
				} else {
					$ci++;
				}
			}
		} else {
			$v = $r->{val};
			if($itr == -1) {
				return ("val", undef, $itab, $r->{val});
			}
			$v -= ($vpc + $vinstrend) if($rel == 1);
			#printf STDERR "(%04x)",$v;
			for my $l (@littable) {
				if($ci >= $itr) {
					if($l->{rl} eq '*') {
						return ("lit", $l, $itab, $v);
					} elsif(($l->{rl} <= $v) && ($l->{ru} >= $v)) {
						return ("lit", $l, $itab, $v);
					}
				} else {
					$ci++;
				}
			}
		}
	}

	($vt, $v) = DecodeValue($sym, $tknum, $minus);
	if ($vt eq 'val') {
		if($itr == -1) {
			return ("val", undef, $itab, $v);
		}
		$v -= ($vpc + $vinstrend) if($rel == 1);
		for my $r (@littable) {
			if($ci >= $itr) {
				print STDERR "LL: $v [ $r->{rl} $r->{ru} ] " if($verbose > 5);
				if($r->{rl} eq '*') {
					return ("lit", $r, $itab, $v);
				} elsif(($r->{rl} <= $v) && ($r->{ru} >= $v)) {
					return ("lit", $r, $itab, $v);
				}
			} else {
				$ci++;
			}
		}
	}
	if ($vt eq "str") {
		if($itr == -1) {
			return ("str", undef, $itab, $v);
		}
	}
	return ("null",undef);
}

sub SplitElem {
	my ($allsym) = @_;
	my @chars = split("",$allsym);
	my @results = ();
	my $item;
	my $last = "";
	my $state = 0;
	foreach my $i (@chars) {
		if($state == 0) {
			if($i eq '"') {
				$state = 1;
			} elsif($i eq "'") {
				$state = 2;
			}
			if($i =~ /[ \t]/) { # actual splitting pattern
				push @results, $item if($item ne '');
				$item = "";
			} else {
				$item .= $i;
			}
		} elsif($state == 1) {
			if($last eq '\\') {
				$item .= $i;
				$last = "";
			} else {
				if($i eq '"') {
					$state = 0;
				}
				$item .= $i;
				$last = $i;
			}
		} elsif($state == 2) {
			if($last eq '\\') {
				$item .= $i;
				$last = "";
			} else {
				if($i eq "'") {
					$state = 0;
				}
				$item .= $i;
				$last = $i;
			}
		} else {
			$state = 0;
		}
	}
	push @results, $item if($item ne '');
	return @results;
}

sub TestOp {
	my ($opname) = @_;
	for my $i ( @optable ) {
		if(uc($i->{op}) eq uc($opname)) {
			return (1, $i);
		}
	}
	return (0, undef);
}
sub TestMacro {
	my ($sym) = @_;
	for my $i ( @macrotable ) {
		if(uc($i->{op}) eq uc($sym)) {
			return (1, $i);
		}
	}
	return (0, undef);
}
sub TestReg {
	my ($sym) = @_;
	for my $r (@regtable) {
		if(uc($r->{reg}) eq uc($sym)) {
			return (1, $r);
		}
	}
	return (0, undef);
}
sub TestKeyw {
	my ($sym) = @_;
	for my $k (@keywtable) {
		if(uc($k->{keyw}) eq uc($sym)) {
			return (1, $k);
		}
	}
	return (0, undef);
}

sub TokenizeLine {
	my ($line, $pstype) = @_;
	my @chars = split('',$line);
	my @results = ();
	my $item;
	my $last = 0;
	my $state = 0;
	my $skipc = 0;
	my $lss = 0;
	my @test;
	push @chars, '';
	for my $c (@chars) {
		if($state == 1) {
			if($last == 0 and $c eq '"') {
				$state = 0;
				$skipc = 1;
				# Add token
				push @results, {tkn=>11, v=>$item}; $item = '';
			} elsif($c eq '\\') {
				$last = 1;
				$item .= $c;
			} else {
				$last = 0;
				$item .= $c;
			}
		} elsif($state == 2) {
			if($last == 0 and $c eq '\'') {
				$state = 0;
				$skipc = 1;
				# Add token
				push @results, {tkn=>10, v=>$item}; $item = '';
			} elsif($c eq '\\') {
				$last = 1;
				$item .= $c;
			} else {
				$last = 0;
				$item .= $c;
			}
		} elsif($state == 3) {
			if($c =~ /[_.a-zA-Z0-9]/) {
				$item .= $c;
			} elsif($last == 0 and $c eq ':') {
				$state = 0;
				# ADD token
				$skipc = 1;
				push @results, {tkn=>2, v=>$item}; $item = '';
			} else {
				$state = 0;
				# add token
				if($last == 1) {
					#label
					push @results, {tkn=>2, v=>$item}; $item = '';
				} else {
					if((@test = TestOp($item))[0] == 1) {
						push @results, {tkn=>3, v=>$item, a=>$test[1]}; $item = '';
						$lss = 3;
					} elsif( (@test = TestMacro($item))[0] == 1) {
						push @results, {tkn=>12, v=>$item, a=>$test[1]}; $item = '';
						$lss = 2;
					} else {
						if( (@test = TestReg($item))[0] == 1) {
							push @results, {tkn=>9, v=>$item, a=>$test[1]}; $item = '';
						} elsif( (@test = TestKeyw($item))[0] == 1) {
							push @results, {tkn=>26, v=>$item, a=>$test[1]}; $item = '';
						} else {
							if($lss < 1) {
								# bare label
								push @results, {tkn=>2, v=>$item}; $item = '';
								$lss = 1;
							} else {
								# ident/A-f...h const
								push @results, {tkn=>4, v=>$item}; $item = '';
								#push @results, {tkn=>6, v=>$item};
							}
						}
					}
				}
			}
		} elsif($state == 4) {
			if($last == 1) {
				if($c eq 'x') {
					$state = 6;
				} elsif($c eq 'b') {
					$state = 7;
				} elsif($c =~ /[0-7]/) {
					$state = 8;
					$item .= "0$c";
				} elsif($c =~ /[0-9a-fhA-F]/) {
					if($c eq 'h') {
						# add 0h
						push @results, {tkn=>6, v=>$item}; $item = '';
					} else {
						$state = 6;
						$item .= $c;
					}
				} else {
					$state = 0;
					# add 0
					push @results, {tkn=>5, v=>'0'}; $item = '';
				}
			} else {
				if($c eq '0') {
					$last = 1;
				} elsif($c =~ /[0-9]/) {
					$state = 5;
					$item .= $c
				} elsif($c =~ /[0-9a-fA-Fh]/) {
					$state = 6;
					$item .= $c
				} else {
					$state = 0;
					#add token
					push @results, {tkn=>5, v=>$item}; $item = '';
				}
			}
		} elsif($state == 5) {
			if($c =~ /[0-9]/) {
				$item .= $c;
			} elsif($c eq 'h') {
				# add as hex
				push @results, {tkn=>6, v=>$item}; $item = '';
				$state = 0;
			} else {
				$state = 0;
				# add dec
				print "TKN-ADEC " if($verbose > 5);
				push @results, {tkn=>5, v=>$item}; $item = '';
			}
		} elsif($state == 6) {
			if($c =~ /[0-9a-fA-F]/) {
				$item .= $c;
			} else {
				$state = 0;
				if($c eq 'h') { $skipc = 1; }
				# add hex
				push @results, {tkn=>6, v=>$item}; $item = '';
			}
		} elsif($state == 7) {
			if($c =~ /[01]/) {
				$item .= $c;
			} else {
				$state = 0;
				#add bin
				push @results, {tkn=>8, v=>$item}; $item = '';
			}
		} elsif($state == 8) {
			if($c =~ /[0-7]/) {
				$item .= $c;
			} else {
				$state = 0;
				#add oct
				push @results, {tkn=>7, v=>$item}; $item = '';
			}
		} elsif($state == 9) {
			$item .= $c;
		} elsif($state == 10) {
			if($c =~ /[a-zA-Z]/) {
				$item .= $c;
			} else {
				$state = 0;
				# add gen ident
				push @results, {tkn=>4, v=>$item}; $item = '';
			}
		}
		if($state == 0) {
			if($skipc == 1) {
				$skipc = 0;
			} else {
				# parse
				if($pstype == 0 and $c =~ /[_.%a-zA-Z]/) {
					$item = $c;
					$state = 3;
					$last = 0;
				} elsif($pstype == 1 and $c eq '%') {
					$state = 7;
					$last = 0;
				} elsif($pstype == 1 and $c =~ /[a-zA-Z]/) {
					$state = 10;
					$last = 0;
					$item .= $c;
				} elsif($c eq '0') {
					$state = 4;
					if($c eq '0') {
						$last = 1;
					} else {
						$last = 0;
						$item .= $c;
					}
				} elsif($c =~ /[1-9]/) {
					$state = 5;
					$last = 0;
					$item = $c;
				} elsif($c eq '"') {
					$state = 1;
					$last = 0;
				} elsif($c eq '\'') {
					$state = 2;
					$last = 0;
				} elsif($c eq '') {
					push @results, {tkn=>1, v=>''};
				} else {
					# special chars
					if($c =~ /[ \t]/) {
						if($last != 2) {
							$last = 2;
							# add ws token
							push @results, {tkn=>21, v=>' '};
						}
					} elsif($c eq ':') {
						$state = 3;
						$last = 1;
					} elsif($c eq ';') {
						$state = 9;
					} elsif($c eq ',') {
						push @results, {tkn=>20, v=>$c};
					} elsif($c eq '+') {
						push @results, {tkn=>13, v=>$c};
					} elsif($c eq '-') {
						push @results, {tkn=>14, v=>$c};
					} elsif($c eq '=') {
						#
					} elsif($c eq '#') {
						push @results, {tkn=>22, v=>$c};
					} elsif($c eq '*') {
						push @results, {tkn=>15, v=>$c};
					} elsif($c eq '/') {
						push @results, {tkn=>24, v=>$c};
					} elsif($c eq '\\') {
						push @results, {tkn=>25, v=>$c};
					} elsif($c eq '$') {
						#
					} elsif($c eq '&') {
						#
					} elsif($c eq '!') {
						#
					} elsif($c eq '[') {
						push @results, {tkn=>16, v=>$c};
					} elsif($c eq ']') {
						push @results, {tkn=>17, v=>$c};
					} elsif($c eq '(') {
						push @results, {tkn=>18, v=>$c};
					} elsif($c eq ')') {
						push @results, {tkn=>19, v=>$c};
					} elsif($c eq '<') {
					} elsif($c eq '>') {
					} elsif($c eq '{') {
					} elsif($c eq '}') {
					} elsif($c eq '|') {
					} elsif($c eq '^') {
					} elsif($c eq '@') {
					} elsif($c eq '~') {
					} else {
						# what?
					}
				}
			}
		}
	}
	return @results;
}

sub ParseSymbol {
	my ($allsym, $itr, $arc) = @_;
	my $userel = 0;
	my (undef, $mrel) = split(',',$arc);
	if($mrel =~ /r/) {
		$userel = 1;
	}
	my $lss = 0;
	for my $tkn (@$allsym) {
		my $arg = $tkn->{v};
		my $elem = $tkn->{tkn};
		my $minus = "";
		if($tkn->{tkn} == 12) {
			$lss = 3;
			next;
		}
		if($lss > 2) {
			if($tkn->{tkn} != 20 and $tkn->{tkn} != 1) {
				if($tkn->{tkn} == 21) { next; }
			if($elem == 16) {
			} elsif($elem == 17) {
			} elsif($elem == 13) {
			} elsif($elem == 14) {
				$minus = "-";
			} elsif($elem == 9) {
			} elsif($elem == 26) {
			} else {
				return DecodeSymbol($arg, $minus, $elem, $itr, $userel);
			}
			}
		}
	}
	return ('nul',undef,0, "");
}

sub DecodeSymbols {
	my ($allsym, $itr, $arc, $wordsz) = @_;
	my $format;
	my @format = ();
	my $maxitr = 0;
	my @encode = ();
	my $userel = 0;
	my ($iis, $mrel) = split(',',$arc);
	my $ewordsz = "";
	if($wordsz != undef && $wordsz > 1) {
		$ewordsz = $wordsz;
	}
	if($mrel =~ /r/) {
		$userel = 1;
	}
	my $lss = 0;
	my @lfs = ();
	for my $tkn (@$allsym) {
		print STDERR "T$tkn->{tkn}:$tkn->{v} " if($verbose > 5);
		my $arg = $tkn->{v};
		my $elem = $tkn->{tkn};
		my $minus = "";
		#my @wsa = SplitElem($arg);
		if($elem == 3 or $elem == 12) {
			$lss = 3;
			next;
		}
		if($lss > 2) {
			if($tkn->{tkn} != 20 and $tkn->{tkn} != 1) {
				if($tkn->{tkn} == 21) { next; }
			print STDERR "[ELEM'$arg']" if($verbose > 5);
			if($elem == 16) {
				if($arg !~ /\[.*\]/) {
					print STDERR "$langtable{error}: $langtable{line} $.: $langtable{unmatchlb}\n";
				} else {
					push @lfs, "[";
				}
			} elsif($elem == 17) {
				if($arg !~ /\[.*\]/) {
					print STDERR "$langtable{error}: $langtable{line} $.: $langtable{unmatchrb}\n";
				} else {
					push @lfs, "]";
				}
			} elsif($elem == 13) {
				push @lfs, "+";
			} elsif($elem == 14) {
				$minus = "-";
				push @lfs, "+";
			} elsif($elem == 9) {
				my ($fnd, $scr) = TestReg($arg);
				push @lfs, $scr->{nam};
				push @encode, $scr->{encode};
			} elsif($elem == 26) {
			} else {
				my ($type, $vec, $i, $v) = DecodeSymbol($arg, $minus, $elem, $itr, $userel);
				if($type eq "reg") {
					push @lfs, $vec->{nam};
					push @encode, $vec->{encode};
				} elsif($type eq "keyw") {
					push @lfs, $vec->{nam};
					push @encode, $vec->{encode} if($vec->{encode} ne '');
				} elsif($type eq "lit") {
					push @lfs, $vec->{nam};
					push @encode, $vec->{encode}. "+$v";
					$maxitr = $i;
				} elsif($type eq "val") {
					push @encode, "+ALM$ewordsz+$v";
				} elsif($type eq "str") {
					push @encode, "+ASLM$ewordsz$v";
				} else {
					push @lfs, "*";
					push @encode, "+$v";
					$maxitr = $i;
				}
				$minus = "";
			}
			} else {
				$format = join('',@lfs);
				push @format, ($format);
				@lfs = ();
			}
		}
	}
	$format = join(',',@format);
	print STDERR "TKF: $format " if($verbose > 3);
	return ($format,$maxitr,@encode);
}

sub PadWord {
	my ($inw) = @_;
	my $inl = length($inw);
	my $disp = ( $inl % $cputable{CPUM} );
	return $inw if($disp == 0);
	my $pdz = $cputable{CPUM} - $disp;
	if($pdz == 0) {
		return $inw;
	} else {
		return sprintf("%0${pdz}b%s",0,$inw);
	}
}

sub BinSplit {
	my @invec = @_;
	my @out;
	my $bitcount;
	my $datout;
	foreach my $bt (@invec) {
		my @lout;
		my $l = length($bt);
		$bitcount += $l;
		my $fwl = $cputable{FILEW};
		my $nwl = $fwl / 4;
		my $bwl = $fwl / 8;
		for(my $i = 0; $i < $l; $i += $fwl) {
			if($cputable{CPUE} eq "LE") {
				unshift @lout, sprintf("%0${nwl}X",oct("0b".substr($bt,$i,$fwl)));
			} else {
				push @lout, sprintf("%0${nwl}X",oct("0b".substr($bt,$i,$fwl)));
			}
		}
		$datout .= pack("H*", join('',@lout)) ;
		push @out, join('',@lout);
	}
	return ($bitcount, $datout, @out);
}

sub RunEncoder {
	my ($instruct, @extra) = @_;
	my @output = ();
	my $output = "";
	my $otlen = 0;
	my $otlv = 0;
	my $otapp = 0;
	my $otrel = 0;
	for my $allenc (split(':',$instruct)) {
		$output = "";
		$otlen = 0;
		my @ienc = split(' ', $allenc);
		foreach my $es (@ienc) {
			if($es =~ /^L([0-9]+)/) {
				print STDERR "ENC-STL: $1\n" if($verbose > 5);
				$otlen = $1;
			} elsif($es =~ /^MR([BN]*)$/) {
				$otrel = 1;
			} elsif($es =~ /\+/) {
				foreach my $k (split('\+', $es)) {
					next if($k eq "");
					if($k =~ /^L([0-9]+)/) {
						print STDERR "ENC-STL: $1\n" if($verbose > 5);
						$otlen = $1 + 0;
					} elsif($k =~ /^AL([0-9]+)/) {
						print STDERR "ENC-ASL: $1\n" if($verbose > 5);
						$otlen = $1 + 0;
						$otapp = 1;
					} elsif($k =~ /^ALM([0-9]*)/) {
						if($1 ne '') {
							$otapp = 1;
							$otlv = 1;
							$otlen = $1;
						} else {
							$otapp = 1;
							$otlv = 1; # added for .DAT
						}
					} elsif($k =~ /^ASLM([0-9]*)/) {
						if($1 ne '') {
							$otapp = 2;
							$otlv = 1;
							$otlen = $1;
						} else {
							$otapp = 2;
							$otlv = 1; # added for .DAT
						}
					} elsif($k =~ /^0x([0-9a-fA-F]+)$/) {
						my $tl = length($1) * 4;
						my $v = oct("0x$1");
						my $ins = sprintf("%0${tl}b", $v);
						$ins = substr($ins,-$otlen,$otlen) if($otlen != 0);
						$ins = PadWord($ins) if($otlv != 0);
						print STDERR "ENC-ACL $ins\n" if($verbose > 5);
						if($otapp == 0) {
						$output .= $ins;
						} else {
						push @output, $ins;
						$otapp = 0 if($otapp == 1);
						}
					} elsif($k =~ /^%([01]+)$/ ) {
						my $x = $1;
						$x = substr($x,-$otlen,$otlen) if($otlen != 0);
						$x = PadWord($x) if($otlv != 0);
						if($otapp == 0) {
							$output .= $x;
						} else {
						push @output, $x;
						$otapp = 0 if($otapp == 1);
						print STDERR "ENC-ACL\n" if($verbose > 5);
						}
					} elsif($k =~ /\\([0-9]+)/ ) {
						my $x = $1 - 1;
						print STDERR "ENC-Rec: $1 $extra[$x]\n" if($verbose > 5);
						my ($ins, @apd) = RunEncoder("+".$extra[$x]);
						if($otapp == 0) {
						$output .= $ins;
						} else {
						push @output, $ins;
						$otapp = 0 if($otapp == 1);
						print STDERR "ENC-ACL\n" if($verbose > 5);
						}
						print STDERR "ENC-Res: $ins " . join('',@apd) . "\n" if($verbose > 5);
						push @output, @apd;
					} elsif($k =~ /^(-*[0-9]+)$/) {
						my $x = $1 + 0;
						my $o = sprintf("%0${otlen}b",$x);
						$o = substr($o,-$otlen,$otlen) if($otlen != 0);
						$o = PadWord($o) if($otlv != 0);
						print STDERR "ENC-int: $x\n" if($verbose > 5);
						if($otapp == 0) {
						$output .= $o;
						} else {
						push @output, $o;
						$otapp = 0 if($otapp == 1);
						print STDERR "ENC-ACL\n" if($verbose > 5);
						}
					} elsif($k eq '*') {
						my $x = sprintf("%0${otlen}b",0);
						print STDERR "ENC-fpw: $x\n" if($verbose > 5);
						if($otapp == 0) {
						$output .= $x;
						} else {
						push @output, $x;
						$otapp = 0 if($otapp == 1);
						print STDERR "ENC-ACL\n" if($verbose > 5);
						}
					} else {
						print STDERR "ENC-unk: '$k'\n" if($verbose > 5);
					}
				}
			}
		}
		$otapp = 0;
		unshift @output,$output if($output ne '');
	}
	return @output;
}

sub Pass2 {
	for my $l (@allfile) {
		my ($linearg,$fnum) = ($l->{line},$l->{fnum});
		my $fname = @incltable[$fnum - 1]->{f};
		my ($macro,$label,$opname);
		for my $r (@$linearg) {
			if($r->{tkn} == 2) { $label = $r->{v}; }
			if($r->{tkn} == 3) { $opname = uc($r->{v}); }
			if($r->{tkn} == 12) { $macro = uc($r->{v}); }
		}
		$vinstrend = $l->{ilen};
		print STDERR join("\t",($l->{lnum},sprintf("%08x",$vpc),$label,$opname,$linearg)) . "\t"  if($verbose > 2);
		if($label ne '' && $macro ne '.EQU') {
			print STDERR "SYMGEN: $label ".$symtable{$label}{type}."\n" if($verbose > 2);
			if($symtable{$label}{type} ne "") {
				if($symtable{$label}{val} eq "*" || $symtable{$label}{val} != $vpc) {
					$symtable{$label}{val} = $vpc;
					$l->{addr} = $vpc;
					print STDERR "SYMSET: $label $symtable{$label}{val}\n" if($verbose > 3);
					$flagpass++;
				}
			}
		}
		my $enc = undef;
		my $found = 0;
		my $addrmode = 0;
		my $arc;
		my $itr = 0;
		my $format; my $maxitr;
		my @encode;
		my $encode;
		my $lformat;
		my @lencode;
		if($opname ne '') {
			do {
				$found = 0;
			foreach my $i ( @optable ) {
				if(($i->{op}) eq ($opname)) {
					$enc = $i->{encode};
					$arc = $i->{arc};
					if($enc eq 'M') {
						$addrmode = 1;
					} else {
						my $align;
						if($cputable{ALIGN} > 0) {
							$align = $vpc % $cputable{ALIGN};
							$vpc += $cputable{ALIGN} - $align if($align > 0); # align words
						}
					}
					$found++;
					my $tmxr;
			($format, $tmxr, @encode) = DecodeSymbols($linearg, $itr, $arc);
					$maxitr = $tmxr if($tmxr > $maxitr);
					if($i->{arf} eq $format) {
						$addrmode = 1;
						last;
					}
				}
			}
			$itr++;
			print STDERR "ITR: $itr $found $addrmode\n" if($verbose > 4);
			} while($found != 0 && $itr < $maxitr && $addrmode == 0);
			$itr--;
		} elsif($macro ne '') {
			$found = 1;
			$addrmode = 1;
		} else {
			$found = -1; #no opcode on line
			$addrmode = -1;
			print STDERR "\n" if($verbose > 2 && $label ne '');
		}
		if($found == 0) {
			print STDERR "$langtable{error}: $fname:$l->{lnum}:",
			" $opname - $langtable{opunkn}\n$l->{ltxt}\n";
			$errors++;
		} else {
			print STDERR "INT: $found$addrmode $format\n" if($verbose > 4);
			if($addrmode == 0) {
				print STDERR "$langtable{error}: $fname:$l->{lnum}:",
					" $opname - $langtable{addrunkn}\n$l->{ltxt}\n";
				$errors++;
			} elsif($macro ne '') {
				if($macro eq '.ORG') {
					my ($type, $vec, $i, $v) = ParseSymbol($linearg,-1,$arc);
					if($type eq "val") {
						$vpc = $v;
					}
					# rerun line labels
					if($label ne '') {
					if($symtable{$label}{type} ne "") {
					if($symtable{$label}{val} eq "*" || $symtable{$label}{val} != $vpc) {
						$symtable{$label}{val} = $vpc;
						print STDERR "RRSYM: $vpc\n" if($verbose > 3);
						$flagpass++;
					}
					}
					}
				} elsif($macro eq '.EQU') {
					my ($type, $vec, $i, $v) = ParseSymbol($linearg,-1,$arc);
					if($type eq "val") {
					if($label ne '') {
					if($symtable{$label}{val} != $v) {
						print STDERR "RRSYM: $symtable{$label}{val} $v\n" if($verbose > 2);
						$symtable{$label}{val} = $v;
						$flagpass++;
					}
					}
					}

				} elsif($macro =~ /\.D(AT|[BDW])/) {
					if($1 eq 'W') {
					($lformat, undef, @lencode) = DecodeSymbols($linearg, -1, $arc, 16);
					} elsif($1 eq 'D') {
					($lformat, undef, @lencode) = DecodeSymbols($linearg, -1, $arc, 32);
					} else {
					($lformat, undef, @lencode) = DecodeSymbols($linearg, -1, $arc);
					}
					print STDERR "\n".join(' ', @lencode)  if($verbose > 3);
					my @words;
					@words = RunEncoder(join(' ', @lencode));
					print STDERR "\n".join(' ', @words) . "\n" if($verbose > 3);
					my ($avl, $dat, @bytes) = BinSplit(@words);
					my $txtbyte = join(' ', @bytes);
					print STDERR $txtbyte . "\n" if($verbose > 2);
					# encode data
					$l->{addr} = $vpc;
					$vinstrend = ($avl / $cputable{CPUM});
					$vpc += $vinstrend;
					$flagpass++ if($l->{ilen} != $vinstrend);
					$l->{ilen} = $vinstrend + 0;
					$l->{byte} = $txtbyte;
					$l->{dat} = $dat;
					$addrmode = -1;
				}

			} else {
			}
		}
		if($found >= 1 && $addrmode == 1) {
			my @words;
			@words = RunEncoder($enc, @encode);
			my ($avl, $dat, @bytes) = BinSplit(@words);
			$l->{addr} = $vpc;
			$vinstrend = ($avl / $cputable{CPUM});
			$vpc += $vinstrend;
			$l->{ilen} = $vinstrend;
			# Reencode for relative jumps
			($lformat, undef, @lencode) = DecodeSymbols($linearg, $itr, $arc);
			### Debug block
			if($verbose > 3) {
				$encode = join('||',@encode);
				print STDERR "\n$format \"$encode\" $enc";
				$encode = join('||',@lencode);
				print STDERR "\n$lformat \"$encode\" $enc\n";
			}
			###
			@words = RunEncoder($enc, @lencode);
			my ($lavl, $ldat, @lbytes) = BinSplit(@words);
			if($lavl != $avl) {
				print STDERR "AVC: $avl $lavl\n" if($verbose > 3);
				$flagpass++;
				my $txtbyte = join('', @lbytes);
				print STDERR $txtbyte . "\n" if($verbose > 2);
				$l->{byte} = $txtbyte;
				$l->{dat} = $ldat;
			} else {
				my $txtbyte = join('', @bytes);
				print STDERR $txtbyte . "\n" if($verbose > 2);
				$l->{byte} = $txtbyte;
				$l->{dat} = $dat;
			}
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

sub LoadInclude {
	my ($file, $from, $scan, $rc) = @_;
	my $fnum = 0;
	my $lfh;
	my $path = "";
	if($file =~ /(.*\/)[^\/]*/) {
		$path = $1;
	}
	if($rc > 20) {
		print STDERR "ERROR: $file: included from $from, recurse limit\n";
		$errors++;
		return;
	}
	print STDERR "Loading file $file\n" if($verbose > 0);
	unless( open($lfh, "<", $file) ) { # try open file
		print STDERR $langtable{fileof1} . $file . $langtable{fileof2} . "\n";
		return;
	}
	push @incltable, {f=>$file, i=>$from};
	$fnum = @incltable; # get length
	while(<$lfh>) {
		# clean up the lines first
		s/\n//;
		my $prl = $_;
		$prl =~ s/;.*$//;
		#$prl =~ s/^\W*$//;
		if($prl =~ /^[ \t]*$/) { # blank line
			next;
		}
		if($prl =~ /[#.][iI]nclude[ \t]+("|<)([^"<>]*)("|>)/ ) { #includes
			if($1 eq '"' && $3 eq '"') {
				LoadInclude($path . $2,$file,1,$rc+1);
			} elsif($1 eq '<' && $3 eq '>') {
				LoadInclude($path . $2,$file,2,$rc+1);
			}
			next;
		}
		my ($label,$opname,@linearg);# = split(/[ \t]+/, $prl);
		my $linearg;# = join(' ',@linearg);
		#if($prl =~ /^([^ \t]*)[ \t]+([^ \t]*)[ \t]*([^ \t]*.*)$/) { # standard line
		#	($label,$opname,$linearg) = ($1,$2,$3);
		#} elsif($prl =~ /^[ \t]*([^ \t]+):[ \t]+([^ \t]*)[ \t]*([^ \t]*.*)$/) { # alt label pos
		#	($label,$opname,$linearg) = ($1,$2,$3);
		#} elsif($prl =~ /^[ \t]*([^ \t]+):[ \t]*$/) { # label only
		#	($label,$opname,$linearg) = ($1,'','');
		#}
		@linearg = TokenizeLine($prl);
	for my $r (@linearg) {
		print STDERR "T$r->{tkn} $r->{v} " if($verbose > 5);
		if($r->{tkn} == 2) { $label = $r->{v}; }
		if($r->{tkn} == 3) { $opname = $r->{v}; }
	}
		print STDERR "\n" if($verbose > 5);
		#$linearg =~ s/\+/ + /g;
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
					print STDERR "$langtable{error}: $file:$.; $slnum->{lnum}: $langtable{duplabel}\n$_\n$slnum->{ltxt}\n";
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
				print STDERR "$langtable{error}: $file:$.: $opname - $langtable{opunkn}\n$_\n";
				$errors++;
			}
		}
		push @allfile, {lnum => $., fnum => $fnum, ltxt => $_, label => $label, op => $opname, line => \@linearg};
		print STDERR join("\t",($.,$label,$opname,$enc))."\n" if($verbose > 2);
	}
	close ISF;
}

sub WriteListing {
	my ($outf) = @_;
	unless( open(OSF, ">", $outf) ) {
		print STDERR $langtable{fileof1} . $outf . $langtable{fileof2} . "\n";
		return;
	}
	foreach my $l (@allfile) {
		my ($linearg) = ($l->{line});
		my $lss = 0;
		print OSF join("\t",($l->{lnum},sprintf("%08x",$l->{addr})))." ";
		for my $r (@$linearg) {
			if($r->{tkn} == 2) { print OSF $r->{v} . "\t"; $lss = 1;
			} elsif($r->{tkn} == 3) {
				print OSF " \t" if($lss == 0);
				print OSF $r->{v} . "\t"; $lss = 2;
			} elsif($r->{tkn} == 12) {
				print OSF " \t" if($lss == 0);
				print OSF $r->{v} ."\t"; $lss = 2;
			} elsif($r->{tkn} != 21) {
				print OSF $r->{v};
			}
		}
		print OSF "\t" . $l->{byte} . "\n" ;
	}
	print OSF "Symbols:\n" if($verbose > 0);
	foreach my $l (keys %symtable) {
		printf OSF "%08x  %s\n",$symtable{$l}{val},$l
	}
	close OSF;
}

sub WriteFlat {
	my ($outf) = @_;
	unless( open(OBF, ">", $outf) ) {
		print STDERR $langtable{fileof1} . $outf . $langtable{fileof2} . "\n";
		return;
	} else {
		binmode OBF;
	}
	my $vpi = undef;
	print STDERR "Writting flat binary to $outf\n" if($verbose > 1);
	foreach my $l (@allfile) {
		if($vpi == undef) { $vpi = $l->{addr}; }
		if($vpi < $l->{addr}) {
			if($vpi + $cputable{ALIGN} >= $l->{addr}) {
				my $align = $vpi % $cputable{ALIGN};
				my $padsz = ($cputable{ALIGN} - $align) if($align > 0); # align words in file
				for(my $i = 0; $i < $padsz; $i++) { print OBF "\0"; }
				print STDERR "Wrote $padsz null bytes\n" if($verbose > 2);
				$vpi += $padsz;
			} else {
				print STDERR "Address break in binary flat file ", sprintf("%08x to %08x",$vpi, $l->{addr}), "\nLine: $l->{lnum}\n" if($verbose > 1);
				$vpi = $l->{addr};
			}
		}
		print OBF $l->{dat};
		$vpi += length($l->{dat});
	}
	close OBF;
}

sub Assemble {
	my ($file, $outfile, $outbinfile) = @_;
	my $assmpass = 0;
	@incltable = ( );
	$errors = 0;
	print STDERR "Pass 1\n" if($verbose > 0);
	LoadInclude($file, "", 0, 0);
	$assmpass = 2;
	do {
	$vpc = 0;
	$errors = 0;
	$flagpass = 0;
	print STDERR "Pass $assmpass\n" if($verbose > 0);
	Pass2();
	} while(($assmpass++) < 9 && $flagpass >= 1 && $errors < 1);
	if($errors == 0) {
		print STDERR "Complete!\n" if($verbose > 0);
		WriteListing($outfile);
		WriteFlat($outbinfile);
	} else {
		print STDERR "Errors in assembly.\n";
	}
}
