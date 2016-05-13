#include "codegen/visitors.hpp"

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h> // for 'verifyFunction'

#include <boost/unordered_map.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace fly;
using namespace llvm;

class NIdentVisitor : public boost::static_visitor<std::string> {
	const std::string &error_message;
public:
	NIdentVisitor(const std::string &error_message)
		: error_message(error_message) {}
	std::string operator()(NIdent &ident) const {
		// Handles specifically NIdent's only.
		return ident.name;
	}

	template <typename T>
	std::string operator()(T &node) const {
		// Handles anything that's not an NIdent.

		// It's saying I can't cast! I can't move, am I lagging, guys?
		std::cerr << error_message << std::endl;
		std::exit(1);

		// Avoid compiler warnings;
		return "";
	}
};

class DefunParamListVisitor : public boost::static_visitor<NList> {
public:
	NList operator()(NList &list) const {
		// Handles specifically NList's only.
		return list;
	}

	template <typename T>
	NList operator()(T &node) const {
		// Crash on anything that's not an NList.
		std::cerr << "Second argument to `defun' must be a sexp list." << std::endl;
		std::exit(1);
		return NList();
	}
};

static inline Value *generate_defun(PLContext &context, SexpList members) {
	// `defun' special form

	// Check number of args to defun.
	if (members.size() < 3) {
		std::cerr << "Too few arguments to `defun'." << std::endl;
		std::exit(1);
	}

	NIdentVisitor visitor("First argument to `defun' (new function name) must be an identifier.");
	std::string new_function_name = boost::apply_visitor(visitor, members.front());
	members.pop_front();

	std::cout << "Creating defun: " << new_function_name << std::endl;

	// Collect parameters.

	NList parsed_params = boost::apply_visitor(DefunParamListVisitor(), members.front());
	members.pop_front();
	IdentNameList new_function_param_names;

	BOOST_FOREACH(SexpList::value_type sexp, parsed_params.members) {
		NIdentVisitor visitor("Second argument to `defun' must be a list of identifiers.");
		std::string param_name = boost::apply_visitor(visitor, sexp);
		new_function_param_names.push_back(param_name);
	}

	// Create parameter types for the function.
	std::vector<Type *> new_function_param_types(new_function_param_names.size(), get_pl_int_ty());

	// Create the new function.
	FunctionType *ftype = FunctionType::get(get_pl_int_ty(), new_function_param_types, /*isVarArg=*/false);
	Function *function = Function::Create(ftype, GlobalValue::InternalLinkage, new_function_name, context.get_module());
	BasicBlock *bblock = BasicBlock::Create(context.get_llvm_context(), "entry", function, 0);

	context.push_block(bblock);

	int i = 0;

	// Give the function arguments names, allowing them to be referenced. Also add them to the symbol table.
	for (Function::arg_iterator it = function->arg_begin(); it != function->arg_end(); ++it, ++i) {
		std::string &name = new_function_param_names[i];
		it->setName(name);
		context.locals()[name] = it;
	}

	// Code-gen the body.
	Value *last = NULL;
	BOOST_FOREACH(SexpList::value_type sexp, members) {
		last = boost::apply_visitor(CodeGenVisitor(context), sexp);
	}

	// We can't just pass the bblock in here; it could have been changed by a conditional (with branching).
	IRBuilder<> builder(context.current_block());
	builder.CreateRet(last);

	context.pop_block();

	// Check the function.
	verifyFunction(*function);

	return function;
}

static inline Value *generate_list(PLContext &context, const SexpList &members) {
	// `list' special form

	// Check number of args.
	if (members.size() == 0) {
		std::cerr << "Too few arguments to `list'." << std::endl;
		std::exit(1);
	}

	std::cout << "Creating list special form" << std::endl;

	IRBuilder<> builder(context.current_block());

	// Array allocation
	// We allocate one more than the list's actual size because we store the size as the first element of the array.
	ArrayType *array_type = ArrayType::get(get_pl_int_ty(), members.size() + 1);
	// It's actually shorter to create AllocaInst without the builder, because using the builder means that we'd have to specify the size twice. Lame.
	AllocaInst *array_alloca = new AllocaInst(array_type, "list", context.current_block());

	// Assignment of values to array
	ConstantInt *zero_int = get_pl_int(0);

	// Store the length of the list as the first element of the array.
	std::vector<Value *> array_ptr_indices(2, zero_int);
	// It's hard to know whether to create an inbounds GEP or not. See these links for more information:
	// - http://llvm.org/docs/LangRef.html#getelementptr-instruction
	// - http://llvm.org/docs/GetElementPtr.html
	// My head was spinning after reading...
	Value *array_ptr = builder.CreateInBoundsGEP(array_alloca, array_ptr_indices, "array_ptr");
	ConstantInt *array_size_int = get_pl_int(members.size());
	builder.CreateStore(array_size_int, array_ptr);

	CodeGenVisitor visitor(context);

	for (int vector_index = 0; vector_index < members.size(); ++vector_index) {
		// Get the value of the integer.
		SexpList::value_type sexp = members[vector_index];
		Value *value = boost::apply_visitor(visitor, sexp);

		if (!value->getType()->isIntegerTy()) {
			std::cerr << "Only integers are supported for the `list' special form." << std::endl;
			std::exit(1);
		}

		// Build the indexing instruction.
		std::vector<Value *> indices;
		// The first index indexes through the array's pointer.
		indices.push_back(zero_int);
		// The second index actually indexes into the array.
		// See <http://llvm.org/docs/GetElementPtr.html>
		int array_index = vector_index + 1;
		indices.push_back(get_pl_int(array_index));
		Value *array_index_ptr = builder.CreateInBoundsGEP(array_alloca, indices, "array_ptr_to_index_" + boost::lexical_cast<std::string>(array_index));

		// Store the integer.
		builder.CreateStore(value, array_index_ptr);
	}

	return array_ptr;
}

static inline Value *generate_if(PLContext &context, const SexpList &members) {
	IRBuilder<> builder(context.current_block());

	if (members.size() != 3) {
		std::cerr << "Expected 3 arguments to `if'; " << members.size() << " given." << std::endl;
		std::exit(1);
	}

	std::cout << "Creating if special form" << std::endl;

	CodeGenVisitor visitor(context);

	// Basically copied from <http://llvm.org/docs/tutorial/LangImpl5.html#code-generation-for-if-then-else>.
	// Names were adapted to match the Scheme specifications and our own conventions.

	Function *current_function = builder.GetInsertBlock()->getParent();

	// Generate comparison operator.
	Value *test_value = boost::apply_visitor(visitor, members[0]);
	Value *icmp_inst = builder.CreateICmpNE(test_value, get_pl_int(0), "if-test");

	// Create blocks for the consequent and alternate cases. Insert consequent block at the end of the function.
	BasicBlock *consequent_block = BasicBlock::Create(context.get_llvm_context(), "if-consequent", current_function);
	BasicBlock *alternate_block = BasicBlock::Create(context.get_llvm_context(), "if-alternate");
	BasicBlock *merge_block = BasicBlock::Create(context.get_llvm_context(), "if-merge");

	// Create conditional branch.
	builder.CreateCondBr(icmp_inst, consequent_block, alternate_block);

	// Emit consequent block.
	builder.SetInsertPoint(consequent_block);
	context.swap_current_block(consequent_block);
	Value *consequent_value = boost::apply_visitor(visitor, members[1]);
	builder.CreateBr(merge_block);

	// Codegen of consequent can change the current block; update consequent for the PHI.
	consequent_block = builder.GetInsertBlock();

	// Emit alternate block.
	current_function->getBasicBlockList().push_back(alternate_block);
	builder.SetInsertPoint(alternate_block);
	context.swap_current_block(alternate_block);
	Value *alternate_value = boost::apply_visitor(visitor, members[2]);
	builder.CreateBr(merge_block);

	// Codegen of alternate can change the current block; update alternate for the PHI.
	alternate_block = builder.GetInsertBlock();

	// Emit merge block.
	current_function->getBasicBlockList().push_back(merge_block);
	builder.SetInsertPoint(merge_block);
	context.swap_current_block(merge_block);

	// Generate phi node.
	PHINode *phi_node = builder.CreatePHI(get_pl_int_ty(), /*NumReservedValues=*/2, "if-tmp");
	phi_node->addIncoming(consequent_value, consequent_block);
	phi_node->addIncoming(alternate_value, alternate_block);

	return phi_node;
}

static inline Value *generate_binary_expression(PLContext &context, const std::string &op, const SexpList &members) {
	// Binary arithmetic expression

	std::cout << "Create binary arithmetic expression: " << op << std::endl;

	if (members.size() != 2) {
		std::cerr << "Operator `" << op << "' is a binary operator; " << members.size() << " arguments given." << std::endl;
		std::exit(1);
	}

	IRBuilder<> builder(context.current_block());
	return builder.CreateBinOp(
	           context.binary_ops[op],
	           boost::apply_visitor(CodeGenVisitor(context), members[0]),
	           boost::apply_visitor(CodeGenVisitor(context), members[1]),
	           op);
}

static inline Value *generate_comparison_expression(PLContext &context, const std::string &op, const SexpList &members) {
	// Binary comparison expression

	std::cout << "Create binary comparison expression: " << op << std::endl;

	if (members.size() != 2) {
		std::cerr << "Operator `" << op << "' is a binary comparison operator; " << members.size() << " arguments given." << std::endl;
		std::exit(1);
	}

	IRBuilder<> builder(context.current_block());
	Value *cmp_result = builder.CreateICmp(
	                        context.comparison_ops[op],
	                        boost::apply_visitor(CodeGenVisitor(context), members[0]),
	                        boost::apply_visitor(CodeGenVisitor(context), members[1]),
	                        op);
	// Convert boolean to our integer type.
	// isSigned indicates whether the integer to cast is interpreted as signed.
	return builder.CreateIntCast(cmp_result, get_pl_int_ty(), /*isSigned=*/false, "cmp_cast");
}

static inline Value *generate_function_call(PLContext &context, std::string function_name, const SexpList &members) {
	// Function call

	int num_passed_args = members.size();

	if (context.stdlib_functions.count(function_name)) {
		// Identifiers in Scheme should use hyphens, but those aren't allowed in our standard library functions because they are written in C++. Convert to hyphens to underscores before creating a call to a stdlib function.
		boost::replace_all(function_name, "-", "_");
	}

	Function *function_to_call = context.get_module()->getFunction(function_name);

	if (!function_to_call) {
		std::cerr << "Function not found: " << function_name << std::endl;
		std::exit(1);
	}

	if (function_to_call->arg_size() != num_passed_args) {
		std::cerr << "Incorrect number of arguments passed to function: " << function_name << " (" << members.size() << " given, " << function_to_call->arg_size() << " expected)" << std::endl;
		std::exit(1);
	}

	std::vector<Value *> code_genned_arguments;
	code_genned_arguments.reserve(members.size());

	BOOST_FOREACH(SexpList::value_type sexp, members) {
		code_genned_arguments.push_back(boost::apply_visitor(CodeGenVisitor(context), sexp));
	}

	std::string call_name;

	// It is an error to assign a name to the call instruction if the type of the function being called is void.
	if (!function_to_call->getReturnType()->isVoidTy()) {
		call_name = function_name;
	}

	IRBuilder<> builder(context.current_block());
	return builder.CreateCall(function_to_call, makeArrayRef(code_genned_arguments), call_name);
}

Value *CodeGenVisitor::operator()(const NList &node) const {
	// Copy the members because we would like to modify the list.
	SexpList members = node.members;

	if (members.size() == 0) {
		std::cerr << "Fly does not support empty lists at this time." << std::endl;
		std::exit(1);
	}

	std::cout << "Creating list with " << members.size() << " members." << std::endl;

	Value *retval;
	NIdentVisitor visitor("Fly does not support lists that do not begin with identifiers at this time.");
	std::string function_name = boost::apply_visitor(visitor, members.front());
	members.pop_front();

	if (function_name == "defun") {
		retval = generate_defun(context, members);
	} else if (function_name == "list") {
		retval = generate_list(context, members);
	} else if (function_name == "if") {
		retval = generate_if(context, members);
	} else if (context.binary_ops.count(function_name)) {
		retval = generate_binary_expression(context, function_name, members);
	} else if (context.comparison_ops.count(function_name)) {
		retval = generate_comparison_expression(context, function_name, members);
	} else {
		retval = generate_function_call(context, function_name, members);
	}

	return retval;
}

std::ostream &PrintVisitor::operator()(const NList &node) {
	return out << "list node with " << node.members.size() << " members";
}
