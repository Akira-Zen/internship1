void Telit_ReceiveDataFromUart3(char *buffer_Uart3) {
    if (UART3_IsRxReady()) {
        *(buffer_Uart3) = UART3_Read();
        UART3_Write(*(buffer_Uart3));
    }
}
