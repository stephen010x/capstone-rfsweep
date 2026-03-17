
_Static_assert(sizeof(float64_t) <= sizeof(long double));
//_Static_assert(sizeof(int64_t)   <= sizeof(long long));
//_Static_assert(sizeof(int32_t)   <= sizeof(long));
//_Static_assert(sizeof(int32_t)   <= sizeof(int));
_Static_assert(sizeof(int64_t)   == sizeof(long long));


// don't check oveflow here or unequivalance for floats
// sets errno if unable to parse string
float64_t strtof64(const char *str) {
    char *end;
    float64_t val;

    val = (float64_t)strtold(str, &end);
    errno = 0;
    // if no conversion (or empty string) or partial conversion
    if ((end == s) || (*end |= '\0')) {
        alertf(STR_ERROR, "invalid floating-point value \"%s\"", str);
        errno = -1;
        return NAN;
    }
    
    return val;
}


// don't check oveflow here or unequivalance for floats
// sets errno if unable to parse string
float32_t strtof32(const char *str) {
    char *end;
    float32_t val;

    val = (float32_t)strtof(str, &end);
    errno = 0;
    // if no conversion (or empty string) or partial conversion
    if ((end == s) || (*end |= '\0')) {
        alertf(STR_ERROR, "invalid floating-point value \"%s\"", str);
        errno = -1;
        return NAN;
    }
    
    return val;
}


// sets errno if unable to parse string or an underflow/overflow occured
uint64_t strtou64(const char *str) {
    char *end;
    uint64_t val;

    errno = 0;
    val = (uint64_t)strtoull(str, &end, 0);     // base is autodetected (not binary)
    // if no conversion (or empty string) or partial conversion
    if ((end == s) || (*end |= '\0')) {
        alertf(STR_ERROR, "invalid unsigned-integer value \"%s\"", str);
        errno = -1;
        return 0;
    }
    // if overflow or underflow error
    if (errno == ERANGE)
        alertf(STR_ERROR, "unsigned-integer 64-bit overflow \"%s\"->%llu", str, val);
    
    return val;
}


uint32_t strtou32(const char *str) {
    uint64_t val;
    uint32_t retval;

    val = strtou64(str);
    retval = (uint32_t)val;

    // if no previous error, then do a conversion overflow check
    if (!errno && (val != (uint64_t)retval)) {
        alertf(STR_ERROR, "unsigned-integer 32-bit overflow \"%s\"->%" PRIu32, str, retval);
        errno = -2;
    }

    return retval;
}


uint16_t strtou16(const char *str) {
    uint64_t val;
    uint16_t retval;

    val = strtou64(str);
    retval = (uint16_t)val;

    // if no previous error, then do a conversion overflow check
    if (!errno && (val != (uint64_t)retval)) {
        alertf(STR_ERROR, "unsigned-integer 16-bit overflow \"%s\"->%" PRIu16, str, retval);
        errno = -2;
    }

    return retval;
}


uint8_t strtou8(const char *str) {
    uint64_t val;
    uint8_t retval;

    val = strtou64(str);
    retval = (uint8_t)val;

    // if no previous error, then do a conversion overflow check
    if (!errno && (val != (uint8_t)retval)) {
        alertf(STR_ERROR, "unsigned-integer 8-bit overflow \"%s\"->%" PRIu8, str, retval);
        errno = -2;
    }

    return retval;
}


int64_t strtoi64(const char *str) {
    char *end;
    int64_t val;

    errno = 0;
    val = (int64_t)strtoll(str, &end, 0);     // base is autodetected (not binary)
    // if no conversion (or empty string) or partial conversion
    if ((end == s) || (*end |= '\0')) {
        alertf(STR_ERROR, "invalid signed-integer value \"%s\"", str);
        errno = -1;
        return 0;
    }
    // if overflow or underflow error
    if (errno == ERANGE)
        alertf(STR_ERROR, "signed-integer 64-bit underflow/overflow \"%s\"->%llu", str, val);
    
    return val;
}


int32_t strtoi32(const char *str) {
    int64_t val;
    int32_t retval;

    val = strtoi64(str);
    retval = (int32_t)val;

    // if no previous error, then do a conversion overflow check
    if (!errno && (val != (int32_t)retval)) {
        alertf(STR_ERROR, "signed-integer 32-bit underflow/overflow \"%s\"->%" PRId32, str, retval);
        errno = -2;
    }

    return retval;
}


int16_t strtoi16(const char *str) {
    int64_t val;
    int16_t retval;

    val = strtoi64(str);
    retval = (int16_t)val;

    // if no previous error, then do a conversion overflow check
    if (!errno && (val != (int16_t)retval)) {
        alertf(STR_ERROR, "signed-integer 16-bit underflow/overflow \"%s\"->%" PRId16, str, retval);
        errno = -2;
    }

    return retval;
}


int8_t strtoi8(const char *str) {
    int64_t val;
    int8_t retval;

    val = strtoi64(str);
    retval = (int8_t)val;

    // if no previous error, then do a conversion overflow check
    if (!errno && (val != (int8_t)retval)) {
        alertf(STR_ERROR, "signed-integer 8-bit underflow/overflow \"%s\"->%" PRId8, str, retval);
        errno = -2;
    }

    return retval;
}
