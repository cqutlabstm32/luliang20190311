#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "lcd.h"
#include "usart.h"
#include "gps.h"
#include "usart3.h"			 	 
#include "string.h"

 
 
 /************************************************
 ��չʵ��2��ALIENTEK STM32F103������ 
 GPSȫ��λʵ��-�⺯���汾  
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/


u8 USART1_TX_BUF[USART3_MAX_RECV_LEN]; 					//����1,���ͻ�����
nmea_msg gpsx; 														//GPS��Ϣ
__align(4) u8 dtbuf[50];   								//��ӡ������
const u8*fixmode_tbl[4]={"Fail","Fail"," 2D "," 3D "};	//fix mode�ַ��� 
	  
//��ʾGPS��λ��Ϣ 
void Gps_Msg_Show(void)
{
		float tp;		   
		POINT_COLOR=BLUE;
		printf("\r\n");
		tp=gpsx.longitude;
		sprintf((char *)dtbuf,"Longitude:%.5f %1c   ",tp/=100000,gpsx.ewhemi);	//�õ������ַ���
		printf("���ȣ�%.2f %1c\r\n",tp,gpsx.ewhemi);
		LCD_ShowString(30,130,200,16,16,dtbuf);
		tp=gpsx.latitude;
		sprintf((char *)dtbuf,"Latitude:%.5f %1c   ",tp/=100000,gpsx.nshemi);	//�õ�γ���ַ���
		printf("γ�ȣ�%.5f %1c\r\n",tp,gpsx.nshemi);
		LCD_ShowString(30,150,200,16,16,dtbuf);
		tp=gpsx.altitude;
		sprintf((char *)dtbuf,"Altitude:%.1fm     ",tp/=10);	    			//�õ��߶��ַ���
		printf("�߶ȣ�%.1f��\r\n",tp);
		LCD_ShowString(30,170,200,16,16,dtbuf);	 			   
		tp=gpsx.speed;
		sprintf((char *)dtbuf,"Speed:%.3fkm/h     ",tp/=1000);		    		//�õ��ٶ��ַ���
		printf("�ٶȣ�%.3fkm/h\r\n",tp);
		LCD_ShowString(30,190,200,16,16,dtbuf);	 				    
		if(gpsx.fixmode<=3)														//��λ״̬
		{
				sprintf((char *)dtbuf,"Fix Mode:%s",fixmode_tbl[gpsx.fixmode]);
				printf("��λ״̬��%s\r\n",fixmode_tbl[gpsx.fixmode]);
				LCD_ShowString(30,210,200,16,16,dtbuf);
		}
		sprintf((char *)dtbuf,"Valid satellite:%02d",gpsx.posslnum);	 		//���ڶ�λ��������
		printf("�ɶ�λ��������%02d��\r\n",gpsx.posslnum);
		printf("���Ǳ�ţ�");
		for(u8 i=0;i<gpsx.posslnum;i++)	printf(" %d,",gpsx.possl[i]);
		printf("\r\n");
		LCD_ShowString(30,230,200,16,16,dtbuf);
		sprintf((char *)dtbuf,"Visible satellite:%02d",gpsx.svnum%100);	 		//�ɼ�������
		printf("�ɼ���������%02d��\r\n",gpsx.svnum%100);
		printf("���Ǳ�ţ�");
		for(u8 i=0;i<gpsx.svnum%100;i++)	printf(" %d,",gpsx.slmsg[i].num);
		printf("\r\n");
		LCD_ShowString(30,250,200,16,16,dtbuf);
		sprintf((char *)dtbuf,"UTC Date:%04d/%02d/%02d   ",gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);	//��ʾUTC����
		printf("���ڣ�%04d��%02d��%02d��\r\n",gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);
		LCD_ShowString(30,270,200,16,16,dtbuf);
		sprintf((char *)dtbuf,"UTC Time:%02d:%02d:%02d   ",gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);	//��ʾUTCʱ��
		printf("ʱ�䣺%02dʱ%02d��%02d��\r\n",gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);
		LCD_ShowString(30,290,200,16,16,dtbuf);
		printf("\r\n");
}

int main(void)
{	 
		u16 i,rxlen;
		u16 lenx;
		u8 key=0XFF;
		u8 upload=0;
		delay_init();	    	 //��ʱ������ʼ��	  
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
		uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
		usart3_init(9600);		//��ʼ������3������Ϊ38400
		LED_Init();				//��ʼ����LED���ӵ�Ӳ���ӿ�
		KEY_Init();				//��ʼ������
		LCD_Init();				//��ʼ��LCD

		POINT_COLOR=RED;
		LCD_ShowString(30,20,200,16,16,"ALIENTEK STM32F1 ^_^");	  
		LCD_ShowString(30,40,200,16,16,"NE0-6M GPS TEST");	
		LCD_ShowString(30,60,200,16,16,"ATOM@ALIENTEK");
		LCD_ShowString(30,80,200,16,16,"KEY0:Upload NMEA Data SW");   	 										   	   
		LCD_ShowString(30,100,200,16,16,"NMEA Data Upload:OFF");
		gpsx.utc.time_zone = UTC8;

		while(1) 
		{
				delay_ms(1);
				if(USART3_RX_STA&0X8000)		//���յ�һ��������
				{
						rxlen=USART3_RX_STA&0X7FFF;	//�õ����ݳ���
						for(i=0;i<rxlen;i++)USART1_TX_BUF[i]=USART3_RX_BUF[i];
						USART3_RX_STA=0;		   	//������һ�ν���
						USART1_TX_BUF[i]=0;			//�Զ���ӽ�����
						GPS_Analysis(&gpsx,(u8*)USART1_TX_BUF);//�����ַ���
						Gps_Msg_Show();				//��ʾ��Ϣ	
						if(upload)
						{
								printf("\r\n%s\r\n",USART1_TX_BUF);//���ͽ��յ������ݵ�����1
						}
				}
				key=KEY_Scan(0);
				if(key==KEY0_PRES)
				{
						upload=!upload;
						POINT_COLOR=RED;
						if(upload)LCD_ShowString(30,100,200,16,16,"NMEA Data Upload:ON ");
						else LCD_ShowString(30,100,200,16,16,"NMEA Data Upload:OFF");
				}
				if((lenx%500)==0)LED0=!LED0; 	    				 
				lenx++;
		}
}

