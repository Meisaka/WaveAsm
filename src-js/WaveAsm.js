function WaveA_Op(opc, mod, fmt, e) {
	this.op = opc;
	this.param = mod;
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
function WaveA_Token(v,t) {
	this.token = v;
	this.type = t;
	this.createSpan = function(z) {
		var r = '<span class="'+t+'">';
		if(this.value !== undefined) { r += this.value.replace(/ /g,"&nbsp;"); }
		if(z !== undefined) { r += z; }
		return r+'</span>';
	}
}
var WaveAsm = {
init : function() {
		this.optable = [];
		this.regtable = [];
		this.littable = [];
		this.cputable = [];
		this.macrotable = [".DAT",".DB",".DW",".DD",".ORG"];
	},
WV_EOL : new WaveA_Token(1,"EOL"),
WV_LABEL : new WaveA_Token(2,"LABEL"),
WV_OPC : new WaveA_Token(3,"OPC"),
WV_IDENT : new WaveA_Token(4,"IDENT"),
WV_NUMDEC : new WaveA_Token(5,"CONST DEC"),
WV_NUMHEX : new WaveA_Token(6,"CONST HEX"),
WV_NUMOCT : new WaveA_Token(7,"CONST OCT"),
WV_NUMBIN : new WaveA_Token(8,"CONST BIN"),
WV_REG : new WaveA_Token(9,"REG"),
WV_NUMCHR : new WaveA_Token(10,"CONST CHAR"),
WV_STRING : new WaveA_Token(11,"STRING"),
WV_MACRO : new WaveA_Token(12,"MACRO"),
WV_PLUS : new WaveA_Token(13,"PUCT PLUS"),
WV_MINUS : new WaveA_Token(14,"PUCT MINUS"),
WV_STAR : new WaveA_Token(15,"PUCT STAR"),
WV_INS : new WaveA_Token(16,"PUCT INS"),
WV_INE : new WaveA_Token(17,"PUCT INE"),
WV_PNS : new WaveA_Token(18,"PUCT PNS"),
WV_PNE : new WaveA_Token(19,"PUCT PNE"),
WV_COMMA : new WaveA_Token(20,"PUCT COMMA"),
WV_WS : new WaveA_Token(21,""),
WV_IMM : new WaveA_Token(22,"IMM"),
WV_CMNT : new WaveA_Token(23,"CM"),
WV_SL : new WaveA_Token(24,"SLF"),
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
		var y, z;
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
						if(x[0] == "*") {
							ql = [];
							x.shift();
							while(x.length > 0) {
								ql.push(this.LineScan(x[0], false).tokens);
								x.shift();
							}
							t = new WaveA_Lit("*","*",nam, ql);
						} else {
							ql = [];
							x.shift();
							while(x.length > 0) {
								ql.push(this.LineScan(x[0], false).tokens);
								x.shift();
							}
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
			if(lt.rl == "*") {
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
						test += tkn.createSpan("IMM ");
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
						test += "VEC ";
						break;
					case '&':
						test += "AMP ";
						break;
					case '!':
						test += "BANG ";
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
					} else if(cc == 'b') {
						xs = 7;
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
					cpl.en = i;
					cpl.tx = lt.substring(cpl.st, cpl.en+1);
					pr.push(tkn = this.makeToken(this.WV_NUMHEX, cpl.tx));
					test += tkn.createSpan();
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
					test += tkn.createSpan();
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
					test += tkn.createSpan();
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
				test += "EOL\n";
				break;
			}
		}
		return {html: test, tokens: pr};
	},
Encode : function(et, beta) {
		var x, i, cek, cet, wtf;
		var rec = false;
		var bin = "";
		var h = "", pf = "", sf = "";
		var na = [];
		for(x in et) {
			cek = et[x];
		for(i in cek) {
			cet = cek[i];
			switch(cet.token) {
			case this.WV_PLUS.token:
				break;
			case this.WV_WS.token:
				bin += " ";
				break;
			case this.WV_BSL.token:
				rec = true;
				break;
			case this.WV_NUMBIN.token:
				bin += cet.value;
				break;
			case this.WV_IDENT.token:
				break;
			case this.WV_NUMDEC.token:
				if(rec) {
					rec = false;
					wtf = this.Encode(beta[parseInt(cet.value,10)-1],[]);
					bin += wtf.bin;
					sf += wtf.suf;
					pf += wtf.pfx;
				}
				break;
			case this.WV_NUMHEX.token:
				bin += cet.value;
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
		var cp = "";
		var cq = "";
		var pass = 0;
		var fagn = false;
		var ltk;
		var etk;
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
		function OpNext(opt) {
			var q;
			q = otr;
			while(q < opt.length) {
				if(opt[q].op == cop) {
					return otr = q;
				} else {
					q++;
				}
			}
		}
		for(i in lines) {
			ltk = this.LineScan(lines[i]);
			vps = 0;
			cp += "<div>" + ltk.html + "</div>";
			cq = "|";
			ltr = 0;
			otr = -1;
			mtr = 0;
			while(ltr <= mtr) {
			fmt = ""
			for(x in ltk.tokens) {
				ctk = ltk.tokens[x];
				switch(ctk.token) {
				case this.WV_OPC.token:
					cop = ctk.value.toUpperCase();
					if(otr < 0) { otr = 0; }
					OpNext(this.optable);
					break;
				case this.WV_LABEL.token:
					rtm = scanIdent1(ctk.value);
					if(rtm) {
						cq += "--ERROR LABEL DUP ("+i+1 +","+ rtm.l+")--";
						nval = rtm.v;
					} else {
						symtable.push({n:ctk.value,v:"*",l:i+1});
						nval = "*";
					}
					break;
				case this.WV_IDENT.token:
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
					if(rtm) {
						fmt += lname(rtm, ltr).name;
						if(mtr < rtm.length) { mtr = rtm.length; }
					}
					break;
				case this.WV_REG.token:
					rtm = this.CheckReg(ctk.value);
					fmt += rtm.name;
					break;
				case this.WV_PLUS.token:
					fmt += "+";
				case this.WV_MINUS.token:
					exps.push(ctk);
					break;
				case this.WV_COMMA.token:
					fmt += ",";
					break;
				case this.WV_INS.token:
					fmt += "[";
					break;
				case this.WV_INE.token:
					fmt += "]";
					break;
				case this.WV_PNS.token:
					fmt += "(";
					break;
				case this.WV_PNE.token:
					fmt += ")";
					break;
				}
			}
			if(otr > -1) {
			if(fmt == this.optable[otr].fmt) {
				cq += "-" + ltr + "-OK-";
				etk = this.Encode(this.optable[otr].enc,[]);
				cq += etk.pfx + etk.bin + etk.suf;
				break;
			} else {
				if(ltr == mtr) {
					otr++;
					OpNext(this.optable);
					if(otr < this.optable.length) {
						ltr = 0;
						continue;
					} else {
						cq += "--OP ERROR--";
						break;
					}
				}
			}
			}
			ltr++;
			}
			linetable.push({n:i+1, t:ltk.tokens, a:vpc, l:vps, x:cq});
			vpc += vps;
		}
		//
		fagn = true;
		for(pass = 1; (pass < 7) && fagn; pass++) {
		fagn = false;
		for(i in linetable) {
			ltk = linetable[i].t;
			vps = 0;
			cq = "|";
			ltr = 0;
			otr = -1;
			mtr = 0;
ventest:	while(ltr <= mtr) {
			fmt = "";
			efmt.length = 0;
			for(x in ltk) {
				ctk = ltk[x];
				switch(ctk.token) {
				case this.WV_OPC.token:
					cop = ctk.value.toUpperCase();
					if(otr < 0) { otr = 0; }
					OpNext(this.optable);
					break;
				case this.WV_IDENT.token:
					rtm = scanIdent1(ctk.value);
					if(rtm) {
						nval = rtm.v;
					} else {
						cq += "--ERROR NO IDENT--";
						mtr = 0;
						break ventest;
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
					if(rtm) {
						fmt += lname(rtm, ltr).name;
						efmt.push(lname(rtm, ltr).enc);
						if(mtr < rtm.length) { mtr = rtm.length; }
					}
					break;
				case this.WV_REG.token:
					rtm = this.CheckReg(ctk.value);
					fmt += rtm.name;
					efmt.push(rtm.enc);
					break;
				case this.WV_PLUS.token:
					fmt += "+";
				case this.WV_MINUS.token:
					exps.push(ctk);
					break;
				case this.WV_COMMA.token:
					fmt += ",";
					break;
				case this.WV_INS.token:
					fmt += "[";
					break;
				case this.WV_INE.token:
					fmt += "]";
					break;
				case this.WV_PNS.token:
					fmt += "(";
					break;
				case this.WV_PNE.token:
					fmt += ")";
					break;
				}
			}
			if(otr > -1) {
			if(fmt == this.optable[otr].fmt) {
				cq += "-" + ltr + "-OK-";
				etk = this.Encode(this.optable[otr].enc,efmt);
				cq += etk.pfx + etk.bin + etk.suf;
				break;
			} else {
				if(ltr == mtr) {
					otr++;
					OpNext(this.optable);
					if(otr < this.optable.length) {
						ltr = 0;
						continue;
					} else {
						cq += "--OP ERROR--"+ fmt;
						break;
					}
				}
			}
			}
			ltr++;
			}
			linetable[i].a = vps;
			vpc += vps;
			if(linetable[i].l != vps) {
				linetable[i].l = vps;
				fagn = true;
			}
			if((otr > -1) && (otr < this.optable.length)) {
				cq += " " + otr + ":" + this.optable[otr].op + " " + this.optable[otr].fmt + "=" + fmt;
			}
			//linetable.push({n:i+1, t:ltk.tokens, a:vpc, l:vps});
			linetable[i].x += cq;
		}
		}
		//
		cq = "";
		for(i in linetable) {
			cq += "<div>" + linetable[i].x + "</div>";
		}
		return {html:cp, bhtml:cq };
	}
};
