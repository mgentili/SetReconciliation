CXX=g++
RM=-rm -f
CPPFLAGS=-std=c++11 -Wall
LDFLAGS=
BASIC_IBLT_SRCS=basicIBLT_testing.cpp
MULTI_IBLT_SRCS=multiIBLT_testing.cpp
TABULATION_SRCS=tabulation_testing.cpp
BASIC_FIELD_SRCS=field_testing.cpp
FINGERPRINT_SRCS=fingerprint_testing.cpp
FILE_SYNC_SRCS=file_sync_testing.cpp
SRCS=$(BASIC_IBLT_SRCS) $(MULTI_IBLT_SRCS) $(TABULATION_SRCS) $(BASIC_FIELD_SRCS) $(FINGERPRINT_SRCS) $(FILE_SYNC_SRCS)
OBJS=$(SRCS:%.cpp=obj/%.o)

BASIC_IBLT=bin/basic_testing
MULTI_IBLT=bin/multi_testing
TABULATION=bin/tabulation_testing
BASIC_FIELD=bin/field_testing
FINGERPRINT=bin/fingerprint_testing
SYNC=bin/file_sync_testing
PROGRAMS=$(BASIC_IBLT) $(MULTI_IBLT) $(TABULATION) $(BASIC_FIELD) $(FINGERPRINT) $(SYNC)

default: all
all: $(PROGRAMS)
tabulation: $(TABULATION)
basic_iblt: $(BASIC_IBLT)
multi_iblt: $(MULTI_IBLT)
field: $(BASIC_FIELD)
fingerprint: $(FINGERPRINT)
sync: $(SYNC)

obj/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) -c -MMD -MP $< -o $@

DEPS=$(OBJS:%.o=%.d)

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

$(SYNC): $(FILE_SYNC_SRCS:%.cpp=obj/%.o)
	$(CXX) $(LDFLAGS) $< -o $@

clean:
	$(RM) $(OBJS) $(PROGRAMS) $(DEPS)

-include $(DEPS)