#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
volatile void* max_ptr = NULL;
volatile void* min_ptr = NULL;

// 异步信号安全输出
void safe_print(const char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

// 信号处理函数
void stack_overflow_handler(int sig) {
    char buf[128];
    ptrdiff_t stack_size = (char*)max_ptr - (char*)min_ptr;
    stack_size = stack_size > 0 ? stack_size : -stack_size;

    snprintf(buf, sizeof(buf),
        "\n栈空间分析报告：\n"
        "最高栈地址: %p\n"
        "最低栈地址: %p\n"
        "实际使用量: %.2f MB\n\n",
        max_ptr, min_ptr, stack_size / (1024.0 * 1024.0));

    safe_print(buf);
    _exit(0);
}

// 禁止优化的递归函数
__attribute__((noinline, optimize("O0")))
void recursive_stack_probe() {
    volatile int stack_marker; // 地址标记
    void* current = &stack_marker;

    // 更新地址范围
    max_ptr = (max_ptr == NULL || current > max_ptr) ? current : max_ptr;
    min_ptr = (min_ptr == NULL || current < min_ptr) ? current : min_ptr;

    // 安全保护（防止无限递归）
    if ((char*)max_ptr - (char*)min_ptr > 8 * 1024 * 1024) {
        safe_print("安全机制：超过8MB递归深度\n");
        _exit(1);
    }

    recursive_stack_probe();

    // 内存屏障防止优化
    asm volatile("" : : "r"(current));
}

int main() {
    // 配置信号处理（Linux专用版）
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = stack_overflow_handler;
    sa.sa_flags = SA_ONSTACK | SA_NODEFER; // 使用通用标志替代SA_RESETHAND

    // 设置备用信号栈
    stack_t ss;
    ss.ss_sp = malloc(SIGSTKSZ);
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;

    if (sigaltstack(&ss, NULL) == -1) {
        safe_print("无法设置备用栈\n");
        return 1;
    }

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        safe_print("信号处理配置失败\n");
        return 1;
    }

    safe_print("正在启动栈空间测量...\n");
    recursive_stack_probe();
    return 0;
}
// gcc -O0 -fno-stack-protector -D_GNU_SOURCE stack.c -o stack
