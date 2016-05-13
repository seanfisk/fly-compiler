#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <vector>
#include <string>

#include <boost/filesystem.hpp>

namespace paralisp {
	// NOTE: Because this uses c_str(), the pointers in the return value are only valid for the lifetime of the vector!!! (I think)
	std::vector<const char *> to_c_str_vector(const std::vector<std::string> &strings);
	boost::filesystem::path make_safe_temp_file(const boost::filesystem::path &model);
}
#endif
