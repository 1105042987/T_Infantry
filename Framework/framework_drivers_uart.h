#ifndef FRAMEWORK_UART_H
#define FRAMEWORK_UART_H

#define size_frame 14  //�������㴮��֡����/�ֽ�
#define byte_SOF 0x7d    //��ʼ�ֽ�	
#define byte_EOF 0x7e    //��ֹ�ֽ�
#define byte_ESCAPE 0xff //ת���ֽ�

#include "framework_utilities_iopool.h"

IOPoolDeclare(ctrlUartIOPool, struct{uint8_t ch[size_frame];});

void ctrlUartInit(void);
void ctrlUartRxCpltCallback(void);
void vSendUart(xdata_ctrlUart data);
xdata_ctrlUart xUartprocess(uint8_t *pData);

#endif
