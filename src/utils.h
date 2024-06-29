#ifndef UTILS_H_
#define UTILS_H_

#define ARRAY_SIZE(Arr) (sizeof(Arr) / sizeof(Arr[0]))

inline uint8_t utils_bit_is_set(uint16_t *value, uint8_t offset) {
    return (*value >> offset) & (uint16_t) 1;
}

inline void utils_bit_set_to(uint16_t *value, uint8_t offset, uint8_t bit) {
    *value = (*value & ~((uint16_t) 1 << offset)) | ((uint16_t) bit << offset);
}

#endif /* UTILS_H_ */
