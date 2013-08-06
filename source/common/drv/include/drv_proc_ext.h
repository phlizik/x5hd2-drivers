/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : CMPI_proc.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/08/03
  Description   :
  History       :
  1.Date        : 2006/00/03
    Author      : c42025
    Modification: Created file
******************************************************************************/

#ifndef __DRV_PROC_EXT_H__
#define __DRV_PROC_EXT_H__

#include <linux/seq_file.h>
#include "hi_type.h"

#ifdef __cplusplus
extern "C"
{
#endif 

/** @addtogroup H_PROC */
/** @{ */

#define DRV_PROC_SHOW DRV_PROC_READ_FN
typedef HI_S32 (*PROC_CTRL)(HI_U32, HI_U32);

typedef HI_S32 (*DRV_PROC_READ_FN)(struct seq_file *, HI_VOID *);
typedef HI_S32 (*DRV_PROC_WRITE_FN)(struct file * file,
    const char __user * buf, size_t count, loff_t *ppos);
typedef HI_S32 (*DRV_PROC_IOCTL_FN)(struct seq_file *, HI_U32 cmd, HI_U32 arg);

typedef struct struCMPI_PROC_ITEM
{
    HI_CHAR entry_name[32];
    struct proc_dir_entry *entry;
    DRV_PROC_READ_FN read;
    DRV_PROC_WRITE_FN write;
    DRV_PROC_IOCTL_FN ioctl;
    HI_VOID *data;
}DRV_PROC_ITEM_S;
typedef struct tagPROCEX
{
    DRV_PROC_READ_FN fnRead;
    DRV_PROC_WRITE_FN fnWrite;
    DRV_PROC_IOCTL_FN fnIoctl;
}DRV_PROC_EX_S;

typedef HI_VOID(*PROC_RemoveModule_Fun)(char *);
typedef DRV_PROC_ITEM_S *(*PROC_AddModule_Fun)(char *, DRV_PROC_EX_S*, void *);

typedef struct struCMPI_Proc_IntfParam{
	PROC_AddModule_Fun     addModulefun;
	PROC_RemoveModule_Fun  rmvModulefun;
}DRV_PROC_INTFPARAM;

HI_S32  HI_DRV_PROC_RegisterParam(DRV_PROC_INTFPARAM *param);
HI_VOID HI_DRV_PROC_UnRegisterParam(HI_VOID);
HI_S32  HI_DRV_PROC_KInit(HI_VOID);
HI_VOID HI_DRV_PROC_KExit(HI_VOID);

ssize_t HI_DRV_PROC_ModuleWrite(struct file * file,
    const char __user * buf, size_t count, loff_t *ppos, PROC_CTRL fun_ctl);

DRV_PROC_ITEM_S* HI_DRV_PROC_AddModule(HI_CHAR *, DRV_PROC_SHOW, HI_VOID *);
DRV_PROC_ITEM_S* HI_DRV_PROC_AddModuleEx(HI_CHAR *,DRV_PROC_EX_S*, HI_VOID *);
HI_VOID HI_DRV_PROC_RemoveModule(HI_CHAR *);

HI_S32  HI_DRV_PROC_Init(HI_VOID);
HI_VOID HI_DRV_PROC_Exit(HI_VOID);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __DRV_PROC_EXT_H__ */
