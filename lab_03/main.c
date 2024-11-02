// Демон, который перечитывает конфигурационный файл по сигналу

#include <pthread.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/syslog.h>
#include <time.h>

#define CONFFILE "/etc/deamon.conf"
#define LOCKFILE "/var/run/deamon.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define TIMESLEEP 10


sigset_t mask;

// функция, блокирующая файл
int lockfile(int fd)
{
    struct flock fl;
    // блокировка на запись
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    // применяет блокировку к файлу
    return fcntl(fd, F_SETLK, &fl);
}
// функция, гарантирующая запуск только одного экземпляра демона (для этого используются файлы блокировки)
int already_running(void)
{
    int fd;
    char buf[16];
    // системный вызов open(полное_имя_файла, создаём_новый файл_в_который_можно_читать_писать, режим_открытия)
    fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);
    if (fd < 0)
    {
        syslog(LOG_ERR, "невозможно открыть %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    if (lockfile(fd) < 0) /* вызов функции lockfile (можно заблокировать только часть файла) */
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "невозможно установить блокировку на %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
    return 0;
}

/* функция, делающая процесс демоном */
void daemonize(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    /*
    * -- Сбросить маску режима создания файла. (сюда входят права доступа (read, write and execute) для 3 категорий пользователей (user, group and others) и тип файла)
    * ! причём эта маска по коду сбрасывается в parent (спец чтобы подчеркнуть, что потомок наследует эту сброшенную маску и сможет создавать любые файлы с любыми правами доступа без ограничений), но на самом деле эту функцию можно вызывать в child-е
    */	
    umask(0);
    
    /*
    * -- Получить максимально возможный номер дескриптора файла.
    */
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        printf("%s: невозможно получить максимальный номер дескриптора ", cmd);
    
    /*
    * -- Стать лидером нового сеанса, чтобы утратить управляющий терминал.
    * создаёт процесс потока, который наследует от родителя маску
    * после fork процесс-предок завершается, потомок становится сиротой и теряет группу (тк любой процесс, вызывающий fork, создаёт группу), потомка усыновляет терминальный процесс с id=1, потомок не является лидером группы и это основное условие для вызова setsid
    * функция setsid лишает потомка управляющего терминала, потомок становится лидером в своей группе, где он один
    * в основном демоны в состоянии itteratble sleep
    */
    if ((pid = fork()) < 0)
        printf("%s: ошибка вызова функции fork", cmd);
    else if (pid != 0) /* родительский процесс */
        exit(0);
    if (setsid() == -1)
    {
        printf("%s: ошибка вызова функции setsid", cmd);
        exit(1);
    }

    /*
    * -- Обеспечить невозможность обретения управляющего терминала в будущем.
    */
    sa.sa_handler = SIG_IGN; /* инизиализация полей структуры sigaction (SIG_IGN - игнор, маски сбрасываются, флаг в нулл)*/
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) /* в результате будет игнорироваться сигнал SIGHUP, чтоб не завершился аварийно, тк лишили управляющего терминала */
        printf("%s: невозможно игнорировать сигнал SIGHUP", cmd);
    
    /*
    * -- Назначить корневой каталог текущим рабочим каталогом, чтобы впоследствии можно было отмонтировать смонтированную файловую систему.
    * тк в линукс файловые системы монтируются, юникс поддерживвает много ФС, но только смонтированная файловая система доступна пользователю и предоставляет доступ к файлам и каталогам
    */
    if (chdir("/") < 0)
        printf("%s: невозможно сделать текущим рабочим каталогом /", cmd);
    
    /*
    * -- Закрыть все открытые файловые дескрипторы. (тк демону открытые файлы не нужны)
    * каждый процесс имеет собственную таблицу открытых файлов (размер таблицы = 512, тк есть в ядре функция, которая ищет в таблице первый свободный дескриптор, если все дескрипторы заняты система переходит на новую таблицу из 512 дескрипторов -> итого 1024, все 1024 заняты -> ошибка, что нельзя открыть файл)
    */
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024; /* процесс может открыть 1024 файла */
    for (i = 0; i < rl.rlim_max; i++) /* закрываются все открытые файлы, начиная с 0 (автоматически в таблицу добавляются stdin, stdout and stderr)*/
        close(i);
    
     /*
    * -- Присоединить файловые дескрипторы 0, 1 и 2 к /dev/null. (чтоб, если вдруг у какого-то демона возникнет фантазия что-то куда-то записать, чтоб это ушло в нулл)
    */
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
    
     /*
    * -- Инициализировать файл журнала.
    */
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "ошибочные файловые дескрипторы %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
}


// Функция перечитывает конфигурационный файл
void reread(void)
{
    FILE *fp = fopen(CONFFILE, "r");
    if (fp == NULL)
        syslog(LOG_ERR, "Не удалось открыть конфигурационный файл: %s", strerror(errno));
    else
    {
        // Чтение конфигурации
        char line[256];
        while (fgets(line, sizeof(line), fp) != NULL)
            // Обработка строки конфигурации
            syslog(LOG_INFO, "Конфигурационная строка: %s", line);
        fclose(fp);
    }
}


// Функция обрабатывает сигналы в отдельном потоке
void *thr_fn(void *arg)
{
    int err, signo;
    
    for (;;)
    {
        err = sigwait(&mask, &signo); // блокирует процесс до возникновения сигнала
        if (err != 0)
        {
            syslog(LOG_ERR, "ошибка вызова функции sigwalt");
            exit(1);
        }

        switch (signo)
        {
            case SIGHUP:
                syslog(LOG_INFO, "Чтение конфигурационного файла");
                reread();
                break;
            case SIGTERM:
                syslog(LOG_INFO, "получеН сигнал SIGTERM; выход");
                exit(0);
            default:
                syslog(LOG_INFO, "не получен нужный сигнал %d\n", signo);
        }
    }
    return(0);
}

/* точка входа */
int main(int argc, char *argv[])
{
    int err;
    pthread_t tid;
    char *cmd;
    struct sigaction sa;

    if	((cmd = strrchr(argv[0], '/')) == NULL)
        cmd = argv[0];
    else
        cmd++;
    
    // перейти в режим демона
    daemonize(cmd);
    
    // убедиться в том, что ранее не была запущена другая копия демона
    if (already_running())
    {
        syslog(LOG_ERR, "демон уже запущен");
        exit(1);
    }

    // восстановить действие по умолчанию для сигнала SIGHUP и заблокировать все сигналы
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        printf("&s: невозможно восстановить действие SIG_DFL для SIGHUP");
    sigfillset(&mask);
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0) /* pthread_sigmask проверяет заблокированные сигналы */
    {
        printf("oшибкa выполнения операции SIG_BLOCK");
        exit(1);
    }
    
    // создать поток, который будет заниматься обработкой SIGHUP и SIGTERM
    err	= pthread_create(&tid, NULL, thr_fn, 0);
    if (err != 0)
    {
        printf("Hевозможно создать поток");
        exit(1);
    }

    // остальная часть программы-демона
    while (1)
    {
        // Логируем текущее значение времени
        time_t current_time = time(NULL);
        syslog(LOG_INFO, "Текущее время: %s", ctime(&current_time));
        sleep(TIMESLEEP); // TIMESLEEP = 10
    }

    return 0;
}