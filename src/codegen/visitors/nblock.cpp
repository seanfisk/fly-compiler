#include "codegen/visitors.hpp"

#include <iostream>

#include <boost/variant/apply_visitor.hpp>
#include <boost/foreach.hpp>

using namespace fly;
using namespace llvm;

Value *CodeGenVisitor::operator()(const NBlock &node) const {
	std::cout << "Creating block containing " << node.statements.size() << " statements." << std::endl;
	Value *last = NULL;

	BOOST_FOREACH(SexpList::value_type sexp, node.statements) {
		last = boost::apply_visitor(CodeGenVisitor(context), sexp);
	}
	return last;
}

std::ostream &PrintVisitor::operator()(const NBlock &node) {
	return out << "block node with " << node.statements.size() << " statements";
}
