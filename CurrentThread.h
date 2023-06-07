#pragma once 

#include "unistd.h"
#include <sys/syscall.h>

namespace CurrentThread {
    extern __thread int t_cacheTid;

    void cacheTid();

    inline int tid() { // 内联函数，只在当前文件中起作用
        if(__builtin_expect(t_cacheTid == 0, 0)) { // 处理分支预测
            cacheTid();
        }
        return t_cacheTid;
    }
}