#include "codegen/plcontext.hpp"
#include "data_types.hpp"
#include "codegen/visitors.hpp"

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TargetSelect.h> // for target initialization

#include <boost/variant/apply_visitor.hpp>

using namespace paralisp;
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
	stdlib_functions.insert("map-sequential");
	stdlib_functions.insert("map-parallel");
	stdlib_functions.insert("list-from-image");
	stdlib_functions.insert("image-from-list");
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

		// Prototype for map_sequential and map_parallel
		{
			std::vector<Type *> passed_function_arg_types;
			passed_function_arg_types.push_back(get_pl_int_ty());
			std::vector<Type *> arg_types;
			arg_types.push_back(
			    PointerType::get(
			        FunctionType::get(get_pl_int_ty(), passed_function_arg_types, /*isVarArg=*/false), /*AddressSpace=*/0));
			arg_types.push_back(pl_int_ptr_ty);
			arg_types.push_back(builder.getInt1Ty());
			Function::Create(
			    FunctionType::get(pl_int_ptr_ty, arg_types, /*isVarArg=*/false),
			    Function::ExternalLinkage, "map_sequential", module);

			arg_types.push_back(get_pl_int_ty());
			Function::Create(
			    FunctionType::get(pl_int_ptr_ty, arg_types, /*isVarArg=*/false),
			    Function::ExternalLinkage, "map_parallel", module);
		}

		// Prototype for list_from_image
		{
			std::vector<Type *> arg_types;
			Function::Create(
			    FunctionType::get(pl_int_ptr_ty, arg_types, /*isVarArg=*/false),
			    Function::ExternalLinkage, "list_from_image", module);
		}

		// Prototype for image_from_list
		{
			std::vector<Type *> arg_types;
			arg_types.push_back(get_pl_int_ty());
			arg_types.push_back(get_pl_int_ty());
			arg_types.push_back(pl_int_ptr_ty);
			Function::Create(
			    FunctionType::get(get_pl_int_ty(), arg_types, /*isVarArg=*/false),
			    Function::ExternalLinkage, "image_from_list", module);
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
