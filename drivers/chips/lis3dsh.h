/**
 * Register definitions for LIS3DSH accelerometer
 * https://www.st.com/resource/en/datasheet/lis3dsh.pdf
 */

#ifndef LIS3DSH_H_
#define LIS3DSH_H_

#include <stdint.h>
#include "accelerometer.h"

extern AccelerometerDriver lis3dsh_driver;

typedef struct {
    uint8_t (*transfer)(uint8_t byte);
    void    (*cs_assert)(void);
    void    (*cs_deassert)(void);
} Lis3dshConfig;

// SPI is MSB-first; STM has MSB (b7) of address byte as R/W bit
#define LIS3DSH_SPI_READ            0x80

#define LIS3DSH_REG_INFO1           0x0D
#define LIS3DSH_REG_INFO2           0x0E
#define LIS3DSH_REG_WHO_AM_I        0x0F
#define LIS3DSH_REG_OFF_X           0x10
#define LIS3DSH_REG_OFF_Y           0x11
#define LIS3DSH_REG_OFF_Z           0x12
#define LIS3DSH_REG_CS_X            0x13
#define LIS3DSH_REG_CS_Y            0x14
#define LIS3DSH_REG_CS_Z            0x15
// Output data rate and axis enable control register (CTRL_REG4)
#define LIS3DSH_REG_CTRL_REG4       0x20
#define LIS3DSH_REG_CTRL_REG1       0x21
#define LIS3DSH_REG_CTRL_REG2       0x22
#define LIS3DSH_REG_CTRL_REG3       0x23
#define LIS3DSH_REG_CTRL_REG5       0x24
#define LIS3DSH_REG_CTRL_REG6       0x25
#define LIS3DSH_REG_CTRL_STATUS     0x27
#define LIS3DSH_REG_OUT_X_L         0x28
// X-axis output register high values
#define LIS3DSH_REG_OUT_X_H         0x29
#define LIS3DSH_REG_OUT_Y_L         0x2A
// Y-axis output register high values
#define LIS3DSH_REG_OUT_Y_H         0x2B
#define LIS3DSH_REG_OUT_Z_L         0x2C
#define LIS3DSH_REG_OUT_Z_H         0x2D
#define LIS3DSH_REG_FIFO_CTRL       0x2E
#define LIS3DSH_REG_FIFO_SRC        0x2F
#define LIS3DSH_REG_ST1_X           0x40
#define LIS3DSH_REG_TIM4_1          0x50
#define LIS3DSH_REG_TIM3_1          0x51
#define LIS3DSH_REG_TIM2_1          0x52
#define LIS3DSH_REG_TIM1_1          0x54
#define LIS3DSH_REG_THRS2_1         0x56
#define LIS3DSH_REG_THRS1_1         0x57
#define LIS3DSH_REG_MASK1_B         0x59
#define LiS3DSH_REG_MASK1_A         0x5A
#define LIS3DSH_REG_SETT1_B         0x5B
#define LIS3DSH_REG_PR1             0x5C
#define LIS3DSH_REG_TC1             0x5D

// LIS3DSH_REG_CTRL_REG4 field descriptors
#define LIS3DSH_CTRL_REG4_ODR_Pos   4U
#define LIS3DSH_CTRL_REG4_ODR_Msk   (0xFU << LIS3DSH_CTRL_REG4_ODR_Pos)
#define LIS3DSH_CTRL_REG4_BDU_Pos   3U
#define LIS3DSH_CTRL_REG4_BDU_Msk   (1U << LIS3DSH_CTRL_REG4_BDU_Pos)
#define LIS3DSH_CTRL_REG4_ZEN_Pos   2U
#define LIS3DSH_CTRL_REG4_ZEN_Msk   (1U << LIS3DSH_CTRL_REG4_ZEN_Pos)
#define LIS3DSH_CTRL_REG4_YEN_Pos   1U
#define LIS3DSH_CTRL_REG4_YEN_Msk   (1U << LIS3DSH_CTRL_REG4_YEN_Pos)
#define LIS3DSH_CTRL_REG4_XEN_Pos   0U
#define LIS3DSH_CTRL_REG4_XEN_Msk   (1U << LIS3DSH_CTRL_REG4_XEN_Pos)

// LIS3DSH_CTRL_REG4_ODR field values
#define LIS3DSH_VAL_CTRL_REG4_ODR_POWERDOWN 0b0000
#define LIS3DSH_VAL_CTRL_REG4_ODR_3D125HZ   0b0001
#define LIS3DSH_VAL_CTRL_REG4_ODR_6D25HZ    0b0010
#define LIS3DSH_VAL_CTRL_REG4_ODR_12D5HZ    0b0011
#define LIS3DSH_VAL_CTRL_REG4_ODR_25HZ      0b0100
#define LIS3DSH_VAL_CTRL_REG4_ODR_50HZ      0b0101
#define LIS3DSH_VAL_CTRL_REG4_ODR_100HZ     0b0110
#define LIS3DSH_VAL_CTRL_REG4_ODR_400HZ     0b0111
#define LIS3DSH_VAL_CTRL_REG4_ODR_800HZ     0b1000
#define LIS3DSH_VAL_CTRL_REG4_ODR_1600HZ    0b1001

void lis3dsh_configure(Lis3dshConfig config);

#endif /* LIS3DSH_H_ */
