/**
	************************************************************
	************************************************************
	************************************************************
	*	�ļ����� 	onenet.c
	*
	*	���ߣ� 		
	*
	*	���ڣ� 		2017-05-08
	*
	*	�汾�� 		V1.1
	*
	*	˵���� 		��onenetƽ̨�����ݽ����ӿڲ�
	*
	*	�޸ļ�¼��	V1.0��Э���װ�������ж϶���ͬһ���ļ������Ҳ�ͬЭ��ӿڲ�ͬ��
	*				V1.1���ṩͳһ�ӿڹ�Ӧ�ò�ʹ�ã����ݲ�ͬЭ���ļ�����װЭ����ص����ݡ�
	************************************************************
	************************************************************
	************************************************************
**/
//Э���ļ�
#include "mqtt.h"
#include "mqttkit.h"

//C��
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

uint8 packet_buff[max_packet_len]={0};

char cmdid_topic[MAX_CMDID_TOPIC_LEN+1] = {0};
char req_payload[MAX_REQ_PAYLOAD_LEN+1] = {0};

unsigned char mqttUsername[30]={0};
//==========================================================
//	�������ƣ�	OneNet_DevLink
//
//	�������ܣ�	��onenet��������
//
//	��ڲ�����	��
//
//	���ز�����	0-�ɹ�	1-ʧ��
//
//	˵����		��onenetƽ̨��������
//==========================================================
_Bool OneNet_DevLink(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = {packet_buff, 0, max_packet_len, MEM_FLAG_STATIC};                //Э���

	unsigned char *dataPtr;
	
	_Bool status = 1;

        sprintf(mqttUsername, "%s&%s", DeviceName, ProductKey);
        debug( "OneNet_DevLink\r\nmqttUsername:%s, mqttPassword:%s,mqttClientId:%s\r\n", mqttUsername, mqttPassword, mqttClientId);
    //����onenet
	if(MQTT_PacketConnect(mqttUsername, mqttPassword, mqttClientId, 256, 0, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//�ϴ�ƽ̨
		
		MQTT_DeleteBuffer(&mqttPacket);								//ɾ��
	}
	else
		debug("WARN:	MQTT_PacketConnect Failed\r\n");
	
	return status;
	
}


//==========================================================
//	�������ƣ�	OneNet_RevPro
//
//	�������ܣ�	ƽ̨�������ݼ��
//
//	��ڲ�����	dataPtr��ƽ̨���ص�����
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	MQTT_PACKET_STRUCTURE mqttPacket = {packet_buff, 0, max_packet_len, 0};								//Э���
	
	unsigned short topic_len = 0;
	unsigned short req_len = 0;

	unsigned char qos = 0;
	static unsigned short pkt_id = 0;

    
	unsigned char type = 0;
	
	short result = 0;
        
	type = MQTT_UnPacketRecv(cmd);
    
	switch(type)
	{
		case MQTT_PKT_CMD:															//�����·�
    {
			result = MQTT_UnPacketCmd(cmd, cmdid_topic, req_payload, &req_len);	//���topic����Ϣ��
			if(result == 0)
			{
				debug("cmdid: %s, req: %s, req_len: %d\r\n", cmdid_topic, req_payload, req_len);
				mqtt_rx(cmdid_topic, req_payload);
#if 1

				if(MQTT_PacketCmdResp(cmdid_topic, req_payload, &mqttPacket) == 0)	//����ظ����
				{
					debug("Tips:	Send CmdResp\r\n");
					
					ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//�ظ�����
					MQTT_DeleteBuffer(&mqttPacket);									//ɾ��
				}
#endif
			}
    }
		break;
			
		case MQTT_PKT_PUBACK:														//����Publish��Ϣ��ƽ̨�ظ���Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
				debug("Tips:	MQTT Publish Send OK\r\n");
			
		break;

        case MQTT_PKT_PINGRESP:
        {
            debug("Tips:  HeartBeat OK\r\n");
            break;
        }
           
        case MQTT_PKT_PUBLISH:  //���յ�Publish��Ϣ
        {
            if(MQTT_UnPacketPublish(cmd, cmdid_topic, &topic_len, req_payload, &req_len, &qos, &pkt_id) == 0)
            {
                debug("topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",cmdid_topic, topic_len, req_payload, req_len);
                mqtt_rx(cmdid_topic, req_payload);

               // MQTT_FreeBuffer(cmdid_topic);
               // MQTT_FreeBuffer(req_payload);        

                switch(qos)
                {
                    case 1:                                                         //�յ�publish��qosΪ1���豸��Ҫ�ظ�Ack
                    {
                        if(MQTT_PacketPublishAck(pkt_id, &mqttPacket) == 0)
                        {
                            debug("Tips:  Send PublishAck\r\n");
                            ESP8266_SendData(mqttPacket._data, mqttPacket._len);
                            MQTT_DeleteBuffer(&mqttPacket);
                        }
                        break;
                    }
                    case 2: //�յ�publish��qosΪ2���豸�Ȼظ�Rec
                    {       //ƽ̨�ظ�Rel���豸�ٻظ�Comp
                        if(MQTT_PacketPublishRec(pkt_id, &mqttPacket) == 0)
                        {
                            debug("Tips:  Send PublishRec\r\n");
                            ESP8266_SendData(mqttPacket._data, mqttPacket._len);
                            MQTT_DeleteBuffer(&mqttPacket);
                        }
                        break;
                    }
                    default:
                        break;
                }
                
            }
            break;
        }
        case MQTT_PKT_PUBREC:   //����Publish��Ϣ��ƽ̨�ظ���Rec���豸��ظ�Rel��Ϣ
        {
            if(MQTT_UnPacketPublishRec(cmd) == 0)
            {
                debug("Tips:  Rev PublishRec\r\n");
                if(MQTT_PacketPublishRel(MQTT_PUBLISH_ID, &mqttPacket) == 0)
                {
                    debug("Tips:  Send PublishRel\r\n");
                    ESP8266_SendData(mqttPacket._data, mqttPacket._len);
                    MQTT_DeleteBuffer(&mqttPacket);
                }
            }
            break;
        }   
        case MQTT_PKT_PUBREL://�յ�Publish��Ϣ���豸�ظ�Rec��ƽ̨�ظ���Rel���豸���ٻظ�Comp
        {
            if(MQTT_UnPacketPublishRel(cmd, pkt_id) == 0)
            {
                debug("Tips:  Rev PublishRel\r\n");
                if(MQTT_PacketPublishComp(MQTT_PUBLISH_ID, &mqttPacket) == 0)
                {
                   debug("Tips:  Send PublishComp\r\n");
                    ESP8266_SendData(mqttPacket._data, mqttPacket._len);
                    MQTT_DeleteBuffer(&mqttPacket);
                }
            }
            break;
        }
        case MQTT_PKT_PUBCOMP:  //����Publish��Ϣ��ƽ̨����Rec���豸�ظ�Rel��ƽ̨�ٷ��ص�Comp
        {
            if(MQTT_UnPacketPublishComp(cmd) == 0)
            {
                debug("Tips:  Rev PublishComp\r\n");
            }
            break;
        }
        case MQTT_PKT_SUBACK:   //����Subscribe��Ϣ��Ack
        {
            if(MQTT_UnPacketSubscribe(cmd) == 0)
            {
                debug("Tips:  MQTT Subscribe OK\r\n");
            }
            else
            {
                debug("Tips:  MQTT Subscribe Err\r\n");
            }
            break;
        }
        case MQTT_PKT_UNSUBACK: //����UnSubscribe��Ϣ��Ack
        {
            if(MQTT_UnPacketUnSubscribe(cmd) == 0)
            {
                debug("Tips:  MQTT UnSubscribe OK\r\n");
            }
            else
            {
                debug("Tips:  MQTT UnSubscribe Err\r\n");
            }
            break;
        } 		
		default:
			result = -1;
		break;
	}
}

/**************************************************************
�������� : onenet_mqtt_send_heart
�������� : ��������
������� : ��
����ֵ  	 : ��
��ע		 : ��
**************************************************************/
void onenet_mqtt_send_heart(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = {packet_buff, 0, max_packet_len, MEM_FLAG_STATIC};                //Э���

	debug("send heart\r\n");
	MQTT_PacketPing(&mqttPacket);
    ESP8266_SendData(mqttPacket._data, mqttPacket._len);
	MQTT_DeleteBuffer(&mqttPacket);									//ɾ��
}


/**************************************************************
�������� : mqtt_publish_topic
�������� : onenet mqtt ����������Ϣ
������� : topic������������
		   msg����Ϣ����
����ֵ  	 : PUBLISH_MSG_OK: ������Ϣ�ɹ�
		   PUBLISH_MSG_ERROR��������Ϣʧ��
��ע		 : ��
**************************************************************/
_Bool mqtt_publish_topic(const char *topic, const char *msg)
{
  MQTT_PACKET_STRUCTURE mqttPacket = {packet_buff, 0, max_packet_len, MEM_FLAG_STATIC};                //Э���
	
	debug("publish topic: %s, msg: %s\r\n", topic, msg);

	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL2, 0, 1, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);//�ϴ�ƽ̨
		MQTT_DeleteBuffer(&mqttPacket);	//ɾ��
		return 0;
	}
	else
	{
		return 1;
	}

}

/*  ��������Ϊ"hello_topic"����ϢΪ"hello_world" */
//if(0 == onenet_mqtt_publish_topic("hello_topic", "hello_world"))
//{;}

 
/**************************************************************
�������� : onenet_mqtt_subscribe_topic
�������� : onenet mqtt ��������
������� : topic�����ĵ�topic
		   topic_cnt��topic����
����ֵ  	 : SUBSCRIBE_TOPIC_OK:���ĳɹ�
		   SUBSCRIBE_TOPIC_ERROR������ʧ��
��ע		 : ��
**************************************************************/
_Bool mqtt_subscribe_topic(const signed char *topic[], unsigned char topic_cnt)
{
	
	unsigned char i = 0;
  MQTT_PACKET_STRUCTURE mqttPacket = {packet_buff, 0, max_packet_len, MEM_FLAG_STATIC};                //Э���
 
	for(; i < topic_cnt; i++)
	{
		debug("subscribe topic: %s\r\n", topic[i]);
	}
    
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL2, topic, topic_cnt, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);//�ϴ�ƽ̨
		MQTT_DeleteBuffer(&mqttPacket);											//ɾ��
		return 0;
	}
	else
	{
		return 1;
	}
}

//���ⶩ�ķ���
//const signed char *g_mqtt_topics[] = {"mqtt_topic", "topic_test"};
//if(0 == onenet_mqtt_subscribe_topic(g_mqtt_topics, 2))


//����MQTT����
void ESP8266_SendData(char* buff, int len)
{
    if(len==0) return;

    //HalUARTWrite(0,buff, len);
}


//���յ��·�������߽��յ����ĵ���Ϣ
//topic:�յ�������
//cmd::�����Ӧ������
void mqtt_rx(uint8* topic, uint8* cmd)
{
    char *ptr_led3_on = NULL;
    char *ptr_led3_off = NULL;


    //Э�����յ����ݺ�
    //������ƾ���������δ���

    if(topic == NULL || cmd==NULL) return;

    //����ֻ�������ݣ���ֻ����CMD������������

    ptr_led3_on = strstr((char *)cmd, "\"led3\":1");
    ptr_led3_off = strstr((char *)cmd, "\"led3\":0");

    if(ptr_led3_on!=NULL)
    {
      //P1_4=0;//D3��
    }
    else if(ptr_led3_off!=NULL)
    {
      //P1_4=1;//D3��
    }
}


