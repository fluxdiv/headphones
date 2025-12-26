CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++23

TARGET = listen
SRCS   = main.cpp eth_lookup.cpp
OBJS   = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm *.o $(TARGET)

