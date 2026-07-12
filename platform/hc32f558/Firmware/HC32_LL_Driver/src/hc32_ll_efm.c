/**
 *******************************************************************************
 * @file  hc32_ll_efm.c
 * @brief This file provides firmware functions to manage the Embedded Flash
 *        Memory unit (EFM).
 @verbatim
   Change Logs:
   Date             Author          Notes
   2026-04-16       CDT             First version
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2026, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_efm.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_EFM EFM
 * @brief Embedded Flash Management Driver Library
 * @{
 */

#if (LL_EFM_ENABLE == DDL_ON)
/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup EFM_Local_Types EFM Local Types
 * @{
 */
/**
 * @brief EFM_program_Struct definition
 */
typedef struct {
    uint32_t *pu32Source;
    uint32_t *pu32Dest;
    uint32_t u32UnitTotal;
    en_flag_status_t enSwap;
    en_flag_status_t enOtp;
} stc_program_param_t;
/**
 * @}
 */

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup EFM_Local_Macros EFM Local Macros
 * @{
 */
/**
 * @defgroup EFM_Timeout_Definition EFM timeout definition
 * @{
 */
#define EFM_TIMEOUT                     (HCLK_VALUE / 20000UL)  /* EFM wait read timeout */
#define EFM_PGM_TIMEOUT                 (HCLK_VALUE / 20000UL)  /* EFM Program timeout max 53us */
#define EFM_ERASE_TIMEOUT               (HCLK_VALUE / 50UL)     /* EFM Erase timeout max 20ms */
#define EFM_SEQ_PGM_TIMEOUT             (HCLK_VALUE / 62500UL)  /* EFM Sequence Program timeout max 16us */
/**
 * @}
 */

/**
 * @defgroup EFM_Sequence_Byte_Length EFM Sequence Byte Length
 * @{
 */
#define EFM_SEQ_BYTE_LEN                (512U)
#define EFM_SEQ_UNIT_GROUP              (8UL)
/**
 * @}
 */

/**
 * @defgroup EFM_Address_Definition EFM address definition
 * @note     For assert check use only.
 * @{
 */
#define EFM_OTP_START_ADDR              (0x03000000UL)
#define EFM_OTP_END_ADDR                (0x030017FFUL)      /*!< OTP end address */
#define EFM_OTP_END_ADDR1               (0x0001FFFFUL)      /*!< OTP end address 1 */
#define EFM_OTP_LOCK_ADDR_START         (0x03001800UL)      /*!< OTP lock address */
#define EFM_ICG_START_ADDR              (0x0300E000UL)      /*!< ICG start address */
#define EFM_ICG_END_ADDR                (0x0300E06BUL)      /*!< ICG end address */
#define EFM_SECURITY_START_ADDR         (0x0300E020UL)      /*!< Flash security start address */
#define EFM_SECURITY_END_ADDR           (0x0300E07BUL)      /*!< Flash security end address */
/**
 * @}
 */

#define REMCR_REG(x)                    (*(__IO uint32_t *)((uint32_t)(&CM_EFM->MMF_REMCR0) + (4UL * (x))))

#define EFM_FLAG0_POS                   (0U)
#define EFM_FLAG1_POS                   (16U)
#define EFM_OTP_END_SECTOR_NUM          (15U)
#define EFM_FLAG_OFFSET(enSwapState, enOtpStat, Addr)                                                \
(   (((Addr) > EFM_ICG_START_ADDR) ||                                                                \
    ((((enSwapState) == RESET) && (((Addr) >= EFM_FLASH_1_START_ADDR) && (Addr) < EFM_END_ADDR))) || \
    (((enSwapState) == SET)  && ((enOtpStat) == RESET) && ((Addr) < EFM_FLASH_1_START_ADDR))      || \
    ((((enSwapState) == SET) && ((enOtpStat) == SET)) && (((Addr) >= EFM_OTP_END_ADDR1) &&           \
    ((Addr) <= EFM_SWAP_FLASH1_END_ADDR)))) ? EFM_FLAG1_POS : EFM_FLAG0_POS)

/**
 * @defgroup EFM_Sector_Write_Protect EFM sector write protect define
 * @{
 */
#define REG_LEN                         (32U)
#define EFM_SWAP_FLASH1_END_SECTOR_NUM  (EFM_FLASH1_START_SECTOR_NUM + EFM_OTP_END_SECTOR_NUM)
#define EFM_SWAP_FLASH1_END_ADDR        (EFM_FLASH_1_START_ADDR + EFM_OTP_END_ADDR1)

#define EFM_FNWPRT_REG_OFFSET           (4UL)
#define EFM_FNWPRT_REG_NUM              (2UL)
#define EFM_FNWPRT_SECTOR_NUM_PER_BIT   (1UL)
#define EFM_FLASH1_START_SECTOR_NUM     (32U)
#define FNWPRT_REG                      (CM_EFM->F0NWPRT)
#define EFM_FNWPRT_REG_END_ADDR         (__IO uint32_t *)(uint32_t)(&CM_EFM->F1NWPRT)
/**
 * @}
 */

/**
 * @defgroup EFM_protect EFM protect define
 * @{
 */
#define EFM_SECURITY_LEN                (12UL)
#define EFM_PROTECT1_KEY                (0xAF180402UL)
#define EFM_PROTECT2_KEY                (0xA85173AEUL)

#define EFM_PROTECT3_KEY                (0x42545048UL)
#define EFM_PROTECT1_ADDR               (0x0300E030UL)
#define EFM_PROTECT2_ADDR               (0x0300E034UL)
#define EFM_PROTECT3_ADDR1              (0x0300E020UL)
#define EFM_PROTECT3_ADDR2              (0x0300E024UL)
#define EFM_PROTECT3_ADDR3              (0x0300E028UL)
/**
 * @}
 */

#define EFM_SWAP_ON_PROTECT_SECTOR_NUM  EFM_FLASH1_START_SECTOR_NUM

/**
 * @defgroup EFM_ECC_BIT_MASK EFM ECC Bit Mask
 * @{
 */
#define EFM_ECC_BIT_MASK_ECC_DATA       (0x1FFUL)
#define EFM_ECC_ERR_INJECT_BIT_MAX      (137UL)
#define EFM_ECC_ADDR_ERR_INJECT_BIT_MIN (140UL)
#define EFM_ECC_ADDR_ERR_INJECT_BIT_MAX (160UL)
/**
 * @}
 */

/**
 * @defgroup EFM_ECC_Error_Addr_Definition ECC error address definition
 * @{
 */
#define EFM_ECC_ERR_ADDR_VALID_BIT      (EFM_FERR0_VALID)
#define EFM_ECC_ERR_FATAL_BIT           (EFM_FERR0_FATAL)
#define EFM_ECC_ERR_OTP_ADDR_MASK       (0x03000000UL)                          /*!< OTP address out of 0x03000000UL   */
#define EFM_ECC_ERR_ADDR_MASK           (0x000FFFF0UL)                          /*!< F0ERR0.INDEX[15:0] -> F0ERR0[23:4] */
/**
 * @}
 */

/**
 * @defgroup EFM_ECC_Error_Inject_Register EFM ECC Error Inject Register
 * @{
 */
#define EFM_REG_ERIx(reg, x)            (*(__IO uint32_t *)((uint32_t)(&(reg)) + ((uint32_t)(x) * 0x4UL)))
/**
 * @}
 */

/**
 * @defgroup EFM_Check_Parameters_Validity EFM Check Parameters Validity
 * @{
 */
/* Parameter validity check for efm chip . */
#define IS_EFM_CHIP(x)                                                          \
(   ((x) == EFM_CHIP0)                  ||                                      \
    ((x) == EFM_CHIP1)                  ||                                      \
    ((x) == EFM_CHIP_ALL))

/* Parameter validity check for flash latency. */
#define IS_EFM_WAIT_CYCLE(x)            ((x) <= EFM_WAIT_CYCLE_MAX)

/* Parameter validity check for operate mode. */
#define IS_EFM_OP_MD(x)                                                         \
(   ((x) == EFM_MD_PE_STOP)             ||                                      \
    ((x) == EFM_MD_ERASE_ONE_CHIP)      ||                                      \
    ((x) == EFM_MD_ERASE_SMART)         ||                                      \
    ((x) == EFM_MD_PGM_SMART1)          ||                                      \
    ((x) == EFM_MD_PGM_SMART2)          ||                                      \
    ((x) == EFM_MD_ERASE_ALL_CHIP)      ||                                      \
    ((x) == EFM_MD_PGM_SEQ_SMART1)      ||                                      \
    ((x) == EFM_MD_PGM_SEQ_SMART2)      ||                                      \
    ((x) == EFM_MD_READONLY))

/* Parameter validity check for flash interrupt select. */
#define IS_EFM_INT_SEL(x)               (((x) | EFM_INT_ALL) == EFM_INT_ALL)

/* Parameter validity check for flash flag. */
#define IS_EFM_FLAG(x)                  (((x) | EFM_FLAG_ALL) == EFM_FLAG_ALL)

/* Parameter validity check for flash clear flag. */
#define IS_EFM_CLRFLAG(x)               (((x) | EFM_FLAG_ALL) == EFM_FLAG_ALL)

#define EFM_GET_FLAG_STATUS(x)          ((x) == READ_REG32_BIT(CM_EFM->FSR,(x)) ? SET : RESET)
/* Parameter validity check for bus status while flash program or erase. */
#define IS_EFM_BUS_STATUS(x)                                                    \
(   ((x) == EFM_BUS_HOLD)               ||                                      \
    ((x) == EFM_BUS_RELEASE))

/* Parameter validity check for efm address. */
#define IS_EFM_ADDR(x)                                                          \
(   ((x) <= EFM_END_ADDR)               ||                                      \
    (((x) >= EFM_OTP_START_ADDR) && ((x) <= EFM_OTP_END_ADDR)) ||               \
    (((x) >= EFM_SWAP_ADDR) && ((x) < (EFM_SWAP_ADDR + EFM_PGM_UNIT_BYTES))) || \
    (((x) >= EFM_OTP_ENABLE_ADDR) && ((x) < (EFM_OTP_ENABLE_ADDR + EFM_PGM_UNIT_BYTES)))  || \
    (((x) >= EFM_ICG_START_ADDR) && ((x) < (EFM_ICG_END_ADDR)))  || \
    (((x) >= EFM_SECURITY_START_ADDR) && ((x) <= EFM_SECURITY_END_ADDR)))

/* Parameter validity check for efm erase address. */
#define IS_EFM_ERASE_ADDR(x)                                                    \
(   ((x) <= EFM_END_ADDR)               ||                                      \
    (((x) >= EFM_SWAP_ADDR) && ((x) < (EFM_SWAP_ADDR + EFM_PGM_UNIT_BYTES))) || \
    (((x) >= EFM_OTP_ENABLE_ADDR) && ((x) < (EFM_OTP_ENABLE_ADDR + EFM_PGM_UNIT_BYTES)))  || \
    (((x) >= EFM_OTP_START_ADDR) && ((x) <= EFM_OTP_END_ADDR)) ||               \
    (((x) >= EFM_SECURITY_START_ADDR) &&((x) <= EFM_SECURITY_END_ADDR)))

/* Parameter validity check for EFM lock status. */
#define IS_EFM_REG_UNLOCK()             (CM_EFM->FAPRT == 0x00000001UL)

#define IS_EFM_CACHE(x)                 (((x) | EFM_CACHE_ALL) == EFM_CACHE_ALL)

/* Parameter validity check for EFM_FWMC register lock status. */
#define IS_EFM_FWMC_UNLOCK()            (bCM_EFM->FWMC_b.KEY1LOCK == 0U)

/* Parameter validity check for OTP lock status. */
#define IS_EFM_OTP_UNLOCK()             (bCM_EFM->FWMC_b.KEY2LOCK == 0U)

/* Parameter validity check for EFM sector number */
#define IS_EFM_SECTOR_NUM(x)            ((x) <= 256U)
#define IS_EFM_SECTOR_IDX(x)            ((x) < 256U)

/* Parameter validity check for sector protected register locking. */
#define IS_EFM_SECTOR_PROTECT_REG_LOCK(x)           ((x) <= 0xFFU)

/* Parameter validity check for EFM remap lock status. */
#define IS_EFM_REMAP_UNLOCK()           (CM_EFM->MMF_REMPRT == 0x00000001UL)

/* Parameter validity check for EFM remap index */
#define IS_EFM_REMAP_IDX(x)                                                     \
(   ((x) == EFM_REMAP_IDX0)             ||                                      \
    ((x) == EFM_REMAP_IDX1))

/* Parameter validity check for EFM remap size */
#define IS_EFM_REMAP_SIZE(x)                                                    \
(   ((x) >= EFM_REMAP_4K)               &&                                      \
    ((x) <= EFM_REMAP_SIZE_MAX))

/* Parameter validity check for EFM remap address */
#define IS_EFM_REMAP_ADDR(x)                                                    \
(   ((x) <= EFM_REMAP_ROM_END_ADDR)     ||                                      \
    (((x) >= EFM_REMAP_RAM_START_ADDR)  &&                                      \
    ((x) <= EFM_REMAP_RAM_END_ADDR)))

/* Parameter validity check for EFM remap state */
#define IS_EFM_REMAP_STATE(x)                                                   \
(   ((x) == EFM_REMAP_OFF)              ||                                      \
    ((x) == EFM_REMAP_ON))

/* Parameter validity check for EFM protect level */
#define IS_EFM_PROTECT_LEVEL(x)         (((x) & EFM_PROTECT_LEVEL_ALL) != 0U)
/* Parameter validity check for EFM security code length */
#define IS_EFM_SECURITY_CODE_LEN(x)     ((x) <= EFM_SECURITY_LEN)

/* Parameter validity check for EFM ECC mode */
#define IS_EFM_ECC_MD(x)                                                        \
(   ((x) == EFM_ECC_MD_INVD             ||                                      \
    ((x) == EFM_ECC_MD1)                ||                                      \
    ((x) == EFM_ECC_MD2)                ||                                      \
    ((x) == EFM_ECC_MD3)))

/* Parameter validity check for EFM ECC exception type */
#define IS_EFM_ECC_EXP_TYPE(x)                                                  \
(   ((x) == EFM_ECC_EXP_TYPE_NMI)       ||                                      \
    ((x) == EFM_ECC_EXP_TYPE_RST))

/* Parameter validity check for EFM ECC data */
#define IS_EFM_ECC_DATA(x)              ((x) <= EFM_ECC_BIT_MASK_ECC_DATA)

/* Parameter validity check for EFM ECC read mode */
#define IS_EFM_ECC_RD_MD(x)             (((x) | EFM_ECC_RD_ERR_INJECT_ALL) == EFM_ECC_RD_ERR_INJECT_ALL)

/* Parameter validity check for EFM ECC blank read mode */
#define IS_EFM_ECC_BLK_RD_MD(x)         (((x) | EFM_FWMC_BLKECCRSEL) == EFM_FWMC_BLKECCRSEL)

/* Parameter validity check for EFM ECC blank write mode */
#define IS_EFM_ECC_BLK_WR_MD(x)         (((x) | EFM_FWMC_BLKECCWSEL) == EFM_FWMC_BLKECCWSEL)

/* Parameter validity check for EFM ECC error flag */
#define IS_EFM_ECC_ERR_FLAG(x)          (((x) | EFM_ECC_FLAG_ALL) == EFM_ECC_FLAG_ALL)

/* Parameter validity check for EFM ECC error inject bit */
#define IS_EFM_ECC_ERR_INJECT_BIT(x)                                            \
(   ((x) < EFM_ECC_ERR_INJECT_BIT_MAX)  ||                                      \
    (((x) >= EFM_ECC_ADDR_ERR_INJECT_BIT_MIN) && ((x) < EFM_ECC_ADDR_ERR_INJECT_BIT_MAX)))

/* Parameter validity check for EFM CKCR register write protect */
#define IS_EFM_CKCR_UNLOCK()            (CM_EFM->CKPR == EFM_ECC_CKCR_UNLOCK_KEY)

/* Parameter validity check for EFM data mode */
#define IS_EFM_ECC_DATA_MD(x)           (((x) | EFM_ECC_DATA_RD_WR_REG) == EFM_ECC_DATA_RD_WR_REG)

/* Parameter validity check for EFM blank read mode */
#define IS_EFM_BLK_RD_MD(x)                                                     \
(   ((x) != 0U)                            &&                                   \
    ((x) | EFM_RD_MD_BLANK) == EFM_RD_MD_BLANK)
/**
 * @}
 */

/**
 * @}
 */
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static int32_t EFM_WaitFlag(uint32_t u32Flag, uint32_t u32Time);
static int32_t EFM_WaitEnd(uint8_t u8Shift, uint32_t u32Time);
static int32_t EFM_SetMode(uint32_t u32Mode);
static int32_t EFM_Prepare(uint32_t u32Addr, uint32_t u32ByteLen, uint32_t *pu32CacheTemp, uint32_t u32Mode);
static int32_t EFM_Recover(uint32_t *pu32CacheTemp, int32_t i32Status);
static int32_t EFM_UnitsImpl(stc_program_param_t *pstcParam);
__EFM_FUNC static int32_t EFM_WaitAndClearFlag(uint32_t u32WaitFlag, uint32_t u32ClearFlag, uint32_t u32Time);
static int32_t EFM_SectorEraseImpl(uint32_t u32Addr);
static void EFM_PadLastUnit(uint8_t *pu8PaddingBuf, uint8_t *pu8RemainByteStart, uint32_t u32RemainByteLen);
static int32_t EFM_ProgramImpl(uint32_t u32Addr, const uint8_t *pu8Data, uint32_t u32ByteLen);
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @defgroup EFM_Local_Functions EFM Local Functions
 * @{
 */

/**
 * @brief  Wait EFM flag.
 * @param  [in] u32Flag     Specifies the flag to be wait. @ref EFM_Flag_Sel
 * @param  [in] u32Time     Specifies the time to wait while the flag not be set.
 * @retval int32_t:
 *         - LL_OK: Flag was set.
 *         - LL_ERR_TIMEOUT: Flag was not set.
 */
static int32_t EFM_WaitFlag(uint32_t u32Flag, uint32_t u32Time)
{
    __IO uint32_t u32Timeout = 0UL;
    int32_t i32Ret = LL_OK;

    while (u32Flag != READ_REG32_BIT(CM_EFM->FSR, u32Flag)) {
        u32Timeout++;
        if (u32Timeout > u32Time) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }

    return i32Ret;
}

/**
 * @brief  Wait EFM operate end.
 * @param  [in] u8Shift     Specifies the flag shift bit. @ref EFM_Flag_Sel
 * @param  [in] u32Time     Specifies the time to wait while the flag not be set.
 * @retval int32_t:
 *         - LL_OK: Flash program or erase success.
 *         - LL_ERR_NOT_RDY: Flash program or erase not ready.
 *         - LL_ERR: Flash program or erase error.
 */
static int32_t EFM_WaitEnd(uint8_t u8Shift, uint32_t u32Time)
{
    int32_t i32Ret = LL_OK;
    /* Wait for ready flag. */
    if (LL_ERR_TIMEOUT == EFM_WaitFlag(EFM_FLAG_RDY << u8Shift, u32Time)) {
        i32Ret = LL_ERR_NOT_RDY;
    }
    if (LL_OK == i32Ret) {
        if ((EFM_FLAG_OPTEND << u8Shift) == READ_REG32_BIT(CM_EFM->FSR, ((EFM_FLAG_OPTEND | EFM_FLAG_ERR) << u8Shift))) {
            /* Clear the operation end flag */
            WRITE_REG32(CM_EFM->FSCLR, EFM_FLAG_OPTEND << u8Shift);
        } else {
            i32Ret = LL_ERR;
        }
    }
    return i32Ret;
}

/**
 * @brief  Set EFM operate end.
 * @param  [in] u32Mode     Specifies the mode to set @ref EFM_OperateMode_Sel
 * @retval int32_t:
 *         - LL_OK: Set mode success.
 */
static int32_t EFM_SetMode(uint32_t u32Mode)
{
    int32_t i32Ret = LL_OK;

    /* Set the mode. */
    MODIFY_REG32(CM_EFM->FWMC, EFM_FWMC_PEMOD, u32Mode);

    return i32Ret;
}

/**
 * @brief  Do prepare work for before efm operate
 * @param  [in] u32Addr                 Starting address for efm programming
 * @param  [in] u32ByteLen              Length of the data in bytes
 * @param  [in] pu32CacheTemp           store cache set data
 * @param  [in] u32Mode                 Specifies the mode to set @ref EFM_OperateMode_Sel
 * @retval int32_t
 *         - LL_OK:                     Success
 */
static int32_t EFM_Prepare(uint32_t u32Addr, uint32_t u32ByteLen, uint32_t *pu32CacheTemp, uint32_t u32Mode)
{
    int32_t i32Ret;
    uint32_t u32ClearFlag;
    uint8_t u8Offset;
    u8Offset = EFM_FLAG_OFFSET(EFM_GetSwapStatus(), EFM_OTP_GetStatus(), u32Addr);
    u32ClearFlag = (u8Offset == EFM_FLAG0_POS) ? EFM_FLAG0_ALL : EFM_FLAG1_ALL;
    u8Offset = EFM_FLAG_OFFSET(EFM_GetSwapStatus(), EFM_OTP_GetStatus(), (u32Addr + u32ByteLen - 1UL));
    u32ClearFlag |= (u8Offset == EFM_FLAG0_POS) ? EFM_FLAG0_ALL : EFM_FLAG1_ALL;

    /* Clear the error flag. */
    SET_REG32_BIT(CM_EFM->FSCLR, u32ClearFlag);
    /* Get CACHE status and disable cache */
    *pu32CacheTemp = READ_REG32_BIT(CM_EFM->FRMC, EFM_CACHE_ALL);
    CLR_REG32_BIT(CM_EFM->FRMC, EFM_CACHE_ALL);
    /* Set Mode */
    i32Ret = EFM_SetMode(u32Mode);

    return i32Ret;
}

/**
 * @brief  Do remaining work after efm operate
 * @param  [in] pu32CacheTemp           Store previous cache set data
 * @param  [in] i32Status               The status of previous operate
 * @retval Result of the programming operation @ref Generic_Error_Codes
 *         - LL_OK:                     Success
 *         - LL_ERR:                    Fail
 *         - LL_ERR_NOT_RDY:            EFM is not ready
 */
static int32_t EFM_Recover(uint32_t *pu32CacheTemp, int32_t i32Status)
{
    int32_t i32Ret;

    /* Set to read only mode */
    i32Ret = EFM_SetMode(EFM_MD_READONLY);
    /* Recover CACHE function */
    MODIFY_REG32(CM_EFM->FRMC, EFM_CACHE_ALL, *pu32CacheTemp);

    if (i32Ret == LL_OK) {
        i32Ret = i32Status;
    }

    return i32Ret;
}

/**
 * @brief  Write several units to flash by words
 * @param  [in] pstcParam               parameter for programming @ref stc_program_param_t
 * @retval An @ref Generic_Error_Codes enumeration type value.
 *         - LL_OK:                     Success
 *         - LL_ERR:                    Fail
 *         - LL_ERR_NOT_RDY:            Fail, EFM is not ready
 */
static int32_t EFM_UnitsImpl(stc_program_param_t *pstcParam)
{
    int32_t i32Ret = LL_OK;
    uint8_t u8Offset;

    /* Program the basic unit */
    while (pstcParam->u32UnitTotal-- > 0UL) {
        /* Get the flag to offset */
        u8Offset = EFM_FLAG_OFFSET(pstcParam->enSwap, pstcParam->enOtp, ((uint32_t)pstcParam->pu32Dest));
        /* Program the basic unit */
        *pstcParam->pu32Dest++ = *pstcParam->pu32Source++;
        *pstcParam->pu32Dest++ = *pstcParam->pu32Source++;
        *pstcParam->pu32Dest++ = *pstcParam->pu32Source++;
        *pstcParam->pu32Dest++ = *pstcParam->pu32Source++;
        /* Wait EFM operate end */
        if (LL_OK != EFM_WaitEnd(u8Offset, EFM_PGM_TIMEOUT)) {
            i32Ret = LL_ERR;
        }
    }

    return i32Ret;
}

/**
 * @brief  Wait EFM flag and clear.
 * @param  [in] u32WaitFlag         Specifies the flag to be wait. @ref EFM_Flag_Sel
 * @param  [in] u32ClearFlag        Specifies the flag to be clear. @ref EFM_Flag_Sel
 * @param  [in] u32Time             Specifies the time to wait while the flag not be set.
 * @retval int32_t:
 *         - LL_OK: Flag was set.
 *         - LL_ERR_TIMEOUT: Flag was not set.
 */
__NOINLINE __EFM_FUNC static int32_t EFM_WaitAndClearFlag(uint32_t u32WaitFlag, uint32_t u32ClearFlag, uint32_t u32Time)
{
    __IO uint32_t u32Timeout = 0UL;
    int32_t i32Ret = LL_OK;
    /* Wait flag set */
    while (u32WaitFlag != READ_REG32_BIT(CM_EFM->FSR, u32WaitFlag)) {
        if (u32Timeout++ >= u32Time) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }
    /* Clear flag */
    SET_REG32_BIT(CM_EFM->FSCLR, u32ClearFlag);

    return i32Ret;
}

/**
 * @brief  Erase efm sector
 * @param  [in] u32Addr  The address in the specified sector
 * @retval Function execution status @ref Generic_Error_Codes
 *         - LL_OK:                     Success
 *         - LL_ERR:                    Fail
 *         - LL_ERR_NOT_RDY:            Fail, EFM is not ready
 */
static int32_t EFM_SectorEraseImpl(uint32_t u32Addr)
{
    int32_t i32Ret;
    uint8_t u8Offset = EFM_FLAG_OFFSET(EFM_GetSwapStatus(), EFM_OTP_GetStatus(), u32Addr);

    /* Erase sector */
    RW_MEM32((uint32_t *)u32Addr) = 0UL;
    /* Wait operate end */

    i32Ret = EFM_WaitEnd(u8Offset, EFM_ERASE_TIMEOUT);

    return i32Ret;
}

/**
 * @brief  Pad last program unit
 * @param  [in] pu8PaddingBuf           Padding data buffer
 * @param  [in] pu8RemainByteStart      Start address of remaining bytes.
 * @param  [in] u32RemainByteLen        Total number of remaining bytes.
 * @retval None
 */
static void EFM_PadLastUnit(uint8_t *pu8PaddingBuf, uint8_t *pu8RemainByteStart, uint32_t u32RemainByteLen)
{
    uint32_t i;
    for (i = 0U; i < u32RemainByteLen; i++) {
        pu8PaddingBuf[i] = pu8RemainByteStart[i];
    }
    for (i = u32RemainByteLen; i < (4U * EFM_PGM_UNIT_WORDS); i++) {
        pu8PaddingBuf[i] = EFM_PGM_PAD_BYTE;
    }
}

/**
 * @brief  EFM program implement
 * @param  [in] u32Addr                 Starting address for efm programming
 * @param  [in] pu8Data                 Pointer to the data array to be programmed
 * @param  [in] u32ByteLen              Length of the data in bytes
 * @retval Result of the programming operation @ref Generic_Error_Codes
 *         - LL_OK:                     Success
 *         - LL_ERR:                    Fail
 *         - LL_ERR_NOT_RDY:            EFM is not ready
 */
static int32_t EFM_ProgramImpl(uint32_t u32Addr, const uint8_t *pu8Data, uint32_t u32ByteLen)
{
    int32_t i32Ret = LL_OK;
    uint32_t u32RemainByteLen = u32ByteLen % (4U * EFM_PGM_UNIT_WORDS);
    uint8_t au8PaddingBuf[EFM_PGM_UNIT_WORDS * 4U];
    stc_program_param_t stcProgramParam;

    stcProgramParam.pu32Source = (uint32_t *)((uint32_t)pu8Data);
    stcProgramParam.pu32Dest = (uint32_t *)u32Addr;
    stcProgramParam.u32UnitTotal = u32ByteLen / (4U * EFM_PGM_UNIT_WORDS);
    stcProgramParam.enSwap = EFM_GetSwapStatus();
    stcProgramParam.enOtp = EFM_OTP_GetStatus();

    if (stcProgramParam.u32UnitTotal > 0U) {
        i32Ret = EFM_UnitsImpl(&stcProgramParam);
    }

    if ((u32RemainByteLen > 0U) && (i32Ret == LL_OK)) {
        EFM_PadLastUnit(au8PaddingBuf, (uint8_t *)stcProgramParam.pu32Source, u32RemainByteLen);
        stcProgramParam.pu32Source = (uint32_t *)(uint32_t)au8PaddingBuf;
        stcProgramParam.u32UnitTotal = 1U;
        i32Ret = EFM_UnitsImpl(&stcProgramParam);
    }

    return i32Ret;
}

/**
 * @}
 */

/********************************** BASE OPERATION ****************************/
/**
 * @defgroup EFM_Global_Functions EFM Global Functions
 * @{
 */
/**
 * @brief  Enable or disable EFM.
 * @param  [in] u32Chip         Specifies the FLASH. @ref EFM_Chip_Sel
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void EFM_Cmd(uint32_t u32Chip, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_CHIP(u32Chip));

    if (ENABLE == enNewState) {
        CLR_REG32_BIT(CM_EFM->FSTP, u32Chip);
    } else {
        SET_REG32_BIT(CM_EFM->FSTP, u32Chip);
    }
}

/**
 * @brief  FWMC register write enable or disable.
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void EFM_FWMC_Cmd(en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        WRITE_REG32(CM_EFM->KEY1, 0x01234567UL);
        WRITE_REG32(CM_EFM->KEY1, 0xFEDCBA98UL);
    } else {
        SET_REG32_BIT(CM_EFM->FWMC, EFM_FWMC_KEY1LOCK);
    }
}

/**
 * @brief  Set bus status while flash program or erase.
 * @param  [in] u32Status                  Specifies the new bus status while flash program or erase.
 *  This parameter can be one of the following values:
 *   @arg  EFM_BUS_HOLD:                   Bus busy while flash program or erase.
 *   @arg  EFM_BUS_RELEASE:                Bus release while flash program or erase.
 * @retval None
 */
void EFM_SetBusStatus(uint32_t u32Status)
{
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_BUS_STATUS(u32Status));
    DDL_ASSERT(IS_EFM_FWMC_UNLOCK());

    WRITE_REG32(bCM_EFM->FWMC_b.BUSHLDCTL, u32Status);
}

/**
 * @brief  Enable or Disable EFM interrupt.
 * @param  [in] u32EfmInt               Specifies the FLASH interrupt source and status. @ref EFM_Interrupt_Sel
 *   @arg  EFM_INT_OPTEND:              End of EFM Operation Interrupt source
 *   @arg  EFM_INT_PEERR:               Program/erase error Interrupt source
 *   @arg  EFM_INT_COLERR:              Read collide error Interrupt source
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   Call EFM_REG_Unlock() unlock EFM register first.
 */
void EFM_IntCmd(uint32_t u32EfmInt, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_INT_SEL(u32EfmInt));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(CM_EFM->FITE, u32EfmInt);
    } else {
        CLR_REG32_BIT(CM_EFM->FITE, u32EfmInt);
    }
}

/**
 * @brief  Check any of the specified flag is set or not.
 * @param  [in] u32Flag                    Specifies the FLASH flag to check.
 *   @arg  This parameter can be of a value of @ref EFM_Flag_Sel
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t EFM_GetAnyStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_EFM_FLAG(u32Flag));

    return ((0UL == READ_REG32_BIT(CM_EFM->FSR, u32Flag)) ? RESET : SET);
}

/**
 * @brief  Check all the specified flag is set or not.
 * @param  [in] u32Flag                    Specifies the FLASH flag to check.
 *   @arg  This parameter can be of a value of @ref EFM_Flag_Sel
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t EFM_GetStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_EFM_FLAG(u32Flag));

    return ((u32Flag == READ_REG32_BIT(CM_EFM->FSR, u32Flag)) ? SET : RESET);
}

/**
 * @brief  Clear the flash flag.
 * @param  [in] u32Flag                  Specifies the FLASH flag to clear.
 *   @arg  This parameter can be of a value of @ref EFM_Flag_Sel
 * @retval None
 * @note   Call EFM_REG_Unlock() unlock EFM register first.
 */
void EFM_ClearStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_CLRFLAG(u32Flag));

    WRITE_REG32(CM_EFM->FSCLR, u32Flag);
}

/**
 * @brief  Set the efm read wait cycles.
 * @param  [in] u32WaitCycle            Specifies the efm read wait cycles.
 *    @arg  This parameter is in the range of 0 to @ref EFM_WAIT_CYCLE_MAX
 * @retval int32_t:
 *         - LL_OK:                     Program successfully.
 *         - LL_ERR_TIMEOUT:            EFM is not ready.
 * @note   Call EFM_REG_Unlock() unlock EFM register first.
 */
int32_t EFM_SetWaitCycle(uint32_t u32WaitCycle)
{
    uint32_t u32Timeout = 0UL;

    /* Param valid check */
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_WAIT_CYCLE(u32WaitCycle));

    MODIFY_REG32(CM_EFM->FRMC, EFM_FRMC_FLWT, u32WaitCycle << EFM_FRMC_FLWT_POS);
    while (u32WaitCycle != (READ_REG32_BIT(CM_EFM->FRMC, EFM_FRMC_FLWT) >> EFM_FRMC_FLWT_POS)) {
        u32Timeout++;
        if (u32Timeout > EFM_TIMEOUT) {
            return LL_ERR_TIMEOUT;
        }
    }
    return LL_OK;
}

/**
 * @brief  Get the efm read wait cycles.
 * @param  [in] None
 * @retval Returns the efm read wait cycles
 */
uint32_t EFM_GetWaitCycle(void)
{
    return (READ_REG32_BIT(CM_EFM->FRMC, EFM_FRMC_FLWT) >> EFM_FRMC_FLWT_POS);
}

/**
 * @brief  Set the FLASH erase or program mode .
 * @param  [in] u32Mode                   Specifies the FLASH erase or program mode.
 *    @arg  This parameter can be of a value of @ref EFM_OperateMode_Sel
 * @retval int32_t:
 *         - LL_OK:                     Set mode successfully.
 *         - LL_ERR_NOT_RDY:            EFM is not ready.
 */
int32_t EFM_SetOperateMode(uint32_t u32Mode)
{
    int32_t i32Ret = LL_OK;

    DDL_ASSERT(IS_EFM_OP_MD(u32Mode));
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_FWMC_UNLOCK());

    if (LL_ERR_TIMEOUT == EFM_WaitFlag(EFM_FLAG_RDY_ALL, EFM_SEQ_PGM_TIMEOUT)) {
        i32Ret = LL_ERR_NOT_RDY;
    }

    if (i32Ret == LL_OK) {
        /* Set the program or erase mode. */
        i32Ret = EFM_SetMode(u32Mode);
    }

    return i32Ret;
}

/**
 * @brief  Get chip ID.
 * @param  None
 * @retval Returns the value of the Chip ID
 */
uint32_t EFM_GetCID(void)
{
    return READ_REG32(CM_EFM->CHIPID);
}

/**
 * @brief  Get unique ID.
 * @param  [out] pstcUID     Unique ID struct
 * @retval Returns the value of the unique ID
 */
void EFM_GetUID(stc_efm_unique_id_t *pstcUID)
{
    if (NULL != pstcUID) {
        pstcUID->u32UniqueID0 = READ_REG32(CM_EFM->UQID0);
        pstcUID->u32UniqueID1 = READ_REG32(CM_EFM->UQID1);
        pstcUID->u32UniqueID2 = READ_REG32(CM_EFM->UQID2);
    }
}

/**
 * @brief  Get wafer ID.
 * @param  None
 * @retval uint8_t     Returns the value of the wafer ID
 */
uint8_t EFM_GetWaferID(void)
{
    return (uint8_t)(READ_REG32_BIT(CM_EFM->UQID0, EFM_UQID0_WAFER_ID) >> EFM_UQID0_WAFER_ID_POS);
}

/**
 * @brief  Get wafer location (x.y).
 * @param  [out] pstcLocation     Wafer coordinate.
 * @retval Returns the wafer coordinate
 */
void EFM_GetLocation(stc_efm_location_t *pstcLocation)
{
    if (NULL != pstcLocation) {
        pstcLocation->u8X_Location = (uint8_t)(READ_REG32_BIT(CM_EFM->UQID0, EFM_UQID0_X_LOCATION) \
                                               >> EFM_UQID0_X_LOCATION_POS);
        pstcLocation->u8Y_Location = (uint8_t)(READ_REG32_BIT(CM_EFM->UQID0, EFM_UQID0_Y_LOCATION));
    }
}

/**
 * @brief  Get LOT ID.
 * @param  None
 * @retval uint64_t     Returns the value of the LOT ID
 */
uint64_t EFM_GetLotID(void)
{
    uint64_t u64LotID;
    uint32_t u32LotID0;
    u32LotID0 = READ_REG32_BIT(CM_EFM->UQID0, EFM_UQID0_LOT_ID) >> EFM_UQID0_LOT_ID_POS;
    u64LotID = ((uint64_t)READ_REG32(CM_EFM->UQID1) << 8U) | u32LotID0;
    u64LotID = u64LotID | ((uint64_t)((uint8_t)READ_REG32_BIT(CM_EFM->UQID2, EFM_UQID2_LOT_ID)) << 40U);

    return u64LotID;
}

/**
 * @brief  Reset cache RAM or cache ram release reset.
 * @param  [in] enNewState          An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void EFM_CacheRamReset(en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_EFM_REG_UNLOCK());

    WRITE_REG32(bCM_EFM->FRMC_b.CRST, enNewState);
}

/**
 * @brief  Enable or disable the flash data cache and instruction cache.
 * @param  [in] u32Cache            An @ref EFM_Cache_Select.
 * @param  [in] enNewState          An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   Call EFM_REG_Unlock() unlock EFM register first.
 */
void EFM_CacheCmd(uint32_t u32Cache, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_CACHE(u32Cache));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(CM_EFM->FRMC, u32Cache);
    } else {
        CLR_REG32_BIT(CM_EFM->FRMC, u32Cache);
    }
}

/******************************** SWAP Operation ******************************/
/**
 * @brief  Get swap status
 * @param  None
 * @retval An @ref en_flag_status_t enumeration type value.
 * @note   The swap status is invalid after called EFM_SwapCmd() without reset.
 */
__EFM_FUNC en_flag_status_t EFM_GetSwapStatus(void)
{
    return ((1U == READ_REG32(bCM_EFM->FSWP_b.FSWP)) ? SET : RESET);
}

/**
 * @brief  Enable or disable the EFM swap function.
 * @param  [in] enNewState                An @ref en_functional_state_t enumeration value.
 * @retval int32_t:
 *         - LL_OK:                     Program successfully
 *         - LL_ERR_NOT_RDY:            EFM is not ready
 *         - LL_ERR:                    Program error
 * @note   Call EFM_REG_Unlock() unlock EFM register first.
 *         Must reset chip immediately after this command and DO NOT execute any flash operations before reset.
 */
int32_t EFM_SwapCmd(en_functional_state_t enNewState)
{
    int32_t i32Ret;
    uint32_t au32SwapData[4] = {EFM_SWAP_DATA, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL};
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (enNewState == ENABLE) {
        i32Ret = EFM_ProgramUnit(EFM_SWAP_ADDR, au32SwapData);
    } else {
        i32Ret = EFM_SectorErase(EFM_SWAP_ADDR);
    }
    return i32Ret;
}

/**
 * @brief  Enable or disable the EFM swap function immediately.
 * @param  [in] enNewState                An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void EFM_RealTimeSwapCmd(en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (enNewState == ENABLE) {
        MODIFY_REG32(CM_EFM->FRLTSWP, EFM_FRLTSWP_RLTSWPC | EFM_FRLTSWP_RLTSWPKW, EFM_SWAP_IMMED_ENABLE);
    } else {
        MODIFY_REG32(CM_EFM->FRLTSWP, EFM_FRLTSWP_RLTSWPC | EFM_FRLTSWP_RLTSWPKW, EFM_SWAP_IMMED_DISABLE);
    }
}

/********************************** OTP ***************************************/
/**
 * @brief  Get otp status
 * @param  None
 * @retval An @ref en_flag_status_t enumeration type value.
 */
__EFM_FUNC en_flag_status_t EFM_OTP_GetStatus(void)
{
    en_flag_status_t enRet = SET;
    uint32_t i;

    for (i = 0UL; i < EFM_PGM_UNIT_WORDS; i++) {
        if ((RW_MEM32(EFM_OTP_ENABLE_ADDR + i * 4UL)) == 0xFFFFFFFFUL) {
            enRet = RESET;
            break;
        }
    }
    return enRet;
}

/**
 * @brief  Enable OTP Function
 * @param  None
 * @retval Function execution status @ref Generic_Error_Codes
 *         - LL_OK:                     Success
 *         - LL_ERR_NOT_RDY:            Fail, EFM is not ready
 *         - LL_ERR:                    Fail, Error occurred during programming
 */
int32_t EFM_OTP_Enable(void)
{
    int32_t i32Ret;
    uint32_t au32EnableCode[4U] = {0UL, 0UL, 0UL, 0UL};

    DDL_ASSERT(IS_EFM_OTP_UNLOCK());

    if (EFM_OTP_GetStatus() == SET) {
        return LL_OK;
    }

    i32Ret = EFM_ProgramUnit(EFM_OTP_ENABLE_ADDR, au32EnableCode);

    return i32Ret;
}

/**
 * @brief  Set block OTP by a range of block index
 * @param  [in]  u32BlockStartIdx       Start block index,range from 0 to @ref EFM_OTP_BLOCK_IDX_MAX
 * @param  [in]  u16Count               Number of block(s) starting from u32BlockStartIdx
 * @retval Function execution status @ref Generic_Error_Codes
 *         - LL_OK:                     Success
 *         - LL_ERR_NOT_RDY:            Fail, EFM is not ready
 *         - LL_ERR:                    Fail, Error occurred during programming
 */
int32_t EFM_OTP_Lock(uint32_t u32BlockStartIdx, uint16_t u16Count)
{
    uint32_t u32LockAddr;
    uint8_t au8LockCode[EFM_PGM_UNIT_BYTES];
    int32_t i32Ret;
    uint32_t u32CacheTemp;
    uint16_t i;

    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_FWMC_UNLOCK());
    DDL_ASSERT(IS_EFM_OTP_UNLOCK());
    DDL_ASSERT(u32BlockStartIdx <= EFM_OTP_BLOCK_IDX_MAX);
    DDL_ASSERT((u32BlockStartIdx + u16Count - 1UL) <= EFM_OTP_BLOCK_IDX_MAX);
    DDL_ASSERT(u16Count > 0UL);

    for (i = 0; i < EFM_PGM_UNIT_BYTES; i++) {
        au8LockCode[i] = 0U;
    }
    u32LockAddr = EFM_OTP_BLOCK_LOCKADDR(u32BlockStartIdx);
    /* Prepare */
    i32Ret = EFM_Prepare(u32LockAddr, EFM_PGM_UNIT_BYTES, &u32CacheTemp, EFM_MD_PGM_SINGLE);
    if (LL_OK == i32Ret) {
        for (i = 0; (i < u16Count) && (LL_OK == i32Ret); i++) {
            u32LockAddr = EFM_OTP_BLOCK_LOCKADDR(u32BlockStartIdx + i);
            i32Ret = EFM_ProgramImpl(u32LockAddr, au8LockCode, EFM_PGM_UNIT_BYTES);
        }
    }
    /* Recover */
    i32Ret = EFM_Recover(&u32CacheTemp, i32Ret);

    return i32Ret;
}

/********************************* Erase Operation ****************************/
/**
 * @brief  Erase efm sector
 *
 * @param  [in] u32Addr  The address in the specified sector
 * @retval Function execution status @ref Generic_Error_Codes
 *         - LL_OK:                     Success
 *         - LL_ERR_NOT_RDY:            Fail, EFM is not ready
 *         - LL_ERR:                    Fail, Error occurred during programming
 */
int32_t EFM_SectorErase(uint32_t u32Addr)
{
    int32_t i32Ret;
    uint32_t u32CacheTemp;

    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_FWMC_UNLOCK());
    DDL_ASSERT(IS_EFM_ERASE_ADDR(u32Addr));
    DDL_ASSERT(IS_ADDR_ALIGN_WORD(u32Addr));

    /* Prepare */
    i32Ret = EFM_Prepare(u32Addr, 1U, &u32CacheTemp, EFM_MD_ERASE_SECTOR);
    if (LL_OK == i32Ret) {
        i32Ret = EFM_SectorEraseImpl(u32Addr);
    }

    /* Recover */
    i32Ret = EFM_Recover(&u32CacheTemp, i32Ret);

    return i32Ret;
}

/**
 * @brief  Sequence erase efm sector
 * @param  [in] u32StartSectorNum   Specifies start sector to erase.
 *                                  This parameter can be set 0~63.
 * @param  [in] u16Count            Specifies count of sectors to erase.
 *                                  This parameter can be set 1~64.
 * @retval Function execution status @ref Generic_Error_Codes
 *         - LL_OK:                     Success
 *         - LL_ERR_NOT_RDY:            Fail, EFM is not ready
 *         - LL_ERR:                    Fail, Error occurred during programming
 */
int32_t EFM_SequenceSectorErase(uint32_t u32StartSectorNum, uint16_t u16Count)
{
    int32_t i32Ret;
    uint32_t u32CacheTemp;

    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_FWMC_UNLOCK());
    DDL_ASSERT(IS_EFM_SECTOR_IDX(u32StartSectorNum));
    DDL_ASSERT(IS_EFM_SECTOR_NUM(u32StartSectorNum + u16Count));
    DDL_ASSERT(u16Count > 0UL);

    /* Prepare */
    i32Ret = EFM_Prepare(u32StartSectorNum * EFM_SECTOR_SIZE, EFM_SECTOR_SIZE * u16Count, &u32CacheTemp, EFM_MD_ERASE_SECTOR);
    if (LL_OK == i32Ret) {
        for (uint32_t i = 0U; i < u16Count; i++) {
            i32Ret = EFM_SectorEraseImpl((u32StartSectorNum + i) * EFM_SECTOR_SIZE);
            if (LL_OK != i32Ret) {
                break;
            }
        }
    }

    /* Recover */
    i32Ret = EFM_Recover(&u32CacheTemp, i32Ret);

    return i32Ret;
}

/**
 * @brief  EFM chip erase.
 * @param  [in] u32Chip      One or any combination of @ref EFM_Chip_Sel
 * @retval Function execution status @ref Generic_Error_Codes
 *         - LL_OK:                     Success
 *         - LL_ERR_NOT_RDY:            Fail, EFM is not ready for erase
 *         - LL_ERR_INVD_PARAM:         Fail, Parameter invalid
 *         - LL_ERR:                    Fail, Error occurred during erasing
 * @note   Call EFM_REG_Unlock() unlock EFM register first.
 *         __EFM_FUNC default value is __RAM_FUNC.
 *         __EFM_FUNC also could be attributed to FLASH0 or FLASH1 which is determined by users.
 *        If want to attribute to other place, please define in hc32xxxx_conf.h.
 */
__NOINLINE __EFM_FUNC int32_t EFM_ChipErase(uint32_t u32Chip)
{
    uint32_t u32Addr = EFM_START_ADDR;
    uint32_t u32Waitflags = (EFM_FLAG_RDY | EFM_FLAG_OPTEND);
    uint32_t u32OptendFlags = EFM_FLAG_OPTEND;
    uint32_t u32ChipSel = EFM_MD_ERASE_ALL_CHIP;
    uint32_t u32ClearFlag;
    uint32_t u32ErrFlag = EFM_FLAG_ERR;
    int32_t i32Ret;
    uint32_t u32CacheTemp;

    /* Assert */
    if (!IS_EFM_REG_UNLOCK()) {
        return LL_ERR_NOT_RDY;
    }
    if ((!IS_EFM_FWMC_UNLOCK())) {
        return LL_ERR_INVD_PARAM;
    }

    if (!IS_EFM_CHIP(u32Chip)) {
        return LL_ERR_INVD_PARAM;
    }

    if (SET == EFM_OTP_GetStatus()) {
        return LL_ERR;
    }

    /* Program Prepare */
    if (EFM_CHIP1 == u32Chip) {
        u32Addr = EFM_FLASH_1_START_ADDR;
        u32Waitflags = (EFM_FLAG_RDY1 | EFM_FLAG_OPTEND1);
        u32OptendFlags = EFM_FLAG_OPTEND1;
        u32ClearFlag = EFM_FLAG1_ALL;
        u32ErrFlag = EFM_FLAG_ERR1;
        u32ChipSel = EFM_MD_ERASE_ONE_CHIP;
    } else if (EFM_CHIP0 == u32Chip) {
        u32ClearFlag = EFM_FLAG0_ALL;
        u32ChipSel = EFM_MD_ERASE_ONE_CHIP;
    } else {
        u32Waitflags = EFM_FLAG_RDY | EFM_FLAG_OPTEND | EFM_FLAG_RDY1 | EFM_FLAG_OPTEND1;
        u32OptendFlags = EFM_FLAG_OPTEND | EFM_FLAG_OPTEND1;
        u32ClearFlag = EFM_FLAG0_ALL | EFM_FLAG1_ALL;
        u32ErrFlag = EFM_FLAG_ERR | EFM_FLAG_ERR1;
    }

    /* Clear the error flag. */
    SET_REG32_BIT(CM_EFM->FSCLR, u32ClearFlag);
    /* Get CACHE status and disable cache */
    u32CacheTemp = READ_REG32_BIT(CM_EFM->FRMC, EFM_CACHE_ALL);
    CLR_REG32_BIT(CM_EFM->FRMC, EFM_CACHE_ALL);

    /* Erase chip Implement */
    MODIFY_REG32(CM_EFM->FWMC, EFM_FWMC_PEMOD, u32ChipSel);
    RW_MEM32((uint32_t *)u32Addr) = 0UL;
    i32Ret = EFM_WaitAndClearFlag(u32Waitflags, u32OptendFlags, EFM_ERASE_TIMEOUT);

    /* Disable Swap */
    if ((1UL == READ_REG32(bCM_EFM->FSWP_b.FSWP)) && (i32Ret  == LL_OK) && (EFM_CHIP_ALL == u32Chip)) {
        MODIFY_REG32(CM_EFM->FWMC, EFM_FWMC_PEMOD, EFM_MD_ERASE_SECTOR);
        RW_MEM32(EFM_SWAP_ADDR) = 0UL;
        i32Ret = EFM_WaitAndClearFlag((EFM_FLAG_RDY | EFM_FLAG_OPTEND), EFM_FLAG_OPTEND, EFM_ERASE_TIMEOUT);
    }

    MODIFY_REG32(CM_EFM->FWMC, EFM_FWMC_PEMOD, EFM_MD_READONLY);

    if (LL_OK == i32Ret) {
        /* Program_after */
        if ((CM_EFM->FSR & u32ErrFlag) != 0UL) {
            i32Ret = LL_ERR;
        }
    }
    /* Restore cache set */
    MODIFY_REG32(CM_EFM->FRMC, EFM_CACHE_ALL, u32CacheTemp);

    return i32Ret;
}

/********************************* Program Operation **************************/
/**
 * @brief  Program EFM using single mode
 * @param  [in] u32Addr                 Starting address for efm programming
 * @param  [in] pu8DataSource           Pointer to the data source to be programmed
 * @param  [in] u32ByteLen              Length of the data in bytes
 * @retval Function execution status @ref Generic_Error_Codes
 *         - LL_OK:                     Success
 *         - LL_ERR_NOT_RDY:            Fail, EFM is not ready for Program
 *         - LL_ERR:                    Fail, Error occurred during programming
 * @note  Call EFM_REG_Unlock() to unlock EFM register first
 */
int32_t EFM_Program(uint32_t u32Addr, const uint8_t *pu8DataSource, uint32_t u32ByteLen)
{
    int32_t i32Ret;
    uint32_t u32CacheTemp;

    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_FWMC_UNLOCK());
    DDL_ASSERT(IS_EFM_ADDR(u32Addr));
    DDL_ASSERT(IS_EFM_ADDR(u32Addr + u32ByteLen - 1UL));
    DDL_ASSERT(IS_ADDR_ALIGN(u32Addr, EFM_PGM_UNIT_BYTES));

    /* Prepare */
    i32Ret = EFM_Prepare(u32Addr, u32ByteLen, &u32CacheTemp, EFM_MD_PGM_SINGLE);
    if (LL_OK == i32Ret) {
        i32Ret = EFM_ProgramImpl(u32Addr, pu8DataSource, u32ByteLen);
    }
    /* Recover */
    i32Ret = EFM_Recover(&u32CacheTemp, i32Ret);

    return i32Ret;
}

/**
 * @brief  Program EFM using sequence mode
 * @param  [in] u32Addr                 Starting address for efm programming
 * @note   This parameter must be a multiple of 512.
 * @param  [in] pu8DataSource           Pointer to the data source to be programmed
 * @param  [in] u32ByteLen              Length of the data in bytes
 * @note   This parameter must be a multiple of 512.
 * @retval Function execution status @ref Generic_Error_Codes
 *         - LL_OK:                     Success
 *         - LL_ERR_NOT_RDY:            Fail, EFM is not ready for Program
 *         - LL_ERR_INVD_PARAM:         Fail, Parameter invalid
 *         - LL_ERR:                    Fail, Error occurred during programming
 * @note   1)__EFM_FUNC  is used to ensure this program is executed on a memory location distinct
 *         from the efm chip being programmed, or alternatively, on a different memory type such as RAM.
 *         __EFM_FUNC default value is __RAM_FUNC.
 *         __EFM_FUNC also could be attributed to FLASH0 or FLASH1 which is determined by users.
 *         If want to attribute to other place, please define in hc32xxxx_conf.h.
 *         2)Call EFM_REG_Unlock() to unlock EFM register first
 */
__EFM_FUNC int32_t EFM_SequenceProgram(uint32_t u32Addr, const uint8_t *pu8DataSource, uint32_t u32ByteLen)
{
    int32_t i32Ret = LL_OK;
    uint32_t u32ClearFlag;
    uint32_t u32ReadFlag;
    uint32_t u32WaitFlag;
    uint32_t u32CacheTemp;
    uint32_t u32SeqPgmGrpCnt = 0UL;
    uint32_t *pu32Source = (uint32_t *)(uint32_t)pu8DataSource;
    uint32_t *pu32Dest = (uint32_t *)u32Addr;
    uint32_t u32Timeout;
    uint32_t u32LoopWords = u32ByteLen / (4U * EFM_PGM_UNIT_WORDS);
    uint32_t u32RemainBytes = u32ByteLen % (4U * EFM_PGM_UNIT_WORDS);
    uint8_t i;
    uint8_t au8PaddingBuf[EFM_PGM_UNIT_WORDS * 4U];
    uint32_t u32WaitFlagInit = EFM_FLAG_OPTEND;
    uint8_t u8Offset;
    en_flag_status_t enSwapStatus = EFM_GetSwapStatus();
    en_flag_status_t enOTPStatus = EFM_OTP_GetStatus();

    if (0UL != (u32ByteLen % EFM_SEQ_BYTE_LEN)) {
        return LL_ERR_INVD_PARAM;
    }
    if (0UL != (u32Addr % EFM_SEQ_BYTE_LEN)) {
        return LL_ERR_INVD_PARAM;
    }
    if (!IS_ADDR_ALIGN_WORD((uint32_t)pu8DataSource)) {
        return LL_ERR_INVD_PARAM;
    }

    /* Parameter check */
    if (!IS_EFM_REG_UNLOCK()) {
        return LL_ERR_NOT_RDY;
    }
    if ((!IS_EFM_FWMC_UNLOCK()) || (!IS_EFM_ADDR(u32Addr)) || (!IS_EFM_ADDR(u32Addr + u32ByteLen - 1UL))) {
        return LL_ERR_INVD_PARAM;
    }
    if (!IS_ADDR_ALIGN(u32Addr, EFM_PGM_UNIT_BYTES)) {
        return LL_ERR_INVD_PARAM;
    }

    /* Clear the error flag. */
    u32ReadFlag = EFM_FLAG_RDY_ALL;
    u8Offset = EFM_FLAG_OFFSET(enSwapStatus, enOTPStatus, u32Addr);
    u32ClearFlag = (u8Offset == EFM_FLAG0_POS) ? EFM_FLAG0_ALL : EFM_FLAG1_ALL;
    u8Offset = EFM_FLAG_OFFSET(enSwapStatus, enOTPStatus, (u32Addr + u32ByteLen - 1UL));
    u32ClearFlag |= (u8Offset == EFM_FLAG0_POS) ? EFM_FLAG0_ALL : EFM_FLAG1_ALL;

    /* Clear the error flag. */
    SET_REG32_BIT(CM_EFM->FSCLR, u32ClearFlag);
    /* Get cache status and disable cache */
    u32CacheTemp = READ_REG32_BIT(CM_EFM->FRMC, EFM_CACHE_ALL);
    CLR_REG32_BIT(CM_EFM->FRMC, EFM_CACHE_ALL);
    /* Set sequence program mode. */
    MODIFY_REG32(CM_EFM->FWMC, EFM_FWMC_PEMOD, EFM_MD_PGM_SEQ);

    while ((u32LoopWords-- > 0UL) && (LL_OK == i32Ret)) {
        if (++u32SeqPgmGrpCnt < EFM_SEQ_UNIT_GROUP) {
            u32WaitFlagInit = EFM_FLAG_SPGMEND;
        } else {
            u32SeqPgmGrpCnt = 0UL;
            u32WaitFlagInit = EFM_FLAG_OPTEND;
        }
        u8Offset = EFM_FLAG_OFFSET(enSwapStatus, enOTPStatus, (uint32_t)pu32Dest);
        u32WaitFlag = u32WaitFlagInit << u8Offset;
        /* program data. */
        *pu32Dest++ = *pu32Source++;
        *pu32Dest++ = *pu32Source++;
        *pu32Dest++ = *pu32Source++;
        *pu32Dest++ = *pu32Source++;
        /* wait for operation end flag. */
        i32Ret = EFM_WaitAndClearFlag(u32WaitFlag, u32WaitFlag, EFM_PGM_TIMEOUT);
    }

    if ((0U != u32RemainBytes) && (LL_OK == i32Ret)) {
        u8Offset = EFM_FLAG_OFFSET(enSwapStatus, enOTPStatus, (uint32_t)pu32Dest);
        u32WaitFlag = u32WaitFlagInit << u8Offset;
        for (i = 0U; i < u32RemainBytes; i++) {
            au8PaddingBuf[i] = pu8DataSource[u32ByteLen - u32RemainBytes + i];
        }
        for (i = (uint8_t)u32RemainBytes; i < (4U * EFM_PGM_UNIT_WORDS); i++) {
            au8PaddingBuf[i] = EFM_PGM_PAD_BYTE;
        }
        pu32Source = (uint32_t *)(uint32_t)au8PaddingBuf;
        /* program data. */
        *pu32Dest++ = *pu32Source++;
        *pu32Dest++ = *pu32Source++;
        *pu32Dest++ = *pu32Source++;
        *pu32Dest = *pu32Source;
        /* wait for operation end flag. */
        i32Ret = EFM_WaitAndClearFlag(u32WaitFlag, u32WaitFlag, EFM_PGM_TIMEOUT);

    }

    /* Set read only mode. */
    MODIFY_REG32(CM_EFM->FWMC, EFM_FWMC_PEMOD, EFM_MD_READONLY);
    /* Wait for ready flag. */
    u32Timeout = 0UL;
    while (u32ReadFlag != READ_REG32_BIT(CM_EFM->FSR, u32ReadFlag)) {
        if (u32Timeout++ >= EFM_PGM_TIMEOUT) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }
    /* Recover CACHE */
    MODIFY_REG32(CM_EFM->FRMC, EFM_CACHE_ALL, u32CacheTemp);

    return i32Ret;
}

/**
 * @brief  Write a unit to flash by word
 * @param  [in] u32Addr                 Specifies the program address.
 * @param  [in] pu32Buf                 Specifies the pointer to be written.
 * @retval An @ref Generic_Error_Codes enumeration type value.
 *         - LL_OK:                     Success
 *         - LL_ERR_NOT_RDY:            Fail, EFM is not ready
 *         - LL_ERR:                    Fail, Error occurred during programming
 */
int32_t EFM_ProgramUnit(uint32_t u32Addr, const uint32_t *pu32Buf)
{
    int32_t i32Ret;
    uint8_t u8Offset;
    const uint32_t *pu32Source = pu32Buf;
    uint32_t u32CacheTemp = 0U;
    uint32_t u32UnitWords = EFM_PGM_UNIT_WORDS;

    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_FWMC_UNLOCK());
    DDL_ASSERT(IS_EFM_ADDR(u32Addr));
    DDL_ASSERT(IS_ADDR_ALIGN(u32Addr, EFM_PGM_UNIT_WORDS * 4U));

    /* Get the flag to offset */
    u8Offset = EFM_FLAG_OFFSET(EFM_GetSwapStatus(), EFM_OTP_GetStatus(), u32Addr);

    /* Prepare */
    i32Ret = EFM_Prepare(u32Addr, EFM_PGM_UNIT_WORDS * 4U, &u32CacheTemp, EFM_MD_PGM_SINGLE);
    if (LL_OK == i32Ret) {
        /* Program */
        while (0UL != u32UnitWords--) {
            /* Program data. */
            RW_MEM32(u32Addr) = *pu32Source++;
            u32Addr += 4U;
        }
        /* Wait EFM operate end */
        i32Ret = EFM_WaitEnd(u8Offset, EFM_PGM_TIMEOUT);
    }

    /* Recover */
    i32Ret = EFM_Recover(&u32CacheTemp, i32Ret);

    return i32Ret;
}

/***************************** Read Operation *********************************/
/**
 * @brief  EFM read byte.
 * @param  [in] u32Addr               Specifies the address to read.
 * @param  [in] pu8ReadBuf            Specifies the read buffer.
 * @param  [in] u32ByteLen            Specifies the length to read.
 * @retval int32_t:
 *         - LL_OK: Read successfully
 *         - LL_ERR_NOT_RDY: EFM is not ready.
 */
int32_t EFM_ReadByte(uint32_t u32Addr, uint8_t *pu8ReadBuf, uint32_t u32ByteLen)
{
    int32_t i32Ret = LL_OK;
    __IO uint8_t *pu8Buf = (uint8_t *)u32Addr;

    DDL_ASSERT(IS_EFM_ADDR(u32Addr));
    DDL_ASSERT(IS_EFM_ADDR(u32Addr + u32ByteLen - 1UL));

    if (NULL == pu8ReadBuf) {
        return LL_ERR_INVD_PARAM;
    }

    while (0UL != u32ByteLen) {
        *(pu8ReadBuf++) = *(pu8Buf++);
        u32ByteLen--;
    }

    return i32Ret;
}

/**
 * @brief  EFM Blank Read
 * @param  [in] u32Addr                 Specifies the address to read.
 * @param  [in] u32ByteLen              Specifies the length to read.
 * @param  [in] u32Mode                 Specifies the blank read mode @ref EFM_Blank_Read_Mode
 * @param  [out] pu32ErrorAddr          The error address, default value is 0xFFFFFFFFUL
 * @retval int32_t:
 *         - LL_OK: check OK, all data include ecc is '1'.
 *         - LL_ERR: Check fail, found not all '1'.
 * @note  It's better to disable IRQ while blank read
 *        It's better to reset cache ram before blank read
 */
__NOINLINE __EFM_FUNC int32_t EFM_BlankRead(uint32_t u32Addr, uint32_t u32ByteLen, uint32_t u32Mode, uint32_t *pu32ErrorAddr)
{
    int32_t i32Ret = LL_OK;

    __IO uint32_t *FRMCx = &CM_EFM->FRMC2;

    if ((!IS_EFM_REG_UNLOCK()) || (!IS_EFM_ADDR(u32Addr)) || (!IS_EFM_ADDR(u32Addr + u32ByteLen - 1UL)) || (!IS_EFM_BLK_RD_MD(u32Mode))) {
        return LL_ERR_INVD_PARAM;
    }

    /* Clear blank check error status */
    WRITE_REG32(CM_EFM->FSCLR, EFM_FLAG_BLKERR_ALL);
    /* Set blank read mode */
    SET_REG32_BIT(*FRMCx, u32Mode);
    /* Blank read */
    while (0UL != u32ByteLen--) {
        (void)RW_MEM8(u32Addr);
        if (0U != READ_REG32_BIT(CM_EFM->FSR, EFM_FLAG_BLKERR_ALL)) {
            if (NULL != pu32ErrorAddr) {
                *pu32ErrorAddr = u32Addr;
            }
            i32Ret = LL_ERR;
            break;
        }
        u32Addr++;
    }

    /* Clear error flag */
    WRITE_REG32(CM_EFM->FSCLR, EFM_FLAG_BLKERR_ALL);
    /* Exit blank read mode */
    CLR_REG32_BIT(*FRMCx, u32Mode);

    return i32Ret;
}

/********************************* ECC ****************************************/
/**
 * @brief  Get ECC error address
 * @param  [in] u32Chip                 One of @ref EFM_Chip_Sel
 * @param  [out] pu32EccAddr            Pointer to the ECC error address
 * @retval None
 */
void EFM_ECC_GetErrorAddr(uint32_t u32Chip, uint32_t *pu32EccAddr)
{
    uint8_t i;
    __IO uint32_t *FxERRx = &CM_EFM->F0ERR0;

    DDL_ASSERT(((EFM_CHIP0 == u32Chip) || (EFM_CHIP1 == u32Chip)));

    if (EFM_CHIP1 == u32Chip) {
        FxERRx = &CM_EFM->F1ERR0;
    }
    if (NULL != pu32EccAddr) {
        for (i = 0U; i < 2U; i++) {
            if (0UL != READ_REG32_BIT(*FxERRx, EFM_ECC_ERR_ADDR_VALID_BIT)) {
                /* Valid address */
                *pu32EccAddr = READ_REG32_BIT(*FxERRx, EFM_ECC_ERR_ADDR_MASK);
                if (0UL != READ_REG32_BIT(*FxERRx, BIT_MASK_22)) {
                    /* OTP address */
                    *pu32EccAddr |= EFM_ECC_ERR_OTP_ADDR_MASK;
                } else {
                    /* Flash address */
                    if (EFM_CHIP1 == u32Chip) {
                        *pu32EccAddr |= EFM_FLASH_1_START_ADDR;
                    }
                }
            } else {
                /* Invalid address */
                *pu32EccAddr = 0xFFFFFFFFUL;
            }
            pu32EccAddr++;
            FxERRx++;
        }
    }
}

/**
 * @brief  Get ECC error address
 * @param  [in] u32Chip                 One of @ref EFM_Chip_Sel
 * @retval None
 */
void EFM_ECC_ClearErrorAddr(uint32_t u32Chip)
{
    uint8_t i;
    __IO uint32_t *FxERRx = &CM_EFM->F0ERR0;

    DDL_ASSERT(((EFM_CHIP0 == u32Chip) || (EFM_CHIP1 == u32Chip)));

    if (EFM_CHIP1 == u32Chip) {
        FxERRx = &CM_EFM->F1ERR0;
    }
    for (i = 0U; i < 2U; i++) {
        WRITE_REG32(*FxERRx, EFM_ECC_ERR_FATAL_BIT);
        FxERRx++;
    }
}

/**
 * @brief  EFM ECC config
 * @param  [in] u32Chip                 Specifies the chip to config ECC @ref EFM_Chip_Sel
 * @param  [in] u32Mode                 Specifies the ECC mode @ref EFM_ECC_Mode
 * @param  [in] u32ExceptionType        Specifies the exception type while ecc error occurs @ref EFM_ECC_Exception_Type
 * @retval None
 */
void EFM_ECC_Config(uint32_t u32Chip, uint32_t u32Mode, uint32_t u32ExceptionType)
{
    DDL_ASSERT(IS_EFM_CHIP(u32Chip));
    DDL_ASSERT(IS_EFM_ECC_MD(u32Mode));
    DDL_ASSERT((IS_EFM_ECC_EXP_TYPE(u32ExceptionType)));
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_CKCR_UNLOCK());

    /* Config the mode & exception type */
    if (EFM_CHIP0 == u32Chip) {
        MODIFY_REG32(CM_EFM->CKCR, (EFM_CKCR_F0ECCOAD | EFM_CKCR_F0ECCMOD), (u32Mode | u32ExceptionType));
    } else if (EFM_CHIP1 == u32Chip) {
        MODIFY_REG32(CM_EFM->CKCR, (EFM_CKCR_F1ECCOAD | EFM_CKCR_F1ECCMOD), (u32Mode | u32ExceptionType) << EFM_CKCR_F1ECCOAD_POS);
    } else {
        WRITE_REG32(CM_EFM->CKCR, (u32Mode | u32ExceptionType) | ((u32Mode | u32ExceptionType) << EFM_CKCR_F1ECCOAD_POS));
    }
}

/**
 * @brief  Get EFM Ecc flag(s) is(are) set or not.
 * @param  [in]  u32Flag                One or any combination of @ref EFM_ECC_Flag :
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t EFM_ECC_GetStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_EFM_ECC_ERR_FLAG(u32Flag));

    return ((u32Flag == (CM_EFM->CKSR & u32Flag)) ? SET : RESET);
}

/**
 * @brief  Clear EFM check flag(s)
 * @param  [in]  u32Flag                One or any combination of  @ref EFM_ECC_Flag
 * @retval None
 */
void EFM_ECC_ClearStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_ECC_ERR_FLAG(u32Flag));

    WRITE_REG32(CM_EFM->CKSR, u32Flag);
}

/**
 * @brief  EFM ECC error injection command.
 * @param  [in] u32Chip                 Specifies the chip to config ECC @ref EFM_Chip_Sel
 * @param  [in] u32Mode                 Specifies the ECC mode @ref EFM_ECC_Mode
 * @retval None
 */
void EFM_ECC_SetMode(uint32_t u32Chip, uint32_t u32Mode)
{

    DDL_ASSERT(IS_EFM_CHIP(u32Chip));
    DDL_ASSERT(IS_EFM_ECC_MD(u32Mode));
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_CKCR_UNLOCK());

    /* Set the ecc mode */
    if (EFM_CHIP0 == u32Chip) {
        MODIFY_REG32(CM_EFM->CKCR, EFM_CKCR_F0ECCMOD, u32Mode);
    } else if (EFM_CHIP1 == u32Chip) {
        MODIFY_REG32(CM_EFM->CKCR, EFM_CKCR_F1ECCMOD, u32Mode << EFM_CKCR_F1ECCOAD_POS);
    } else {
        MODIFY_REG32(CM_EFM->CKCR, EFM_CKCR_F0ECCMOD | EFM_CKCR_F1ECCMOD, (u32Mode | (u32Mode << EFM_CKCR_F1ECCOAD_POS)));
    }
}

/**
 * @brief  EFM ECC error expect operation set.
 * @param  [in] u32Chip                 Specifies the chip to config ECC @ref EFM_Chip_Sel
 * @param  [in] u32ExceptionType        Specifies the exception type while ecc error occurs @ref EFM_ECC_Exception_Type
 * @retval None
 */
void EFM_ECC_SetExceptionType(uint32_t u32Chip, uint32_t u32ExceptionType)
{

    DDL_ASSERT(IS_EFM_CHIP(u32Chip));
    DDL_ASSERT((IS_EFM_ECC_EXP_TYPE(u32ExceptionType)));
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_CKCR_UNLOCK());

    /* Set the ecc exception type */
    if (EFM_CHIP0 == u32Chip) {
        MODIFY_REG32(CM_EFM->CKCR, EFM_CKCR_F0ECCOAD, u32ExceptionType);
    } else if (EFM_CHIP1 == u32Chip) {
        MODIFY_REG32(CM_EFM->CKCR, EFM_CKCR_F1ECCOAD, u32ExceptionType << EFM_CKCR_F1ECCOAD_POS);
    } else {
        MODIFY_REG32(CM_EFM->CKCR, EFM_CKCR_F0ECCOAD | EFM_CKCR_F1ECCOAD, u32ExceptionType | (u32ExceptionType << EFM_CKCR_F1ECCOAD_POS));
    }
}

/**
 * @brief  Set EFM ECC read mode.
 * @param  [in]  u32Mode                Specifies the ECC read mode @ref EFM_ECC_Read_mode
 * @retval None
 */
void EFM_ECC_SetReadMode(uint32_t u32Mode)
{
    __IO uint32_t *REGx = &CM_EFM->EIEN;

    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_ECC_RD_MD(u32Mode));

    /* Set ECC read mode (normal or error inject) */
    if (EFM_ECC_RD_NORMAL == u32Mode) {
        CLR_REG32_BIT(*REGx, EFM_ECC_RD_ERR_INJECT_ALL);
    } else {
        SET_REG32_BIT(*REGx, u32Mode);
    }
}

/**
 * @brief  Set EFM ECC blank read mode.
 * @param  [in]  u32Mode                Specifies the ECC blank read mode @ref EFM_ECC_Blank_Read_Mode
 * @retval None
 */
void EFM_ECC_SetBlankRead(uint32_t u32Mode)
{
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_ECC_BLK_RD_MD(u32Mode));

    MODIFY_REG32(CM_EFM->FWMC, EFM_FWMC_BLKECCRSEL, u32Mode);
}

/**
 * @brief  Set EFM ECC blank write mode.
 * @param  [in]  u32Mode             Specifies the ECC blank write mode @ref EFM_ECC_Blank_Write_Mode
 * @retval None
 */
void EFM_ECC_SetBlankWrite(uint32_t u32Mode)
{
    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_ECC_BLK_WR_MD(u32Mode));

    MODIFY_REG32(CM_EFM->FRMC, EFM_FWMC_BLKECCWSEL, u32Mode);
}

/**
 * @brief  Set the bit to inject error.
 * @param  [in] u32Chip                 Specifies the chip to set  @ref EFM_Chip_Sel
 * @param  [in] u16BitPos               Specifies the bit to inject error
 *                                      If flash data error inject, the bit position is 0 ~ 127
 *                                      If ecc data error inject, the bit position is 128 ~ 136
 *                                      If address error inject, the bit position is 140 ~ 159
 * @param  [in] enNewState              An @ref en_functional_state_t
 * @retval None
 */
void EFM_ECC_ErrorInjectBitCmd(uint32_t u32Chip, uint16_t u16BitPos, en_functional_state_t enNewState)
{
    __IO uint32_t *EFM_ERIx;

    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_CHIP(u32Chip));
    DDL_ASSERT(IS_EFM_ECC_ERR_INJECT_BIT(u16BitPos));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (EFM_CHIP0 == u32Chip) {
        EFM_ERIx = &EFM_REG_ERIx(CM_EFM->F0EIR0, ((uint32_t)u16BitPos / 32U));
    } else {
        EFM_ERIx = &EFM_REG_ERIx(CM_EFM->F1EIR0, ((uint32_t)u16BitPos / 32U));
    }

    if (ENABLE == enNewState) {
        SET_REG32_BIT(*EFM_ERIx, 1UL << (u16BitPos % 32U));
    } else {
        CLR_REG32_BIT(*EFM_ERIx, 1UL << (u16BitPos % 32U));
    }
}

/**
 * @brief  Set ECC data mode while read flash or program flash.
 * @param  [in] u32Chip                 Specifies the chip to set  @ref EFM_Chip_Sel
 * @param  [in] u32Mode                 Specifies the ECC data mode @ref EFM_ECC_Data_Sel
 * @retval None
 * @note   If call the API several times, the last setting will be valid
 */
void EFM_ECC_SetEccDataMode(uint32_t u32Chip, uint32_t u32Mode)
{
    DDL_ASSERT(IS_EFM_CHIP(u32Chip));
    DDL_ASSERT(IS_EFM_ECC_DATA_MD(u32Mode));

    /* Set ECC data mode (register or hardware) */
    if (EFM_CHIP0 == (u32Chip & EFM_CHIP0)) {
        MODIFY_REG32(CM_EFM->F0ECCCR, EFM_ECC_DATA_RD_WR_REG, u32Mode);
    }
    if (EFM_CHIP1 == (u32Chip & EFM_CHIP1)) {
        MODIFY_REG32(CM_EFM->F1ECCCR, EFM_ECC_DATA_RD_WR_REG, u32Mode);
    }
}

/**
 * @brief  Write ECC Data to register.
 *         The data will be programmed as ECC data during the programming operation while
 *         ecc data mode set to EFM_ECC_DATA_PGM_REG (EFM_ECCCR.GDIS = 1).
 * @param  [in] u32Chip                 One of @ref EFM_Chip_Sel
 * @param  [in] u32EccData              Ecc data  at range of 0~0x1FF
 * @retval None
 */
void EFM_ECC_Write(uint32_t u32Chip, uint32_t u32EccData)
{
    DDL_ASSERT(IS_EFM_CHIP(u32Chip));
    DDL_ASSERT(IS_EFM_ECC_DATA(u32EccData));

    if (EFM_CHIP0 == (u32Chip & EFM_CHIP0)) {
        WRITE_REG32(CM_EFM->F0ECCDR, u32EccData);
    }
    if (EFM_CHIP1 == (u32Chip & EFM_CHIP1)) {
        WRITE_REG32(CM_EFM->F1ECCDR, u32EccData);
    }
}

/**
 * @brief  Get ECC Data from register.
 *         ECC data will be updated to register during read cycle when the ecc
 *         data mode set to EFM_ECC_DATA_PGM_REG (EFM_ECCCR.VDIS = 1)
 * @param  [in] u32Chip                 One of @ref EFM_Chip_Sel
 * @retval uint32_t                     Ecc data at range of 0~0x1FF
 */
uint32_t EFM_ECC_Read(uint32_t u32Chip)
{
    uint32_t u32EccData;

    DDL_ASSERT((u32Chip == EFM_CHIP0) || (u32Chip == EFM_CHIP1));

    if (EFM_CHIP0 == u32Chip) {
        u32EccData = READ_REG32(CM_EFM->F0ECCDR) >> EFM_FECCDR_DRD_POS;
    } else {
        u32EccData = READ_REG32(CM_EFM->F1ECCDR) >> EFM_FECCDR_DRD_POS;
    }

    return u32EccData;
}

/************************************ PROTECT *********************************/
/**
 * @brief  Enable efm protect.
 * @param  [in] u8Level      Specifies the protect level.One or Combine of @ref EFM_Protect_Level
 * @retval None
 */
void EFM_Protect_Enable(uint8_t u8Level)
{
    uint32_t au32ProtectKey[4] = {0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL};

    DDL_ASSERT(IS_EFM_PROTECT_LEVEL(u8Level));

    if (0U != (u8Level & (EFM_PROTECT_LEVEL1 | EFM_PROTECT_LEVEL2))) {
        if (EFM_PROTECT_LEVEL1 == (u8Level & EFM_PROTECT_LEVEL1)) {
            au32ProtectKey[0] = EFM_PROTECT1_KEY;
        }
        if (EFM_PROTECT_LEVEL2 == (u8Level & EFM_PROTECT_LEVEL2)) {
            au32ProtectKey[1U] = EFM_PROTECT2_KEY;
        }
        (void)EFM_ProgramUnit(EFM_PROTECT1_ADDR, au32ProtectKey);
    }

    if (EFM_PROTECT_LEVEL3 == (u8Level & EFM_PROTECT_LEVEL3)) {
        au32ProtectKey[0U] = EFM_PROTECT3_KEY;
        au32ProtectKey[1U] = EFM_PROTECT3_KEY;
        au32ProtectKey[2U] = EFM_PROTECT3_KEY;
        (void)EFM_ProgramUnit(EFM_PROTECT3_ADDR1, au32ProtectKey);
    }
}

/**
 * @brief  Write the security code.
 * @param  [in] pu8Buf       Specifies the security code.
 * @param  [in] u32ByteLen       Specifies the length of the security code.
 * @note   1.Reading security code address always return 0xFFFFFFFF
 *         2.Make sure security code address are all 0xFFFFFFFF before calling this function
 *           One of the following methods can clear security code address to all 0xFFFFFFFF:
 *           --chip erase @ref EFM_ChipErase
 *           --Security code sector erase (only when chip is unprotected) @ref EFM_SectorErase
 * @retval int32_t
 *         - LL_OK:                     Success
 *         - LL_ERR:                    Fail
 *         - LL_ERR_TIMEOUT:            Flag was not set.
 *         - LL_ERR_NOT_RDY:            EFM is not ready for programming
 */
int32_t EFM_WriteSecurityCode(const uint8_t *pu8Buf, uint32_t u32ByteLen)
{
    int32_t i32Ret;
    uint32_t u32CacheTemp;

    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_FWMC_UNLOCK());
    DDL_ASSERT(IS_EFM_SECURITY_CODE_LEN(u32ByteLen));
    DDL_ASSERT(IS_ADDR_ALIGN_WORD((uint32_t)pu8Buf));

    /* Prepare */
    i32Ret = EFM_Prepare(EFM_SECURITY_ADDR, u32ByteLen, &u32CacheTemp, EFM_MD_PGM_SINGLE);
    if (LL_OK == i32Ret) {
        /* Program */
        i32Ret = EFM_ProgramImpl(EFM_SECURITY_ADDR, pu8Buf, u32ByteLen);
    }

    /* Recover */
    i32Ret = EFM_Recover(&u32CacheTemp, i32Ret);

    return i32Ret;
}

/**
 * @brief  Sector protected register lock.
 * @param  [in] u32RegLock                      Specifies sector protected register locking.
 *  @arg   EFM_WRLOCK0 ~ EFM_WRLOCK7
 * @retval None
 * @note   Call EFM_REG_Unlock() unlock EFM register first.
 */
void EFM_SectorProtectRegLock(uint32_t u32RegLock)
{
    DDL_ASSERT(IS_EFM_SECTOR_PROTECT_REG_LOCK(u32RegLock));
    DDL_ASSERT(IS_EFM_REG_UNLOCK());

    SET_REG32_BIT(CM_EFM->WLOCK, u32RegLock);
}

/**
 * @brief  Set sector lock or unlock (Single).
 * @param  [in] u32SectorNum    Specifies sector for unlock.
 *                              This parameter can be set 0~63.
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   Call EFM_REG_Unlock() unlock EFM register first.
 *         If you want to unlock sequence sectors,Please call EFM_SequenceSectorOperateCmd function
 */
void EFM_SingleSectorOperateCmd(uint32_t u32SectorNum, en_functional_state_t enNewState)
{
    __IO uint32_t *EFM_FxNWPRTy;
    uint32_t u32RegIndex;
    uint32_t u32BitPos;

    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_SECTOR_IDX(u32SectorNum));

    if (SET == EFM_GetSwapStatus()) {
        if (0xFFFFFFFFUL != RW_MEM32(EFM_OTP_ENABLE_ADDR)) {
            if (u32SectorNum > EFM_SWAP_FLASH1_END_SECTOR_NUM) {
                u32SectorNum -= EFM_FLASH1_START_SECTOR_NUM;
            } else if ((u32SectorNum > EFM_OTP_END_SECTOR_NUM) && (u32SectorNum < EFM_FLASH1_START_SECTOR_NUM)) {
                u32SectorNum += EFM_FLASH1_START_SECTOR_NUM;
            } else {
                /* rsvd */
            }
        } else {
            if (u32SectorNum >= EFM_FLASH1_START_SECTOR_NUM) {
                u32SectorNum -= EFM_FLASH1_START_SECTOR_NUM;
            } else {
                u32SectorNum += EFM_FLASH1_START_SECTOR_NUM;
            }
        }
    }

    u32BitPos = (u32SectorNum / EFM_FNWPRT_SECTOR_NUM_PER_BIT) % REG_LEN;
    u32RegIndex = (u32SectorNum / EFM_FNWPRT_SECTOR_NUM_PER_BIT) / REG_LEN;
    EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + (u32RegIndex << EFM_FNWPRT_REG_OFFSET));
    MODIFY_REG32(*EFM_FxNWPRTy, 1UL << u32BitPos, (uint32_t)enNewState << u32BitPos);
}

/**
 * @brief  Set sector lock or unlock (Sequence).
 * @param  [in] u32StartSectorNum   Specifies start sector to unlock.
 *                                  This parameter can be set 0~63.
 * @param  [in] u16Count            Specifies count of sectors to unlock.
 *                                  This parameter can be set 1~64.
 * @param  [in] enNewState          An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   Call EFM_REG_Unlock() unlock EFM register first.
 *         If you want to unlock only one sector,Please call EFM_SingleSectorOperateCmd function
 */
void EFM_SequenceSectorOperateCmd(uint32_t u32StartSectorNum, uint16_t u16Count, en_functional_state_t enNewState)
{
    __IO uint32_t *EFM_FxNWPRTy;
    uint32_t u32EndSectorNum;
    uint16_t u16StartRegIndex;
    uint16_t u16StartBitPos;
    uint16_t u16EndRegIndex;
    uint16_t u16EndBitPos;
    uint32_t u32RegValue;

    DDL_ASSERT(IS_EFM_REG_UNLOCK());
    DDL_ASSERT(IS_EFM_SECTOR_IDX(u32StartSectorNum));
    DDL_ASSERT(IS_EFM_SECTOR_NUM(u32StartSectorNum + u16Count));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (SET == EFM_GetSwapStatus()) {
        if (0xFFFFFFFFUL != RW_MEM32(EFM_OTP_ENABLE_ADDR)) {
            if (u32StartSectorNum > EFM_SWAP_FLASH1_END_SECTOR_NUM) {
                u32StartSectorNum -= EFM_FLASH1_START_SECTOR_NUM;
            } else if ((u32StartSectorNum > EFM_OTP_END_SECTOR_NUM) && (u32StartSectorNum < EFM_FLASH1_START_SECTOR_NUM)) {
                u32StartSectorNum += EFM_FLASH1_START_SECTOR_NUM;
            } else {
                /* rsvd */
            }
        } else {
            if (u32StartSectorNum >= EFM_FLASH1_START_SECTOR_NUM) {
                u32StartSectorNum -= EFM_FLASH1_START_SECTOR_NUM;
            } else {
                u32StartSectorNum += EFM_FLASH1_START_SECTOR_NUM;
            }
        }
    }

    u16StartRegIndex = (uint16_t)(u32StartSectorNum / REG_LEN);            /* Register offset for the start sector */
    u16StartBitPos = (uint16_t)(u32StartSectorNum % REG_LEN);              /* Bit offset for the start sector */
    u32EndSectorNum = (uint16_t)(u32StartSectorNum + u16Count - 1U);       /* Calculate the end sector */
    u16EndRegIndex = (uint16_t)(u32EndSectorNum / REG_LEN);                /* Register offset for the end sector */
    u16EndBitPos = (uint16_t)(u32EndSectorNum % REG_LEN);                  /* Bit offset for the end sector */

    if ((u16StartBitPos == 0U) && (u16EndBitPos == 31U)) {
        while (u16StartRegIndex <= u16EndRegIndex) {
            EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + ((uint32_t)u16StartRegIndex << EFM_FNWPRT_REG_OFFSET));
            if (EFM_FxNWPRTy > EFM_FNWPRT_REG_END_ADDR) {
                EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + (((uint32_t)u16StartRegIndex - EFM_FNWPRT_REG_NUM) << EFM_FNWPRT_REG_OFFSET));
            }
            if (enNewState == ENABLE) {
                WRITE_REG32(*EFM_FxNWPRTy, 0xFFFFFFFFUL);
            } else {
                WRITE_REG32(*EFM_FxNWPRTy, 0x0UL);
            }
            u16StartRegIndex += 1U;
        }
    } else {
        EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + ((uint32_t)u16StartRegIndex << EFM_FNWPRT_REG_OFFSET));
        if (u16StartRegIndex == u16EndRegIndex) {
            u32RegValue = ((1UL << u16EndBitPos) - (1UL << u16StartBitPos)) | (1UL << u16EndBitPos);
            if (enNewState == ENABLE) {
                SET_REG32_BIT(*EFM_FxNWPRTy, u32RegValue);
            } else {
                CLR_REG32_BIT(*EFM_FxNWPRTy, u32RegValue);
            }
        } else {
            u32RegValue = (BIT_MASK_31 - (1UL << u16StartBitPos)) | BIT_MASK_31;
            if (enNewState == ENABLE) {
                SET_REG32_BIT(*EFM_FxNWPRTy, u32RegValue);
                while (u16StartRegIndex < (u16EndRegIndex - 1U)) {
                    u16StartRegIndex += 1U;
                    EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + ((uint32_t)u16StartRegIndex << EFM_FNWPRT_REG_OFFSET));
                    if (EFM_FxNWPRTy > EFM_FNWPRT_REG_END_ADDR) {
                        EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + (((uint32_t)u16StartRegIndex - EFM_FNWPRT_REG_NUM) << EFM_FNWPRT_REG_OFFSET));
                    }
                    WRITE_REG32(*EFM_FxNWPRTy, 0xFFFFFFFFUL);
                }
                u16StartRegIndex += 1U;
                u32RegValue = ((1UL << u16EndBitPos) - 1UL) | (1UL << u16EndBitPos);
                EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + ((uint32_t)u16StartRegIndex << EFM_FNWPRT_REG_OFFSET));
                if (EFM_FxNWPRTy > EFM_FNWPRT_REG_END_ADDR) {
                    EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + (((uint32_t)u16EndRegIndex - EFM_FNWPRT_REG_NUM) << EFM_FNWPRT_REG_OFFSET));
                }
                SET_REG32_BIT(*EFM_FxNWPRTy, u32RegValue);
            } else {
                CLR_REG32_BIT(*EFM_FxNWPRTy, u32RegValue);
                while (u16StartRegIndex < (u16EndRegIndex - 1U)) {
                    u16StartRegIndex += 1U;
                    EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + ((uint32_t)u16StartRegIndex << EFM_FNWPRT_REG_OFFSET));
                    if (EFM_FxNWPRTy > EFM_FNWPRT_REG_END_ADDR) {
                        EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + (((uint32_t)u16StartRegIndex - EFM_FNWPRT_REG_NUM) << EFM_FNWPRT_REG_OFFSET));
                    }
                    WRITE_REG32(*EFM_FxNWPRTy, 0x0UL);
                }
                u32RegValue = ((1UL << u16EndBitPos) - 1UL) | (1UL << u16EndBitPos);
                EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + ((uint32_t)u16StartRegIndex << EFM_FNWPRT_REG_OFFSET));
                if (EFM_FxNWPRTy > EFM_FNWPRT_REG_END_ADDR) {
                    EFM_FxNWPRTy = (__IO uint32_t *)((uint32_t)(&FNWPRT_REG) + (((uint32_t)u16EndRegIndex - EFM_FNWPRT_REG_NUM) << EFM_FNWPRT_REG_OFFSET));
                }
                CLR_REG32_BIT(*EFM_FxNWPRTy, u32RegValue);
            }
        }
    }
}

/************************************ REMAP ***********************************/
/**
 * @brief  Init REMAP initial structure with default value.
 * @param  [in] pstcEfmRemapInit specifies the Parameter of REMAP.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t EFM_REMAP_StructInit(stc_efm_remap_init_t *pstcEfmRemapInit)
{
    int32_t i32Ret = LL_OK;
    if (NULL == pstcEfmRemapInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcEfmRemapInit->u32State = EFM_REMAP_OFF;
        pstcEfmRemapInit->u32Addr = 0UL;
        pstcEfmRemapInit->u32Size = EFM_REMAP_4K;
    }
    return i32Ret;
}

/**
 * @brief  REMAP initialize.
 * @param  [in] u8RemapIdx      Specifies the remap ID.
 * @param  [in] pstcEfmRemapInit specifies the Parameter of REMAP.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t EFM_REMAP_Init(uint8_t u8RemapIdx, stc_efm_remap_init_t *pstcEfmRemapInit)
{
    int32_t i32Ret = LL_OK;
    __IO uint32_t *REMCRx;

    if (NULL == pstcEfmRemapInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_EFM_REMAP_UNLOCK());
        DDL_ASSERT(IS_EFM_REMAP_IDX(u8RemapIdx));
        DDL_ASSERT(IS_EFM_REMAP_SIZE(pstcEfmRemapInit->u32Size));
        DDL_ASSERT(IS_EFM_REMAP_ADDR(pstcEfmRemapInit->u32Addr));
        DDL_ASSERT(IS_EFM_REMAP_STATE(pstcEfmRemapInit->u32State));
        if ((pstcEfmRemapInit->u32Addr % (1UL << pstcEfmRemapInit->u32Size)) != 0U) {
            i32Ret = LL_ERR_INVD_PARAM;
        } else {
            REMCRx = &REMCR_REG(u8RemapIdx);
            MODIFY_REG32(*REMCRx, EFM_MMF_REMCR_EN | EFM_MMF_REMCR_RMTADDR | EFM_MMF_REMCR_RMSIZE, \
                         pstcEfmRemapInit->u32State | pstcEfmRemapInit->u32Addr | pstcEfmRemapInit->u32Size);
        }
    }
    return i32Ret;
}

/**
 * @brief  EFM REMAP de-initialize.
 * @param  None
 * @retval None
 */
void EFM_REMAP_DeInit(void)
{
    DDL_ASSERT(IS_EFM_REMAP_UNLOCK());

    WRITE_REG32(CM_EFM->MMF_REMCR0, 0UL);
    WRITE_REG32(CM_EFM->MMF_REMCR1, 0UL);
}

/**
 * @brief  Enable or disable remap function.
 * @param  [in] u8RemapIdx      Specifies the remap ID.
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void EFM_REMAP_Cmd(uint8_t u8RemapIdx, en_functional_state_t enNewState)
{
    __IO uint32_t *REMCRx;

    DDL_ASSERT(IS_EFM_REMAP_UNLOCK());
    DDL_ASSERT(IS_EFM_REMAP_IDX(u8RemapIdx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    REMCRx = &REMCR_REG(u8RemapIdx);
    if (ENABLE == enNewState) {
        SET_REG32_BIT(*REMCRx, EFM_MMF_REMCR_EN);
    } else {
        CLR_REG32_BIT(*REMCRx, EFM_MMF_REMCR_EN);
    }
}

/**
 * @brief  Set specified remap target address.
 * @param  [in] u8RemapIdx      Specifies the remap ID.
 * @param  [in] u32Addr         Specifies the target address.
 * @retval None
 */
void EFM_REMAP_SetAddr(uint8_t u8RemapIdx, uint32_t u32Addr)
{
    __IO uint32_t *REMCRx;

    DDL_ASSERT(IS_EFM_REMAP_UNLOCK());
    DDL_ASSERT(IS_EFM_REMAP_IDX(u8RemapIdx));
    DDL_ASSERT(IS_EFM_REMAP_ADDR(u32Addr));

    REMCRx = &REMCR_REG(u8RemapIdx);
    MODIFY_REG32(*REMCRx, EFM_MMF_REMCR_RMTADDR, u32Addr);
}

/**
 * @brief  Set specified remap size.
 * @param  [in] u8RemapIdx      Specifies the remap ID.
 * @param  [in] u32Size         Specifies the remap size.
 * @retval None
 */
void EFM_REMAP_SetSize(uint8_t u8RemapIdx, uint32_t u32Size)
{
    __IO uint32_t *REMCRx;

    DDL_ASSERT(IS_EFM_REMAP_UNLOCK());
    DDL_ASSERT(IS_EFM_REMAP_IDX(u8RemapIdx));
    DDL_ASSERT(IS_EFM_REMAP_SIZE(u32Size));

    REMCRx = &REMCR_REG(u8RemapIdx);
    MODIFY_REG32(*REMCRx, EFM_MMF_REMCR_RMSIZE, u32Size);
}

/**
 * @}
 */

#endif /* LL_EFM_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
