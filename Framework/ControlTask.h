

//initiate status: 
typedef enum
{
    PREPARE_STATE,     		//�ϵ���ʼ��״̬ 4s������
    STANDBY_STATE,			//��ֹ̨ͣ��ת״̬
    NORMAL_STATE,			//������״̬
    STOP_STATE,        	//ֹͣ�˶�״̬
    CALI_STATE,    			//У׼״̬
}WorkState_e;

WorkState_e GetWorkState(void);

