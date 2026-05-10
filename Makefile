CXX      := g++
CXXFLAGS := -std=c++17 -Isrc -Ilib
TARGET   := namemark

SRCS := $(wildcard src/*.cpp) \
        $(wildcard src/states/*.cpp) \
        lib/customio23.cpp

OBJS := $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
