const width = 128;
const height = 64;
let function_table = null;
let input_callback = null;
let render_callback = null;
let canvas = null;
let gl = null;
let vertex_src = null;
let fragment_src = null;
let obj = null;
let buf_ptr = null;
let buf = null;
let mouse_down = false;
let mouse_x = 0;
let mouse_y = 0;
let program = null;

async function main() {
    vertex_src = await (await fetch("shader.vert")).text();
    fragment_src = await (await fetch("shader.frag")).text();

    const import_object = {
        env: {
            csandPlatformInit: csandPlatformInit,
            csandPlatformSetPalette: csandPlatformSetPalette,
            csandPlatformSetInputCallback: (callback_index) => {
                input_callback = function_table.get(callback_index);
                document.addEventListener("keydown", (event) => {
                    const c = event.code;
                    if (c === "Digit0") {
                        input_callback(0);
                    } else if (c === "Digit1") {
                        input_callback(1);
                    } else if (c === "Digit2") {
                        input_callback(2);
                    } else if (c === "Digit3") {
                        input_callback(3);
                    } else if (c === "Digit4") {
                        input_callback(4);
                    } else if (c === "Digit5") {
                        input_callback(5);
                    } else if (c === "Digit6") {
                        input_callback(6);
                    } else if (c === "Digit7") {
                        input_callback(7);
                    } else if (c === "Digit8") {
                        input_callback(8);
                    } else if (c === "Digit9") {
                        input_callback(9);
                    } else if (c === "Space") {
                        input_callback(10);
                    } else if (c === "Equal") {
                        input_callback(11);
                    } else if (c === "Minus") {
                        input_callback(12);
                    } else if (c === "Period") {
                        input_callback(13);
                    }
                });
            },
            csandPlatformSetRenderCallback: (callback_index) => {
                render_callback = function_table.get(callback_index);
            },
            csandPlatformRun: csandPlatformRun,
            csandPlatformIsMouseButtonPressed: () => {return mouse_down;},
            csandPlatformGetCursorPos: (x_ptr, y_ptr) => {
                const view = new DataView(obj.instance.exports.memory.buffer);
                view.setUint16(x_ptr, mouse_x, true);
                view.setUint16(y_ptr, mouse_y, true);
            },
        }
    };

    obj = await WebAssembly.instantiateStreaming(fetch("csand.wasm"), import_object);

    function_table = obj.instance.exports.__indirect_function_table;
    buf_ptr = obj.instance.exports.memory.grow(1) * 64 * 1024;
    buf = new Uint8Array(obj.instance.exports.memory.buffer, buf_ptr, width * height);
    obj.instance.exports.main();
}

function csandPlatformInit() {
    canvas = document.getElementById("canvas");
    gl = canvas.getContext("webgl");

    const vbo = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
    const vbo_data = new Int8Array([
        -3, -1,
         1, -1,
         1,  3,
    ]);
    gl.bufferData(gl.ARRAY_BUFFER, vbo_data, gl.STATIC_DRAW);

    program = csandLoadShaderProgram();
    gl.useProgram(program);

    const position_attr_loc = gl.getAttribLocation(program, "position");
    gl.vertexAttribPointer(position_attr_loc, 2, gl.BYTE, false, 0, 0);
    gl.enableVertexAttribArray(position_attr_loc);

    const texture = gl.createTexture();
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, texture);

    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);

    gl.texImage2D(gl.TEXTURE_2D, 0, gl.LUMINANCE, width, height, 0, gl.LUMINANCE, gl.UNSIGNED_BYTE, null);

    gl.uniform1i(gl.getUniformLocation(program, "texture"), 0);

    canvas.addEventListener("mousedown", (event) => {
        if (event.button == 0) mouse_down = true;
    });

    document.addEventListener("mouseup", (event) => {
        if (event.button == 0) mouse_down = false;
    });

    canvas.addEventListener("mousemove", (event) => {
        mouse_x = event.offsetX * width / canvas.width;
        mouse_y = height - 1 - Math.floor(event.offsetY * height / canvas.height);
    });
}

function csandLoadShaderProgram() {
    const vertex_shader = csandLoadShader(vertex_src, gl.VERTEX_SHADER);
    const fragment_shader = csandLoadShader(fragment_src, gl.FRAGMENT_SHADER);
    const program = gl.createProgram();
    gl.attachShader(program, vertex_shader);
    gl.attachShader(program, fragment_shader);
    gl.linkProgram(program);

    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
        console.error("failed to link shader program: " + gl.getProgramInfoLog(program));
        gl.deleteProgram(program);
        program = null;
    }

    gl.deleteShader(fragment_shader);
    gl.deleteShader(vertex_shader);

    return program;
}

function csandLoadShader(src, type) {
    const shader = gl.createShader(type);

    gl.shaderSource(shader, src);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        console.error("failed to compile shader '" + name + "': " + gl.getShaderInfoLog(shader));
        gl.deleteShader(shader);
        return null;
    }

    return shader;
}

function csandPlatformSetPalette(colors_ptr, colors_count) {
    const palette = gl.createTexture();

    gl.activeTexture(gl.TEXTURE1);
    gl.bindTexture(gl.TEXTURE_2D, palette);

    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);

    const colors = new Uint8Array(obj.instance.exports.memory.buffer, colors_ptr, colors_count * 4);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, colors_count, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, colors);

    gl.uniform1i(gl.getUniformLocation(program, "palette"), 1);
    gl.uniform1i(gl.getUniformLocation(program, "palette_size"), colors_count);

    gl.activeTexture(gl.TEXTURE0);
}

function csandPlatformRun() {
    if (render_callback != null) {
        render_callback(buf_ptr, width, height);
    }

    gl.clearColor(0.0, 0.0, 0.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT);

    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, width, height, gl.LUMINANCE, gl.UNSIGNED_BYTE, buf);

    gl.drawArrays(gl.TRIANGLES, 0, 3);
    requestAnimationFrame(csandPlatformRun);
}

main();
