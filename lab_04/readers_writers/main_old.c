// Монитор Хоара «Читатели-писатели»

/* В следующем примере создается набор из двух семафоров с идентификатором 100. Затем значение 
 семафора уменьшается на 1, а значение второго семафора проверяется (подробнее см. Тренс Чан стр. 328).*/
# include < sys / types.h >
# include < sys / ipc.h >
# include < sys / sem.h >
struct sem_buf sem_arr[2] = {{0, -1, SEM_UNDO | SEM_NOWAIT}, {1, 0, 1} };
int main(void)
{
   int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
   int isem_descr = semget(100, 2, IRC_CREATE | perms );
   if (isem_descry = = -1) { perror(“semget”); return 1;}
   if ( semop (isem_descry, sem_arr, 2) = = -1) { perror(“semop”); return 1;}
   return 0;
}

/* В примере программа открывает сегмент разделяемой памяти размером 1024 байта с идентификатором 100.
 Если такая область не существует, то она создается системным вызовом shmget() с полными правами доступа
 для всех категорий пользователей. Затем сегмент присоединяется к виртуальному адресному пространству процесса. 
 Системный вызов shmat() возвращает адрес сегмента и по этому адресу записывается сообщение «Hello». 
 После этого сегмент отсоединяется. 
После этого любой процесс по идентификатору сегмента может присоединить этот сегмент к своему адресному пространству
 и прочитать записанное сообщение. */

# include < sys / types.h >
# include < sys / ipc.h >
# include < sys / shm.h >
# include <string.h>
int main(void)
{
   int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
   int fd = shmget(100, 1024, IRC_CREATE | perms );
   if (fd = = -1) { perror(“shmget”); exit(1);}
   char *addr = (char*)shmat(fd,0,0);
   if (*addr == (char*) -1) { perror(“shmat”); return 1;}
   strcpy(addr, “Hello”);
   if (shmdt(addr) == -1) perror(“shmdt”);
   return 0;
}

RESOURCE MONITOR;
var
     active_readers : integer;
       active_writer : logical;
     can_read, can_write : conditional;
procedure star_read
     begin
           if (active_writer or turn(can_write)) then wait(can_read);
           active_readers++; //инкремент читатетей
           signal(can_read);
     end;
procedure stop_read
      begin
            active_readers--; //декремент читателей
            if (active_readers = 0) then signal(can_write);
       end;
procedure start_write
        begin
               if ((active_readers > 0) or active_writer) then wait(can_write);
               active_writer:= true;
        end;
procedure stop_write
         begin
                active_writer:= false;
                if (turn(can_read) then signal(can_read)
                   else signal(can_write);
end;
begin
     active_readers:=0;
     active_writer:=false;
end.
