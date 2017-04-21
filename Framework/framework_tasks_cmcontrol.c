#include "framework_tasks_cmcontrol.h"
#include "pid_Regulator.h"
#include "framework_drivers_uartremotecontrol.h"
#include "framework_drivers_motorcan.h"
#include "framework_utilities_debug.h"
#include "tim.h"
#include "stdint.h"

PID_Regulator_t CMRotatePID = CHASSIS_MOTOR_ROTATE_PID_DEFAULT; 
PID_Regulator_t CM1SpeedPID = CHASSIS_MOTOR_SPEED_PID_DEFAULT;
PID_Regulator_t CM2SpeedPID = CHASSIS_MOTOR_SPEED_PID_DEFAULT;
PID_Regulator_t CM3SpeedPID = CHASSIS_MOTOR_SPEED_PID_DEFAULT;
PID_Regulator_t CM4SpeedPID = CHASSIS_MOTOR_SPEED_PID_DEFAULT;
PID_Regulator_t ShootMotorPositionPID = SHOOT_MOTOR_POSITION_PID_DEFAULT;      //shoot motor
PID_Regulator_t ShootMotorSpeedPID = SHOOT_MOTOR_SPEED_PID_DEFAULT;

extern volatile Encoder CM1Encoder;
extern volatile Encoder CM2Encoder;
extern volatile Encoder CM3Encoder;
extern volatile Encoder CM4Encoder;
extern volatile Encoder GMYawEncoder;

WorkState_e workState = PREPARE_STATE;
WorkState_e lastWorkState = PREPARE_STATE;
WorkState_e GetWorkState()
{
	return workState;
}
static uint32_t time_tick_1ms = 0;
void ControtLoopTaskInit(void)
{
	//������ʼ��
	time_tick_1ms = 0;   //�ж��еļ�������
	//���������ʼ��
//	AppParamInit();
	//У׼�����ƫ��ֵ��ʼ��
//	Sensor_Offset_Param_Init(&gAppParamStruct);
	//���ù���ģʽ
//	SetWorkState(PREPARE_STATE);
	//б�³�ʼ��
//	GMPitchRamp.SetScale(&GMPitchRamp, PREPARE_TIME_TICK_MS);
//	GMYawRamp.SetScale(&GMYawRamp, PREPARE_TIME_TICK_MS);
//	GMPitchRamp.ResetCounter(&GMPitchRamp);
//	GMYawRamp.ResetCounter(&GMYawRamp);
	//��̨�����Ƕȳ�ʼ��
//	GimbalRef.pitch_angle_dynamic_ref = 0.0f;
//	GimbalRef.yaw_angle_dynamic_ref = 0.0f;
    //��������ʼ��
	/*
    LostCounterFeed(GetLostCounter(LOST_COUNTER_INDEX_RC));
    LostCounterFeed(GetLostCounter(LOST_COUNTER_INDEX_IMU));
    LostCounterFeed(GetLostCounter(LOST_COUNTER_INDEX_ZGYRO));
    LostCounterFeed(GetLostCounter(LOST_COUNTER_INDEX_MOTOR1));
    LostCounterFeed(GetLostCounter(LOST_COUNTER_INDEX_MOTOR2));
    LostCounterFeed(GetLostCounter(LOST_COUNTER_INDEX_MOTOR3));
    LostCounterFeed(GetLostCounter(LOST_COUNTER_INDEX_MOTOR4));
    LostCounterFeed(GetLostCounter(LOST_COUNTER_INDEX_MOTOR5));
    LostCounterFeed(GetLostCounter(LOST_COUNTER_INDEX_MOTOR6));
    LostCounterFeed(GetLostCounter(LOST_COUNTER_INDEX_DEADLOCK));
    LostCounterFeed(GetLostCounter(LOST_COUNTER_INDEX_NOCALI));
  */  
	//PID��ʼ��
/*	ShootMotorSpeedPID.Reset(&ShootMotorSpeedPID);
	GMPPositionPID.Reset(&GMPPositionPID);
	GMPSpeedPID.Reset(&GMPSpeedPID);
	GMYPositionPID.Reset(&GMYPositionPID);
	GMYSpeedPID.Reset(&GMYSpeedPID);
	*/
	CMRotatePID.Reset(&CMRotatePID);
	CM1SpeedPID.Reset(&CM1SpeedPID);
	CM2SpeedPID.Reset(&CM2SpeedPID);
	CM3SpeedPID.Reset(&CM3SpeedPID);
	CM4SpeedPID.Reset(&CM4SpeedPID);
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
	HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);
}
/*���̿�������*/
extern xSemaphoreHandle motorCanTransmitSemaphore;
void CMControlTask(void const * argument)
{
		portTickType xLastWakeTime;
		xLastWakeTime = xTaskGetTickCount();
		WorkStateFSM();
	  WorkStateSwitchProcess();
	 while(1){       //motor control frequency 4ms
		//�������
//		SuperviseTask();    
		static int countwhile = 0;
		if(countwhile >= 500){
			countwhile = 0;
			fw_printfln("in CMControlTask");
		}
		else{
			countwhile++;
		}
		CMControlLoop();			 
		ShooterMControlLoop();       //���������������
		if( xSemaphoreGive( motorCanTransmitSemaphore ) != pdTRUE )
	 {
			fw_printfln("xemaphoregive error");
	 }
		vTaskDelayUntil( &xLastWakeTime, ( 4 / portTICK_RATE_MS ) );
			
	}
}
	

/**********************************************************
*����״̬�л�״̬��
**********************************************************/

void WorkStateFSM(void)
{
	lastWorkState = workState;
	switch(workState)
	{
		case PREPARE_STATE:
		{
			if(GetInputMode() == STOP )//|| Is_Serious_Error())
			{
				workState = STOP_STATE;
			}
			else if(time_tick_1ms > PREPARE_TIME_TICK_MS)
			{
				workState = NORMAL_STATE;
			}			
		}break;
		case NORMAL_STATE:     
		{
			if(GetInputMode() == STOP )//|| Is_Serious_Error())
			{
				workState = STOP_STATE;
			}
			else if(!IsRemoteBeingAction()  && GetShootState() != SHOOTING) //||(Get_Lost_Error(LOST_ERROR_RC) == LOST_ERROR_RC
			{
				workState = STANDBY_STATE;      
			}			
		}break;
		case STANDBY_STATE:  
		{
			if(GetInputMode() == STOP )//|| Is_Serious_Error())
			{
				workState = STOP_STATE;
			}
			else if(IsRemoteBeingAction() || (GetShootState()==SHOOTING) || GetFrictionState() == FRICTION_WHEEL_START_TURNNING)
			{
				workState = NORMAL_STATE;
			}				
		}break;
		case STOP_STATE:   
		{
			if(GetInputMode() != STOP )//&& !Is_Serious_Error())
			{
				workState = PREPARE_STATE;   
			}
		}break;
		default:
		{
			
		}
	}	
}
void WorkStateSwitchProcess(void)
{
	//���������ģʽ�л���prapareģʽ��Ҫ��һϵ�в�����ʼ��
	if((lastWorkState != workState) && (workState == PREPARE_STATE))  
	{
		ControtLoopTaskInit();
		RemoteTaskInit();
	}
}
//���̿�������
void CMControlLoop(void)
{  
	//������ת������
	if(GetWorkState()==PREPARE_STATE) //�����׶Σ����̲���ת
	{
		ChassisSpeedRef.rotate_ref = 0;	 
	}
	else
	{
		 //���̸����������תPID����
		 CMRotatePID.ref = 500;
		 CMRotatePID.fdb = GMYawEncoder.ecd_angle;
		 CMRotatePID.Calc(&CMRotatePID);   
		 ChassisSpeedRef.rotate_ref = CMRotatePID.output;
				 ChassisSpeedRef.rotate_ref = 0;
	}
/*	if(Is_Lost_Error_Set(LOST_ERROR_RC))      //���ң������ʧ��ǿ�ƽ��ٶ��趨ֵreset
	{
		ChassisSpeedRef.forward_back_ref = 0;
		ChassisSpeedRef.left_right_ref = 0;
	}
*/	
	CM1SpeedPID.ref =  -ChassisSpeedRef.forward_back_ref*0.075 + ChassisSpeedRef.left_right_ref*0.075 + ChassisSpeedRef.rotate_ref;
	CM2SpeedPID.ref = ChassisSpeedRef.forward_back_ref*0.075 + ChassisSpeedRef.left_right_ref*0.075 + ChassisSpeedRef.rotate_ref;
	CM3SpeedPID.ref = ChassisSpeedRef.forward_back_ref*0.075 - ChassisSpeedRef.left_right_ref*0.075 + ChassisSpeedRef.rotate_ref;
	CM4SpeedPID.ref = -ChassisSpeedRef.forward_back_ref*0.075 - ChassisSpeedRef.left_right_ref*0.075 + ChassisSpeedRef.rotate_ref;

	CM1SpeedPID.fdb = CM1Encoder.filter_rate;
	CM2SpeedPID.fdb = CM2Encoder.filter_rate;
	CM3SpeedPID.fdb = CM3Encoder.filter_rate;
	CM4SpeedPID.fdb = CM4Encoder.filter_rate;
	
	CM1SpeedPID.Calc(&CM1SpeedPID);
	CM2SpeedPID.Calc(&CM2SpeedPID);
	CM3SpeedPID.Calc(&CM3SpeedPID);
	CM4SpeedPID.Calc(&CM4SpeedPID);
	
	 if((GetWorkState() == STOP_STATE)  || GetWorkState() == CALI_STATE || GetWorkState() == PREPARE_STATE)    //||Is_Serious_Error()|| dead_lock_flag == 1����ͣ����������У׼���޿�������ʱ����ʹ���̿���ֹͣ
	 {
		 Set_CM_Speed(0,0,0,0);
	 }
	 else
	 {
		 Set_CM_Speed(CHASSIS_SPEED_ATTENUATION * CM1SpeedPID.output, CHASSIS_SPEED_ATTENUATION * CM2SpeedPID.output, CHASSIS_SPEED_ATTENUATION * CM3SpeedPID.output, CHASSIS_SPEED_ATTENUATION * CM4SpeedPID.output);		 
	 } 
	 
}
static char Encoder_Dir = 1;
  
int32_t GetQuadEncoderDiff(void)
{
  int32_t cnt = 0;    
	cnt = 0x7fff - __HAL_TIM_GET_COUNTER(&htim3);
//	fw_printfln("%x",cnt);
	 __HAL_TIM_SET_COUNTER(&htim3, 0x7fff);
    if(Encoder_Dir == 1)
	{
		return cnt;	
	}
	else
	{
		return -cnt;            
	}
}

//�����������������
int16_t pwm_ccr = 0;
extern TIM_HandleTypeDef htim9;
uint16_t x = 0;
void ShooterMControlLoop(void)	
{				      
	if(GetShootState() == SHOOTING)
	{
		ShootMotorSpeedPID.ref = PID_SHOOT_MOTOR_SPEED;	
	}
	else
	{
		ShootMotorSpeedPID.ref = 0;		
	}
	
	ShootMotorSpeedPID.fdb = GetQuadEncoderDiff();   
	ShootMotorSpeedPID.Calc(&ShootMotorSpeedPID);
//	fw_printfln("%d",(uint32_t)ShootMotorSpeedPID.output);
	__HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, ShootMotorSpeedPID.output);
}

