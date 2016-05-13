#include "stdlib.hpp"

#include <boost/program_options.hpp>

#include <iostream>

using namespace paralisp;
namespace po = boost::program_options;

PLInt threshold(PLInt pixel_value) {
	return pixel_value < 230 ? 0 : 255;
}

static std::string generate_usage(const char *program_name, const po::options_description &options) {
	std::ostringstream ss;
	ss << "Usage: " << program_name << " [options]\n\n" <<
	   "C++ Threshold Function w/ Stdlib Benchmark" <<
	   options;
	return ss.str();
}


int main(int argc, char *argv[]) {
	po::options_description cmdline_options("Execution options");
	cmdline_options.add_options()
	("parallel,P", "run operations in parallel")
	("num-threads,n", po::value<PLInt>()->default_value(-1),
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


PLInt *in_list_array = list_from_image();
PLInt *out_list_array;
if (args.count("parallel")) {
out_list_array = map_parallel(threshold, in_list_array, true, args["num-threads"].as<PLInt>());
} else {
out_list_array = map_sequential(threshold, in_list_array, true);
}
// image_from_list(29566, 14321, out_list_array);
	return 0;
}
