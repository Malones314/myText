#include<unistd.h>
#include<termios.h>
#include<stdlib.h>

struct termios orig_termios;

void disableRawMode(){    //得到terminal的副本，配合atexit()函数在程序结束时还原terminal
  tcsetattr( STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

//可以通过以下方式设置terminal的属性：
//1.使用tcgetattr()将当前属性读入结构
//2.手动修改结构
//3.将修改后的结构传递给tcsetattr()以写入新的terminal属性
void enableRawMode() {				       
  tcgetattr( STDIN_FILEND, &orig_termios);
  atexit( disableRawMode);  //atexit 是线程安全的：从多个线程调用该函数不会引发数据竞争。
                            //用它来注册 disableRawMode() 函数，以便在程序退出时自动调用
  struct termios raw = orig_termios;	        
  raw.c_lflag &= ~(ECHO | ICANON);			//关闭ECHO、ICANON,c_lflag：本地模式
                                        //ICANON：标准模式，关闭后逐字节读取，而非逐行读取
  tcsetattr( STDIN_FILENO, TCSAFLUSH, &raw);
    //STDIN_FILENO：stdin的文件编号，TCSAFLUSH：改变在所有写入STDIN_FILENO的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃
    //即终端不显示输入的字符
}

int main(){
  enableRawMode();
  char c;
  while( read( STDIN_FILENO, &c, 1) == 1 && c != 'q' );  
	 //read()、 STDIN_FILENO都来自 <unistd.h>, 使用read()从标准输入中得到1byte直到没有更多输入  

  return 0;
}
