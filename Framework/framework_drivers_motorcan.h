#ifndef FRAMEWORK_MOTORCAN_H
#define FRAMEWORK_MOTORCAN_H

#include "cmsis_os.h"
#include "can.h"
#include "framework_utilities_iopool.h"

IOPoolDeclare(motorCanRxIOPool, CanRxMsgTypeDef);
IOPoolDeclare(ZGYROCanRxIOPool, CanRxMsgTypeDef);
IOPoolDeclare(motorCanTxIOPool, CanTxMsgTypeDef);

#define MOTOR1_ID 0x201u
#define MOTOR2_ID 0x202u
#define MOTOR3_ID 0x203u
#define MOTOR4_ID 0x204u
#define MOTORYAW_ID 0x205u
#define MOTORPITCH_ID 0x206u

#define ZGYRO_ID   0x401u

#define MOTORCM_ID 0x200u
#define MOTORGIMBAL_ID 0x1FFu

#define motorCan hcan2
#define ZGYROCAN hcan1

#define RATE_BUF_SIZE 6
typedef struct{
	int32_t raw_value;   									//���������������ԭʼֵ
	int32_t last_raw_value;								//��һ�εı�����ԭʼֵ
	int32_t ecd_value;                       //��������������ı�����ֵ
	int32_t diff;													//���α�����֮��Ĳ�ֵ
	int32_t temp_count;                   //������
	uint8_t buf_count;								//�˲�����buf��
	int32_t ecd_bias;											//��ʼ������ֵ	
	int32_t ecd_raw_rate;									//ͨ������������õ����ٶ�ԭʼֵ
	int32_t rate_buf[RATE_BUF_SIZE];	//buf��for filter
	int32_t round_cnt;										//Ȧ��
	int32_t filter_rate;											//�ٶ�
	float ecd_angle;											//�Ƕ�
}Encoder;

void motorInit(void);
void Set_CM_Speed(int16_t cm1_iq, int16_t cm2_iq, int16_t cm3_iq, int16_t cm4_iq);
void EncoderProcess(volatile Encoder *v, CanRxMsgTypeDef * msg);
void GetEncoderBias(volatile Encoder *v, CanRxMsgTypeDef * msg);
void CanReceiveMsgProcess(CanRxMsgTypeDef * msg);
void gyroinit(void);
	
#endif
