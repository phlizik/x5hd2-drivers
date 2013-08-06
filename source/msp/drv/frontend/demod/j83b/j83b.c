/***********************************************************************************
 *              Copyright 2004 - 2050, Hisilicon Tech. Co., Ltd.
 *                           ALL RIGHTS RESERVED
 * FileName: hi3130.c
 * Description:
 *
 * History:
 * Version   Date             Author     DefectNum        Description
 * main\1    2007-10-16   w54542    NULL                Create this file.
 ***********************************************************************************/

#include <linux/delay.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/time.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <asm/io.h>

//#include "common_sys.h"
#include "hi_type.h"
#include "drv_i2c_ext.h"
#include "hi_debug.h"
#include "hi_error_mpi.h"
#include "drv_proc_ext.h"
#include "drv_reg_ext.h"
#include "drv_sys_ext.h"
#include "drv_tuner_ioctl.h"
#include "drv_demod.h"

#define SYMRATE_DEVISION 4800   /* KHz */

extern wait_queue_head_t g_QamSpecialProcessWQ;
extern  HI_VOID HI_DRV_SYS_GetChipVersion(HI_CHIP_TYPE_E *penChipType, HI_CHIP_VERSION_E *penChipVersion);

/* TDA18250 J83B CONFIG */
static REGVALUE_S s_stTda18250Qam16[]=
{
#include "qamdata/TDA18250/TDA18250_QAM16_J83B.txt"
};

static REGVALUE_S s_stTda18250Qam32[]=
{
#include "qamdata/TDA18250/TDA18250_QAM32_J83B.txt"
};

static REGVALUE_S s_stTda18250Qam64[]=
{
#include "qamdata/TDA18250/TDA18250_QAM64_J83B.txt"
};

static REGVALUE_S s_stTda18250Qam128[]=
{
#include "qamdata/TDA18250/TDA18250_QAM128_J83B.txt"
};

static REGVALUE_S s_stTda18250Qam256[]=
{
#include "qamdata/TDA18250/TDA18250_QAM256_J83B.txt"
};

static REGVALUE_S *s_astTda18250QamRegs[HI_TUNER_QAM_TYPE_MAX] =
{
    s_stTda18250Qam16, s_stTda18250Qam32, s_stTda18250Qam64, s_stTda18250Qam128, s_stTda18250Qam256
};

/*R820C J83B CONFIG*/
static REGVALUE_S s_stR820cQam16[]=
{
#include "qamdata/R820C/R820C_QAM16_J83B.txt"
};

static REGVALUE_S s_stR820cQam32[]=
{
#include "qamdata/R820C/R820C_QAM32_J83B.txt"
};

static REGVALUE_S s_stR820cQam64[]=
{
#include "qamdata/R820C/R820C_QAM64_J83B.txt"
};

static REGVALUE_S s_stR820cQam128[]=
{
#include "qamdata/R820C/R820C_QAM128_J83B.txt"
};

static REGVALUE_S s_stR820cQam256[]=
{
#include "qamdata/R820C/R820C_QAM256_J83B.txt"
};

static REGVALUE_S *s_astR820cQamRegs[HI_TUNER_QAM_TYPE_MAX] =
{
    s_stR820cQam16, s_stR820cQam32, s_stR820cQam64, s_stR820cQam128, s_stR820cQam256
};

/* TDA18250B J83B CONFIG */
static REGVALUE_S s_stTda18250bQam16[]=
{
#include "qamdata/TDA18250B/TDA18250B_QAM16_J83B.txt"
};

static REGVALUE_S s_stTda18250bQam32[]=
{
#include "qamdata/TDA18250B/TDA18250B_QAM32_J83B.txt"
};

static REGVALUE_S s_stTda18250bQam64[]=
{
#include "qamdata/TDA18250B/TDA18250B_QAM64_J83B.txt"
};

static REGVALUE_S s_stTda18250bQam128[]=
{
#include "qamdata/TDA18250B/TDA18250B_QAM128_J83B.txt"
};

static REGVALUE_S s_stTda18250bQam256[]=
{
#include "qamdata/TDA18250B/TDA18250B_QAM256_J83B.txt"
};

static REGVALUE_S *s_astTda18250bQamRegs[HI_TUNER_QAM_TYPE_MAX] =
{	
    s_stTda18250bQam16, s_stTda18250bQam32, s_stTda18250bQam64, s_stTda18250bQam128, s_stTda18250bQam256
};

static char *s_stQamRegName[]=
{
#include "qamdata/QAMPRINT_J83B.txt"
};


#define X5HDQAMV200_I2C_CHECK(tunerport) \
 do\
    {\
        HI_U8 u8Tmp = 0;\
        if (HI_SUCCESS != qam_read_byte(tunerport, AGC_CTRL_1_ADDR, &u8Tmp))\
        {\
            HI_ERR_TUNER("j83b_i2c_check failure,please check i2c:%d\n", g_stTunerOps[tunerport].enI2cChannel);\
            return HI_FAILURE;\
        }\
    } while (0)

static HI_VOID set_symrate(HI_U32 u32TunerPort, HI_U32 u32SymRate)
{
    HI_U32 u32Temp;/*= (HI_U32)(M_CLK * 8192 / nSymRate);*/
    HI_U8  u8Temp = 0;

    u32Temp = (HI_U32)(25000 * 8192 / u32SymRate);

    qam_read_byte(u32TunerPort, TR_CTRL_8_ADDR, &u8Temp);

    u8Temp = (u8Temp & 0xf8) | (u32Temp & 0x70000) >> 16;

    qam_write_byte(u32TunerPort, TR_CTRL_8_ADDR, u8Temp);
    qam_write_byte(u32TunerPort, TR_CTRL_9_ADDR, (u32Temp & 0xff00) >> 8);
    qam_write_byte(u32TunerPort, TR_CTRL_10_ADDR, u32Temp & 0xff);

    switch (g_stTunerOps[u32TunerPort].u32CurrQamMode)
    {
        case QAM_TYPE_64:
            if ( u32SymRate < SYMRATE_DEVISION )
            {
                qam_write_byte(u32TunerPort, DEPHASE_GAIN_K_HI_ADDR, 0x02);
            }
            else
            {
                qam_write_byte(u32TunerPort, DEPHASE_GAIN_K_HI_ADDR, 0x08);
            }

            break;

        case QAM_TYPE_128:
            if ( u32SymRate < SYMRATE_DEVISION )
            {
                qam_write_byte(u32TunerPort, DEPHASE_GAIN_K_HI_ADDR, 0x01);
            }
            else
            {
                qam_write_byte(u32TunerPort, DEPHASE_GAIN_K_HI_ADDR, 0x05);
            }

            break;

        case QAM_TYPE_256:
            if ( u32SymRate < SYMRATE_DEVISION )
            {
                qam_write_byte(u32TunerPort, DEPHASE_GAIN_K_HI_ADDR, 0x00);
            }
            else
            {
                qam_write_byte(u32TunerPort, DEPHASE_GAIN_K_HI_ADDR, 0x06);
            }

            break;

        default:
            break;

    }
}

HI_VOID j83b_test_single_agc( HI_U32 u32TunerPort, AGC_TEST_S * pstAgcTest )
{
    HI_S32 s32Ret = 0;
    HI_U8  u8RegVal = 0;
    HI_U8  au8Tmp[2] = {0};

    s32Ret = qam_read_byte(u32TunerPort, MCTRL_11_ADDR, &u8RegVal);
    if (HI_SUCCESS != s32Ret)
    {
        HI_ERR_TUNER("i2c operation error\n");
        return;
    }

    if ( 0xf7 == u8RegVal )
    {
        pstAgcTest->bLockFlag = HI_TRUE;
        pstAgcTest->bAgcLockFlag = HI_TRUE;
    }
    else if ( 1 == ( u8RegVal & 0x80 ) )
    {
        pstAgcTest->bAgcLockFlag = HI_TRUE;
    }

    qam_write_byte(u32TunerPort, CR_CTRL_21_ADDR, 0x1);

    qam_read_byte(u32TunerPort, BAGC_STAT_3_ADDR, &au8Tmp[0]);
    qam_read_byte(u32TunerPort, BAGC_STAT_4_ADDR, &au8Tmp[1]);
    pstAgcTest->u32Agc2 = ((au8Tmp[0]) << 4) | (( au8Tmp[1] >> 4) & 0x0f );

    qam_read_byte(u32TunerPort, BAGC_CTRL_12_ADDR, &pstAgcTest->u8BagcCtrl12 );

    return;
}

static HI_VOID j83b_chip_reset(HI_U32 u32TunerPort, qam_module_e enResetModule)
{
    qam_write_byte(u32TunerPort, AGC_CTRL_3_ADDR, 0x20);

    if (ALL_CHIP == enResetModule)
    {
        qam_write_byte(u32TunerPort, MCTRL_1_ADDR, 0xff);
        qam_write_byte(u32TunerPort, MCTRL_1_ADDR, 0x00);
    }
    else
    {
        qam_write_bit(u32TunerPort, MCTRL_1_ADDR, (HI_U8)enResetModule, 1);
        qam_write_bit(u32TunerPort, MCTRL_1_ADDR, (HI_U8)enResetModule, 0);
    }

    qam_write_byte(u32TunerPort, AGC_CTRL_3_ADDR, 0xef);

}

/*note:this function should not be deleted,reserved for debug*/

HI_VOID j83b_get_registers(HI_U32 u32TunerPort, void *p)
{
    HI_U8   u8Value = 0;
    HI_S32  i = 0;
    HI_S32  s32RegSum = 0;
    HI_U32  u32QamType = 0;
    REGVALUE_S *pstReg = HI_NULL;

    u32QamType = g_stTunerOps[u32TunerPort].u32CurrQamMode;
    pstReg = s_astTda18250QamRegs[u32QamType];
    s32RegSum = sizeof(s_stTda18250Qam64)/sizeof(REGVALUE_S);

    for( i = 0; i < s32RegSum; i++ )
    {
        if ( 0 == i % 3 )
        {
            seq_printf(p, "\n");
        }
        qam_read_byte(u32TunerPort, pstReg->u8Addr, &u8Value);
        pstReg++;
        seq_printf(p, "{%s  ,0x%02x },  ", s_stQamRegName[i], u8Value);
    }
    seq_printf(p, "\n\n");

}

static HI_VOID j83b_set_customreg_value(HI_U32 u32TunerPort, HI_BOOL bInversion, HI_U32 u32SymbolRate)
{

    HI_U8 u8Val;
    HI_U32 u32RegVal;

    HI_CHIP_TYPE_E enChipType;
    HI_U32 u32ChipVersion = 0;

    /*set adc type*/
    if ( QAM_AD_OUTSIDE == g_stTunerOps[u32TunerPort].u8AdcType )
    {
        qam_write_bit(u32TunerPort, MCTRL_8_ADDR, 7, 1);
    }
    else
    {
        qam_write_bit(u32TunerPort, MCTRL_8_ADDR, 7, 0);
    }


    if ( QAM_AGC_CMOS_OUTPUT == g_stTunerOps[u32TunerPort].u8AgcOutputSel )
    {
        qam_write_bit( u32TunerPort, MCTRL_5_ADDR, 7, 0 );
        qam_write_bit( u32TunerPort, MCTRL_8_ADDR, 2, 0 );
    }
    else    /* OD */
    {
        qam_write_bit( u32TunerPort, MCTRL_5_ADDR, 7, 1 );
        qam_write_bit( u32TunerPort, MCTRL_8_ADDR, 2, 1 );
    }

#if defined (CHIP_TYPE_hi3716cv200es)|| defined (CHIP_TYPE_hi3716cv200)
    HI_DRV_SYS_GetChipVersion( &enChipType, &u32ChipVersion );
    if(((HI_CHIP_TYPE_HI3716CES == enChipType)||(HI_CHIP_TYPE_HI3716C == enChipType )) && (HI_CHIP_VERSION_V200 == u32ChipVersion))
    {
        u32RegVal = 0;
        HI_REG_READ(IO_ADDRESS(PERI_CRG57), u32RegVal);
        if(HI_UNF_TUNER_OUTPUT_MODE_SERIAL_50 == g_stTunerOps[u32TunerPort].enTsType
           || HI_UNF_TUNER_OUTPUT_MODE_SERIAL == g_stTunerOps[u32TunerPort].enTsType)
        {
            /*select QAM TSCLK source 150MHz,PERI_CRG57[19:18]:01 ; QAM TSCLK 2 div,PERI_CRG57[23:20]:0000 */
            u32RegVal = ((u32RegVal  & (~(0x3 << 18))) | (0x1 << 18)) & (~(0xF << 20));
        }
        else
        {
            /*select QAM TSCLK source 150MHz,PERI_CRG57[19:18]:01 ; QAM TSCLK 16 div,PERI_CRG57[23:20] :0111,*/
            u32RegVal = (((u32RegVal  & (~(0x3 << 18))) | (0x1 << 18)) & (~(0xF << 20))) | (0x7 << 20);
        }
        HI_REG_WRITE(IO_ADDRESS(PERI_CRG57), u32RegVal);
    }
#else
    HI_REG_READ(IO_ADDRESS(0x101f50a0), u32RegVal);

    HI_DRV_SYS_GetChipVersion( &enChipType, &u32ChipVersion );
    if((HI_CHIP_TYPE_HI3716M == enChipType) && (HI_CHIP_VERSION_V300 == u32ChipVersion))
    {
        if(HI_UNF_TUNER_OUTPUT_MODE_SERIAL_50 == g_stTunerOps[u32TunerPort].enTsType
           || HI_UNF_TUNER_OUTPUT_MODE_SERIAL == g_stTunerOps[u32TunerPort].enTsType)
        {
            /*select QAM TSCLK source = 118.8M*/
            u32RegVal = u32RegVal & (~(0x3 << 17));
            /*QAM TSCLK, 59.4M*/
            u32RegVal = u32RegVal & (~(0xf << 24));
            HI_REG_WRITE(IO_ADDRESS(0x101f50a0), u32RegVal);
        }
        else
        {
            /*select QAM TSCLK source = 118.8M*/
            u32RegVal = u32RegVal & (~(0x3 << 17));
            /*QAM TSCLK, 11.88M*/
            u32RegVal = (u32RegVal & (~(0xf << 24))) | (0x4 << 24);
            HI_REG_WRITE(IO_ADDRESS(0x101f50a0), u32RegVal);
        }
    }
    else
    {
        if(HI_UNF_TUNER_OUTPUT_MODE_SERIAL_50 == g_stTunerOps[u32TunerPort].enTsType)
        {
            u32RegVal = (u32RegVal & ~(0x1 << 17)) | (0X1 << 18);/* QAM TSCLK 50M */
            HI_REG_WRITE(IO_ADDRESS(0x101f50a0), u32RegVal);
        }
        else if(HI_UNF_TUNER_OUTPUT_MODE_SERIAL == g_stTunerOps[u32TunerPort].enTsType)
        {
            u32RegVal = u32RegVal  | (0X3 << 17);/* QAM TSCLK 74.25M */
            HI_REG_WRITE(IO_ADDRESS(0x101f50a0), u32RegVal);
        }
        else
        {
            u32RegVal = u32RegVal  & ~(0X3 << 17);/*QAM TSCLK 13.5M*/
            HI_REG_WRITE(IO_ADDRESS(0x101f50a0), u32RegVal);
        }
    }
#endif

    /*ts type control*/
    switch (g_stTunerOps[u32TunerPort].enTsType)
    {
        case HI_UNF_TUNER_OUTPUT_MODE_PARALLEL_MODE_A:
        case HI_UNF_TUNER_OUTPUT_MODE_DEFAULT:
        {
            qam_write_bit(u32TunerPort, TS_CTRL_2_ADDR, 6, 1);  /* dvb */
            qam_write_bit(u32TunerPort, TS_CTRL_1_ADDR, 3, 0);  /* no  bserial */
            qam_write_bit(u32TunerPort, TS_CTRL_2_ADDR, 4, 0);
            break;
        }
        case HI_UNF_TUNER_OUTPUT_MODE_SERIAL:
        case HI_UNF_TUNER_OUTPUT_MODE_SERIAL_50:
        {
            qam_write_bit(u32TunerPort, TS_CTRL_2_ADDR, 6, 0);  /* not dvb */
            qam_write_bit(u32TunerPort, TS_CTRL_1_ADDR, 3, 1);  /* bserial */
            qam_write_bit(u32TunerPort, TS_CTRL_2_ADDR, 4, 1);
            break;
        }
        case HI_UNF_TUNER_OUTPUT_MODE_PARALLEL_MODE_B:
        {
            qam_write_bit(u32TunerPort, TS_CTRL_2_ADDR, 6, 0);  /* not dvb */
            qam_write_bit(u32TunerPort, TS_CTRL_1_ADDR, 3, 0);  /* no bserial */
            qam_write_bit(u32TunerPort, TS_CTRL_2_ADDR, 4, 0);
            break;
        }
        default:
            break;
    }

    /*inversion control */
    if(bInversion)
    {
        qam_write_bit(u32TunerPort, BS_CTRL_1_ADDR, 3, 1);
    }
    else
    {
        qam_write_bit(u32TunerPort, BS_CTRL_1_ADDR, 3, 0);
    }

    set_symrate(u32TunerPort, u32SymbolRate);


    qam_read_byte(u32TunerPort, MCTRL_5_ADDR, &u8Val);

    if(g_stTunerOps[u32TunerPort].u8AdcDataFmt == 0)
    {
        u8Val &= 0xF7;
    }
    else
    {
        u8Val |= 0x08;
    }

    qam_write_byte(u32TunerPort, MCTRL_5_ADDR, u8Val);

}

static HI_VOID j83b_set_reg_value(HI_U32 u32TunerPort)
{
    HI_S32  i = 0;
    HI_S32  s32RegSum;
    HI_U32  u32QamType;
    REGVALUE_S *pstReg;


    /*set for qam 16/32/64/128/256*/
    u32QamType = g_stTunerOps[u32TunerPort].u32CurrQamMode;

   switch( g_stTunerOps[u32TunerPort].enTunerDevType )
   {
        case HI_UNF_TUNER_DEV_TYPE_TDA18250:
        {
            pstReg = s_astTda18250QamRegs[u32QamType];
            break;
        }

        case HI_UNF_TUNER_DEV_TYPE_R820C:
        {
        	pstReg = s_astR820cQamRegs[u32QamType];
        	break;
        }	     
		case HI_UNF_TUNER_DEV_TYPE_TDA18250B:
        {			
        	pstReg = s_astTda18250bQamRegs[u32QamType];
        	break;
        }	     	     
        default:
        {
        	pstReg = s_astTda18250QamRegs[u32QamType];
    		break;
        } 	
    } 
  
    s32RegSum = sizeof(s_stTda18250Qam64)/sizeof(REGVALUE_S);

    for(i=0; i<s32RegSum; i++)
    {
        qam_write_byte(u32TunerPort, pstReg->u8Addr, pstReg->u8Value);
        pstReg++;
    }

}

HI_S32 j83b_get_status (HI_U32 u32TunerPort, HI_UNF_TUNER_LOCK_STATUS_E  *penTunerStatus)
{
    HI_U8  u8Status = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 i = 0;


    HI_ASSERT(HI_NULL != penTunerStatus);
    X5HDQAMV200_I2C_CHECK(u32TunerPort);

    for (i = 0; i < 4; i++)  /*prevent twittering*/
    {
        s32Ret = qam_read_byte(u32TunerPort, MCTRL_11_ADDR, &u8Status);


        if (HI_SUCCESS != s32Ret)
        {
            HI_ERR_TUNER( "i2c operation error\n");
            return HI_FAILURE;
        }

        if (0xF7 == (u8Status & 0xF7))
        {
            *penTunerStatus = HI_UNF_TUNER_SIGNAL_LOCKED;

            break;
        }
        else
        {
            *penTunerStatus = HI_UNF_TUNER_SIGNAL_DROPPED;
        }
    }

    return HI_SUCCESS;
}

#define J83B_CONNCECT_TIMEOUT   1000        /* ms */
#define J83B_CLOSE_EQU_TIME 400
HI_VOID j83b_manage_after_chipreset(HI_U32 u32TunerPort)
{
    HI_S32  s32Cnt = 0;
    HI_BOOL bStartFlag = HI_FALSE;
    HI_U8   u8IsReset = 0;

    HI_U8  u8LockRegVal = 0;
    HI_U8  u8Tmp = 0;
    HI_U32 u32Timeout = 0;

    HI_U32 u32StartTime = 0;
    HI_U32 u32EndTime = 0;
    HI_U32 u32EquResetTime = 0;

    #define J83B_CONNECT_CHECK_FREQ 20      /* ms */
    #define EQU_CTRL_11_ADDR        EQU_BASE_ADDR + 0x09

    HI_DRV_SYS_GetTimeStampMs(&u32StartTime);

    if ((QAM_TYPE_256 == g_stTunerOps[u32TunerPort].u32CurrQamMode) ||
             (QAM_TYPE_128 == g_stTunerOps[u32TunerPort].u32CurrQamMode))
    {
        while  (u32Timeout < J83B_CONNCECT_TIMEOUT)
        {
            qam_read_byte(u32TunerPort, MCTRL_11_ADDR, &u8LockRegVal);

            if (0xf7 == u8LockRegVal)
            {
                mdelay(5);

                qam_read_byte(u32TunerPort, MCTRL_11_ADDR, &u8LockRegVal);

                if (0xf7 != u8LockRegVal)
                {
                    continue;
                }

                /*reset_special_process_flag(HI_TRUE);
                wake_up_interruptible(&g_QamSpecialProcessWQ);*/
                break;
            }
            else
            {
                qam_read_byte(u32TunerPort, QAM_DEBUG_1_ADDR, &u8Tmp);

                HI_DRV_SYS_GetTimeStampMs(&u32EndTime);
                u32EquResetTime = u32EndTime - u32StartTime;
                if (u32EquResetTime > J83B_CLOSE_EQU_TIME)
                {
                    u8Tmp = u8Tmp & 0x7;

                    if (u8Tmp >= 0x3)
                    {
                        bStartFlag = HI_TRUE;
                    }

                    if (u8IsReset == 0)
                    {
                        /*don't update tap: dfe 8~24*/
                        qam_write_byte(u32TunerPort, EQU_CTRL_11_ADDR, 0x70);
                        qam_write_byte(u32TunerPort, EQU_CTRL_11_ADDR, 0x80);
                        qam_write_byte(u32TunerPort, EQU_CTRL_11_ADDR, 0x90);
                        qam_write_byte(u32TunerPort, EQU_CTRL_11_ADDR, 0xa0);

                        qam_write_byte(u32TunerPort, MCTRL_1_ADDR, 0x08);
                        qam_write_byte(u32TunerPort, MCTRL_1_ADDR, 0x00);

                        bStartFlag = HI_FALSE;
                        s32Cnt = 0;

                        u8IsReset = 1;
                    }

                    if (bStartFlag)
                    {
                        s32Cnt++;
                    }

                    if ((20 <= s32Cnt) && (u8Tmp >= 0x3))
                    {
                        u8IsReset = 0;
                    }
                }

                msleep_interruptible( J83B_CONNECT_CHECK_FREQ );
                u32Timeout = u32Timeout + J83B_CONNECT_CHECK_FREQ;
            }
        } /*while*/
    }

    if (u32EquResetTime > J83B_CLOSE_EQU_TIME)
    {
        qam_write_byte(u32TunerPort, EQU_CTRL_11_ADDR, 0x7f);
        qam_write_byte(u32TunerPort, EQU_CTRL_11_ADDR, 0x8f);
        qam_write_byte(u32TunerPort, EQU_CTRL_11_ADDR, 0x9f);
        qam_write_byte(u32TunerPort, EQU_CTRL_11_ADDR, 0xaf);
    }
    return;
}

static HI_U32 s_u32J83bConnectTimeout = J83B_CONNCECT_TIMEOUT;
HI_VOID j83b_connect_timeout(HI_U32 u32ConnectTimeout)
{
    s_u32J83bConnectTimeout = u32ConnectTimeout;
    return;
}
HI_S32 j83b_connect(HI_U32 u32TunerPort, TUNER_ACC_QAM_PARAMS_S *pstChannel)
{
    HI_S32 s32FuncRet = HI_SUCCESS;
    HI_U32 u32SymbolRate = 0;

    reset_special_process_flag(HI_FALSE);

    HI_ASSERT(HI_NULL != pstChannel);
    X5HDQAMV200_I2C_CHECK(u32TunerPort);

    //memcpy( &g_curr_para[u32TunerPort], stChannel, sizeof( TUNER_ACC_QAM_PARAMS_S ) );

    g_stTunerOps[u32TunerPort].u32CurrQamMode = pstChannel->enQamType;
    u32SymbolRate = pstChannel->unSRBW.u32SymbolRate / 1000; /* KHz */

    /*step 1:config tuner register*/
    s32FuncRet = (*g_stTunerOps[u32TunerPort].set_tuner)(u32TunerPort, g_stTunerOps[u32TunerPort].enI2cChannel, pstChannel->u32Frequency);
    if (HI_SUCCESS != s32FuncRet)
    {
        HI_ERR_TUNER( "set tuner error\n");
        return s32FuncRet;
    }

    /*step 2:config qam register */
    j83b_set_reg_value(u32TunerPort);    /*set for qam and tunner*/

    /*set for different usr request about symbolrate, ts type, demo in or out,  inversion or not*/
    j83b_set_customreg_value(u32TunerPort, pstChannel->bSI, u32SymbolRate);

    /*step 3:reset chip*/
    j83b_chip_reset(u32TunerPort, ALL_CHIP);

    if(s_u32J83bConnectTimeout < J83B_CLOSE_EQU_TIME)
    {
        s_u32J83bConnectTimeout = J83B_CONNCECT_TIMEOUT;
        return HI_SUCCESS;
    }

    if (g_stTunerOps[u32TunerPort].manage_after_chipreset)
        g_stTunerOps[u32TunerPort].manage_after_chipreset(u32TunerPort);

    return HI_SUCCESS;
}


HI_S32 j83b_get_signal_strength(HI_U32 u32TunerPort, HI_U32 *pu32SignalStrength)
{
    HI_U8  au8RegVal[2] = {0};
    HI_U32 u32TmpSigStrength = 0;
    HI_U32 i;

    HI_ASSERT(HI_NULL != pu32SignalStrength);
    X5HDQAMV200_I2C_CHECK(u32TunerPort);


    for ( i = 0; i < 64; i++ )
    {
        qam_write_byte(u32TunerPort, CR_CTRL_21_ADDR, 0x1);

        qam_read_byte(u32TunerPort, BAGC_STAT_3_ADDR, &au8RegVal[0]);

        qam_read_byte(u32TunerPort, BAGC_STAT_4_ADDR, &au8RegVal[1]);

        pu32SignalStrength[1] = ( au8RegVal[0]  << 4 ) | ( (au8RegVal[1] >> 4) & 0x0f );

        u32TmpSigStrength += pu32SignalStrength[1];

    }

    u32TmpSigStrength =     u32TmpSigStrength >> 6;
    pu32SignalStrength[1] = u32TmpSigStrength;

    if(g_stTunerOps[u32TunerPort].recalculate_signal_strength)
    {
        g_stTunerOps[u32TunerPort].recalculate_signal_strength(u32TunerPort, pu32SignalStrength);
    }
    return HI_SUCCESS;
}

HI_S32 j83b_get_ber(HI_U32 u32TunerPort, HI_U32 *pu32ber)
{
    HI_U32 u32BerThres = 4; /* modify by chenxu 2008-01-15 AI7D02092 */
    HI_U8  u8Temp   = 0;
    HI_U8  u8Val = 0;
    HI_S32 i = 0;

    HI_ASSERT(HI_NULL != pu32ber);
    X5HDQAMV200_I2C_CHECK(u32TunerPort);

    qam_write_bit(u32TunerPort, BER_1_ADDR, 7, 0); /*disable*/

    u8Val = (u32BerThres) << 2;
    qam_write_byte( u32TunerPort, BER_1_ADDR, u8Val );

    qam_write_bit(u32TunerPort, BER_1_ADDR, 7, 1); /*enable*/

    for (i = 0; i < 40; i++)
    {
        msleep(10);
        qam_read_bit(u32TunerPort, BER_1_ADDR, 7, &u8Temp);
        if (!u8Temp)
        {
            break;
        }
    }

    qam_write_byte(u32TunerPort, CR_CTRL_21_ADDR, 1);

    if (i >= 40)
    {
        return HI_FAILURE;
    }

    qam_read_byte(u32TunerPort, BER_2_ADDR, &u8Val);
    pu32ber[0] = u8Val;
    qam_read_byte(u32TunerPort, BER_3_ADDR, &u8Val);
    pu32ber[1] = u8Val;
    qam_read_byte(u32TunerPort, BER_4_ADDR, &u8Val);
    pu32ber[2] = u8Val;

    return HI_SUCCESS;
}

HI_S32 j83b_get_rs(HI_U32 u32TunerPort, HI_U32 *pu32Rs)
{
    HI_U32 u32BerThres = 4; /* modify by chenxu 2008-01-15 AI7D02092 */
    HI_U8  u8Temp    = 0;
    HI_U8  u8Val     = 0;
    HI_S32 i         = 0;

    HI_U8  u8Addr      = CR_CTRL_21_ADDR;
    HI_U8  u8RegValue  = 1;
    HI_U32 u8RegValue1 = 0;
    HI_U32 u8RegValue2 = 0;
    HI_U32 u8RegValue3 = 0;

    X5HDQAMV200_I2C_CHECK(u32TunerPort);

    qam_write_bit(u32TunerPort, BER_1_ADDR, 7, 0); /*disable*/

    u8Val = (u32BerThres) << 2;
    qam_write_byte( u32TunerPort, BER_1_ADDR, u8Val );

    qam_write_bit(u32TunerPort, BER_1_ADDR, 7, 1); /*enable*/

    for (i = 0; i < 40; i++)
    {
        msleep(10);
        qam_read_bit(u32TunerPort, BER_1_ADDR, 7, &u8Temp);
        if (!u8Temp)
        {
            break;
        }
    }

    qam_write_byte(u32TunerPort, CR_CTRL_21_ADDR, 1);

    if (i >= 40)
    {
        return HI_FAILURE;
    }

    /* rs package,calculate RS error  package*/
    qam_write_byte(u32TunerPort, u8Addr,u8RegValue);

    qam_read_byte(u32TunerPort, RS_CTRL_1_ADDR, &u8Temp);
    qam_read_byte(u32TunerPort, RS_CTRL_2_ADDR, &u8Val);
    u8RegValue1 = ( u8Temp << 8 ) | u8Val;

    qam_read_byte(u32TunerPort, RS_CTRL_3_ADDR, &u8Temp);
    qam_read_byte(u32TunerPort, RS_CTRL_4_ADDR, &u8Val);
    u8RegValue2 = ( u8Temp << 8 ) | u8Val;

    qam_read_byte(u32TunerPort, RS_CTRL_5_ADDR, &u8Temp);
    qam_read_byte(u32TunerPort, RS_CTRL_6_ADDR, &u8Val);
    u8RegValue3 = ( u8Temp << 8 ) | u8Val;

    qam_read_byte( u32TunerPort, RS_CTRL_8_ADDR, &u8RegValue );

    pu32Rs[0] = u8RegValue1;
    pu32Rs[1] = u8RegValue2;
    pu32Rs[2] = u8RegValue3;

    return HI_SUCCESS;
}


HI_S32 j83b_get_snr(HI_U32 u32TunerPort, HI_U32* pu32SNR)
{
    HI_U32 u32Snr = 0;
    HI_U8  u8RdVal = 0;
    HI_U32 i ;
    HI_U32 u32SumSnr = 0;

    HI_ASSERT(HI_NULL != pu32SNR);
    X5HDQAMV200_I2C_CHECK(u32TunerPort);

    for (i=0;i<64;i++)
    {
        qam_write_byte(u32TunerPort, CR_CTRL_21_ADDR, 1);

        qam_read_byte(u32TunerPort, EQU_STAT_2_ADDR, &u8RdVal);
        u32Snr = u8RdVal << 8;
        qam_read_byte(u32TunerPort, EQU_STAT_3_ADDR, &u8RdVal);
        u32Snr  += u8RdVal;
        u32SumSnr += u32Snr;
    }

    u32Snr = u32SumSnr >> 6;
    *pu32SNR = u32Snr;

    return HI_SUCCESS;
}

HI_S32 j83b_set_ts_type(HI_U32 u32TunerPort, HI_UNF_TUNER_OUPUT_MODE_E enTsType)
{
    g_stTunerOps[u32TunerPort].enTsType = enTsType;

    return HI_SUCCESS;
}


HI_S32 j83b_get_freq_symb_offset(HI_U32 u32TunerPort, HI_U32 * pu32Freq, HI_U32 * pu32Symb )
{
    HI_U8  au8Freq[4] = { 0 };
    HI_S32 s32RealFreq = 0;
    HI_U8  au8Symb[2] = { 0 };
    HI_S32 s32RealSymb = 0;

    HI_ASSERT(HI_NULL != pu32Freq);
    HI_ASSERT(HI_NULL != pu32Symb);
    X5HDQAMV200_I2C_CHECK(u32TunerPort);

    qam_write_byte(u32TunerPort, CR_CTRL_21_ADDR, 0x1);

    /* calc freq */
    qam_read_byte(u32TunerPort, CR_STAT_4_ADDR, &au8Freq[0]);
    au8Freq[0] = au8Freq[0] & 0x0F;

    qam_read_byte(u32TunerPort, CR_STAT_5_ADDR, &au8Freq[1]);
    qam_read_byte(u32TunerPort, CR_STAT_6_ADDR, &au8Freq[2]);
    qam_read_byte(u32TunerPort, CR_STAT_7_ADDR, &au8Freq[3]);

    s32RealFreq = (au8Freq[0]<<24) + (au8Freq[1]<<16) + (au8Freq[2]<<8) + au8Freq[3] ;

    *pu32Freq = s32RealFreq;

    /* calc symbolrate */
    qam_read_byte(u32TunerPort, TR_STAT_1_ADDR, &au8Symb[0]);

    qam_read_byte(u32TunerPort, TR_STAT_2_ADDR, &au8Symb[1]);

    s32RealSymb = au8Symb[0]*256 + au8Symb[1];

    *pu32Symb = s32RealSymb;

    return HI_SUCCESS;
}