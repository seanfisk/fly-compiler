#include "codegen/visitors.hpp"

#include <iostream>

using namespace fly;
using namespace llvm;

Value *CodeGenVisitor::operator()(const NIdent &node) const {
	std::cout << "Creating identifier reference: " << node.name << std::endl;
	Value *ref;

	try {
		// First look in the function's local variables.
		ref = context.locals().at(node.name);
	} catch (const std::out_of_range &) {
		// Then look to see if it is a function name.
		ref = context.get_module()->getFunction(node.name);

		if (!ref) {
			std::cerr << "Variable not found in symbol table: " << node.name << std::endl;
			std::exit(1);
		}
	}

	return ref;
}

std::ostream &PrintVisitor::operator()(const NIdent &node) {
	return out << "identifier node with name '" << node.name << "'";
}
