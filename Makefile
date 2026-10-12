CXX ?= c++
CPPFLAGS += -Iinclude
CXXFLAGS ?= -O2 -std=c++20 -Wall -Wextra -Wpedantic
BUILD ?= build
.PHONY: all test clean sanitize
all: $(BUILD)/audiofx-render $(BUILD)/audiofx-tests
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/engine.o: src/engine.cpp include/audiofx/engine.hpp include/audiofx/dsp.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(BUILD)/audiofx-render: tools/render.cpp $(BUILD)/engine.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
$(BUILD)/audiofx-tests: tests/test_engine.cpp $(BUILD)/engine.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
test: all
	$(BUILD)/audiofx-tests
	python3 tests/oracles.py $(BUILD)/audiofx-render
sanitize:
	$(MAKE) BUILD=build-sanitize CXXFLAGS='-O1 -g -std=c++20 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer' LDFLAGS='-fsanitize=address,undefined' all
	ASAN_OPTIONS=detect_leaks=0 build-sanitize/audiofx-tests
clean:
	rm -rf build build-sanitize
