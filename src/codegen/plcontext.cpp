#include "codegen/plcontext.hpp"
#include "data_types.hpp"
#include "codegen/visitors.hpp"

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TargetSelect.h> // for target initialization

#include <boost/variant/apply_visitor.hpp>

using namespace fly;
using namespace llvm;

PLContext::~PLContext() {
	delete module;
}

void PLContext::initialize() {
	binary_ops["+"] = llvm::Instruction::Add;
	binary_ops["-"] = llvm::Instruction::Sub;
	binary_ops["*"] = llvm::Instruction::Mul;
	binary_ops["/"] = llvm::Instruction::SDiv;

	comparison_ops["="] = llvm::CmpInst::ICMP_EQ;
	comparison_ops["<"] = llvm::CmpInst::ICMP_SLT;
	comparison_ops[">"] = llvm::CmpInst::ICMP_SGT;
	comparison_ops["<="] = llvm::CmpInst::ICMP_SLE;
	comparison_ops[">="] = llvm::CmpInst::ICMP_SGE;

	stdlib_functions.insert("print");
	stdlib_functions.insert("print-list");
}


// Compile the AST into a module.
void PLContext::generate_code(NBlock &root) {
	std::cout << "Generating code..." << std::endl;

	// Initialize LLVM's native target (the target for this machine). Without this, we cannot generate code.
	llvm::InitializeNativeTarget();
	Function *main_function;

	// New scope for builder
	{
		IRBuilder<> builder(get_llvm_context());

		// Create the top level interpreter function to call as entry.
		// Create the main function with external linkage so that the linker recognizes it as the main function.
		{
			std::vector<Type *> arg_types;
			main_function = Function::Create(
			                    FunctionType::get(get_pl_int_ty(), arg_types, /*isVarArg=*/false),
			                    GlobalValue::ExternalLinkage, "main", module);
		}

		// Create prototypes to standard library functions.

		// The second argument is address space, which apparently can just be zero.
		PointerType *pl_int_ptr_ty = PointerType::get(get_pl_int_ty(), /*AddressSpace=*/0);

		// Prototype for print.
		{
			std::vector<Type *> arg_types;
			arg_types.push_back(get_pl_int_ty());
			Function::Create(
			    FunctionType::get(get_pl_int_ty(), arg_types, /*isVarArg=*/false),
			    Function::ExternalLinkage, "print", module);
		}

		// Prototype for print_list
		{
			std::vector<Type *> arg_types;
			arg_types.push_back(pl_int_ptr_ty);
			Function::Create(
			    FunctionType::get(builder.getVoidTy(), arg_types, /*isVarArg=*/false),
			    Function::ExternalLinkage, "print_list", module);
		}
	}

	// New scope for builder
	{
		BasicBlock *bblock = BasicBlock::Create(get_llvm_context(), "entry", main_function);
		push_block(bblock);
		// We don't need to apply_visitor because we already know the type.
		CodeGenVisitor visitor(*this);
		visitor(root);
		IRBuilder<> builder(current_block());
		builder.CreateRet(get_pl_int(0));
		pop_block();
	}
}
