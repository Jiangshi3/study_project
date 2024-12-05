#include "pch.h"
#include "ServerSocket.h"

// 没使用单例模式之前：
// CServerSocket server;  // 全局变量；会先初始化，然后在执行main()函数；
// 在main()之前执行，肯定不存在多线程的问题，此时主线程都还没有创建；
// 如果没有下面的m_helper，则是懒汉式，初始化为null，什么时候用到m_instance才创建；但在静态的m_helper中，构造函数中调用了getinstance();
CServerSocket* CServerSocket::m_instance = NULL;  // 静态成员函数显示初始化
// 声明：static CHelper m_helper;
CServerSocket::CHelper CServerSocket::m_helper;   // 实现
CServerSocket* pServer = CServerSocket::getInstance();   // 这里不能注释?，是饿汉模式；【好像也可以注释掉，因为有上面的m_helper】


