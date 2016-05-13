// We are looking for a 64-bit integer. <cstdint> *can* define it, but it's not guaranteed.
// Here, we just lean on LLVM because it appears to do the right thing on MSVC++, etc., to get us a real 64-bit integer.

#include <llvm/Support/DataTypes.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>

namespace fly {
	typedef int64_t Int;
	llvm::ConstantInt *get_pl_int(Int value);
	llvm::IntegerType *get_pl_int_ty();
}
