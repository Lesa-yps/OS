# Руководство пользователя

## Запустим демон

gcc main.c -o my_deamon
sudo ./my_deamon

## Проверим, что работает (ищем my_deamon и его id в списке)

ps -ajx

## Получим PID (или из пункта выше)

cat /var/run/deamon.pid

## Отправим сигнал

sudo kill -SIGHUP <PID>  # Для перечитывания файла конфигурации
sudo kill -SIGTERM <PID> # Для завершения работы

## В Linux syslog обычно записывает логи в файлы, находящиеся в директории /var/log/.

/var/log/syslog — общий системный журнал.