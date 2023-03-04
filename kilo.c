#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>

struct termios orig_termios;

void disableRawMode(){    //得到terminal的副本，配合atexit()函数在程序结束时还原terminal
  tcsetattr( STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

//可以通过以下方式设置terminal的属性：
//1.使用tcgetattr()将当前属性读入结构
//2.手动修改结构
//3.将修改后的结构传递给tcsetattr()以写入新的terminal属性
void enableRawMode() {				       
  tcgetattr( STDIN_FILENO, &orig_termios);
  atexit( disableRawMode);  //atexit 是线程安全的：从多个线程调用该函数不会引发数据竞争。
                            //用它来注册 disableRawMode() 函数，以便在程序退出时自动调用
  struct termios raw = orig_termios;	        
  raw.c_iflag &= ~(IXON | ICRNL); //Ctrl-S 停止数据传输到终端，直到您按下 Ctrl-Q,用于停止输入数据，catch up 类似于打印机的设备
                          //XON表示恢复传输，XOFF表示暂停传输
                          //终端自动帮用户把Enter转换成(10,‘\n’),关闭ICRNL来关闭这个自动转换
                          //Ctrl-M 被读作 13，Enter 键也被读作 13。
  raw.c_oflag &= ~(OPOST);  //OPOST关闭输出处理，终端把每一个'\n'转换成"\r\n"
  
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);			//关闭ECHO、ICANON,c_lflag：本地模式
                                        //ICANON：标准模式，关闭后逐字节读取，而非逐行读取
                                        //默认情况下，Ctrl-C 向当前进程发送 SIGINT 信号使其终止，
                                        //而 Ctrl-Z 向当前进程发送 SIGTSTP 信号使其挂起
                                        //关闭ISIG，现在 Ctrl-C 可以读作 3 ，Ctrl-Z 可以读作 26 。
                                        //IEXTEN：关闭Ctrl-V，Ctrl-V 现在可以读取为 22 
  tcsetattr( STDIN_FILENO, TCSAFLUSH, &raw);
    //STDIN_FILENO：stdin的文件编号，TCSAFLUSH：改变在所有写入STDIN_FILENO的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃
    //即终端不显示输入的字符
}

int main(){
  enableRawMode();
  char c;
  
  //read()、 STDIN_FILENO都来自 <unistd.h>, 使用read()从标准输入中得到1byte直到没有更多输入  
  while( read( STDIN_FILENO, &c, 1) == 1 && c != 'q' ){
    if( iscntrl(c)){  //iscntrl(c)检查c是否为c语言中的控制字符
      printf( "%d\n", c);   //c为控制字符，打印c的ASCII码
    }else{
      printf( "%d('%c')\n", c, c);  //打印c的ASCII码，并输出c
    }
  }

  return 0;
}
