#ifndef DRIVERS_UARTUPPER_USER_H
#define DRIVERS_UARTUPPER_USER_H

#include "utilities_iopool.h"
#define size_frame 14  //�������㴮��֡����/�ֽ�
#define byte_SOF 0x7d    //��ʼ�ֽ�	
#define byte_EOF 0x7e    //��ֹ�ֽ�
#define byte_ESCAPE 0xff //ת���ֽ�

typedef struct{
	uint16_t dev_yaw;
	uint16_t dev_pitch;
	uint16_t target_vl;	
	uint16_t target_dis;
	uint8_t DLC;
	uint8_t Success;
}xdata_ctrlUart;

IOPoolDeclare(ctrlUartIOPool, struct{uint8_t ch[size_frame];});

void ctrlUartRxCpltCallback(void);

#endif
