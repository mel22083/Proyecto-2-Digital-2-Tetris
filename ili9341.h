/*
 * ili9341.h
 *
 *  Created on: Apr 7, 2026
 *      Author: willi
 */

#ifndef ILI9341_H
#define ILI9341_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

// ===============================
// COMANDOS BÁSICOS ILI9341
// ===============================
#define ILI9341_INVOFF  0x20
#define ILI9341_SLPOUT  0x11
#define ILI9341_DISPON  0x29

// ===============================
// CONFIGURACIÓN DE FUENTES
// ===============================
#define fontXSizeSmal 5
#define fontYSizeSmal 8

#define fontXSizeBig 11
#define fontYSizeBig 16

#define fontXSizeNum 16
#define fontYSizeNum 24

// ===============================
// FUNCIONES PRINCIPALES
// ===============================
void LCD_Init(void);

void LCD_CMD(uint8_t cmd);
void LCD_DATA(uint8_t data);

void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);

void LCD_Clear(unsigned int color);

void H_line(unsigned int x, unsigned int y, unsigned int length, unsigned int color);
void V_line(unsigned int x, unsigned int y, unsigned int length, unsigned int color);

void Rect(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color);
void FillRectFast(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color);

// ===============================
// TEXTO (requiere fuentes)
// ===============================
void LCD_Print(char *text, int x, int y, int fontSize, int color, int background);

// ===============================
// IMÁGENES
// ===============================
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]);

void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],
                int columns, int index, char flip, char offset);

// ===============================
// FUNCIONES RÁPIDAS (OPTIMIZADAS)
// ===============================
void FillRect(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color);

void LCD_BitmapFast(unsigned int x, unsigned int y,
                    unsigned int width, unsigned int height,
                    const uint8_t *bitmap);

void LCD_SpriteFast(int x, int y,
                    int width, int height,
                    const uint8_t *bitmap,
                    int columns, int index,
                    bool flipX, bool flipY,
                    int offset);

#endif
