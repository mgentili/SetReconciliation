CXX=g++
RM=-rm -f
CPPFLAGS=-std=c++11 -Wall
LDFLAGS=

SRCS=IBLT_testing.cpp
DEPS=basicIBLT.hpp MurmurHash2.hpp
OBJS=$(SRCS:.cpp=.o)
TARGET=test

default: $(TARGET)
all: default

%.o: %.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $< -o $@

clean:
	$(RM) *.o
	$(RM) $(TARGET)

# depend: .depend

# .depend: $(SRCS)
# 	rm -f ./.depend
# 	$(CXX) $(CPPFLAGS) -MM $^ -MF  ./.depend;

# include .depend
