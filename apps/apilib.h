/* apilib.h       BY:STON */

///////////////////////////////////
//  copyright (C)  pengzekai
///////////////////////////////////
//  系统的api
///////////////////////////////////
#define RTC_SECOND			0x00
#define RTC_MINUTE			0x02
#define RTC_HOURS			0x04
#define RTC_WEEKDAY			0x06
#define RTC_DAY_OF_MONTH	0x07
#define RTC_MONTH 			0x08
#define RTC_YEAR			0x09
#define RTC_CENTURY			0x32

struct MEMMAP_ENTRY {
	unsigned int addr;
	unsigned int size;
};

//cmd
void api_putchar(int c); //输出一个字符
void api_putstr0(char *s); //命令行输出
void api_putstr1(char *s, int l); //命令输入字符串
void api_end(void); //应用结束 Append

//win
int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title); //打开窗口 WinMake
void api_putstrwin(int win, int x, int y, int col, int len, char *str);//窗口输出 winputs
void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col); //WinSquare

//app
void api_initmalloc(void);
char *api_malloc(int size);
void api_free(char *addr, int size);
void api_point(int win, int x, int y, int col);

//win
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
void api_closewin(int win);
int api_getkey(int mode); //win结束

//app
void api_syncdemo_start(void);

// user sync test
void api_sync_reset(void);
int api_get_shared(void);
void api_set_shared(int val);
void api_sem_wait(void);
void api_sem_post(void);
int api_alloctimer(void);
void api_inittimer(int timer, int data);
void api_settimer(int timer, int time);
void api_freetimer(int timer);
void api_beep(int tone); //应用程序提示音
int api_fopen(char *fname);
void api_fclose(int fhandle);
void api_fseek(int fhandle, int offset, int mode);
int api_fsize(int fhandle, int mode);
int api_fread(char *buf, int maxsize, int fhandle);
int api_cmdline(char *buf, int maxsize);
int api_getlang(void);
int api_rtc(int option);

int api_bitblt(int win, short *buf, int x, int y, int xsize, int ysize);
void api_systime(char *s);
void api_autorefresh(int win, int sw);
int api_bitbltEx(int win, short *buf, int x, int y, int sx, int sy, int xsize, int ysize, int width);
void api_putstrwinEx(int win, int x, int y, int col, int len, int scl, char *str);
int api_fopenEx(char *fname, int mode);
void api_fcloseEx(int fhandle);
int api_fwrite(char *buf, int maxsize, int fhandle);
void api_sysinfo(struct SYSINFO *sysinfo);
int api_getmemalgo(void);
int api_getmemmap(struct MEMMAP_ENTRY *entries, int max_entries, int *free_bytes);

// cpuid
void asm_cpuid(int id_eax, int id_ecx, int *eax, int *ebx, int *ecx, int *edx);
void asm_rdtsc(int *high, int *low);

/* Haritomo common API */
int tomo_gettick(void);
void tomo_setlang(int land);

/* OSAkkie API */
void osak_putministrwin(int win, int x, int y, int col, int len, char *str);
void osak_exec(char *name);

// User sequence / Kernel synchronization logic 
void api_open_syncmon(void);
int api_mutex_create(void);
void api_mutex_lock(int mutex_id);
void api_mutex_unlock(int mutex_id);
void api_mutex_destroy(int mutex_id);
void api_sync_init(void);

// Producer-Consumer
void api_pc_init(void);
void api_pc_produce(int val);
int api_pc_consume(void);