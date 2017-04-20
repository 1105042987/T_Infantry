#include "framework_tasks_remotecontrol.h"
#include "framework_drivers_uartremotecontrol.h"
#include "framework_drivers_uartremotecontrol_iopool.h"
#include "framework_drivers_led.h"
#include "framework_utilities_debug.h"
#include "stdint.h"
#include "stddef.h"
#include "ramp.h"
#include "framework_tasks_cmcontrol.h"

#define VAL_LIMIT(val, min, max)\
if(val<=min)\
{\
	val = min;\
}\
else if(val>=max)\
{\
	val = max;\
}\

extern ChassisSpeed_Ref_t ChassisSpeedRef;
extern Gimbal_Ref_t GimbalRef;
extern FrictionWheelState_e friction_wheel_state ;
static RemoteSwitch_t switch1;   //ң������ದ��

extern RampGen_t frictionRamp ;  //Ħ����б��
extern RampGen_t LRSpeedRamp ;   //mouse�����ƶ�б��
extern RampGen_t FBSpeedRamp  ;   //mouseǰ���ƶ�б��

extern RC_Ctl_t RC_CtrlData; 
extern xSemaphoreHandle xSemaphore_rcuart;
extern float yawAngleTarget, pitchAngleTarget;
void printRcTask(void const * argument){
	uint8_t data[18];
			static int countwhile = 0;
	while(1){
		xSemaphoreTake(xSemaphore_rcuart, osWaitForever);
		if(IOPool_hasNextRead(rcUartIOPool, 0)){
			IOPool_getNextRead(rcUartIOPool, 0);
	
			uint8_t *pData = IOPool_pGetReadData(rcUartIOPool, 0)->ch;
			for(uint8_t i = 0; i != 18; ++i){
				data[i] = pData[i];
			}
		
			RemoteDataProcess(data);
			
			if(countwhile >= 300){
			countwhile = 0;
			fw_printf("ch0 = %d | ", RC_CtrlData.rc.ch0);
				fw_printf("ch1 = %d | ", RC_CtrlData.rc.ch1);
				fw_printf("ch2 = %d | ", RC_CtrlData.rc.ch2);
				fw_printf("ch3 = %d \r\n", RC_CtrlData.rc.ch3);
				
				fw_printf("s1 = %d | ", RC_CtrlData.rc.s1);
				fw_printf("s2 = %d \r\n", RC_CtrlData.rc.s2);
				
				fw_printf("x = %d | ", RC_CtrlData.mouse.x);
				fw_printf("y = %d | ", RC_CtrlData.mouse.y);
				fw_printf("z = %d | ", RC_CtrlData.mouse.z);
				fw_printf("l = %d | ", RC_CtrlData.mouse.press_l);
				fw_printf("r = %d \r\n", RC_CtrlData.mouse.press_r);
				
				fw_printf("key = %d \r\n", RC_CtrlData.key.v);
				fw_printf("===========\r\n");
		}else{
			countwhile++;
		}
		}
	}
}

void RemoteTaskInit()
{
	//б�³�ʼ��
	frictionRamp.SetScale(&frictionRamp, FRICTION_RAMP_TICK_COUNT);
	LRSpeedRamp.SetScale(&LRSpeedRamp, MOUSE_LR_RAMP_TICK_COUNT);
	FBSpeedRamp.SetScale(&FBSpeedRamp, MOUSR_FB_RAMP_TICK_COUNT);
	frictionRamp.ResetCounter(&frictionRamp);
	LRSpeedRamp.ResetCounter(&LRSpeedRamp);
	FBSpeedRamp.ResetCounter(&FBSpeedRamp);
	//������̨����ֵ��ʼ��
	GimbalRef.pitch_angle_dynamic_ref = 0.0f;
	GimbalRef.yaw_angle_dynamic_ref = 0.0f;
	ChassisSpeedRef.forward_back_ref = 0.0f;
	ChassisSpeedRef.left_right_ref = 0.0f;
	ChassisSpeedRef.rotate_ref = 0.0f;
	//Ħ��������״̬��ʼ��
	SetFrictionState(FRICTION_WHEEL_OFF);
}


void RemoteDataProcess(uint8_t *pData)
{
    if(pData == NULL)
    {
        return;
    }
    RC_CtrlData.rc.ch0 = ((int16_t)pData[0] | ((int16_t)pData[1] << 8)) & 0x07FF; 
    RC_CtrlData.rc.ch1 = (((int16_t)pData[1] >> 3) | ((int16_t)pData[2] << 5)) & 0x07FF;
    RC_CtrlData.rc.ch2 = (((int16_t)pData[2] >> 6) | ((int16_t)pData[3] << 2) |
                         ((int16_t)pData[4] << 10)) & 0x07FF;
    RC_CtrlData.rc.ch3 = (((int16_t)pData[4] >> 1) | ((int16_t)pData[5]<<7)) & 0x07FF;
    
    RC_CtrlData.rc.s1 = ((pData[5] >> 4) & 0x000C) >> 2;
    RC_CtrlData.rc.s2 = ((pData[5] >> 4) & 0x0003);

    RC_CtrlData.mouse.x = ((int16_t)pData[6]) | ((int16_t)pData[7] << 8);
    RC_CtrlData.mouse.y = ((int16_t)pData[8]) | ((int16_t)pData[9] << 8);
    RC_CtrlData.mouse.z = ((int16_t)pData[10]) | ((int16_t)pData[11] << 8);    

    RC_CtrlData.mouse.press_l = pData[12];
    RC_CtrlData.mouse.press_r = pData[13];
 
    RC_CtrlData.key.v = ((int16_t)pData[14]);// | ((int16_t)pData[15] << 8);
		
		SetInputMode(&RC_CtrlData.rc);
	
	switch(GetInputMode())
	{
		case REMOTE_INPUT:
		{
			//ң��������ģʽ
			RemoteControlProcess(&(RC_CtrlData.rc));
		}break;
		case KEY_MOUSE_INPUT:
		{
			//�����̿���ģʽ
			MouseKeyControlProcess(&RC_CtrlData.mouse,&RC_CtrlData.key);
		}break;
		case STOP:
		{
			//����ͣ��
		}break;
	}
}
//ң��������ģʽ����
void RemoteControlProcess(Remote *rc)
{
    if(GetWorkState()!=PREPARE_STATE)
    {
        ChassisSpeedRef.forward_back_ref = (RC_CtrlData.rc.ch1 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET) * STICK_TO_CHASSIS_SPEED_REF_FACT;
        ChassisSpeedRef.left_right_ref   = (rc->ch0 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET) * STICK_TO_CHASSIS_SPEED_REF_FACT; 
			  pitchAngleTarget += (rc->ch3 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET) * STICK_TO_PITCH_ANGLE_INC_FACT;
        yawAngleTarget   -= (rc->ch2 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET) * STICK_TO_YAW_ANGLE_INC_FACT; 
    }

    if(GetWorkState() == NORMAL_STATE)
    {
        GimbalRef.pitch_angle_dynamic_ref += (rc->ch3 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET) * STICK_TO_PITCH_ANGLE_INC_FACT;
        GimbalRef.yaw_angle_dynamic_ref    += (rc->ch2 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET) * STICK_TO_YAW_ANGLE_INC_FACT;      	
//	      pitchAngleTarget -= (rc->ch3 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET) * STICK_TO_PITCH_ANGLE_INC_FACT;
 //       yawAngleTarget   -= (rc->ch2 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET) * STICK_TO_YAW_ANGLE_INC_FACT; 
		}
	
	/* not used to control, just as a flag */ 
    GimbalRef.pitch_speed_ref = rc->ch3 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET;    //speed_ref�����������ж���
    GimbalRef.yaw_speed_ref   = (rc->ch2 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET);
	GimbalAngleLimit();
	//���-Ħ���֣����̵��״̬
	RemoteShootControl(&switch1, rc->s1);
		

}
//����������ģʽ����

void MouseKeyControlProcess(Mouse *mouse, Key *key)
{
	static uint16_t forward_back_speed = 0;
	static uint16_t left_right_speed = 0;
    if(GetWorkState()!=PREPARE_STATE)
    {
					VAL_LIMIT(mouse->x, -150, 150); 
		VAL_LIMIT(mouse->y, -150, 150); 
		
        pitchAngleTarget -= mouse->y* MOUSE_TO_PITCH_ANGLE_INC_FACT;  //(rc->ch3 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET) * STICK_TO_PITCH_ANGLE_INC_FACT;
        yawAngleTarget    -= mouse->x* MOUSE_TO_YAW_ANGLE_INC_FACT;
		//speed mode: normal speed/high speed
		if(key->v & 0x10)
		{
			forward_back_speed =  HIGH_FORWARD_BACK_SPEED;
			left_right_speed = HIGH_LEFT_RIGHT_SPEED;
		}
		else
		{
			forward_back_speed =  NORMAL_FORWARD_BACK_SPEED;
			left_right_speed = NORMAL_LEFT_RIGHT_SPEED;
		}
		//movement process
		if(key->v & 0x01)  // key: w
		{
			ChassisSpeedRef.forward_back_ref = forward_back_speed* FBSpeedRamp.Calc(&FBSpeedRamp);
		}
		else if(key->v & 0x02) //key: s
		{
			ChassisSpeedRef.forward_back_ref = -forward_back_speed* FBSpeedRamp.Calc(&FBSpeedRamp);
		}
		else
		{
			ChassisSpeedRef.forward_back_ref = 0;
			FBSpeedRamp.ResetCounter(&FBSpeedRamp);
		}
		
		
		if(key->v & 0x04)  // key: d
		{
			ChassisSpeedRef.left_right_ref = -left_right_speed* LRSpeedRamp.Calc(&LRSpeedRamp);
		}
		else if(key->v & 0x08) //key: a
		{
			ChassisSpeedRef.left_right_ref = left_right_speed* LRSpeedRamp.Calc(&LRSpeedRamp);
		}
		else
		{
			ChassisSpeedRef.left_right_ref = 0;
			LRSpeedRamp.ResetCounter(&LRSpeedRamp);
		}
	}
	//step2: gimbal ref calc
 /*   if(GetWorkState() == NORMAL_STATE)
    {
		VAL_LIMIT(mouse->x, -150, 150); 
		VAL_LIMIT(mouse->y, -150, 150); 
		
        pitchAngleTarget -= mouse->y* MOUSE_TO_PITCH_ANGLE_INC_FACT;  //(rc->ch3 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET) * STICK_TO_PITCH_ANGLE_INC_FACT;
        yawAngleTarget    -= mouse->x* MOUSE_TO_YAW_ANGLE_INC_FACT;

	}
	*/
	/* not used to control, just as a flag */ 
    GimbalRef.pitch_speed_ref = mouse->y;    //speed_ref�����������ж���
    GimbalRef.yaw_speed_ref   = mouse->x;
	GimbalAngleLimit();	
	MouseShootControl(mouse);
	
}



