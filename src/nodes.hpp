#ifndef _NODES_HPP
#define _NODES_HPP

#include "data_types.hpp"

#include <llvm/IR/Value.h>
#include <boost/variant/recursive_variant.hpp>

#include <deque>
#include <vector>
#include <iostream>

namespace fly {
	struct Node {
		virtual ~Node() {}
	};

	struct NSexp : public Node {};

	struct NAtom : public NSexp {};

	struct NInteger : public NAtom {
		Int value;
		// The default constructor is needed by Bison -- initialize value to 0, I guess?
		NInteger() : value(0) {}
		NInteger(Int value) : value(value) {}
	};

	struct NIdent : public NAtom {
		std::string name;
		// The default constructor is needed by Bison.
		NIdent() {}
		NIdent(const std::string &name) : name(name) {}
	};

	typedef std::vector<std::string> IdentNameList;

	// The issue here is circular dependency. NList is a type of symbolic expression, which means it needs to be part of the SexpVariant. However, an NList also holds SexpVariant's. Kill the circular dependency by using boost::recursive_wrapper. boost::make_recursive_variant is also available, but it only works with very conformant compilers. Better play it safe.
	struct NList;
	typedef boost::variant<NInteger, NIdent, boost::recursive_wrapper<NList> > SexpVariant;
	typedef std::deque<SexpVariant> SexpList;

	struct NBlock : public Node {
		SexpList statements;
		NBlock() {}
		NBlock(const SexpList &statements) :
			statements(statements) {}
	};

	struct NList : public NSexp {
		SexpList members;
		NList() {}
		NList(const SexpList &members) :
			members(members) {
		}
	};
}

#endif
