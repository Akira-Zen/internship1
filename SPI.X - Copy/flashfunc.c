#include "flashfunc.h"
#include "system.h"
#include "mcc.h"
#include "cfg/Project_Cfg.h"
#include "stdio.h"
#include "System/TickTimer.h"
#include "spi2_driver.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#define JEDEC_ID_Read 0x9F //JEDEC ID Read
#define WREN 0x06   //Write Enable
#define Read 0x0B        // Read Memory at Higher Speed
#define Fast_Read_Dual_Output 0x3B  //Read Memory with Dual Output
#define Fast_Read_Dual_IO 0xBB      //Read Memory with Dual Address Input and Data Output
#define four_Kb_Sector_Erase 0x20      //Erase 4 -Kbyte of memory array
#define Sixfour_Kb_Block_Erase 0xD8      //Erase 64-Kbyte block of memory array
#define Chip_Erase 0xC7             //Erase Full Memory Array
#define Page_Program 0x02    //To program up to 256 Bytes         
#define RDSR 0x05   //Read STATUS Register
#define WRSR 0x01   //Write STATUS Register
#define WRDI 0x04   //Write Disable
#define RDID 0xAB   //Read-ID 
#define DPD 0xB9    //Deep Power-Down Mode
#define Sector_Erase 0x20

char Extra_Sector[4096];
int i_extra = 0;

uint32_t Flash_ExchangeNByte(uint8_t data_send, uint8_t NByte){//NByte don vi la byte
    uint32_t data_receive=0;
    FLASH_CS_N_SetLow();
    spi2_exchangeByte(data_send);
    for(int i=0; i<NByte; i++){
        data_receive<<=8;
        data_receive|=spi2_exchangeByte(0x00);
        
    }
    FLASH_CS_N_SetHigh();
    return data_receive;
}
bool Busy_flag(){
    return Flash_ExchangeNByte(RDSR,1)&0x01;
}
void Write_Enable(){
    Flash_ExchangeNByte(WREN,0);
}
void Write_Disable(){
    Flash_ExchangeNByte(WRDI,0);
}
void Flash_Init(){//mode0 spi
    spi2_open(MASTER0_CONFIG);
    Write_Enable();
    FLASH_CS_N_SetLow();
    spi2_exchangeByte(WRSR);
    spi2_exchangeByte(0b00000010);
    FLASH_CS_N_SetHigh();
    while(Busy_flag());
    Write_Enable();
}
void Erase_full_mem(){
    Write_Enable();
    Flash_ExchangeNByte(Chip_Erase,0);
    while(Busy_flag());
    printf("Chip erased %d\n", Busy_flag());
    Write_Disable();
}
void Flash_Read(uint32_t address) {
    uint8_t value[256]; // Adjust size if reading less data
    FLASH_CS_N_SetLow();
    spi2_exchangeByte(Read);                  // Send read command
    spi2_exchangeByte((address >> 16) & 0xFF); // Send address bytes
    spi2_exchangeByte((address >> 8) & 0xFF);
    spi2_exchangeByte(address & 0xFF);

    // Read data bytes
    for (int i = 0; i < 256; i++) {
        value[i] = spi2_exchangeByte(0xFF);  // Send dummy byte to receive data
    }

    FLASH_CS_N_SetHigh();

    // Print the read values
    printf("Data read from address %08X:\n", address);
    for (int i = 0; i < 256; i++) {
        printf("%02X ", value[i]);
        if ((i + 1) % 16 == 0) {  // Print new line every 16 bytes for readability
            printf("\n");
        }
    }
}
void Flash_Sector_Program(uint32_t address, const uint8_t value[256]) {
    Write_Enable();                 // Enable writing to flash memory
    FLASH_CS_N_SetLow();           // Assert chip select
    spi2_exchangeByte(Page_Program);  // Send page program command
    spi2_exchangeByte((address >> 16) & 0xFF); // Send address bytes
    spi2_exchangeByte((address >> 8) & 0xFF);
    spi2_exchangeByte(address & 0xFF);

    // Send the data bytes
    for (int i = 0; i < 256; i++) {
        spi2_exchangeByte(value[i]);
    }

    FLASH_CS_N_SetHigh();          // Deassert chip select
    while (Busy_flag());           // Wait for programming to complete
    Write_Disable();               // Disable writing to flash memory
}
void Flash_Read_Sector(uint32_t address, uint8_t *buffer) {
    FLASH_CS_N_SetLow();
    spi2_exchangeByte(Read);                  // Send read command
    spi2_exchangeByte((address >> 16) & 0xFF); // Send address bytes
    spi2_exchangeByte((address >> 8) & 0xFF);
    spi2_exchangeByte(address & 0xFF);

    // Read data bytes
    for (int i = 0; i < 256; i++) {
        buffer[i] = spi2_exchangeByte(0xFF);  // Send dummy byte to receive data
    }

    FLASH_CS_N_SetHigh();
}
void Modify_Byte_In_RAM(uint8_t *buffer, size_t index, uint8_t new_value) {
    if (index < 256) { // Ensure index is within bounds
        buffer[index] = new_value;
    }
}
void Flash_Write_Sector(uint32_t address, const uint8_t *buffer) {
    Write_Enable();                 // Enable writing to flash memory
    FLASH_CS_N_SetLow();           // Assert chip select
    spi2_exchangeByte(Page_Program);  // Send page program command
    spi2_exchangeByte((address >> 16) & 0xFF); // Send address bytes
    spi2_exchangeByte((address >> 8) & 0xFF);
    spi2_exchangeByte(address & 0xFF);

    // Send the data bytes
    for (int i = 0; i < 256; i++) {
        spi2_exchangeByte(buffer[i]);
    }

    FLASH_CS_N_SetHigh();          // Deassert chip select
    while (Busy_flag());           // Wait for programming to complete
    Write_Disable();               // Disable writing to flash memory
}
void DeleteSector(uint32_t address){
    Write_Enable();                 // Enable writing to flash memory
    FLASH_CS_N_SetLow(); 
    spi2_exchangeByte(Sector_Erase);  // Send page program command
    spi2_exchangeByte((address >> 16) & 0xFF); // Send address bytes
    spi2_exchangeByte((address >> 8) & 0xFF);
    spi2_exchangeByte(address & 0xFF);
    FLASH_CS_N_SetHigh();
    while (Busy_flag());   
    printf("Sector erased %d\n", Busy_flag());// Wait for programming to complete
    Write_Disable();  
}