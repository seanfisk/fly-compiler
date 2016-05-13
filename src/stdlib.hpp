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

	paralisp::PLInt *map_sequential(paralisp::PLInt(*func)(paralisp::PLInt), paralisp::PLInt list_array[], bool enable_profiling);
	paralisp::PLInt *map_parallel(paralisp::PLInt(*func)(paralisp::PLInt), paralisp::PLInt list_array[], bool enable_profiling, paralisp::PLInt num_threads);

	// Graphics
	paralisp::PLInt *list_from_image();
	paralisp::PLInt image_from_list(paralisp::PLInt width, paralisp::PLInt height, paralisp::PLInt list_array[]);
}

#endif
