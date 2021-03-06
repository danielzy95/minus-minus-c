%option noyywrap
%option yylineno

%x SINGLE_COMMENT
%x MULTI_COMMENT

%{
    #include <stdio.h>
    #include "tokens.h"

    extern char *yytext;
    extern int yylineno;

    int column = 0;

    void count();
%}

DIGIT [0-9]
OCT [0-7]
HEX [0-9a-fA-F]
LETTER [a-zA-Z_]
IGNORE [ \n\t\v\f]

%%

"char" { count(); return KW_CHAR; }
"int"  { count(); return KW_INT; }
"void" { count(); return KW_VOID; }

"break"    { count(); return KW_BREAK; }
"continue" { count(); return KW_CONTINUE; }
"else"     { count(); return KW_ELSE; }
"for"      { count(); return KW_FOR; }
"if"       { count(); return KW_IF; }
"return"   { count(); return KW_RETURN; }
"while"    { count(); return KW_WHILE; }

"sizeof" { count(); return KW_SIZEOF; }

"time(0)" { count(); return KW_TIME; }


"++" { count(); return OP_INCREMENT; }
"--" { count(); return OP_DECREMENT; }

"+"  { count(); return '+'; }
"-"  { count(); return '-'; }

"*"  { count(); return '*'; }
"/"  { count(); return '/'; }
"%"  { count(); return '%'; }

"="   { count(); yylval.char_ptr = strdup(yytext); return '='; }
"+="  { count(); yylval.char_ptr = strdup(yytext); return OP_PLUSEQ; }
"-="  { count(); yylval.char_ptr = strdup(yytext); return OP_MINUSEQ; }
"*="  { count(); yylval.char_ptr = strdup(yytext); return OP_TIMESEQ; }
"/="  { count(); yylval.char_ptr = strdup(yytext); return OP_DIVEQ; }
"%="  { count(); yylval.char_ptr = strdup(yytext); return OP_MODEQ; }
"&="  { count(); yylval.char_ptr = strdup(yytext); return OP_ANDEQ; }
"|="  { count(); yylval.char_ptr = strdup(yytext); return OP_OREQ; }
"^="  { count(); yylval.char_ptr = strdup(yytext); return OP_XOREQ; }
">>=" { count(); yylval.char_ptr = strdup(yytext); return OP_SRLEQ; }
"<<=" { count(); yylval.char_ptr = strdup(yytext); return OP_SLLEQ; }

"==" { count(); return OP_EQ; }
"!=" { count(); return OP_NOTEQ; }

"&"  { count(); return '&'; }
"|"  { count(); return '|'; }
"^"  { count(); return '^'; }
"~"  { count(); return '~'; }
">>" { count(); return OP_SRL; }
"<<" { count(); return OP_SLL; }

"&&" { count(); return OP_CAND; }
"||" { count(); return OP_COR; }
"!"  { count(); return '!'; }

">"  { count(); return '>'; }
"<"  { count(); return '<'; }
">=" { count(); return OP_GEQ; }
"<=" { count(); return OP_LEQ; }

"?" { count(); return '?'; }


{LETTER}({LETTER}|{DIGIT})* { count(); yylval.char_ptr = strdup(yytext); return ID; }


0[xX]{HEX}+       { count(); yylval.char_ptr = strdup(yytext); return HEX_INT; }
0{OCT}+           { count(); yylval.char_ptr = strdup(yytext); return OCT_INT; }
(0|[1-9]{DIGIT}*) { count(); yylval.char_ptr = strdup(yytext); return DEC_INT; }


'(\\.|[^\\'])+'   { count(); yylval.char_ptr = strdup(yytext); return CHAR_LIT; }
\"(\\.|[^\\"])*\" { count(); yylval.char_ptr = strdup(yytext); return STRING_LIT; }


"//" { BEGIN(SINGLE_COMMENT); }
"/*" { BEGIN(MULTI_COMMENT); }


"("  { count(); return '('; }
")"  { count(); return ')'; }
"{"  { count(); return '{'; }
"}"  { count(); return '}'; }
"["  { count(); return '['; }
"]"  { count(); return ']'; }
","  { count(); return ','; }
";"  { count(); return ';'; }
":"  { count(); return ':'; }


{IGNORE} { count(); }


. { count(); printf("Lexer Error[%d:%d]: %s\n", yylineno, column + 1, yytext); }

<SINGLE_COMMENT>[^\n] { count(); }
<SINGLE_COMMENT>\n { count(); BEGIN(INITIAL); }

<MULTI_COMMENT><<EOF>> { 
    printf("Lexer Error[%d:%d]: You must close the multiline comment.\n", yylineno, column + 1); 
    yyterminate();
}
<MULTI_COMMENT>.|\n { count(); }
<MULTI_COMMENT>"*/" { count(); BEGIN(INITIAL); }

%%

void count() {
    int i;

	for (i = 0; yytext[i]; i++)
		if (yytext[i] == '\n')
			column = 0;
		else if (yytext[i] == '\t')
			column += 8 - (column % 8);
		else
			column++;
}