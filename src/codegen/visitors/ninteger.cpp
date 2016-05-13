#include "codegen/visitors.hpp"
#include "data_types.hpp"

using namespace paralisp;
using namespace llvm;

Value *CodeGenVisitor::operator()(const NInteger &node) const {
	std::cout << "Creating integer: " << node.value << std::endl;
	return get_pl_int(node.value);
}

std::ostream &PrintVisitor::operator()(const NInteger &node) {
	return out << "integer node of value " << node.value;
}
