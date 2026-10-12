#include "Daemon.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>

class TeeBuffer : public std::streambuf {
	public:
		TeeBuffer(std::streambuf * sb1, std::streambuf * sb2) : sb1(sb1), sb2(sb2) {}
	protected:
		virtual int overflow(int c) override {
			if (c == EOF) return EOF;
			bool err = (sb1->sputc(c) == EOF) || (sb2->sputc(c) == EOF);
			return err ? EOF : c;
		}
		virtual int sync() override {
			return (sb1->pubsync() == 0 && sb2->pubsync() == 0) ? 0 : -1;
		}
	private:
		std::streambuf * sb1;
		std::streambuf * sb2;
};

int main(int argc, char* argv[]) {
	std::ofstream log_file("out.log");
	std::streambuf * _cout = std::cout.rdbuf();
	std::streambuf * _cerr = std::cerr.rdbuf();
	TeeBuffer cout_tee(_cout, log_file.rdbuf());
	TeeBuffer cerr_tee(_cerr, log_file.rdbuf());
	std::cout.rdbuf(&cout_tee);
	std::cerr.rdbuf(&cerr_tee);

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

    Daemon daemon(port);
    daemon.run();

	std::cout.rdbuf(_cout);
	std::cerr.rdbuf(_cerr);

    return EXIT_SUCCESS;
}
