#include "pch.h"
#include "ServerSocket.h"

// ûʹ�õ���ģʽ֮ǰ��
// CServerSocket server;  // ȫ�ֱ��������ȳ�ʼ����Ȼ����ִ��main()������
// ��main()֮ǰִ�У��϶������ڶ��̵߳����⣬��ʱ���̶߳���û�д�����
// ���û�������m_helper����������ʽ����ʼ��Ϊnull��ʲôʱ���õ�m_instance�Ŵ��������ھ�̬��m_helper�У����캯���е�����getinstance();
CServerSocket* CServerSocket::m_instance = NULL;  // ��̬��Ա������ʾ��ʼ��
// ������static CHelper m_helper;
CServerSocket::CHelper CServerSocket::m_helper;   // ʵ��
CServerSocket* pServer = CServerSocket::getInstance();   // ���ﲻ��ע��?���Ƕ���ģʽ��������Ҳ����ע�͵�����Ϊ�������m_helper��


