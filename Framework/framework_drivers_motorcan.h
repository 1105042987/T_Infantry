#ifndef FRAMEWORK_MOTORCAN_H
#define FRAMEWORK_MOTORCAN_H

#include "cmsis_os.h"
#include "can.h"

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
void printMotorTask(void const * argument);
void controlMotorTask(void const * argument);
void Set_CM_Speed(int16_t cm1_iq, int16_t cm2_iq, int16_t cm3_iq, int16_t cm4_iq);
void motorCanTransmitTask(void const * argument);
void EncoderProcess(volatile Encoder *v, CanRxMsgTypeDef * msg);
void GetEncoderBias(volatile Encoder *v, CanRxMsgTypeDef * msg);
void CanReceiveMsgProcess(CanRxMsgTypeDef * msg);
	
#endif
