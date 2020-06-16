#±àÒëÆ÷Ãû³Æ
CC=mpic++


h1=./include/


h2=./src/flowdir/


h3=./

CFLAGS=-I$(h1) -I$(h2) -I$(h3) -std=c++11 -fpermissive -O3
LIBS=-lm -lgdal 
#.hÎÄ¼þ
DEPS=$(shell find ./src/flowdir/ -name "*.h") $(shell find ./include/paradem/ -name "*.h") $(shell find ./ -name "*.hpp")

src=$(shell find ./src/common/ -name "*.cpp") $(shell find ./src/flowdir/ -name "*.cpp")

OBJ = $(src:%.cpp=%.o)

%.o : %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

flowdirPara:$(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -rf $(OBJ) flowdirPara
