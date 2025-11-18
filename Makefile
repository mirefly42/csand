.POSIX:

COMMON_SRC = csand.c nuklear.c renderer.c
SRC = ${COMMON_SRC} platform_glfw.c
EMBED_HDR = glow.frag.embed.h nuklear.vert.embed.h nuklear.frag.embed.h shader.vert.embed.h shader.frag.embed.h
HDR = math.h nuklear_config.h platform.h random.h renderer.h rgba.h vec2.h x_macros.h ${EMBED_HDR}
OBJ = ${SRC:.c=.o}
LIBS = -lglfw -lGLESv2 -lm

csand: ${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS} ${LDFLAGS}

all: csand csand.wasm

csand.wasm: ${COMMON_SRC} wasm_libc.c ${HDR}
	clang -o $@ ${COMMON_SRC} wasm_libc.c --target=wasm32 -nostdlib -Wl,--entry=main,--import-undefined,--export-table -Ithird_party/include -Ithird_party/web/include ${CFLAGS} ${LDFLAGS}

embed: embed.c
	${CC} embed.c -o $@

glow.frag.embed.h: embed glow.frag
	./embed < glow.frag > $@

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
