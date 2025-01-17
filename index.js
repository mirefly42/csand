import { GLES2Context } from "./gles2.js";
import { getNullTerminatedString } from "./memutils.js";

const width = 128;
const height = 64;
let function_table = null;
let input_callback = null;
let render_callback = null;
let canvas = null;
let gl = null;
let obj = null;
let mouse_down = false;
let mouse_x = 0;
let mouse_y = 0;
const gles2_context = new GLES2Context(null, null);

async function main() {
    document.getElementById("button_toggle_buttons").addEventListener("click", () => {
        const buttons = document.getElementById("buttons");
        buttons.hidden = !buttons.hidden;
    });

    const import_object = {
        env: {
            csandPlatformInit: csandPlatformInit,
            csandPlatformSetInputCallback: (callback_index) => {
                input_callback = function_table.get(callback_index);
                function setButtonInput(id, input) {
                    document.getElementById(id).addEventListener("pointerdown", () => {input_callback(input);});
                }
                setButtonInput("button_air", 0);
                setButtonInput("button_wall", 1);
                setButtonInput("button_sand", 2);
                setButtonInput("button_water", 3);
                setButtonInput("button_fire", 4);
                setButtonInput("button_wood", 5);
                setButtonInput("button_coal", 6);
                setButtonInput("button_oil", 7);
                setButtonInput("button_hydrogen_gas", 8);
                setButtonInput("button_hydrogen_liquid", 9);
                setButtonInput("button_pause", 10);
                setButtonInput("button_simulation_speed_plus", 11);
                setButtonInput("button_simulation_speed_minus", 12);
                setButtonInput("button_simulate_frame", 13);
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
            csandPlatformPrintErr: (str_ptr) => {
                console.error(getNullTerminatedString(obj.instance.exports.memory.buffer, str_ptr));
            },
        }
    };

    gles2_context.importIntoObject(import_object.env);
    obj = await WebAssembly.instantiateStreaming(fetch("csand.wasm"), import_object);
    gles2_context.memory = obj.instance.exports.memory;

    function_table = obj.instance.exports.__indirect_function_table;
    obj.instance.exports.main();
}

function csandPlatformInit() {
    canvas = document.getElementById("canvas");
    gl = canvas.getContext("webgl");
    gles2_context.gl = gl;

    canvas.addEventListener("mousedown", (event) => {
        if (event.button === 0) mouse_down = true;
    });

    document.addEventListener("mouseup", (event) => {
        if (event.button === 0) mouse_down = false;
    });

    canvas.addEventListener("mousemove", (event) => {
        mouse_x = event.offsetX * width / canvas.width;
        mouse_y = height - 1 - Math.floor(event.offsetY * height / canvas.height);
    });
}

function csandPlatformRun() {
    if (render_callback != null) {
        render_callback();
    }

    requestAnimationFrame(csandPlatformRun);
}

main();
