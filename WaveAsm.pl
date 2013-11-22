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
my $vinstrend = 0;
my $errors = 0;
my $flagpass = 0;
# CPU specs (these are replaced in loaded file)
my %cputable = (ISN => "NONE", FILEW => 8, CPUW => 8, CPUM => 8, CPUE => "LE", FILEE => "LE", ALIGN => 0);

# build-in instructions
my @optable = (
	{op => '.ORG', arc => 1, arf => '*', encode => 'M'},
	{op => '.EQU', arc => 1, arf => '*', encode => 'M'},
	{op => '.DAT', arc => -1, arf => '*', encode => 'M'},
	{op => '.DB', arc => -1, arf => '*', encode => 'M'},
	{op => '.DW', arc => -1, arf => '*', encode => 'M'},
	{op => '.DD', arc => -1, arf => '*', encode => 'M'}
);

print STDERR "Wave Asm - version 0.1.4\n";
LoadInstructions( $instructionsetfile );
print STDERR "<optable>\n";
foreach my $o (@optable) {
	print STDERR $o->{op} . " ";
}
print STDERR "\n<regtable>\n";
foreach my $o (@regtable) {
	print STDERR "$o->{reg} $o->{nam} $o->{encode} ";
}
foreach(@ARGV) {
	if(/^--/) {
	} elsif (/^-/) {
	} else {
		@allfile = (); # clear file
		%symtable = ();
		Assemble( $_, ($_ . ".lst") , ($_ . ".bin"));
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
			if(/^\W*#/ or /\</) {
			} elsif(/:/) {
				my ($k, $v) = split(':', $_);
				$cputable{$k} = $v;
			}
		} elsif(/<REG>/ .. /<\/REG>/) {
			if(/^\W*#/ or /\</) {
			} elsif(/^\+/) {
				#print STDERR "AttribSet: $_\n";
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
					#print STDERR "REG: $_ " . $creg[1]  . "\n";
					push @regtable, ({reg => $_, nam => $att{nam}, encode => $creg[1]});
				}
			}
		} elsif(/<LIT>/ .. /<\/LIT>/) {
			if(/^\W*#/ or /\</) {
				next;
			} elsif(/^\+/) {
				#print STDERR "AttribSet: $_\n";
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
				#print STDERR "+NAME=$att{nam} +LEN=$att{len}\n";
			} else {
				my ($range, @encode) = split(':', $_);
				my ($rl,$ru) = split(',', $range);
				($rl,$ru) = ('*','*') if($range eq '*');
				for my $i (@encode) {
					$i = "L$att{len}*" if($i eq '*');
				}
				#print STDERR "LIT: $att{nam} $range $att{len} " . join(':',@encode) . "\n";
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
				#print STDERR "OPCODE: $opname $fcode\n";
				push @optable, {op => $opname, arc => $opparam[1], arf => $fcode, encode => $opparam[3]};
			}
		} elsif(/^[\t ]*#/) {
			#print STDERR $_;
		}
	}
	close ISF;
}

sub DecodeSymbol {
	my ($sym, $itr, $rel) = @_;
	return ("null", undef) if(!defined($sym));
	return ("null", undef) if($sym eq "");
	my $v;
	my $itab = @littable;
	my $ci = 0;
	for my $r (@regtable) {
		if(uc($r->{reg}) eq uc($sym)) {
			return ("reg", $r);
		}
	}
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
	
  if($sym =~ /^(-*)(0[xX][0-9a-fA-F]+)$/) { # Hexadecimal 0x...
		$v = oct($2);
		$v = -$v if(length($1) % 2 == 1);
		if($itr == -1) {
			return ("val", undef, $itab, $v);
		}
		$v -= ($vpc + $vinstrend) if($rel == 1);
		for my $r (@littable) {
			if($ci >= $itr) {
				if($r->{rl} eq '*') {
					return ("lit", $r, $itab, $v);
				} elsif(($r->{rl} <= $v) && ($r->{ru} >= $v)) {
					return ("lit", $r, $itab, $v);
				}
			} else {
				$ci++;
			}
		}

	} elsif($sym =~ /^(-*)(0[bB][01]+)$/) { # Binary 0b...
		$v = oct($2);
		$v = -$v if(length($1) % 2 == 1);
		if($itr == -1) {
			return ("val", undef, $itab, $v);
		}
		$v -= ($vpc + $vinstrend) if($rel == 1);
		for my $r (@littable) {
			if($ci >= $itr) {
				if($r->{rl} eq '*') {
					return ("lit", $r, $itab, $v);
				} elsif(($r->{rl} <= $v) && ($r->{ru} >= $v)) {
					return ("lit", $r, $itab, $v);
				}
			} else {
				$ci++;
			}
		}

	} elsif($sym =~ /^(-*)([0-9a-fA-F]+)([hH])$/) { # Hexadecimal ...h
		$v = oct("0x" . $2);
		$v = -$v if(length($1) % 2 == 1);
		if($itr == -1) {
			return ("val", undef, $itab, $v);
		}
		$v -= ($vpc + $vinstrend) if($rel == 1);
		for my $r (@littable) {
			if($ci >= $itr) {
				if($r->{rl} eq '*') {
					return ("lit", $r, $itab, $v);
				} elsif(($r->{rl} <= $v) && ($r->{ru} >= $v)) {
					return ("lit", $r, $itab, $v);
				}
			} else {
				$ci++;
			}
		}

	} elsif($sym =~ /^(-*)([0-9]+)$/) { # decimal
		$v = ($2);
		$v = -$v if(length($1) % 2 == 1);
		if($itr == -1) {
			return ("val", undef, $itab, $v);
		}
		$v -= ($vpc + $vinstrend) if($rel == 1);
		for my $r (@littable) {
			if($ci >= $itr) {
				#print STDERR "LL: $v [ $r->{rl} $r->{ru} ] ";
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
	return ("null",undef);
}

sub DecodeSymbols {
	my ($allsym, $itr, $arc, $wordsz) = @_;
	my $format;
	my @format = ();
	my $maxitr = 0;
	my @decarg = split(",",$allsym);
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
	for my $arg (@decarg) {
		#print STDERR "DECARG: '$arg' ";
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
				my ($type, $vec, $i, $v) = DecodeSymbol($minus . $elem, $itr,$userel);
				if($type eq "reg") {
					push @lfs, $vec->{nam};
					push @encode, $vec->{encode};
				} elsif($type eq "lit") {
					push @lfs, $vec->{nam};
					push @encode, $vec->{encode} ."+$v";
					$maxitr = $i;
				} elsif($type eq "val") {
					push @encode, "+ALM$ewordsz+$v";
				} else {
					push @lfs, "*";
					push @encode, "+$v";
					$maxitr = $i;
				}
				$minus = "";
			} else {
				my ($type, $vec, $i, $v) = DecodeSymbol($minus . $elem, $itr, $userel);
				if($type eq "reg") {
					push @lfs, $vec->{nam};
					push @encode, $vec->{encode};
				} elsif($type eq "lit") {
					push @lfs, $vec->{nam};
					push @encode, $vec->{encode}. "+$v";
					$maxitr = $i;
				} elsif($type eq "val") {
					push @encode, "+ALM$ewordsz+$v";
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
				#print STDERR "ENC-STL: $1\n";
				$otlen = $1;
			} elsif($es =~ /^MR([BN]*)$/) {
				$otrel = 1;
			} elsif($es =~ /\+/) {
				foreach my $k (split('\+', $es)) {
					next if($k eq "");
					if($k =~ /^L([0-9]+)/) {
						#print STDERR "ENC-STL: $1\n";
						$otlen = $1 + 0;
					} elsif($k =~ /^AL([0-9]+)/) {
						#print STDERR "ENC-ASL: $1\n";
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
					} elsif($k =~ /^0x([0-9a-fA-F]+)$/) {
						my $tl = length($1) * 4;
						my $v = oct("0x$1");
						my $ins = sprintf("%0${tl}b", $v);
						$ins = substr($ins,-$otlen,$otlen) if($otlen != 0);
						$ins = PadWord($ins) if($otlv != 0);
						#print STDERR "ENC-ACL $ins\n";
						if($otapp == 0) {
						$output .= $ins;
						} else {
						push @output, $ins;
						$otapp = 0;
						}
					} elsif($k =~ /^%([01]+)$/ ) {
						my $x = $1;
						$x = substr($x,-$otlen,$otlen) if($otlen != 0);
						$x = PadWord($x) if($otlv != 0);
						if($otapp == 0) {
							$output .= $x;
						} else {
						push @output, $x;
						$otapp = 0;
						#print STDERR "ENC-ACL\n";
						}
					} elsif($k =~ /\\([0-9]+)/ ) {
						my $x = $1 - 1;
						#print STDERR "ENC-Rec: $1 $extra[$x]\n";
						my ($ins, @apd) = RunEncoder("+".$extra[$x]);
						if($otapp == 0) {
						$output .= $ins;
						} else {
						push @output, $ins;
						$otapp = 0;
						#print STDERR "ENC-ACL\n";
						}
						#print STDERR "ENC-Res: $ins " . join('',@apd) . "\n";
						push @output, @apd;
					} elsif($k =~ /^(-*[0-9]+)$/) {
						my $x = $1 + 0;
						my $o = sprintf("%0${otlen}b",$x);
						$o = substr($o,-$otlen,$otlen) if($otlen != 0);
						$o = PadWord($o) if($otlv != 0);
						#print STDERR "ENC-int: $x\n";
						if($otapp == 0) {
						$output .= $o;
						} else {
						push @output, $o;
						$otapp = 0;
						#print STDERR "ENC-ACL\n";
						}
					} elsif($k eq '*') {
						my $x = sprintf("%0${otlen}b",0);
						#print STDERR "ENC-fpw: $x\n";
						if($otapp == 0) {
						$output .= $x;
						} else {
						push @output, $x;
						$otapp = 0;
						#print STDERR "ENC-ACL\n";
						}
					} else {
						#print STDERR "ENC-unk: '$k'\n";
					}
				}
			}
		}
		unshift @output,$output if($output ne '');
	}
	return @output;
}

sub Pass2 {
	for my $l (@allfile) {
		my ($label,$opname,$linearg) = ($l->{label},$l->{op},$l->{arg});
		$opname = uc $opname;
		$vinstrend = $l->{ilen};
		print STDERR join("\t",($l->{lnum},sprintf("%08x",$vpc),$label,$opname,$linearg)) . "\t" ;
		if($label ne '' && $opname ne '.EQU') {
			#print STDERR "SYMGEN: $label ".$symtable{$label}{type}."\n";
			if($symtable{$label}{type} ne "") {
				if($symtable{$label}{val} eq "*" || $symtable{$label}{val} != $vpc) {
					$symtable{$label}{val} = $vpc;
					$l->{addr} = $vpc;
					#print STDERR "SYMSET: $label $symtable{$label}{val}\n";
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
			#print STDERR "ITR: $itr $found $addrmode\n";
			} while($found != 0 && $itr < $maxitr && $addrmode == 0);
			$itr--;
		} else {
			$found = -1; #no opcode on line
			$addrmode = -1;
			print STDERR "\n" if($label ne '');
		}
		if($found == 0) {
			print STDERR "$langtable{error}: $langtable{line} $l->{lnum}:",
			" $opname - $langtable{opunkn}\n$l->{ltxt}\n";
			$errors++;
		} else {
			#print STDERR "INT: $found$addrmode $format\n";
			if($addrmode == 0) {
				print STDERR "$langtable{error}: $langtable{line} $l->{lnum}:",
					" $opname - $langtable{addrunkn}\n$l->{ltxt}\n";
				$errors++;
			} elsif($enc eq 'M') {
				if($opname eq '.ORG') {
					my ($type, $vec, $i, $v) = DecodeSymbol($linearg,-1);
					if($type eq "val") {
						$vpc = $v;
					}
					# rerun line labels
					if($label ne '') {
					if($symtable{$label}{type} ne "") {
					if($symtable{$label}{val} eq "*" || $symtable{$label}{val} != $vpc) {
						$symtable{$label}{val} = $vpc;
						#print STDERR "RRSYM: $vpc\n";
						$flagpass++;
					}
					}
					}
				} elsif($opname eq '.EQU') {
					my ($type, $vec, $i, $v) = DecodeSymbol($linearg,-1);
					if($type eq "val") {
					if($label ne '') {
					if($symtable{$label}{val} != $v) {
						#print STDERR "RRSYM: $symtable{$label}{val} $v\n";
						$symtable{$label}{val} = $v;
						$flagpass++;
					}
					}
					}

				} elsif($opname =~ /^.D(AT|[BDW])$/) {
					if($1 eq 'W') {
					($lformat, undef, @lencode) = DecodeSymbols($linearg, -1, $arc, 16);
					} elsif($1 eq 'D') {
					($lformat, undef, @lencode) = DecodeSymbols($linearg, -1, $arc, 32);
					} else {
					($lformat, undef, @lencode) = DecodeSymbols($linearg, -1, $arc);
					}
					#print STDERR "\n".join(' ', @lencode) ;
					my @words;
					@words = RunEncoder(join(' ', @lencode));
					#print STDERR "\n".join(' ', @words) . "\n";
					my ($avl, $dat, @bytes) = BinSplit(@words);
					my $txtbyte = join(' ', @bytes);
					print STDERR $txtbyte . "\n";
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
			#$encode = join('||',@encode);
			#print STDERR "\n$format \"$encode\" $enc";
			#$encode = join('||',@lencode);
			#print STDERR "\n$lformat \"$encode\" $enc\n";
			@words = RunEncoder($enc, @lencode);
			my ($lavl, $ldat, @lbytes) = BinSplit(@words);
			if($lavl != $avl) {
				#print STDERR "AVC: $avl $lavl\n";
				$flagpass++;
				my $txtbyte = join('', @lbytes);
				print STDERR $txtbyte . "\n";
				$l->{byte} = $txtbyte;
				$l->{dat} = $ldat;
			} else {
				my $txtbyte = join('', @bytes);
				print STDERR $txtbyte . "\n";
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

sub Assemble {
	my ($file, $outfile, $outbinfile) = @_;
	my $assmpass = 0;
	unless( open(ISF, "<", $file) ) {
		print STDERR $langtable{fileof1} . $file . $langtable{fileof2} . "\n";
		return;
	}
	unless( open(OSF, ">", $outfile) ) {
		print STDERR $langtable{fileof1} . $outfile . $langtable{fileof2} . "\n";
		close ISF;
		return;
	}
	unless( open(OBF, ">", $outbinfile) ) {
		print STDERR $langtable{fileof1} . $outbinfile . $langtable{fileof2} . "\n";
		close ISF;
		close OSF;
		return;
	} else {
		binmode OBF;
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
		print STDERR join("\t",($.,$label,$opname,$linearg,$enc)) ;
		print STDERR "\n";
	}
	$assmpass = 2;
	do {
	$vpc = 0;
	$errors = 0;
	$flagpass = 0;
	print STDERR "Pass $assmpass\n";
	Pass2();
	} while(($assmpass++) < 9 && $flagpass >= 1);
	if($errors == 0) {
		print STDERR "Complete!\n";
		foreach my $l (@allfile) {
		my ($label,$opname,$linearg) = ($l->{label},$l->{op},$l->{arg});
		print OSF join("\t",($l->{lnum},sprintf("%08x",$l->{addr}),$label,$opname,$linearg),$l->{byte}) . "\n" ;
			print OBF $l->{dat};
		}
		print OSF "Symbols:\n";
		foreach my $l (keys %symtable) {
			printf OSF "%08x  %s\n",$symtable{$l}{val},$l
		}
	}
	close ISF;
	close OSF;
	close OBF;
}
