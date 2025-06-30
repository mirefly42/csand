import {
    getString,
    getNullTerminatedString,
    setNullTerminatedString,
    setUint32Le,
    setInt32Le,
    Uint32LeArray
} from "./memutils.js";

class HandlePool {
    #objects = [null];

    allocHandle(object) {
        const handle = this.#objects.length;
        this.#objects.push(object);
        return handle;
    }

    // A stub function that isn't doing anything yet.
    freeHandle(handle) {}

    derefHandle(handle) {
        return this.#objects[handle];
    }
}

export class GLES2Context {
    gl;
    memory;
    static #api = {
        glBlendEquation(mode) {
            this.gl.blendEquation(mode);
        },

        glBlendFunc(sfactor, dfactor) {
            this.gl.blendFunc(sfactor, dfactor);
        },

        glEnable(cap) {
            this.gl.enable(cap);
        },

        glDisable(cap) {
            this.gl.disable(cap);
        },

        glViewport(x, y, width, height) {
            this.gl.viewport(x, y, width, height);
        },

        glClearColor(r, g, b, a) {
            this.gl.clearColor(r, g, b, a);
        },

        glClear(mask) {
            this.gl.clear(mask);
        },

        glGenBuffers(n, buffers_ptr) {
            if (n < 0) {
                return; // TODO: should generate GL_INVALID_VALUE
            }
            const buffers = new Uint32LeArray(this.memory.buffer, buffers_ptr, n);
            for (let i = 0; i < n; i++) {
                const handle = this.#buffers.allocHandle(this.gl.createBuffer());
                buffers.set(i, handle);
            }
        },

        glBindBuffer(target, buffer_handle) {
            this.gl.bindBuffer(target, this.#buffers.derefHandle(buffer_handle));
        },

        glBufferData(target, size, data_ptr, usage) {
            if (data_ptr === 0) {
                this.gl.bufferData(target, size, usage);
                return;
            }

            this.gl.bufferData(target, this.memory.buffer.slice(data_ptr, data_ptr + size), usage);
        },

        glBufferSubData(target, offset, size, data_ptr) {
            this.gl.bufferSubData(target, offset, this.memory.buffer.slice(data_ptr, data_ptr + size));
        },

        glGetAttribLocation(program_handle, name_ptr) {
            return this.gl.getAttribLocation(this.#programs.derefHandle(program_handle), getNullTerminatedString(this.memory.buffer, name_ptr));
        },

        glVertexAttribPointer(index, size, type, normalized, stride, pointer) {
            this.gl.vertexAttribPointer(index, size, type, normalized, stride, pointer);
        },

        glEnableVertexAttribArray(index) {
            this.gl.enableVertexAttribArray(index);
        },

        glCreateShader(type) {
            return this.#shaders.allocHandle(this.gl.createShader(type));
        },

        glShaderSource(shader_handle, count, strings_ptr, lengths_ptr) {
            let str = "";
            const buffer = this.memory.buffer;
            const strings = new Uint32LeArray(buffer, strings_ptr, count);
            if (lengths_ptr != 0) {
                const lengths = new Uint32LeArray(buffer, lengths_ptr, count);

                for (let i = 0; i < count; i++) {
                    const str_ptr = strings.get(i);
                    const len = lengths.get(i);
                    str += getString(buffer, str_ptr, len);
                }
            } else {
                for (let i = 0; i < count; i++) {
                    const str_ptr = strings.get(i);
                    str += getNullTerminatedString(this.memory.buffer, str_ptr);
                }
            }
            this.gl.shaderSource(this.#shaders.derefHandle(shader_handle), str);
        },

        glCompileShader(shader_handle) {
            this.gl.compileShader(this.#shaders.derefHandle(shader_handle));
        },

        glGetShaderiv(shader_handle, pname, params_ptr) {
            const value = this.gl.getShaderParameter(this.#shaders.derefHandle(shader_handle), pname);
            setInt32Le(this.memory.buffer, params_ptr, value);
        },

        glGetShaderInfoLog(shader_handle, buf_size, length_ptr, info_log_ptr) {
            const info_log = this.gl.getShaderInfoLog(this.#shaders.derefHandle(shader_handle));
            const length = setNullTerminatedString(this.memory.buffer, info_log_ptr, buf_size, info_log);
            if (length_ptr != 0) {
                setUint32Le(this.memory.buffer, length_ptr, length);
            }
        },

        glDeleteShader(shader_handle) {
            this.gl.deleteShader(this.#shaders.derefHandle(shader_handle));
            this.#shaders.freeHandle(shader_handle);
        },

        glCreateProgram() {
            return this.#programs.allocHandle(this.gl.createProgram());
        },

        glAttachShader(program_handle, shader_handle) {
            this.gl.attachShader(this.#programs.derefHandle(program_handle), this.#shaders.derefHandle(shader_handle));
        },

        glLinkProgram(program_handle) {
            this.gl.linkProgram(this.#programs.derefHandle(program_handle));
        },

        glGetProgramiv(program_handle, pname, params_ptr) {
            const value = this.gl.getProgramParameter(this.#programs.derefHandle(program_handle), pname);
            setInt32Le(this.memory.buffer, params_ptr, value);
        },

        glGetProgramInfoLog(shader_handle, buf_size, length_ptr, info_log_ptr) {
            const info_log = this.gl.getProgramInfoLog(this.#programs.derefHandle(shader_handle));
            const length = setNullTerminatedString(this.memory.buffer, info_log_ptr, buf_size, info_log);
            if (length_ptr != 0) {
                setUint32Le(this.memory.buffer, length_ptr, length);
            }
        },

        glDeleteProgram(program_handle) {
            this.gl.deleteProgram(this.#programs.derefHandle(program_handle));
            this.#programs.freeHandle(program_handle);
        },

        glUseProgram(program_handle) {
            this.gl.useProgram(this.#programs.derefHandle(program_handle));
        },

        glDrawArrays(mode, first, count) {
            this.gl.drawArrays(mode, first, count);
        },

        glDrawElements(mode, count, type, offset) {
            this.gl.drawElements(mode, count, type, offset);
        },

        glGenTextures(n, textures_ptr) {
            if (n < 0) {
                return; // TODO: should generate GL_INVALID_VALUE
            }
            const textures = new Uint32LeArray(this.memory.buffer, textures_ptr, n);
            for (let i = 0; i < n; i++) {
                const handle = this.#textures.allocHandle(this.gl.createTexture());
                textures.set(i, handle);
            }
        },

        glActiveTexture(texture) {
            this.gl.activeTexture(texture);
        },

        glBindTexture(target, texture_handle) {
            this.gl.bindTexture(target, this.#textures.derefHandle(texture_handle));
        },

        glTexParameteri(target, pname, param) {
            this.gl.texParameteri(target, pname, param);
        },

        glGetUniformLocation(program_handle, name_ptr) {
            const program = this.#programs.derefHandle(program_handle);
            const name = getNullTerminatedString(this.memory.buffer, name_ptr);

            const active_uniforms_count = this.gl.getProgramParameter(program, this.gl.ACTIVE_UNIFORMS);
            for (let i = 0; i < active_uniforms_count; i++) {
                const uniform = this.gl.getActiveUniform(program, i);
                if (uniform.name === name) {
                    return i;
                }
            }

            return -1;
        },

        glUniform1i(location_index, value) {
            const program = this.gl.getParameter(this.gl.CURRENT_PROGRAM);
            this.gl.uniform1i(this.#getUniformLocationByIndex(program, location_index), value);
        },

        glUniform2f(location_index, v0, v1) {
            const program = this.gl.getParameter(this.gl.CURRENT_PROGRAM);
            this.gl.uniform2f(this.#getUniformLocationByIndex(program, location_index), v0, v1);
        },

        glScissor(x, y, width, height) {
            this.gl.scissor(x, y, width, height);
        },

        glTexImage2D(target, level, internal_format, width, height, border, format, type, pixels_ptr) {
            let pixels = this.#pixelsFromMemory(width, height, format, type, pixels_ptr);
            this.gl.texImage2D(target, level, internal_format, width, height, border, format, type, pixels);
        },

        glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels_ptr) {
            let pixels = this.#pixelsFromMemory(width, height, format, type, pixels_ptr);
            this.gl.texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
        },
    };

    #buffers = new HandlePool();
    #shaders = new HandlePool();
    #programs = new HandlePool();
    #textures = new HandlePool();

    constructor(gl, memory) {
        this.gl = gl;
        this.memory = memory;
    }

    importIntoObject(object) {
        for (const key in GLES2Context.#api) {
            object[key] = GLES2Context.#api[key].bind(this);
        }
    }

    #getUniformLocationByIndex(program, index) {
        // TODO: probably should cache this.
        const active_uniform = this.gl.getActiveUniform(program, index);
        if (active_uniform === null) {
            return null;
        }

        return this.gl.getUniformLocation(program, active_uniform.name);
    }

    #getChannelsCountFromFormat(format) {
        switch (format) {
            case this.gl.ALPHA:
            case this.gl.LUMINANCE:
                return 1;
            case this.gl.LUMINANCE_ALPHA:
                return 2;
            case this.gl.RGB:
                return 3;
            case this.gl.RGBA:
                return 4;
        }

        // TODO: perhaps should generate some kind of error, but glTex[Sub]Image2D probably gonna do that on its own.
        return 0;
    }

    #pixelsFromMemory(width, height, format, type, pixels_ptr) {
        const channels = this.#getChannelsCountFromFormat(format);

        if (pixels_ptr === 0) {
            return null;
        } else if (type === this.gl.UNSIGNED_BYTE) {
            return new Uint8Array(this.memory.buffer, pixels_ptr, width * height * channels);
        } else if (type === this.gl.UNSIGNED_SHORT_5_6_5 || type === UNSIGNED_SHORT_4_4_4_4 || type === UNSIGNED_SHORT_5_5_5_1) {
            throw new Error("not implemented");
        }
    }
}
