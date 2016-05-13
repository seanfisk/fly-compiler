/* -*- mode: c++ -*- */

/* NOTE: Flex will not accept C++-style (//) comments. Only C-style comments are allowed! */

/* Definition section */

%{
/* C++ code to be copied verbatim */
#include "parser.hpp"
#include "driver.hpp"

#include <iostream>

/* The location of the current token. */
static fly::Parser::location_type loc;

%}

/*
	These options tell flex not to generate these functions.
	Otherwise, when using -Wall, clang++ and g++ will emit warnings about unused functions.
*/
%option noinput
%option nounput
%option noyywrap

/* Line number tracking (%option lineno) is not necessary because we do that on our own. */

%option warn
%option batch
%option debug

/* Definitions */
ident [!@%^&<>=.a-zA-Z0-9_]+
integer -?[0-9]+
blank [ \t]

%{
/* Code run each time a pattern is matched. */
#define YY_USER_ACTION  loc.columns(yyleng);
%}

%%
 /* Rules section */

%{
	/* Code run each time yylex is called. */
  loc.step();
%}

{blank}+ loc.step();
[\n]+ loc.lines(yyleng); loc.step();
"(" return fly::Parser::make_LPAREN(loc);
")" return fly::Parser::make_RPAREN(loc);
{integer} return fly::Parser::make_INTEGER(yytext, loc);
{ident} return fly::Parser::make_IDENT(yytext, loc);
. driver.error(loc, "Unknown token: `" + std::string(yytext) + "'");
<<EOF>> return fly::Parser::make_END(loc);

%%
/* C++ code section */

void fly::Driver::lex_begin()
{
	yy_flex_debug = trace_lexing;
	if (input_path == "-") {
		yyin = stdin;
		// This isn't a valid filename, but it's nicer to see in errors than "-".
		input_path = "<stdin>";
	}
	else if (!(yyin = fopen(input_path.c_str(), "r")))
	{
		error("cannot open " + input_path.native() + ": " + strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void fly::Driver::lex_end()
{
	fclose(yyin);
}
