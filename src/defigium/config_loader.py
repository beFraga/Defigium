import yaml
from typing import Dict, Any


def load_config(path: str = 'config.yaml') -> Dict[str, Any]:
    """
    Load the YAML config file and return the Python-relevant section.

    Benchmark selection (active_benchmark resolution + deep-merge) is the
    responsibility of the C++ master (Config.cpp).  Python only needs the
    pipeline and component settings that drive log parsing and synthesis,
    which live under ``defaults``.

    For backward-compatibility with old flat config files that have no
    ``defaults`` block, the root node is returned as-is.
    """
    print(f"Loading configuration from '{path}'...")
    with open(path, 'r', encoding='utf-8') as f:
        raw = yaml.safe_load(f)

    # New-style config: read Python-relevant settings from `defaults`.
    if 'defaults' in raw:
        return raw['defaults']

    # Old-style flat config: return root directly.
    return raw
