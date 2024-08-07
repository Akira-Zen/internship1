#include "AppMain.h"
#include "coretimer.h"
#include "stdio.h"
#include "spi2_driver.h"
#include "uart2.h"
#define THERMO 0x48
#define TEMP_REG 0x00
//page 16
#define TEMP_DATA 0b0110000000000000
//page 20
#define LIGHT 0x38
#define LIGHT_REG 0x41
//page 12
#define LIGHT_DATA 0b11000110
//page 12
#define ACC 0x1E
#define ACC_REG 0b00100111
uint32_t add = 0x000000;
bool state_flag = 1;
uint8_t buffer[256];
uint8_t data[256] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
int space_Flag = 0;
int a = 0;
int b = 0;

typedef enum {
    APP_INIT = 0,
    APP_AT_CHECK,
    APP_RUN,
    APP_ERROR,
    APP_IDLE
} app_task_t;

static struct {
    tick_timer_t Tick1;
    tick_timer_t Tick2;
    app_task_t Next;
} AppCxt;

uint8_t I2C_Scan(void) // <editor-fold defaultstate="collapsed" desc="Scan I2C slaver">
{
    uint8_t found, slv;

    __dbs("\nScan I2C: ");

    for (slv = 1, found = 0; slv < 128; slv++) {
        ClrWdt();

        if (I2C1_Master_Write(slv, NULL, 0)) {
            found++;
            __dbs("\n-> Found 0x");
            __dbh2(slv);
        }
    }

    return found;
} // </editor-fold>

void Display_Volatge(void) // <editor-fold defaultstate="collapsed" desc="Display all analog inputs as mV">
{
    __tsdbs("VOTAGE (mV):");
    __dbsu("\nVbat=", GetVbat());
    __dbsu("\nVcore=", GetVCore());
    __dbsu("\nVbg=", GetVBGap());
    __dbsu("\nVUno1=", GetUnoAn1());
    __dbsu("\nVUno2=", GetUnoAn2());
    __dbsu("\nVUno3=", GetUnoAn3());
    __dbsu("\nVUnoVref=", GetUnoVref());
} // </editor-fold>

private void APP_DebugCmd_Process(uint8_t *pCmd) // <editor-fold defaultstate="collapsed" desc="Debug CMD process">
{
    str_uppercase(pCmd);

    if (strstr((const char *) pCmd, "AT#CUSTOMAT") != NULL) {

    }
} // </editor-fold>

public void APP_Main_Initialize(void) // <editor-fold defaultstate="collapsed" desc="Application Initialize">
{
    __dbs("\n---------- APP INIT ----------\n");
    TRISAbits.TRISA15 = 0;
    LATAbits.LATA15 = 0;
    TRISAbits.TRISA7 = 0;
    LATAbits.LATA7 = 0;
    TRISAbits.TRISA10 = 1;
    PORTAbits.RA10 = 1;
    Tick_Timer_Reset(AppCxt.Tick1);
    Tick_Timer_Reset(AppCxt.Tick2);
    PORTAbits.RA6 = 0;
    LATAbits.LATA6 = 0;
    TRISBbits.TRISB8 = 1;
    LATBbits.LATB8 = 1;
    TRISBbits.TRISB9 = 0;
    LATBbits.LATB9 = 0;
    //i2c_write2ByteRegister(THERMO, TEMP_REG, TEMP_DATA);
    //i2c_write1ByteRegister(LIGHT, LIGHT_REG, LIGHT_DATA);
    //i2c_write1ByteRegister(ACC, ACC_REG,0b11000110 );  
    __dbs("\n------------------------------\n");
} // </editor-fold>  


// </editor-fold>
//void DataPlot(uint16_t ALS, uint16_t PS) {
//    UART3_Write(0x03);
//    UART3_Write((uint8_t) ALS);
//    UART3_Write((uint8_t) (ALS >> 8));
//    UART3_Write((uint8_t) PS);
//    UART3_Write((uint8_t) (PS >> 8));
//    UART3_Write(0xFC);
//}

//void Telit_SendDataBack(char *buffer_Uart2) {
//    if (UART2_IsRxReady()) {
//        *(buffer_Uart2) = UART2_Read();
//        //        UART3_Write(*(buffer_Uart2));
//    }
//}
//
//void Telit_ReceiveData(char *DataSend) {
//    for (int i = 0; i < strlen(DataSend); i++) {
//        UART2_Write(*(DataSend + i));
//    }
//}

tick_timer_t RxTick;
uint8_t RxBuff[256];
int RxLen;

void send_cmd(uint8_t *cmd) {
    printf("\nCMD: %s", cmd);

    while (*cmd != 0x00) {
        while (!UART2_IsTxReady());
        UART2_Write(*cmd++);
    }

    while (!UART2_IsTxDone());
    Tick_Timer_Reset(RxTick);
    RxLen = 0;
    memset(RxBuff, 0x00, sizeof(RxBuff));
}

bool get_res(void) {
    if (UART2_IsRxReady()) {
        RxBuff[RxLen++] = UART2_Read();

        if (RxLen >= sizeof (RxBuff))
            RxLen = 0;

        Tick_Timer_Reset(RxTick);
    } else if (Tick_Timer_Is_Over_Ms(RxTick, 100)) {
        if (RxBuff > 0)
            printf("\nRES %d: %s", RxLen, RxBuff);
        else
            printf("\nRES NULL");
        return 1;
    }

    return 0;
}

void APP_Main_Tasks(void) {
    static uint8_t DoNext = 0;

    switch (DoNext) {
        case 0:
            send_cmd((uint8_t *)"AT#CGSN\r");
            DoNext++;
            break;

        case 1:
            if (get_res()) {
                uint8_t imei[24];
                int i, j;
                //\r\nAT+CGSN 12345678906553434\r\nOK\r\n
                i = 5 + str_1st_contain_idx("CGSN: ", (char *)&RxBuff[0]);

                if (i >= 5) {
                    j = i + str_1st_index((char *)&RxBuff[i], '\r');

                    if (j > i) {
                        RxBuff[j] = 0x00;
                        strcpy((char *)imei, (char *)&RxBuff[i]);
                        printf("\n--> %s", imei);
                    }
                }

                DoNext++;
            }
            break;
            case 2:
            send_cmd((uint8_t *)"AT#CGMM\r");
            DoNext++;
            break;

        case 3:
            if (get_res()) {
                uint8_t model[24];
                int i, j;
                //\r\nAT+CGSN 12345678906553434\r\nOK\r\n
                i = 5 + str_1st_contain_idx("CGMM: ", (char *)&RxBuff[0]);

                if (i >= 5) {
                    j = i + str_1st_index((char *)&RxBuff[i], '\r');

                    if (j > i) {
                        RxBuff[j] = 0x00;
                        strcpy((char *)model, (char *)&RxBuff[i]);
                        printf("\n--> %s", model);
                    }
                }

                DoNext++;
            }
            break;
            case 4:
            send_cmd((uint8_t *)"AT+CEREG\r");
            DoNext++;
            break;

        case 5:
            if (get_res()) {
                uint8_t network[24];
                int i, j;
                //\r\nAT+CGSN 12345678906553434\r\nOK\r\n
                i = 5 + str_1st_contain_idx("CEREG: ", (char *)&RxBuff[0]);

                if (i >= 5) {
                    j = i + str_1st_index((char *)&RxBuff[i], '\r');

                    if (j > i) {
                        RxBuff[j] = 0x00;
                        strcpy((char *)network, (char *)&RxBuff[i]);
                        printf("\n--> %s", network);
                    }
                }

                DoNext++;
            }
            break;
            

        default:
            if(Tick_Timer_Is_Over_Ms(AppCxt.Tick1, 1000))
                DoNext=0;
            break;
    }
}
