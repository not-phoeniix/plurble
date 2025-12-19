// utils file, holds a handful of utility functions

// this is the Jenkins one-at-a-time hash function!
//   https://en.wikipedia.org/wiki/Jenkins_hash_function
//! NOTE: this may result in hash collisions, as far as I've tested 
//!   there have been none but be aware in case of future issues
export function genHash(str: string) {
    let hash = 0;
    for (var i = 0; i < str.length; i++) {
        hash += str.charCodeAt(i);
        hash += hash << 10;
        hash ^= hash >> 6;
    }

    hash += hash << 3;
    hash ^= hash << 11;
    hash += hash << 15;

    // zero-fill right shift to ensure unsigned
    return (hash >>> 0) % 0xFFFFFFFF;
}

// converts an array of 32 bit integers into a quadruple-sized 8-bit 
//   integer array containing all the bytes of the inputted array
export function toByteArray(array: number[]): number[] {
    const byteArr: number[] = [];

    for (const num of array) {
        byteArr.push((num >>> 24) & 0xFF);
        byteArr.push((num >>> 16) & 0xFF);
        byteArr.push((num >>> 8) & 0xFF);
        byteArr.push((num >>> 0) & 0xFF);
    }

    return byteArr;
}

// converts an array of 8-bit integers back into a quarter-sized
//   32-bit integer array from the array of bytes
export function fromByteArray(array: number[]): number[] {
    const outputArr: number[] = [];

    for (let i = 0; i < array.length; i += 4) {
        let num = 0;
        num |= ((array[i + 0] & 0xFF) << 24);
        num |= ((array[i + 1] & 0xFF) << 16);
        num |= ((array[i + 2] & 0xFF) << 8);
        num |= ((array[i + 3] & 0xFF) << 0);

        outputArr.push(num);
    }

    return outputArr;
}

export function toARGB8Color(hexCode: string): number {
    // input should be in RGBA format "#FF0e0FFF"

    // bits are in format AARRGGBB
    const hexStripped = hexCode.replace("#", "").replace("0x", "");
    const hexConverted = Number(`0x${hexStripped}`);

    let color = 0;
    color |= (hexConverted >>> (16 + 6) & 0b11) << 4;   // red
    color |= (hexConverted >>> (8 + 6) & 0b11) << 2;    // green
    color |= (hexConverted >>> (0 + 6) & 0b11) << 0;    // blue

    // handling alpha
    if (hexStripped.length === 8) {
        color |= (hexConverted >>> (24 + 6) & 0b11) << 6;
    } else {
        color |= 0b11000000;
    }

    return color;
}

export function calcByteLen(str: string): number {
    let len = 0;
    Array.from(str).forEach(c => len += c.charCodeAt(0) > 255 ? 4 : 1);
    return len;
}

export function cleanString(str: string, maxBytes: number): string {
    str = str.trim();

    let retStr = "";
    let numBytes = 0;

    Array.from(str).forEach(c => {
        numBytes += calcByteLen(c);

        if (numBytes <= maxBytes) {
            retStr += c;
        }
    });

    return retStr.trim();
}
