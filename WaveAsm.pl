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
	duplabel => "Duplicate label on line",
	nolabel => "No such label",
	synerr => "Syntax error",
	badlabel => "Invalid label"
);
my @regtable = ( {reg => '*', nam => 'intern'} );
my @littable = ( );
my @incltable = ( );
my @keywtable = ( );
my @grptable = ( );
my %symtable;
my @allfile;
my $vpc = 0;
my $vinstrend = 0;
my $errors = 0;
my $flagpass = 0;
my $verbose = 1;
my $binformat = 'f';
my $exten = '.ffi';
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

print STDERR "Wave Asm - version 0.4.3\n";
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
			$exten = '.ffi';
		} elsif($flags eq 'fs') {
			$binformat = 's';
			$exten = '.s19';
		} elsif($flags eq 'fE') {
			$binformat = 'E';
			$exten = '.a';
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
	my $filen = $_;
	if($filen =~ /(.*)(\.[aA][sS][mM]|\.s)$/) {
		$filen = $1;
	}
	Assemble( $_, ($filen . ".lst") , ($filen . $exten));
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
					} elsif($a =~ /^O(-?[0-9]+)/) {
						$att{ofs} = $1 * 1;
					}
				}
				print STDERR "+NAME=$att{nam} +LEN=$att{len} +OFFSET=$att{ofs}\n" if($verbose > 4);
			} else {
				my ($range, @encode) = split(':', $_);
				my ($rl,$ru) = split(',', $range);
				($rl,$ru) = ('*','*') if($range eq '*');
				for my $i (@encode) {
					$i = "L$att{len}*" if($i eq '*');
				}
				push @littable, ({
						rl => $rl,
						ru => $ru,
						nam => $att{nam},
						encode => join(':',@encode),
						ofs=>$att{ofs}
					});
				print STDERR "LIT: $att{nam} $range $att{len} " . join(':',@encode) . "\n" if($verbose > 4);

			}
		} elsif(/<GROUP>/ .. /<\/GROUP>/) {
			if(/^\W*#/ or /</) {
			} elsif(/^\W*$/) {
			} else {
				my @gsec = split(':', $_);
				my $gname = ($gsec[0]);
				$gname =~ s/"//g;
				my $fcode = $gsec[1];
				$fcode =~ s/"([^"]+)"/\1/;
				push @grptable, {nam => $gname, arf => $fcode, encode => $gsec[2]};
				print STDERR "GROUP: $gname $fcode\n" if($verbose > 4);
			}
		} elsif(/<OPCODE>/ .. /<\/OPCODE>/) {
			if(/^\W*#/ or /</) {
			} elsif(/^\W*$/) {
			} else {
				my @opparam = split(':', $_);
				my $opname = ($opparam[0]);
				$opname =~ s/"//g;
				my $fcode = $opparam[2];
				$fcode =~ s/"([^"]*)"/\1/;
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
	} elsif($tkval == 11) { # String literal
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
	} elsif($tkval == 4) { # Labels / Hexadecimal ...h
		my $r = $symtable{$sym};
		if($r->{val} ne '') {
			# label
			return ("val",$r->{val});
		} elsif($sym =~ /^([0-9a-fA-F]+)h$/) {
			$v = oct("0x$1");
			$v = -$v if(length($nvc) % 2 == 1);
		} else {
			# label not found or not hex const
			return ("err", "\"$sym\" $langtable{nolabel}");
		}
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

sub ParseValue {
	my ($v, $rel) = @_;

	my $ivf = [];
	my $pv;
	$v -= ($vpc + $vinstrend) if($rel == 1);
	my $cname = '';
	for my $r (@littable) {
		if($r->{rl} eq '*') {
			if($cname ne $r->{nam}) {
				$pv = $v + $r->{ofs};
				print STDERR "LLA: $v [ $r->{rl} $r->{ru} ]\n" if($verbose > 5);
				push @$ivf, {type=>'V',nam => $r->{nam}, encode => $r->{encode}."+$pv", val => $v};
				$cname = $r->{nam};
			}
		} elsif(($r->{rl} <= $v) && ($r->{ru} >= $v)) {
			if($cname ne $r->{nam}) {
				$pv = $v + $r->{ofs};
				print STDERR "LLR: $v [ $r->{rl} $r->{ru} ]\n" if($verbose > 5);
				push @$ivf, {type=>'V',nam => $r->{nam}, encode => $r->{encode}."+$pv", val => $v};
				$cname = $r->{nam};
			}
		}
	}
	return $ivf;
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
	my ($line, $file, $pstype) = @_;
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
					if($item eq '') {
						print STDERR "$langtable{error}: $file:$.: $langtable{badlabel}\n";
						$errors++;
					}
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
						# nani?
					}
				}
			}
		}
	}
	return @results;
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
		return ('0'x$pdz).$inw;
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

sub FullParse {
	for my $l (@allfile) {
		my ($linearg,$fnum) = ($l->{line},$l->{fnum});
		my $fname = @incltable[$fnum - 1]->{f};
		my ($macro,$label,$opname) = ('','','');
		my $labelca = $vpc;
		my $labelcv = $vpc;
		my $lss = 0;
		$vinstrend = $l->{ilen};
		my $format;
		my @format = ();
		my @values = ();
		my $maxitr = 1;
		my $encode;
		my @lfs = ();
		my @lvs = ();
		my $curval = 0;
		my $curtype = '';
		my $curop = '';

		for my $r (@$linearg) {
			my $arg = $r->{v};
			my $elem = $r->{tkn};
			if($elem == 2) {
				if($lss > 0) {
				print STDERR "$langtable{error}: $fname:$l->{lnum}: $langtable{synerr}\n";
				$errors++;
				} else {
					$label = $arg;
					$lss = 1;
				}
			} elsif($elem == 3) {
				if($lss > 1) {
				print STDERR "$langtable{error}: $fname:$l->{lnum}: $langtable{synerr}\n";
					$errors++;
				} else {
					$opname = uc($arg);
					$lss = 2;
				}
			} elsif($elem == 12) {
				if($lss > 1) {
				print STDERR "$langtable{error}: $fname:$l->{lnum}: $langtable{synerr}\n";
					$errors++;
				} else {
					$macro = uc($arg);
					$lss = 2;
				}
			} else {
				if($lss > 1) {
					if($elem != 20 and $elem != 1) {
						if($elem != 21) {
							print STDERR "[ELEM'$arg']" if($verbose > 5);
						}
						if($elem == 16) {
							if($curtype eq 'V') {
								if($curop ne '') { push @lfs, $curop; $curop = ''; }
								push @lvs, $curval;
								push @lfs, ParseValue($curval, $l->{reladdr});
							}
							$curtype = '';
							if($curop ne '') { push @lfs, $curop; $curop = ''; }
							push @lfs, "[";
						} elsif($elem == 17) {
							if($curtype eq 'V') {
								if($curop ne '') { push @lfs, $curop; $curop = ''; }
								push @lvs, $curval;
								push @lfs, ParseValue($curval, $l->{reladdr});
							}
							$curtype = '';
							if($curop ne '') { push @lfs, $curop; $curop = ''; }
							push @lfs, "]";
						} elsif($elem == 13) {
							# plus
							if($curop ne '') { push @lfs, $curop; $curop = ''; }
							$curop = '+';
						} elsif($elem == 14) {
							if($curop ne '') { push @lfs, $curop; $curop = ''; }
							$curop = '-';
						} elsif($elem == 9) {
							# reg
							if($curop ne '') { push @lfs, $curop; $curop = ''; }
							$curtype = 'R';
							my ($fnd, $scr) = TestReg($arg);
							push @lfs, {type=>'R',nam=>$scr->{nam},encode=>$scr->{encode}};
						} elsif($elem == 26) {
							# keyword
							if($curop ne '') { push @lfs, $curop; $curop = ''; }
							$curtype = 'K';
							my ($fnd, $scr) = TestKeyw($arg);
							push @lfs, {type=>'K',nam=>$scr->{nam},encode=>$scr->{encode}};
						} elsif($elem == 21) {
							#ignore this
						} else {
							print STDERR "UKT: $elem\n" if($verbose > 5);
							my ($type, $v) = DecodeValue($arg, $elem, '');
							if($type eq "val") {
								print STDERR "VALTYPE: $v\n" if($verbose > 5);
								if($curtype eq 'V') {
									if($curop eq '+') {
										$curval += $v;
										$curop = '';
									} elsif($curop eq '-') {
										$curval -= $v;
										$curop = '';
									} else {
										push @lvs, $v;
										push @lfs, ParseValue($v, $l->{reladdr});
									}
								} else {
									if($curop eq '-' and $curtype eq '') {
										$curop = '';
										$curval = -$v;
									} else {
										push @lfs, $curop if($curop ne '');
										$curop = '';
										$curval = $v;
									}
								}
								$curtype = 'V';
							} elsif($type eq "str") {
								$curtype = 'S';
								push @lfs, $curop if($curop ne '');
								push @lvs, {type=>'S',encode=>"+ASLM$v"};
							} elsif($type eq "err") {
								$curtype = '';
							print STDERR "$langtable{error}: $fname:$l->{lnum}: $v\n";
								$errors++;
							} else {
								$curtype = '';
								push @lfs, {type=>"*",encode=>"+$v"};
							}
						}
					} else {
						if($curtype eq 'V') {
							if($curop ne '') { push @lfs, $curop; $curop = ''; }
							push @lvs, $curval;
							push @lfs, ParseValue($curval, $l->{reladdr});
						}
						$format = [{f=>'',e=>[]}];
						$encode = [[]];
						for my $x (@lfs) {
							if(ref($x) eq 'HASH') {
								if($verbose > 5) {
									print STDERR "HASH: ". $x->{type} ."\n";
									print STDERR "ENCD: ". $x->{encode}."\n";
								}
								if($x->{type} ne 'S') {
									for(my $xe=0; $xe<@$format; $xe++) {
										$$format[$xe]->{f} .= $x->{nam};
										push @{$$format[$xe]->{e}}, $x->{encode};
									}
								}
							} elsif(ref($x) eq 'ARRAY') {
								print STDERR "ARRY: ". (@$x + 0) ."\n" if($verbose > 5);
								my $insvarcnt;
								$insvarcnt = @$format;
								for(my $cax=1;$cax<@$x;$cax++) {
									if($verbose > 5) {
										print STDERR "ARRI: ". $$x[$cax]->{nam} ."\n";
										print STDERR "ENCD: ". $$x[$cax]->{encode}."\n";
									}
									for(my $xe=0;$xe<$insvarcnt;$xe++) {
										push @$format, {
											f=>$$format[$xe]->{f}.$$x[$cax]->{nam},
											e=>[@{$$format[$xe]->{e}},$$x[$cax]->{encode}]
										};
									}
								}
								for(my $xe=0;$xe<$insvarcnt;$xe++) {
									$$format[$xe]->{f}.=$$x[0]->{nam};
									push @{$$format[$xe]->{e}}, $$x[0]->{encode};
									if($verbose > 5) {
										print STDERR "ARRI: ". $$x[0]->{nam} ."\n";
										print STDERR "ENCD: ". $$x[0]->{encode}."\n";
									}
								}
							} else {
								print STDERR "VAL: ". $x ."\n" if($verbose > 5);
								for(my $xe=0; $xe<@$format; $xe++) {
									$$format[$xe]->{f} .= $x;
								}
							}
						}
						my $svarcnt;
						$svarcnt = @$format;
						for( my $xe = 0; $xe < $svarcnt; $xe++) {
							for my $gq (@grptable) {
								if($gq->{arf} eq $$format[$xe]->{f}) {
									if($verbose > 5) {
										print STDERR "GRP-MATCH: \"". $gq->{arf}."\" ". $gq->{nam}. " ". $gq->{encode} ."\n";
									}
									my $cess = $gq->{encode};
									my @suf;
									while($cess =~ /\\([0-9]+)/) {
										my $cenn = $$format[$xe]->{e}[$1 - 1];
										my ($cee, @cee) = split(':', $cenn);
										$cess =~ s/\\([0-9]+)/$cee/;
										push @suf, @cee;
									}
									$cess.=":".join('+', @suf);
									my $insvarcnt;
									$insvarcnt = @$format;
									push @$format, {
										f=>$gq->{nam},
										e=>[$cess]
									};
									print STDERR "GRP-ENKOD: ".$cess."\n" if($verbose >5);
								}
							}
						}
						if($verbose > 7) {
							for( my $xe = 0; $xe < @$format; $xe++) {
								print STDERR "FMT: " . $$format[$xe]->{f} ."\nENC: ";
								for my $xxe (@{$$format[$xe]->{e}}) {
									print STDERR "{".$xxe."}";
								}
								print STDERR "\n";
							}
						}
						if(@$format > $maxitr) {
							$maxitr = @$format;
						}
						push @values, @lvs;
						push @format, $format;
						push @format, ',' if($elem == 20);
						@lfs = ();
						@lvs = ();
						$curtype = '';
						$curval = 0;
						$curop = '';
						#print STDERR "$langtable{error}: $langtable{line} $.:",
						#" $langtable{unmatchlb}\n";
						#print STDERR "$langtable{error}: $langtable{line} $.:",
						#" $langtable{unmatchrb}\n";
					}
				} else {
					if($elem == 21) { next; }
					if($elem == 1) { next; }
					print STDERR "$langtable{error}: $fname:$l->{lnum}: $arg ($elem) $langtable{synerr}\n";
					$errors++;
				}
			}
		}
		# process line
		if($opname ne '') {
			print STDERR "OPC: $opname\n" if($verbose > 4);
			$format = [ ];
			$encode = [ ];
			for( my $x = 0; $x < $maxitr; $x++ ) {
				for(my $y = 0; $y < $maxitr; $y++) {
					push @$format, {f=>'', e=>[], w=>-1};
				}
			}
			for(my $xc=0; $xc < $maxitr; $xc++) {
				my $xr = 0;
				for(my $q=0;$q<@format;$q++) {
					my $x = $format[$q];

					if(ref($x) eq 'HASH') {
					} elsif(ref($x) eq 'ARRAY') {
						for(my $xe=0; $xe < $maxitr; $xe++) {

							$$format[$xc*$maxitr+$xe]->{w} = $xe + $xc;
							$$format[$xc*$maxitr+$xe]->{f} .= $$x[($xe+$xr*$xc) % @$x]->{f};
							push @{$$format[$xc*$maxitr+$xe]->{e}}, join(' ',@{$$x[($xe+$xr*$xc) % @$x]->{e}});
		print STDERR "EP: $xe $xr $xc ".$$x[($xe+$xr*$xc) % @$x]->{f}."="
		. join('--',@{$$x[($xe+$xr*$xc) % @$x]->{e}}) . "\t" if($verbose > 5);
						}
					} else {
						$xr++;
						print STDERR "$xc PARAM: ". $x if($verbose > 5);
						for(my $xe=0; $xe < $maxitr; $xe++) {
							$$format[$xc*$maxitr+$xe]->{f} .= $x;
						}
					}
					print STDERR "\n" if($verbose > 5);
				}
			}
			if($verbose > 4) {
				for(my $xe=0; $xe < @$format; $xe++) {
					print STDERR "SCNFMT: ". $$format[$xe]->{f} . "\n";
					print STDERR "SCNENC: ". join(' ', @{$$format[$xe]->{e}})."\n";
				}
			}

			my $enc = undef;
			my $arc = undef;
			my $encsel = -1;
			my $selw = -1;
			foreach my $i ( @optable ) {
				# TODO: Use Hashes here!!!
				if(($i->{op}) eq ($opname)) {
					if($i->{arf} eq '') {
						$enc = $i->{encode};
						$arc = $i->{arc};
						$encsel = 0;
						$selw = 0;
						if(@$format > 1) {
							$encsel = -1;
					print STDERR "ARFC $i->{arf} $i->{encode} ".@$format."\n" if($verbose > 5);
						}
					} else {
						for(my $xe=0; $xe < @$format; $xe++) {
							if($i->{arf} eq $$format[$xe]->{f}) {
								if($selw == -1 or $selw > $$format[$xe]->{w}) {
									$enc = $i->{encode};
									$arc = $i->{arc};
									$encsel = $xe;
									if($verbose > 5) {
										if($selw > $$format[$xe]->{w}) {
											print STDERR "BETTER MATCH ON $xe\n" ;
										} else {
											print STDERR "MATCH ON $xe\n" ;
										}
									}
									$selw = $$format[$xe]->{w};
								}
							#	last;
							}
						}
					}
					last if($enc != undef);
				}
			}
			if($encsel == -1) {
				print STDERR "$langtable{error}: $fname:$l->{lnum}:",
					" $opname - $langtable{addrunkn}\n$l->{ltxt}\n";
				$errors++;
			}
			my @words;
			if($arc != undef) {
				if($arc =~ /r/) {
					if($l->{reladdr} != 1) { $flagpass++; }
					$l->{reladdr} = 1;
				}
			}
			@words = RunEncoder($enc, @{$$format[$encsel]->{e}});
			print STDERR "OPCE".join('+',@words)."\n" if($verbose > 5);
			my ($avl, $dat, @bytes) = BinSplit(@words);
			$l->{addr} = $vpc;
			$vinstrend = ($avl / $cputable{CPUM});
			$vpc += $vinstrend;
			if($l->{ilen} != $vinstrend) {
				$flagpass++;
			}
			$l->{ilen} = $vinstrend;

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
		} elsif($macro ne '') {
			$format = join(',',@format);
			if($macro eq '.ORG') {
				if(@values > 0) {
					$vpc = 0 + $values[0];
					$l->{addr} = $vpc;
					$labelca = $vpc;
					$labelcv = $vpc;
				}
			} elsif($macro eq '.EQU') {
				if(@values > 0) {
					$labelcv = $values[0];
				}
			} elsif($macro =~ /\.D(AT|[BDW])/) {
				my @lencode;
				if($1 eq 'W') {
					for my $x (@values) {
						if(ref($x) eq 'HASH') {
							my $sv = $x->{encode};
							$sv =~ s/^+ASLM+/+ASLM16+/;
							push @lencode, $sv;
						} else {
							push @lencode, "+ALM16+$x";
						}
					}
				} elsif($1 eq 'D') {
					for my $x (@values) {
						if(ref($x) eq 'HASH') {
							my $sv = $x->{encode};
							$sv =~ s/^+ASLM+/+ASLM32+/;
							push @lencode, $sv;
						} else {
							push @lencode, "+ALM32+$x";
						}
					}
				} else {
					for my $x (@values) {
						if(ref($x) eq 'HASH') {
							my $sv = $x->{encode};
							push @lencode, $sv;
						} else {
							push @lencode, "+ALM+$x";
						}
					}
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
			}
		}
		# process line labels
		if($label ne "") {
			if($symtable{$label}{type} ne "") {
				if($symtable{$label}{val} eq "*" || $symtable{$label}{val} != $labelcv) {
					$symtable{$label}{val} = $labelcv;
					print STDERR "SYMSET: $label $symtable{$label}{val}\n" if($verbose > 3);
					$flagpass++;
				}
				$l->{addr} = $labelca;
			}
		}
		# end process line
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
		@linearg = TokenizeLine($prl,$file);
	for my $r (@linearg) {
		print STDERR "T$r->{tkn} $r->{v} " if($verbose > 5);
		if($r->{tkn} == 2) { $label = $r->{v}; }
		if($r->{tkn} == 3) { $opname = uc($r->{v}); }
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
	print STDERR "Writting flat binary to $outf\n" if($verbose > 0);
	foreach my $l (@allfile) {
		if($vpi == undef) { $vpi = $l->{addr}; }
		if($vpi < $l->{addr}) {
			if($vpi < $l->{addr}) {
				my $align = $l->{addr} - $vpi;
				for(my $i = 0; $i < $align; $i++) { print OBF "\0"; }
				print STDERR "Wrote $align null bytes\n" if($verbose > 2);
				$vpi += $align;
			} else {
				print STDERR "Address break in binary flat file ", sprintf("%08x to %08x",$vpi, $l->{addr}), "\nLine: $l->{lnum}\n" if($verbose > 0);
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
	if($errors > 0) {
		print STDERR "Errors in assembly.\n";
		return;
	}
	$assmpass = 2;
	do {
		$vpc = 0;
		$errors = 0;
		$flagpass = 0;
		print STDERR "Pass $assmpass\n" if($verbose > 0);
		FullParse();
	} while(($assmpass++) < 9 && $flagpass >= 1 && $errors < 1);
	if($errors == 0) {
		print STDERR "Complete!\n" if($verbose > 0);
		WriteListing($outfile);
		WriteFlat($outbinfile);
	} else {
		print STDERR "Errors in assembly.\n";
	}
}
