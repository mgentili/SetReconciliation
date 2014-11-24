CXX=g++
RM=-rm -f
CPPFLAGS=-std=c++11 -Wall
LDFLAGS=

BASIC_IBLT_SRCS=src/basicIBLT_testing.cpp
MULTI_IBLT_SRCS=src/multiIBLT_testing.cpp
TABULATION_SRCS=src/tabulation_testing.cpp
BASIC_FIELD_SRCS=src/field_testing.cpp
SRCS=$(BASIC_IBLT_SRCS) $(MULTI_IBLT_SRCS) $(TABULATION_SRCS) $(BASIC_FIELD_SRCS)

BASIC_IBLT=bin/basic_testing
MULTI_IBLT=bin/multi_testing
TABULATION=bin/tabulation_testing
BASIC_FIELD=bin/field_testing

PROGRAMS=$(BASIC_IBLT) $(MULTI_IBLT) $(TABULATION) $(BASIC_FIELD)

default: all
all: $(PROGRAMS)

tabulation: $(TABULATION)
basic_iblt: $(BASIC_IBLT)
multi_iblt: $(MULTI_IBLT)
field: $(BASIC_FIELD)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(BASIC_IBLT): $(BASIC_IBLT_SRCS:.cpp=.o)
	$(CXX) $(LDFLAGS) $< -o $@

$(MULTI_IBLT): $(MULTI_IBLT_SRCS:.cpp=.o)
	$(CXX) $(LDFLAGS) $< -o $@

$(TABULATION): $(TABULATION_SRCS:.cpp=.o)
	$(CXX) $(LDFLAGS) $< -o $@

$(BASIC_FIELD): $(BASIC_FIELD_SRCS:.cpp=.o)
	$(CXX) $(LDFLAGS) $< -o $@

clean:
	$(RM) src/*.o
	$(RM) $(PROGRAMS)

#AUTO-GENERATE HEADER DEPENDENCIES
depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) -MM $^ > ./.depend;

include .depend

# # The final executable
# TARGET := something
# # Source files (without src/)
# INPUTS := foo.c bar.c baz.c

# # OBJECTS will contain: obj/foo.o obj/bar.o obj/baz.o
# OBJECTS := $(INPUTS:%.cpp=obj/%.o)
# # DEPFILES will contain: obj/foo.d obj/bar.d obj/baz.d
# DEPFILES := $(OBJECTS:%.o=%.d)

# all: $(TARGET)

# obj/%.o: src/%.cpp
#     $(CC) $(CFLAGS) -c -o $@ $<

# obj/%.d: src/%.cpp
#     $(CC) $(CFLAGS) -M -MF $@ -MT $(@:%.d=%.o) $<

# $(TARGET): $(OBJECTS)
#     $(LD) $(LDFLAGS) -o $@ $(OBJECTS)

# .PHONY: clean
# clean:
#     -rm -f $(OBJECTS) $(DEPFILES) $(RPOFILES) $(TARGET)

# -include $(DEPFILES)