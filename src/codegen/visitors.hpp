#ifndef _VISITORS_HPP
#define _VISITORS_HPP

#include "codegen/plcontext.hpp"
#include "nodes.hpp"

#include <boost/variant/static_visitor.hpp>

#include <llvm/IR/Value.h>

#include <iostream>

namespace fly {
	class CodeGenVisitor : public boost::static_visitor<llvm::Value *> {
		PLContext &context;
	public:
		CodeGenVisitor(PLContext &context)
			: context(context) {}

		llvm::Value *operator()(const NBlock &node) const;
		llvm::Value *operator()(const NIdent &node) const;
		llvm::Value *operator()(const NInteger &node) const;
		llvm::Value *operator()(const NList &node) const;
	};

	struct PrintVisitor : public boost::static_visitor<std::ostream &> {
		std::ostream &out;
	public:
		PrintVisitor(std::ostream &out)
			: out(out) {}

		std::ostream &operator()(const NBlock &node);
		std::ostream &operator()(const NIdent &node);
		std::ostream &operator()(const NInteger &node);
		std::ostream &operator()(const NList &node);
	};
}


#endif
