// videoRTSPServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "RTSPServer.h"


int main()
{
    RTSPServer server;
    server.Init();
    server.Invoke();
    printf("press any key to stop...\r\n");
    getchar();  // 等待用户输入一个字符
    server.Stop();
    return 0;
}

