// "Standard library" functions

#include "stdlib.hpp"

#include <iostream>
#include <cerrno>
#include <cstring>

#include <pthread.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/predef.h>
#include <boost/lexical_cast.hpp>
#include <boost/timer/timer.hpp>

#include <Magick++.h>

using paralisp::PLInt;

static boost::mutex print_mutex;

static inline PLInt *make_list(PLInt size) {
	PLInt *list_array = new PLInt[size + 1];
	list_array[0] = size;
	return list_array;
}

static inline PLInt get_list_size(PLInt list_array[]) {
	return list_array[0];
}

static inline PLInt *get_list_contents_ptr(PLInt list_array[]) {
	return &list_array[1];
}

typedef boost::function<PLInt()> MappedBoundFunction;

void worker(PLInt thread_num, PLInt(*func)(PLInt), PLInt *in_list, PLInt *out_list, PLInt list_size) {
	// Set the name of the thread. Useful for debugging purposes.
	// This is not a standard pthread interface, and is implemented differently across pthread implementations.
	std::string thread_name("Thread " + boost::lexical_cast<std::string>(thread_num));
#if BOOST_OS_MACOS
	// Mac OS X only lets us set the name of the current thread.
	int ret = pthread_setname_np(thread_name.c_str());
#elif BOOST_OS_LINUX
	pthread_t current_thread = pthread_self();
	int ret = pthread_setname_np(current_thread, thread_name.c_str());
#endif
#if BOOST_OS_MACOS || BOOST_OS_LINUX

	if (ret != 0) {
		std::cerr << "Could not set thread name: " << strerror(errno) << std::endl;
		std::exit(1);
	}

#endif

	for (PLInt i = 0; i < list_size; ++i) {
		out_list[i] = func(in_list[i]);
	}
}

extern "C" {
	// Language features

	PLInt print(PLInt x) {
		boost::lock_guard<boost::mutex> guard(print_mutex);
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

	paralisp::PLInt *map_parallel(PLInt(*func)(PLInt), PLInt in_list_array[], bool enable_profiling, paralisp::PLInt num_threads) {
		const char *thread_name = "Main thread";
#if BOOST_OS_MACOS
		int ret = pthread_setname_np(thread_name);
#elif BOOST_OS_LINUX
		pthread_t current_thread = pthread_self();
		int ret = pthread_setname_np(current_thread, thread_name);
#endif
#if BOOST_OS_MACOS || BOOST_OS_LINUX

		if (ret != 0) {
			std::cerr << "Could not set thread name: " << strerror(errno) << std::endl;
			std::exit(1);
		}

#endif

		PLInt ideal_num_threads = num_threads == -1 ? boost::thread::hardware_concurrency() : num_threads;

		boost::timer::cpu_timer *timer;

		if (enable_profiling) {
			timer = new boost::timer::cpu_timer;
		} else {
			// Don't print extra junk if we're profiling.
			std::cout << "Mapping in parallel with " << ideal_num_threads << " threads" << std::endl;
		}

		PLInt in_list_size = get_list_size(in_list_array);
		PLInt *in_list = get_list_contents_ptr(in_list_array);

		PLInt *out_list_array = make_list(in_list_size); // FIXME: Memory leak
		PLInt *out_list = get_list_contents_ptr(out_list_array);

		boost::thread_group group;

		PLInt elements_per_thread = in_list_size / ideal_num_threads;

		// Spawn worker threads (one less than ideal).
		for (PLInt thread_num = 0; thread_num < ideal_num_threads - 1; ++thread_num) {
			PLInt start_index = thread_num * elements_per_thread;
			group.create_thread(boost::bind(worker, thread_num, func,
			                                &in_list[start_index],
			                                &out_list[start_index],
			                                elements_per_thread));
		}

		// Current thread does the rest.
		for (PLInt i = (ideal_num_threads - 1) * elements_per_thread; i < in_list_size; ++i) {
			out_list[i] = func(in_list[i]);
		}

		group.join_all();

		if (enable_profiling) {
			timer->stop();
			// Print wall clock timing
			std::cout << timer->format(boost::timer::default_places, "%w") << std::endl;
			delete timer;
		}

		return out_list_array;
	}

	paralisp::PLInt *map_sequential(PLInt(*func)(PLInt), PLInt in_list_array[], bool enable_profiling) {

		boost::timer::cpu_timer *timer;

		if (enable_profiling) {
			timer = new boost::timer::cpu_timer;
		} else {
			// Don't print extra junk if we're profiling.
			std::cout << "Mapping sequentially" << std::endl;
		}

		PLInt in_list_size = get_list_size(in_list_array);
		PLInt *in_list = get_list_contents_ptr(in_list_array);

		PLInt *out_list_array = make_list(in_list_size); // FIXME: Memory leak
		PLInt *out_list = get_list_contents_ptr(out_list_array);

		for (PLInt i = 0; i < in_list_size; ++i) {
			out_list[i] = func(in_list[i]);
		}

		if (enable_profiling) {
			timer->stop();
			// Print wall clock timing
			std::cout << timer->format(boost::timer::default_places, "%w") << std::endl;
			delete timer;
		}

		return out_list_array;
	}

	// Graphics

	PLInt *list_from_image() {
		Magick::InitializeMagick(NULL);
		Magick::Image image;

		try {
			image.read("in.jpg");
			image.type(Magick::GrayscaleType); // Convert to grayscale
		} catch (Magick::Exception &error) {
			std::cout << "GraphicsMagick exception: " << error.what() << std::endl;
			std::exit(1);
		}

		PLInt num_pixels = image.columns() * image.rows();

		PLInt *pixel_list_array = make_list(num_pixels); // FIXME: Memory leak
		PLInt *pixel_list = get_list_contents_ptr(pixel_list_array);

		const Magick::PixelPacket *pixel_cache = \
		        image.getConstPixels(/*x_=*/0, /*y_=*/0, /*columns_=*/image.columns(), /*rows_=*/image.rows());

		for (PLInt i = 0; i < num_pixels; ++i) {
			pixel_list[i] = pixel_cache[i].red;
		}

		// image.write(/*x_=*/10, /*y_=*/10, /*columns_=*/10, /*rows_=*/1, // Use a LongPixel (32 bits) and pad each quantum. /*map_=*/"RGB", /*type_=*/MagickLib::LongPixel, /*pixels_=*/raw_pixel_list);
		return pixel_list_array;
	}

	PLInt image_from_list(PLInt width, PLInt height, PLInt pixel_list_array[]) {
		PLInt pixel_list_size = get_list_size(pixel_list_array);
		PLInt *pixel_list = get_list_contents_ptr(pixel_list_array);

		if (pixel_list_size != width * height) {
			std::cerr << "Given image dimensions inconsistent with list size." << std::endl;
			std::exit(1);
		}

		Magick::Image image(Magick::Geometry(width, height), "white");
		// Ensure that there is only one reference to underlying image.
		// If this is not done, then image pixels will not be modified.
		image.modifyImage();
		// Pixels don't seem to be editable if we use GrayscaleType...
		image.type(Magick::TrueColorType);
		Magick::PixelPacket *pixel_cache = image.setPixels(/*x_=*/0, /*y_=*/0, /*columns_=*/width, /*rows_=*/height);

		for (PLInt i = 0; i < pixel_list_size; ++i) {
			pixel_cache[i] = Magick::Color(pixel_list[i], pixel_list[i], pixel_list[i]);
		}

		image.syncPixels();

		// Now, since we ran a threshold operation, convert to black and white.
		image.type(Magick::BilevelType);

		try {
			image.write("out.jpg");
		} catch (Magick::Exception &error) {
			std::cout << "GraphicsMagick exception: " << error.what() << std::endl;
			std::exit(1);
		}

		return pixel_list_size;
	}
}
