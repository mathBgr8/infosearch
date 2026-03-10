CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Wpedantic
LDFLAGS = -lstdc++fs

SOURCES = main.cpp indexer.cpp tokenizer.cpp stemmer.cpp
HEADERS = indexer.h tokenizer.h stemmer.h
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = bool_search

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

# Для Windows (MinGW или MSVC)
# Если используете MSVC, раскомментируйте:
# CXX = cl
# CXXFLAGS = /EHsc /O2
# LDFLAGS =