.POSIX:

SRC = csand.c
OBJ = ${SRC:.c=.o}
LIBS = -lglfw -lepoxy

all: csand

csand: ${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS} ${LDFLAGS}

.c.o:
	${CC} -c -o $@ $< ${CFLAGS}

validate:
	glslangValidator shader.vert shader.frag

clean:
	rm -f csand ${OBJ}

.PHONY: all validate clean
