#ifndef _PLCONTEXT_HPP
#define _PLCONTEXT_HPP

#include "cmdline.hpp"
#include "data_types.hpp"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/InstrTypes.h> // for llvm::CmpInst::*
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/filesystem.hpp>

#include <stack>
#include <typeinfo>
#include <string>

namespace fly {

	struct NBlock;

	typedef boost::unordered_map<std::string, llvm::Value *> SymbolTable;
	typedef boost::unordered_map<std::string, llvm::CmpInst::Predicate> ComparisonOps;

	struct PLBlock {
		llvm::BasicBlock *block;
		SymbolTable locals;
	};

	/**
	 * Maintain state while going through steps for code generation. It is very similar in purpose to LLVMContext.
	 */
	class PLContext {
		std::stack<PLBlock> blocks;
		llvm::Module *module;

		void initialize();

	public:
		ComparisonOps comparison_ops;
		boost::unordered_set<std::string> stdlib_functions;

		PLContext() :
			module(new llvm::Module("main", llvm::getGlobalContext())) {
			initialize();
		}

		PLContext(const PLContext &other) :
			// Not exactly sure if this works.
			module(CloneModule(other.module)) {
			initialize();
		}

		llvm::Module *get_module() {
			return module;
		}

		llvm::LLVMContext &get_llvm_context() {
			// The module store's LLVM's context.
			return module->getContext();
		}

		virtual ~PLContext();

		void generate_code(NBlock &root);

		SymbolTable &locals() {
			return blocks.top().locals;
		}
		llvm::BasicBlock *current_block() {
			return blocks.top().block;
		}
		void swap_current_block(llvm::BasicBlock *block) {
			blocks.top().block = block; // Yeah, that's not confusing.
		}
		void push_block(llvm::BasicBlock *block) {
			blocks.push(PLBlock());
			blocks.top().block = block;
		}
		void pop_block() {
			blocks.pop();
		}
	};
}

#endif
