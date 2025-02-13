.POSIX:

SRC = csand.c platform_glfw.c renderer.c
EMBED_HDR = shader.vert.embed.h shader.frag.embed.h
HDR = math.h platform.h random.h renderer.h rgba.h vec2.h ${EMBED_HDR}
OBJ = ${SRC:.c=.o}
LIBS = -lglfw -lGLESv2

csand: ${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS} ${LDFLAGS}

all: csand csand.wasm

csand.wasm: csand.c renderer.c ${HDR}
	clang -o $@ csand.c renderer.c --target=wasm32 -nostdlib -Wl,--entry=main,--import-undefined,--export-table -Ithird_party/web/include ${CFLAGS} ${LDFLAGS}

embed: embed.c
	${CC} embed.c -o $@

shader.vert.embed.h: embed shader.vert
	./embed < shader.vert > $@

shader.frag.embed.h: embed shader.frag
	./embed < shader.frag > $@

.c.o:
	${CC} -c -o $@ $< ${CFLAGS}

${OBJ}: ${HDR}

validate:
	glslangValidator shader.vert shader.frag

clean:
	rm -f csand csand.wasm embed ${EMBED_HDR} ${OBJ}

.PHONY: all validate clean
