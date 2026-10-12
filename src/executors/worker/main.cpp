#include "Engine.h"
#include <iostream>
#include <string>
#include <cstdlib>

int main(int argc, char* argv[]) {
	if (argc < 7) {
		std::cerr << "Usage: " << argv[0] << "<PORT> <THREADS> <TYPE> <TARGET_IP> <TARGET_PORT>\n";
		return EXIT_FAILURE;
	}

	try {
		int port		 = std::stoi(argv[1]);
		int threads		 = std::stoi(argv[2]);
		std::string type = argv[3];
		std::string tip  = argv[4];
		int tport		 = std::stoi(argv[5]);
		long long t0	 = std::stoll(argv[6]);

		std::cout << "[Worker] Starting with PORT=" << port << " THREADS=" 
				  << threads << " TYPE=" << type << " TARGET=" << tip 
				  << ":" << tport << " - T0=" << t0 << std::endl;

		Engine engine(port, threads, type, tip, tport, t0);
		engine.init();
	} catch (const std::exception& e) {
		std::cerr << "[Worker] Error passing arguments: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

    return EXIT_SUCCESS;
}
