#include "log.h"

#include "main.h"
#include "usart.h"

#include <stdio.h>
#include <string.h>

static void log_vprint(const char *level, const char *fmt, va_list args)
{
  char body[160];
  char line[220];
  int body_len;
  int line_len;
  uint32_t ms = HAL_GetTick();

  body_len = vsnprintf(body, sizeof(body), fmt, args);
  if (body_len < 0)
  {
    return;
  }

  line_len = snprintf(line, sizeof(line), "[%010lu][%s] %s\r\n", (unsigned long)ms, level, body);
  if (line_len < 0)
  {
    return;
  }

  if ((size_t)line_len > sizeof(line))
  {
    line_len = (int)sizeof(line);
  }

  (void)HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)strnlen(line, sizeof(line)), 100);
}

void log_info(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  log_vprint("INFO", fmt, args);
  va_end(args);
}

void log_warn(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  log_vprint("WARN", fmt, args);
  va_end(args);
}

void log_error(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  log_vprint("ERROR", fmt, args);
  va_end(args);
}
