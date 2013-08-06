/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : CMPI_proc.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/08/02
  Description   :
  History       :
  1.Date        : 2006/08/02
    Author      : c42025
    Modification: Created file

******************************************************************************/
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include "drv_proc_ext.h"

#define MAX_PROC_ENTRIES 256

static struct proc_dir_entry *s_pCMPI_proc = NULL;
static struct proc_dir_entry *s_pHisi_proc = NULL;
static DRV_PROC_ITEM_S s_proc_items[MAX_PROC_ENTRIES];

static int CMPI_proc_open(struct inode *inode, struct file *file)
{
        DRV_PROC_ITEM_S *item = PDE(inode)->data;
        return single_open(file, item->read, item);
}

static ssize_t CMPI_ProcWrite(struct file * file,
    const char __user * buf, size_t count, loff_t *ppos)
{
    struct seq_file *s = file->private_data;
    DRV_PROC_ITEM_S *item = s->private;

    if (item->write)
        return item->write(file, buf, count, ppos);
    return -ENOSYS;
}

static long CMPI_ProcUnlockedIoctl(struct file * pFile, unsigned int cmd, unsigned long arg)
{
    struct seq_file *s = pFile->private_data;
    DRV_PROC_ITEM_S *item = s->private;

    
    if (item->ioctl)
    {
        return item->ioctl(s, cmd, arg);
    }
    
    return 0;
}

static struct file_operations CMPI_proc_ops = {
    .owner   = THIS_MODULE,
    .open    = CMPI_proc_open,
    .read    = seq_read,
    .write   = CMPI_ProcWrite,
    .llseek  = seq_lseek,
    .release = single_release,
    .unlocked_ioctl   = CMPI_ProcUnlockedIoctl,
};

static HI_BOOL IsHex(char *ptr)
{
	int i, strlength;
	strlength = strlen(ptr);

	if(strlength<=2)
	{
		return HI_FALSE;
	}

	ptr += 2;
	for (i = 0; i < strlength -2; i++)
	{
		if((ptr[i]<'0') || ptr[i]>'f')
		{
			return HI_FALSE;
		}

		if((ptr[i]>'9') && ptr[i]<'a')
		{
			return HI_FALSE;
		}
	}
	return HI_TRUE;
}

static HI_S32 String2Digit(unsigned char  *ptr, HI_U32 *pu32Para)
{
	int i, strlength;
	HI_U32 u32Value = 0;

	strlength = strlen(ptr);
	if(strlength<=2)
	{
		return HI_FAILURE;
	}

	ptr += 2;
	for(i=0;i<strlength-2;i++)
	{
		u32Value <<= 4;
		if(ptr[i] <= '9')
			u32Value += ptr[i] - '0';
		else
			u32Value += ptr[i] - 'a' + 0x0a;
	}
    *pu32Para = u32Value;

	return HI_SUCCESS;
}

HI_S32 CMPI_PROC_PaserCommand(char *ptr, HI_U32 *pu32Para1, HI_U32 *pu32Para2)
{
	int i;
    HI_CHAR the_args[2][40];


	while(*ptr==' ' && *ptr++ != '\0');

	for(i=strlen(ptr);i>0;i--)
	{
		if((*(ptr+i-1) < '0') || (*(ptr+i-1) > 'f'))
		{
			*(ptr+i-1) = '\0';
		}
		else if((*(ptr+i-1) > '9') && (*(ptr+i-1) < 'a'))
		{
			*(ptr+i-1) = '\0';
		}
		else
			break;
	}

	for(i=0;i<2;i++)
	{
		int j = 0;
		while(*ptr==' ' && *ptr++ != '\0');
		while((*ptr !=' ') && (*ptr!='\0'))
		{
			the_args[i][j++] = *ptr++;
		}
		the_args[i][j] = '\0';
		if('\0' == *ptr)
		{
			i++;
			break;
		}
		the_args[i][j] = '\0';
	}

	if(('0' != the_args[0][0]) ||('x' != the_args[0][1]) ||
		('0' != the_args[1][0]) ||('x' != the_args[1][1]))
	{
		return -1;
	}
	if((!IsHex(the_args[0])) || (!IsHex(the_args[1])))
	{
		return -1;
	}

	if(HI_SUCCESS != String2Digit(the_args[0], pu32Para1))
    {
        return -1;
    }

	if(HI_SUCCESS != String2Digit(the_args[1], pu32Para2))
    {
        return -1;
    }

	return i;
}

static HI_VOID WriteProcShowHelp(HI_VOID)
{
	printk("\nPLS type \"echo 0xxxxxxxxx 0xxxxxxxxx > /proc/msp/demux\"\n");
	printk("E.g.: \"echo 0x40002 0x4010 > /proc/msp/demux\"\n");
}

ssize_t HI_DRV_PROC_ModuleWrite(struct file * file,
    const char __user *buf, size_t count, loff_t *ppos, PROC_CTRL fun_ctl)
{
	HI_CHAR szBuffer[41];
	HI_U32 para1, para2;

    if (*ppos >= 41)
        return -EFBIG;

	if(count >= 40)
	{
		WriteProcShowHelp();
		goto out;
	}

    if(copy_from_user(szBuffer, buf, count))
        return -EFAULT;

    //printk("proc command:%s\n", szBuffer);

    if (CMPI_PROC_PaserCommand(szBuffer, &para1, &para2) > 0)
    {
        //printk("value:0x%x, 0x%x\n", para1,  para2);
        fun_ctl(para1, para2);
    }
    else
    {
        WriteProcShowHelp();
    }

out:
    *ppos = count;
    return count;
}


static DRV_PROC_ITEM_S *proc_addModule(char *entry_name, DRV_PROC_EX_S* pFnOp, void * data)
{
    struct proc_dir_entry *entry;
    int i;
    for (i = 0; i < MAX_PROC_ENTRIES; i++)
        if (! s_proc_items[i].entry)
            break;

    if (MAX_PROC_ENTRIES == i)
        return NULL;

    entry = create_proc_entry(entry_name, 0, s_pCMPI_proc);
    if (!entry)
        return NULL;

    entry->proc_fops = &CMPI_proc_ops;
    strcpy(s_proc_items[i].entry_name, entry_name);
    s_proc_items[i].entry = entry;

    if (pFnOp != NULL)
    {
        s_proc_items[i].read = pFnOp->fnRead;
        s_proc_items[i].write = pFnOp->fnWrite;
        s_proc_items[i].ioctl = pFnOp->fnIoctl;
    }
    else
    {
        s_proc_items[i].read = NULL;
        s_proc_items[i].write = NULL;
        s_proc_items[i].ioctl = NULL;
    }
    s_proc_items[i].data = data;
    entry->data = &s_proc_items[i];
    return &s_proc_items[i];
}

static HI_VOID proc_removeModule(char *entry_name)
{
    int i;
    for (i = 0; i < MAX_PROC_ENTRIES; i++)
        if (! strcmp(s_proc_items[i].entry_name, entry_name))
            break;
    if (MAX_PROC_ENTRIES == i)
        return ;
    remove_proc_entry(s_proc_items[i].entry_name, s_pCMPI_proc);
    s_proc_items[i].entry = NULL;
}

static DRV_PROC_INTFPARAM  procparam = {
	.addModulefun = proc_addModule,
	.rmvModulefun = proc_removeModule,
};


HI_S32 HI_DRV_PROC_Init(HI_VOID)
{
    s_pHisi_proc = proc_mkdir("hisi", NULL);
	s_pCMPI_proc = proc_mkdir("msp", s_pHisi_proc);
	memset(s_proc_items, 0x00, sizeof(s_proc_items));

	HI_DRV_PROC_RegisterParam(&procparam);

    proc_mkdir("graphics", s_pHisi_proc);
    proc_symlink("msp", NULL, "hisi/msp");
    proc_symlink("graphics", NULL, "hisi/graphics");
	return HI_SUCCESS;
}

HI_VOID HI_DRV_PROC_Exit(HI_VOID)
{
	HI_DRV_PROC_UnRegisterParam();

    remove_proc_entry("msp", NULL);
    remove_proc_entry("graphics", NULL);
    remove_proc_entry("msp", s_pHisi_proc);
    remove_proc_entry("graphics", s_pHisi_proc);
    remove_proc_entry("hisi", NULL);
    return;
}

EXPORT_SYMBOL(HI_DRV_PROC_ModuleWrite);
