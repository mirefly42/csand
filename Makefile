.POSIX:

SRC = csand.c platform_glfw.c
HDR = platform.h
OBJ = ${SRC:.c=.o}
LIBS = -lglfw -lepoxy

csand: ${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS} ${LDFLAGS}

all: csand csand.wasm

csand.wasm: csand.c ${HDR}
	clang -o $@ csand.c --target=wasm32 -nostdlib -Wl,--entry=main,--import-undefined,--export-table ${CFLAGS} ${LDFLAGS}

.c.o:
	${CC} -c -o $@ $< ${CFLAGS}

${OBJ}: ${HDR}

validate:
	glslangValidator shader.vert shader.frag

clean:
	rm -f csand csand.wasm ${OBJ}

.PHONY: all validate clean
