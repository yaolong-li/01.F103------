/*
 * onenet_config.h
 * sz-yy.taobao.com
 *
 * 2019.05.01
 *
 *by ��
 */


#ifndef _ONENET_CONFIG_H_
#define _ONENET_CONFIG_H_


//�豸��Ϣ
#define DeviceName           "little"
#define ProductKey           "a1c8iNLOevX"
#define DeviceSecret           "ef87008b74300d18c90dc4ca40c35d08"

//clientID
#define mqttClientId "34EAE7244CEF|securemode=3,signmethod=hmacsha1,timestamp=789|"
//#define mqttClientId-1 "clientId12345deviceNameabcdproductKeya1lZYHY3VzYtimestamp789"


//���µ������Ǹ���mqttClientId-1��DeviceSecret����վhttps://1024tools.com/hmac�������
#define mqttPassword          "1e86943bd413ee21c8d6072c3b8e66dec1a49d7a"
//username little&a1c8iNLOevX
//serveIP 43.103.184.100    a1c8iNLOevX.iot-as-mqtt.cn-shanghai.aliyuncs.com
//port 1883
//����   /sys/a1c8iNLOevX/little/thing/event/property/post 
//����   /sys/a1c8iNLOevX/little/thing/service/property/set
#endif
