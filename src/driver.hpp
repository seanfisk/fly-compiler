// Most of this is adapted from Bison's Calc++ example. Thanks, Bison, for having such great example code!

#ifndef _DRIVER_HPP
#define _DRIVER_HPP

#include "parser.hpp"

#include <boost/filesystem.hpp>

#include <string>

// Tell Flex the lexer's prototype ...
#define YY_DECL paralisp::Parser::symbol_type yylex(paralisp::Driver &driver)
// ... and declare it for the parser's sake.
YY_DECL;

namespace paralisp {
	// Conduct the whole lexing and parsing process.
	class Driver {
		// The top level root node of our final AST.
		paralisp::NBlock root_node;
	public:
		Driver();
		virtual ~Driver();

		int result;
		// Handling the lexer.
		void lex_begin();
		void lex_end();
		paralisp::NBlock get_root() const;
		void set_root(const paralisp::NBlock &root_node);
		void set_trace_parsing(bool trace_parsing);
		void set_trace_lexing(bool trace_lexing);
		bool trace_lexing;
		// Run the parser on file F.
		// Return 0 on success.
		int parse(const boost::filesystem::path &input_path);
		// The name of the file being parsed.
		// Used later to pass the file name to the location tracker.
		boost::filesystem::path input_path;
		// Whether parser traces should be generated.
		bool trace_parsing;
		// Error handling.
		void error(const paralisp::Parser::location_type &loc, const std::string &msg);
		void error(const std::string &msg);
	};
}
#endif
