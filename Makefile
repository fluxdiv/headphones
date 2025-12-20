CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17

TARGET = listener
SRCS   = main.cpp listener.cpp
OBJS   = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm *.o $(TARGET)

