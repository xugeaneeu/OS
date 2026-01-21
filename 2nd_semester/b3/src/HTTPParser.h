#pragma once

int Parse_request(const char* buf, char** out_host, char** out_port,
                  char** out_path);