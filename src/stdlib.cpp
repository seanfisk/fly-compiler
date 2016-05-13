// "Standard library" functions

#include "stdlib.hpp"

#include <iostream>

using fly::Int;

static inline Int get_list_size(Int list_array[]) {
	return list_array[0];
}

static inline Int *get_list_contents_ptr(Int list_array[]) {
	return &list_array[1];
}

extern "C" {
	// Language features

	Int print(Int x) {
		std::cout << "The integer: " << x << std::endl;
		return x;
	}

	Int print_list(Int list_array[]) {
		Int list_size = get_list_size(list_array);
		Int *list = get_list_contents_ptr(list_array);
		std::cout << "List: ";

		for (Int i = 0; i < list_size - 1; ++i) {
			std::cout << list[i] << " ";
		}

		std::cout << list[list_size - 1] << std::endl;
		return list_size;
	}
}
