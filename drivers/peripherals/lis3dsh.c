/**
 * LIS3DSH accelerometer driver
 * https://www.st.com/resource/en/datasheet/lis3dsh.pdf
 */

#include "lis3dsh.h"

static Lis3dshConfig cfg;

/**
 * @brief Configure the LIS3DSH driver with the necessary SPI transfer and chip select functions
 * @param config: Configuration struct containing function pointers for SPI transfer and chip select control
 */
void lis3dsh_configure(Lis3dshConfig config)
{
    cfg = config;
}

/**
 * @brief       Write a byte to a register on the LIS3DSH
 * @param reg:  Register address
 * @param data: Byte to write
 */
static void write_reg(uint8_t reg, uint8_t data)
{
    cfg.cs_assert();
    cfg.transfer(reg);
    cfg.transfer(data);
    cfg.cs_deassert();
}

/**
 * @brief       Read a byte from a register on the LIS3DSH
 * @param reg:  Register address
 * @return:     Byte read from the register
 */
static uint8_t read_reg(uint8_t reg)
{
    cfg.cs_assert();
    cfg.transfer(reg | LIS3DSH_SPI_READ);
    uint8_t val = cfg.transfer(0x00);
    cfg.cs_deassert();
    return val;
}

/**
 * @brief 
 * @param reg 
 * @param clear_mask 
 * @param set_mask 
 */
static void modify_reg(uint8_t reg, uint8_t clear_mask, uint8_t set_mask)
{
    write_reg(reg, (read_reg(reg) & ~clear_mask) | set_mask);
}

static void lis3dsh_init(void)
{
    // write_reg(LIS3DSH_REG_CTRL_REG4,
    //          (LIS3DSH_VAL_CTRL_REG4_ODR_100HZ << LIS3DSH_CTRL_REG4_ODR_Pos) |
    //          (1U << LIS3DSH_CTRL_REG4_ZEN_Pos) |
    //          (1U << LIS3DSH_CTRL_REG4_YEN_Pos) |
    //          (1U << LIS3DSH_CTRL_REG4_XEN_Pos));

    modify_reg(LIS3DSH_REG_CTRL_REG4,
               LIS3DSH_CTRL_REG4_ODR_Msk | LIS3DSH_CTRL_REG4_YEN_Msk | LIS3DSH_CTRL_REG4_XEN_Msk,
               (LIS3DSH_VAL_CTRL_REG4_ODR_100HZ << LIS3DSH_CTRL_REG4_ODR_Pos) |
               LIS3DSH_CTRL_REG4_YEN_Msk |
               LIS3DSH_CTRL_REG4_XEN_Msk);

}

static float lis3dsh_read_x_axis(void)
{
    return (float)(int8_t)read_reg(LIS3DSH_REG_OUT_X_H);
}

static float lis3dsh_read_y_axis(void)
{
    return (float)(int8_t)read_reg(LIS3DSH_REG_OUT_Y_H);
}

AccelerometerDriver lis3dsh_driver = {
    .init        = lis3dsh_init,
    .read_x_axis = lis3dsh_read_x_axis,
    .read_y_axis = lis3dsh_read_y_axis,
};
