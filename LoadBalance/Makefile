TARGET := AgentServer

MUDUOSRC := ../SimplifyMuduo/src/
PROTOSRC := ../Proto/
LBSRC := ./src/
MUDUOINC := ../SimplifyMuduo/include/
PROTOINC := ../Proto/
LBINC := ./include/
LIB := -lpthread -lmysqlclient -lprotobuf

CXX = g++
CPPFLAGS = -g -O2 -Wall -std=c++11
CPPFLAGS += $(addprefix -I,$(MUDUOINC))
CPPFLAGS += $(addprefix -I,$(PROTOINC))
CPPFLAGS += $(addprefix -I,$(LBINC))

FILETYPE = cc
OBJS := $(MUDUOSRC)*.$(FILETYPE)
OBJS += $(PROTOSRC)*.$(FILETYPE)
OBJS += $(LBSRC)*.$(FILETYPE)
#OBJS := $(addsuffix .o, $(basename $(wildcard $(SRC)*.$(FILETYPE))))
#objs : $(OBJS)

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CPPFLAGS) -o $(TARGET) $(OBJS) $(LIB)

clean:
	-rm -f $(TARGET)
