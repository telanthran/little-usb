#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdint.h>

typedef struct {
    void     (*init)(void);
    uint8_t  (*is_pressed)(void);
} ButtonDriver;

#endif /* BUTTON_H_ */
