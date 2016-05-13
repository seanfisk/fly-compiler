#include "utils.hpp"

#include <iostream>
#include <cerrno>
#include <cstring>

#include <sys/stat.h>
#include <fcntl.h>

namespace fs = boost::filesystem;

struct to_c_str {
	const char *operator()(const std::string &s) const {
		return s.c_str();
	}
};

// Credit: http://stackoverflow.com/a/17027869
std::vector<const char *> paralisp::to_c_str_vector(const std::vector<std::string> &strings) {
	std::vector<const char *> c_strings(strings.size());
	std::transform(strings.begin(), strings.end(), c_strings.begin(), to_c_str());
	return c_strings;
}

// Uses Boost.Filesystem's unique_path along with atomic open to safely create a temporary file.
fs::path paralisp::make_safe_temp_file(const fs::path &model) {
	// unique_path allows us to create a very probably unique temporary file. But we still need to check for the file's existence. In checking for the file's existence, a race condition can occur between the check and the creation of the file. Therefore, we must use the O_EXCL which makes open(...) atomic.
	// POSIX mkstemp() is pretty close to doing what we want, but it doesn't allow the template to have a custom extension. That's a problem for us, because we need native assembler to have a .s extension for clang to recognize it. Moving the file undoes the whole purpose of this function...
	// Of course, after the file is open, there is nothing preventing other processes from writing to it then as well.
	fs::path temp_path;
	int temp_fd;

	while (true) {
		// Generate a hopefully unique path.
		temp_path = fs::temp_directory_path() / fs::unique_path(model);
		// TODO: Do more error checking -- errno, etc.
		// There is a choice here to use POSIX open() with O_EXCL or fopen() with "x" in the mode flags. fopen() is part of the C standard library, and "x" is specified by C2011. That's not required in C++. The "x" mode is supported by glibc, so all platforms that use it will succeed. However, fopen() doesn't make any guarantees about setting errno and its only error indicator is returning a null pointer. That's not acceptable, since we need to know why it failed. We'll drop to the POSIX open() for now.
		// For a start at Windows compat (if necessary), check out _s_open_s <http://msdn.microsoft.com/en-us/library/w64k0ytk.aspx>. Windows doesn't support the "x" mode.
		errno = 0; // reset errno
		temp_fd = open(temp_path.c_str(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

		if (temp_fd != -1) {
			break;
		} else if (errno != EEXIST) {
			std::cerr << "Error creating temp file: " << strerror(errno) << std::endl;
			std::exit(1);
		}

		// This means that tempFd was invalid and errno was EEXIST. Try again.
	};

	// Immediately close the file.
	close(temp_fd);

	return temp_path;
}
