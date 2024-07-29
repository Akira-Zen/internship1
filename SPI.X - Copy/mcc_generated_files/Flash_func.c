#include "Flash_func.h"
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
#define High_Speed_Read 0x0B        // Read Memory at Higher Speed
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
void Flask_Init(){//mode0 spi
    spi2_open(SPI2_DEFAULT);
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
void Erase_4Kb_mem(uint32_t add1){
    Write_Enable();
    FLASH_CS_N_SetLow();
    
    spi2_exchangeByte(four_Kb_Sector_Erase);
    spi2_exchangeByte(add1>>16);
    spi2_exchangeByte(add1>>8);
    spi2_exchangeByte(add1);
    
    FLASH_CS_N_SetHigh();
    while(Busy_flag());
    printf("Erase 4Kb memory %d\n", Busy_flag());
    Write_Disable();
}
void Flash_Print_Sector(uint32_t add, int num_of_Sector){
    add &=0xFFF000;
    uint32_t Sector=add/4096;
    char get_s_Flash;
    
    printf("address start Read: 0x%x\n",add);
    FLASH_CS_N_SetLow();
    spi2_exchangeByte(High_Speed_Read);
    spi2_exchangeByte(add>>16);
    spi2_exchangeByte(add>>8);
    spi2_exchangeByte(add);
    spi2_exchangeByte(0x00);
    int count =0;
    int count_page =0;
    printf("\n+++++++++++++++++++++++++++Sector %u+++++++++++++++++++++++++++++++ \n\n", Sector);
    for(uint32_t i=0;i<num_of_Sector*4096;i++){
        get_s_Flash = spi2_exchangeByte(0x00);
        int s_Flash_to_i = (int)get_s_Flash;
        if (s_Flash_to_i>127 || s_Flash_to_i<0){
            printf(" %x",get_s_Flash);
        }
        else printf("%c",get_s_Flash);
        if(count%32==0) printf("\n");
        if(count == 255){
            
            printf("     page %d\n", count_page);
            count = 0;
            if(count_page==15&&i<num_of_Sector*4096-1){ 
                count_page=0;
                Sector++;
                printf("\n+++++++++++++++++++++++++++Sector %u+++++++++++++++++++++++++++++++ \n\n", Sector);
            }
            else count_page++;
        }
        else count++;
    }
    FLASH_CS_N_SetHigh();
}
void Flash_Sector_Program(uint32_t add, char Flash_4Kb[4096] ){
    for(int i=0;i<16;i++){
        Write_Enable();
        FLASH_CS_N_SetLow();
        spi2_exchangeByte(Page_Program);
        spi2_exchangeByte((add+i*256)>>16);
        spi2_exchangeByte((add+i*256)>>8);
        spi2_exchangeByte(add+i*256);

        for(int j=0;j<256;j++){
            spi2_exchangeByte(Flash_4Kb[j+i*256]);
        }
        FLASH_CS_N_SetHigh();
        while(Busy_flag());
        Write_Disable();
    }
}
void Save_Sector_to_String(uint32_t add, char Flash_4Kb[4096]){
    add &=0xFFF000;
    FLASH_CS_N_SetLow();
    spi2_exchangeByte(High_Speed_Read);
    spi2_exchangeByte(add>>16);
    spi2_exchangeByte(add>>8);
    spi2_exchangeByte(add);
    
    spi2_exchangeByte(0x00);
    for(int i=0;i<4096;i++){
            Flash_4Kb[i]=spi2_exchangeByte(0x00);
    }
    FLASH_CS_N_SetHigh();
}
void Flash_replace_String(uint32_t add, int vitri, char new_str[4096]){
    char Flash_4Kb[4096];
    add &=0xFFF000;
    Save_Sector_to_String(add, Flash_4Kb);
    bool Extra_flag=1;
    for(int i=0; i<strlen(new_str); i++){
        if((vitri+i)>=4096&&Extra_flag){
            Save_Sector_to_String(add+4096, Extra_Sector);
            Extra_flag = 0;
            i_extra = i;
            Extra_Sector[0] = new_str[i];
        }
        else if(!Extra_flag){
            Extra_Sector[i-i_extra] = new_str[i];
        }
        else Flash_4Kb[vitri+i]= new_str[i];
    }
    Erase_4Kb_mem(add);
    Flash_Sector_Program(add, Flash_4Kb);
    if(!Extra_flag){    
        Erase_4Kb_mem(add+4096);
        Flash_Sector_Program(add+4096, Extra_Sector);
    }
}


void Flash_Print_String(uint32_t add, char str[256]){
    
    FLASH_CS_N_SetLow();
    spi2_exchangeByte(High_Speed_Read);
    spi2_exchangeByte(add>>16);
    spi2_exchangeByte(add>>8);
    spi2_exchangeByte(add);
    
    spi2_exchangeByte(0x00);

    for(int i=0;i<strlen(str);i++){
        printf("%c",spi2_exchangeByte(str[i]));
    }
    printf("%d",strlen(str));
    printf("\n");
    
    FLASH_CS_N_SetHigh();
}

uint32_t JEDEC_ID(){
    uint32_t JDEC_ID;
      JDEC_ID = Flash_ExchangeNByte(JEDEC_ID_Read,4);
    return JDEC_ID;
}

void Flash_Write_Full_T_PageProgram(uint32_t add, uint32_t num_of_Sector){
    add &=0xFFF000;
    uint32_t ini_Add = add;
    for(add; add < 4096*num_of_Sector + ini_Add; add=add+256){
        Write_Enable();
        FLASH_CS_N_SetLow();
        spi2_exchangeByte(Page_Program);
        char t = 't';
        spi2_exchangeByte(add>>16);
        spi2_exchangeByte(add>>8);
        spi2_exchangeByte(add);

        for(int i=0;i<256;i++){
            spi2_exchangeByte(t);
        }
        FLASH_CS_N_SetHigh();
        while(Busy_flag());
        Write_Disable();
    }
}
void Print_Replaced_String(char get_add[255],char get_location[255],char get_str[4096], uint32_t ad0){
            int add_conv = atoi(get_add);
            int location_conv = atoi(get_location);
            printf("address: 0x%x\n",add_conv);
            printf("Location: %d\n",location_conv);
            printf("String replaced: ");
            for(int i=0; i<strlen(get_str); i++){
                printf("%c",get_str[i]);
            }
            printf("\n");
            Flash_replace_String(add_conv, location_conv, get_str);//write str s from positon 4094
            Flash_Print_Sector(ad0,3);//write first 5 sector
            printf("Enter code follow: address(0-524287)-location(0-4096)-string\n");
}
void Detech_add_loc_str(int section, uint32_t count_digit, char Buffer_read[255], char get_add[255],char get_location[255],char get_str[4096]){
    switch(section){
        case 0:
            get_add[count_digit] = Buffer_read[count_digit];
            break;
        case 1:
            get_location[count_digit] = Buffer_read[count_digit];
            break;    
        case 2:
            get_str[count_digit] = Buffer_read[count_digit];
            break; 
        default:
            printf("wrong code");
            break;        
    }
}
