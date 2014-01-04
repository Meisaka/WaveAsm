
%{
int yyFlexLexer::yywrap(void) { return 1; }
int paren_layer = 0;
int inde_layer = 0;
#ifndef EOF
#define EOF -1
#endif
%}

%option never-interactive
%option 8bit
%option nounistd

hex [0-9a-fA-F]
digit [0-9]
letter  [a-zA-Z]
uletter  [_a-zA-Z]
uscore  [_]
opletter [_.a-zA-Z]
idletter [_%a-zA-Z]
esquot  \\\"
notquot [^"\n]
%x argmode
%x comment

%%

({uletter})({uletter}|{digit})*":" std::cerr << "LABEL \"" << yytext << "\"\n";
^({opletter})({opletter}|{digit})*  std::cerr << "AMBLABEL \"" << yytext << "\"\n";
[.]{opletter}* std::cerr << "MACR \"" << yytext << "\" "; BEGIN(argmode);
{opletter}* std::cerr << "OPER \"" << yytext << "\" "; BEGIN(argmode);
[ \t] ;
<argmode>{
["]({notquot}|{esquot})*["]  std::cerr << "STRING:" << yytext << ":";
['](([^'])|(\\.))[']  std::cerr << "CHAR" << yytext;
({idletter})({uletter}|{digit})*  std::cerr << "IDENT \"" << yytext << "\" ";
([$]|0x){hex}* |
{hex}*h  std::cerr << "HEX " << yytext << ' ';
{digit}* std::cerr << "DEC " << yytext << ' ';
"[" inde_layer++; std::cerr << "INDS ";
"]" %{ if(inde_layer > 0) { std::cerr << "INDE "; inde_layer--; } else { std::cerr << "!!INDERR!!"; } %}
"(" { paren_layer++; std::cerr << "PARENS "; }
")" %{ if(paren_layer > 0) { std::cerr << "PARENE "; paren_layer--; } else { std::cerr << "!!PARENERR!!"; } %}
"," { std::cerr << "COMMA "; }
"#" { std::cerr << "IMME "; }
[ \t]
"+" { std::cerr << "PLUS "; }
"-" { std::cerr << "MINUS "; }
}
<*>[\n\r] %{ std::cerr << "EOL\n";
 if(inde_layer) { std::cerr << "!!INDERR!!"; inde_layer=0; }
 if(paren_layer) { std::cerr << "!!PARENERR!!"; paren_layer=0; }
 BEGIN(0); %}
<*>";" std::cerr << "COMM"; BEGIN(comment);
<comment>.* ;
%%
