/*
 * ili9341.c
 *
 *  Created on: Apr 7, 2026
 *      Author: willi
 */


#include <stdlib.h> // malloc()
#include <string.h> // memset()
#include "pgmspace.h"
#include "ili9341.h"
#include "main.h"
#include <stdbool.h>


#define LCD_WR_PORT    GPIOA
#define LCD_WR_PIN     GPIO_PIN_1
#define LCD_WR_L()     (LCD_WR_PORT->BSRR = (LCD_WR_PIN<<16))
#define LCD_WR_H()     (LCD_WR_PORT->BSRR =  LCD_WR_PIN)

#define LCD_RS_PORT    GPIOA
#define LCD_RS_PIN     GPIO_PIN_4
#define LCD_RS_L()     (LCD_RS_PORT->BSRR = (LCD_RS_PIN<<16))
#define LCD_RS_H()     (LCD_RS_PORT->BSRR =  LCD_RS_PIN)

#define LCD_CS_PORT    GPIOB
#define LCD_CS_PIN     GPIO_PIN_0
#define LCD_CS_L()     (LCD_CS_PORT->BSRR = (LCD_CS_PIN<<16))
#define LCD_CS_H()     (LCD_CS_PORT->BSRR =  LCD_CS_PIN)

// ——— Máscaras del bus de datos ———
static const uint32_t DATA_PA = LCD_D0_Pin | LCD_D2_Pin | LCD_D7_Pin;
static const uint32_t DATA_PB = LCD_D3_Pin | LCD_D4_Pin | LCD_D5_Pin | LCD_D6_Pin;
static const uint32_t DATA_PC = LCD_D1_Pin;


extern const uint8_t smallFont[1140];
extern const uint16_t bigFont[1520];

//***************************************************************************************************************************************
// Función para inicializar LCD
//***************************************************************************************************************************************
void LCD_Init(void) {

	//****************************************
	// Secuencia de Inicialización
	//****************************************
	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin | LCD_WR_Pin | LCD_RS_Pin,
			GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(5);
	HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(150);
	HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);

	//****************************************
	LCD_CMD(0xE9);  // SETPANELRELATED
	LCD_DATA(0x20);
	//****************************************
	LCD_CMD(0x11); // Exit Sleep SLEEP OUT (SLPOUT)
	HAL_Delay(100);
	//****************************************
	LCD_CMD(0xD1);    // (SETVCOM)
	LCD_DATA(0x00);
	LCD_DATA(0x71);
	LCD_DATA(0x19);
	//****************************************
	LCD_CMD(0xD0);   // (SETPOWER)
	LCD_DATA(0x07);
	LCD_DATA(0x01);
	LCD_DATA(0x08);
	//****************************************
	LCD_CMD(0x36);  // (MEMORYACCESS)
	LCD_DATA(0x48); // LCD_DATA(0x19);
	//****************************************
	LCD_CMD(0x3A); // Set_pixel_format (PIXELFORMAT)
	LCD_DATA(0x05); // color setings, 05h - 16bit pixel, 11h - 3bit pixel
	//****************************************
	LCD_CMD(0xC1);    // (POWERCONTROL2)
	LCD_DATA(0x10);
	LCD_DATA(0x10);
	LCD_DATA(0x02);
	LCD_DATA(0x02);
	//****************************************
	LCD_CMD(0xC0); // Set Default Gamma (POWERCONTROL1)
	LCD_DATA(0x00);
	LCD_DATA(0x35);
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA(0x01);
	LCD_DATA(0x02);
	//****************************************
	LCD_CMD(0xC5); // Set Frame Rate (VCOMCONTROL1)
	LCD_DATA(0x04); // 72Hz
	//****************************************
	LCD_CMD(0xD2); // Power Settings  (SETPWRNORMAL)
	LCD_DATA(0x01);
	LCD_DATA(0x44);
	//****************************************
	LCD_CMD(0xC8); //Set Gamma  (GAMMASET)
	LCD_DATA(0x04);
	LCD_DATA(0x67);
	LCD_DATA(0x35);
	LCD_DATA(0x04);
	LCD_DATA(0x08);
	LCD_DATA(0x06);
	LCD_DATA(0x24);
	LCD_DATA(0x01);
	LCD_DATA(0x37);
	LCD_DATA(0x40);
	LCD_DATA(0x03);
	LCD_DATA(0x10);
	LCD_DATA(0x08);
	LCD_DATA(0x80);
	LCD_DATA(0x00);
	//****************************************
	LCD_CMD(0x2A); // Set_column_address 320px (CASET)
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA(0x01);
	LCD_DATA(0x3F);
	//****************************************
	LCD_CMD(0x2B); // Set_page_address 480px (PASET)
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA(0x01);
	LCD_DATA(0xE0);
	//  LCD_DATA(0x8F);
	LCD_CMD(0x29); //display on
	LCD_CMD(0x2C); //display on

	LCD_CMD(ILI9341_INVOFF); //Invert Off
	HAL_Delay(120);
	LCD_CMD(ILI9341_SLPOUT);    //Exit Sleep
	HAL_Delay(120);
	LCD_CMD(ILI9341_DISPON);    //Display on
	HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
}
//***************************************************************************************************************************************
// Función para enviar comandos a la LCD - parámetro (comando)
//***************************************************************************************************************************************
void LCD_CMD(uint8_t cmd) {
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LCD_WR_GPIO_Port, LCD_WR_Pin, GPIO_PIN_RESET);

	if ((cmd & (1 << 0)) == 1) {
		HAL_GPIO_WritePin(LCD_D0_GPIO_Port, LCD_D0_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D0_GPIO_Port, LCD_D0_Pin, GPIO_PIN_RESET);
	}
	if ((cmd & (1 << 1)) == 0x02) {
		HAL_GPIO_WritePin(LCD_D1_GPIO_Port, LCD_D1_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D1_GPIO_Port, LCD_D1_Pin, GPIO_PIN_RESET);
	}
	if ((cmd & (1 << 2)) == 0x04) {
		HAL_GPIO_WritePin(LCD_D2_GPIO_Port, LCD_D2_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D2_GPIO_Port, LCD_D2_Pin, GPIO_PIN_RESET);
	}
	if ((cmd & (1 << 3)) == 0x08) {
		HAL_GPIO_WritePin(LCD_D3_GPIO_Port, LCD_D3_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D3_GPIO_Port, LCD_D3_Pin, GPIO_PIN_RESET);
	}
	if ((cmd & (1 << 4)) == 0x10) {
		HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, GPIO_PIN_RESET);
	}
	if ((cmd & (1 << 5)) == 0x20) {
		HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, GPIO_PIN_RESET);
	}
	if ((cmd & (1 << 6)) == 0x40) {
		HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, GPIO_PIN_RESET);
	}
	if ((cmd & (1 << 7)) == 0x80) {
		HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, GPIO_PIN_RESET);
	}
	//GPIO_PortB_DATA_R = cmd;
	HAL_GPIO_WritePin(LCD_WR_GPIO_Port, LCD_WR_Pin, GPIO_PIN_SET);

}
//***************************************************************************************************************************************
// Función para enviar datos a la LCD - parámetro (dato)
//***************************************************************************************************************************************
void LCD_DATA(uint8_t data) {
	//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LCD_WR_GPIO_Port, LCD_WR_Pin, GPIO_PIN_RESET);
	if ((data & (1 << 0)) == 1) {
		HAL_GPIO_WritePin(LCD_D0_GPIO_Port, LCD_D0_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D0_GPIO_Port, LCD_D0_Pin, GPIO_PIN_RESET);
	}
	if ((data & (1 << 1)) == 0x02) {
		HAL_GPIO_WritePin(LCD_D1_GPIO_Port, LCD_D1_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D1_GPIO_Port, LCD_D1_Pin, GPIO_PIN_RESET);
	}
	if ((data & (1 << 2)) == 0x04) {
		HAL_GPIO_WritePin(LCD_D2_GPIO_Port, LCD_D2_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D2_GPIO_Port, LCD_D2_Pin, GPIO_PIN_RESET);
	}
	if ((data & (1 << 3)) == 0x08) {
		HAL_GPIO_WritePin(LCD_D3_GPIO_Port, LCD_D3_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D3_GPIO_Port, LCD_D3_Pin, GPIO_PIN_RESET);
	}
	if ((data & (1 << 4)) == 0x10) {
		HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, GPIO_PIN_RESET);
	}
	if ((data & (1 << 5)) == 0x20) {
		HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, GPIO_PIN_RESET);
	}
	if ((data & (1 << 6)) == 0x40) {
		HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, GPIO_PIN_RESET);
	}
	if ((data & (1 << 7)) == 0x80) {
		HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, GPIO_PIN_RESET);
	}
	HAL_GPIO_WritePin(LCD_WR_GPIO_Port, LCD_WR_Pin, GPIO_PIN_SET);
}
//***************************************************************************************************************************************
// Función para definir rango de direcciones de memoria con las cuales se trabajara (se define una ventana)
//***************************************************************************************************************************************
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2,
		unsigned int y2) {
	LCD_CMD(0x2a); // Set_column_address 4 parameters
	LCD_DATA(x1 >> 8);
	LCD_DATA(x1);
	LCD_DATA(x2 >> 8);
	LCD_DATA(x2);
	LCD_CMD(0x2b); // Set_page_address 4 parameters
	LCD_DATA(y1 >> 8);
	LCD_DATA(y1);
	LCD_DATA(y2 >> 8);
	LCD_DATA(y2);
	LCD_CMD(0x2c); // Write_memory_start
}
//***************************************************************************************************************************************
// Función para borrar la pantalla - parámetros (color)
//***************************************************************************************************************************************
void LCD_Clear(unsigned int c) {
    unsigned int x, y;
    LCD_CMD(0x02c);
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    SetWindows(0, 0, 319, 479);   // 320x480
    for (x = 0; x < 320; x++)
        for (y = 0; y < 480; y++) {
            LCD_DATA(c >> 8);
            LCD_DATA(c);
        }
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}
//***************************************************************************************************************************************
// Función para dibujar una línea horizontal - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
    LCD_CMD(0x02c);
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    SetWindows(x, y, x + l - 1, y);
    for (unsigned int i = 0; i < l; i++) {  // <-- itera solo 'l' veces
        LCD_DATA(c >> 8);
        LCD_DATA(c);
    }
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}
//***************************************************************************************************************************************
// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
    LCD_CMD(0x02c);
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    SetWindows(x, y, x, y + l - 1);   // corregido
    for (unsigned int i = 0; i < l; i++) {
        LCD_DATA(c >> 8);
        LCD_DATA(c);
    }
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		unsigned int c) {
	H_line(x, y, w, c);
	H_line(x, y + h, w, c);
	V_line(x, y, h, c);
	V_line(x + w, y, h, c);
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
/*void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
 unsigned int i;
 for (i = 0; i < h; i++) {
 H_line(x  , y  , w, c);
 H_line(x  , y+i, w, c);
 }
 }
 */

void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		unsigned int c) {
	LCD_CMD(0x02c); // write_memory_start
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

	unsigned int x2, y2;
	x2 = x + w;
	y2 = y + h;
	SetWindows(x, y, x2 - 1, y2 - 1);
	unsigned int k = w * h * 2 - 1;
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			LCD_DATA(c >> 8);
			LCD_DATA(c);

			//LCD_DATA(bitmap[k]);
			k = k - 2;
		}
	}
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}
//***************************************************************************************************************************************
// Función para dibujar texto - parámetros ( texto, coordenada x, cordenada y, color, background)
//***************************************************************************************************************************************
void LCD_Print(char *text, int x, int y, int fontSize, int color,
		int background) {

	int fontXSize;
	int fontYSize;

	if (fontSize == 1) {
		fontXSize = fontXSizeSmal;
		fontYSize = fontYSizeSmal;
	}
	if (fontSize == 2) {
		fontXSize = fontXSizeBig;
		fontYSize = fontYSizeBig;
	}
	if (fontSize == 3) {
			fontXSize = fontXSizeNum;
			fontYSize = fontYSizeNum;
		}

	char charInput;
	int cLength = strlen(text);
	//Serial.println(cLength, DEC);
	int charDec;
	int c;
	//int charHex;
	char char_array[cLength + 1];
	for(int i = 0; text[i] != '\0'; i++){
		char_array[i]=text[i];
	}

	//text.toCharArray(char_array, cLength + 1);

	for (int i = 0; i < cLength; i++) {
		charInput = char_array[i];
		//Serial.println(char_array[i]);
		charDec = (int) charInput;
		HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
		SetWindows(x + (i * fontXSize), y, x + (i * fontXSize) + fontXSize - 1, y + fontYSize);
		long charHex1;
		for (int n = 0; n < fontYSize; n++) {
			if (fontSize == 1) {
				charHex1 = pgm_read_word_near(
						smallFont + ((charDec - 32) * fontYSize) + n);
			}
			if (fontSize == 2) {
				charHex1 = pgm_read_word_near(
						bigFont + ((charDec - 32) * fontYSize) + n);
			}
			for (int t = 1; t < fontXSize + 1; t++) {
				if ((charHex1 & (1 << (fontXSize - t))) > 0) {
					c = color;
				} else {
					c = background;
				}
				LCD_DATA(c >> 8);
				LCD_DATA(c);
			}
		}
		HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	}
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//***************************************************************************************************************************************
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width,
		unsigned int height, unsigned char bitmap[]) {
	LCD_CMD(0x02c); // write_memory_start
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

	unsigned int x2, y2;
	x2 = x + width;
	y2 = y + height;
	SetWindows(x, y, x2 - 1, y2 - 1);
	unsigned int k = 0;

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			LCD_DATA(bitmap[k]);
			LCD_DATA(bitmap[k + 1]);
			//LCD_DATA(bitmap[k]);
			k = k + 2;
		}
	}
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}
//***************************************************************************************************************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//***************************************************************************************************************************************
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],
		int columns, int index, char flip, char offset) {
	LCD_CMD(0x02c); // write_memory_start
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

	unsigned int x2, y2;
	x2 = x + width;
	y2 = y + height;
	SetWindows(x, y, x2 - 1, y2 - 1);
	int k = 0;
	int ancho = ((width * columns));
	if (flip) {
		for (int j = 0; j < height; j++) {
			k = (j * (ancho) + index * width - 1 - offset) * 2;
			k = k + width * 2;
			for (int i = 0; i < width; i++) {
				LCD_DATA(bitmap[k]);
				LCD_DATA(bitmap[k + 1]);
				k = k - 2;
			}
		}
	} else {
		for (int j = 0; j < height; j++) {
			k = (j * (ancho) + index * width + 1 + offset) * 2;
			for (int i = 0; i < width; i++) {
				LCD_DATA(bitmap[k]);
				LCD_DATA(bitmap[k + 1]);
				k = k + 2;
			}
		}

	}
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void FillRectFast(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
    uint8_t hi = c >> 8;
    uint8_t lo = c & 0xFF;

    LCD_CS_L();              // CS = 0
    SetWindows(x, y, x + w - 1, y + h - 1);  // establece CASET/PASET y envía 0x2C
    LCD_RS_H();              // RS = 1 ➞ ¡ahora sí datos!

    for (unsigned int j = 0; j < h; j++) {
        for (unsigned int i = 0; i < w; i++) {
            // ——— Byte alto ———
            LCD_WR_L();     // WR = 0
            GPIOA->ODR = (GPIOA->ODR & ~DATA_PA)
                       | (((hi >> 0) & 1) ? LCD_D0_Pin : 0)
                       | (((hi >> 2) & 1) ? LCD_D2_Pin : 0)
                       | (((hi >> 7) & 1) ? LCD_D7_Pin : 0);
            GPIOB->ODR = (GPIOB->ODR & ~DATA_PB)
                       | (((hi >> 3) & 1) ? LCD_D3_Pin : 0)
                       | (((hi >> 4) & 1) ? LCD_D4_Pin : 0)
                       | (((hi >> 5) & 1) ? LCD_D5_Pin : 0)
                       | (((hi >> 6) & 1) ? LCD_D6_Pin : 0);
            GPIOC->ODR = (GPIOC->ODR & ~DATA_PC)
                       | (((hi >> 1) & 1) ? LCD_D1_Pin : 0);
            LCD_WR_H();     // WR = 1

            // ——— Byte bajo ———
            LCD_WR_L();
            GPIOA->ODR = (GPIOA->ODR & ~DATA_PA)
                       | (((lo >> 0) & 1) ? LCD_D0_Pin : 0)
                       | (((lo >> 2) & 1) ? LCD_D2_Pin : 0)
                       | (((lo >> 7) & 1) ? LCD_D7_Pin : 0);
            GPIOB->ODR = (GPIOB->ODR & ~DATA_PB)
                       | (((lo >> 3) & 1) ? LCD_D3_Pin : 0)
                       | (((lo >> 4) & 1) ? LCD_D4_Pin : 0)
                       | (((lo >> 5) & 1) ? LCD_D5_Pin : 0)
                       | (((lo >> 6) & 1) ? LCD_D6_Pin : 0);
            GPIOC->ODR = (GPIOC->ODR & ~DATA_PC)
                       | (((lo >> 1) & 1) ? LCD_D1_Pin : 0);
            LCD_WR_H();
        }
    }

    LCD_CS_H();  // CS = 1
}

void LCD_SpriteFast(int x, int y,
                    int width, int height,
                    const uint8_t *bitmap,
                    int columns, int index,
                    bool flipX, bool flipY,
                    int offset)
{
    // 1) Activo CS, defino ventana y arranco memoria
    LCD_CS_L();
    SetWindows(x, y, x + width - 1, y + height - 1);
    LCD_RS_H();  // ¡ahora van datos!

    const int stride = width * columns * 2;             // bytes por fila completa
    const uint8_t *basePtr = bitmap + (index * width + offset) * 2;

    for (int row = 0; row < height; row++) {
        // si flipY, mapeo la fila row→(height-1-row), si no, row→row
        int ry = flipY ? (height - 1 - row) : row;

        for (int col = 0; col < width; col++) {
            // si flipX, mapeo la columna col→(width-1-col), si no, col→col
            int cx = flipX ? (width - 1 - col) : col;

            // puntero al pixel (ry, cx)
            const uint8_t *p = basePtr + ry * stride + cx * 2;
            uint8_t hi = *p++;
            uint8_t lo = *p;

            // ——— envío byte alto ———
            LCD_WR_L();
            GPIOA->ODR = (GPIOA->ODR & ~DATA_PA)
                       | ((hi &    0x01) ? LCD_D0_Pin : 0)
                       | (((hi>>2) &0x01) ? LCD_D2_Pin : 0)
                       | (((hi>>7) &0x01) ? LCD_D7_Pin : 0);
            GPIOB->ODR = (GPIOB->ODR & ~DATA_PB)
                       | (((hi>>3) &0x01)? LCD_D3_Pin : 0)
                       | (((hi>>4) &0x01)? LCD_D4_Pin : 0)
                       | (((hi>>5) &0x01)? LCD_D5_Pin : 0)
                       | (((hi>>6) &0x01)? LCD_D6_Pin : 0);
            GPIOC->ODR = (GPIOC->ODR & ~DATA_PC)
                       | (((hi>>1) &0x01)? LCD_D1_Pin : 0);
            LCD_WR_H();

            // ——— envío byte bajo ———
            LCD_WR_L();
            GPIOA->ODR = (GPIOA->ODR & ~DATA_PA)
                       | ((lo &    0x01) ? LCD_D0_Pin : 0)
                       | (((lo>>2) &0x01) ? LCD_D2_Pin : 0)
                       | (((lo>>7) &0x01) ? LCD_D7_Pin : 0);
            GPIOB->ODR = (GPIOB->ODR & ~DATA_PB)
                       | (((lo>>3) &0x01)? LCD_D3_Pin : 0)
                       | (((lo>>4) &0x01)? LCD_D4_Pin : 0)
                       | (((lo>>5) &0x01)? LCD_D5_Pin : 0)
                       | (((lo>>6) &0x01)? LCD_D6_Pin : 0);
            GPIOC->ODR = (GPIOC->ODR & ~DATA_PC)
                       | (((lo>>1) &0x01)? LCD_D1_Pin : 0);
            LCD_WR_H();
        }
    }

    // 4) Desactivo CS
    LCD_CS_H();
}


void LCD_BitmapFast(unsigned int x, unsigned int y,
                    unsigned int width, unsigned int height,
                    const uint8_t *bitmap)
{
    // 1) Activo CS, defino ventana y arranco memoria
    LCD_CS_L();
    SetWindows(x, y, x + width - 1, y + height - 1);
    LCD_RS_H();  // ¡ahora sí van datos!

    // 2) Para cada fila
    for (unsigned int row = 0; row < height; row++) {
        // offset en bytes al inicio de la fila
        uint32_t base = row * width * 2;
        for (unsigned int col = 0; col < width; col++) {
            uint8_t hi = bitmap[base + col * 2];
            uint8_t lo = bitmap[base + col * 2 + 1];

            // ——— Byte alto ———
            LCD_WR_L();
            GPIOA->ODR = (GPIOA->ODR & ~DATA_PA)
                       | ((hi & 0x01) ? LCD_D0_Pin : 0)
                       | (((hi >> 2) & 0x01) ? LCD_D2_Pin : 0)
                       | (((hi >> 7) & 0x01) ? LCD_D7_Pin : 0);
            GPIOB->ODR = (GPIOB->ODR & ~DATA_PB)
                       | (((hi >> 3) & 0x01) ? LCD_D3_Pin : 0)
                       | (((hi >> 4) & 0x01) ? LCD_D4_Pin : 0)
                       | (((hi >> 5) & 0x01) ? LCD_D5_Pin : 0)
                       | (((hi >> 6) & 0x01) ? LCD_D6_Pin : 0);
            GPIOC->ODR = (GPIOC->ODR & ~DATA_PC)
                       | (((hi >> 1) & 0x01) ? LCD_D1_Pin : 0);
            LCD_WR_H();

            // ——— Byte bajo ———
            LCD_WR_L();
            GPIOA->ODR = (GPIOA->ODR & ~DATA_PA)
                       | ((lo & 0x01) ? LCD_D0_Pin : 0)
                       | (((lo >> 2) & 0x01) ? LCD_D2_Pin : 0)
                       | (((lo >> 7) & 0x01) ? LCD_D7_Pin : 0);
            GPIOB->ODR = (GPIOB->ODR & ~DATA_PB)
                       | (((lo >> 3) & 0x01) ? LCD_D3_Pin : 0)
                       | (((lo >> 4) & 0x01) ? LCD_D4_Pin : 0)
                       | (((lo >> 5) & 0x01) ? LCD_D5_Pin : 0)
                       | (((lo >> 6) & 0x01) ? LCD_D6_Pin : 0);
            GPIOC->ODR = (GPIOC->ODR & ~DATA_PC)
                       | (((lo >> 1) & 0x01) ? LCD_D1_Pin : 0);
            LCD_WR_H();
        }
    }

    // 3) Cierro CS
    LCD_CS_H();
}


//PRUEBA DE FUNCIÓN INESTABLE:




