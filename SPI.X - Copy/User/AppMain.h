#ifndef APPMAIN_H
#define APPMAIN_H

#include "Common/Debug.h"
#include "System/PIC32MM_Gpio.h"
#include "Buttons/Buttons.h"
#include "System/TickTimer.h"
#include "System/TaskManager.h"
#include "System/softWdt.h"
#include "Common/Utils.h"
#include "VCP_Debug.h"
#include "VCP.h"
#include "i2c1_driver.h"
#include "drivers/i2c_master.h"
#include  "../../ARDI/APP.X/mcc_generated_files/drivers/i2c_simple_master.h"
#include "../mcc_generated_files/drivers/i2c_types.h"

//#include "i2c1_driver.h"
//#include "drivers/i2c_simple_master.h"

public void APP_Main_Initialize(void); // Called by KIT_Tasks(), after global interrupt enabling and after start-up delay
public void APP_Main_Tasks(void);

#endif