import random
import re
import string
import sys
from datetime import datetime
from typing import Iterator, List, Optional, Tuple
from ...models.fei import FEIEvent
from ..interfaces import IParser


class V2IParser(IParser):
    """
    Parser for V2I log files. Converts raw V2I log lines into
    FEIEvent domain objects, formats events back to raw log string representations,
    and supports synthetic argument generation.
    """
    _LOG_HEADER_REGEX = re.compile(
            r'^(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})\s+(?:SmartData\\Backend>\s+\[([^\]]+)\]\s+)?(.*)$'
            )
    _HTTP_REGEX = re.compile(r'^HTTPS:([^\s]+)\s+(.*)$')
    _TRACKER_REGEX = re.compile(r'^Creating Tracker\s*:\s*\{(.*)\}$')
    _TYPE_REGEX = re.compile(r'^Type:\s*(\d+)\s+isDigital:\s*(\d*)\s+unit:\s*(\d+)\s+dev:\s*(\d+)\s+sig:\s*(\S+)$')

    _OPERATION_SEMANTICS = {
            "HTTPS:CREATE.PHP": ["CREATE"],
            "HTTPS:PUT.PHP":    ["UPDATE"],
            "HTTPS:LIST.PHP":   ["READ"],
            "TRACKER":          ["CREATE"],
            "METRIC":           ["READ"],
            }
    _DEFAULT_SEMANTIC_TYPE = ["READ"]

    def __init__(self, timestamp_granularity: int):
        self.timestamp_granularity = timestamp_granularity

    def _datetime_to_timestamp(self, dt_str: str) -> float:
        dt = datetime.strptime(dt_str, "%Y-%m-%d %H:%M:%S")
        return dt.timestamp()

    def _timestamp_to_datetime(self, timestamp: float) -> str:
        dt = datetime.fromtimestamp(timestamp)
        return dt.strftime("%Y-%m-%d %H:%M:%S")

    def _parse_line_to_fei(self, line: str) -> Optional[FEIEvent]:
        match = self._LOG_HEADER_REGEX.match(line.strip())
        if not match:
            return None

        try:
            dt_str, client_id, payload = match.groups()
            timestamp = round(self._datetime_to_timestamp(dt_str), self.timestamp_granularity)

            op_type, target, raw_args = self._extract_event_details(payload)
            if not op_type:
                return None

            semantic_type = self._OPERATION_SEMANTICS.get(op_type, self._DEFAULT_SEMANTIC_TYPE)

            return FEIEvent(
                    timestamp=timestamp,
                    client_id=client_id,
                    op_type=op_type,
                    semantic_type=semantic_type,
                    target=target,
                    additional_data={'raw_args': raw_args}
                    )
        except (ValueError, IndexError, Exception) as e:
            print(f"[WARN] Error parsing line '{line.strip()}': {e}", file=sys.stderr)
            return None

    def _extract_event_details(self, payload: str) -> Tuple[Optional[str], str, List[str]]:
        """Parses the payload portion of the line to extract op_type, target and raw arguments."""
        http_match = self._HTTP_REGEX.match(payload)
        if http_match:
            endpoint, rest = http_match.groups()
            op_type = f"HTTPS:{endpoint.upper()}"
            raw_args = rest.split()
            target = raw_args[0] if raw_args else endpoint
            return op_type, target, raw_args

        tracker_match = self._TRACKER_REGEX.match(payload)
        if tracker_match:
            tracker_body = tracker_match.group(1)
            op_type = "TRACKER"
            raw_args = [arg.strip() for arg in tracker_body.split(',')]
            target = "Tracker"
            return op_type, target, raw_args

        type_match = self._TYPE_REGEX.match(payload)
        if type_match:
            type_val, is_digital, unit, dev, sig = type_match.groups()
            op_type = "METRIC"
            raw_args = [f"type={type_val}", f"isDigital={is_digital}", f"dev={dev}", f"sig={sig}"]
            target = f"unit:{unit}"
            return op_type, target, raw_args

        return None, "", []

    def format(self, event: FEIEvent) -> str:
        """Formats an FEIEvent into a raw V2I log line."""
        dt_str = self._timestamp_to_datetime(event['timestamp'])
        client_id = event['client_id']
        op_type = event['op_type']
        raw_args = event['additional_data'].get('raw_args', [])
        args_str = " ".join(raw_args)

        if op_type.startswith("HTTPS:"):
            endpoint = op_type.split(":", 1)[1].lower()
            return f"{dt_str} SmartData\\Backend> [{client_id}] HTTPS:{endpoint}  {args_str}".strip()
        elif op_type == "TRACKER":
            return f"{dt_str} SmartData\\Backend> [{client_id}] Creating Tracker : {{{', '.join(raw_args)}}}"
        else:
            return f"{dt_str} SmartData\\Backend> [{client_id}] {op_type} {args_str}".strip()

    def generate_args(self, op_type: str, target: str, available_pool: List[str]) -> List[str]:
        """Generates realistic synthetic arguments for V2I operations."""

        pool = list(available_pool) if available_pool else []

        if op_type.startswith("HTTPS:"):
            crt = ''.join(random.choices(string.hexdigits.upper(), k=40))
            crd_id = random.choice(pool) if pool else "smartdatatest1784210208"
            return [f"crt:{{{crt}}}", f"crd:{{smartdata@{crd_id}}}"]

        elif op_type == "TRACKER":
            sig = random.choice(pool) if pool else "sig1"
            unit = random.randint(1000000000, 9999999999)
            return [
                    f"v={random.randint(1, 20)}",
                    f"u={{{unit}, SI, n=0, m=0, s^1}}",
                    "x=0", "y=0", "z=0", "r=100",
                    f"t0={random.randint(0, 50)}",
                    f"t1={random.randint(51, 200)}",
                    "type=OLD", "period=0", "count=", "uncertainty=-1", "event=",
                    f"d={random.randint(0, 255)}",
                    f"sig={sig}"
                    ]

        return []

    def parse(self, file_path: str) -> Iterator[FEIEvent]:
        """Reads a log file and yields a stream of FEIEvent objects."""
        with open(file_path, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                if not line.strip():
                    continue
                event = self._parse_line_to_fei(line)
                if event:
                    yield event
                else:
                    print(f"[WARN] Line {line_num} was skipped due to parsing error.", file=sys.stderr)
