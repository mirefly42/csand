import { GLES2Context } from "./gles2.js";
import { getNullTerminatedString, Uint16LeArray } from "./memutils.js";

let function_table = null;
let input_callback = null;
let render_callback = null;
let framebuffer_size_callback = null;
let key_callback = null;
let char_callback = null;
let mouse_button_callback = null;
let mouse_motion_callback = null;
let mouse_scroll_callback = null;
let canvas = null;
let gl = null;
let obj = null;
let mouse_down = false;
let captured_pointer_id = null;
let mouse_x = 0;
let mouse_y = 0;
let scratch_buf_ptr = 0;
const gles2_context = new GLES2Context(null, null);

async function main() {
    const import_object = {
        env: {
            csandPlatformInit: csandPlatformInit,
            csandPlatformSetRenderCallback: (callback_index) => {
                render_callback = function_table.get(callback_index);
            },
            csandPlatformSetKeyCallback: (callback_index) => {
                key_callback = function_table.get(callback_index);
            },
            csandPlatformSetCharCallback: (callback_index) => {
                char_callback = function_table.get(callback_index);
            },
            csandPlatformSetMouseButtonCallback: (callback_index) => {
                mouse_button_callback = function_table.get(callback_index);
            },
            csandPlatformSetMouseMotionCallback: (callback_index) => {
                mouse_motion_callback = function_table.get(callback_index);
            },
            csandPlatformSetMouseScrollCallback: (callback_index) => {
                mouse_scroll_callback = function_table.get(callback_index);
            },
            csandPlatformSetFramebufferSizeCallback: (callback_index) => {
                framebuffer_size_callback = function_table.get(callback_index);
            },
            csandPlatformRun: () => {
                requestAnimationFrame(csandPlatformRun);
            },
            csandPlatformIsMouseButtonPressed: () => {return mouse_down;},
            csandPlatformGetCursorPos: (vec_ptr) => {
                const vec = new Uint16LeArray(obj.instance.exports.memory.buffer, vec_ptr, 2);
                vec.set(0, mouse_x);
                vec.set(1, mouse_y);
            },
            csandPlatformGetWindowSize: csandPlatformGetWindowSize,
            csandPlatformGetFramebufferSize: csandPlatformGetWindowSize,
            csandPlatformPrintErr: (str_ptr) => {
                console.error(getNullTerminatedString(obj.instance.exports.memory.buffer, str_ptr));
            },
            csandPlatformToggleFullscreen: () => {
                toggleFullscreen();
            },
        }
    };

    gles2_context.importIntoObject(import_object.env);
    obj = await WebAssembly.instantiateStreaming(fetch("csand.wasm"), import_object);
    gles2_context.memory = obj.instance.exports.memory;

    function_table = obj.instance.exports.__indirect_function_table;
    const wasm_page_size = 64 * 1024;
    scratch_buf_ptr = obj.instance.exports.memory.grow(1) * wasm_page_size;
    obj.instance.exports.main();
}

function resizeCanvas(width, height) {
    canvas.width = width;
    canvas.height = height;

    if (framebuffer_size_callback != null) {
        csandPlatformGetWindowSize(scratch_buf_ptr);
        framebuffer_size_callback(scratch_buf_ptr);
    }
}

function csandPlatformInit() {
    canvas = document.getElementById("canvas");
    resizeCanvas(window.innerWidth, window.innerHeight);

    gl = canvas.getContext("webgl");
    gles2_context.gl = gl;

    function sendKey(event, pressed) {
        if (key_callback === null) {
            return;
        }

        const code = event.code;
        const key_start = "Key";
        const digit_start = "Digit";
        const mods = event.shiftKey | (event.ctrlKey << 1) | (event.metaKey << 2);
        const mapping = {
            "Space": 32,
            "Comma": 44,
            "Minus": 45,
            "Period": 46,
            "Equal": 61,
            "Enter": 257,
            "Tab": 258,
            "Backspace": 259,
        };

        function f(num_code) {
            if (key_callback(num_code, pressed, mods)) {
                event.preventDefault();
            }
        }

        if (code.startsWith(key_start)) {
            f(code.codePointAt(key_start.length));
        } else if (code.startsWith(digit_start)) {
            f(code.codePointAt(digit_start.length));
        } else {
            const num_code = mapping[code];
            if (num_code != undefined) {
                f(num_code);
            }
        }
        // NOTE: unfinished
    }

    document.addEventListener("keydown", (event) => {
        if (char_callback != null) {
            if (event.key.length <= 2) { // a hacky way to check that the key isn't something like "Enter"
                char_callback(event.key.codePointAt(0));
            } else if (event.key === "Enter") {
                char_callback(10);
            }
            // NOTE: unfinished
        }

        sendKey(event, true);
    });

    document.addEventListener("wheel", (event) => {
        if (mouse_scroll_callback != null) {
            mouse_scroll_callback(event.deltaX, -Math.sign(event.deltaY));
        }
    });

    document.addEventListener("keyup", (event) => {
        sendKey(event, false);
    });

    canvas.addEventListener("pointerdown", (event) => {
        if (!mouse_down && event.button === 0) {
            canvas.setPointerCapture(event.pointerId);
            captured_pointer_id = event.pointerId;
            updateMousePosition(event.offsetX, event.offsetY);
            mouse_down = true;
            mouse_button_callback(event.button, mouse_down);
        }
    });

    function pointerUp(event) {
        if (event.pointerId === captured_pointer_id) {
            canvas.releasePointerCapture(event.pointerId);
            captured_pointer_id = null;
            updateMousePosition(event.offsetX, event.offsetY);
            mouse_down = false;
            mouse_button_callback(event.button, mouse_down);
        }
    }

    canvas.addEventListener("pointercancel", pointerUp);
    canvas.addEventListener("pointerup", pointerUp);

    canvas.addEventListener("pointermove", (event) => {
        if (captured_pointer_id === null || event.pointerId === captured_pointer_id) {
            updateMousePosition(event.offsetX, event.offsetY);
        }
    });

    window.addEventListener("resize", (event) => {
        resizeCanvas(window.innerWidth, window.innerHeight);
    });
}

function clamp(x, min, max) {
    if (x <= min) {
        return min;
    } else if (x <= max) {
        return x;
    } else {
        return max;
    }
}

function updateMousePosition(x, y) {
    const uint16_max = 65535;
    mouse_x = clamp(x, 0, uint16_max);
    mouse_y = clamp(y, 0, uint16_max);
    if (mouse_motion_callback != null) {
        mouse_motion_callback(mouse_x, mouse_y);
    }
}

function toggleFullscreen() {
    if (!document.fullscreenElement) {
        game.requestFullscreen();
    } else {
        document.exitFullscreen();
    }
}

function csandPlatformRun(time) {
    if (render_callback != null) {
        render_callback(time / 1000.0);
    }

    requestAnimationFrame(csandPlatformRun);
}

function csandPlatformGetWindowSize(vec_ptr) {
    const vec = new Uint16LeArray(obj.instance.exports.memory.buffer, vec_ptr, 2);

    vec.set(0, canvas.width);
    vec.set(1, canvas.height);
}

main();
