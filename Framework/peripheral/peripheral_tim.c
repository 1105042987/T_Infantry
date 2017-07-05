/**
  ******************************************************************************
  * File Name          : peripheral_tim.c
  * Description        : 用户自定义定时器
  ******************************************************************************
  *
  * Copyright (c) 2017 Team TPP-Shanghai Jiao Tong University
  * All rights reserved.
  *
  * 用户自定义定时器初始化：
	*	摩擦轮、舵机PWM所需定时器
  ******************************************************************************
  */
#include "peripheral_tim.h"
#include "cmsis_os.h"
#include "tim.h"
#include "peripheral_define.h"
#include "pwm_server_motor.h"
#include "drivers_uartrc_user.h"
#include "tasks_motor.h"
void UserTimerInit(void)
{
//	HAL_TIM_Encoder_Start(&PLATE_TIM, TIM_CHANNEL_ALL);
//	HAL_TIM_PWM_Start(&PLATE_MOTOR_TIM , TIM_CHANNEL_1);//���̵��
	HAL_TIM_PWM_Start(&FRICTION_TIM, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&FRICTION_TIM, TIM_CHANNEL_2);
	pwm_server_motor_init(0);
#ifdef Infantry_4
				pwm_server_motor_set_angle(0,0.f);
#endif
#ifdef Infantry_3
				pwm_server_motor_set_angle(0,0.f);
#endif
#ifdef Infantry_2
				pwm_server_motor_set_angle(0,0.f);
#endif
				SetSlabState(OPEN);
}
