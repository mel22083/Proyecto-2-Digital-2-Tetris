/*
 * fatfs_sd.c
 *
 *  Created on: Apr 18, 2026
 *      Author: willi
 */


#define TRUE  1
#define FALSE 0
#define bool BYTE

#include "stm32f4xx_hal.h"
#include "diskio.h"
#include "fatfs_sd.h"

volatile uint16_t Timer1, Timer2;
static volatile DSTATUS Stat = STA_NOINIT;
static uint8_t CardType;
static uint8_t PowerFlag = 0;

static void SELECT(void) {
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
}

static void DESELECT(void) {
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
}

static void SPI_TxByte(uint8_t data) {
    while(!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE));
    HAL_SPI_Transmit(HSPI_SDCARD, &data, 1, SPI_TIMEOUT);
}

static void SPI_TxBuffer(uint8_t *buffer, uint16_t len) {
    while(!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE));
    HAL_SPI_Transmit(HSPI_SDCARD, buffer, len, SPI_TIMEOUT);
}

static uint8_t SPI_comandanteByte(void) {
    uint8_t dummy, data;
    dummy = 0xFF;
    while(!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE));
    HAL_SPI_TransmitReceive(HSPI_SDCARD, &dummy, &data, 1, SPI_TIMEOUT);
    return data;
}

static void SPI_comandanteBytePtr(uint8_t *buff) {
    *buff = SPI_comandanteByte();
}

static uint8_t SD_ReadyWait(void) {
    uint8_t res;
    Timer2 = 500;
    do {
        res = SPI_comandanteByte();
    } while ((res != 0xFF) && Timer2);
    return res;
}

static void SD_PowerOn(void) {
    uint8_t args[6];
    uint32_t cnt = 0x1FFF;

    DESELECT();
    for(int i = 0; i < 10; i++)
        SPI_TxByte(0xFF);

    SELECT();

    args[0] = CMD0;
    args[1] = 0;
    args[2] = 0;
    args[3] = 0;
    args[4] = 0;
    args[5] = 0x95;

    SPI_TxBuffer(args, sizeof(args));

    while ((SPI_comandanteByte() != 0x01) && cnt)
        cnt--;

    DESELECT();
    SPI_TxByte(0xFF);
    PowerFlag = 1;
}

static void SD_PowerOff(void) {
    PowerFlag = 0;
}

static uint8_t SD_CheckPower(void) {
    return PowerFlag;
}

static bool SD_RxDataBlock(BYTE *buff, UINT len) {
    uint8_t token;
    Timer1 = 200;
    do {
        token = SPI_comandanteByte();
    } while((token == 0xFF) && Timer1);

    if(token != 0xFE) return FALSE;

    while(len--)
        SPI_comandanteBytePtr(buff++);

    SPI_comandanteByte();
    SPI_comandanteByte();
    return TRUE;
}

#if _USE_WRITE == 1
static bool SD_TxDataBlock(const uint8_t *buff, BYTE token) {
    uint8_t resp = 0xFF;
    uint8_t i = 0;

    if (SD_ReadyWait() != 0xFF) return FALSE;

    SPI_TxByte(token);

    if (token != 0xFD) {
        SPI_TxBuffer((uint8_t*)buff, 512);
        SPI_comandanteByte();
        SPI_comandanteByte();

        while (i <= 64) {
            resp = SPI_comandanteByte();
            if ((resp & 0x1F) == 0x05) break;
            i++;
        }

        Timer1 = 200;
        while ((SPI_comandanteByte() == 0) && Timer1);
    }

    if ((resp & 0x1F) == 0x05) return TRUE;
    return FALSE;
}
#endif

static BYTE SD_SendCmd(BYTE cmd, uint32_t arg) {
    uint8_t crc, res;

    if (SD_ReadyWait() != 0xFF) return 0xFF;

    SPI_TxByte(cmd);
    SPI_TxByte((uint8_t)(arg >> 24));
    SPI_TxByte((uint8_t)(arg >> 16));
    SPI_TxByte((uint8_t)(arg >> 8));
    SPI_TxByte((uint8_t)arg);

    if(cmd == CMD0)      crc = 0x95;
    else if(cmd == CMD8) crc = 0x87;
    else                 crc = 1;

    SPI_TxByte(crc);

    if (cmd == CMD12) SPI_comandanteByte();

    uint8_t n = 10;
    do {
        res = SPI_comandanteByte();
    } while ((res & 0x80) && --n);

    return res;
}

DSTATUS SD_disk_initialize(BYTE drv) {
    uint8_t n, type, ocr[4];

    if(drv) return STA_NOINIT;
    if(Stat & STA_NODISK) return Stat;

    SD_PowerOn();
    SELECT();

    type = 0;

    if (SD_SendCmd(CMD0, 0) == 1) {
        Timer1 = 1000;

        if (SD_SendCmd(CMD8, 0x1AA) == 1) {
            for (n = 0; n < 4; n++)
                ocr[n] = SPI_comandanteByte();

            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
                do {
                    if (SD_SendCmd(CMD55, 0) <= 1 &&
                        SD_SendCmd(CMD41, 1UL << 30) == 0) break;
                } while (Timer1);

                if (Timer1 && SD_SendCmd(CMD58, 0) == 0) {
                    for (n = 0; n < 4; n++)
                        ocr[n] = SPI_comandanteByte();
                    type = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
                }
            }
        } else {
            type = (SD_SendCmd(CMD55, 0) <= 1 &&
                    SD_SendCmd(CMD41, 0) <= 1) ? CT_SD1 : CT_MMC;

            do {
                if (type == CT_SD1) {
                    if (SD_SendCmd(CMD55, 0) <= 1 &&
                        SD_SendCmd(CMD41, 0) == 0) break;
                } else {
                    if (SD_SendCmd(CMD1, 0) == 0) break;
                }
            } while (Timer1);

            if (!Timer1 || SD_SendCmd(CMD16, 512) != 0) type = 0;
        }
    }

    CardType = type;
    DESELECT();
    SPI_comandanteByte();

    if (type) {
        Stat &= ~STA_NOINIT;
    } else {
        SD_PowerOff();
    }

    return Stat;
}

DSTATUS SD_disk_status(BYTE drv) {
    if (drv) return STA_NOINIT;
    return Stat;
}

DRESULT SD_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
    if (pdrv || !count) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;
    if (!(CardType & CT_BLOCK)) sector *= 512;

    SELECT();

    if (count == 1) {
        if ((SD_SendCmd(CMD17, sector) == 0) && SD_RxDataBlock(buff, 512))
            count = 0;
    } else {
        if (SD_SendCmd(CMD18, sector) == 0) {
            do {
                if (!SD_RxDataBlock(buff, 512)) break;
                buff += 512;
            } while (--count);
            SD_SendCmd(CMD12, 0);
        }
    }

    DESELECT();
    SPI_comandanteByte();
    return count ? RES_ERROR : RES_OK;
}

#if _USE_WRITE == 1
DRESULT SD_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) {
    if (pdrv || !count) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;
    if (Stat & STA_PROTECT) return RES_WRPRT;
    if (!(CardType & CT_BLOCK)) sector *= 512;

    SELECT();

    if (count == 1) {
        if ((SD_SendCmd(CMD24, sector) == 0) && SD_TxDataBlock(buff, 0xFE))
            count = 0;
    } else {
        if (CardType & CT_SD1) {
            SD_SendCmd(CMD55, 0);
            SD_SendCmd(CMD23, count);
        }

        if (SD_SendCmd(CMD25, sector) == 0) {
            do {
                if(!SD_TxDataBlock(buff, 0xFC)) break;
                buff += 512;
            } while (--count);

            if(!SD_TxDataBlock(0, 0xFD))
                count = 1;
        }
    }

    DESELECT();
    SPI_comandanteByte();
    return count ? RES_ERROR : RES_OK;
}
#endif

DRESULT SD_disk_ioctl(BYTE drv, BYTE ctrl, void *buff) {
    DRESULT res;
    uint8_t n, csd[16], *ptr = buff;

    if (drv) return RES_PARERR;
    res = RES_ERROR;

    if (ctrl == CTRL_POWER) {
        switch (*ptr) {
            case 0: SD_PowerOff(); res = RES_OK; break;
            case 1: SD_PowerOn();  res = RES_OK; break;
            case 2:
                *(ptr + 1) = SD_CheckPower();
                res = RES_OK;
                break;
            default: res = RES_PARERR;
        }
    } else {
        if (Stat & STA_NOINIT) return RES_NOTRDY;

        SELECT();

        switch (ctrl) {
            case GET_SECTOR_COUNT:
                if ((SD_SendCmd(CMD9, 0) == 0) && SD_RxDataBlock(csd, 16)) {
                    if ((csd[0] >> 6) == 1) {
                        DWORD c_size;
                        c_size = (DWORD)(csd[7] & 0x3F) << 16 |
                                 (WORD)csd[8] << 8 | csd[9];
                        *(DWORD*)buff = (c_size + 1) << 10;
                    } else {
                        WORD csize;
                        n = (csd[5] & 15) + ((csd[10] & 128) >> 7) +
                            ((csd[9] & 3) << 1) + 2;
                        csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) +
                                ((WORD)(csd[6] & 3) << 10) + 1;
                        *(DWORD*)buff = (DWORD)csize << (n - 9);
                    }
                    res = RES_OK;
                }
                break;
            case GET_SECTOR_SIZE:
                *(WORD*)buff = 512;
                res = RES_OK;
                break;
            case CTRL_SYNC:
                if (SD_ReadyWait() == 0xFF) res = RES_OK;
                break;
            case MMC_GET_CSD:
                if (SD_SendCmd(CMD9, 0) == 0 && SD_RxDataBlock(ptr, 16))
                    res = RES_OK;
                break;
            case MMC_GET_CID:
                if (SD_SendCmd(CMD10, 0) == 0 && SD_RxDataBlock(ptr, 16))
                    res = RES_OK;
                break;
            case MMC_GET_OCR:
                if (SD_SendCmd(CMD58, 0) == 0) {
                    for (n = 0; n < 4; n++)
                        *ptr++ = SPI_comandanteByte();
                    res = RES_OK;
                }
                break;
            default:
                res = RES_PARERR;
        }

        DESELECT();
        SPI_comandanteByte();
    }

    return res;
}
