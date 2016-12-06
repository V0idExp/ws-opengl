CFLAGS := $(CFLAGS) -std=c99 -Wall -Werror -g -DDEBUG `sdl2-config --cflags` `pkg-config --cflags glew assimp`
LDFLAGS := $(LDFLAGS) `sdl2-config --libs` `pkg-config --libs glew assimp`
OS := $(shell uname -s)
OBJS = main.o matlib.o ioutils.o objloader.o

ifeq ($(OS), Linux)
	LDFLAGS += -lm -lblas
else ifeq ($(OS), Darwin)
	LDFLAGS += -framework OpenGL -framework Accelerate
endif

all: demo

demo: $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	rm -fv $(OBJS) demo
