.POSIX:

SRC = csand.c nuklear.c platform_glfw.c renderer.c
EMBED_HDR = nuklear.vert.embed.h nuklear.frag.embed.h shader.vert.embed.h shader.frag.embed.h
HDR = math.h nuklear_config.h platform.h random.h renderer.h rgba.h vec2.h x_macros.h ${EMBED_HDR}
OBJ = ${SRC:.c=.o}
LIBS = -lglfw -lGLESv2 -lm

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

nuklear.vert.embed.h: embed nuklear.vert
	./embed < nuklear.vert > $@

nuklear.frag.embed.h: embed nuklear.frag
	./embed < nuklear.frag > $@

.c.o:
	${CC} -c -o $@ $< -Ithird_party/include ${CFLAGS}

${OBJ}: ${HDR}

validate:
	glslangValidator nuklear.vert nuklear.frag shader.vert shader.frag

clean:
	rm -f csand csand.wasm embed ${EMBED_HDR} ${OBJ}

.PHONY: all validate clean
