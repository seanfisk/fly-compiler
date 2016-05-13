#ifndef _CMDLINE_HPP
#define _CMDLINE_HPP

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <vector>
#include <string>

namespace fly {
	boost::program_options::variables_map parse_args(int argc, char *argv[]);
	typedef std::vector<boost::filesystem::path> LibrarySearchPaths;
}

#endif
