#ifndef _TARGETS_HPP
#define _TARGETS_HPP

#include "codegen/plcontext.hpp"
#include "cmdline.hpp"

#include <boost/filesystem.hpp>

namespace paralisp {
	void dump_ir(PLContext &context);
	void print_ir(PLContext &context, const boost::filesystem::path &output_path);
	void print_native_asm(PLContext &context, const boost::filesystem::path &output_path);
	void compile_executable(PLContext &context, const boost::filesystem::path &output_path, const LibrarySearchPaths &lib_search_paths);
}


#endif
