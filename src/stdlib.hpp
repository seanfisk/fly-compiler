#ifndef _STDLIB_HPP
#define _STDLIB_HPP

// TODO: The type int64_t is not guaranteed.
#include <cstdint>

namespace paralisp {
	typedef int64_t PLInt;
}

extern "C" {
	// Language features
	paralisp::PLInt print(paralisp::PLInt x);
	paralisp::PLInt print_list(paralisp::PLInt list_array[]);
}

#endif
