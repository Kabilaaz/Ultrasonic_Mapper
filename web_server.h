#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

// Initializes the ESP32 as an Access Point and starts the HTTP server
void init_web_server(void);

#ifdef __cplusplus
}
#endif

#endif // WEB_SERVER_H