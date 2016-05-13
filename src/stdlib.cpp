// "Standard library" functions

#include "stdlib.hpp"

#include <iostream>

using paralisp::PLInt;

static inline PLInt get_list_size(PLInt list_array[]) {
	return list_array[0];
}

static inline PLInt *get_list_contents_ptr(PLInt list_array[]) {
	return &list_array[1];
}

extern "C" {
	// Language features

	PLInt print(PLInt x) {
		std::cout << "The integer: " << x << std::endl;
		return x;
	}

	PLInt print_list(PLInt list_array[]) {
		PLInt list_size = get_list_size(list_array);
		PLInt *list = get_list_contents_ptr(list_array);
		std::cout << "List: ";

		for (PLInt i = 0; i < list_size - 1; ++i) {
			std::cout << list[i] << " ";
		}

		std::cout << list[list_size - 1] << std::endl;
		return list_size;
	}
}
