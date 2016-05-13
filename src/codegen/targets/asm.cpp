#include "codegen/targets.hpp"

#include <string>
#include <iostream>

#include <llvm/PassManager.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h> // for target initialization
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/Host.h> // for llvm::sys::getHostCPUName() and llvm::sys::getDefaultTargetTriple()
#include <llvm/ADT/Triple.h>
#include <llvm/Support/FileSystem.h>


using namespace llvm;

namespace fs = boost::filesystem;

void paralisp::print_native_asm(paralisp::PLContext &context, const fs::path &output_path) {
	// This code is based on the code for the `llc' command, the LLVM static compiler.

	// IMPORTANT: We must initialize the printer before targets are available. If we don't do this, we will fail out on addPassesToEmitFile.
	InitializeNativeTargetAsmPrinter();

	PassManager pm;
	std::string output_file_error_info;
	raw_fd_ostream output_stream(output_path.c_str(), output_file_error_info, sys::fs::F_Text);
	formatted_raw_ostream formatted_output_stream(output_stream);

	// Get the closest target for this host. llc allows a bunch of customization, but we don't really care about that.
	std::string host_triple_string(sys::getDefaultTargetTriple()); // Get the default triple.
	std::string target_error_message;
	const Target *host_target = TargetRegistry::lookupTarget(host_triple_string, target_error_message);

	if (host_target == NULL) {
		std::cerr << target_error_message << std::endl;
		std::exit(1);
	}

	// Create a target machine based upon host defaults.
	Triple host_triple(host_triple_string);
	std::string host_cpu = sys::getHostCPUName(); // Get the default CPU name.
	TargetOptions target_options; // Just leave at default.
	SubtargetFeatures host_features;
	host_features.getDefaultSubtargetFeatures(host_triple); // Populate host_features with defaults (usually empty).
	TargetMachine *target_machine = host_target->createTargetMachine(host_triple.str(), host_cpu, host_features.getString(), target_options);

	if (target_machine == NULL) {
		std::cerr << "Could not allocate the target machine!" << std::endl;
		std::exit(1);
	}

	// Confusingly, the 4th argument to addPassesToEmitFile is whether to *disable* verification. We want verification, though, so we have to pass `false'.
	if (target_machine->addPassesToEmitFile(pm, formatted_output_stream, TargetMachine::CGFT_AssemblyFile, false)) {
		std::cerr << "Target does not support generation of native assembler!" << std::endl;
		std::exit(1);
	}

	pm.run(*context.get_module());
}
