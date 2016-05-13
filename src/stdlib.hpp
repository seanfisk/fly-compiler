#ifndef _STDLIB_HPP
#define _STDLIB_HPP

// TODO: The type int64_t is not guaranteed.
#include <cstdint>

namespace fly {
	typedef int64_t Int;
}

extern "C" {
	// Language features
	fly::Int print(fly::Int x);
	fly::Int print_list(fly::Int list_array[]);
}

#endif
