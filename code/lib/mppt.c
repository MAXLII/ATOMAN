/**
 ******************************************************************************
 * @file           : xxx_module.c
 * @brief          : 双扰动MPPT追踪主流程，
 * @author         : ss
 * @date           : 2025-10-18
 * @version        : v1.0.0
 * @par 参考文件:
 *      《双扰动MPPT算法研究》
 *
 * @par 修改历史:
 * | 版本号 | 日期       | 作者     | 说明                |
 * | :-----: | :--------- | :-------- | :------------------ |
 * | v1.0.0  | 2025-10-18 | ss       | 初版创建            |
 ******************************************************************************
 **/

#include "mppt.h"
#include "math.h"

/*****************************************************************************
 * @函数名   : PvInputPowerCalculation
 * @功能     : 双扰动MPPT追踪主流程
 * @参数     : self - MPPT配置参数结构体指针
 * @返回值   : 无
 * @说明     :
 *****************************************************************************/
void MpptProcess(mppt_cfg_para_t *self)
{
    if ((self->setMpptRef == NULL) ||
        (self->getMpptRef == NULL) ||
        (self->getMpptVoltFdk == NULL) ||
        (self->getMpptPwrFdk == NULL))
        return;
    float currStepDeltaVolt;
    float currStepVolt;
    int16_t curStepDir;
    float deltaPwrStep1;
    float deltaPwrStep2;
    float curVoltFdb;
    if (self->mpptEnableFlg == 0 || self->mpptPauseFlg == 0)
    {
        return;
    }
    if (++(self->mpptTimeCnt) < self->mpptTimeThres)
    { // mpptTimeThres * 任务时间，进入一次MPPT
        return;
    }
    self->mpptTimeCnt = 0;
    curVoltFdb = self->getMpptVoltFdk();
    switch (self->mpptSubStep)
    {
    case MPPT_DISTURB:
        currStepVolt = self->getMpptRef();
        /* 电压跟踪不上对应的参考和反馈差值阈值，需要考虑采样误差，各项目自定，默认值可为2.0f */
        if (fabsf(curVoltFdb - currStepVolt) > self->mpptLoseCtrVoltThres)
        {
            /* 电压不受控超过一定时间给定当前电压作为参考 */
            if ((self->mpptLoseCtrTimeCnt)++ > self->mpptLoseCtrTimeThres)
            {
                self->setMpptRef(curVoltFdb - self->fastStepDeltaVolt);
                self->mpptDir = MPPT_DIR_DECREASE;
            }
            self->mpptSubStep = MPPT_DISTURB;
            break;
        }
        else
        {
            self->mpptLoseCtrTimeCnt = 0;
        }
        self->pwrStep1 = self->getMpptPwrFdk();
        curStepDir = self->mpptDir;
        currStepDeltaVolt = curStepDir * self->stepDeltaVolt;
        currStepVolt += currStepDeltaVolt;
        self->setMpptRef(currStepVolt);
        self->mpptSubStep = MPPT_REV_DISTURB;
        break;

    case MPPT_REV_DISTURB:
        self->pwrStep2 = self->getMpptPwrFdk();
        curStepDir = -self->mpptDir;
        currStepVolt = self->getMpptRef();
        currStepDeltaVolt = curStepDir * self->stepDeltaVolt;
        currStepVolt += currStepDeltaVolt;
        self->setMpptRef(currStepVolt);
        self->mpptSubStep = MPPT_DIR_DECIDE;
        break;
    case MPPT_DIR_DECIDE:
        self->pwrStep3 = self->getMpptPwrFdk();
        deltaPwrStep1 = self->pwrStep2 - self->pwrStep1;
        deltaPwrStep2 = self->pwrStep3 - self->pwrStep2;
        if (deltaPwrStep1 > deltaPwrStep2)
        { // 初始扰动方向正确，往初始方向扰动回去
            curStepDir = self->mpptDir;
            currStepVolt = self->getMpptRef();
            currStepDeltaVolt = curStepDir * self->stepDeltaVolt;
            currStepVolt += currStepDeltaVolt;
            self->setMpptRef(currStepVolt);
            if ((deltaPwrStep1 - deltaPwrStep2) < self->slowStepVoltThr)
            {
                self->stepDeltaVolt = self->slowStepDeltaVolt;
            }
            else if ((deltaPwrStep1 - deltaPwrStep2) > self->fastStepVoltThr)
            {
                self->stepDeltaVolt = self->fastStepDeltaVolt;
            }
            else
            {
                self->stepDeltaVolt = self->midStepDeltaVolt;
            }
        }
        else
        { // 初始扰动方向错误，则调换方向
            self->mpptDir = -self->mpptDir;
            self->stepDeltaVolt = self->slowStepDeltaVolt;
        }
        self->mpptSubStep = MPPT_DISTURB;
        break;
    default:
        self->mpptSubStep = MPPT_DISTURB;
        break;
    }
}

/*****************************************************************************
 * @函数名   : MpptEnable
 * @功能     : 使能MPPT追踪
 * @参数     : self - MPPT配置参数结构体指针
 * @返回值   : 无
 * @说明     :
 *****************************************************************************/
void MpptEnable(mppt_cfg_para_t *self)
{
    self->mpptDir = MPPT_DIR_DECREASE;
    self->mpptEnableFlg = 1;
    self->mpptPauseFlg = 1;
    self->mpptSubStep = MPPT_DISTURB;
    /* 刚开机时从0.8Voc追踪 */
    self->setMpptRef(self->mpptStartVolt);
}

/*****************************************************************************
 * @函数名   : MpptDisable
 * @功能     : 禁能MPPT追踪，用于故障处理
 * @参数     : self - MPPT配置参数结构体指针
 * @返回值   : 无
 * @说明     :
 *****************************************************************************/
void MpptDisable(mppt_cfg_para_t *self)
{
    self->mpptStartVolt = 0.0f;
    self->voltRef = 0.0f;
    self->pwrStep1 = 0.0f;
    self->pwrStep2 = 0.0f;
    self->pwrStep3 = 0.0f;
    self->mpptVoc = 0.0f;
    self->mpptVoltUplimit = 0.0f;
    self->mpptVoltDnlimit = 0.0f;
    self->mpptEnableFlg = 0;
    self->mpptPauseFlg = 0;
    self->mpptLoseCtrTimeCnt = 0;
    self->mpptTimeCnt = 0;
}

/*****************************************************************************
 * @函数名   : MpptPause
 * @功能     : 暂停MPPT追踪，用于短时处理
 * @参数     : self - MPPT配置参数结构体指针
 * @返回值   : 无
 * @说明     :
 *****************************************************************************/
void MpptPause(mppt_cfg_para_t *self)
{
    self->mpptPauseFlg = 0;
}

/*****************************************************************************
 * @函数名   : MpptResume
 * @功能     : 用于暂停MPPT追踪时的唤醒
 * @参数     : self - MPPT配置参数结构体指针
 * @返回值   : 无
 * @说明     :
 *****************************************************************************/
void MpptResume(mppt_cfg_para_t *self)
{
    self->mpptPauseFlg = 1;
    self->mpptSubStep = MPPT_DISTURB;
}

/*****************************************************************************
 * @函数名   : SetMpptVoltRef
 * @功能     : 设置MPPT电压参考
 * @参数     : self - MPPT配置参数结构体指针
 * @返回值   : 无
 * @说明     :
 *****************************************************************************/
void SetMpptVoltRef(mppt_cfg_para_t *self, float voltRef)
{
    self->voltRef = voltRef;
    CLAMP(self->voltRef, self->mpptVoltDnlimit, self->mpptVoltUplimit);
}

/*****************************************************************************
 * @函数名   : GetMpptVoltRef
 * @功能     : 获取MPPT电压参考
 * @参数     : self - MPPT配置参数结构体指针
 * @返回值   : 无
 * @说明     :
 *****************************************************************************/
float GetMpptVoltRef(mppt_cfg_para_t *self)
{
    return self->voltRef;
}

/*****************************************************************************
 * @函数名   : SetMpptVoc
 * @功能     : 设置MPPT开路电压
 * @参数     : self - MPPT配置参数结构体指针
 * @返回值   : 无
 * @说明     :
 *****************************************************************************/
void SetMpptVoc(mppt_cfg_para_t *self, float voltRef)
{
    self->mpptVoc = voltRef;
}

/*****************************************************************************
 * @函数名   : SetMpptUpLimitVolt
 * @功能     : 设置MPPT电压上限
 * @参数     : self - MPPT配置参数结构体指针
 * @返回值   : 无
 * @说明     :
 *****************************************************************************/
void SetMpptUpLimitVolt(mppt_cfg_para_t *self, float volt)
{
    self->mpptVoltUplimit = volt;
}

/*****************************************************************************
 * @函数名   : SetMpptDnLimitVolt
 * @功能     : 设置MPPT电压下限
 * @参数     : self - MPPT配置参数结构体指针
 * @返回值   : 无
 * @说明     :
 *****************************************************************************/
void SetMpptDnLimitVolt(mppt_cfg_para_t *self, float volt)
{
    self->mpptVoltDnlimit = volt;
}

/*****************************************************************************
 * @函数名   : SetMpptStartVolt
 * @功能     : 设置MPPT追踪起始电压
 * @参数     : self - MPPT配置参数结构体指针
 * @返回值   : 无
 * @说明     :
 *****************************************************************************/
void SetMpptStartVolt(mppt_cfg_para_t *self, float voltRef)
{
    self->mpptStartVolt = voltRef;
    CLAMP(self->mpptStartVolt, MPPT_VOLT_MIN_COFF * self->mpptVoc, MPPT_VOLT_MAX_COFF * self->mpptVoc);
    CLAMP(self->mpptStartVolt, self->mpptVoltDnlimit, self->mpptVoltUplimit);
}
