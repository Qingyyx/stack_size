# stack_size
ai 写了个递归re，看栈大小的代码  


- SIGSEGV 信号：当程序访问非法内存（如栈溢出）时触发。
- 使用 sigaltstack 设置备用栈，确保在栈溢出时信号处理程序可以使用这块独立的栈空间。
- 如果不设置备用栈，栈溢出时信号处理程序可能无法运行，因为主栈已经耗尽。

```
gcc -O0 -fno-stack-protector -D_GNU_SOURCE stack.c -o stack
./stack
```

输出：
```
正在启动栈空间测量...

栈空间分析报告：
最高栈地址: 0x7ffea93a1864
最低栈地址: 0x7ffea8ba5004
实际使用量: 7.99 MB
```
