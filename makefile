CXX = g++
CXXFLAGS = -std=c++17 `pkg-config --cflags opencv4`
LDFLAGS = `pkg-config --libs opencv4`
TARGET = main
SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

# Phony targets
.PHONY: all clean
