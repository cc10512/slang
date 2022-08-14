//------------------------------------------------------------------------------
// CharInfo.h
// Various character-related utilities
//
// File is under the MIT license; see LICENSE for details
//------------------------------------------------------------------------------
#pragma once

#include <cstdint>

namespace slang {

/// Returns whether the given character is a valid ASCII character.
constexpr bool isASCII(char c) {
    return static_cast<unsigned char>(c) < 128;
}

/// Returns whether the given character is considered "printable",
/// which notably includes whitespace (while the c function `isprint` does not).
constexpr bool isPrintableASCII(char c) {
    return (c >= 32 && c <= 126) || (c >= 9 && c <= 13);
}

/// Returns whether the given character is considered whitespace.
constexpr bool isWhitespace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\v':
        case '\f':
        case '\r':
        case '\n':
            return true;
    }
    return false;
}

/// Returns whether the given character is considered a new line.
constexpr bool isNewline(char c) {
    return c == '\r' || c == '\n';
}

/// Returns whether the given character is considered a decimal digit.
constexpr bool isDecimalDigit(char c) {
    return c >= '0' && c <= '9';
}

/// Returns whether the given character is considered an octal digit.
constexpr bool isOctalDigit(char c) {
    return c >= '0' && c <= '7';
}

/// Returns whether the given character is considered a hexadecimal digit.
constexpr bool isHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/// Returns whether the given character is considered a binary digit.
constexpr bool isBinaryDigit(char c) {
    return c == '0' || c == '1';
}

/// Returns whether the given character is considered an alphanumeric character.
constexpr bool isAlphaNumeric(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/// Returns whether the given character is valid in a C language identifier.
/// That includes all alphanumeric characters and the underscore.
constexpr bool isValidCIdChar(char c) {
    return isAlphaNumeric(c) || c == '_';
}

/// Returns whether the given character is considered a special logic digit,
/// which encompasses various ways to say Unknown (X) or High Impedance (Z).
constexpr bool isLogicDigit(char c) {
    switch (c) {
        case 'z':
        case 'Z':
        case '?':
        case 'x':
        case 'X':
            return true;
        default:
            return false;
    }
}

/// Gets the numeric value of the given decimal digit. If the given character
/// is not a valid decimal digit, the result is undefined.
constexpr uint8_t getDigitValue(char c) {
    return uint8_t(c - '0');
}

/// Gets the numeric value of the given hexadecimal digit. If the given character
/// is not a valid hexadecimal digit, the result is undefined.
constexpr uint8_t getHexDigitValue(char c) {
    if (c <= '9')
        return uint8_t(c - '0');
    if (c <= 'F')
        return uint8_t(10 + c - 'A');
    return uint8_t(10 + c - 'a');
}

constexpr int utf8Len(unsigned char first) {
    constexpr const char lengths[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                       0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0 };
    return lengths[first >> 3];
}

// A public domain branchless UTF-8 decoder by Christopher Wellons:
// https://github.com/skeeto/branchless-utf8
//
/// Decode the next character, @a c from @a buf
///
/// Since this is a branchless decoder, four bytes will be read from the
/// buffer regardless of the actual length of the next character. This
/// means the buffer _must_ have at least three bytes of zero padding
/// following the end of the data stream.
///
/// Errors are reported in @a e which will be non-zero if the parsed
/// character was somehow invalid: invalid byte sequence, non-canonical
/// encoding, or a surrogate half.
///
/// The function returns a pointer to the next character. When an error
/// occurs, this pointer will be a guess that depends on the particular
/// error, but it will always advance at least one byte.
///
constexpr const char* utf8Decode(const char* b, uint32_t* c, int* e) {
    constexpr const int masks[] = { 0x00, 0x7f, 0x1f, 0x0f, 0x07 };
    constexpr const uint32_t mins[] = { 4194304, 0, 128, 2048, 65536 };
    constexpr const int shiftc[] = { 0, 18, 12, 6, 0 };
    constexpr const int shifte[] = { 0, 6, 4, 2, 0 };

    using uc = unsigned char;
    int len = utf8Len(uc(*b));

    // Compute the pointer to the next character early so that the next
    // iteration can start working on the next character. Neither Clang
    // nor GCC figure out this reordering on their own.
    const char* next = b + len + !len;

    // Assume a four-byte character and load four bytes. Unused bits are
    // shifted out.
    *c = (uint32_t)(uc(b[0]) & masks[len]) << 18;
    *c |= (uint32_t)(uc(b[1]) & 0x3f) << 12;
    *c |= (uint32_t)(uc(b[2]) & 0x3f) << 6;
    *c |= (uint32_t)(uc(b[3]) & 0x3f) << 0;
    *c >>= shiftc[len];

    // Accumulate the various error conditions.
    *e = (*c < mins[len]) << 6;      // non-canonical encoding
    *e |= ((*c >> 11) == 0x1b) << 7; // surrogate half?
    *e |= (*c > 0x10FFFF) << 8;      // out of range?
    *e |= (uc(b[1]) & 0xc0) >> 2;
    *e |= (uc(b[2]) & 0xc0) >> 4;
    *e |= (uc(b[3])) >> 6;
    *e ^= 0x2a; // top two bits of each tail byte correct?
    *e >>= shifte[len];

    return next;
}

} // namespace slang
