#ifndef __OLED_H__
#define __OLED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void OLED_Init(void);
void OLED_Clear(void);
void OLED_SetCursor(uint8_t row, uint8_t col);
void OLED_Print(const char *str);
void OLED_PrintAt(uint8_t row, uint8_t col, const char *str);
void OLED_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H__ */
