#ifndef __DRV_ADSP_EXT_H__
#define __DRV_ADSP_EXT_H__

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* __cplusplus */

#include "hi_type.h"
#include "hi_audsp_common.h"

typedef union hiADSP_VERSIONTYPE_U
{
    struct
    {
        HI_U8 u8VersionMajor;      /**< Major version */
        HI_U8 u8VersionMinor;      /**< Minor version */
        HI_U8 u8Revision;          /**< Revision version */
        HI_U8 u8Step;              /**< Step version */
    } s;
    HI_U32 u32Version;
} ADSP_VERSIONTYPE_U;

typedef struct hiADSP_FIRMWARE_INFO_S
{
    /**< IN. dsp code Id */
    ADSP_CODEID_E u32DspCodeId;

    /**< IN. dsp code version */
    ADSP_VERSIONTYPE_U uVersion;

    /**< IN. Description infomation of the code */
    HI_PCHAR pszDescription;
} ADSP_FIRMWARE_INFO_S;

typedef HI_S32 (*FN_ADSP_LoadFrm)(ADSP_CODEID_E);

typedef HI_S32 (*FN_ADSP_UnLoadFrm)(ADSP_CODEID_E);

typedef HI_S32 (*FN_ADSP_GetFrmInfo)(ADSP_CODEID_E, ADSP_FIRMWARE_INFO_S *);

typedef struct tagADSP_EXPORT_FUNC_S
{
    FN_ADSP_LoadFrm pfnADSP_LoadFirmware;
    FN_ADSP_UnLoadFrm pfnADSP_UnLoadFirmware;
    FN_ADSP_GetFrmInfo pfnADSP_GetFirmwareInfo;
} ADSP_EXPORT_FUNC_S;

HI_S32	ADSP_DRV_ModInit(HI_VOID);
HI_VOID ADSP_DRV_ModExit(HI_VOID);

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* __cplusplus */

#endif /* __DRV_ADSP_EXT_H__ */