# Compiler
CXX = g++

# Base flags
CXXFLAGS = -std=c++17 -I./ -Wall -I/usr/local/include

# Base libs
LDFLAGS = -L/usr/local/lib
LIBS = -lpthread

THIRD_PARTY = ../../../third_party

JIT_MODULES    = 
JIT_MAKE_DIRS  =
JIT_CMAKE_DIRS = 
JIT_INCLUDES   =
JIT_LIB_DIRS   =
JIT_LINK_FLAGS =

# 2. Strategy-Specific Dependencies
ifdef TYPE
    TYPE_UPPER = $(shell echo $(TYPE) | tr '[:lower:]' '[:upper:]')
    CXXFLAGS += -DBUILD_$(TYPE_UPPER)_STRATEGY

    ifeq ($(TYPE), redis)
        JIT_MODULES    += hiredis redis-plus-plus
        JIT_MAKE_DIRS  += $(THIRD_PARTY)/hiredis
        JIT_CMAKE_DIRS += $(THIRD_PARTY)/redis-plus-plus
        JIT_INCLUDES   += $(THIRD_PARTY)/hiredis $(THIRD_PARTY)/redis-plus-plus/src
        JIT_LIB_DIRS   += $(THIRD_PARTY)/hiredis $(THIRD_PARTY)/redis-plus-plus/build
        JIT_LINK_FLAGS += -lredis++ -lhiredis
    endif

    ifeq ($(TYPE), http)
        JIT_MODULES    += curl
        JIT_CMAKE_DIRS += $(THIRD_PARTY)/curl
        JIT_INCLUDES   += $(THIRD_PARTY)/curl/include
        JIT_LIB_DIRS   += $(THIRD_PARTY)/curl/build/lib
        JIT_LINK_FLAGS += -lcurl
    endif
endif

# 3. JIT Fetching
ifneq ($(strip $(JIT_MODULES)),)
    FETCH_CMD := $(foreach mod, $(JIT_MODULES), $(shell git submodule update --init $(THIRD_PARTY)/$(mod)))
endif

# 4. JIT Compilation (Handles both Make and CMake natively)
ifneq ($(strip $(JIT_MAKE_DIRS)),)
    MAKE_CMD := $(foreach bdir, $(JIT_MAKE_DIRS), $(shell $(MAKE) -C $(bdir) > /dev/null 2>&1))
endif

ifneq ($(strip $(JIT_CMAKE_DIRS)),)
    # Automatically creates a build folder, runs cmake, and compiles for CMake-based modules
    CMAKE_CMD := $(foreach cdir, $(JIT_CMAKE_DIRS), $(shell cd $(cdir) && mkdir -p build && cd build && cmake .. > /dev/null 2>&1 && $(MAKE) > /dev/null 2>&1))
endif

# 5. Append dynamically resolved paths
CXXFLAGS += $(foreach inc, $(JIT_INCLUDES), -I$(inc))
LIBS     += $(foreach libdir, $(JIT_LIB_DIRS), -L$(libdir)) $(JIT_LINK_FLAGS)

# Target executable name
TARGET = worker

# Base files
SOURCES = $(shell find . -type f -name "*.cpp")
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "--- Linkando o executável '$(TARGET)' ---"
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)
	@echo "--- Executável '$(TARGET)' criado com sucesso! ---"

%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compilando $< com flags: $(CXXFLAGS)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "--- Limpando arquivos de build ---"
	find . -type f -name "*.o" -delete
	rm -f $(TARGET)
	@echo "--- Limpeza concluída ---"

run: ($TARGET)
	@echo "--- Executando '$(TARGET)' com os parametros do Daemon ---"
	./$(TARGET) $(PORT) $(THREADS) $(TYPE) $(TIP) $(TPORT)
