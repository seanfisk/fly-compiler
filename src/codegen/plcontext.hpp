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

namespace paralisp {

	struct NBlock;

	typedef boost::unordered_map<std::string, llvm::Value *> SymbolTable;
	typedef boost::unordered_map<std::string, llvm::Instruction::BinaryOps> BinaryOps;
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
		bool map_is_parallel;
		bool enable_map_profiling;
		paralisp::PLInt num_threads;

		void initialize();

	public:
		BinaryOps binary_ops;
		ComparisonOps comparison_ops;
		boost::unordered_set<std::string> stdlib_functions;

		PLContext(bool map_is_parallel, bool enable_map_profiling, paralisp::PLInt num_threads) :
			module(new llvm::Module("main", llvm::getGlobalContext())),
			map_is_parallel(map_is_parallel),
			enable_map_profiling(enable_map_profiling),
			num_threads(num_threads) {
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

		bool get_map_is_parallel() const {
			return map_is_parallel;
		}

		bool get_enable_map_profiling() const {
			return enable_map_profiling;
		}

		paralisp::PLInt get_num_threads() const {
			return num_threads;
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
