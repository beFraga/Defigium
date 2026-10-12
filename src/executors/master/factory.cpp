#include "factory.h"
#include <stdexcept>

#ifdef BUILD_REDIS_STRATEGY
    #include "redis/redis.h"
#endif

#ifdef BUILD_V2I_STRATEGY
	#include "v2i/v2i.h"
#endif

// Example of adding another strategy
#ifdef BUILD_HTTP_STRATEGY
    #include "http/http.h"
#endif


std::unique_ptr<IParserStrategy> ParserFactory::create(const std::string& type) {

    #ifdef BUILD_REDIS_STRATEGY
        if (type == "redis")
            return std::make_unique<RedisParserStrategy>();
    #endif
	
	#ifdef BUILD_V2I_STRATEGY
		if (type == "v2i")
			return std::make_unique<V2IParserStrategy>();
	#endif

    // Example of adding another strategy
    #ifdef BUILD_HTTP_STRATEGY
        if (type == "http")
            return std::make_unique<HttpParserStrategy>();
    #endif

    throw std::runtime_error("Executor type not recognized " + type);
}
