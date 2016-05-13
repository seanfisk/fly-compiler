#include "codegen/targets.hpp"
#include "utils.hpp"

#include <string>
#include <iostream>

#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>

#include <llvm/Support/Program.h> // for FindProgramByName()
#include <llvm/Support/Host.h> // for llvm::sys::getDefaultTargetTriple()
#include <llvm/Support/raw_ostream.h> // for llvm::errs()

#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>

using namespace llvm;

namespace fs = boost::filesystem;

void paralisp::compile_executable(paralisp::PLContext &context, const fs::path &output_path, const LibrarySearchPaths &lib_search_paths) {
	// Major credit here goes to <http://fdiv.net/2012/08/15/compiling-code-clang-api>
	// Also see (cfe/lib/Frontend/CreateInvocationFromCommandLine.cpp)

	// First produce the native assembler.

	fs::path assembly_path = make_safe_temp_file("paralisp-%%%%-%%%%-%%%%-%%%%.s");
	paralisp::print_native_asm(context, assembly_path);

	// Now use the Clang driver to compile to an executable. Clang will take care of calling the native assembler and linker for us.

	// Path to clang (e.g. /usr/local/bin/clang)
#if LLVM_VERSION_MAJOR <= 3 && LLVM_VERSION_MINOR <= 3
	// LLVM 3.3 and below (not sure how far below)
	std::string clang_path = llvm::sys::Program::FindProgramByName("clang++").str();
#else
	// LLVM 3.4 and newer
	std::string clang_path = llvm::sys::FindProgramByName("clang++");
#endif

	// Arguments to pass to the clang driver:
	//	clang getinmemory.c -lcurl -v
	std::vector<std::string> args;
	args.push_back(clang_path);

	// Enable verbose output.
	// One would think that clangDriver.CCCEcho would do it, but no.
	args.push_back("-v");

	// Add inputs.
	args.push_back(assembly_path.native());

	// Include library search paths, primarily used for finding libparalisp.a.
	BOOST_FOREACH(const fs::path & lib_search_path, lib_search_paths) {
		args.push_back("-L");
		args.push_back(lib_search_path.native());
	}

	// Link to libparalisp.a, the paralisp standard library.
	args.push_back("-lparalisp");

	// Also link with LLVM'S libc++. This has to be hard-coded because we are not invoking Clang in a way (even though we are using clang++) where it would think that we are linking C++ code.
	// Note that this doesn't work, at least on my Mac OS 10.9, with `-lstdc++' (GCC's libstdc++). Only `-lc++' (LLVM's libc++) seems to work.
	args.push_back("-lc++");

	// Add output file name.
	args.push_back("-o");
	args.push_back(output_path.native());

	std::cout << "Calling clang with command-line: " << boost::algorithm::join(args, " ") << std::endl;

	// The Clang driver needs a DiagnosticsEngine so it can report problems.

	// TODO: These allocations seems really questionable. I'm not quite sure what's happening here pointer-wise.
	clang::DiagnosticOptions *diag_options = new clang::DiagnosticOptions();
	clang::TextDiagnosticPrinter *diag_client = new clang::TextDiagnosticPrinter(llvm::errs(), diag_options);
	// We use an IntrusiveRefCntPtr because the DiagnosticsEngine constructor does.
	clang::IntrusiveRefCntPtr<clang::DiagnosticIDs> diag_id(new clang::DiagnosticIDs());
	clang::DiagnosticsEngine diags(diag_id, diag_options, diag_client);

	// Create the clang driver
	clang::driver::Driver clang_driver(clang_path.c_str(), llvm::sys::getDefaultTargetTriple(), diags);
	// This is the default output file name (e.g., 'a.out').
	clang_driver.DefaultImageName = output_path.c_str();

	// This would turn on g++-like behavior. We DON'T want this because it activates linking with libstdc++.
	// clangDriver.CCCIsCXX = true;

	// Create the set of actions to perform.
	clang::driver::Compilation *compilation = clang_driver.BuildCompilation(to_c_str_vector(args));

	// Print the set of actions.
	clang_driver.PrintActions(*compilation);

	// This is really quite confusing to declare. I stole it from the Clang tree (cfe/tools/driver/driver.cpp).
	llvm::SmallVector<std::pair<int, const clang::driver::Command *>, 4> failing_commands;

	// Execute the actions. The return value is not very reliable because commands can fail and it will still be reported as 0. However, the command *will* be added to failingCommands. To understand how it works, you really have to take a look at the source (cfe/tools/driver/driver.cpp).
	int compilation_retval = clang_driver.ExecuteCompilation(*compilation, failing_commands);

	// Report any problems.
	if (compilation_retval != 0 || failing_commands.size() > 0) {
		// Although ExecuteCompilation produces a list of failing commands, generateCompilationDiagnostics only accepts one failing command. Give it the first one, I guess?
		// This command does not "try to generate diagnostics for link or dsymutil jobs," so nothing will be printed in those cases. See `cfe/tools/driver/driver.cpp'.
		clang_driver.generateCompilationDiagnostics(*compilation, failing_commands[0].second);
		std::cout << "\nCompilation failed" << std::endl;
	} else {
		std::cout << "\nCompilation succeeded" << std::endl;
	}

	// Remove the temporary assembler file.
	fs::remove(assembly_path);
}
