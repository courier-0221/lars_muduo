TARGET := AgentClientDemo

MUDUOSRC := ../../../SimplifyMuduo/src/
PROTOSRC := ../../../Proto/
AGENTCLIENTSRC := ./
MUDUOINC := ../../../SimplifyMuduo/include/
PROTOINC := ../../../Proto/
AGENTCLIENTINC := ./
LIB := -lpthread -lprotobuf

CXX = g++
CPPFLAGS = -g -O2 -Wall -std=c++11
CPPFLAGS += $(addprefix -I,$(MUDUOINC))
CPPFLAGS += $(addprefix -I,$(AGENTCLIENTINC))
CPPFLAGS += $(addprefix -I,$(PROTOINC))

FILETYPE = cc
OBJS := $(MUDUOSRC)*.$(FILETYPE)
OBJS += $(PROTOSRC)*.$(FILETYPE)
OBJS += $(AGENTCLIENTSRC)*.$(FILETYPE)
#OBJS := $(addsuffix .o, $(basename $(wildcard $(SRC)*.$(FILETYPE))))
#objs : $(OBJS)

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CPPFLAGS) -o $(TARGET) $(OBJS) $(LIB)

clean:
	-rm -f $(TARGET)
