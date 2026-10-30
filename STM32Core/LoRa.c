#include "LoRa.h"
#include "cmsis_os.h"

extern SPI_HandleTypeDef hspi2;

#define LORA_CS_LOW()      HAL_GPIO_WritePin(LoRa_CS_GPIO_Port, LoRa_CS_Pin, GPIO_PIN_RESET)
#define LORA_CS_HIGH()     HAL_GPIO_WritePin(LoRa_CS_GPIO_Port, LoRa_CS_Pin, GPIO_PIN_SET)
#define LORA_RST_LOW()     HAL_GPIO_WritePin(LoRa_Rst_GPIO_Port, LoRa_Rst_Pin, GPIO_PIN_RESET)
#define LORA_RST_HIGH()    HAL_GPIO_WritePin(LoRa_Rst_GPIO_Port, LoRa_Rst_Pin, GPIO_PIN_SET)

void LoRa_Reset(void)
{
    LORA_RST_LOW();
    HAL_Delay(10);
    LORA_RST_HIGH();
    HAL_Delay(10);
}

uint8_t LoRa_ReadRegister(uint8_t address)
{
    uint8_t tx = address & 0x7F;
    uint8_t rx = 0;
    LORA_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &tx, 1, 100);
    HAL_SPI_Receive(&hspi2, &rx, 1, 100);
    LORA_CS_HIGH();
    return rx;
}

void LoRa_WriteRegister(uint8_t address, uint8_t value)
{
    uint8_t tx[2] = { (uint8_t)(address | 0x80), value };
    LORA_CS_LOW();
    HAL_SPI_Transmit(&hspi2, tx, 2, 100);
    LORA_CS_HIGH();
}

uint8_t LoRa_Init(void)
{
    LoRa_Reset();

    uint8_t version = LoRa_ReadRegister(REG_VERSION);
    if (version != 0x12) return 0;

    LoRa_WriteRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);

    LoRa_WriteRegister(REG_FRF_MSB, 0x6C);
    LoRa_WriteRegister(REG_FRF_MID, 0x40);
    LoRa_WriteRegister(REG_FRF_LSB, 0x00);

    LoRa_WriteRegister(REG_PA_CONFIG, 0x8F);
    LoRa_WriteRegister(REG_MODEM_CONFIG_1, 0x72);
    LoRa_WriteRegister(REG_MODEM_CONFIG_2, 0x74);
    LoRa_WriteRegister(REG_MODEM_CONFIG_3, 0x04);
    LoRa_WriteRegister(0x39, 0x12);   // sync word

    LoRa_WriteRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
    return 1;
}

uint8_t LoRa_Transmit(uint8_t *payload, uint8_t length)
{
    LoRa_WriteRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
    LoRa_WriteRegister(REG_FIFO_TX_BASE_ADDR, 0x80);
    LoRa_WriteRegister(REG_FIFO_ADDR_PTR, 0x80);

    for (uint8_t i = 0; i < length; i++) {
        LoRa_WriteRegister(REG_FIFO, payload[i]);
    }
    LoRa_WriteRegister(REG_PAYLOAD_LENGTH, length);
    LoRa_WriteRegister(REG_DIO_MAPPING_1, 0x40);
    LoRa_WriteRegister(REG_IRQ_FLAGS, 0xFF);
    LoRa_WriteRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

    uint32_t timeout_count = 100;
    while (((LoRa_ReadRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0) && (timeout_count > 0)) {
        osDelay(10);
        timeout_count--;
    }

    LoRa_WriteRegister(REG_IRQ_FLAGS, 0xFF);
    LoRa_WriteRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);

    return (timeout_count > 0) ? 1 : 0;
}

void LoRa_Sleep(void)
{
    LoRa_WriteRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
}

void LoRa_StartReceive(void)
{
    LoRa_WriteRegister(REG_FIFO_RX_BASE_ADDR, 0x00);
    LoRa_WriteRegister(REG_FIFO_ADDR_PTR,     0x00);
    LoRa_WriteRegister(REG_DIO_MAPPING_1,     0x00);
    LoRa_WriteRegister(REG_IRQ_FLAGS,         0xFF);
    LoRa_WriteRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RXCONTINUOUS);
}

uint8_t LoRa_ReceivePacket(uint8_t *buf, uint8_t maxlen)
{
    uint8_t irq = LoRa_ReadRegister(REG_IRQ_FLAGS);
    if (!(irq & IRQ_RX_DONE_MASK)) {
        return 0;
    }

    uint8_t len = LoRa_ReadRegister(REG_RX_NB_BYTES);
    uint8_t fifo_addr = LoRa_ReadRegister(REG_FIFO_RX_CURRENT_ADDR);

    LoRa_WriteRegister(REG_IRQ_FLAGS, 0xFF);

    if (irq & IRQ_PAYLOAD_CRC_ERROR_MASK) {
        return 0;
    }
    if (len > maxlen) len = maxlen;
    if (len == 0) return 0;

    LoRa_WriteRegister(REG_FIFO_ADDR_PTR, fifo_addr);

    uint8_t tx = REG_FIFO & 0x7F;
    LORA_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &tx, 1, 100);
    HAL_SPI_Receive(&hspi2, buf, len, 100);
    LORA_CS_HIGH();

    return len;
}
