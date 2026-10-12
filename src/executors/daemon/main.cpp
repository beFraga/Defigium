#include "Daemon.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <ext/stdio_filebuf.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	int log_fd = open("out.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (log_fd < 0) {
		std::cerr << "[Daemon] Critical Error: could not open out.log" << std::endl;
		return EXIT_FAILURE;
	}

	__gnu_cxx::stdio_filebuf<char> log_filebuf(log_fd, std::ios::out);
	std::ostream log_file(&log_filebuf);

	std::streambuf * _cout = std::cout.rdbuf();
	std::streambuf * _cerr = std::cerr.rdbuf();
	std::cout.rdbuf(log_file.rdbuf());
	std::cerr.rdbuf(log_file.rdbuf());

	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <port_number>" << std::endl;
		std::cerr << "Example: ./daemon 6769" << std::endl;
		return EXIT_FAILURE;
	}

	int port = 0;

	try {
		port = std::stoi(argv[1]);
	} catch (const std::invalid_argument& e) {
		std::cerr << "[Daemon] Critical Error: Invalid argument '" << argv[1] 
                  << "'. Please provide a valid integer port." << std::endl;
        return EXIT_FAILURE;
    } catch (const std::out_of_range& e) {
        std::cerr << "[Daemon] Critical Error: Port argument '" << argv[1] 
                  << "' is out of range." << std::endl;
        return EXIT_FAILURE;
    }

	if (port <= 1024 || port > 65535) {
		std::cerr << "[Daemon] Critical Error: Port " << port 
                  << " is out of valid range (1025-65535)." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[Daemon] Starting daemon on port " << port << "..." << std::endl;

    Daemon daemon(port, log_fd);
    daemon.run();

	std::cout.rdbuf(_cout);
	std::cerr.rdbuf(_cerr);

    return EXIT_SUCCESS;
}
