from typing import Dict, Any
from .interfaces import IParser


class ParserFactory:
    """
    Factory responsible for creating Parser instances.
    """
    def create_parser(self, config: Dict[str, Any], parser_type: str) -> IParser:
        if parser_type == 'redis':
            from .redis.redis_parser import RedisParser

            granularity = config.get('timestamp_granularity', 6)
            return RedisParser(timestamp_granularity=granularity)

        if parser_type == 'http':
            from .http.http_parser import HttpParser

            granularity = config.get('timestamp_granularity', 6)
            chunk_size = config.get('chunk_size', 50000)

            return HttpParser(
                timestamp_granularity=granularity,
                chunk_size=chunk_size
            )
        
        if parser_type == 'v2i':
            from .v2i.v2i_parser import V2IParser

            granularity = config.get('timestamp_granularity', 6)

            return V2IParser(timestamp_granularity=granularity)

        """ Example for future parsers:
        if parser_type == 'mongodb':
            from .mongodb.mongodb_parser import MongoDBParser

            granularity = config.get('timestamp_granularity', 6)
            return MongoDBParser(timestamp_granularity=granularity)
        """
        raise ValueError(f"Parser of type '{parser_type}' is not supported.")
