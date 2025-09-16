// utils file, holds a handful of utility functions

// https://stackoverflow.com/questions/7616461/generate-a-hash-from-string-in-javascript
//! NOTE: this may result in hash collisions, as far as I've tested 
//!   there have been none but be aware in case of future issues
function genHash(str) {
    var hash = 0;
    for (const char of str) {
        hash = (hash << 5) - hash + char.charCodeAt(0);
        hash = hash >>> 0;  // Constrain to unsigned 32 bit int
    }
    return hash;
}

module.exports = {
    genHash: genHash
};
