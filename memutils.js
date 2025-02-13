const utf8_decoder = new TextDecoder("utf-8");
const utf8_encoder = new TextEncoder("utf-8");

export function strlen(buffer, ptr) {
    const view = new DataView(buffer);
    let i = 0;
    while (view.getUint8(ptr + i) != 0) {
        i++;
    }
    return i;
}

export function getString(buffer, ptr, len) {
    return utf8_decoder.decode(buffer.slice(ptr, ptr + len));
}

export function getNullTerminatedString(buffer, ptr) {
    const len = strlen(buffer, ptr);
    return getString(buffer, ptr, len);
}

export function setNullTerminatedString(buffer, ptr, max_len, str) {
    max_len = Math.max(0, max_len);

    if (max_len === 0) {
        return 0;
    }

    const array = new Uint8Array(buffer, ptr, max_len - 1);
    const result = utf8_encoder.encodeInto(str, array);
    array[result.written] = 0;
    return result.written;
}

export function setUint32Le(buffer, ptr, value) {
    const view = new DataView(buffer);
    view.setUint32(ptr, value, true);
}

export function setInt32Le(buffer, ptr, value) {
    const view = new DataView(buffer);
    view.setInt32(ptr, value, true);
}

const uint16_size = 2;
export class Uint16LeArray {
    #view;

    set(index, value) {
        this.#view.setUint16(index * uint16_size, value, true);
    }

    get(index) {
        return this.#view.getUint16(index * uint16_size, true);
    }

    constructor(buffer, ptr, len) {
        this.#view = new DataView(buffer, ptr, len * uint16_size);
    }
}

const uint32_size = 4;
export class Uint32LeArray {
    #view;

    set(index, value) {
        this.#view.setUint32(index * uint32_size, value, true);
    }

    get(index) {
        return this.#view.getUint32(index * uint32_size, true);
    }

    constructor(buffer, ptr, len) {
        this.#view = new DataView(buffer, ptr, len * uint32_size);
    }
}
