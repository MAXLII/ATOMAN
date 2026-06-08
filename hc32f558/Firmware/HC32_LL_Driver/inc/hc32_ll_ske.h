/**
 *******************************************************************************
 * @file  hc32_ll_ske.h
 * @brief This file contains all the functions prototypes of the SKE driver
 *        library.
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
#ifndef __HC32_LL_SKE_H__
#define __HC32_LL_SKE_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_def.h"

#include "hc32f5xx.h"
#include "hc32f5xx_conf.h"
/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @addtogroup LL_SKE
 * @{
 */

#if (LL_SKE_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup SKE_Global_Types SKE Global Types
 * @{
 */
/**
 * @brief SKE GCM mode initialization parameters.
 */
typedef struct {
    uint32_t u32CryptoSize;                 /*!< Number of byte of plaintext or ciphertext.
                                                 This parameter could be any value, including 0.
                                                 NOTE: u32CryptoSize and u32AadSize could not be zero at the same time due to hardware implementation. */
    uint32_t u32AadSize;                    /*!< Number of byte of AAD(Additional Authenticated Data).
                                                 This parameter could be any value, including 0.
                                                 NOTE: u32CryptoSize and u32AadSize could not be zero at the same time due to hardware implementation. */
    const uint8_t *pu8Aad;                  /*!< Pointer to AAD buffer. Set it to NULL if AAD is not needed. */
} stc_ske_gcm_init_para_t;

/**
 * @brief SKE CCM mode initialization parameters.
 */
typedef struct {
    uint32_t u32CryptoSize;                 /*!< Number of byte of plaintext or ciphertext.
                                                 This parameter could be any value, including 0.
                                                 NOTE: u32CryptoSize and u32AadSize could not be zero at the same time due to hardware implementation. */
    uint32_t u32AadSize;                    /*!< Number of byte of AAD(Additional Authenticated Data).
                                                 This parameter could be any value, including 0.
                                                 NOTE: u32CryptoSize and u32AadSize could not be zero at the same time due to hardware implementation. */
    uint32_t u32MacSize;                    /*!< Number of byte of MAC(Message Authentication Code).
                                                 This parameter must be a value of {4, 6, 8, 10, 12, 14, 16} */
    uint32_t u32LengthSize;                 /*!< Number of byte of length field.
                                                 This parameter must be greater than or equal to the size of the value of u32CryptoSize.
                                                 This parameter must be a value between 2 and 8. */
    const uint8_t *pu8Nonce;                /*!< Pointer to a nonce buffer which size(15-u32LengthSize) is between 7 and 13. */
    const uint8_t *pu8Aad;                  /*!< Pointer to AAD buffer. Set it to NULL if AAD is not needed. */
} stc_ske_ccm_init_para_t;

/**
 * @brief SKE crypto structure for all modes except CMAC
 */
typedef struct {
    uint32_t u32Alg;                        /*!< SKE algorithm.
                                                 This parameter can be a value of @ref SKE_Algorithm */
    uint32_t u32Mode;                       /*!< SKE operation mode.
                                                 This parameter can be a value of @ref SKE_Crypto_Mode but CMAC */
    const uint8_t *pu8In;                   /*!< Pointer to the plaintext byte buffer if SKE encrypting.
                                                 Pointer to the ciphertext byte buffer if SKE decrypting. */
    uint8_t *pu8Out;                        /*!< Pointer to the ciphertext byte buffer if SKE encrypting.
                                                 Pointer to the plaintext byte buffer if SKE decrypting. */
    uint32_t u32CryptoSize;                 /*!< Size of the byte buffer that to be encrypted or decrypted */
} stc_ske_crypto_t;

/**
 * @brief SKE CMAC mode action structure
 */
typedef struct {

    uint32_t u32Alg;                        /*!< SKE algorithm.
                                                 This parameter can be a value of @ref SKE_Algorithm */
    uint32_t u32Action;                     /*!< The CMAC mode action.
                                                 This parameter can be a value of @ref SKE_CMAC_Action */
    const uint8_t *pu8Msg;                  /*!< Pointer to the message byte buffer. */
    uint32_t u32MsgSize;                    /*!< Size of message buffer in bytes */
    uint8_t *pu8Mac;                        /*!< Pointer to the MAC byte buffer.
                                                 When u32Action is SKE_CMAC_GENERATE: pointer to the MAC byte buffer to store the MAC.
                                                 When u32Action is SKE_CMAC_VERIFY: pointer to the MAC byte buffer that to be authenticated. */
    uint32_t u32MacSize;                    /*!< Size of MAC buffer in bytes */
} stc_ske_cmac_action_t;

/**
 * @brief SKE GCM mode and CCM mode final calculation structure.
 */
typedef struct {
    uint32_t u32Alg;                        /*!< SKE algorithm: AES-128, AES-192, AES-256, SM4. */
    uint32_t u32Mode;                       /*!< SKE mode: SKE_MD_GCM, SKE_MD_CCM. */
    uint32_t u32Crypto;                     /*!< SKE crypto action.
                                                 This parameter can be a value of @ref SKE_Crypto_Action */
    uint8_t *pu8Mac;                        /*!< Pointer to MAC buffer of CCM mode or GCM mode.
                                                 When u32Crypto is SKE_CRYPTO_ENCRYPT: pointer to the MAC byte buffer to store the MAC.
                                                 When u32Crypto is SKE_CRYPTO_DECRYPT: pointer to the MAC byte buffer that to be authenticated. */
    uint32_t u32MacSize;                    /*!< Number of byte of the MAC to get or authenticated.
                                                 For CCM mode: this parameter must be a value of {4, 6, 8, 10, 12, 14, 16}
                                                 For GCM mode: this parameter must be a value between 0 and 16. */
} stc_ske_xcm_final_t;

/**
 * @brief SKE initialization structure.
 */
typedef struct {
    uint32_t u32Alg;                        /*!< SKE algorithm.
                                                 This parameter can be a value of @ref SKE_Algorithm */
    uint32_t u32Mode;                       /*!< SKE crypto mode.
                                                 This parameter can be a value of @ref SKE_Crypto_Mode */
    uint32_t u32DataType;                   /*!< SKE data type.
                                                 This parameter can be a value of @ref SKE_Data_Type */
    uint32_t u32Crypto;                     /*!< SKE crypto action.
                                                 This parameter can be a value of @ref SKE_Crypto_Action
                                                 NOTE: Ignore if u32Mode is SKE_MD_CMAC */
    const uint8_t *pu8Key;                  /*!< Pointer to SKE key buffer. The key size is determined by the algorithm. */
    uint8_t *pu8Iv;                         /*!< Pointer to SKE IV buffer. The key size is determined by the mode.
                                                 NOTE!!!:
                                                 ECB and CMAC mode: set this parameter to NULL.
                                                 GCM mode: The IV size is 12 byte.
                                                 CCM mode: Will generated from parameters 'u32LengthSize' and 'pu8Nonce' of @ref stc_ske_ccm_init_para_t */
    stc_ske_gcm_init_para_t *pstcGcmInit;   /*!< More initialization parameters for GCM mode. Set it to NULL if u32Mode is not SKE_MD_GCM */
    stc_ske_ccm_init_para_t *pstcCcmInit;   /*!< More initialization parameters for CCM mode. Set it to NULL if u32Mode is not SKE_MD_CCM */
} stc_ske_init_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup SKE_Global_Macros SKE Global Macros
 * @{
 */

/**
 * @defgroup SKE_Crypto_Mode SKE Crypto Mode
 * @{
 */
/* SKE algorithms and corresponding mode */
/**
@verbatim
 * |-----------|-------|-------|-------|-------|-------|-------|-------|-------|
 * |           |  ECB  |  CBC  |  CFB  |  OFB  |  CTR  |  CMAC |  GCM  |  CCM  |
 * |-----------|-------|-------|-------|-------|-------|-------|-------|-------|
 * |  AES_128  |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |
 * |-----------|-------|-------|-------|-------|-------|-------|-------|-------|
 * |  AES_192  |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |
 * |-----------|-------|-------|-------|-------|-------|-------|-------|-------|
 * |  AES_256  |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |
 * |-----------|-------|-------|-------|-------|-------|-------|-------|-------|
 * |    SM4    |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |   Y   |
 * |-----------|-------|-------|-------|-------|-------|-------|-------|-------|
 * Y: Supported
 * N: NOT supported
@endverbatim
 */
#define SKE_MD_ECB                  (1UL << SKE_CFG_MODE_POS)           /*!< Electronic Code Book(ECB) mode, defined by NIST SP800-38A */
#define SKE_MD_CBC                  (3UL << SKE_CFG_MODE_POS)           /*!< Cipher Block Chaining(CBC) mode, defined by NIST SP800-38A */
#define SKE_MD_CFB                  (4UL << SKE_CFG_MODE_POS)           /*!< Cipher FeedBack(CFB) mode(CFB-128 only), defined by NIST SP800-38A */
#define SKE_MD_OFB                  (5UL << SKE_CFG_MODE_POS)           /*!< Output FeedBack(OFB) mode, defined by NIST SP800-38A */
#define SKE_MD_CTR                  (6UL << SKE_CFG_MODE_POS)           /*!< Counter(CTR) mode, defined by NIST SP800-38A*/
#define SKE_MD_CMAC                 (7UL << SKE_CFG_MODE_POS)           /*!< Cipher-based message authentication code(CMAC) mode, defined by NIST SP800-38B */
#define SKE_MD_GCM                  (9UL << SKE_CFG_MODE_POS)           /*!< Galois/Counter Mode(GCM), defined by NIST SP800-38D */
#define SKE_MD_CCM                  (10UL << SKE_CFG_MODE_POS)          /*!< Generic authenticate-and-encrypt block cipher mode, defined by NIST SP800-38C */
/**
 * @}
 */

/**
 * @defgroup SKE_Algorithm SKE Algorithm
 * @{
 */
/* SKE algorithm and corresponding key size and block size */
/**
@verbatim
 * |-----------|---------------|-----------------|
 * |  SKE Alg. | Key size(bit) | Block size(bit) |
 * |-----------|---------------|-----------------|
 * |  AES-128  |      128      |       128       |
 * |-----------|---------------|-----------------|
 * |  AES-192  |      192      |       128       |
 * |-----------|---------------|-----------------|
 * |  AES-256  |      256      |       128       |
 * |-----------|---------------|-----------------|
 * |    SM4    |      128      |       128       |
 * |-----------|---------------|-----------------|
@endverbatim
 */
#define SKE_ALG_AES_128             (0x1UL)                             /*!< Select AES-128 as SKE algorithm.*/
#define SKE_ALG_AES_192             (0x4UL)                             /*!< Select AES-192 as SKE algorithm. */
#define SKE_ALG_AES_256             (0x5UL)                             /*!< Select AES-256 as SKE algorithm. */
#define SKE_ALG_SM4                 (0x2UL)                             /*!< Select SM4 as SKE algorithm. */
/**
 * @}
 */

/**
 * @defgroup SKE_Key_Size SKE Key Size
 * @{
 */
#define SKE_KEY_SIZE_8BYTE          (8U)                                /*!< Key size is 8 bytes */
#define SKE_KEY_SIZE_16BYTE         (16U)                               /*!< Key size is 16 bytes */
#define SKE_KEY_SIZE_24BYTE         (24U)                               /*!< Key size is 24 bytes */
#define SKE_KEY_SIZE_32BYTE         (32U)                               /*!< Key size is 32 bytes */

#define SKE_KEY_SIZE_64BIT          (SKE_KEY_SIZE_8BYTE)                /*!< Key size is 64 bits */
#define SKE_KEY_SIZE_128BIT         (SKE_KEY_SIZE_16BYTE)               /*!< Key size is 128 bits */
#define SKE_KEY_SIZE_192BIT         (SKE_KEY_SIZE_24BYTE)               /*!< Key size is 192 bits */
#define SKE_KEY_SIZE_256BIT         (SKE_KEY_SIZE_32BYTE)               /*!< Key size is 256 bits */
/**
 * @}
 */

/**
 * @defgroup SKE_Block_Size SKE Block Size
 * @{
 */
#define SKE_BLOCK_SIZE_8BYTE        (8U)                                /*!< Block size is 8 bytes */
#define SKE_BLOCK_SIZE_16BYTE       (16U)                               /*!< Block size is 16 bytes */
#define SKE_AES_BLOCK_SIZE          (SKE_BLOCK_SIZE_16BYTE)
#define SKE_SM4_BLOCK_SIZE          (SKE_BLOCK_SIZE_16BYTE)
/**
 * @}
 */

/**
 * @defgroup SKE_IV_Size SKE Initial Vector Size
 * @{
 */
#define SKE_IV_SIZE_8BYTE           (8U)                                /*!< Initial vector size is 8 bytes */
#define SKE_IV_SIZE_12BYTE          (12U)                               /*!< Initial vector size is 12 bytes. GCM IV size is 12 bytes */
#define SKE_IV_SIZE_16BYTE          (16U)                               /*!< Initial vector size is 16 bytes */
/**
 * @}
 */

/**
 * @defgroup SKE_Data_Type SKE Data Type
 * @brief Data swapping of DIN or DOUT
 * @{
 */
#define SKE_DATA_SWAP_NON           (0x0U)                              /*!< No swap: data not changed */
#define SKE_DATA_SWAP_HALF_WORD     (0x1UL << SKE_CFG_DATA_TYPE_POS)    /*!< Half word swap: 0x11223344 -> 0x33441122 */
#define SKE_DATA_SWAP_BYTE          (0x2UL << SKE_CFG_DATA_TYPE_POS)    /*!< Byte swap: 0x11223344 -> 0x44332211 */
#define SKE_DATA_SWAP_BIT           (0x3UL << SKE_CFG_DATA_TYPE_POS)    /*!< Bit swap: 0x11223344 -> 0x22CC4488 */
/**
 * @}
 */

/**
 * @defgroup SKE_Crypto_Action SKE Crypto Action
 * @{
 */
#define SKE_CRYPTO_ENCRYPT          (0x0U)                              /*!< SKE encrypting */
#define SKE_CRYPTO_DECRYPT          (0x1U)                              /*!< SKE decrypting */
/**
 * @}
 */

/**
 * @defgroup SKE_CMAC_Action SKE CMAC Action
 * @{
 */
#define SKE_CMAC_GENERATE           SKE_CRYPTO_ENCRYPT                  /*!< SKE CMAC mode generates MAC */
#define SKE_CMAC_VERIFY             SKE_CRYPTO_DECRYPT                  /*!< SKE CMAC mode verifies MAC */
/**
 * @}
 */

/**
 * @defgroup SKE_Status_Flag SKE Status Flag
 * @{
 */
#define SKE_FLAG_BUSY               (1UL << 0U)                         /*!< SKE is busy */
#define SKE_FLAG_MID_VALID          (1UL << 1U)                         /*!< The middle value is valid */
#define SKE_FLAG_DONE               (1UL << 16U)                        /*!< SKE calculation done */

#define SKE_FLAG_ALL                (SKE_FLAG_BUSY | SKE_FLAG_MID_VALID | SKE_FLAG_DONE)
#define SKE_FLAG_CLR_ALL            (SKE_FLAG_MID_VALID | SKE_FLAG_DONE)
/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
  Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup SKE_Global_Functions
 * @{
 */
int32_t SKE_Init(const stc_ske_init_t *pstcSkeInit);
int32_t SKE_StructInit(stc_ske_init_t *pstcSkeInit);
int32_t SKE_DeInit(void);

void SKE_UpdateConfigCmd(en_functional_state_t enNewState);
void SKE_SetDataType(uint32_t u32DataType);
void SKE_SetCrypto(uint32_t u32Crypto);
void SKE_SetAlgorithm(uint32_t u32Alg);
void SKE_SetMode(uint32_t u32Mode);
void SKE_SetLastBlockMark(void);
void SKE_ResetLastBlockMark(void);
void SKE_SetCmacLastBlockSize(uint32_t u32Size);
void SKE_ResetCmacLastBlockSize(void);
int32_t SKE_SetKey(uint32_t u32Alg, const uint8_t *pu8Key);
int32_t SKE_ExpandKey(void);
int32_t SKE_SetIv(uint32_t u32Alg, uint32_t u32Mode, const uint8_t *pu8Iv);
void SKE_ResetIv(void);
void SKE_SetAadSize(uint32_t u32AadSize);
void SKE_SetCryptoSize(uint32_t u32CryptoSize);
uint8_t SKE_GetBlockSize(uint32_t u32Alg);
int32_t SKE_WriteBlock(uint32_t u32Alg, const uint8_t *pu8In);
int32_t SKE_ReadBlock(uint32_t u32Alg, uint8_t *pu8Out);
int32_t SKE_ReadMidIv(uint8_t *pu8Out);
void SKE_IntCmd(en_functional_state_t enNewState);
void SKE_Start(void);
int32_t SKE_WaitTillDone(void);
en_flag_status_t SKE_GetStatus(uint32_t u32Flag);
void SKE_ClearStatus(uint32_t u32Flag);

/* For all modes except CMAC */
int32_t SKE_CryptoBlocks(stc_ske_crypto_t *pstcCrypto);

/* For CMAC */
int32_t SKE_CmacAction(stc_ske_cmac_action_t *pstcAction);

/* For GCM mode and CCM mode */
int32_t SKE_XcmFinal(stc_ske_xcm_final_t *pstcFinal);

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

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_SKE_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
