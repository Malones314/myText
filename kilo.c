/*** includes ***/

#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>
#include<errno.h>
#include<sys/ioctl.h>
#include<string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*** function&struct defines ***/
struct String_buff;
struct Text_config;
void clear_screen();  //清除屏幕
void output_draw_rows();  //在每一行开头绘制-
void output_system(); //屏幕打印
void error_information( const char* s); //错误信息打印，当函数错误时手动调用
void disable_raw_mode();  //得到terminal的副本，配合atexit()函数在程序结束时还原terminal
void enable_raw_mode(); //设置终端属性
int get_window_size( int* rows, int* cols); //得到窗口大小
char get_read_from_keyboard();  //从键盘读取字符
void input_system();  //input system
void init_text(); //初始化myText
int get_cursor_position( int* rows, int* cols);  //获得光标位置
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*** defines ***/

//CTRL加字母映射到字母对应的位数, 定义一个推到是否按下了CTRL和字母键的组合
#define WITH_CTRL(n) ( (n) & 0x1f )   //取ACSII码后5位

#define KILO_VERSION "0.0.1"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*** string buff ***/
//因为C没有动态字符串，设置一种字符串，可以在原字符串上追加字符
struct String_buff {
  char* b;
  int len;
};

// String buff初始化函数 ctor
void string_buff_init( struct String_buff* strb){
  strb->b = NULL;
  strb->len = 0;
}

//在strb指向字符串后面追加字符串s
void string_buff_append( struct String_buff* strb, const char* s, int len_ ){
  char* new  = realloc( strb->b, strb->len+len_);
  //void *realloc(void *ptr, size_t size);
    //改变内存块的大小
  if( new == NULL)
    return ;

  memcpy( &new[ strb->len], s, len_);
  //void *memcpy(void *s1, const void *s2, size_t n);
    //s1: 目标缓冲区地址
    //s2: 源缓冲区地址
    //n:  复制字节个数
  strb->b = new;
  strb->len += len_;
}

//释放strb空间 dtor
void string_buff_free( struct String_buff* strb){
  free( strb->b);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*** data ***/

struct Text_config{
  int screen_rows;
  int screen_cols;
  struct termios orig_termios;  //保存程序开始时的终端属性，用于程序结束后还原终端属性
};

struct Text_config text;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*** output system ***/

void clear_screen(){
  write(STDOUT_FILENO, "\x1b[2J", 4); //清除屏幕
  write(STDOUT_FILENO, "\x1b[H", 3);  //重定位光标
}

//在每一行的开头绘制-
void output_draw_rows( struct String_buff* strb ){
  int row_ = -1;
  for( row_ = 0; row_ < text.screen_rows; ++row_){ 

    if( row_ == text.screen_rows / 3){  //在屏幕三分之一处显示欢迎信息
      char welcome[100];
      int welcome_len = snprintf( welcome, sizeof( welcome_len),
        "myText--Kilo -- version %s", KILO_VERSION);
      if( welcome_len > text.screen_cols)
        welcome_len = text.screen_cols;
      int in_half = (text.screen_cols - welcome_len) / 2;
      if( in_half){
        string_buff_append( strb, "-", 1);
        in_half--;
      }
      while( in_half--)
        string_buff_append( strb, " ", 1);
      string_buff_append( strb, welcome, welcome_len);
    } else {
      string_buff_append( strb, "-", 1);
    }

    string_buff_append( strb, "\x1b[K", 3);
    //k:擦除当前行的一部分, 默认参数为0
      //2:擦除整行
      //1:擦除光标左侧的部分行
      //0:擦除光标右侧的部分行
    if( row_ < text.screen_rows - 1)
      string_buff_append( strb, "\r\n", 2);
  }
}

//刷新屏幕

//屏幕打印
void output_system(){
  //第一版 刷新屏幕
/*
  write( STDOUT_FILENO, "\x1b[2J", 4);
    //ssize_t write(int fildes, const void *buf, size_t nbyte);
      //filds:要写入的打开文件的文件描述符
      //buf:要写入打开文件的数据数组。
      //nbyte:写入文件的字节数。
    //write() 函数尝试将指定数量的字节从指定的缓冲区写入指定的文件描述符。
    //如果 write() 请求写入的字节数超过可用空间，则只写入可用空间所允许的字节数。
    //如果 write() 在写入任何数据之前被信号中断，它将返回 -1 并将 errno 设置为 EINTR。
    //如果 write() 在成功写入一些数据后被信号中断，它会返回写入的字节数。
    //在对常规文件的 write() 成功返回后：
      //从被该写入修改的文件中每个字节位置的任何成功的 read() 返回由 write() 为该位置指定的数据，直到再次修改这些字节位置。
      //对文件中相同字节位置的任何后续 write() 都会覆盖该文件数据。
    
    //\x1b: escape sequence 转义序列.转义序列始终以转义字符(\x1b)开头，后跟 [ 字符
    //转义序列指示终端执行各种文本格式化任务，例如为文本着色、四处移动光标和清除部分屏幕。
    //使用 J 命令（Erase In Display）来清除屏幕
    //转义序列命令采用参数，这些参数位于命令之前。参数为 2，表示清除整个屏幕。
    //<esc>[1J 将清除屏幕直到光标所在的位置，<esc>[0J 将清除从光标到屏幕末尾的屏幕。
    //转义字符文档https://vt100.net/docs/vt100-ug/chapter3.html
  write( STDOUT_FILENO, "\x1b[H", 3);
  //使用H命令定位光标，H 命令实际上有两个参数：光标所在的行号和列号。
  //80×24 大小的终端并且希望光标位于屏幕中央，使用命令 <esc>[12;40H。多个参数使用;间隔
  //H 的默认参数都是 1，可以将这两个参数都省略，将光标定位在第一行第一列，<esc>[1;1H 命令一样。(行和列从1开始编号，而不是 0)

  output_draw_rows();

  write( STDOUT_FILENO, "\x1b[H", 3);
*/

  //第二版 通过string_buff刷新屏幕
  struct String_buff strb = { NULL, 0};
  string_buff_init( &strb);

  string_buff_append( &strb, "\x1b[?25l", 6);
  //l:Reset Mode,用于重置各种终端功能或模式

  string_buff_append( &strb, "\x1b[H", 3);
  
  output_draw_rows( &strb);
  
  string_buff_append( &strb, "\x1b[H", 3);
  string_buff_append( &strb, "\x1b[?25h", 6);
  //h:Set Mode,用于打开各种终端功能或模式

  write( STDOUT_FILENO, strb.b, strb.len);
  
  string_buff_free( &strb);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*** terminal ***/
//错误信息打印，当函数错误时手动调用
void error_information( const char* s){ //s:指向带有解释性消息的空终止字符串的指针
  clear_screen();
  //perror将当前存储在系统变量 errno 中的错误代码的文本描述打印到 stderr。
    //一些标准库函数通过将正整数写入 errno 来指示错误。程序启动时 errno 的值为 0 ，
    //尽管无论是否发生错误，库函数都允许将正整数写入 errno，但库函数永远不会将 0 存储在 errno 中。
    //stderr:与标准错误流关联，用于写入诊断输出。在程序启动时，流没有完全缓冲。
  perror( s);
  exit(1);  //退出程序，退出状态为 1，表示失败
}

//得到terminal的副本，配合atexit()函数在程序结束时还原terminal
void disable_raw_mode(){    
  if( tcsetattr( STDIN_FILENO, TCSAFLUSH, &text.orig_termios) == -1)
    error_information( "tcsetattr");
    //int tcsetattr(int fildes, int optional_actions, const struct termios *termios_p); 设置终端文件描述符参数
    //TCSANOW:变化立即发生。
    //TCSADRAIN:写入文件描述符的所有输出都已传输后发生更改。更改影响输出的参数时应使用此操作。
    //TCSAFLUSH:更改发生在写入文件描述符的所有输出都已传输之后，并且在进行更改之前丢弃到目前为止收到但未读取的所有输入。
    //成功时返回0，失败时返回-1并设置errno为对应值:
      //EBADF:fildes 参数不是有效的文件描述符
      //EINTR:一个信号打断了通话
      //EINVAL:optional_actions 参数不是正确的值，或者试图将 termios 结构中表示的属性更改为不受支持的值
      //ENOTTY:与 fildes 关联的文件不是终端
}

//设置终端属性
void enable_raw_mode() {				
  //可以通过以下方式设置terminal的属性：
  //1.使用tcgetattr()将当前属性读入结构 2.手动修改结构  3.将修改后的结构传递给tcsetattr()以写入新的terminal属性
  if( tcgetattr( STDIN_FILENO, &text.orig_termios) == -1)
    error_information( "tcgetattr");
    //int tcgetattr(int fildes, struct termios *termios_p);获取终端文件描述符参数
    //未出错时返回0，出错时返回-1，并设置errno:
      //EBADF:fildes 参数不是有效的文件描述符
      //ENOTTY:与 fildes 关联的文件不是终端
  atexit( disable_raw_mode);  //atexit 是线程安全的：从多个线程调用该函数不会引发数据竞争。
                              //用它来注册 disable_raw_mode() 函数，以便在程序退出时自动调用
  struct termios raw = text.orig_termios;	        
  raw.c_iflag &= ~( IXON| ICRNL| BRKINT| INPCK| ISTRIP); 
    //XON表示恢复传输，XOFF表示暂停传输
    //终端自动帮用户把Enter转换成(10,‘\n’),关闭ICRNL来关闭这个自动转换
    //Ctrl-M 被读作 13，Enter 键也被读作 13。
    //BRKINT: 当 BRKINT 打开时，中断条件将导致 SIGINT 信号被发送到程序，就像按 Ctrl-C。
    //INPCK: 启用奇偶校验，
    //ISTRIP: 导致每个输入字节的第 8 位被剥离
  raw.c_oflag &= ~(OPOST);  //OPOST关闭输出处理，终端把每一个'\n'转换成"\r\n"
  raw.c_cflag |= ( CS8);  //它将字符大小 (CS) 设置为每字节 8 位。
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);  //关闭ECHO、ICANON,c_lflag：本地模式
    //ICANON：标准模式，关闭后逐字节读取，而非逐行读取
    //默认情况下，Ctrl-C 向当前进程发送 SIGINT 信号使其终止，
    //而 Ctrl-Z 向当前进程发送 SIGTSTP 信号使其挂起
    //关闭ISIG，现在 Ctrl-C 可以读作 3 ，Ctrl-Z 可以读作 26 。
    //IEXTEN：关闭Ctrl-V，Ctrl-V 现在可以读取为 22 
  
  //c_cc 字段的索引，它代表“控制字符”，一个控制各种终端设置的字节数组。
  raw.c_cc[VMIN] = 0;   //VMIN 值设置 read() 返回之前所需的最小输入字节数。
  raw.c_cc[VTIME] = 1;  //VTIME 值设置在 read() 返回之前等待的最长时间。单位是0.1s, 当read超时，返回0，逻辑符合

  if( tcsetattr( STDIN_FILENO, TCSAFLUSH, &raw) == -1 )
    error_information( "tcsetattr");
    //STDIN_FILENO：stdin的文件编号，TCSAFLUSH：改变在所有写入STDIN_FILENO的输出都被传输后生效，
    //所有已接受但未读入的输入都在改变发生前丢弃，即终端不显示输入的字符
}

//得到坐标位置
int get_cursor_position( int* rows, int* cols){

  //设置输入缓冲区
  char buf[32];
  unsigned int ui = 0;

  if ( write( STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;
    //n命令用于查询终端的状态信息
  while( ui < sizeof( buf) - 1) {
    if( read( STDIN_FILENO, &buf[ui], 1) != 1 )
      break;
    if( buf[ui] == 'R')  //到达R退出
      break;
    ui++;
  }
  buf[ui] = '\0'; //设置结尾字符为'\0'

  if( buf[0] != '\x1b' || buf[1] != '[' ) //保证转义字符\x1b响应
    return -1;
  if( sscanf( &buf[2], "%d;%d", rows, cols) != 2)
    return -1;
  
  return 0;
}

//得到窗口大小
int get_window_size( int* rows, int* cols){
  
  struct winsize ws;  //得到窗口大小结构体

  //int ioctl(int fildes, int request, ...);
    //对设备专用文件执行各种设备特定的控制功能。
    //...：可选的第三个参数 (arg)，用于请求特定信息。数据类型取决于特定的控制请求，但它可以是 int 或指向特定于设备的数据结构的指针。
    //返回类型：
      //EBADF：fildes 参数不是有效的打开文件描述符。
      //EFAULT：请求参数需要向 arg 指向的缓冲区或从缓冲区传输数据，但此指针无效。
      //EINTR: 一个信号打断了通话
      //EINVAL: 请求或 arg 对此设备无效。
      //EIO: 发生了一些物理 I/O 错误。
      //ENOTTY: fildes 与接受控制功能的设备驱动程序无关。
      //ENXIO: 请求和 arg 参数对此设备驱动程序有效，但请求的服务无法在此特定子设备上执行。
  if( ioctl( STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 ){
    if( write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12 )
      return -1;
      //C：光标移到右边
      //B：光标移到下面
      //通过C、B，让光标移到右下角，C、B命令赚蒙用于阻止光标越过屏幕
    return get_cursor_position( rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*** input system ***/

//从键盘读取字符
char get_read_from_keyboard() {
  int read_errno = 0;
  char c;

  //read()、 STDIN_FILENO都来自 <unistd.h>, 使用read()从标准输入中得到1byte直到没有更多输入  
  while( (read_errno = read( STDIN_FILENO, &c, 1)) != 1){
    if( read_errno == -1 && errno != EAGAIN)
      error_information("read");
  }

  return c;
}

//input system
void input_system(){
  char c = get_read_from_keyboard();
  /* 测试
    if( iscntrl(c)){  //iscntrl(c)检查c是否为c语言中的控制字符
        printf( "%d\r\n", c);   //c为控制字符，打印c的ASCII码
    }else{
      printf( "%d('%c')\r\n", c, c);  //打印c的ASCII码，并输出c
    }
  */
  if( c == WITH_CTRL('q') ){ //当按下Ctrl-q/Q 时退出
    clear_screen();   //清除屏幕，atexit()也可以在退出时清除屏幕，但是error_information()的错误信息也会被清除
    exit(0);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*** init system ***/

void init_text(){
  if( get_window_size( &text.screen_rows, &text.screen_cols) == -1 )
    error_information("get window size");
}

int main(){
  clear_screen();
  enable_raw_mode();
  init_text();
  
  while(1){
    output_system();
    input_system();
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////