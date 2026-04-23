/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "math.h"
#include <stdio.h>
#include "ili9341.h"
#include "fatfs.h"
#include "menu_image.h"
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TIM_FREQ     84000000
#define ARR          100

#define SCREEN_W     240
#define SCREEN_H     320

#define BLOQUE       16
#define COLS         10
#define FILAS        20

#define TABLERO_W    (COLS * BLOQUE)
#define TABLERO_H    (FILAS * BLOQUE)

#define TETRIS_X     0
#define TETRIS_Y     0

#define PANEL_X      (TABLERO_W + 4)
#define PANEL_W      (SCREEN_W - PANEL_X - 2)
#define PANEL_Y      0

#define LINES_Y      4
#define LINES_H      60

#define SCORE_Y      (LINES_Y + LINES_H + 4)
#define SCORE_H      80

#define NEXT_Y       (SCORE_Y + SCORE_H + 4)
#define NEXT_H       90

#define LEVEL_Y      (NEXT_Y + NEXT_H + 4)
#define LEVEL_H      60

#define HOLD_Y       (LEVEL_Y + LEVEL_H + 4)
#define HOLD_H       70


#define COLOR_VACIO  0x0000
#define COLOR_BORDE  0x8410
#define COLOR_BORDE2 0x4208
#define COLOR_PANEL  0x2104
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
uint8_t tablero[FILAS][COLS] = {0};

const uint16_t coloresPieza[8] = {
	    0x0000, 0x07FF, 0xFFE0, 0xF81F,
	    0x07E0, 0xF800, 0xFD20, 0x001F
	};

const int8_t piezas[7][4][4][2] = { // tipo, rotacion, bloque, coordenada
    {{{0,1},{1,1},{2,1},{3,1}},{{2,0},{2,1},{2,2},{2,3}},{{0,2},{1,2},{2,2},{3,2}},{{1,0},{1,1},{1,2},{1,3}}}, // I
    {{{0,0},{1,0},{0,1},{1,1}},{{0,0},{1,0},{0,1},{1,1}},{{0,0},{1,0},{0,1},{1,1}},{{0,0},{1,0},{0,1},{1,1}}}, // O
    {{{0,1},{1,1},{2,1},{1,0}},{{1,0},{1,1},{1,2},{2,1}},{{0,1},{1,1},{2,1},{1,2}},{{1,0},{1,1},{1,2},{0,1}}}, // T
    {{{1,0},{2,0},{0,1},{1,1}},{{1,0},{1,1},{2,1},{2,2}},{{1,1},{2,1},{0,2},{1,2}},{{0,0},{0,1},{1,1},{1,2}}}, // S
    {{{0,0},{1,0},{1,1},{2,1}},{{2,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{1,2},{2,2}},{{1,0},{0,1},{1,1},{0,2}}}, // Z
    {{{0,1},{1,1},{2,1},{2,0}},{{1,0},{1,1},{1,2},{2,2}},{{0,2},{0,1},{1,1},{2,1}},{{0,0},{1,0},{1,1},{1,2}}}, // J
    {{{0,0},{0,1},{1,1},{2,1}},{{1,0},{2,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},{{1,0},{1,1},{0,2},{1,2}}}  // L
};
typedef enum {
    MENU,
    JUEGO
} Estado;

Estado estado = MENU;
typedef struct {
	int8_t tipo;
	int8_t rotacion;
	int8_t x;
	int8_t y;
} Pieza;

Pieza piezaActual;
Pieza piezaSiguiente;
Pieza holdPieza;

uint32_t velocidad = 500;
uint8_t  holdActivo = 0;
uint8_t  holdUsado = 0;
uint32_t score = 0;
uint32_t topScore = 0;
uint32_t lines = 0;
uint8_t  nivel    = 1;

uint8_t  comandanteByte     = 0;

volatile uint8_t reproducirMusica = 0;
volatile uint8_t reproducirEfecto = 0;
volatile uint8_t efectoSonido = 0;

uint32_t tiempoMusica = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// ********************************** FUNCION PARA DIBUJAR O BORRAR BLOQUE (EFECTO 3D)***********************************

void crearBloque(int col, int fila, uint16_t color){
	int px = TETRIS_X + col * BLOQUE;
	    int py = TETRIS_Y + fila * BLOQUE;
	    if (color == COLOR_VACIO) {
	        FillRectFast(px + 1, py + 1, BLOQUE - 2, BLOQUE - 2, COLOR_VACIO);
	        return;
	    }
	    FillRectFast(px + 1, py + 1, BLOQUE - 2, BLOQUE - 2, color);
	    H_line(px + 1, py + 1,          BLOQUE - 2, 0xFFFF); // efecto 3D linea blanca
	    V_line(px + 1, py + 1,          BLOQUE - 2, 0xFFFF);
	    H_line(px + 1, py + BLOQUE - 2, BLOQUE - 2, 0x2104); // efecto 3D linea oscura
	    V_line(px + BLOQUE - 2, py + 1, BLOQUE - 2, 0x2104);
}

// ******************************* FUNCION PARA DIBUJAR LA GRILLA **********************************************************

void dibujarGrid(void){
	for (int i = 1; i < COLS; i++)
		V_line(TETRIS_X + i * BLOQUE, TETRIS_Y, TABLERO_H, 0x2104);
	for (int j = 1; j < FILAS; j++)
		H_line(TETRIS_X, TETRIS_Y + j * BLOQUE, TABLERO_W, 0x2104);
}

// ******************************** FUNCION PARA DIBUJAR EL BORDE DEL TABLERO *********************************************

void dibujarBordeTablero(void) {
    Rect(TETRIS_X,     TETRIS_Y,     TABLERO_W + 1, TABLERO_H,     0x8410);
    Rect(TETRIS_X - 1, TETRIS_Y - 1, TABLERO_W + 3, TABLERO_H + 2, 0x4208);
}

// **************************************** FUNCION PARA DIBUJAR EL PANEL DERECHO ****************************************

void dibujarPanelDerecho(void) {
    FillRectFast(PANEL_X, PANEL_Y, PANEL_W, SCREEN_H, COLOR_PANEL);

    FillRectFast(PANEL_X, LINES_Y, PANEL_W, LINES_H, 0x0000);
    Rect(PANEL_X, LINES_Y, PANEL_W - 1, LINES_H - 1, COLOR_BORDE);
    LCD_Print("LINES", PANEL_X + 8, LINES_Y + 8,  1, 0xFFFF, 0x0000);
    LCD_Print("000",   PANEL_X + 8, LINES_Y + 30, 1, 0x07FF, 0x0000);

    FillRectFast(PANEL_X, SCORE_Y, PANEL_W, SCORE_H, 0x0000);
    Rect(PANEL_X, SCORE_Y, PANEL_W - 1, SCORE_H - 1, COLOR_BORDE);
    LCD_Print("TOP",    PANEL_X + 8, SCORE_Y + 8,  1, 0xFFFF, 0x0000);
    LCD_Print("000000", PANEL_X + 8, SCORE_Y + 24, 1, 0xF800, 0x0000);
    LCD_Print("SCORE",  PANEL_X + 8, SCORE_Y + 44, 1, 0xFFFF, 0x0000);
    LCD_Print("000000", PANEL_X + 8, SCORE_Y + 60, 1, 0xF800, 0x0000);

    FillRectFast(PANEL_X, NEXT_Y, PANEL_W, NEXT_H, 0x0000);
    Rect(PANEL_X, NEXT_Y, PANEL_W - 1, NEXT_H - 1, COLOR_BORDE);
    LCD_Print("NEXT", PANEL_X + 8, NEXT_Y + 8, 1, 0xFFFF, 0x0000);

    FillRectFast(PANEL_X, LEVEL_Y, PANEL_W, LEVEL_H, 0x0000);
    Rect(PANEL_X, LEVEL_Y, PANEL_W - 1, LEVEL_H - 1, COLOR_BORDE);
    LCD_Print("LEVEL", PANEL_X + 8, LEVEL_Y + 8,  1, 0xFFFF, 0x0000);
    LCD_Print("01",    PANEL_X + 8, LEVEL_Y + 28, 1, 0x07FF, 0x0000);
}

// ************************************ FUNCION PARA DIBUJAR EL FONDO ********************************************************

void dibujarFondo(void){
	FillRectFast(0, 0, SCREEN_W, SCREEN_H, 0x2104);
	FillRectFast(TETRIS_X, TETRIS_Y, TABLERO_W, TABLERO_H, COLOR_VACIO);
	Rect(TETRIS_X, TETRIS_Y, TABLERO_W, TABLERO_H, COLOR_BORDE);
	FillRectFast(PANEL_X, PANEL_Y, PANEL_W, SCREEN_H, 0x1082);
	Rect(PANEL_X, PANEL_Y, PANEL_W - 1, SCREEN_H - 1, COLOR_BORDE);
	dibujarBordeTablero();
	dibujarPanelDerecho();
	dibujarGrid();
}

// ******************************** FUNCION QUE DIBUJA LA PIEZA SIGUIENTE A SALIR ***************************************************

void dibujarSiguientePieza(void) {
    FillRectFast(PANEL_X + 4, NEXT_Y + 24, PANEL_W - 8, NEXT_H - 28, 0x0000);
    int ox = PANEL_X + 20;
    int oy = NEXT_Y + 30;
    uint16_t color = coloresPieza[piezaSiguiente.tipo + 1];
    for (int i = 0; i < 4; i++) {
        int bx = piezas[piezaSiguiente.tipo][0][i][0];
        int by = piezas[piezaSiguiente.tipo][0][i][1];
        int px = ox + bx * (BLOQUE - 2);
        int py = oy + by * (BLOQUE - 2);
        FillRectFast(px, py, BLOQUE - 4, BLOQUE - 4, color);
        H_line(px, py, BLOQUE - 4, 0xFFFF);
        V_line(px, py, BLOQUE - 4, 0xFFFF);
    }
}

// ******************* Funciones del DFPlayer para la música del juego ************************************

void DFPlayer_SendCmd(uint8_t cmd, uint8_t param1, uint8_t param2) {
    uint8_t buf[10];
    uint16_t checksum = -(0xFF + 0x06 + cmd + 0x00 + param1 + param2);
    buf[0] = 0x7E;
    buf[1] = 0xFF;
    buf[2] = 0x06;
    buf[3] = cmd;
    buf[4] = 0x00;
    buf[5] = param1;
    buf[6] = param2;
    buf[7] = (checksum >> 8) & 0xFF;
    buf[8] = checksum & 0xFF;
    buf[9] = 0xEF;
    HAL_UART_Transmit(&huart5, buf, 10, 10);
}

void DFPlayer_Init(void) {
    HAL_Delay(2000);
    DFPlayer_SendCmd(0x06, 0x00, 15);  // configuración del dfplayer
    HAL_Delay(100);
}

void DFPlayer_Play(uint8_t track) {
    DFPlayer_SendCmd(0x12, 0x00, track); // reproduce la canción en loop
}

// ********************************* EFECTOS DE SONIDO CON PWM USANDO TIM1 ***************************************************

void tono(int frecuencia, int duracion) {
    if (frecuencia == 0) {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
        return;
    }
    int prescaler = (TIM_FREQ / (ARR * frecuencia)) - 1;
    if (prescaler < 0) prescaler = 0;
    __HAL_TIM_SET_PRESCALER(&htim1, prescaler);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, ARR / 2);
    HAL_Delay(duracion);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
}


void efectoMover(void) {
    tono(1200, 5);
}

void efectoRotar(void) {
    tono(2000, 8);
}

void efectoFijar(void) {
    tono(250, 10);
}

void efectoLinea(void) {
    tono(800,  15);
    tono(1200, 15);
}


// ****************************** FUNCION PARA MOSTRAR LOS CAMBIOS EN EL SCORE VISUALMENTE *****************************

void actualizarPanelScore(void) {
    char buf[10];

    FillRectFast(PANEL_X + 8, LINES_Y + 30, 60, 10, 0x0000);
    sprintf(buf, "%03lu", lines);
    LCD_Print(buf, PANEL_X + 8, LINES_Y + 30, 1, 0x07FF, 0x0000);

    FillRectFast(PANEL_X + 8, SCORE_Y + 24, 70, 10, 0x0000);
    sprintf(buf, "%06lu", topScore);
    LCD_Print(buf, PANEL_X + 8, SCORE_Y + 24, 1, 0xF800, 0x0000);

    FillRectFast(PANEL_X + 8, SCORE_Y + 60, 70, 10, 0x0000);
    sprintf(buf, "%06lu", score);
    LCD_Print(buf, PANEL_X + 8, SCORE_Y + 60, 1, 0xF800, 0x0000);

    FillRectFast(PANEL_X + 8, LEVEL_Y + 28, 30, 10, 0x0000);
    sprintf(buf, "%02d", nivel);
    LCD_Print(buf, PANEL_X + 8, LEVEL_Y + 28, 1, 0x07FF, 0x0000);
}

// **************************** FUNCION PARA CALCULAR Y ACTUALIZAR EL SCORE ********************************************************

void actualizarScore(uint8_t lineasEliminadas) {
    const uint32_t puntos[5] = {0, 100, 300, 500, 800};
    lines += lineasEliminadas;
    score += puntos[lineasEliminadas] * nivel;
    if (score > topScore){
    	topScore = score;
    	 FIL fil;
    	        if (f_open(&fil, "topscore.txt", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
    	            f_printf(&fil, "%lu", topScore);
    	            f_close(&fil);
    	    }
    }

    if (lines >= (uint32_t)nivel * 10) {
        nivel++;
        if (velocidad > 100) velocidad -= 40;
    }
    efectoSonido = 4;
    reproducirEfecto = 1;
    actualizarPanelScore();
}

//*************** FUNCION PARA CALCULAR POSICIONES DE LOS BLOQUES QUE FORMAN UN TETRIMINO*******************************

void obtenerBloques(Pieza *p, int8_t bx[4], int8_t by[4]) {
    for (int i = 0; i < 4; i++) {
        bx[i] = p->x + piezas[p->tipo][p->rotacion][i][0];
        by[i] = p->y + piezas[p->tipo][p->rotacion][i][1];
    }
}

// **************************** FUNCION PARA VALIDAR MOVIMIENTOS ANTES DE EJECUTARLOS************************************

uint8_t esValido(Pieza *p, int8_t nx, int8_t ny, int8_t nr) {
    for (int i = 0; i < 4; i++) {
        int8_t bx = nx + piezas[p->tipo][nr][i][0];
        int8_t by = ny + piezas[p->tipo][nr][i][1];
        if (bx < 0 || bx >= COLS) return 0;
        if (by < 0)               continue;
        if (by >= FILAS)          return 0;
        if (tablero[by][bx] != 0) return 0;
    }
    return 1;
}

// ********************************* FUNCION PARA PINTAR NUEVA PIEZA EN EL TABLERO ******************************************

void dibujarPieza(Pieza *p, uint16_t color) {
    int8_t bx[4], by[4];
    obtenerBloques(p, bx, by);
    for (int i = 0; i < 4; i++)
        if (by[i] >= 0 && by[i] < FILAS && bx[i] >= 0 && bx[i] < COLS)
            crearBloque((int)bx[i], (int)by[i], color);
}

// ********************************** FUNCION PARA GENERAR UNA PIEZA ALEATORIA ************************************************

Pieza nuevaPieza(void) {
    Pieza p;
    static uint32_t valorRandom = 12345;
    valorRandom ^= valorRandom << 13;
    valorRandom ^= valorRandom >> 17;
    valorRandom ^= valorRandom << 5;
    valorRandom += HAL_GetTick();
    p.tipo     = valorRandom % 7;
    p.rotacion = 0;
    p.x        = COLS / 2 - 2;
    p.y        = 0;
    return p;
}

// *********************************** FUNCION PARA FIJAR PIEZA ***********************************************************

void fijarPieza(void) {
    int8_t bx[4], by[4];
    obtenerBloques(&piezaActual, bx, by);
    for (int i = 0; i < 4; i++)
        if (by[i] >= 0)
            tablero[by[i]][bx[i]] = piezaActual.tipo + 1;
    efectoSonido = 3;
    reproducirEfecto = 1;
}

// ******************************* FUNCION PARA ELIMINAR lINEAS *********************************************************
uint8_t eliminarLineas(void) {
    uint8_t lineasEliminadas = 0;
    for (int f = FILAS - 1; f >= 0; f--) {
        uint8_t completa = 1;
        for (int c = 0; c < COLS; c++)
            if (tablero[f][c] == 0) { completa = 0; break; }
        if (completa) {
            lineasEliminadas++;
            for (int ff = f; ff > 0; ff--)
                for (int c = 0; c < COLS; c++)
                    tablero[ff][c] = tablero[ff-1][c];
            for (int c = 0; c < COLS; c++)
                tablero[0][c] = 0;
            f++;
        }
    }
    return lineasEliminadas;
}

//************************************* FUNCION PARA DIBUJAR EL TABLERO DEL JUEGO **************************************

void dibujarTablero(void) {
    for (int f = 0; f < FILAS; f++)
        for (int c = 0; c < COLS; c++)
            crearBloque(c, f, coloresPieza[tablero[f][c]]);
}

uint8_t spawnPieza(void) {
    piezaActual    = piezaSiguiente;
    piezaActual.x  = COLS / 2 - 2;
    piezaActual.y  = 0;
    piezaSiguiente = nuevaPieza();
    holdUsado      = 0;
    dibujarSiguientePieza();
    if (!esValido(&piezaActual, piezaActual.x, piezaActual.y, piezaActual.rotacion))
        return 0;
    return 1;
}

// ******************************** FUNCION PARA DIBUJAR LA PIEZA FANTASMA GUÍA *********************************************

void dibujarGhost(uint16_t color) {
    Pieza ghost = piezaActual;
    while (esValido(&ghost, ghost.x, ghost.y + 1, ghost.rotacion))
        ghost.y++;

    if (ghost.y == piezaActual.y) return;

    int8_t bx[4], by[4];
    obtenerBloques(&ghost, bx, by);

    for (int i = 0; i < 4; i++) {
        if (by[i] < 0 || by[i] >= FILAS || bx[i] < 0 || bx[i] >= COLS) continue;

        int8_t abx[4], aby[4];
        obtenerBloques(&piezaActual, abx, aby);
        uint8_t esParteDePieza = 0;
        for (int j = 0; j < 4; j++) {
            if (bx[i] == abx[j] && by[i] == aby[j]) {
                esParteDePieza = 1;
                break;
            }
        }
        if (esParteDePieza) continue;

        if (color == COLOR_VACIO) {

            if (tablero[by[i]][bx[i]] != 0)
                crearBloque((int)bx[i], (int)by[i], coloresPieza[tablero[by[i]][bx[i]]]);
            else
                crearBloque((int)bx[i], (int)by[i], COLOR_VACIO);
        } else {

            if (tablero[by[i]][bx[i]] == 0)
                crearBloque((int)bx[i], (int)by[i], 0x2104);
        }
    }
}

//************************************** FUNCION DEL MENU DE INICIO *****************************************************

void mostrarMenu(void)
{
    uint32_t i;

    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    SetWindows(0, 0, 239, 319);
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);

    for (i = 0; i < (240 * 320); i++)
    {
        uint16_t color = menu_map[i];
        LCD_DATA(color & 0xFF);
        LCD_DATA(color >> 8);
    }

    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

// ***************************************** FUNCION PARA REINICIAR JUEGO ***********************************************

void reiniciarJuego(void) {
    for (int f = 0; f < FILAS; f++)
        for (int c = 0; c < COLS; c++)
            tablero[f][c] = 0;
    score      = 0;
    lines      = 0;
    nivel      = 1;
    velocidad  = 500;
    holdActivo = 0;
    holdUsado  = 0;
    LCD_Clear(0x0000);
    dibujarFondo();
    actualizarPanelScore();
    piezaSiguiente = nuevaPieza();
    spawnPieza();
    dibujarPieza(&piezaActual, coloresPieza[piezaActual.tipo + 1]);
}

// ****************************************** FUNCIONES PARA MOVER PIEZAS ************************************************

void moverIzquierda(void) {
    dibujarGhost(COLOR_VACIO);
    dibujarPieza(&piezaActual, COLOR_VACIO);
    if (esValido(&piezaActual, piezaActual.x - 1, piezaActual.y, piezaActual.rotacion))
        piezaActual.x--;
    dibujarPieza(&piezaActual, coloresPieza[piezaActual.tipo + 1]);
    dibujarGhost(0x2104);
}

void moverDerecha(void) {
    dibujarGhost(COLOR_VACIO);
    dibujarPieza(&piezaActual, COLOR_VACIO);
    if (esValido(&piezaActual, piezaActual.x + 1, piezaActual.y, piezaActual.rotacion))
        piezaActual.x++;
    dibujarPieza(&piezaActual, coloresPieza[piezaActual.tipo + 1]);
    dibujarGhost(0x2104);
}

void moverAbajo(void) {
    dibujarGhost(COLOR_VACIO);
    dibujarPieza(&piezaActual, COLOR_VACIO);
    if (esValido(&piezaActual, piezaActual.x, piezaActual.y + 1, piezaActual.rotacion))
        piezaActual.y++;
    dibujarPieza(&piezaActual, coloresPieza[piezaActual.tipo + 1]);
    dibujarGhost(0x2104);
}

void rotar(void) {
    dibujarGhost(COLOR_VACIO);
    dibujarPieza(&piezaActual, COLOR_VACIO);
    int8_t nr = (piezaActual.rotacion + 1) % 4;
    if (esValido(&piezaActual, piezaActual.x, piezaActual.y, nr))
        piezaActual.rotacion = nr;
    dibujarPieza(&piezaActual, coloresPieza[piezaActual.tipo + 1]);
    dibujarGhost(0x2104);
}

void rotarIzquierda(void) {
    dibujarGhost(COLOR_VACIO);
    dibujarPieza(&piezaActual, COLOR_VACIO);
    int8_t nr = (piezaActual.rotacion + 3) % 4;
    if (esValido(&piezaActual, piezaActual.x, piezaActual.y, nr))
        piezaActual.rotacion = nr;
    dibujarPieza(&piezaActual, coloresPieza[piezaActual.tipo + 1]);
    dibujarGhost(0x2104);
}

void bajarRapido(void) {
    dibujarGhost(COLOR_VACIO);
    dibujarPieza(&piezaActual, COLOR_VACIO);
    while (esValido(&piezaActual, piezaActual.x, piezaActual.y + 1, piezaActual.rotacion))
        piezaActual.y++;
    dibujarPieza(&piezaActual, coloresPieza[piezaActual.tipo + 1]);
    dibujarGhost(0x2104);
}

//************************************** FUNCION PARA LA PANTALLA DE GAMEOVER ****************************************

void pantallaGameOver(void) {
    estado = MENU;
    char buf[12];

    FillRectFast(0, 0, SCREEN_W, SCREEN_H, 0x0000);
    FillRectFast(20, 60, SCREEN_W - 40, 30, 0x0000);
    Rect(20, 60, SCREEN_W - 41, 29, 0xF800);
    LCD_Print("GAME OVER", 28, 70, 1, 0xF800, 0x0000);
    H_line(20, 95, SCREEN_W - 40, 0x8410);

    LCD_Print("SCORE:", 30, 115, 1, 0xFFFF, 0x0000);
    sprintf(buf, "%06lu", score);
    LCD_Print(buf, 110, 115, 1, 0xF800, 0x0000);

    LCD_Print("TOP  :", 30, 135, 1, 0xFFFF, 0x0000);
    sprintf(buf, "%06lu", topScore);
    LCD_Print(buf, 110, 135, 1, 0xFFE0, 0x0000);

    LCD_Print("LINES:", 30, 155, 1, 0xFFFF, 0x0000);
    sprintf(buf, "%03lu", lines);
    LCD_Print(buf, 110, 155, 1, 0x07FF, 0x0000);

    LCD_Print("LEVEL:", 30, 175, 1, 0xFFFF, 0x0000);
    sprintf(buf, "%02d", nivel);
    LCD_Print(buf, 110, 175, 1, 0x07E0, 0x0000);

    H_line(20, 200, SCREEN_W - 40, 0x8410);
    LCD_Print("Press X to play", 30, 220, 1, 0x8410, 0x0000);
    LCD_Print("again", 30, 236, 1, 0x8410, 0x0000);

    while (1)
    {
        if (comandanteByte == 'X')
        {
            comandanteByte = 0;
            reiniciarJuego();
            estado = JUEGO;
            return;
        }
    }
}

// ***************************** FUNCIÓN PARA PONER PIEZA EN HOLD ***********************************************

void usarHold(void) {
    if (holdUsado) return;

    dibujarGhost(COLOR_VACIO);
    dibujarPieza(&piezaActual, COLOR_VACIO);

    if (!holdActivo) {
        holdPieza          = piezaActual;
        holdPieza.rotacion = 0;
        holdActivo         = 1;
        holdUsado          = 1;
        piezaActual        = piezaSiguiente;
        piezaActual.x      = COLS / 2 - 2;
        piezaActual.y      = 0;
        piezaSiguiente     = nuevaPieza();
        dibujarSiguientePieza();
    } else {
        Pieza temp         = holdPieza;
        holdPieza          = piezaActual;
        holdPieza.rotacion = 0;
        piezaActual        = temp;
        piezaActual.x      = COLS / 2 - 2;
        piezaActual.y      = 0;
        holdUsado          = 1;
    }

    if (!esValido(&piezaActual, piezaActual.x, piezaActual.y, piezaActual.rotacion)) {
           pantallaGameOver();
           return;
        }
        dibujarPieza(&piezaActual, coloresPieza[piezaActual.tipo + 1]);
        dibujarGhost(0x2104);
    }

// ************************ CALLBACK DEL UART PARA EL CONTROL DE PS5 *********************************************

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        if (estado == MENU)
        {

            if (comandanteByte == 'X')
            {

            }
        }
        else if (estado == JUEGO)
        {
            switch (comandanteByte)
            {
                case 'L': moverIzquierda(); efectoSonido = 1; reproducirEfecto = 1; break;
                case 'R': moverDerecha();   efectoSonido = 1; reproducirEfecto = 1; break;
                case 'D': moverAbajo();     break;
                case 'X': rotar();          efectoSonido = 2; reproducirEfecto = 1; break;
                case 'Q': rotarIzquierda(); efectoSonido = 2; reproducirEfecto = 1; break;
                case 'H': usarHold();       break;
                case 'F': bajarRapido();    break;
            }
            comandanteByte = 0;
        }

        HAL_UART_Receive_IT(&huart3, &comandanteByte, 1);
    }
}

uint16_t swap565(uint16_t c)
{
    return (uint16_t)((c << 8) | (c >> 8));
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_USART3_UART_Init();
  MX_SPI1_Init();
  MX_FATFS_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */

  // ************************** Top Score usando la SD**************************************
  FATFS fs;
  FIL fil;
  FRESULT fres;

  fres = f_mount(&fs, "", 1);
  if (fres == FR_OK) {
      fres = f_open(&fil, "topscore.txt", FA_READ);
      if (fres == FR_OK) {
          char buf[16];
          UINT br;
          f_read(&fil, buf, sizeof(buf), &br);
          f_close(&fil);
          topScore = (uint32_t)atoi(buf);
      }
  }

  HAL_GPIO_WritePin(LCD_RD_GPIO_Port, LCD_RD_Pin, GPIO_PIN_SET);
  LCD_Init();
  LCD_Clear(0x0000);
  mostrarMenu();
  DFPlayer_Init();
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
  HAL_UART_Receive_IT(&huart3, &comandanteByte, 1);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      uint32_t ahora = HAL_GetTick();

   //****************************AUDIO*****************************

      static uint32_t lastAudioTime = 0;
          static uint32_t tiempoMusica = 0;
          static uint8_t musicStarted = 0;

          if (ahora - lastAudioTime >= 50)
          {
              lastAudioTime = ahora;

              if (estado == JUEGO)
              {
                  if (!musicStarted)
                  {
                      DFPlayer_Play(1);
                      tiempoMusica = ahora;
                      musicStarted = 1;
                  }
                  else if (ahora - tiempoMusica >= 25000)
                  {
                      DFPlayer_Play(1);
                      tiempoMusica = ahora;
                  }
              }
              else
              {
                  musicStarted = 0;
              }

      //********************** EFECTOS ********************************
              if (reproducirEfecto) {
                  reproducirEfecto = 0;
                  if (efectoSonido == 1) efectoMover();
                  if (efectoSonido == 2) efectoRotar();
                  if (efectoSonido == 3) efectoFijar();
                  if (efectoSonido == 4) efectoLinea();
              }
          }

      // ******************** MENU *****************************************
         if (estado == MENU)
         {
             static uint32_t lastBlink = 0;
             static uint8_t visible = 1;

             if (ahora - lastBlink > 500)
             {
                 lastBlink = ahora;
                 visible = !visible;

                 if (visible)
                 {
                     mostrarMenu();
                 }
                 else
                 {
                     uint16_t fondo = swap565(menu_map[(250 * 240) + 60]);
                     FillRect(35, 225, 180, 28, fondo);
                 }
             }

             if (comandanteByte == 'X')
             {
                 comandanteByte = 0;
                 LCD_Clear(0x0000);
                 reiniciarJuego();
                 estado = JUEGO;
             }
         }
 // ************************* LÓGICA DEL JUEGO ****************************************
      else if (estado == JUEGO)
      {
          static uint32_t lastTick = 0;


          if (ahora - lastTick >= velocidad)
          {
              lastTick = ahora;

              dibujarGhost(COLOR_VACIO);
              dibujarPieza(&piezaActual, COLOR_VACIO);

              if (esValido(&piezaActual, piezaActual.x, piezaActual.y + 1, piezaActual.rotacion))
              {
                  piezaActual.y++;
              }
              else
              {
                  fijarPieza();
                  uint8_t lineasEliminadas = eliminarLineas();

                  if (lineasEliminadas > 0)
                      actualizarScore(lineasEliminadas);

                  dibujarTablero();
                  dibujarGrid();

                  if (!spawnPieza())
                  {
                      pantallaGameOver();
                  }
              }

              dibujarPieza(&piezaActual, coloresPieza[piezaActual.tipo + 1]);
              dibujarGhost(0x2104);
          }

          if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET)
          {
              rotar();
              HAL_Delay(200);
          }
      }
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */
  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */
  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 100-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 50;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */
  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 9600;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */
  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */
  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */
  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */
  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */
  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, F_CS_Pin|LCD_RST_Pin|LCD_D1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin|SD_SS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : F_CS_Pin LCD_RST_Pin LCD_D1_Pin */
  GPIO_InitStruct.Pin = F_CS_Pin|LCD_RST_Pin|LCD_D1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RD_Pin LCD_WR_Pin LCD_RS_Pin LCD_D7_Pin
                           LCD_D0_Pin LCD_D2_Pin */
  GPIO_InitStruct.Pin = LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_D6_Pin LCD_D3_Pin LCD_D5_Pin
                           LCD_D4_Pin SD_SS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin|SD_SS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
