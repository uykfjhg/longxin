/*
 * upload.c
 *
 * created: 2023/6/5
 *  author:
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "upload.h"
#include "ns16550.h"
int count;
float ph_value=0;
float last_num = 0;
unsigned char buf[20] = {0};
char rbuf[256];
char phcode[8];

extern int swj_ph_max_flag;  //��λ����ֵ���ñ�־λ
extern int swj_ph_min_flag;
extern int swj_tdsvalue_max_flag;
extern int swj_tds_max_flag;
extern int swj_ntu_max_flag;

extern int ght_ph_max_flag;  //�ֻ�����ֵ���ñ�־λ
extern int ght_ph_min_flag;
extern int ght_tdsvalue_max_flag;
extern int ght_tds_max_flag;
extern int ght_ntu_max_flag;

/********************************************************************************************************************
�ӹ��ͨ�·����������ȡ��ֵ
********************************************************************************************************************/
static int readphmax () // �ӹ��ͨ�·����������ȡ��ֵ
{
    unsigned  char str[256];//���ڽ����·����

    int i,rel=0,len=0,qian=0,hou=0,xs_num=1; //��ʼ��relΪ0
    int q;
    int ph=0;

    unsigned char strhp[5][8]={"PH_max\":","PH_min\":","DD_max\":","GR_max\":","WZ_max\":"};
    len=ls1x_uart_read(devUART4, str, 256, NULL);
    delay_ms(50);

    if(len>0)
    {
        for(q=0;q<5;q++)
        {
            for(i=0;i<len-1;i++) //�޸�ѭ������Ϊi<len-1
            {
                if(rel!=0&&rel<8)
                    if(str[i]==strhp[q][rel])
                    {
                        rel++; //�޸�Ϊrel++
                    }
                else
                    rel=0;
                if(rel==0)
                    if(str[i]==strhp[q][rel])
                        rel++; //�޸�Ϊrel++
                if(rel==8)
                {
                    i++; // ����ð��
                    while ((str[i] >= '0' && str[i] <= '9') || str[i] == '.')// ��ȡ��ֵ
                    {
                        if (str[i] == '.')
                        {
                            i++;
                            while (str[i] >= '0' && str[i] <= '9')
                            {
                                hou = hou * 10 + (str[i] - '0'); //ת��Ϊ int ��
                                i++;
                                xs_num=xs_num*10;
                            }
                            break;
                        }
                        else
                        {
                            qian = qian * 10 + (str[i] - '0'); // ת��Ϊ int ��
                            i++;
                        }
                    }
                }

            }
            ph=qian+hou/xs_num;
            last_num = qian+hou/(xs_num*1.0);


            if(ph!=0)
            {
                phcode[0]=q+1;
                q=6;
                phcode[1]=ph/256;
                phcode[2]=ph%256;
                phcode[3]=qian;
                phcode[4]=hou;
                //ls1x_uart_write(devUART4,phcode,5,NULL);
            }

        }

    }

    return last_num;
}




void uartrs(float *ph_max_phone,float *ph_min_phone,float *tdsvalue_max_phone,float *tds_max_phone,float *ntu_max_phone)
{
    ph_value=readphmax ();

    //sprintf((char *)buf, "����ֵΪ��%.1f ", last_num);  //phֵ
    //fb_textout(251, 560, buf);
    //memset(buf, 0, sizeof(buf));

    if(ph_value>0)
    {
        if(phcode[0] == 1)
        {
            *ph_max_phone = last_num;
            //phcode[0]=0;
            ght_ph_max_flag = 1;
        }

        if(phcode[0] == 2)
        {
            *ph_min_phone = last_num;
            //phcode[0]=0;
            ght_ph_min_flag = 1;
        }

        if(phcode[0] == 3)
        {
            *tdsvalue_max_phone = last_num;
            //phcode[0]=0;
            ght_tdsvalue_max_flag = 1;
        }

        if(phcode[0] == 4)
        {
            *tds_max_phone = last_num;
            //phcode[0]=0;
            ght_tds_max_flag = 1;
        }

        if(phcode[0] == 5)
        {
            *ntu_max_phone = last_num;
            //phcode[0]=0;
            ght_ntu_max_flag = 1;
        }
    }

}

//-------------------------------------------------------------------------------------------------
// ��λ�����ݼ����봫��
//-------------------------------------------------------------------------------------------------

/*У�������*/
static unsigned short crcmodbus_cal(unsigned char*data, unsigned char len)//Ҫ��У��
{
    unsigned short tmp = 0xFFFF,ret;
    unsigned char i,j;

    for(i=0;i<len;i++){
        tmp = data[i] ^ tmp;
        for(j=0;j<8;j++){
            if(tmp & 0x01){
                tmp = tmp >> 1;
                tmp = tmp ^ 0xA001;
            }else
                tmp = tmp >> 1;
        }
    }

    ret = tmp >> 8;
    ret = ret | (tmp << 8);

    return ret;
}

/********************************************************************************************************************
�ȶ������ַ����Ƿ���ȫ��ͬ
********************************************************************************************************************/
static unsigned char contrast(const char *array1, int len_array1, const char *array2,int len_array2)//�ȶ������ַ����Ƿ���ȫ��ͬ
{
    int i;

    if(len_array1 != len_array2)
    {
        return 0;
    }
    for(i=0;i<len_array1;i++)
    {
        if(*(array1+i) != *(array2+i))
        {
            return 0;
        }
    }
    return 1;
}


/********************************************************************************************************************
��λ���ϴ�ʵʱ����
********************************************************************************************************************/
void senddata(unsigned int ph,unsigned int tdsvalue,unsigned int ntu,unsigned int tds,unsigned int hwd, int wwd, int *ph_max, int *ph_min, int *tdsvalue_max, int *tds_max, int *ntu_max)//Ҫ�÷���    ntu, tdsvalue, ph, tds;
{
    unsigned char data[17] = {0x01,0x03,0x0C};  //01 03�̶� ��3������λ���� ������һ��Ϊ�������� �����λΪУ����
    unsigned char strhp[8]={0x01,0x03,0,0,0,0x06,0xC5,0xC8};//��λ��������ֵѯ�����
    unsigned char strhp_max[8]={0x01,0x03,0,0x06,0,0x05,0x65,0xC8};//��λ����ֵѯ�����
    unsigned char str[12];
    unsigned char data_max[15] = {0x01,0x03,0x0A};
    unsigned char num_max[1];
    unsigned int num;
    unsigned int len_str;
    unsigned int len_strhp;
    unsigned int len_strhp_max;
    unsigned int len_strhp_a;
    unsigned short buf;
    unsigned char numH,numL;

    numH=ph/256;
    numL=ph%256;
    data[3] = numH;   //��λ
    data[4] = numL;//��λ

    numH=tdsvalue/256;
    numL=tdsvalue%256;
    data[5] = numH;   //��λ
    data[6] = numL;//��λ

    numH=ntu/256;
    numL=ntu%256;
    data[7] = numH;   //��λ
    data[8] = numL;//��λ

    numH=tds/256;
    numL=tds%256;
    data[9] = numH;   //��λ
    data[10] = numL;//��λ

    numH=hwd/256;
    numL=hwd%256;
    data[11] = numH;   //��λ
    data[12] = numL;//��λ

    numH=wwd/256;
    numL=wwd%256;
    data[13] = numH;   //��λ
    data[14] = numL;//��λ


    buf = crcmodbus_cal(data, 15);
    data[15] = buf >> 8;
    data[16] = buf & 0xff;

    len_str = ls1x_uart_read(devUART5, str, 12, NULL);

    len_strhp = sizeof(strhp) / sizeof(strhp[0]);
    len_strhp_max = sizeof(strhp_max) / sizeof(strhp_max[0]);

    if(contrast(str, len_str, strhp, len_strhp))
    {
        ls1x_uart_write(devUART5, data, sizeof(data), NULL);
    }

    else if(contrast(str, len_str, strhp_max, len_strhp_max))
    {

        numH=*ph_max/256;
        numL=*ph_max%256;
        data_max[3] = numH;   //��λ
        data_max[4] = numL;//��λ

        numH=*ph_min/256;
        numL=*ph_min%256;
        data_max[5] = numH;   //��λ
        data_max[6] = numL;//��λ

        numH=*tdsvalue_max/256;
        numL=*tdsvalue_max%256;
        data_max[7] = numH;   //��λ
        data_max[8] = numL;//��λ

        numH=*tds_max/256;
        numL=*tds_max%256;
        data_max[9] = numH;   //��λ
        data_max[10] = numL;//��λ

        numH=*ntu_max/256;
        numL=*ntu_max%256;
        data_max[11] = numH;   //��λ
        data_max[12] = numL;//��λ

        buf = crcmodbus_cal(data_max, 13);
        data_max[13] = buf >> 8;
        data_max[14] = buf & 0xff;

        ls1x_uart_write(devUART5, data_max,15, NULL);
    }
    else if((str[0]==0x01)&&(str[1]==0x06)&&(str[2]==0))//�����·�����ֵ
    {
        num = str[4]*256+str[5];

        switch(str[3])
        {
            case 0x06:
                *ph_max=num;
                swj_ph_max_flag = 1;
                break;

            case 0x07:
                *ph_min=num;
                swj_ph_min_flag = 1;
                break;

            case 0x08:
                *tdsvalue_max=num;
                swj_tdsvalue_max_flag = 1;
                break;

            case 0x09:
                *tds_max=num;
                swj_tds_max_flag = 1;
                break;

            case 0x0A:
                *ntu_max=num;
                swj_ntu_max_flag = 1;
                break;
        }
    }

}
/********************************************************************************************************************
����ͷʶ��

��δдѯ����䣡����
********************************************************************************************************************/
void sxt()
{
    extern int sxt_flag ;
                            //��������
    unsigned char data[6] = {0x55,0xAA,0x11,0x00,0x24,0x34};  //55aa11002131
    unsigned char strhp[4]= {0x8B, 0xAA, 0x11, 0x0A};//�������ǰ��λ55 AA 11 0A 29
    unsigned char str[256];   //���յ�����
    unsigned char str_temp[5]; //��Ž�������ǰ��λ
    unsigned char color_num[5]={0xf0};//���ʶ�𵽵���ɫ  ǰ��λΪУ����
    unsigned char len_str;  //���յ������ݳ���
    unsigned char len_strhp;//�������ǰ��λ�ռ��С
    char i = 0;

    ls1x_uart_write(devUART1, data, 6, NULL);  //�����������󷵻���ɫ����

    len_str = ls1x_uart_read(devUART1, str, 256, NULL);  //���շ��ص�����ɫ

    if(len_str == 16);  //���ص����ݳ���Ϊ16  ��ʾû��ʶ����ɫ  ��ִ�в���

    else  //ʶ����ɫ
    {
        sxt_flag = 0 ;
        for(i=0;i<4;i++)  //�ѽ��յ�������ǰ��λ��ֵ��temp
        {
            str_temp[i] = str[i];
        }

        len_strhp = sizeof(strhp) / sizeof(strhp[0]);  //�������ǰ��λ�ռ��С
        //ls1x_uart_write(devUART0, str, len_str, NULL);  //���Խ������

        if((str_temp[1]==str[1])&&(str_temp[2]==str[2])&&(str_temp[3]==str[3]))//��������У��
        //if(contrast(str_temp, 5, strhp, len_strhp))  //�ж�temp�ͽ�������ǰ��λ�Ƿ���ͬ
        {
            //ls1x_uart_write(devUART0, str, len_str, NULL);  //���Խ������

            color_num[2] = str[5];  //ʶ�𵽵���ɫ��
            color_num[1]=0xbb;    //��У��

            switch(len_str/16)
            {
                case 2:
                    color_num[3] = str[29];
                    ls1x_uart_write(devUART0, color_num,4, NULL);//���ʶ��1����ɫ���
                    break;
                case 3:
                    color_num[3] = str[29];
                    color_num[4] = str[45];
                    ls1x_uart_write(devUART0, color_num,5, NULL);//���ʶ��2����ɫ���
                    break;
                case 4:
                    color_num[3] = str[29];
                    color_num[4] = str[45];
                    color_num[5] = str[61];
                    ls1x_uart_write(devUART0, color_num,6, NULL);//���ʶ��3����ɫ���
                    break;
            }
            color_num[1]=0x00;    //�ر�У��
        }
    }
}


/*
 * upload.c
 *
 * created: 2023/6/5
 *  author:
 */



