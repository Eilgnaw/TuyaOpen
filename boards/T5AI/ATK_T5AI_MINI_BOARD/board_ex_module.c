/**
 * @file board_ex_module.c
 * @version 0.1
 * @date 2025-07-01
 */

#include "tal_api.h"

#include "tkl_gpio.h"
#include "tkl_pinmux.h"
#include "board_ex_module.h"

/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/

/***********************************************************
***********************function define**********************
***********************************************************/
#if (defined(ATK_T5AI_MINI_BOARD_LCD_MD0240_SPI) && (ATK_T5AI_MINI_BOARD_LCD_MD0240_SPI ==1))
static OPERATE_RET __board_register_display(void)
{
    OPERATE_RET rt = OPRT_OK;
    DISP_SPI_DEVICE_CFG_T display_cfg;

    memset(&display_cfg, 0, sizeof(DISP_SPI_DEVICE_CFG_T));

    /* Configure the SPI0 pins */
    tkl_io_pinmux_config(BOARD_LCD_SPI_CS_PIN, TUYA_SPI0_CS);
    tkl_io_pinmux_config(BOARD_LCD_SPI_SCL_PIN, TUYA_SPI0_CLK);
    tkl_io_pinmux_config(BOARD_LCD_SPI_SDA_PIN, TUYA_SPI0_MOSI);
    tkl_io_pinmux_config(BOARD_LCD_SPI_SDI_PIN, TUYA_SPI0_MISO);

    display_cfg.width     = BOARD_LCD_WIDTH;
    display_cfg.height    = BOARD_LCD_HEIGHT;
    display_cfg.pixel_fmt = BOARD_LCD_PIXELS_FMT;
    display_cfg.rotation  = BOARD_LCD_ROTATION;

    display_cfg.port      = BOARD_LCD_SPI_PORT;
    display_cfg.spi_clk   = BOARD_LCD_SPI_CLK;
    display_cfg.cs_pin    = BOARD_LCD_SPI_CS_PIN;
    display_cfg.dc_pin    = BOARD_LCD_SPI_DC_PIN;
    display_cfg.rst_pin   = BOARD_LCD_SPI_RST_PIN;

    display_cfg.bl.type              = BOARD_LCD_BL_TYPE;
    display_cfg.bl.gpio.pin          = BOARD_LCD_BL_PIN;
    display_cfg.bl.gpio.active_level = BOARD_LCD_BL_ACTIVE_LV;

    display_cfg.power.pin = BOARD_LCD_POWER_PIN;

    TUYA_CALL_ERR_RETURN(tdd_disp_spi_st7789_register(DISPLAY_NAME, &display_cfg)); 

    return rt;
}
#elif (defined(ATK_T5AI_MINI_BOARD_LCD_MD0240_8080) && (ATK_T5AI_MINI_BOARD_LCD_MD0240_8080 ==1))
static OPERATE_RET __board_register_display(void)
{
    DISP_MCU8080_DEVICE_CFG_T display_cfg;

    memset(&display_cfg, 0, sizeof(DISP_MCU8080_DEVICE_CFG_T));

    display_cfg.width     = BOARD_LCD_WIDTH;
    display_cfg.height    = BOARD_LCD_HEIGHT;
    display_cfg.pixel_fmt = BOARD_LCD_PIXELS_FMT;
    display_cfg.rotation  = BOARD_LCD_ROTATION;

    display_cfg.clk       = BOARD_LCD_8080_CLK;
    display_cfg.data_bits = BOARD_LCD_8080_BITS_DATA;
    display_cfg.te_pin    = BOARD_LCD_8080_TE_PIN;

    display_cfg.bl.type              = BOARD_LCD_BL_TYPE;
    display_cfg.bl.gpio.pin          = BOARD_LCD_BL_PIN;
    display_cfg.bl.gpio.active_level = BOARD_LCD_BL_ACTIVE_LV;

    display_cfg.power.pin = BOARD_LCD_POWER_PIN;

    return tdd_disp_mcu8080_st7789_register(DISPLAY_NAME, &display_cfg);

}
#else
static OPERATE_RET __board_register_display(void)
{
    return OPRT_OK;
}

#endif

#if defined (ATK_T5AI_MINI_BOARD_CAMERA_OV2640) && (ATK_T5AI_MINI_BOARD_CAMERA_OV2640 ==1)
static OPERATE_RET __board_register_camera(void)
{
#if defined(CAMERA_NAME)
    OPERATE_RET rt = OPRT_OK;
    TDD_DVP_SR_USR_CFG_T camera_cfg = {
        .pwr = {
            .pin = BOARD_CAMERA_POWER_PIN,
            .active_level = BOARD_CAMERA_PWR_ACTIVE_LV,
        },
        .rst = {
            .pin = BOARD_CAMERA_RST_PIN,
            .active_level = BOARD_CAMERA_RST_ACTIVE_LV,
        },
        .i2c ={
            .port = BOARD_CAMERA_I2C_PORT,
            .clk  = BOARD_CAMERA_I2C_SCL,
            .sda  = BOARD_CAMERA_I2C_SDA,
        },
    };

    TUYA_CALL_ERR_RETURN(tdl_camera_dvp_ov2640_register(CAMERA_NAME, &camera_cfg)); 
#endif

    return OPRT_OK;
}
#else 
static OPERATE_RET __board_register_camera(void)
{
    return OPRT_OK;
}
#endif

OPERATE_RET board_register_ex_module(void)
{
    OPERATE_RET rt = OPRT_OK;

    TUYA_CALL_ERR_RETURN(__board_register_display());

    TUYA_CALL_ERR_RETURN(__board_register_camera());

    return rt;
}
