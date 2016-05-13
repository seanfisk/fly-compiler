#include "codegen/targets.hpp"

#include <iostream>
#include <string>

#include <llvm/PassManager.h>
#include "llvm/IR/IRPrintingPasses.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>

using namespace llvm;

namespace fs = boost::filesystem;

void fly::dump_ir(fly::PLContext &context) {
	// Print the bytecode in a human-readable format to see if our program compiled properly.
	std::cout << "Code is generated." << std::endl;
	PassManager pm;
	pm.add(createPrintModulePass(outs()));
	pm.run(*context.get_module());
}

void fly::print_ir(fly::PLContext &context, const fs::path &output_path) {
	std::string output_file_error_info;
	raw_fd_ostream output_stream(output_path.c_str(), output_file_error_info, sys::fs::F_Text);
	// The 2nd argument is an (AssemblyAnnotationWriter *), which is apparently optional.
	context.get_module()->print(output_stream, NULL);
	output_stream.close();
}
