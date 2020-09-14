"use strict";
function WaveA_Op(opc, mod, fmt, e) {
	this.op = opc;
	if(typeof(mod) == 'string') {
		var sc = mod.indexOf(',');
		if(sc == -1) {
			this.param = parseInt(mod);
			this.madj = 'n'.repeat(mod);
		} else {
			this.param = parseInt(mod.slice(0,sc));
			mod = mod.slice(sc+1);
			if(mod == '') mod = 'n';
			if(mod.length < this.param) mod = mod.repeat(Math.ceil(this.param / mod.length));
			this.madj = mod;
		}
	} else {
		this.param = mod;
		this.madj = 'n'.repeat(mod);
	}
	this.fmt = fmt;
	this.enc = e;
}
function WaveA_Reg(r, n, e) {
	this.reg = r;
	this.name = n;
	this.enc = e;
}
function WaveA_Lit(rl, rh, n, e) {
	this.rl = rl;
	this.rh = rh;
	this.name = n;
	this.enc = e;
}
function WaveA_Token(v,t,eb) {
	this.token = v;
	this.type = t;
	if(eb == undefined) {
		this.exprbreak = true;
	} else {
		this.exprbreak = eb;
	}
}
WaveA_Token.prototype.createSpan = function(z) {
	var r = '<span class="'+this.type+'">';
	if(this.value !== undefined) { r += this.value.replace(/ /g,"&nbsp;"); }
	if(z !== undefined) { r += z; }
	return r+'</span>';
}

class WaveA_EvalError extends Error {
	constructor() {
		super('WaveAsm_EvalError');
	}
}
function WaveA_Eval(f, prio) {
	switch(f) {
	case '+':
		this.fn = function(s) {
			var a,b;
			a = s.pop();
			b = s.pop();
			if(a === undefined || b === undefined)
				throw new WaveA_EvalError;
			s.push(a + b);
		};
		break;
	case '-':
		this.fn = function(s) {
			var a,b;
			a = s.pop();
			b = s.pop();
			if(a === undefined)
				throw new WaveA_EvalError;
			if(b === undefined) {
				s.push(-a);
			} else if(typeof(b) != 'number') {
				s.push(b);
				s.push(-a);
			} else {
				s.push(b - a);
			}
		};
		break;
	default:
		this.fn = f;
	}
	this.prio = prio;
}
var WaveAsm = {
init : function() {
		this.optable = [];
		this.regtable = [];
		this.littable = [];
		this.cputable = [];
		this.macrotable = [".DAT",".DB",".DW",".DD",".DQ",".ORG",".EQU",".RESB",".RESW",".RESD",".RESQ"];
	},
WV_EOL : new WaveA_Token(1,"EOL"),
WV_LABEL : new WaveA_Token(2,"LABEL"),
WV_OPC : new WaveA_Token(3,"OPC"),
WV_IDENT : new WaveA_Token(4,"IDENT", false),
WV_NUMDEC : new WaveA_Token(5,"CONST DEC", false),
WV_NUMHEX : new WaveA_Token(6,"CONST HEX", false),
WV_NUMOCT : new WaveA_Token(7,"CONST OCT", false),
WV_NUMBIN : new WaveA_Token(8,"CONST BIN", false),
WV_REG : new WaveA_Token(9,"REG"),
WV_NUMCHR : new WaveA_Token(10,"CONST CHAR", false),
WV_STRING : new WaveA_Token(11,"STRING"),
WV_MACRO : new WaveA_Token(12,"MACRO"),
WV_PLUS : new WaveA_Token(13,"PUCT PLUS", false),
WV_MINUS : new WaveA_Token(14,"PUCT MINUS", false),
WV_STAR : new WaveA_Token(15,"PUCT STAR", false),
WV_INS : new WaveA_Token(16,"PUCT INS"),
WV_INE : new WaveA_Token(17,"PUCT INE"),
WV_PNS : new WaveA_Token(18,"PUCT PNS"),
WV_PNE : new WaveA_Token(19,"PUCT PNE"),
WV_COMMA : new WaveA_Token(20,"PUCT COMMA"),
WV_WS : new WaveA_Token(21,"WS", false),
WV_IMM : new WaveA_Token(22,"IMM"),
WV_CMNT : new WaveA_Token(23,"CM"),
WV_SL : new WaveA_Token(24,"SLF", false),
WV_BSL : new WaveA_Token(25,"SLB"),
makeToken : function(t,v) {
	var f = Object.create(t);
	if(v !== undefined) {
	f.value = v;
	}
	return f;
},
Loadisf : function(isf) {
		var lines = isf.split(/\n/);
		var t, i;
		var readmode = 0;
		var y, z, w;
		var x, ql = [];
		var rgs;
		var nam, lnv;
		for( z in lines ) {
			y = lines[z];
			if((y.length < 2) || (y.search(/^[ \t]*#/) > -1)) {
				// ignore line
			} else {
				switch(readmode) {
				case 1:
					if(y.toUpperCase() == "</OPCODE>") {
						readmode = 0;
						break;
					}
					// process opcodes
					x = y.split(/:/);
					ql = [this.LineScan(x[3].replace(/[ \t][\t ]*/, " "), false).tokens];
					t = new WaveA_Op(x[0].toUpperCase(), x[1], x[2].substring(1, x[2].length-1), ql);
					this.optable.push(t);
					break;
				case 2:
					if(y.toUpperCase() == "</LIT>") {
						readmode = 0;
						break;
					}
					if(y.charAt(0) == "+") {
						x = y.substr(1).split(/,/);
						for( i in x ) {
							switch(x[i].charAt(0)) {
							case "N":
								nam = x[i].substring(2,x[i].length - 1);
								break;
							case "L":
								lnv = parseInt(x[i].substring(1,x[i].length));
								break;
							}
						}
					} else {
						x = y.split(/:/);
						rgs = x[0].split(/,/);
						ql = [];
						x[1] = "L" + lnv.toString(10) + "+" + x[1];
						for(w = 1; w < x.length; w++) {
							ql.push(this.LineScan(x[w], false).tokens);
						}
						if(x[0] == "*") {
							t = new WaveA_Lit("*","*",nam, ql);
						} else {
							t = new WaveA_Lit(parseInt(rgs[0]), parseInt(rgs[1]), nam, ql);
						}
						this.littable.push(t);
					}
					break;
				case 3:
					if(y.toUpperCase() == "</REG>") {
						readmode = 0;
						break;
					}
					if(y.charAt(0) == "+") {
						x = y.substr(1).split(/,/);
						for( i in x ) {
							if(x[i].charAt(0) == "N") {
								nam = x[i].substring(2,x[i].length - 1);
							}
						}
					} else {
						x = y.split(/:/);
						rgs = x[0].toUpperCase().split(/,/);
						ql = [this.LineScan(x[1], false).tokens];
						for( i in rgs ) {
							t = new WaveA_Reg(rgs[i], nam, ql);
							this.regtable.push(t);
						}
					}
					break;
				case 4:
					if(y.toUpperCase() == "</HEAD>") {
						readmode = 0;
						break;
					} else {
						x = y.split(/:/);
						this.cputable[x[0]] = x[1];
					}
					break;
				default:
					if(y.toUpperCase() == "<OPCODE>") {
						readmode = 1;
					}
					if(y.toUpperCase() == "<LIT>") {
						readmode = 2;
					}
					if(y.toUpperCase() == "<REG>") {
						readmode = 3;
					}
					if(y.toUpperCase() == "<HEAD>") {
						readmode = 4;
					}
					break;
				}
			}
		}
	},
CheckOp : function(o) {
		var i;
		var op = o.toUpperCase();
		for(i in this.optable) {
			if(op == this.optable[i].op) {
				return true;
			}
		}
		return false;
	},
CheckReg : function(o) {
		var i;
		var op = o.toUpperCase();
		for(i in this.regtable) {
			if(op == this.regtable[i].reg) {
				return this.regtable[i];
			}
		}
		return false;
	},
CheckMacro : function(o) {
		var i;
		var mc = o.toUpperCase();
		for(i in this.macrotable) {
			if(mc == this.macrotable[i]) {
				return true;
			}
		}
		return false;
	},
ScanLit : function(v) {
		var i;
		var lt;
		var lr = [];
		for(i in this.littable) {
			lt = this.littable[i];
			if(v == "*" || lt.rl == "*") {
				lr.push(lt);
			} else if(v != "*" && v >= lt.rl && v <= lt.rh) {
				lr.push(lt);
			}
		}
		return lr;
	},
LineScan : function(lt, ids) {
				// FSM line scanner + simple parser
		var i, cc, lc;
		var xs, cpl, oponl, tkn;
		var pr = [];
		var test = "";
		var alp = true; // false = tokenize for encoder
		if(lt == undefined) {
			return {html: test, tokens: pr};
		}
		if(ids != undefined) { alp = ids; }
		cpl = {st:0, en:0, ty:0};
		xs = 0;
		i = 0;
		oponl = 0;
		while(i <= lt.length) {
			if(i == lt.length) {
				cc = "";
			} else {
				cc = lt.charAt(i);
			}
			switch(xs) {
			default:
				if(alp && (cc.search(/[_.%a-zA-Z]/) > -1)) {
					cpl.st = i;
					lc = 0;
					xs = 3;
				} else if(!alp && (cc == '%')) {
					cpl.st = i+1;
					lc = 0;
					xs = 7;
				} else if(!alp && (cc.search(/[a-zA-Z]/) > -1)) {
					cpl.st = i;
					lc = 0;
					xs = 10;
				} else if(cc.search(/[0-9]/) > -1) {
					cpl.st = i;
					lc = 0;
					xs = 4;
					continue;
				} else if(cc == '"') {
					cpl.st = i+1;
					xs = 1;
					lc = 0;
				} else if(cc == '\'') {
					cpl.st = i+1;
					xs = 2;
					lc = 0;
				} else {
					switch(cc) {
					case '\t':
						test += "&nbsp;&nbsp;&nbsp;";
					case ' ':
						test += "&nbsp;";
						if(lc != 2) {
							//test += "WS";
							pr.push(this.makeToken(this.WV_WS));
							lc = 2;
						}
						break;
					case ':':
						if(alp) { xs = 3; }
						cpl.st = i + 1;
						lc = 1;
						break;
					case ';':
						cpl.st = i;
						xs = 9;
						break;
					case ',':
						pr.push(tkn = this.makeToken(this.WV_COMMA));
						test += tkn.createSpan(",");
						break;
					case '+':
						pr.push(tkn = this.makeToken(this.WV_PLUS));
						test += tkn.createSpan("+");
						break;
					case '-':
						pr.push(tkn = this.makeToken(this.WV_MINUS));
						test += tkn.createSpan("-");
						break;
					case '=':
						pr.push(tkn = this.makeToken(this.WV_EQUALS));
						test += tkn.createSpan("=");
						break;
					case '#':
						pr.push(tkn = this.makeToken(this.WV_IMM));
						test += tkn.createSpan("#");
						break;
					case '*':
						pr.push(this.makeToken(this.WV_STAR));
						break;
					case '/':
						pr.push(this.makeToken(this.WV_SL));
						break;
					case '\\':
						pr.push(this.makeToken(this.WV_BSL));
						break;
					case '$':
						test += "$";
						break;
					case '&':
						test += "&amp;";
						break;
					case '!':
						test += "!";
						break;
					case '[':
						pr.push(tkn = this.makeToken(this.WV_INS));
						test += tkn.createSpan("[");
						break;
					case ']':
						pr.push(tkn = this.makeToken(this.WV_INE));
						test += tkn.createSpan("]");
						break;
					case '(':
						pr.push(tkn = this.makeToken(this.WV_PNS));
						test += tkn.createSpan("(");
						break;
					case ')':
						pr.push(tkn = this.makeToken(this.WV_PNE));
						test += tkn.createSpan(")");
						break;
					default:
						test += cc;
					}
				}
				break;
			case 1:
				if((lc == 0) && (cc == '"')) {
					cpl.en = i - 1;
					xs = 0;
					cpl
					cpl.tx = lt.substring(cpl.st, cpl.en+1);
					pr.push(tkn = this.makeToken(this.WV_STRING, cpl.tx));
					test += '"'+tkn.createSpan()+'"';
				} else if(cc == "\\") {
					lc = 1;
				} else {
					lc = 0;
				}
				break;
			case 2:
				if((lc == 0) && (cc == '\'')) {
					cpl.en = i - 1;
					xs = 0;
					cpl.tx = lt.substring(cpl.st, cpl.en+1);
					pr.push(tkn = this.makeToken(this.WV_NUMCHR, cpl.tx));
					test += "'"+tkn.createSpan()+"'";
				} else if(cc == '\\') {
					lc = 1;
				} else {
					lc = 0;
				}
				break;
			case 3:
				if(cc.search(/[_.a-zA-Z0-9]/) > -1) {
					cpl.en = i;
				} else if((lc == 0) && (cc == ':')) {
					xs = 0;
					cpl.en = i - 1;
					cpl.tx = lt.substring(cpl.st, cpl.en+1);
					pr.push(tkn = this.makeToken(this.WV_LABEL, cpl.tx));
					test += tkn.createSpan(":");
				} else {
					xs = 0;
					cpl.en = i - 1;
					if(lc == 1) {
						cpl.tx = lt.substring(cpl.st, cpl.en+1);
						pr.push(tkn = this.makeToken(this.WV_LABEL, cpl.tx));
						test += ":" + tkn.createSpan();
					} else {
						cpl.tx = lt.substring(cpl.st, cpl.en+1);
						if((oponl == 0) && this.CheckOp(cpl.tx)) {
							pr.push(tkn = this.makeToken(this.WV_OPC, cpl.tx));
							test += tkn.createSpan();
							oponl = 1;
						} else if((oponl == 0) && this.CheckMacro(cpl.tx)) {
							pr.push(tkn = this.makeToken(this.WV_MACRO, cpl.tx));
							test += tkn.createSpan();
							oponl = 1;
						} else {
							if((cpl.st == 0)) {
								pr.push(tkn = this.makeToken(this.WV_LABEL, cpl.tx));
								test += tkn.createSpan();
							} else if(this.CheckReg(cpl.tx)) {
								pr.push(tkn = this.makeToken(this.WV_REG, cpl.tx));
								test += tkn.createSpan();
							} else {
								pr.push(tkn = this.makeToken(this.WV_IDENT, cpl.tx));
								test += tkn.createSpan();
							}
						}
					}
					// Rescan needed
					continue;
				}
				break;
			case 4:
				if(lc == 1) {
					if(cc == 'x') {
						xs = 6;
						cpl.st = i+1;
					} else if(cc == 'b') {
						xs = 7;
						cpl.st = i+1;
					} else if(cc.search(/[0-7]/) > -1) {
						xs = 8;
						cpl.st = i;
					} else {
						xs = 0;
						cpl.en = i-1;
						cpl.tx = lt.substring(cpl.st, cpl.en+1);
						// Zero
						pr.push(tkn = this.makeToken(this.WV_NUMDEC, cpl.tx));
						test += tkn.createSpan();
						// Rescan needed
						continue;
					}
					lc = 0;
				} else {
					if(cc == '0') {
						lc = 1;
					} else if(cc.search(/[1-9]/) > -1) {
						xs = 5;
					} else {
						xs = 0;
						cpl.en = i-1;
						cpl.tx = lt.substring(cpl.st, cpl.en+1);
						pr.push(tkn = this.makeToken(this.WV_NUMDEC, cpl.tx));
						test += tkn.createSpan();
						// Rescan needed
						continue;
					}
				}
				break;
			case 5:
				if(cc.search(/[0-9]/) > -1) {
					cpl.en = i;
				} else if(cc == 'h') {
					cpl.en = i-1;
					cpl.tx = lt.substring(cpl.st, cpl.en+1);
					pr.push(tkn = this.makeToken(this.WV_NUMHEX, cpl.tx));
					test += "0x" + tkn.createSpan();
					xs = 0;
				} else {
					xs = 0;
					cpl.en = i-1;
					cpl.tx = lt.substring(cpl.st, cpl.en+1);
					pr.push(tkn = this.makeToken(this.WV_NUMDEC, cpl.tx));
					test += tkn.createSpan();
					// Rescan needed
					continue;
				}
				break;
			case 6:
				if(cc.search(/[0-9a-fA-F]/) > -1) {
					cpl.en = i;
				} else {
					xs = 0;
					cpl.en = i-1;
					cpl.tx = lt.substring(cpl.st, cpl.en+1);
					pr.push(tkn = this.makeToken(this.WV_NUMHEX, cpl.tx));
					test += "0x" + tkn.createSpan();
					// Rescan needed
					continue;
				}
				break;
			case 7: // bin
				if(cc.search(/[01]/) > -1) {
					cpl.en = i;
				} else {
					xs = 0;
					cpl.en = i-1;
					cpl.tx = lt.substring(cpl.st, cpl.en+1);
					pr.push(tkn = this.makeToken(this.WV_NUMBIN, cpl.tx));
					test += "0b" + tkn.createSpan();
					// Rescan needed
					continue;
				}
				break;
			case 8: // oct
				if(cc.search(/[0-7]/) > -1) {
					cpl.en = i;
				} else {
					xs = 0;
					cpl.en = i-1;
					cpl.tx = lt.substring(cpl.st, cpl.en+1);
					pr.push(tkn = this.makeToken(this.WV_NUMOCT, cpl.tx));
					test += tkn.createSpan();
					// Rescan needed
					continue;
				}
				break;
			case 9:
				cpl.en = i;
				if(cc == "") {
					cpl.tx = lt.substring(cpl.st, cpl.en+1);
					pr.push(tkn = this.makeToken(this.WV_CMNT, cpl.tx));
					test += tkn.createSpan();
				}
				break;
			case 10:
				if(cc.search(/[a-zA-Z]/) > -1) {
					cpl.en = i;
				} else {
					xs = 0;
					cpl.en = i - 1;
					cpl.tx = lt.substring(cpl.st, cpl.en+1);
					pr.push(tkn = this.makeToken(this.WV_IDENT, cpl.tx));
					test += tkn.createSpan();
					
					// Rescan needed
					continue;
				}
				break;
			}

			// must be last
			i++;
			if(i > lt.length) {
				pr.push(this.makeToken(this.WV_EOL));
				//test += "EOL\n";
				break;
			}
		}
		return {html: test, tokens: pr};
	},
Encode : function(et, beta, betv) {
		var x, i, rin, cek, cet, wtf;
		var nval, otlen = 0, ota = false, psk = false;
		var rec = false;
		var bin = "";
		var h = "", pf = [], sf = [];
		var hextobin = function(h) {
			var m = parseInt(h,16).toString(2);
			m = "0".repeat((4 - m.length % 4) % 4) + m;
			return m;
		};
		function OTADirect(v) { bin += v; }
		function OTASuffix(v) { sf.push( v ); ota = OTADirect; }
		function BinWrite(v, constrain) {
			if(constrain && otlen > 0) {
				if(v.length < otlen) v = "0".repeat((otlen-v.length % otlen) % otlen) + v;
				if(v.length > otlen) v = v.slice(v.length - otlen);
			}
			ota(v);
		}
		var na = [];
		for(x in et) {
			cek = et[x];
			otlen = 0;
			ota = OTADirect;
		for(i in cek) {
			cet = cek[i];
			if(psk) { psk = false; continue; }
			switch(cet.token) {
			case this.WV_PLUS.token:
				break;
			case this.WV_WS.token:
				break;
			case this.WV_BSL.token:
				rec = true;
				break;
			case this.WV_NUMBIN.token:
				bin += cet.value;
				break;
			case this.WV_IDENT.token:
				rin = parseInt(i) + 1;
				if(((rin) < cek.length)) {
					if(cek[rin].token == this.WV_NUMDEC.token) {
						nval = parseInt(cek[rin].value,10);
					}
				}
				switch(cet.value) {
				case "L":
					otlen = nval;
					psk = true;
					break;
				case "AL":
					otlen = nval;
					ota = OTASuffix;
					psk = true;
				default:
					break;
				}
				break;
			case this.WV_STAR.token:
				if(otlen > 0 && betv < 0) {
					betv = 2**otlen + betv;
				}
				wtf = betv.toString(2);
				BinWrite(wtf, true);
				break;
			case this.WV_NUMDEC.token:
				if(rec) {
					rec = false;
					rin = parseInt(cet.value,10)-1;
					wtf = this.Encode(beta[rin],[],betv[rin]);
					bin += wtf.bin;
					sf = sf.concat(wtf.suf);
					pf = pf.concat(wtf.pfx);
				} else {
					wtf = parseInt(cet.value,10).toString(2);
					BinWrite(wtf, true);
				}
				break;
			case this.WV_NUMHEX.token:
				wtf = cet.value.replace(/[0-9a-fA-f]/g,hextobin);
				bin += wtf;
				break;
			case this.WV_NUMOCT.token:
				break;
			}
		}
		}
		return {bin:bin, pfx:pf, suf:sf, hex:h, na:na};
	},
Assemble : function(sc) {
		var lines = sc.split(/\n/);
		var i;
		var x;
		var ctk;
		var rtm;
		var nval;
		var exps = [];
		var symtable = [];
		var linetable = [];
		var vpc = 0;
		var vps = 0;
		var ltr = 0;
		var otr = 0;
		var mtr = 0;
		var ocp = 0;
		var cop;
		var fmt = "";
		var efmt = [];
		var efvl = [];
		var cp = "";
		var cq = "";
		var pass = 0;
		var fagn = false;
		var ltk;
		var etk;
		var mose;
		var cll = "";
		var cllp = 0;
		var clec = 0;
		var cpul = parseInt(this.cputable["CPUM"]);
		var curopc = undefined;
		function lname(a,c) {
			if(c >= a.length) { c = a.length - 1; }
			return a[c];
		}
		function scanIdent1(a) {
			var q ,w;
			for(q in symtable) {
				w = symtable[q];
				if(w.n == a) {
					return w;
				}
			}
			return false;
		}
		function padBin(b, p) {
			return '0'.repeat((p - b.length % p) % p) + b
		}
		function binToHex(b) {
			return padBin(b, 4).replace(/[01]{4}/g,function(bs){
				return parseInt(bs,2).toString(16);
				//return "0".repeat((2 - m.length % 2) % 2) + m;
			});
		}
		function OpNext(opt) {
			var q;
			q = otr;
			while(q < opt.length) {
				if(opt[q].op == cop) {
					otr = q;
					curopc = opt[otr];
					return otr;
				} else {
					q++;
				}
			}
			return otr = q;
		}
		for(i in lines) {
			ltk = this.LineScan(lines[i]);
			var linenum = parseInt(i) + 1;
			cp = ltk.html;
			cq = "";
			for(x in ltk.tokens) {
				ctk = ltk.tokens[x];
				switch(ctk.token) {
					case this.WV_MACRO.token:
					otr = -2;
					break;
				case this.WV_LABEL.token:
					rtm = scanIdent1(ctk.value);
					if(rtm) {
						if(linenum != rtm.l) {
						cq += "--ERROR LABEL DUP ("+linenum +","+ rtm.l+")--";
						clec += 1;
						nval = rtm.v;
						}
					} else {
						symtable.push({n:ctk.value,v:"*",l:linenum});
						nval = "*";
					}
					break;
				case this.WV_IDENT.token:
					if(cll.length == 0 || cllp == 0) { cll = ctk.value; cllp = 1; }
					rtm = scanIdent1(ctk.value);
					if(rtm) {
						nval = rtm.v;
					} else {
						nval = "*";
					}
				case this.WV_NUMHEX.token:
				case this.WV_NUMDEC.token:
				case this.WV_NUMBIN.token:
				case this.WV_NUMOCT.token:
				case this.WV_NUMCHR.token:
					switch(ctk.token) {
					case this.WV_NUMHEX.token:
						nval = parseInt(ctk.value,16); break;
					case this.WV_NUMDEC.token:
						nval = parseInt(ctk.value,10); break;
					case this.WV_NUMBIN.token:
						nval = parseInt(ctk.value,2); break;
					case this.WV_NUMOCT.token:
						nval = parseInt(ctk.value,8); break;
					case this.WV_NUMCHR.token:
						nval = ctk.value.charCodeAt(0);
						if(nval == 92) {
							nval = ctk.value.charCodeAt(1);
							switch(nval) {
								case 116:
									nval = 9;
									break;
								case 114:
									nval = 13;
									break;
								case 110:
									nval = 10;
									break;
								case 32:
									nval = 0;
									break;
							}
						}
						break;
					}
					rtm = false;
					if(exps.length == 0) {
						//fmt += ctk.value.toString(10);
						rtm = this.ScanLit(nval);
					} else if(exps.length == 1 && exps[0].token == this.WV_MINUS.token){
						nval = -nval;
						exps.pop();
						//fmt += ctk.value.toString(10);
						rtm = this.ScanLit(nval);
					} else {
						exps.push(ctk);
					}
					break;
				}
			}
			linetable.push({n:linenum, t:ltk.tokens, ec:0, a:0, l:0, o:'', x:cq, xt:cp});
		}
		cp = undefined;
		//
		fagn = true;
		for(pass = 1; (pass < 7) && fagn; pass++) {
		fagn = false;
		vpc = 0;
		var linelabels = [];
		for(i in linetable) {
			var curline = linetable[i];
			var linenum = parseInt(i) + 1;
			curopc = undefined;
			cop = undefined;
			var clmacro = undefined;
			ltk = curline.t;
			vps = 0;
			cq = "|";
			otr = -1;
			clec = 0;
			if(curline.ec > 0) {
				continue;
			}
			fmt = [];
			exps.length = 0;
			efmt.length = 0; efvl.length = 0;
ventest:	for(x in ltk) {
				ctk = ltk[x];
				if(ctk.exprbreak && exps.length > 0) {
					nval = 0;
					var exprop = undefined;
					var exprstack = [];
					var exprcmd = [];
					for(var k in exps) {
						if(typeof(exps[k]) == 'number') {
							exprstack.push(exps[k]);
							if(exprcmd.length > 0) {
								exprstack.push(exprcmd.pop());
							}
						} else {
							switch(exps[k].token) {
							case this.WV_MINUS.token:
								exprcmd.push(new WaveA_Eval('-'));
								break;
							case this.WV_PLUS.token:
								exprcmd.push(new WaveA_Eval('+'));
								break;
							case this.WV_STAR.token:
								exprcmd.push(new WaveA_Eval('*'));
								break;
							}
						}
					}
					var evallist = exprstack;
					exprstack = [];
					while(evallist.length > 0) {
						var e = evallist.shift();
						if(e instanceof WaveA_Eval) {
							e.fn(exprstack);
						} else {
							exprstack.push(e);
						}
					}
					if(exprstack.length != 1 || exprcmd.length > 0) {
						cq += "--EXPR ERROR--";
					}
					nval = exprstack.pop();
					fmt.push(new WaveA_Eval('u', efvl.length));
					efvl.push(nval);
					exps.length = 0;
				}
				switch(ctk.token) {
				case this.WV_MACRO.token:
					clmacro = ctk.value;
					otr = -2;
					break;
				case this.WV_OPC.token:
					// resolve labels here and now
					for(var k in linelabels) {
						linelabels[k].v = vpc;
						cq += "LL:"+linelabels[k].v.toString(16)+"\\";
					}
					linelabels.length = 0;

					cop = ctk.value.toUpperCase();
					if(otr < 0) { otr = 0; }
					OpNext(this.optable);
					break;
				case this.WV_LABEL.token:
					var linelabel = scanIdent1(ctk.value);
					if(linelabel) linelabels.push(linelabel);
					break;
				case this.WV_IDENT.token:
					rtm = scanIdent1(ctk.value);
					if(rtm) {
						nval = rtm.v;
					} else {
						if(ctk.value.search(/^[0-9A-Fa-f]+h$/) > -1) {
							cq += "-AHC-";
							nval = parseInt(ctk.value.substr(0,ctk.value.length-1),16);
						} else {
							cq += "--ERROR NO IDENT--";
							clec += 1;
							break ventest;
						}
					}
					// ident is a value, no break
				case this.WV_NUMHEX.token:
				case this.WV_NUMDEC.token:
				case this.WV_NUMBIN.token:
				case this.WV_NUMOCT.token:
				case this.WV_NUMCHR.token:
					switch(ctk.token) {
					case this.WV_NUMHEX.token:
						nval = parseInt(ctk.value,16); break;
					case this.WV_NUMDEC.token:
						nval = parseInt(ctk.value,10); break;
					case this.WV_NUMBIN.token:
						nval = parseInt(ctk.value,2); break;
					case this.WV_NUMOCT.token:
						nval = parseInt(ctk.value,8); break;
					case this.WV_NUMCHR.token:
						nval = ctk.value.charCodeAt(0);
						if(nval == 92) {
							nval = ctk.value.charCodeAt(1);
							switch(nval) {
								case 116:
									nval = 9;
									break;
								case 114:
									nval = 13;
									break;
								case 110:
									nval = 10;
									break;
								case 32:
									nval = 0;
									break;
							}
						}
						break;
					}
					if(exps.length == 1 && exps[0].token == this.WV_MINUS.token){
						nval = -nval;
						exps.pop();
						exps.push(nval);
					} else {
						if(nval == '*') nval = 0;
						exps.push(nval);
					}
					break;
				case this.WV_REG.token:
					rtm = this.CheckReg(ctk.value);
					fmt.push(rtm);
					efvl.push(0);
					break;
				case this.WV_PLUS.token:
					exps.push(ctk);
					break;
				case this.WV_MINUS.token:
					exps.push(ctk);
					break;
				case this.WV_IMM.token:
					fmt.push("#");
					break;
				case this.WV_COMMA.token:
					fmt.push(",");
					break;
				case this.WV_INS.token:
					fmt.push("[");
					break;
				case this.WV_INE.token:
					fmt.push("]");
					break;
				case this.WV_PNS.token:
					fmt.push("(");
					break;
				case this.WV_PNE.token:
					fmt.push(")");
					break;
				case this.WV_WS.token:
				case this.WV_EOL.token:
				case this.WV_CMNT.token:
					break;
				default:
					fmt.push("?");
					break;
				}
			}
			var shex = '';
			var testfmt = '';
			var curfmt = '';
			if(otr > -1) {
				mtr = fmt.length;
				var efvlindex = 0;
				var fmtindex = new Array(mtr).fill(0);
				ltr = 0;
				while(ltr <= mtr) {
					if(ltr < mtr) {
						var cfmt = fmt[ltr];
						var vfmtlen = 0;
						if(typeof(cfmt) == 'string') {
							curfmt += cfmt;
							ltr++;
							continue;
						} else if(cfmt instanceof Array) {
							if(cfmt.length == 0) {
								if(pass > 1) cq += "--SYN ERROR--";
								break;
							}
							testfmt = cfmt[fmtindex[ltr]].name;
							vfmtlen = cfmt.length;
						} else if(cfmt instanceof WaveA_Eval) {
							if(fmtindex[ltr] == 0) {
								efvlindex = cfmt.prio;
								if(curopc.madj[efvlindex] == 'r') {
									cfmt.v = efvl[efvlindex] - (vpc + curline.l);
								} else {
									cfmt.v = efvl[efvlindex];
								}
								rtm = this.ScanLit(cfmt.v);
								if(rtm) {
									if(rtm.length == 0) {
										if(pass > 1) {
											cq += "--RANGE ERROR--";
										}
										ltr++;
										continue;
									}
									cfmt.a = rtm;
								} else {
									cq += "--VALUE ERROR--";
								}
							}
							testfmt = cfmt.a[fmtindex[ltr]].name;
							vfmtlen = cfmt.a.length;
						} else {
							testfmt = cfmt.name;
						}
					}
					if(curfmt + testfmt == curopc.fmt && ltr+1 >= mtr) {
						efmt.length = 0;
						if(cq.includes('ERROR')) {
							cq = cq.replace('ERROR', 'WARN');
						}
						for(i in fmt) {
							if(fmt[i] instanceof Array) {
								efmt.push(fmt[i][fmtindex[i]].enc);
							} else if(fmt[i] instanceof WaveA_Eval) {
								efmt.push(fmt[i].a[fmtindex[i]].enc);
							} else if(typeof(fmt[i]) == 'object') {
								efmt.push(fmt[i].enc);
							}
						}
						for(i in fmt) {
							if(fmt[i] instanceof WaveA_Eval) {
								efvlindex = fmt[i].prio;
								efvl[efvlindex] = fmt[i].v;
							}
						}
						etk = this.Encode(curopc.enc,efmt,efvl);
						shex = etk.pfx.join("") + etk.bin + etk.suf.join("");
						vps = Math.ceil(shex.length / cpul);
						shex = binToHex(padBin(shex, cpul));
						cq += shex;
						break;
					} else if(ltr < mtr && curopc.fmt.startsWith(curfmt + testfmt)) {
						curfmt += testfmt;
						testfmt = '';
					} else if(ltr < mtr && fmtindex[ltr] + 1 < vfmtlen && !curopc.fmt.startsWith(curfmt + testfmt)) {
						fmtindex[ltr]++;
						continue;
					} else {
						if(ltr == mtr) {
							fmtindex.fill(0);
							curfmt = '';
							otr++;
							OpNext(this.optable);
							if(otr < this.optable.length) {
								ltr = 0;
								continue;
							} else {
								if(pass > 1) cq += "--OP PARAM ERROR--" + curfmt + testfmt;
								clec += 1;
								break;
							}
						}
					}
					ltr++;
				}
			} else if(otr == -2) {
				function resolveLabels() {
					for(var k in linelabels) {
						linelabels[k].v = vpc;
						cq += "MLL:"+linelabels[k].v.toString(16)+"\\";
					}
					linelabels.length = 0;
				}
				function MacroArgError(n) {
					if(n === undefined) {
						this.message = "__MACRO ARG ERROR__";
					} else {
						this.message = "__MACRO ARG ERROR:"+n+"__"
					}
				}
				function macroMinArg(n) {
					if(efvl.length < n) {
						throw new MacroArgError();
					}
				}
				function macroNumArg(n) {
					if(efvl.length != n) {
						throw new MacroArgError("expects "+n+" got "+efvl.length);
					}
				}
				try {
					switch(clmacro.toUpperCase()) {
					case ".ORG":
						macroNumArg(1);
						vpc = efvl[0];
						break;
					case ".EQU":
						macroNumArg(1);
						for(var k in linelabels) {
							linelabels[k].v = efvl[0];
							cq += "EQU:"+linelabels[k].v.toString(16)+"\\";
						}
						linelabels.length = 0;
						break;
					case ".RESB":
						resolveLabels();
						macroMinArg(1);
						if(efvl[0] < 0) { cq += "--MACRO ERROR--"; break; }
						nval = 0;
						if(efvl.length > 1) nval = efvl[1];
						shex = padBin(nval.toString(2), cpul);
						shex = binToHex(shex.repeat(efvl[0]));
						vps = Math.ceil((shex.length * 4) / cpul);
						break;
					case ".DB":
						resolveLabels();
						macroMinArg(1);
						shex = ''
						for(var k in efvl) {
							nval = efvl[k];
							var tmphex = binToHex(padBin(nval.toString(2), cpul));
							vps += Math.ceil((tmphex.length * 4) / cpul);
							shex += tmphex;
						}
						break;
					case ".DW":
						resolveLabels();
						macroMinArg(1);
						shex = ''
						for(var k in efvl) {
							nval = efvl[k];
							var tmphex = binToHex(padBin(nval.toString(2), cpul*2));
							vps += Math.ceil((tmphex.length * 4) / cpul);
							shex += tmphex;
						}
						break;
					}
				} catch(e) {
					cq += e.message;
				}
			}
			if(curline.a != vpc) {
				curline.a = vpc;
				fagn = true;
			}
			curline.o = shex;
			vpc += vps;
			if(curline.l != vps) {
				curline.l = vps;
				fagn = true;
			}
			if((otr > -1) && (otr < this.optable.length)) {
				cq += " :" + curopc.op + " " + curopc.fmt + "=" + curfmt+testfmt;
			}
			curline.x = cq;
		}
		}
		//
		cq = "";
		var rpro = [];
		var eline = {a:0, v:''};
		function genResultLine() {
			eline.sa = binToHex(padBin(eline.a.toString(2), 16));
			var chk = 0;
			var elt = (eline.sa + eline.v);
			var ell = binToHex(padBin(((elt.length >> 1) + 1).toString(2), 8));
			(ell + elt).match(/[0-9A-Fa-f]{2}/g).forEach(function(v){
				chk += parseInt(v, 16);
			});
			chk = (chk & 0xff) ^ 0xff;
			eline.slen = ell;
			eline.chk = chk;
			eline.schk = binToHex(padBin(chk.toString(2), 8));
			rpro.push(eline);
			eline = {a:0, v:''};
		}
		vpc = undefined;
		for(i in linetable) {
			var cline = linetable[i];
			if(vpc === undefined) {
				vpc = cline.a;
			}
			if(cline.a != vpc) {
				genResultLine();
				vpc = cline.a;
			}
			if(eline.v.length == 0) {
				eline.a = cline.a;
			}
			vpc += cline.o.length >> 1;
			eline.v += cline.o;
			if(eline.v.length >= 48) {
				genResultLine();
			}
			cq += "<div";
			if(cline.x.search(/ERR/) > -1) {
				cq += " class=\"ERR\"";
			}
			cq += "><span class=\"INSTR\">" + cline.xt +"</span><span class=\"asmsg\">"+ cline.x + "</span></div>";
		}
		if(eline.v.length > 0) {
			genResultLine();
		}
		return {html:cq, program:rpro };
	}
};
