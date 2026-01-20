Конфиг настраивается в файле config.h
По дефолту:
PORT 80
LRU_CACHE_CAP ~250MB
LRU_CACHE_BUCKETS 1024
MAX_CONNECTIONS 1000

Компиляция: make all

Запуск: sudo ./build/proxy

Запуск тестов: sudo make run-tests
