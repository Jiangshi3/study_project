#include "pch.h"
#include "ServerSocket.h"

// 没使用单例模式之前：
// CServerSocket server;  // 全局变量；会先初始化，然后在执行main()函数；
// 在main()之前执行，肯定不存在多线程的问题，此时主线程都还没有创建；

CServerSocket* CServerSocket::m_instance = NULL;  // 静态成员函数显示初始化
// 声明：static CHelper m_helper;
CServerSocket::CHelper CServerSocket::m_helper;   // 实现
// CServerSocket* pServer = CServerSocket::getInstance();


