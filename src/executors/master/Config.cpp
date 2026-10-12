#include "Config.h"
#include <yaml-cpp/yaml.h>

// ---------------------------------------------------------------------------
// deep_merge
//
// Recursively overlays `override` on top of `base` (both YAML::Node Maps).
// Scalar / Sequence nodes in `override` replace those in `base`; Map nodes
// are merged key-by-key so that unmentioned keys are inherited from `base`.
// ---------------------------------------------------------------------------
static YAML::Node deep_merge(const YAML::Node& base, const YAML::Node& override) {
	YAML::Node result = YAML::Clone(base);

	if (!override.IsMap() || !base.IsMap())
		return override;

	for (const auto& kv : override) {
		const std::string key = kv.first.as<std::string>();
		const YAML::Node& ov  = kv.second;

		if (result[key] && result[key].IsMap() && ov.IsMap())
			result[key] = deep_merge(result[key], ov);
		else
			result[key] = YAML::Clone(ov);
	}
	return result;
}

// ---------------------------------------------------------------------------
// _apply  –  extract all runtime fields from an already-merged YAML node
// ---------------------------------------------------------------------------
void Config::_apply(const YAML::Node& config) {
	_nodes.clear();

	_bin_path = config["binary_dir"].as<std::string>();

	const auto& pipeline_config = config["pipeline"];
	const auto& workers         = config["cluster"]["workers"];
	const auto& target          = config["cluster"]["target"];
	const auto& executor_config = config["components"]["executor"];

	const std::string input_log_path  = pipeline_config["generator_log_file"].as<std::string>();
	const std::string number_log_path = pipeline_config["executor_log_file"].as<std::string>();
	_file_path = "../../../" + filepath_concat(input_log_path, number_log_path);

	for (const auto& worker : workers)
		_nodes.push_back(Node(worker[0].as<std::string>(), worker[1].as<int>()));

	_type        = target["type"].as<std::string>();
	_num_threads = executor_config["num_threads"].as<int>();
	_target      = Node(target["host"].as<std::string>(), target["port"].as<int>());
}

// ---------------------------------------------------------------------------
// Constructor  –  loads the file, builds the sequence, resolves first entry
// ---------------------------------------------------------------------------
Config::Config(const std::string& path_config) : _path_config(path_config) {
	try {
		_raw = YAML::LoadFile(_path_config);
	} catch (const std::exception& e) {
		std::cerr << "[Config] Error loading '" << _path_config
		          << "': " << e.what() << std::endl;
		return;
	}

	// ── Build benchmark sequence ────────────────────────────────────────────
	if (_raw["benchmark_sequence"] && _raw["benchmark_sequence"].IsSequence()) {
		for (const auto& entry : _raw["benchmark_sequence"])
			_benchmark_sequence.push_back(entry.as<std::string>());
	}

	// Fall back to active_benchmark when sequence is absent / empty.
	if (_benchmark_sequence.empty() && _raw["active_benchmark"])
		_benchmark_sequence.push_back(_raw["active_benchmark"].as<std::string>());

	// Resolve the first benchmark so the object is immediately usable.
	if (!_benchmark_sequence.empty())
		resolve(_benchmark_sequence[0]);
}

// ---------------------------------------------------------------------------
// resolve  –  switch to a named benchmark without re-reading the file
// ---------------------------------------------------------------------------
void Config::resolve(const std::string& benchmark_name) {
	YAML::Node defaults  = _raw["defaults"];
	YAML::Node config;

	if (_raw["benchmarks"] && _raw["benchmarks"][benchmark_name]) {
		std::cout << "[Config] Resolving benchmark: '" << benchmark_name << "'" << std::endl;
		config = deep_merge(defaults, _raw["benchmarks"][benchmark_name]);
	} else {
		// Backward-compatible: old flat config or unknown name → use defaults.
		if (!benchmark_name.empty())
			std::cerr << "[Config] WARNING: benchmark '" << benchmark_name
			          << "' not found – falling back to defaults." << std::endl;
		config = _raw["defaults"] ? YAML::Clone(defaults) : YAML::Clone(_raw);
	}

	_apply(config);
}

// ---------------------------------------------------------------------------
// filepath_concat
// ---------------------------------------------------------------------------
std::string Config::filepath_concat(const std::string base_path, const std::string number) {
	size_t last_dot = base_path.rfind('.');
	std::string name = base_path;
	std::string ext  = "";
	if (last_dot != std::string::npos && last_dot != 0) {
		name = base_path.substr(0, last_dot);
		ext  = base_path.substr(last_dot);
		return name + '_' + number + ext;
	}
	return base_path;
}
