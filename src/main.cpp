#include "cmdline.hpp"
#include "codegen/targets.hpp"
#include "nodes.hpp"
#include "driver.hpp"
#include "data_types.hpp"

#include <iostream>
#include <string>

#include <boost/variant/get.hpp>
#include <boost/program_options.hpp>
// There is a choice here between using Boost.Filesystem or llvm::sys::fs and llvm::sys:path. We already require both libraries, so it's not like we're adding a dependency with either. The LLVM implementation is based on Boost, so it has a similar API. However, the LLVM implementation uses error codes instead of exceptions. Boost can also use error codes, but in my humble opinion exceptions are A Good Thing(TM).
#include <boost/filesystem.hpp>

#include <llvm/Support/ManagedStatic.h>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

// When this object is destructed, internal LLVM structures will be deallocated.
// See <http://lists.cs.uiuc.edu/pipermail/llvmdev/2007-December/011631.html>.
// Another approach is llvm_shutdown(), shown later.
llvm::llvm_shutdown_obj llvm_cleaner;

int main(int argc, char *argv[]) {
	po::variables_map args = paralisp::parse_args(argc, argv);

	paralisp::Driver driver;
	driver.set_trace_lexing(args.count("trace-lexing") > 0);
	driver.set_trace_parsing(args.count("trace-parsing") > 0);
	int result = driver.parse(args["input-file"].as<fs::path>());

	if (result == 1) {
		std::cerr << "Parsing failed." << std::endl;
		std::exit(1);
	}

	paralisp::PLContext context;
	paralisp::NBlock root_node = driver.get_root();
	context.generate_code(root_node);

	if (args.count("asm-only")) {
		fs::path output_path;

		if (args.count("output-file")) {
			output_path = args["output-file"].as<fs::path>().native();
		} else {
			// Instead of placing the output file in the same directory as the input file, we place it in the current directory. This is consistent with the behavior of `clang -S /path/to/myfile.c'.
			fs::path input_path(args["input-file"].as<fs::path>());
			output_path = input_path.filename();

			if (args.count("emit-llvm")) {
				output_path.replace_extension("ll");
			} else {
				output_path.replace_extension("s");
			}
		}

		if (args.count("emit-llvm")) {
			paralisp::print_ir(context, output_path);
		} else {
			paralisp::print_native_asm(context, output_path);
		}
	} else if (args.count("emit-llvm")) {
		std::cerr << "Option `--emit-llvm' may only be used with `--asm-only'." << std::endl;
		return 1;
	} else {
		const fs::path output_path = args.count("output-file") > 0 ? args["output-file"].as<fs::path>() : "a.out";

		if (args.count("lib-path")) {
			paralisp::compile_executable(context, output_path, args["lib-path"].as<paralisp::LibrarySearchPaths>());
		} else {
			paralisp::LibrarySearchPaths lib_search_paths;
			paralisp::compile_executable(context, output_path, lib_search_paths);
		}
	}

	// This is another way to "shut down" LLVM and deallocate internal structures.
	// llvm::llvm_shutdown();

	return 0;
}
