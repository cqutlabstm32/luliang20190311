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
 扩展实验2：ALIENTEK STM32F103开发板 
 GPS全球定位实验-库函数版本  
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/


u8 USART1_TX_BUF[USART3_MAX_RECV_LEN]; 					//串口1,发送缓存区
nmea_msg gpsx; 														//GPS信息
__align(4) u8 dtbuf[50];   								//打印缓存器
const u8*fixmode_tbl[4]={"Fail","Fail"," 2D "," 3D "};	//fix mode字符串 
	  
//显示GPS定位信息 
void Gps_Msg_Show(void)
{
		float tp;		   
		POINT_COLOR=BLUE;
		printf("\r\n");
		tp=gpsx.longitude;
		sprintf((char *)dtbuf,"Longitude:%.5f %1c   ",tp/=100000,gpsx.ewhemi);	//得到经度字符串
		printf("经度：%.2f %1c\r\n",tp,gpsx.ewhemi);
		LCD_ShowString(30,130,200,16,16,dtbuf);
		tp=gpsx.latitude;
		sprintf((char *)dtbuf,"Latitude:%.5f %1c   ",tp/=100000,gpsx.nshemi);	//得到纬度字符串
		printf("纬度：%.5f %1c\r\n",tp,gpsx.nshemi);
		LCD_ShowString(30,150,200,16,16,dtbuf);
		tp=gpsx.altitude;
		sprintf((char *)dtbuf,"Altitude:%.1fm     ",tp/=10);	    			//得到高度字符串
		printf("高度：%.1f米\r\n",tp);
		LCD_ShowString(30,170,200,16,16,dtbuf);	 			   
		tp=gpsx.speed;
		sprintf((char *)dtbuf,"Speed:%.3fkm/h     ",tp/=1000);		    		//得到速度字符串
		printf("速度：%.3fkm/h\r\n",tp);
		LCD_ShowString(30,190,200,16,16,dtbuf);	 				    
		if(gpsx.fixmode<=3)														//定位状态
		{
				sprintf((char *)dtbuf,"Fix Mode:%s",fixmode_tbl[gpsx.fixmode]);
				printf("定位状态：%s\r\n",fixmode_tbl[gpsx.fixmode]);
				LCD_ShowString(30,210,200,16,16,dtbuf);
		}
		sprintf((char *)dtbuf,"Valid satellite:%02d",gpsx.posslnum);	 		//用于定位的卫星数
		printf("可定位卫星数：%02d颗\r\n",gpsx.posslnum);
		printf("卫星编号：");
		for(u8 i=0;i<gpsx.posslnum;i++)	printf(" %d,",gpsx.possl[i]);
		printf("\r\n");
		LCD_ShowString(30,230,200,16,16,dtbuf);
		sprintf((char *)dtbuf,"Visible satellite:%02d",gpsx.svnum%100);	 		//可见卫星数
		printf("可见卫星数：%02d颗\r\n",gpsx.svnum%100);
		printf("卫星编号：");
		for(u8 i=0;i<gpsx.svnum%100;i++)	printf(" %d,",gpsx.slmsg[i].num);
		printf("\r\n");
		LCD_ShowString(30,250,200,16,16,dtbuf);
		sprintf((char *)dtbuf,"UTC Date:%04d/%02d/%02d   ",gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);	//显示UTC日期
		printf("日期：%04d年%02d月%02d日\r\n",gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);
		LCD_ShowString(30,270,200,16,16,dtbuf);
		sprintf((char *)dtbuf,"UTC Time:%02d:%02d:%02d   ",gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);	//显示UTC时间
		printf("时间：%02d时%02d分%02d秒\r\n",gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);
		LCD_ShowString(30,290,200,16,16,dtbuf);
		printf("\r\n");
}

int main(void)
{	 
		u16 i,rxlen;
		u16 lenx;
		u8 key=0XFF;
		u8 upload=0;
		delay_init();	    	 //延时函数初始化	  
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
		uart_init(115200);	 	//串口初始化为115200
		usart3_init(9600);		//初始化串口3波特率为38400
		LED_Init();				//初始化与LED连接的硬件接口
		KEY_Init();				//初始化按键
		LCD_Init();				//初始化LCD

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
				if(USART3_RX_STA&0X8000)		//接收到一次数据了
				{
						rxlen=USART3_RX_STA&0X7FFF;	//得到数据长度
						for(i=0;i<rxlen;i++)USART1_TX_BUF[i]=USART3_RX_BUF[i];
						USART3_RX_STA=0;		   	//启动下一次接收
						USART1_TX_BUF[i]=0;			//自动添加结束符
						GPS_Analysis(&gpsx,(u8*)USART1_TX_BUF);//分析字符串
						Gps_Msg_Show();				//显示信息	
						if(upload)
						{
								printf("\r\n%s\r\n",USART1_TX_BUF);//发送接收到的数据到串口1
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

