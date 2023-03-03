#include<unistd.h>
#include<termios.h>

void enableRawMode() {				//可以通过以下方式设置terminal的属性： 
						//1.使用tcgetattr()将当前属性读入结构
  struct termios raw;				//2.手动修改结构
						//3.将修改后的结构传递给tcsetattr()以写入新的
  tcgetattr( STDIN_FILENO, &raw);		//terminal属性退出

  raw.c_lflag &= ~(ECHO);			//关闭ECHO

  tcsetattr( STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(){
  enableRawMode();
  char c;
  while( read( STDIN_FILENO, &c, 1) == 1 && c != 'q' );  
	 //read()、 STDIN_FILENO都来自 <unistd.h>, 使用read()从标准输入中得到1byte直到没有更多输入  

  return 0;
}
