CXX      := g++
CXXFLAGS := -std=c++17 -Isrc -Ilib

ifeq ($(OS),Windows_NT)
    TARGET := namemark.exe
    RM     := del /q
else
    TARGET := namemark
    RM     := rm -f
endif

SRCS := $(wildcard src/*.cpp) \
        $(wildcard src/states/*.cpp) \
        lib/customio23.cpp

OBJS := $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY: clean
