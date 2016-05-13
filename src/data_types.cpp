#include "data_types.hpp"

#include <llvm/IR/LLVMContext.h>

using paralisp::PLInt;
using namespace llvm;

IntegerType *paralisp::get_pl_int_ty() {
	return Type::getInt64Ty(getGlobalContext());
}

ConstantInt *paralisp::get_pl_int(PLInt value) {
	return ConstantInt::getSigned(paralisp::get_pl_int_ty(), value);
}
