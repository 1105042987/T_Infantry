#include "framework_drivers_uartremotecontrol_iopool.h"
#include "framework_drivers_uartremotecontrol_task.h"
#include "framework_drivers_uartremotecontrol.h"
#include "framework_drivers_led.h"
#include "usart.h"
#include "ramp.h"
#include "ControlTask.h"
/*****Begin define ioPool*****/
#define DataPoolInit {0}
#define ReadPoolSize 1
#define ReadPoolMap {0}
#define GetIdFunc 0 
#define ReadPoolInit {0, Empty, 1}

IOPoolDefine(rcUartIOPool, DataPoolInit, ReadPoolSize, ReadPoolMap, GetIdFunc, ReadPoolInit);
	
#undef DataPoolInit 
#undef ReadPoolSize 
#undef ReadPoolMap
#undef GetIdFunc
#undef ReadPoolInit
/*****End define ioPool*****/

void rcInit(){
	//ң����DMA���տ���(һ�ν���18���ֽ�)
	if(HAL_UART_Receive_DMA(&rcUart, IOPool_pGetWriteData(rcUartIOPool)->ch, 18) != HAL_OK){
			Error_Handler();
	} 
}

void rcUartRxCpltCallback(){
	//osStatus osMessagePut (osMessageQId queue_id, uint32_t info, uint32_t millisec);
	IOPool_getNextWrite(rcUartIOPool);
	HAL_UART_Receive_DMA(&rcUart, IOPool_pGetWriteData(rcUartIOPool)->ch, 18);
}

RC_Ctl_t RC_CtrlData;   //remote control data
ChassisSpeed_Ref_t ChassisSpeedRef;
Gimbal_Ref_t GimbalRef;
 FrictionWheelState_e friction_wheel_state = FRICTION_WHEEL_OFF;
static RemoteSwitch_t switch1;   //ң������ದ��
static volatile Shoot_State_e shootState = NOSHOOTING;
static InputMode_e inputmode = REMOTE_INPUT;   //����ģʽ�趨

RampGen_t frictionRamp = RAMP_GEN_DAFAULT;  //Ħ����б��
RampGen_t LRSpeedRamp = RAMP_GEN_DAFAULT;   //mouse�����ƶ�б��
RampGen_t FBSpeedRamp = RAMP_GEN_DAFAULT;   //mouseǰ���ƶ�б��
 
void GetRemoteSwitchAction(RemoteSwitch_t *sw, uint8_t val)
{
	static uint32_t switch_cnt = 0;

	/* ����״ֵ̬ */
	sw->switch_value_raw = val;
	sw->switch_value_buf[sw->buf_index] = sw->switch_value_raw;

	/* ȡ����ֵ����һ��ֵ */
	sw->switch_value1 = (sw->switch_value_buf[sw->buf_last_index] << 2)|
	(sw->switch_value_buf[sw->buf_index]);


	/* ���ϵ�״ֵ̬������ */
	sw->buf_end_index = (sw->buf_index + 1)%REMOTE_SWITCH_VALUE_BUF_DEEP;

	/* �ϲ�����ֵ */
	sw->switch_value2 = (sw->switch_value_buf[sw->buf_end_index]<<4)|sw->switch_value1;	

	/* �����ж� */
	if(sw->switch_value_buf[sw->buf_index] == sw->switch_value_buf[sw->buf_last_index])
	{
		switch_cnt++;	
	}
	else
	{
		switch_cnt = 0;
	}

	if(switch_cnt >= 40)
	{
		sw->switch_long_value = sw->switch_value_buf[sw->buf_index]; 	
	}

	//����ѭ��
	sw->buf_last_index = sw->buf_index;
	sw->buf_index++;		
	if(sw->buf_index == REMOTE_SWITCH_VALUE_BUF_DEEP)
	{
		sw->buf_index = 0;	
	}			
}
//return the state of the remote 0:no action 1:action 
uint8_t IsRemoteBeingAction(void)
{
	return (abs(ChassisSpeedRef.forward_back_ref)>=10 || abs(ChassisSpeedRef.left_right_ref)>=10 || fabs(GimbalRef.yaw_speed_ref)>=10 || fabs(GimbalRef.pitch_speed_ref)>=10);
}

//����ģʽ���� 
void SetInputMode(Remote *rc)
{
	if(rc->s2 == 1)
	{
		inputmode = REMOTE_INPUT;
	}
	else if(rc->s2 == 3)
	{
		inputmode = KEY_MOUSE_INPUT;
	}
	else if(rc->s2 == 2)
	{
		inputmode = STOP;
	}	
}

InputMode_e GetInputMode()
{
	return inputmode;
}

/*
input: RemoteSwitch_t *sw, include the switch info
*/
void RemoteShootControl(RemoteSwitch_t *sw, uint8_t val) 
{
	GetRemoteSwitchAction(sw, val);
	switch(friction_wheel_state)
	{
		case FRICTION_WHEEL_OFF:
		{
			if(sw->switch_value1 == REMOTE_SWITCH_CHANGE_3TO1)   //�ӹرյ�start turning
			{
				SetShootState(NOSHOOTING);
				frictionRamp.ResetCounter(&frictionRamp);
				friction_wheel_state = FRICTION_WHEEL_START_TURNNING;	 
	//			LASER_ON(); 
			}				 		
		}break;
		case FRICTION_WHEEL_START_TURNNING:
		{
			if(sw->switch_value1 == REMOTE_SWITCH_CHANGE_3TO1)   //�������ͱ��ر�
			{
//				LASER_OFF();
				SetShootState(NOSHOOTING);
//				SetFrictionWheelSpeed(1000);
				friction_wheel_state = FRICTION_WHEEL_OFF;
				frictionRamp.ResetCounter(&frictionRamp);
			}
			else
			{
				//Ħ���ּ���
				
//				SetFrictionWheelSpeed(1000 + (FRICTION_WHEEL_MAX_DUTY-1000)*frictionRamp.Calc(&frictionRamp)); 
				if(frictionRamp.IsOverflow(&frictionRamp))
				{
					friction_wheel_state = FRICTION_WHEEL_ON; 	
				}
				
			}
		}break;
		case FRICTION_WHEEL_ON:
		{
			if(sw->switch_value1 == REMOTE_SWITCH_CHANGE_3TO1)   //�ر�Ħ����
			{
//				LASER_OFF();
				friction_wheel_state = FRICTION_WHEEL_OFF;				  
//				SetFrictionWheelSpeed(1000); 
				frictionRamp.ResetCounter(&frictionRamp);
				SetShootState(NOSHOOTING);
			}
			else if(sw->switch_value_raw == 2)
			{
				SetShootState(SHOOTING);
			}
			else
			{
				SetShootState(NOSHOOTING);
			}					 
		} break;				
	}
}
	 
void MouseShootControl(Mouse *mouse)
{
	int16_t closeDelayCount = 0;   //�Ҽ��ر�Ħ����3s��ʱ����
	switch(friction_wheel_state)
	{
		case FRICTION_WHEEL_OFF:
		{
			if(mouse->last_press_r == 0 && mouse->press_r == 1)   //�ӹرյ�start turning
			{
				SetShootState(NOSHOOTING);
				frictionRamp.ResetCounter(&frictionRamp);
				friction_wheel_state = FRICTION_WHEEL_START_TURNNING;	 
//				LASER_ON(); 
				closeDelayCount = 0;
			}				 		
		}break;
		case FRICTION_WHEEL_START_TURNNING:
		{
			if(mouse->press_r == 1)
			{
				closeDelayCount++;
			}
			else
			{
				closeDelayCount = 0;
			}
			if(closeDelayCount>50)   //�ر�Ħ����
			{
//				LASER_OFF();
				friction_wheel_state = FRICTION_WHEEL_OFF;				  
//				SetFrictionWheelSpeed(1000); 
				frictionRamp.ResetCounter(&frictionRamp);
				SetShootState(NOSHOOTING);
			}
			else
			{
				//Ħ���ּ���				
//				SetFrictionWheelSpeed(1000 + (FRICTION_WHEEL_MAX_DUTY-1000)*frictionRamp.Calc(&frictionRamp)); 
				if(frictionRamp.IsOverflow(&frictionRamp))
				{
					friction_wheel_state = FRICTION_WHEEL_ON; 	
				}
				
			}
		}break;
		case FRICTION_WHEEL_ON:
		{
			if(mouse->press_r == 1)
			{
				closeDelayCount++;
			}
			else
			{
				closeDelayCount = 0;
			}
			
			if(closeDelayCount>50)   //�ر�Ħ����
			{
//				LASER_OFF();
				friction_wheel_state = FRICTION_WHEEL_OFF;				  
//				SetFrictionWheelSpeed(1000); 
				frictionRamp.ResetCounter(&frictionRamp);
				SetShootState(NOSHOOTING);
			}			
			else if(mouse->press_l== 1)  //������������
			{
				SetShootState(SHOOTING);				
			}
			else
			{
				SetShootState(NOSHOOTING);				
			}					 
		} break;				
	}	
	mouse->last_press_r = mouse->press_r;
}



//����������ģʽ����
void MouseKeyControlProcess(Mouse *mouse, Key *key)
{
	static uint16_t forward_back_speed = 0;
	static uint16_t left_right_speed = 0;
    if(GetWorkState()!=PREPARE_STATE)
    {
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
	#define MINMAX(value, min, max) value = (value < min) ? min : (value > max ? max : value)
    if(GetWorkState() == NORMAL_STATE)
    {
		MINMAX(mouse->x, -150, 150); 
		MINMAX(mouse->y, -150, 150); 
		
        GimbalRef.pitch_angle_dynamic_ref -= mouse->y* MOUSE_TO_PITCH_ANGLE_INC_FACT;  //(rc->ch3 - (int16_t)REMOTE_CONTROLLER_STICK_OFFSET) * STICK_TO_PITCH_ANGLE_INC_FACT;
        GimbalRef.yaw_angle_dynamic_ref   += mouse->x* MOUSE_TO_YAW_ANGLE_INC_FACT;

	}
	
	/* not used to control, just as a flag */ 
    GimbalRef.pitch_speed_ref = mouse->y;    //speed_ref�����������ж���
    GimbalRef.yaw_speed_ref   = mouse->x;
	GimbalAngleLimit();	
	MouseShootControl(mouse);
	
}

//
Shoot_State_e GetShootState()
{
	return shootState;
}

void SetShootState(Shoot_State_e v)
{
	shootState = v;
}

FrictionWheelState_e GetFrictionState()
{
	return friction_wheel_state;
}

void SetFrictionState(FrictionWheelState_e v)
{
	friction_wheel_state = v;
}
//ң��������ֵ���ã�
void GimbalAngleLimit()
{
	MINMAX(GimbalRef.pitch_angle_dynamic_ref, -PITCH_MAX+7, PITCH_MAX);
//	MINMAX(GimbalRef.yaw_angle_dynamic_ref, GMYPositionPID.fdb - 60, GMYPositionPID.fdb + 60);
}

//ң�������ݳ�ʼ����б�º����ȵĳ�ʼ��
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
