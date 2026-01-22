#include "HTTPParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  Use some lib to parse HTTP request. (Пока что это дипсик сгенерил xd)
*/
int Parse_request(const char* buf, char** out_host, char** out_port,
                  char** out_path) {
  char        method[16], url[1024], version[16];
  const char* p = buf;

  char *host = NULL, *port = NULL, *path = NULL, *hostport = NULL;

  // 1) Разбор request-line
  if (sscanf(p, "%15s %1023s %15s", method, url, version) != 3)
    return -1;
  if (strcmp(method, "GET") != 0)
    return -1;
  // Разрешаем HTTP/1.0 и HTTP/1.1
  if (strcmp(version, "HTTP/1.0") != 0 && strcmp(version, "HTTP/1.1") != 0)
    return -1;

  // 2) Абсолютный URI?
  if (strncmp(url, "http://", 7) == 0) {
    const char* hp = url + 7;
    // найти первый slash
    const char* slash = strchr(hp, '/');
    size_t      hlen = slash ? (size_t)(slash - hp) : strlen(hp);
    // скопировать host[:port]
    hostport = strndup(hp, hlen);
    if (!hostport)
      goto error;
    // разделить на host и port
    char* colon = strchr(hostport, ':');
    if (colon) {
      *colon = '\0';
      host = strdup(hostport);
      port = strdup(colon + 1);
    } else {
      host = strdup(hostport);
      port = strdup("80");
    }
    free(hostport);
    hostport = NULL;
    if (!host || !port)
      goto error;
    // путь
    if (slash)
      path = strdup(slash);
    else
      path = strdup("/");
    if (!path)
      goto error;
  } else {
    // 3) Относительный URI — ищем Host: header
    if (url[0] != '/')
      return -1;
    // Находим "\r\nHost:" или в начале после request-line — "Host:"
    char* hpos = strcasestr(buf, "\r\nHost:");
    if (!hpos) {
      // возможно без \r\n — на границе буфера
      hpos = strcasestr(buf, "\nHost:");
      if (!hpos)
        return -1;
    }
    // перепрыгнуть до текста значения
    hpos = strchr(hpos, ':');
    if (!hpos)
      return -1;
    hpos++;
    while (*hpos == ' ' || *hpos == '\t')
      hpos++;
    // читаем до \r или \n
    char* end = strpbrk(hpos, "\r\n");
    if (!end)
      return -1;
    hostport = strndup(hpos, (size_t)(end - hpos));
    if (!hostport)
      goto error;
    // разделяем host и port
    char* colon = strchr(hostport, ':');
    if (colon) {
      *colon = '\0';
      host = strdup(hostport);
      port = strdup(colon + 1);
    } else {
      host = strdup(hostport);
      port = strdup("80");
    }
    free(hostport);
    hostport = NULL;
    if (!host || !port)
      goto error;
    path = strdup(url);
    if (!path)
      goto error;
  }

  // 4) Успешно
  *out_host = host;
  *out_port = port;
  *out_path = path;
  return 0;

error:
  free(host);
  free(port);
  free(path);
  free(hostport);
  return -1;
}