import { FrontableCollection, Frontable } from "./types";

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

export function toFrontableCollection(frontableArr: Frontable[]): FrontableCollection {
    const collection: FrontableCollection = {};

    for (const frontable of frontableArr) {
        const hash = genHash(frontable.id);
        collection[hash] = frontable;
    }

    return collection;
}