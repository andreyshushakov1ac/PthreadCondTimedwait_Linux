/* 
Напишите функцию, удовлетворяющую следующим условиям:
	a. Прототип функции:
int lab_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, unsigned int timetowait);
	b. Входные параметры: адрес условной переменной, адрес мьютекса, количество мил-лисекунд, устанавливаемое 
для ожидания условной переменной.
	c. Функция возвращает следующие значения:
  0 в случае успешного выполнения функции,
  1 в случае, если время ожидания условной переменной превысило заданное в па-раметре timetowait количество 
миллисекунд,
 -1 в случае любой ошибки.
	При реализации функции разрешается использовать стандартные структуры и функции библиотек ОС Linux, 
за исключением функции pthread_cond_timedwait(…).

	В lab_pthread_cond_timedwait(...) создаётся ожидание от усл.перем. Также там создаётся отдельный поток,
в котором идёт ожидание указанное пользователем кол-во времени в мс. Если время прошло, то глобальная 
переменная flag устанавливается в 1, далее идёт сигнал усл.переменной. flag говорит нашей ф-ции, что надо 
вернуть значение 1, иначе 0 (ошибка=>-1).
	В тестовой программе создаётся два потока. 
Поток А  ставит флаг gg в 0, показывая потоку "А", что тот может начинать запрос-ожидание от усл.перем.
Далее ждёт рандомное время (для получения разных рез-тов) и отправляет сигнал усл.переменной
	Далее, в зависимости от того, что произошло быстрее (TimeUp или сигнал), поток В выводит на экран рез-т
выполнения программы и возвращаемое значение нашей ф-ции.
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

/* Структура param используется для передачи параметров в функцию func, которая запускается 
в отдельном потоке */
struct param
{
	pthread_cond_t *cond;
	int timewait;
};

pthread_mutex_t my_sync;
pthread_t kidA,kidB;
pthread_cond_t rx;

int gg=1; // для хранения значения, возвращаемого функцией lab_pthread_cond_timedwait
int flag; // прошло ли время
int t;// аргумнт командой строки: время ожидание в мс на усл. переменной
 
/* 	Функция, выполняемая в отдельном потоке, вызываемом из lab_pthread_cond_timedwait(...) для 
реализации ожидания на усл.переменной заданное время, а затем сигнализации об окончании этого времени
	Отдельный поток - параллельное выполнение. Чтобы пока проходит заданное время, была возможность всё равно получить сигнал 
от других потоков*/
void *func(void *str) // принимает указатель на структуру param
{
	/*создаем копию структуры param, чтобы избежать конфликтов при использовании 
	указателя на одну и ту же структуру в нескольких потоках.*/
	struct param *qw=(struct param *)str;
	struct param wq=*qw;
	
	// Ожидаем заданное время
	sleep (wq.timewait);
	
	/*Cообщаем через глобальную переменную всем потокам, ждущим на усл.переменной,
	что лимит ожидания исчерпан, и они могут продолжать выполнение*/
	flag=1;	
	pthread_cond_signal(wq.cond);
}
 
/*создает новый поток, который будет ждать заданное время, после чего устанавливает 
flag в 1 и отправляет сигнал на условную переменную. Функция lab_pthread_cond_timedwait 
затем ждет на этой условной переменной и возвращает результат в зависимости от значения флага.*/
int lab_pthread_cond_timedwait(pthread_cond_t *cond,pthread_mutex_t *mutex, unsigned int timetowait)
{
	// выставляем флаг успешного выполнения функции,
	flag=0;
	
	 // инициализация струткуры param и идентификатора потока
	struct param conds;
	pthread_t tid;
	
	// Сохраняем переданные значения в структуре
	conds.timewait=timetowait;
	conds.cond=cond;
	
	pthread_create(&tid,NULL,&func,&conds); // tid, атрибуты потока, функция для выполнения в отд. потоке и её передаваемые параметры
	
	// Ожидаем на усл. переменной
	pthread_cond_wait(cond,mutex);
	
	// Проверяем флаг по получении сигнала усл. переменной
	if (flag==0)
	{
		return 0; // функция завершена успешно
	}
	else if (flag==1)
	{
		return 1; // превышен лимит времени
	}
	else
	{
		return -1; // ошибка
	}
}
 
 
void * streamA () //Поток А
{
srand(time(NULL));
    for (;;)
    {
        sleep(2.5);
		printf("----------\n");
        pthread_mutex_lock (&my_sync);
        printf("Thread A locks MUTEX\n");
		
		// Выставляем флаг gg в 0, показывая потоку "А", что тот может начинать запрос-ожидание от усл.перем.
		gg=0;
		
		// Засыпаем на рандомное время, чтобы иногда был TimeUp, а иногда сигнал на усл.перем. приходил быстрее
		sleep (rand()%3);
		
		pthread_cond_signal(&rx);
		
		printf("Thread A keeps locking MUTEX\n");
		pthread_mutex_unlock (&my_sync);
		printf("Thread A unlocks MUTEX and ends\n----------\n");
    }
}
 
void * streamB () // Поток B
{
    for (;;)
    {
    int res=0;
        sleep(1.5);
        pthread_mutex_lock(&my_sync);
    printf("**********\nThread B locks MUTEX\n");
	
	//gg выставляет в 0 через какое-то время поток "А" и начинается выполнение нашей ф-ции
    while (gg!=0) // хотя бы пару итераций всё равно будет из-за разнице в первых sleep у потоков "А" и "В"
		res=lab_pthread_cond_timedwait(&rx,&my_sync,t); //отсчёт времени начинается с последней итерации
	
	/*Или lab_pthread_cond_timedwait вернуло 0 (=> Поток А до TimeUp послал сигнал усл. перем.)
	или	 вернула 1, т.е. поток "А" спал долго и наступил TimeUp*/
    if (res==0) 
		printf("Success.		lab_pthread_cond_timedwait = %d\n",res);
    else if (res==1)
		printf("TimeUp.		lab_pthread_cond_timedwait = %d\n",res);
    else
		perror("cond_error:\n");
	
    printf("Thread B ends.\n**********\n");
	
	// возвращаем флаг gg в один
    gg=1;
	
    pthread_mutex_unlock(&my_sync);
    }
}
 
int main(int argc,char **argv)
{
	// аргумнт командой строки: время ожидание в мс на усл. переменной
    t=atoi(argv[1]);
    t=t/1000;
	
    pthread_attr_t attr;
    pthread_mutex_init (&my_sync, NULL);
    pthread_attr_init(&attr);
    pthread_create (&kidA, &attr, streamA, NULL);
    pthread_create (&kidB, &attr, streamB, NULL);
    pthread_join(kidA,NULL);
    pthread_join(kidB,NULL);
}