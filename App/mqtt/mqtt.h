#ifndef _MQTT_H_
#define _MQTT_H_

#include "mqtt_config.h"
#include "mqttkit.h"


//��������ֵ�������޸�,�����ʱ��ʹ��
#define max_packet_len  300     //�������󳤶�
#define MAX_CMDID_TOPIC_LEN  50   //�������󳤶�
#define MAX_REQ_PAYLOAD_LEN  100   //���ݵ���󳤶�


_Bool OneNet_DevLink(void);

//void OneNet_SendData(int t, int h);

void OneNet_RevPro(unsigned char *cmd);


/**************************************************************
�������� : onenet_mqtt_send_heart
�������� : ��������
������� : ��
����ֵ  	 : ��
��ע		 : ��
**************************************************************/
void onenet_mqtt_send_heart(void);


/**************************************************************
�������� : onenet_mqtt_publish_topic
�������� : onenet mqtt ����������Ϣ
������� : topic������������
		   msg����Ϣ����
����ֵ  	 : PUBLISH_MSG_OK: ������Ϣ�ɹ�
		   PUBLISH_MSG_ERROR��������Ϣʧ��
��ע		 : ��
**************************************************************/
_Bool mqtt_publish_topic(const char *topic, const char *msg);


/**************************************************************
�������� : onenet_mqtt_subscribe_topic
�������� : onenet mqtt ��������
������� : topic�����ĵ�topic
		   topic_cnt��topic����
����ֵ  	 : SUBSCRIBE_TOPIC_OK:���ĳɹ�
		   SUBSCRIBE_TOPIC_ERROR������ʧ��
��ע		 : ��
**************************************************************/
_Bool mqtt_subscribe_topic(const signed char *topic[], unsigned char topic_cnt);

//����MQTT����
extern void ESP8266_SendData(char* buff, int len);

//���յ��·�������߽��յ����ĵ���Ϣ
extern void mqtt_rx(uint8* topic, uint8* cmd);

extern void debug(uint8 * msg, ...);

#endif
