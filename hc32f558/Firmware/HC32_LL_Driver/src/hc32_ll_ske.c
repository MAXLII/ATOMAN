/**
 *******************************************************************************
 * @file  hc32_ll_ske.c
 * @brief This file provides firmware functions to manage the Symmetric Key
 *        Engine(SKE).
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
#include "hc32_ll_ske.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_SKE SKE
 * @brief SKE Driver Library
 * @{
 */

#if (LL_SKE_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup SKE_Local_Types SKE Local Types
 * @{
 */

/**
 * @brief SKE CCM mode context structure
 */
typedef struct {
    uint32_t u32Alg;                        /*!< SKE algorithm.
                                                 This parameter can be a value of @ref SKE_Algorithm
                                            */
    uint32_t u32CryptoSize;                 /*!< Number of byte of the crypto data. */
    uint32_t u32AadSize;                    /*!< Number of byte of the AAD(Additional Authenticated Data). */
    uint32_t u32MacSize;                    /*!< Number of byte of the MAC(Message Authentication Code). */
    uint32_t u32LengthSize;                 /*!< Number of byte of length field. */
    uint32_t u32AadStartOffset;             /*!< AAD[0] to AAD[u32AadStartOffset-1] is in B1.
                                                 B1 is followed by the remaining part of AAD when block updating. */
    uint32_t u32AadRemainSize;              /*!< The remaining size of AAD. */
    uint8_t au8Buffer[16U];                 /*!< Temporary buffer, may be used to store B0 or B1. */
} stc_ske_ccm_ctx_t;

/**
 * @}
 */

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup SKE_Local_Macros SKE Local Macros
 * @{
 */

/**
 * @defgroup SKE_Check_Parameters_Validity SKE Check Parameters Validity
 * @{
 */
#define IS_SKE_BIT_MASK(x, mask)        (((x) != 0U) && (((x) | (mask)) == (mask)))

#define IS_SKE_DATA_TYPE(x)                                                    \
(   ((x) == SKE_DATA_SWAP_NON)          ||                                     \
    ((x) == SKE_DATA_SWAP_HALF_WORD)    ||                                     \
    ((x) == SKE_DATA_SWAP_BYTE)         ||                                     \
    ((x) == SKE_DATA_SWAP_BIT))

#define IS_SKE_CRYPTO(x)        (((x) == SKE_CRYPTO_ENCRYPT) || ((x) == SKE_CRYPTO_DECRYPT))

#define IS_SKE_ALG(x)                                                           \
(   ((x) == SKE_ALG_AES_128)            ||                                      \
    ((x) == SKE_ALG_AES_192)            ||                                      \
    ((x) == SKE_ALG_AES_256)            ||                                      \
    ((x) == SKE_ALG_SM4))

#define IS_SKE_XCM_MD_ALG(x)    IS_SKE_ALG(x)

#define IS_SKE_MD(x)                                                           \
(   ((x) == SKE_MD_ECB)         ||                                             \
    ((x) == SKE_MD_CBC)         ||                                             \
    ((x) == SKE_MD_CFB)         ||                                             \
    ((x) == SKE_MD_OFB)         ||                                             \
    ((x) == SKE_MD_CTR)         ||                                             \
    ((x) == SKE_MD_CMAC)        ||                                             \
    ((x) == SKE_MD_GCM)         ||                                             \
    ((x) == SKE_MD_CCM))

#define IS_SKE_IV_MD(x)                                                        \
(   ((x) == SKE_MD_CBC)         ||                                             \
    ((x) == SKE_MD_CFB)         ||                                             \
    ((x) == SKE_MD_OFB)         ||                                             \
    ((x) == SKE_MD_CTR)         ||                                             \
    ((x) == SKE_MD_GCM)         ||                                             \
    ((x) == SKE_MD_CCM))

#define IS_SKE_BASE_MD(x)                                                      \
(   ((x) == SKE_MD_ECB)         ||                                             \
    ((x) == SKE_MD_CBC)         ||                                             \
    ((x) == SKE_MD_CFB)         ||                                             \
    ((x) == SKE_MD_OFB)         ||                                             \
    ((x) == SKE_MD_CTR))

#define IS_SKE_XCM_MD(x)                                                       \
(   ((x) == SKE_MD_GCM)         ||                                             \
    ((x) == SKE_MD_CCM))

#define IS_SKE_ALG_MD(alg, md)                                                 \
(   (IS_SKE_ALG(alg) && IS_SKE_BASE_MD(md))             ||                     \
    (IS_SKE_ALG(alg) && ((md) == SKE_MD_CMAC))          ||                     \
    (IS_SKE_XCM_MD_ALG(alg) && IS_SKE_XCM_MD(md)))

#define IS_SKE_GCM_MAC_SIZE(x)          ((x) <= 16U)

#define IS_SKE_CCM_MAC_SIZE(x)          ((((x) >= 4U) && ((x) <= 16U)) && (((x) & 0x1U) == 0U))

#define IS_SKE_CCM_L_SIZE(x)            (((x) >= 2U) && ((x) <= 8U))

#define IS_SKE_XCM_MAC_SIZE(md, s)                                             \
(   (((md) == SKE_MD_GCM) && IS_SKE_GCM_MAC_SIZE(s))    ||                     \
    (((md) == SKE_MD_CCM) && IS_SKE_CCM_MAC_SIZE(s)))

#define IS_SKE_FLAG(x)                  IS_SKE_BIT_MASK(x, SKE_FLAG_ALL)
#define IS_SKE_FLAG_CLR(x)              IS_SKE_BIT_MASK(x, SKE_FLAG_CLR_ALL)
/**
 * @}
 */

/**
 * @defgroup SKE_Driver_Const_Value SKE Driver Const Value
 * @{
 */
#define SKE_CFG_INIT_MASK               (SKE_CFG_MODE | SKE_CFG_DATA_TYPE | SKE_CFG_ALG)
#define SKE_TIMEOUT_VAL                 (1000UL)
#define SKE_BLOCK_SIZE_MAX              (SKE_BLOCK_SIZE_16BYTE)
#define SKE_MAC_SIZE_MAX                (SKE_BLOCK_SIZE_16BYTE)

#define SKE_SR1_FLAG                    (SKE_FLAG_BUSY | SKE_FLAG_MID_VALID)

/* RMU timeout value */
#define SKE_RMU_TIMEOUT                 (100U)
/**
 * @}
 */

/**
 * @defgroup SKE_Func_Like_Macro SKE Function Like Macro
 * @{
 */
#define SKE_START()                     WRITE_REG32(bCM_SKE->CTRL_b.START, 1U)
#define SKE_SET_LAST_BLOCK_MARK()       WRITE_REG32(bCM_SKE->DIN_CR_b.LAST, 1U)
#define SKE_RST_LAST_BLOCK_MARK()       WRITE_REG32(bCM_SKE->DIN_CR_b.LAST, 0U)
#define SKE_CFG_UPD_CMD(cmd)            WRITE_REG32(bCM_SKE->CFG_b.UP_CFG, (cmd))
/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/**
 * @defgroup SKE_Local_Variables SKE Local Variables
 * @{
 */
static const uint8_t m_au8SkeAlgBlockSize[] = {16U, 16U, 16U, 8U, 16U, 16U};
static const uint8_t m_au8SkeAlgKeySize[] = {16U, 16U, 16U, 8U, 24U, 32U};
/**
 * @}
 */

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 * @defgroup SKE_Local_Functions SKE Local Functions
 * @{
 */
/**
 * @brief  Copy data byte from pu8Src to pu8Dest.
 * @param  [out] pu8Dest                Pointer to destination data buffer.
 * @param  [in]  pu8Src                 Pointer to source data buffer.
 * @param  [in]  u32Size                Size(in byte) of the data to be copied.
 * @retval None
 */
static void SKE_CopyByte(uint8_t *pu8Dest, const uint8_t *pu8Src, uint32_t u32Size)
{
    uint32_t i;
    for (i = 0U; i < u32Size; i++) {
        pu8Dest[i] = pu8Src[i];
    }
}

/**
 * @brief  Set the value of the specified buffer byte by byte.
 * @param  [out] pu8Dest                Pointer to destination data buffer that to be set value.
 * @param  [in]  u8Value                Byte value.
 * @param  [in]  u32Size                Size(in byte) of the data to be set.
 * @retval None
 */
static void SKE_SetByte(uint8_t *pu8Dest, uint8_t u8Value, uint32_t u32Size)
{
    uint32_t i;
    for (i = 0U; i < u32Size; i++) {
        pu8Dest[i] = u8Value;
    }
}

/**
 * @brief  Compare tow buffers byte by byte.
 * @param  [in]  pu8Src1                Pointer to source data buffer1
 * @param  [in]  pu8Src2                Pointer to source data buffer2
 * @param  [in]  u32Size                Size(in byte) of the data to be compared.
 * @retval int32_t:
 *           - LL_OK:                   Two buffers are equal.
 *           - LL_ERR:                  Two buffers are not equal.
 */
static int32_t SKE_CompareByte(const uint8_t *pu8Src1, const uint8_t *pu8Src2, uint32_t u32Size)
{
    uint32_t i;

    for (i = 0U; i < u32Size; i++) {
        if (pu8Src1[i] != pu8Src2[i]) {
            return LL_ERR;
        }
    }

    return LL_OK;
}

/**
 * @brief  Reverse byte array
 * @param  [in]  pu8Src                 Pointer to the source buffer to be reversed byte
 * @param  [out] pu8Out                 Pointer to the buffer to save the source buffer after reversed.
 * @param  [in]  u32Size                Size(in byte) of the data to be reversed.
 * @retval None
 */
static void SKE_ReverseByteArray(const uint8_t *pu8Src, uint8_t *pu8Out, uint32_t u32Size)
{
    uint8_t u8Tmp;
    uint32_t u32Idx;
    uint32_t u32Round = u32Size >> 1U;

    for (u32Idx = 0; u32Idx < u32Round; u32Idx++) {
        u8Tmp = pu8Src[u32Idx];
        pu8Out[u32Idx] = pu8Src[u32Size - 1U - u32Idx];
        pu8Out[u32Size - 1U - u32Idx] = u8Tmp;
    }

    if (((u32Size & 0x1U) != 0U) && (pu8Src != pu8Out)) {
        pu8Out[u32Round] = pu8Src[u32Round];
    }
}

/**
 * @brief SKE update one block of plaintext or ciphertext.
 * @param  [in]  u32Alg                 SKE algorithm.
 *                                      This parameter can be a value of @ref SKE_Algorithm
 *   @arg  SKE_ALG_AES_128:             The algorithm is AES-128.
 *   @arg  SKE_ALG_AES_192:             The algorithm is AES-192.
 *   @arg  SKE_ALG_AES_256:             The algorithm is AES-256.
 *   @arg  SKE_ALG_SM4:                 The algorithm is SM4.
 * @param  [in]  u32Mode                SKE operation mode.
 *                                      This parameter can be a value of @ref SKE_Crypto_Mode
 *   @arg  SKE_MD_ECB:                  Electronic Code Book(ECB) mode.
 *   @arg  SKE_MD_CBC:                  Cipher Block Chaining(CBC) mode.
 *   @arg  SKE_MD_CFB:                  Cipher FeedBack(CFB) mode.
 *   @arg  SKE_MD_OFB:                  Output FeedBack(OFB) mode.
 *   @arg  SKE_MD_CTR:                  Counter(CTR) mode.
 *   @arg  SKE_MD_GCM:                  Galois/Counter Mode(GCM).
 *   @arg  SKE_MD_CCM:                  Generic authenticate-and-encrypt block cipher mode.
 * @param  [in]  pu8In                  Pointer to the plaintext byte buffer if SKE encrypting.
 *                                      Pointer to the ciphertext byte buffer if SKE decrypting.
 * @param  [out] pu8Out                 Pointer to the ciphertext byte buffer if SKE encrypting.
 *                                      Pointer to the plaintext byte buffer if SKE decrypting.
 *                                      Set it to NULL if the output data is not needed.
 * @param  [in]  u8IsLastBlock          For GCM mode and CCM mode only, ignored for other modes.
 *                                      This parameter can be:
 *                                      zero: The specified block is not the last block.
 *                                      non-zero: The specified block is the last block.
 * @retval int32_t:
 *           - LL_OK:                   The specified block updated with no error occurred.
 *           - LL_ERR_INVD_PARAM:       pu8In == NULL
 *           - LL_ERR_TIMEOUT:          SKE calculation timeout.
 */
static int32_t SKE_UpdateOneBlock(uint32_t u32Alg, uint32_t u32Mode, const uint8_t *pu8In, uint8_t *pu8Out, uint8_t u8IsLastBlock)
{
    int32_t i32Ret;

    DDL_ASSERT(IS_SKE_ALG_MD(u32Alg, u32Mode));

    if (pu8In == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    /* Check if last block of GCM/CCM/CMAC */
    if ((u32Mode == SKE_MD_GCM) || (u32Mode == SKE_MD_CCM) || (u32Mode == SKE_MD_CMAC)) {
        if (u8IsLastBlock != 0U) {
            SKE_SET_LAST_BLOCK_MARK();
        }
    }
    (void)SKE_WriteBlock(u32Alg, pu8In);
    /* SKE start */
    SKE_START();
    /* Wait till done */
    i32Ret = SKE_WaitTillDone();
    if (i32Ret != LL_OK) {
        return i32Ret;
    }

    if (pu8Out != NULL) {
        (void)SKE_ReadBlock(u32Alg, pu8Out);
    }
    if ((u32Mode == SKE_MD_GCM) || (u32Mode == SKE_MD_CCM) || (u32Mode == SKE_MD_CMAC)) {
        if (u8IsLastBlock != 0U) {
            SKE_RST_LAST_BLOCK_MARK();
            if (u32Mode == SKE_MD_CMAC) {
                /* Reset the last block size of CMAC mode */
                CLR_REG32_BIT(CM_SKE->DIN_CR, SKE_DIN_CR_LAST_LEN);
            }
        }
    }

    return LL_OK;
}

/**
 * @brief  GMAC mode initialization preparation
 * @param  [in]  pstcSkeInit            Pointer to a @ref stc_ske_init_t structure value that
 *                                      contains the configuration information for the SKE.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 * @note The IV for CMAC will be set to zero.
 */
static int32_t SKE_CmacPrepareInit(const stc_ske_init_t *pstcSkeInit)
{
    WRITE_REG32(CM_SKE->CFG, 0U);
    return LL_OK;
}

/**
 * @brief  GCM mode initialization preparation
 * @param  [in]  pstcSkeInit            Pointer to a @ref stc_ske_init_t structure value that
 *                                      contains the configuration information for the SKE.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       Parameters invalid.
 * @note The IV for GCM use au8Iv[0]~au8Iv[11], the remain bytes will be set to zero.
 */
static int32_t SKE_GcmPrepareInit(const stc_ske_init_t *pstcSkeInit)
{
    if (pstcSkeInit->pstcGcmInit == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    /* Check function parameters */
    DDL_ASSERT((pstcSkeInit->pstcGcmInit->u32AadSize | pstcSkeInit->pstcGcmInit->u32CryptoSize) != 0U);
    if ((pstcSkeInit->pstcGcmInit->u32AadSize | pstcSkeInit->pstcGcmInit->u32CryptoSize) == 0U) {
        return LL_ERR_INVD_PARAM;
    }

    /* Set AAD bit length */
    SKE_SetAadSize(pstcSkeInit->pstcGcmInit->u32AadSize);

    /* Set crypto bit length */
    SKE_SetCryptoSize(pstcSkeInit->pstcGcmInit->u32CryptoSize);

    return LL_OK;
}

/**
 * @brief  CCM mode initialization preparation
 * @param  [in]  pstcSkeInit            Pointer to a @ref stc_ske_init_t structure value that
 *                                      contains the configuration information for the SKE.
 * @param  [in]  pCtx                   Pointer to a @ref stc_ske_ccm_ctx_t structure
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       Parameters invalid.
 * @note The IV of CCM will generated from 'pstcContext->u32LengthSize' and 'pstcSkeInit->pu8Nonce'.
 */
static int32_t SKE_CcmPrepareInit(const stc_ske_init_t *pstcSkeInit, stc_ske_ccm_ctx_t *pCtx)
{
    uint32_t u32Tmp;
    uint32_t u32Len = 0U;

    if ((pCtx == NULL) || (pstcSkeInit->pstcCcmInit == NULL) || (pstcSkeInit->pstcCcmInit->pu8Nonce == NULL) || \
        ((pstcSkeInit->pstcCcmInit->u32AadSize | pstcSkeInit->pstcCcmInit->u32CryptoSize) == 0U)) {
        return LL_ERR_INVD_PARAM;
    }

    /* Check function parameters */
    DDL_ASSERT(IS_SKE_CCM_MAC_SIZE(pstcSkeInit->pstcCcmInit->u32MacSize));
    DDL_ASSERT(IS_SKE_CCM_L_SIZE(pstcSkeInit->pstcCcmInit->u32LengthSize));
    DDL_ASSERT(pstcSkeInit->pstcCcmInit->pu8Nonce != NULL);
    DDL_ASSERT((pstcSkeInit->pstcCcmInit->u32AadSize | pstcSkeInit->pstcCcmInit->u32CryptoSize) != 0U);

    /* Check u32CryptoSize */
    u32Tmp = pstcSkeInit->pstcCcmInit->u32CryptoSize;
    while (u32Tmp != 0U) {
        u32Len++;
        u32Tmp >>= 8U;
    }

    DDL_ASSERT(pstcSkeInit->pstcCcmInit->u32LengthSize >= u32Len);
    if (u32Len > pstcSkeInit->pstcCcmInit->u32LengthSize) {
        return LL_ERR_INVD_PARAM;
    }
    /* Initializes the context fields */
    pCtx->u32Alg        = pstcSkeInit->u32Alg;
    pCtx->u32MacSize    = pstcSkeInit->pstcCcmInit->u32MacSize;
    pCtx->u32LengthSize = pstcSkeInit->pstcCcmInit->u32LengthSize;

    /* A0. Caution: IV here is A0 */
    pstcSkeInit->pu8Iv[0U] = (uint8_t)(pCtx->u32LengthSize - 1U);
    SKE_CopyByte(&pstcSkeInit->pu8Iv[1U], pstcSkeInit->pstcCcmInit->pu8Nonce, 15UL - pCtx->u32LengthSize);
    SKE_SetByte(&pstcSkeInit->pu8Iv[16UL - pCtx->u32LengthSize], 0U, pCtx->u32LengthSize);

    pCtx->u32CryptoSize = pstcSkeInit->pstcCcmInit->u32CryptoSize;
    SKE_SetCryptoSize(pstcSkeInit->pstcCcmInit->u32CryptoSize);

    pCtx->u32AadSize = pstcSkeInit->pstcCcmInit->u32AadSize;
    SKE_SetAadSize(pstcSkeInit->pstcCcmInit->u32AadSize);

    return LL_OK;
}

/**
 * @brief  CCM mode get B0.
 * @param  [in]  pCtx                   Pointer to a @ref stc_ske_ccm_ctx_t structure value that
 *                                      contains the context for CCM mode.
 * @param  [in] pu8Nonce                Pointer to a nonce buffer for CCM mode.
 * @retval None
 */
static void SKE_CcmGetB0(stc_ske_ccm_ctx_t *pCtx, const uint8_t *pu8Nonce)
{
    uint8_t au8Tmp[4U];
    /**
     * B0[0]: flag
     * B0[1] ~ B0[15-u32LengthSize]: nonce
     * B0[16-u32LengthSize] ~ B0[15]: u32CryptoSize
     */
    /* Get B0 into pCtx->au8Buffer */
    /* B0 flag */
    pCtx->au8Buffer[0U] = 0U;
    pCtx->au8Buffer[0U] |= ((uint8_t)(((pCtx->u32MacSize - 2U) / 2U) << 3U));
    pCtx->au8Buffer[0U] |= ((uint8_t)(pCtx->u32LengthSize - 1U));

    if (pCtx->u32AadSize > 0U) {
        /* With AAD flag */
        pCtx->au8Buffer[0U] |= 0x40U;
    }

    /* B0 nonce */
    SKE_CopyByte(&pCtx->au8Buffer[1U], pu8Nonce, 15U - pCtx->u32LengthSize);
    SKE_SetByte(&pCtx->au8Buffer[16U - pCtx->u32LengthSize], 0U, pCtx->u32LengthSize);

    /* B0 crypto data byte length, little endian */
    SKE_ReverseByteArray((uint8_t *)(&(pCtx->u32CryptoSize)), au8Tmp, 4U);

    if (pCtx->u32LengthSize <= 4U) {
        SKE_CopyByte(&pCtx->au8Buffer[16U - pCtx->u32LengthSize], &au8Tmp[4U - pCtx->u32LengthSize], pCtx->u32LengthSize);
    } else {
        SKE_CopyByte(&pCtx->au8Buffer[12U], au8Tmp, 4U);
    }
}

/**
 * @brief  CCM mode prepare B1.
 * @param  [in]  pCtx                   Pointer to a @ref stc_ske_ccm_ctx_t structure value that
 *                                      contains the context for CCM mode.
 * @param  [in] pu8Aad                  Pointer to an AAD buffer for CCM mode.
 * @retval None
 */
static void SKE_CcmPrepareB1(stc_ske_ccm_ctx_t *pCtx, const uint8_t *pu8Aad)
{
    uint8_t au8Tmp[4U];

    /* Offset of pCtx->au8Buffer */
    uint32_t u32Offset;
    /* Remain-size of pCtx->au8Buffer */
    uint32_t u32RemainSize;

    /* Little endian */
    SKE_ReverseByteArray((uint8_t *)(&pCtx->u32AadSize), au8Tmp, 4U);

    if (pCtx->u32AadSize < 65280UL) {
        /* 65280: 2^16 - 2^8 */
        SKE_CopyByte(pCtx->au8Buffer, &au8Tmp[2U], 2U);
        u32Offset = 2U;
        /* 16-2 */
        u32RemainSize = 14U;
    } else {
        /* 65280 <= pCtx->u32AadSize < 2^32 */
        pCtx->au8Buffer[0U] = 0xFFU;
        pCtx->au8Buffer[1U] = 0xFEU;
        SKE_CopyByte(&pCtx->au8Buffer[2U], au8Tmp, 4U);
        u32Offset = 6U;
        /* 16-6 */
        u32RemainSize = 10U;
    }

    if (pCtx->u32AadSize <= u32RemainSize) {
        SKE_CopyByte(&pCtx->au8Buffer[u32Offset], pu8Aad, pCtx->u32AadSize);
        u32Offset += pCtx->u32AadSize;
        u32RemainSize = 16U - u32Offset;
        SKE_SetByte(&pCtx->au8Buffer[u32Offset], 0U, u32RemainSize);
        pCtx->u32AadRemainSize  = 0U;
        pCtx->u32AadStartOffset = 0U;
    } else {
        SKE_CopyByte(&pCtx->au8Buffer[u32Offset], pu8Aad, u32RemainSize);
        pCtx->u32AadRemainSize  = pCtx->u32AadSize - u32RemainSize;
        pCtx->u32AadStartOffset = u32RemainSize;
    }
}

/**
 * @brief  Continue to initialize CCM mode.
 * @param  [in]  pstcSkeInit            Pointer to a @ref stc_ske_init_t structure value that
 *                                      contains the configuration information for the SKE.
 * @param  [in]  pCtx                   Pointer to a @ref stc_ske_ccm_ctx_t structure value that
 *                                      contains the context for CCM mode.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_TIMEOUT:          SKE calculating timeout.
 */
static int32_t SKE_CcmContinueInit(const stc_ske_init_t *pstcSkeInit, stc_ske_ccm_ctx_t *pCtx)
{
    int32_t i32Ret;

    if ((pstcSkeInit->pstcCcmInit == NULL) || (pCtx == NULL)) {
        return LL_ERR_INVD_PARAM;
    }

    /* Get B0, store in pCtx->au8Buffer */
    SKE_CcmGetB0(pCtx, pstcSkeInit->pstcCcmInit->pu8Nonce);

    if (pstcSkeInit->pstcCcmInit->u32AadSize == 0U) {
        SKE_SET_LAST_BLOCK_MARK();
    }

    /* Update B0 */
    i32Ret = SKE_UpdateOneBlock(pstcSkeInit->u32Alg, pstcSkeInit->u32Mode, pCtx->au8Buffer, NULL, 0U);

    if (i32Ret == LL_OK) {
        if (pCtx->u32AadSize == 0U) {
            SKE_RST_LAST_BLOCK_MARK();
        }
        /* Get B1 if u32AadSize is non-zero, and store in pCtx->au8Buffer */
        if (pstcSkeInit->pstcCcmInit->u32AadSize != 0U) {
            SKE_CcmPrepareB1(pCtx, pstcSkeInit->pstcCcmInit->pu8Aad);
        }
    }

    return i32Ret;
}

/**
 * @brief  SKE internal initialization.
 * @param  [in]  pstcSkeInit            Pointer to a @ref stc_ske_init_t structure value that
 *                                      contains the configuration information for the SKE.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcSkeInit == NULL.
 *           - LL_ERR_TIMEOUT:          Expand the key timeout.
 */
static int32_t SKE_InitInternal(const stc_ske_init_t *pstcSkeInit)
{
    /* Enable update configuration */
    SKE_CFG_UPD_CMD(ENABLE);
    /* Write configuration value */
    MODIFY_REG32(CM_SKE->CFG, SKE_CFG_INIT_MASK, pstcSkeInit->u32Mode | pstcSkeInit->u32DataType | pstcSkeInit->u32Alg);
    if (pstcSkeInit->u32Mode == SKE_MD_CMAC) {
        WRITE_REG32(bCM_SKE->CFG_b.DEC, SKE_CRYPTO_ENCRYPT);
    } else {
        WRITE_REG32(bCM_SKE->CFG_b.DEC, pstcSkeInit->u32Crypto);
    }
    SKE_RST_LAST_BLOCK_MARK();
    /* Set IV according to the mode */
    if (pstcSkeInit->u32Mode != SKE_MD_ECB) {
        (void)SKE_SetIv(pstcSkeInit->u32Alg, pstcSkeInit->u32Mode, pstcSkeInit->pu8Iv);
    }

    /* Set key and expand key */
    (void)SKE_SetKey(pstcSkeInit->u32Alg, pstcSkeInit->pu8Key);
    return SKE_ExpandKey();
}

/**
 * @brief SKE crypto all blocks of plaintext or ciphertext.
 * @param  [in]  u32Alg                 SKE algorithm.
 *                                      This parameter can be a value of @ref SKE_Algorithm
 *   @arg  SKE_ALG_AES_128:             The algorithm is AES-128.
 *   @arg  SKE_ALG_AES_192:             The algorithm is AES-192.
 *   @arg  SKE_ALG_AES_256:             The algorithm is AES-256.
 *   @arg  SKE_ALG_SM4:                 The algorithm is SM4.
 * @param  [in]  u32Mode                SKE operation mode.
 *                                      This parameter can be a value of @ref SKE_Crypto_Mode
 *   @arg  SKE_MD_ECB:                  Electronic Code Book(ECB) mode.
 *   @arg  SKE_MD_CBC:                  Cipher Block Chaining(CBC) mode.
 *   @arg  SKE_MD_CFB:                  Cipher FeedBack(CFB) mode.
 *   @arg  SKE_MD_OFB:                  Output FeedBack(OFB) mode.
 *   @arg  SKE_MD_CTR:                  Counter(CTR) mode.
 *   @arg  SKE_MD_GCM:                  Galois/Counter Mode(GCM).
 *   @arg  SKE_MD_CCM:                  Generic authenticate-and-encrypt block cipher mode.
 * @param  [in]  pu8In                  Pointer to the plaintext byte buffer if SKE encrypting.
 *                                      Pointer to the ciphertext byte buffer if SKE decrypting.
 * @param  [out] pu8Out                 Pointer to the ciphertext byte buffer if SKE encrypting.
 *                                      Pointer to the plaintext byte buffer if SKE decrypting.
 *                                      Set it to NULL if the output data is not needed.
 * @param  [in]  u32CryptoSize          Size of the byte buffer that to be encrypted or decrypted.
 * @retval int32_t:
 *           - LL_OK:                   The specified blocks updated with no error occurred.
 *           - LL_ERR_INVD_PARAM:       pu8In == NULL or u32UpdateSize == 0U or invalid algorithm parameter.
 *           - LL_ERR_TIMEOUT:          SKE calculation timeout.
 * @note If u32UpdateSize % u32BlockSize != 0, the remain data bytes of the last block will be set to zero when encrypting.
 */
static int32_t SKE_CryptoBlocksInternal(uint32_t u32Alg, uint32_t u32Mode, const uint8_t *pu8In, uint8_t *pu8Out, uint32_t u32CryptoSize)
{
    uint8_t u8LastBlockFlag = 1U;
    uint32_t i;
    uint32_t u32BlockSize;
    uint32_t u32RmainSize;
    uint8_t au8LastBlock[SKE_BLOCK_SIZE_MAX] = {0U};
    int32_t i32Ret;

    if ((pu8In == NULL) || (u32CryptoSize == 0U) || (!IS_SKE_ALG_MD(u32Alg, u32Mode))) {
        return LL_ERR_INVD_PARAM;
    }
    /* CMAC mode handles last block separately in SKE_CmacAction, so mark all blocks as non-last here */
    if (u32Mode == SKE_MD_CMAC) {
        u8LastBlockFlag = 0U;
    }

    u32BlockSize = m_au8SkeAlgBlockSize[u32Alg];
    u32RmainSize = u32CryptoSize % u32BlockSize;
    if (u32RmainSize == 0U) {
        u32RmainSize = u32BlockSize;
    }
    u32CryptoSize -= u32RmainSize;
    SKE_CopyByte(au8LastBlock, &pu8In[u32CryptoSize], u32RmainSize);

    for (i = 0U; i < u32CryptoSize; i += u32BlockSize) {
        if (pu8Out == NULL) {
            i32Ret = SKE_UpdateOneBlock(u32Alg, u32Mode, &pu8In[i], NULL, 0U);
        } else {
            i32Ret = SKE_UpdateOneBlock(u32Alg, u32Mode, &pu8In[i], &pu8Out[i], 0U);
        }
        if (i32Ret != LL_OK) {
            return i32Ret;
        }
    }

    /* Crypto the last block. */
    i32Ret = SKE_UpdateOneBlock(u32Alg, u32Mode, au8LastBlock, au8LastBlock, u8LastBlockFlag);
    if ((i32Ret == LL_OK) && (pu8Out != NULL)) {
        SKE_CopyByte(&pu8Out[i], au8LastBlock, u32RmainSize);
    }

    return i32Ret;
}

/**
 * @brief GCM mode update AAD all data bytes.
 * @param  [in]  u32Alg                 SKE algorithm.
 *                                      This parameter can be a value of @ref SKE_Algorithm
 *   @arg  SKE_ALG_AES_128:             SKE algorithm is AES-128.
 *   @arg  SKE_ALG_AES_192:             SKE algorithm is AES-192.
 *   @arg  SKE_ALG_AES_256:             SKE algorithm is AES-256.
 *   @arg  SKE_ALG_SM4:                 SKE algorithm is SM4.
 * @param  [in]  pu8Aad                 Pointer to the AAD byte buffer.
 * @param  [in]  u32AadSize             Number of byte of the AAD.
 * @retval int32_t:
 *           - LL_OK:                   The AAD updated with no error occurred.
 *           - LL_ERR_BUF_EMPTY:        There is no AAD for the GCM mode.
 *           - LL_ERR_TIMEOUT:          SKE calculation timeout.
 */
static int32_t SKE_GcmUpdateAadBlocks(uint32_t u32Alg, const uint8_t *pu8Aad, uint32_t u32AadSize)
{
    int32_t i32Ret = LL_ERR_BUF_EMPTY;

    DDL_ASSERT(IS_SKE_XCM_MD_ALG(u32Alg));
    if ((u32AadSize != 0U) && (pu8Aad != NULL)) {
        i32Ret = SKE_CryptoBlocksInternal(u32Alg, SKE_MD_GCM, pu8Aad, NULL, u32AadSize);
    }
    return i32Ret;
}

/**
 * @brief CCM mode update AAD all data bytes.
 * @param  [in]  pCtx                   Pointer to a @ref stc_ske_ccm_ctx_t structure value that
 *                                      contains the context for CCM mode.
 * @param  [in]  pu8Aad                 Pointer to the AAD byte buffer.
 * @retval int32_t:
 *           - LL_OK:                   The AAD updated with no error occurred.
 *           - LL_ERR_BUF_EMPTY:        There is no AAD for the CCM mode.
 *           - LL_ERR_INVD_PARAM:       pCtx == NULL.
 *           - LL_ERR_TIMEOUT:          SKE calculation timeout.
 */
static int32_t SKE_CcmUpdateAadBlocks(stc_ske_ccm_ctx_t *pCtx, const uint8_t *pu8Aad)
{
    int32_t i32Ret;

    if (pCtx == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    if ((pu8Aad == NULL) || (pCtx->u32AadSize == 0U)) {
        /* No AAD for the CCM mode */
        return LL_ERR_BUF_EMPTY;
    }

    if (pCtx->u32AadRemainSize == 0U) {
        i32Ret = SKE_UpdateOneBlock(pCtx->u32Alg, SKE_MD_CCM, pCtx->au8Buffer, NULL, 1U);
    } else {
        i32Ret = SKE_UpdateOneBlock(pCtx->u32Alg, SKE_MD_CCM, pCtx->au8Buffer, NULL, 0U);
        if (i32Ret == LL_OK) {
            i32Ret = SKE_CryptoBlocksInternal(pCtx->u32Alg, SKE_MD_CCM, &pu8Aad[pCtx->u32AadStartOffset], \
                                              NULL, pCtx->u32AadRemainSize);
        }
    }

    return i32Ret;
}
/**
 * @}
 */

/**
 * @defgroup SKE_Global_Functions SKE Global Functions
 * @{
 */
/**
 * @brief  Initializes the SKE peripheral according to the specified parameters
 *         in the structure pstcSkeInit.
 * @param  [in]  pstcSkeInit            Pointer to a @ref stc_ske_init_t structure value that
 *                                      contains the configuration information for the SKE.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       Parameters invalid.
 *           - LL_ERR_TIMEOUT:          Expand the key timeout.
 */
int32_t SKE_Init(const stc_ske_init_t *pstcSkeInit)
{
    int32_t i32Ret = LL_OK;
    __IO uint32_t u32TimeCount = 0UL;
    stc_ske_ccm_ctx_t stcCcmCtx;

    if ((pstcSkeInit == NULL) || (pstcSkeInit->pu8Key == NULL) || \
        ((pstcSkeInit->u32Mode != SKE_MD_ECB) && (pstcSkeInit->u32Mode != SKE_MD_CMAC) && (pstcSkeInit->pu8Iv == NULL))) {
        return LL_ERR_INVD_PARAM;
    }

    /* Check function parameters */
    DDL_ASSERT(IS_SKE_ALG_MD(pstcSkeInit->u32Alg, pstcSkeInit->u32Mode));
    DDL_ASSERT(IS_SKE_DATA_TYPE(pstcSkeInit->u32DataType));
    DDL_ASSERT(IS_SKE_CRYPTO(pstcSkeInit->u32Crypto));

    while (READ_REG32(bCM_SKE->SR1_b.BUSY) != 0U) {
        u32TimeCount++;
        if (u32TimeCount > SKE_TIMEOUT_VAL) {
            /* SKE is busy, initialization failed. */
            return LL_ERR;
        }
    }

    /* Clear SKE done flag. */
    WRITE_REG32(bCM_SKE->SR2_b.CORE_DONE, 0U);
    /* Preparation for initialization of GCM/CCM/CMAC modes */
    if (pstcSkeInit->u32Mode == SKE_MD_GCM) {
        i32Ret = SKE_GcmPrepareInit(pstcSkeInit);
    } else if (pstcSkeInit->u32Mode == SKE_MD_CCM) {
        i32Ret = SKE_CcmPrepareInit(pstcSkeInit, &stcCcmCtx);
    } else if (pstcSkeInit->u32Mode == SKE_MD_CMAC) {
        i32Ret = SKE_CmacPrepareInit(pstcSkeInit);
    } else {
        /* rsvd */
    }
    if (i32Ret != LL_OK) {
        return i32Ret;
    }

    i32Ret = SKE_InitInternal(pstcSkeInit);
    if (i32Ret == LL_OK) {
        if (pstcSkeInit->u32Mode == SKE_MD_GCM) {
            if ((pstcSkeInit->pstcGcmInit->pu8Aad != NULL) && (pstcSkeInit->pstcGcmInit->u32AadSize > 0U)) {
                i32Ret = SKE_GcmUpdateAadBlocks(pstcSkeInit->u32Alg, pstcSkeInit->pstcGcmInit->pu8Aad, pstcSkeInit->pstcGcmInit->u32AadSize);
            }
        } else if (pstcSkeInit->u32Mode == SKE_MD_CCM) {
            i32Ret = SKE_CcmContinueInit(pstcSkeInit, &stcCcmCtx);
            if ((i32Ret == LL_OK) && (pstcSkeInit->pstcCcmInit->u32AadSize > 0U)) {
                i32Ret = SKE_CcmUpdateAadBlocks(&stcCcmCtx, pstcSkeInit->pstcCcmInit->pu8Aad);
            }
        } else {
        }
    }

    return i32Ret;
}

/**
 * @brief  Set each member of @ref stc_ske_init_t to a default value.
 * @param  [in]  pstcSkeInit            Pointer to a @ref stc_ske_init_t structure
 *                                      whose members will be set to default values.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcSkeInit == NULL.
 */
int32_t SKE_StructInit(stc_ske_init_t *pstcSkeInit)
{
    if (pstcSkeInit  == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    pstcSkeInit->u32Alg       = SKE_ALG_AES_128;
    pstcSkeInit->u32Mode      = SKE_MD_ECB;
    pstcSkeInit->u32DataType  = SKE_DATA_SWAP_NON;
    pstcSkeInit->u32Crypto    = SKE_CRYPTO_ENCRYPT;
    pstcSkeInit->pu8Key       = NULL;
    pstcSkeInit->pu8Iv        = NULL;
    pstcSkeInit->pstcGcmInit  = NULL;
    pstcSkeInit->pstcCcmInit  = NULL;

    return LL_OK;
}

/**
 * @brief Deinitialize the SKE peripheral registers to their default reset values.
 * @param  None
 * @retval int32_t:
 *           - LL_OK:                   De-Initialize success.
 *           - LL_ERR_TIMEOUT:          Timeout.
 */
int32_t SKE_DeInit(void)
{
    __IO uint8_t u8TimeOut = 0U;

    /* Check FRST register protect */
    DDL_ASSERT((CM_PWC->FPRC & PWC_FPRC_FPRCB1) == PWC_FPRC_FPRCB1);

    /* Reset CANx */
    WRITE_REG32(bCM_RMU->FRST0_b.SKE, 0UL);

    /* Ensure reset procedure is completed */
    while (READ_REG32(bCM_RMU->FRST0_b.SKE) != 1UL) {
        u8TimeOut++;
        if (u8TimeOut > SKE_RMU_TIMEOUT) {
            return LL_ERR_TIMEOUT;
        }
    }

    return LL_OK;
}

/**
 * @brief Enable or disable update SKE configuration.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 *   @arg  ENABLE:                      Enable update SKE configuration.
 *                                      Update the configuration, SKE uses the new configuration for calculation.
 *   @arg  DISABLE:                     Disable update SKE configuration.
 *                                      Do not update the configuration, SKE uses the last configuration for calculation.
 * @retval None
 */
void SKE_UpdateConfigCmd(en_functional_state_t enNewState)
{
    /* Check function parameters */
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    SKE_CFG_UPD_CMD(enNewState);
}

/**
 * @brief Set SKE data type
 * @param  [in]  u32DataType            SKE data type.
 *                                      This parameter can be a value of @ref SKE_Data_Type
 *   @arg  SKE_DATA_SWAP_NON:           No swap.
 *   @arg  SKE_DATA_SWAP_HALF_WORD:     Half word swap.
 *   @arg  SKE_DATA_SWAP_BYTE:          Byte swap.
 *   @arg  SKE_DATA_SWAP_BIT:           Bit swap.
 * @retval None
 */
void SKE_SetDataType(uint32_t u32DataType)
{
    /* Check function parameters */
    DDL_ASSERT(IS_SKE_DATA_TYPE(u32DataType));
    MODIFY_REG32(CM_SKE->CFG, SKE_CFG_DATA_TYPE, u32DataType);
}

/**
 * @brief Set SKE encrypting or decrypting.
 * @param  [in]  u32Crypto              SKE encrypting or decrypting.
 *                                      This parameter can be a value of @ref SKE_Crypto_Action
 *   @arg  SKE_CRYPTO_ENCRYPT:          SKE encrypting.
 *   @arg  SKE_CRYPTO_DECRYPT:          SKE decrypting.
 * @retval None
 */
void SKE_SetCrypto(uint32_t u32Crypto)
{
    /* Check function parameters */
    DDL_ASSERT(IS_SKE_CRYPTO(u32Crypto));
    WRITE_REG32(bCM_SKE->CFG_b.DEC, u32Crypto);
}

/**
 * @brief Set SKE Algorithm.
 * @param  [in]  u32Alg                 SKE algorithm.
 *   @arg  SKE_ALG_AES_128:             Select AES-128 as SKE algorithm.
 *   @arg  SKE_ALG_AES_192:             Select AES-192 as SKE algorithm.
 *   @arg  SKE_ALG_AES_256:             Select AES-256 as SKE algorithm.
 *   @arg  SKE_ALG_SM4:                 Select SM4 as SKE algorithm.
 * @retval None
 */
void SKE_SetAlgorithm(uint32_t u32Alg)
{
    /* Check function parameters */
    DDL_ASSERT(IS_SKE_ALG(u32Alg));
    MODIFY_REG32(CM_SKE->CFG, SKE_CFG_ALG, u32Alg);
}

/**
 * @brief Set SKE operation mode
 * @param  [in]  u32Mode                SKE operation mode.
 *                                      This parameter can be a value of @ref SKE_Crypto_Mode
 *   @arg  SKE_MD_ECB:                  Electronic Code Book(ECB) mode.
 *   @arg  SKE_MD_CBC:                  Cipher Block Chaining(CBC) mode.
 *   @arg  SKE_MD_CFB:                  Cipher FeedBack(CFB) mode.
 *   @arg  SKE_MD_OFB:                  Output FeedBack(OFB) mode.
 *   @arg  SKE_MD_CTR:                  Counter(CTR) mode.
 *   @arg  SKE_MD_GCM:                  Galois/Counter Mode(GCM).
 *   @arg  SKE_MD_CCM:                  Generic authenticate-and-encrypt block cipher mode.
 * @retval None
 */
void SKE_SetMode(uint32_t u32Mode)
{
    /* Check function parameters */
    DDL_ASSERT(IS_SKE_MD(u32Mode));
    /* Update operation mode */
    MODIFY_REG32(CM_SKE->CFG, SKE_CFG_MODE, u32Mode);
}

/**
 * @brief Set the mark of the last block.
 * @param  None
 * @retval None
 */
void SKE_SetLastBlockMark(void)
{
    SKE_SET_LAST_BLOCK_MARK();
}

/**
 * @brief Reset the mark of the last block.
 * @param  None
 * @retval None
 */
void SKE_ResetLastBlockMark(void)
{
    SKE_RST_LAST_BLOCK_MARK();
}

/**
 * @brief Set CMAC mode last block size in byte
 * @param  [in]  u32Size                Size of the last block of CMAC mode in byte.
 * @retval None
 * @note Call this function after algorithm configured.
 */
void SKE_SetCmacLastBlockSize(uint32_t u32Size)
{
    DDL_ASSERT(u32Size <= SKE_BLOCK_SIZE_16BYTE);
    WRITE_REG32(CM_SKE->DIN_CR, u32Size << 3U);
}

/**
 * @brief Reset CMAC mode last block size
 * @param  None
 * @retval None
 */
void SKE_ResetCmacLastBlockSize(void)
{
    CLR_REG32_BIT(CM_SKE->DIN_CR, SKE_DIN_CR_LAST_LEN);
}

/**
 * @brief  Set SKE key.
 * @param  [in]  u32Alg                 SKE algorithm.
 *                                      This parameter can be a value of @ref SKE_Algorithm
 *   @arg  SKE_ALG_AES_128:             SKE algorithm is AES-128.
 *   @arg  SKE_ALG_AES_192:             SKE algorithm is AES-192.
 *   @arg  SKE_ALG_AES_256:             SKE algorithm is AES-256.
 *   @arg  SKE_ALG_SM4:                 SKE algorithm is SM4.
 * @param  [in]  pu8Key                 Pointer to the key buffer.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pu8Key == NULL or invalid algorithm parameter.
 * @note The input key is only valid when CFG.UP_CFG is 1.
 */
int32_t SKE_SetKey(uint32_t u32Alg, const uint8_t *pu8Key)
{
    uint8_t i;
    uint8_t u8WordSize;
    __IO uint32_t *regKR = &CM_SKE->KEY1;
    const uint32_t *pu32Key;

    if ((pu8Key == NULL) || (!IS_SKE_ALG(u32Alg))) {
        return LL_ERR_INVD_PARAM;
    }

    DDL_ASSERT(((uint32_t)pu8Key & 0x3U) == 0U);
    pu32Key = (const uint32_t *)((uint32_t)pu8Key);

    u8WordSize = m_au8SkeAlgKeySize[u32Alg] / 4U;
    /* Write KEY registers */
    for (i = 0U; i < u8WordSize; i++) {
        regKR[i] = pu32Key[i];
    }

    return LL_OK;
}

/**
 * @brief  SKE expand key.
 * @param  None
 * @retval int32_t:
 *           - LL_OK:                   SKE calculating done successfully.
 *           - LL_ERR_TIMEOUT:          SKE calculating timeout.
 * @note Call after SKE_SetCrypto() and SKE_SetAlgorithm(), and the key is already set.
 */
int32_t SKE_ExpandKey(void)
{
    int32_t i32Ret;

    /* Enable configuration update */
    SKE_CFG_UPD_CMD(ENABLE);
    /* Expand key */
    SKE_START();

    /* Wait operation done */
    i32Ret = SKE_WaitTillDone();
    if (i32Ret == LL_OK) {
        /* Disable configuration update */
        SKE_CFG_UPD_CMD(DISABLE);
    }

    return i32Ret;
}

/**
 * @brief  Set SKE IV.
 * @param  [in]  u32Alg                 SKE algorithm.
 *                                      This parameter can be a value of @ref SKE_Algorithm
 *   @arg  SKE_ALG_AES_128:             SKE algorithm is AES-128.
 *   @arg  SKE_ALG_AES_192:             SKE algorithm is AES-192.
 *   @arg  SKE_ALG_AES_256:             SKE algorithm is AES-256.
 *   @arg  SKE_ALG_SM4:                 SKE algorithm is SM4.
 * @param  [in]  u32Mode                SKE crypto mode.
 *                                      This parameter can be a value of @ref SKE_Crypto_Mode except SKE_MD_ECB
 *   @arg  SKE_MD_CBC:                  Cipher Block Chaining(CBC) mode.
 *   @arg  SKE_MD_CFB:                  Cipher FeedBack(CFB) mode, CFB-128 only.
 *   @arg  SKE_MD_OFB:                  Output FeedBack(OFB) mode.
 *   @arg  SKE_MD_CTR:                  Counter(CTR) mode.
 *   @arg  SKE_MD_GCM:                  Galois/Counter Mode(GCM).
 *   @arg  SKE_MD_CCM:                  Generic authenticate-and-encrypt block cipher mode.
 * @param  [in]  pu8Iv                  Pointer to the IV byte buffer.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pu8Iv == NULL or invalid algorithm parameter.
 */
int32_t SKE_SetIv(uint32_t u32Alg, uint32_t u32Mode, const uint8_t *pu8Iv)
{
    uint8_t i;
    uint32_t u32IvSize;
    uint32_t au32Iv[4U] = {0UL, 0UL, 0UL, 0UL};
    __IO uint32_t *regIV = &CM_SKE->IV1;

    if ((pu8Iv == NULL) || (!IS_SKE_ALG_MD(u32Alg, u32Mode))) {
        return LL_ERR_INVD_PARAM;
    }

    /* Check function parameters */
    DDL_ASSERT(IS_SKE_IV_MD(u32Mode));

    if (u32Mode == SKE_MD_GCM) {
        u32IvSize = SKE_IV_SIZE_12BYTE;
    } else {
        u32IvSize = m_au8SkeAlgBlockSize[u32Alg];
    }

    SKE_CopyByte((uint8_t *)au32Iv, pu8Iv, u32IvSize);

    /* Write IV registers */
    for (i = 0U; i < 4U; i++) {
        regIV[i] = au32Iv[i];
    }

    return LL_OK;
}

/**
 * @brief  Set SKE all IV registers to zero.
 * @param  None
 * @retval None
 */
void SKE_ResetIv(void)
{
    CLR_REG32(CM_SKE->IV1);
    CLR_REG32(CM_SKE->IV2);
    CLR_REG32(CM_SKE->IV3);
    CLR_REG32(CM_SKE->IV4);
}

/**
 * @brief  Set SKE AAD size.
 * @param  [in] u32AadSize              Number of byte of the AAD.
 * @retval None
 * @note Just for CCM and GCM mode.
 */
void SKE_SetAadSize(uint32_t u32AadSize)
{
    WRITE_REG32(CM_SKE->AAD1, (u32AadSize << 3U) & 0xFFFFFFF8UL);
    WRITE_REG32(CM_SKE->AAD2, (u32AadSize >> 29U) & 0x7UL);
}

/**
 * @brief  Set SKE crypto size.
 * @param  [in] u32CryptoSize           Number of byte of the crypto data.
 * @retval None
 * @note Just for CCM and GCM mode.
 */
void SKE_SetCryptoSize(uint32_t u32CryptoSize)
{
    WRITE_REG32(CM_SKE->CLEN1, (u32CryptoSize << 3U) & 0xFFFFFFF8UL);
    WRITE_REG32(CM_SKE->CLEN2, (u32CryptoSize >> 29U) & 0x7UL);
}

/**
 * @brief  Get block size of the specified algorithm
 * @param  [in]  u32Alg                 SKE algorithm.
 *                                      This parameter can be a value of @ref SKE_Algorithm
 *   @arg  SKE_ALG_AES_128:             SKE algorithm is AES-128.
 *   @arg  SKE_ALG_AES_192:             SKE algorithm is AES-192.
 *   @arg  SKE_ALG_AES_256:             SKE algorithm is AES-256.
 *   @arg  SKE_ALG_SM4:                 SKE algorithm is SM4.
 * @retval An uint8_t type value of block size. Return 0 means invalid algorithm parameter.
 */
uint8_t SKE_GetBlockSize(uint32_t u32Alg)
{
    if (IS_SKE_ALG(u32Alg)) {
        return m_au8SkeAlgBlockSize[u32Alg];
    } else {
        return 0U;
    }
}

/**
 * @brief  Input one SKE block.
 * @param  [in]  pu8In                  Pointer to the plaintext or ciphertext in byte buffer.
 * @param  [in]  u32Alg                 SKE algorithm.
 *                                      This parameter can be a value of @ref SKE_Algorithm
 *   @arg  SKE_ALG_AES_128:             SKE algorithm is AES-128.
 *   @arg  SKE_ALG_AES_192:             SKE algorithm is AES-192.
 *   @arg  SKE_ALG_AES_256:             SKE algorithm is AES-256.
 *   @arg  SKE_ALG_SM4:                 SKE algorithm is SM4.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pu8In == NULL or invalid algorithm parameter.
 */
int32_t SKE_WriteBlock(uint32_t u32Alg, const uint8_t *pu8In)
{
    const uint32_t *pu32In;
    __IO uint32_t *regDIN  = &CM_SKE->DIN1;

    if ((pu8In == NULL) || (!IS_SKE_ALG(u32Alg))) {
        return LL_ERR_INVD_PARAM;
    }

    DDL_ASSERT(((uint32_t)pu8In & 0x3U) == 0U);
    pu32In = (const uint32_t *)((uint32_t)pu8In);

    regDIN[0U] = pu32In[0U];
    regDIN[1U] = pu32In[1U];
    if (m_au8SkeAlgBlockSize[u32Alg] == SKE_BLOCK_SIZE_16BYTE) {
        /* For AES/SM4 */
        regDIN[2U] = pu32In[2U];
        regDIN[3U] = pu32In[3U];
    }

    return LL_OK;
}

/**
 * @brief  Read one SKE block.
 * @param  [out] pu8Out                 One block output of SKE in byte buffer
 * @param  [in]  u32Alg                 SKE algorithm.
 *                                      This parameter can be a value of @ref SKE_Algorithm
 *   @arg  SKE_ALG_AES_128:             SKE algorithm is AES-128.
 *   @arg  SKE_ALG_AES_192:             SKE algorithm is AES-192.
 *   @arg  SKE_ALG_AES_256:             SKE algorithm is AES-256.
 *   @arg  SKE_ALG_SM4:                 SKE algorithm is SM4.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pu8Out == NULL or invalid algorithm parameter.
 */
int32_t SKE_ReadBlock(uint32_t u32Alg, uint8_t *pu8Out)
{
    uint32_t *pu32Out;
    __I uint32_t *regDOUT = &CM_SKE->DOUT1;

    if ((pu8Out == NULL) || (!IS_SKE_ALG(u32Alg))) {
        return LL_ERR_INVD_PARAM;
    }

    DDL_ASSERT(((uint32_t)pu8Out & 0x3U) == 0U);
    pu32Out = (uint32_t *)((uint32_t)pu8Out);

    pu32Out[0U] = regDOUT[0U];
    pu32Out[1U] = regDOUT[1U];
    if (m_au8SkeAlgBlockSize[u32Alg] == SKE_BLOCK_SIZE_16BYTE) {
        /* For AES/SM4 */
        pu32Out[2U] = regDOUT[2U];
        pu32Out[3U] = regDOUT[3U];
    }

    return LL_OK;
}

/**
 * @brief  Read one middle IV block.
 * @param  [out] pu8Out                 One block output of middle IV in byte buffer
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pu8Out == NULL
 */
int32_t SKE_ReadMidIv(uint8_t *pu8Out)
{
    uint32_t *pu32Out;
    __I uint32_t *regDOUT = &CM_SKE->MID_IV1;

    if (pu8Out == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    DDL_ASSERT(((uint32_t)pu8Out & 0x3U) == 0U);
    pu32Out = (uint32_t *)((uint32_t)pu8Out);

    pu32Out[0U] = regDOUT[0U];
    pu32Out[1U] = regDOUT[1U];
    pu32Out[2U] = regDOUT[2U];
    pu32Out[3U] = regDOUT[3U];

    return LL_OK;
}

/**
 * @brief Enable or disable SKE interrupt.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 *   @arg  ENABLE:                      Enable SKE interrupt.
 *   @arg  DISABLE:                     Disable SKE interrupt.
 * @retval None
 */
void SKE_IntCmd(en_functional_state_t enNewState)
{
    /* Check function parameters */
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    WRITE_REG32(bCM_SKE->CFG_b.IRQEN, enNewState);
}

/**
 * @brief Start SKE. When CFG.UP_CFG is 1, start update SKE configuration.
 *        When CFG.UP_CFG is 0, start SKE calculation.
 * @param  None
 * @retval None
 */
void SKE_Start(void)
{
    SKE_START();
}

/**
 * @brief  Wait till SKE calculating done
 * @param  None
 * @retval int32_t:
 *           - LL_OK:                   SKE calculating done successfully.
 *           - LL_ERR_TIMEOUT:          SKE calculating timeout.
 */
int32_t SKE_WaitTillDone(void)
{
    uint32_t u32Busy;
    uint32_t u32Done;
    __IO uint32_t u32TimeCount = 0UL;

    for (;;) {
        u32Busy = READ_REG32(bCM_SKE->SR1_b.BUSY);
        u32Done = READ_REG32(bCM_SKE->SR2_b.CORE_DONE);
        if ((u32Done != 0U) && (u32Busy == 0U)) {
            WRITE_REG32(bCM_SKE->SR2_b.CORE_DONE, 0U);
            break;
        }
        u32TimeCount++;
        if (u32TimeCount > SKE_TIMEOUT_VAL) {
            return LL_ERR_TIMEOUT;
        }
    }

    return LL_OK;
}

/**
 * @brief  Get the status of the specified SKE flag.
 * @param  [in]  u32Flag                SKE status flag.
 *                                      This parameter can be a value of @ref SKE_Status_Flag
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t SKE_GetStatus(uint32_t u32Flag)
{
    en_flag_status_t enStatus = RESET;

    DDL_ASSERT(IS_SKE_FLAG(u32Flag));

    /* If one of the specified flags is set, return SET. */
    /* Check SR1 */
    if (READ_REG32_BIT(CM_SKE->SR1, SKE_SR1_FLAG) != 0U) {
        enStatus = SET;
    }
    /* Check SR2 */
    if ((u32Flag & SKE_FLAG_DONE) != 0U) {
        if (READ_REG32(bCM_SKE->SR2_b.CORE_DONE) != 0U) {
            enStatus = SET;
        }
    }

    return enStatus;
}

/**
 * @brief  Clear the status of the specified SKE flag.
 * @param  [in]  u32Flag                SKE status flag.
 *                                      This parameter can be values of @ref SKE_Status_Flag
 *   @arg  SKE_FLAG_MID_VALID:          The middle value is valid.
 *   @arg  SKE_FLAG_DONE:               SKE calculation done.
 * @retval None
 */
void SKE_ClearStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_SKE_FLAG_CLR(u32Flag));

    /* Clear SR1 */
    if ((u32Flag & SKE_FLAG_MID_VALID) != 0U) {
        WRITE_REG32(bCM_SKE->SR1_b.MID_O_VALID, 0U);
    }

    /* Clear SR2 */
    if ((u32Flag & SKE_FLAG_DONE) != 0U) {
        WRITE_REG32(CM_SKE->SR2, 0U);
    }
}

/**
 * @brief SKE crypto all blocks of plaintext or ciphertext.
 * @param  [in]  pstcCrypto             Pointer to a @ref stc_ske_crypto_t structure value that
 *                                      contains the information for crypto.
 * @retval int32_t:
 *           - LL_OK:                   The specified blocks updated with no error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcCrypto == NULL or pstcCrypto->pu8In == NULL or pstcCrypto->u32CryptoSize == 0U
 *           - LL_ERR_TIMEOUT:          SKE calculation timeout.
 * @note If pstcCrypto->u32CryptoSize % u32BlockSize != 0, the remain data bytes of the last block will be set to zero when encrypting.
 */
int32_t SKE_CryptoBlocks(stc_ske_crypto_t *pstcCrypto)
{
    if (pstcCrypto == NULL) {
        return LL_ERR_INVD_PARAM;
    }
    return SKE_CryptoBlocksInternal(pstcCrypto->u32Alg, pstcCrypto->u32Mode, \
                                    pstcCrypto->pu8In, pstcCrypto->pu8Out, \
                                    pstcCrypto->u32CryptoSize);
}

/**
 * @brief Get or authenticate the MAC for CMAC mode.
 * @param  [in]  pstcAction             Pointer to a @ref stc_ske_cmac_action_t structure value that
 *                                      contains the information for generate or authenticate the MAC.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcAction == NULL or pstcAction->pu8Mac == NULL or pstcAction->u32MacSize == 0U or invalid algorithm parameter.
 *           - LL_ERR_TIMEOUT:          SKE calculation timeout.
 */
int32_t SKE_CmacAction(stc_ske_cmac_action_t *pstcAction)
{
    uint32_t u32MsgSize;
    uint32_t u32BlockSize;
    uint32_t u32RmainSize = 0U;
    int32_t i32Ret = LL_OK;
    __IO uint32_t u32TimeCount = 0U;
    uint8_t au8Buffer[SKE_BLOCK_SIZE_MAX] = {0U};

    if ((pstcAction == NULL) || (pstcAction->pu8Mac == NULL) || \
        (pstcAction->u32MacSize == 0U) || (!IS_SKE_ALG(pstcAction->u32Alg))) {
        return LL_ERR_INVD_PARAM;
    }

    u32MsgSize   = pstcAction->u32MsgSize;
    u32BlockSize = m_au8SkeAlgBlockSize[pstcAction->u32Alg];

    if ((pstcAction->pu8Msg != NULL) && (u32MsgSize > 0U)) {
        u32RmainSize = u32MsgSize % u32BlockSize;
        if (u32RmainSize == 0U) {
            u32RmainSize = u32BlockSize;
        }
        u32MsgSize -= u32RmainSize;
        if (u32MsgSize > 0U) {
            i32Ret = SKE_CryptoBlocksInternal(pstcAction->u32Alg, SKE_MD_CMAC, pstcAction->pu8Msg, NULL, u32MsgSize);
        }
        SKE_CopyByte(au8Buffer, &pstcAction->pu8Msg[u32MsgSize], u32RmainSize);
    }

    if (i32Ret == LL_OK) {
        /* Set the bit size of the last block */
        WRITE_REG32(CM_SKE->DIN_CR, u32RmainSize << 3U);
        /* Update the last block */
        i32Ret = SKE_UpdateOneBlock(pstcAction->u32Alg, SKE_MD_CMAC, au8Buffer, NULL, 1U);
    }

    if (i32Ret == LL_OK) {
        for (;;) {
            if (READ_REG32(bCM_SKE->SR1_b.MID_O_VALID) == 1U) {
                WRITE_REG32(bCM_SKE->SR1_b.MID_O_VALID, 0U);
                (void)SKE_ReadMidIv(au8Buffer);
                if (pstcAction->u32Action == SKE_CMAC_GENERATE) {
                    /* Get the MAC */
                    SKE_CopyByte(pstcAction->pu8Mac, au8Buffer, pstcAction->u32MacSize);
                } else {
                    /* Verify the MAC */
                    i32Ret = SKE_CompareByte(pstcAction->pu8Mac, au8Buffer, pstcAction->u32MacSize);
                }
                break;
            }
            u32TimeCount++;
            if (u32TimeCount > SKE_TIMEOUT_VAL) {
                i32Ret = LL_ERR_TIMEOUT;
                break;
            }
        }
    }

    return i32Ret;
}

/**
 * @brief The final calculation for GCM mode and CCM mode: get or authenticate the MAC for GCM mode and CCM mode.
 * @param  [in]  pstcFinal              Pointer to a @ref stc_ske_xcm_final_t structure value that
 *                                      contains the final calculation information for GCM mode or CCM mode.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcFinal == NULL or pstcFinal->pu8Mac > SKE_MAC_SIZE_MAX;
 *           - LL_ERR_TIMEOUT:          SKE calculation timeout.
 *           - LL_ERR:                  MAC authentication failure.
 */
int32_t SKE_XcmFinal(stc_ske_xcm_final_t *pstcFinal)
{
    int32_t i32Ret;
    uint8_t au8Mac[SKE_BLOCK_SIZE_MAX];

    if (pstcFinal == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    DDL_ASSERT(IS_SKE_XCM_MD_ALG(pstcFinal->u32Alg));
    DDL_ASSERT(IS_SKE_XCM_MD(pstcFinal->u32Mode));
    DDL_ASSERT(IS_SKE_CRYPTO(pstcFinal->u32Crypto));
    DDL_ASSERT(IS_SKE_XCM_MAC_SIZE(pstcFinal->u32Mode, pstcFinal->u32MacSize));

    SKE_START();
    i32Ret = SKE_WaitTillDone();
    if (i32Ret == LL_OK) {
        if ((pstcFinal->pu8Mac != NULL) && (pstcFinal->u32MacSize != 0U)) {
            (void)SKE_ReadBlock(pstcFinal->u32Alg, au8Mac);
            if (pstcFinal->u32Crypto == SKE_CRYPTO_ENCRYPT) {
                SKE_CopyByte(pstcFinal->pu8Mac, au8Mac, pstcFinal->u32MacSize);
            } else if (pstcFinal->u32Crypto == SKE_CRYPTO_DECRYPT) {
                i32Ret = SKE_CompareByte(au8Mac, pstcFinal->pu8Mac, pstcFinal->u32MacSize);
            } else {
                /* rsvd */
            }
        }
    }

    return i32Ret;
}

/**
 * @}
 */

#endif /* LL_SKE_ENABLE */

/**
 * @}
 */

/**
 * @}
 */
/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

