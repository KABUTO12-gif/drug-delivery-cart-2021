#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);

#define LOGI(...) log_info(__VA_ARGS__)
#define LOGW(...) log_warn(__VA_ARGS__)
#define LOGE(...) log_error(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* __LOG_H__ */
