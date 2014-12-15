CXX=g++
RM=-rm -f
CPPFLAGS=-std=c++11 -Wall
LDFLAGS=
BASIC_IBLT_SRCS=basicIBLT_testing.cpp
MULTI_IBLT_SRCS=multiIBLT_testing.cpp
TABULATION_SRCS=tabulation_testing.cpp
BASIC_FIELD_SRCS=field_testing.cpp
FINGERPRINT_SRCS=fingerprint_testing.cpp
SRCS=$(BASIC_IBLT_SRCS) $(MULTI_IBLT_SRCS) $(TABULATION_SRCS) $(BASIC_FIELD_SRCS) $(FINGERPRINT_SRCS)
OBJS=$(SRCS:%.cpp=obj/%.o)

BASIC_IBLT=bin/basic_testing
MULTI_IBLT=bin/multi_testing
TABULATION=bin/tabulation_testing
BASIC_FIELD=bin/field_testing
FINGERPRINT=bin/fingerprint_testing
PROGRAMS=$(BASIC_IBLT) $(MULTI_IBLT) $(TABULATION) $(BASIC_FIELD) $(FINGERPRINT)

default: all
all: $(PROGRAMS)
tabulation: $(TABULATION)
basic_iblt: $(BASIC_IBLT)
multi_iblt: $(MULTI_IBLT)
field: $(BASIC_FIELD)
fingerprint: $(FINGERPRINT)
#lala:
#	@echo $(DEPS)

DEPS=$(OBJS:%.o=%.d)

obj/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) -c -MMD -MP $< -o $@

#obj/%.d: src/%.cpp
#	$(CC) $(CFLAGS) -M -MF $@ -MT $(@:%.d=%.o) $<

$(BASIC_IBLT): $(BASIC_IBLT_SRCS:%.cpp=obj/%.o)
	$(CXX) $(LDFLAGS) $< -o $@

$(MULTI_IBLT): $(MULTI_IBLT_SRCS:%.cpp=obj/%.o)
	$(CXX) $(LDFLAGS) $< -o $@

$(TABULATION): $(TABULATION_SRCS:%.cpp=obj/%.o)
	$(CXX) $(LDFLAGS) $< -o $@

$(BASIC_FIELD): $(BASIC_FIELD_SRCS:%.cpp=obj/%.o)
	$(CXX) $(LDFLAGS) $< -o $@

$(FINGERPRINT): $(FINGERPRINT_SRCS:%.cpp=obj/%.o)
	$(CXX) $(LDFLAGS) $< -o $@

clean:
	$(RM) $(OBJS) $(PROGRAMS) $(DEPS)

include $(DEPS)

# clean:
# 	$(RM) $(OBJS) $(PROGRAMS)
# 	$(RM) $(PROGRAMS)

# #AUTO-GENERATE HEADER DEPENDENCIES
# depend: .depend

# .depend: $(SRCS)
# 	rm -f ./.depend
# 	$(CXX) $(CPPFLAGS) -MM $^ > ./.depend;

# include .depend

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