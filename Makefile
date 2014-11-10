CXX=g++
RM=-rm -f
CPPFLAGS=-std=c++11
LDFLAGS=

SRCS=IBLT_testing.cpp
OBJS=$(SRCS:.cpp=.o)
TARGET=test

default: $(TARGET)
all: default

%.o: %.cpp 
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $< -o $@

clean:
	$(RM) *.o
	$(RM) $(TARGET)


