#include "pch.h"
#include "CServer.h"
#pragma warning(disable:4407)

template <MyOperator op>
AcceptOverlapped<op>::AcceptOverlapped()
{
	m_operator = EAccept;
	// m_worker(this, &AcceptOverlapped::AcceptWorker); // ���ֲ���
	// m_worker = CThreadWorker(this, &AcceptOverlapped::AcceptWorker); // ����
	m_worker = CThreadWorker(this, (FUNCTYPE) & AcceptOverlapped<op>::AcceptWorker);  // ����ᱨ���棬����C4407��
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}

template <MyOperator op>
int AcceptOverlapped<op>::AcceptWorker() {  // ���ӵ�һ���ͻ��˾�acceptһ��
	INT lLength = 0, rLength = 0;
	if (*(LPDWORD)*m_client.get() > 0) {
		// ��ȡ���ѽ����׽��ֹ����ı��ص�ַ��Զ�̵�ַ
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)m_client->GetLocalAddr(), &lLength, // ���ص�ַ
			(sockaddr**)m_client->GetRemoteAddr(), &rLength  // Զ�̵�ַ
		);
		if (!m_server->NewAccept()) {
			return -2;
		}
	}
	return -1;
}


MyClient::MyClient() :m_isbusy(false), m_overlapped(new AcceptOVERLAPPED()) {
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

void MyClient::SetOverlapped(PCLIENT& ptr) {
	m_overlapped->m_client = ptr;
}

MyClient::operator LPOVERLAPPED() {
	return &m_overlapped->m_overlapped;
}

