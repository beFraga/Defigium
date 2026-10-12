#include "Config.h"
#include "Engine.h"
#include <iostream>
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

int main() {
	std::ofstream log_file("out.log");
	std::streambuf * _cout = std::cout.rdbuf();
	std::streambuf * _cerr = std::cerr.rdbuf();
	TeeBuffer cout_tee(_cout, log_file.rdbuf());
	TeeBuffer cerr_tee(_cerr, log_file.rdbuf());
	std::cout.rdbuf(&cout_tee);
	std::cerr.rdbuf(&cerr_tee);

	Config conf("../../../config.yaml");

	const auto sequence = conf.get_benchmark_sequence();
	if (sequence.empty()) {
		std::cerr << "[Main] No benchmarks to run. Check config.yaml." << std::endl;
		return 1;
	}

	std::cout << "[Main] Running " << sequence.size() << " benchmark(s) sequentially." << std::endl;

	for (size_t i = 0; i < sequence.size(); ++i) {
		const std::string& name = sequence[i];

		std::cout << "\n[Main] ── Benchmark " << (i + 1) << "/" << sequence.size()
		          << ": '" << name << "' ──" << std::endl;

		conf.resolve(name);

		Engine eng(&conf);
		eng.init();
	}

	std::cout << "\n[Main] All benchmarks completed." << std::endl;

	std::cout.rdbuf(_cout);
	std::cerr.rdbuf(_cerr);
	return 0;
}
