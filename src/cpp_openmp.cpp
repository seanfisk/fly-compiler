#include "stdlib.hpp"

#include <boost/timer/timer.hpp>
#include <boost/program_options.hpp>

#include <omp.h>

#include <iostream>

using namespace paralisp;
namespace po = boost::program_options;

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

PLInt threshold(PLInt pixel_value) {
	return pixel_value < 230 ? 0 : 255;
}

static std::string generate_usage(const char *program_name, const po::options_description &options) {
	std::ostringstream ss;
	ss << "Usage: " << program_name << " [options]\n\n" <<
	   "OpenMP Extra Benchmark" <<
	   options;
	return ss.str();
}


int main(int argc, char *argv[]) {
	po::options_description cmdline_options("Execution options");
	cmdline_options.add_options()
	("parallel,P", "run operations in parallel")
	("num-threads,n", po::value<int>()->default_value(omp_get_max_threads()),
	 "number of threads on which to execute, default is num processors at *runtime*")
	;

	po::variables_map args;
	po::command_line_parser parser(argc, argv);

	try {
		po::store(parser.options(cmdline_options).run(), args);

		if (args.count("help")) {
			std::cout << generate_usage(argv[0], cmdline_options) << std::endl;
			std::exit(EXIT_SUCCESS);
		}
	} catch (po::error &e) {
		std::cerr << e.what() << "\n\n" << generate_usage(argv[0], cmdline_options) << std::endl;
		std::exit(EXIT_FAILURE);
	}

	int num_threads = args.count("parallel") ? args["num-threads"].as<int>() : 1;

	// This seems not to work at all. Setting it explicitly in the for pragma later with the value of omp_get_max_threads() doesn't even work.
// omp_set_num_threads(num_threads);

	PLInt *in_list_array = list_from_image();
	PLInt in_list_size = get_list_size(in_list_array);
	PLInt *in_list = get_list_contents_ptr(in_list_array);

	PLInt *out_list_array = make_list(in_list_size);
	PLInt *out_list = get_list_contents_ptr(out_list_array);

	boost::timer::cpu_timer timer;

	// num_threads is used explicitly because we couldn't the above to work. The environment variable, OMP_NUM_THREADS, also seems to work.
#pragma omp parallel for num_threads(num_threads)
	for (PLInt i = 0; i < in_list_size; ++i) {
		out_list[i] = threshold(in_list[i]);
	}

	timer.stop();
	// Print wall clock timing
	std::cout << timer.format(boost::timer::default_places, "%w") << std::endl;

	// image_from_list(29566, 14321, out_list_array);

	return 0;
}
