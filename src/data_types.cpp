#include "data_types.hpp"

#include <llvm/IR/LLVMContext.h>

using fly::Int;
using namespace llvm;

IntegerType *fly::get_pl_int_ty() {
	return Type::getInt64Ty(getGlobalContext());
}

ConstantInt *fly::get_pl_int(Int value) {
	return ConstantInt::getSigned(fly::get_pl_int_ty(), value);
}
