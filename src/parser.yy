// -*- mode: c++ -*-

// Definition section

// Use the C++ skeleton
%skeleton "lalr1.cc"
// Require Bison version 3.0 (especially since the C++ skeleton has changed from previous versions)
%require "3.0"
%defines
// Aggregate semantic value, token type, and location.
%define api.token.constructor
// Use a variant instead of union.
%define api.value.type variant
// Assert that C++ variants are being used correctly.
%define parse.assert
// Redefine parser namespace and class name.
%define api.namespace {paralisp}
%define parser_class_name {Parser}
// Allow tracing of the parser.
%define parse.trace
// Report verbose error messages.
%define parse.error verbose

%code requires
{
	#include "nodes.hpp"
	#include "codegen/visitors.hpp"
	namespace paralisp {
		class Driver;
	}
}

%param { paralisp::Driver& driver }
%locations
// This code was in the Bison Calc++ example. However, this doesn't do anything because the locations get overwritten. The solution is to assign the filename right before the error is printed.
// %initial-action
// {
// 	// Initialize the initial location.
// 	@$.begin.filename = @$.end.filename = &driver.filename;
// };

%code
{
	// C++ code to be copied verbatim
	#include "driver.hpp"
	#include "data_types.hpp"

	#include <boost/lexical_cast.hpp>
	#include <boost/variant/apply_visitor.hpp>
}

%define api.token.prefix {T}

// Define terminal symbols.
// 1st column: type (optional)
// 2nd column: token name (should match flex file)
// 3rd column: numeric code (optional)
// 4th column: human-readable name that will be printed in errors (optional)
// See more by running this: info bison 'Token Decl'
%token <std::string> INTEGER "integer"
%token <std::string> IDENT "identifier"
%token LPAREN "("
%token RPAREN ")"
%token END 0 "end of file"

// Define the type of node our nonterminal symbols represent.
%type <paralisp::SexpVariant> sexp atom
%type <paralisp::NList> list
%type <paralisp::SexpList> sexp_list
%type <paralisp::NInteger> numeric
%type <paralisp::NIdent> ident

%printer { yyoutput << '"' << $$ << '"'; } <std::string>;
%printer {
	// We must create the visitor a priori and not pass it inline otherwise it will be `const'.
	PrintVisitor visitor(yyoutput);
	boost::apply_visitor(visitor, $$);
} <paralisp::SexpVariant>;
%printer { yyoutput << "list of size " << $$.size(); } sexp_list;
%printer {
	// We must create the visitor a priori and not pass it inline otherwise it will be `const'.
	PrintVisitor visitor(yyoutput);
	// We need to *create* a variant in order to apply the visitor.
	paralisp::SexpVariant variant = $$;
	boost::apply_visitor(visitor, variant);
} list numeric ident;


%start program

%%
 // Rules section

 // Prefer left-recursive rules. See more by running this on the command-line: info bison Recursion

program:
	%empty
| sexp_list { driver.set_root(paralisp::NBlock($1)); }
;

sexp_list:
	sexp { $$ = paralisp::SexpList(); $$.push_back($1); }
| sexp_list sexp { $1.push_back($2); $$ = $1; }
;

sexp:
	atom { $$ = $1; }
| list { $$ = $1; }
;

list:
	LPAREN sexp_list RPAREN { $$ = paralisp::NList($2); }
| LPAREN RPAREN { paralisp::SexpList sexp_list; $$ = paralisp::NList(sexp_list); }
;

atom:
	numeric { $$ = $1; }
| ident { $$ = $1; }
;

numeric:
	INTEGER { $$ = paralisp::NInteger(boost::lexical_cast<PLInt>($1.c_str())); }
;

ident:
	IDENT { $$ = paralisp::NIdent($1); }
;

%%
// C++ code section

void paralisp::Parser::error(const paralisp::Parser::location_type& loc, const std::string &msg) {
	driver.error(loc, msg);
}
