#include "driver.hpp"
#include "parser.hpp"

using fly::Driver;
namespace fs = boost::filesystem;

Driver::Driver()
	: trace_lexing(false), trace_parsing(false) {
}

Driver::~Driver() {
}

int Driver::parse(const fs::path &input_path) {
	this->input_path = input_path;
	lex_begin();
	fly::Parser parser(*this);
	parser.set_debug_level(trace_parsing);
	int res = parser.parse();
	lex_end();
	return res;
}

fly::NBlock Driver::get_root() const {
	return root_node;
}

void Driver::set_root(const fly::NBlock &root_node) {
	this->root_node = root_node;
}

void Driver::set_trace_parsing(bool trace_parsing) {
	this->trace_parsing = trace_parsing;
}

void Driver::set_trace_lexing(bool trace_lexing) {
	this->trace_lexing = trace_lexing;
}

void Driver::error(const fly::Parser::location_type &loc, const std::string &msg) {
	fly::Parser::location_type loc_with_file(loc);
	// The location objects want a pointer to a non-const std::string. Make a copy to get around it.
	std::string input_path_copy = input_path.native();
	loc_with_file.begin.filename = loc_with_file.end.filename = &input_path_copy;
	std::cerr << loc_with_file << ": " << msg << std::endl;
	std::exit(1);
}

void Driver::error(const std::string &msg) {
	std::cerr << msg << std::endl;
	std::exit(1);
}
