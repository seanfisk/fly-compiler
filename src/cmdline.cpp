#include "cmdline.hpp"
#include "data_types.hpp"

#include <sstream>
#include <iostream>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

static std::string version_info() {
	return "Paralisp 0.1";
}

static std::string generate_usage(const char *program_name, const po::options_description &options) {
	std::ostringstream ss;
	ss << "Usage: " << program_name << " [options] [INPUT_FILE]\n\n" <<
	   "Paralisp compiler\n" <<
	   options << "\n" <<
	   version_info() << "\n" <<
	   "Written by Sean Fisk <sean@seanfisk.com>\n" <<
	   "Advised by Dr. Greg Wolffe";
	return ss.str();
}

po::variables_map paralisp::parse_args(int argc, char *argv[]) {
	po::options_description generic_options("Generic options");
	generic_options.add_options()
	("help,h", "show this help")
	("version,V", "print version info")
	;
	po::options_description io_options("I/O options");
	io_options.add_options()
	("input-file,i", po::value<fs::path>()->default_value(fs::path("-")), "input source file")
	("output-file,o", po::value<fs::path>(), "output file")
	;
	po::options_description compilation_options("Compilation options");
	compilation_options.add_options()
	// composing() tells program options that successive values should be merged.
	("lib-path,L", po::value<LibrarySearchPaths>()->composing(), "library search path (passed to clang with -L)")
	("asm-only,S", "output assembler instead of an executable, defaults to native asm")
	("emit-llvm,R", "output LLVM IR instead of native asm")
	("parallel,P", "run map operations in parallel")
	("enable-map-profiling", "profile the map operation")
	("num-threads,n", po::value<paralisp::PLInt>()->default_value(-1),
	 "number of threads on which to map, default is num processors at *runtime*")
	;
	po::options_description debug_options("Debug options");
	debug_options.add_options()
	("trace-lexing", "show lexer trace info")
	("trace-parsing", "show parser trace info")
	;
	po::options_description cmdline_options;
	cmdline_options.add(generic_options).add(io_options).add(compilation_options).add(debug_options);

	po::positional_options_description positional_options;
	positional_options.add("input-file", 1);

	po::variables_map args;
	po::command_line_parser parser(argc, argv);

	try {
		po::store(parser.options(cmdline_options).positional(positional_options).run(), args);

		if (args.count("help")) {
			std::cout << generate_usage(argv[0], cmdline_options) << std::endl;
			std::exit(EXIT_SUCCESS);
		} else if (args.count("version")) {
			std::cout << version_info() << std::endl;
			std::exit(EXIT_SUCCESS);
		}
	} catch (po::error &e) {
		std::cerr << e.what() << "\n\n" << generate_usage(argv[0], cmdline_options) << std::endl;
		std::exit(EXIT_FAILURE);
	}

	return args;
}
