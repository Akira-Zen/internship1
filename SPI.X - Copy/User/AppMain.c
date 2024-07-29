#include "AppMain.h"
#include "coretimer.h"
#include "stdio.h"
#include "spi2_driver.h"
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
uint8_t data[256]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
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
    printf("hello\n");
    //i2c_write2ByteRegister(THERMO, TEMP_REG, TEMP_DATA);
    //i2c_write1ByteRegister(LIGHT, LIGHT_REG, LIGHT_DATA);
    //i2c_write1ByteRegister(ACC, ACC_REG,0b11000110 );  
    __dbs("\n------------------------------\n");
    Flash_Init();
    Erase_full_mem();
} // </editor-fold>  

void Telit_ReceiveDataFromUart3(char *buffer_Uart3) {
    if (UART3_IsRxReady()) {
        *(buffer_Uart3) = UART3_Read();
        UART3_Write(*(buffer_Uart3));
    }
}


//void DataPlot(uint16_t ALS, uint16_t PS) {
//    UART3_Write(0x03);
//    UART3_Write((uint8_t) ALS);
//    UART3_Write((uint8_t) (ALS >> 8));
//    UART3_Write((uint8_t) PS);
//    UART3_Write((uint8_t) (PS >> 8));
//    UART3_Write(0xFC);
//}

public void APP_Main_Tasks(void) // <editor-fold defaultstate="collapsed" desc="Application Main Task">
{
    
if (Tick_Timer_Is_Over_Ms(AppCxt.Tick2, 100)) {
    if (state_flag) {
        Flash_Sector_Program(add, data);
        Flash_Read(add);  // Read and print the data
        // Read the sector into RAM
    Flash_Read_Sector(add, buffer);

// Modify the byte at index 10
    Modify_Byte_In_RAM(buffer, 10, 0xAB);
    DeleteSector(add);
// Write the modified buffer back to flash
    Flash_Write_Sector(add, buffer);
    Flash_Read(add); 
        state_flag = 0;
        
    }
    LED2_Toggle();
}
    
}