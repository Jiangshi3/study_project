#pragma once
#include <WinSock2.h>
#include <memory>

/*
* 1 ������
*	a �򻯲���
*	b �������䣨�������䣩
*	c ���̼�
* 2 ����ֲ�ԣ����ھۣ�����ϣ�
*	a ���Ĺ��ܵ�����ʲô��
*	b ҵ���߼���ʲô��
*	����Щ�Ǹ��û�ȥ���ģ���Щ���Լ�ȥ����
*/

/*
* #define SOCK_STREAM     1               
* #define SOCK_DGRAM      2       
*/


class MSockaddrIn {
public:
	MSockaddrIn() {
		memset(&m_addr, 0, sizeof(m_addr));
		m_port = -1;
	}
	MSockaddrIn(sockaddr_in addr) {
		memcpy(&m_addr, &addr, sizeof(addr));
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	MSockaddrIn(UINT nIP, short nPort) {
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = htonl(nIP);
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = nPort;
	}
	MSockaddrIn(const std::string& strIP, short nPort) {
		m_ip = strIP;
		m_port = nPort;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str());  // inet_addr(); ���ַ�����ʽ��IP��ַת����32λ�������ݣ���ת���������ֽ����У�
	}
	MSockaddrIn(const MSockaddrIn& addr) {
		memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
		m_ip = addr.GetIP();
		m_port = addr.GetPort();
		//m_ip = addr.m_ip;  // TODO :��ʦûд����������д
		//m_port = addr.m_port;
	}
	MSockaddrIn& operator=(const MSockaddrIn& addr) {
		if (this != &addr) {
			memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
			m_ip = addr.GetIP();
			m_port = addr.GetPort();
		}
		return *this;
	}
	operator sockaddr* () const {
		return (sockaddr*)&m_addr;
	} // ���ﴫ����һ����ַ���п��ܻ���ģ�������Ҫ����
	void update() {
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	operator void* () {
		return (void*)&m_addr;
	}
	std::string GetIP() const {
		return m_ip;
	}
	short GetPort() const {
		return m_port;
	}
	inline int size() const { return sizeof(sockaddr_in); } // ����sockaddr_in�Ĵ�С
private:
	sockaddr_in m_addr;
	std::string m_ip;
	short m_port;
};

// MBuffer�Ϳ���ֱ��ʹ��string�ĺ��������磺.size();
class MBuffer :public std::string {
public:
	MBuffer(size_t size = 0) : std::string() {     // ����Ĺ��캯��������Ĺ��캯��ҲҪ����
		if (size > 0) {
			resize(size);
			// memset((void*)c_str(), 0, size);
			memset(*this, 0, this->size());        // ��һ������*this������Ϊ�в���������
		}
	}
	MBuffer(void* buffer, size_t size):std::string() { // ����Ĺ��캯��������Ĺ��캯��ҲҪ����
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	MBuffer(const char* str) {
		// operator=(str); // �����޵ݹ���ɶ�ջ���
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}
	void Update(void* buffer, size_t size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	~MBuffer() {
		std::string::~basic_string(); // ���������������virtual��������Ҫ�ֶ����û������������
	}
	operator char* () const { return (char*)c_str(); }
	operator const char* () const { return c_str(); }
	operator BYTE* () const { return (BYTE*)c_str(); }
	operator void* () const { return (void*)c_str(); }
	//MBuffer& operator=(const char* str) {
	//	std::string::operator=(str);
	//	return *this;
	//}
};


enum MTYPE{
	MTypeTCP = 1,  // �պ�������ĺ궨���Ӧ
	MTypeUDP
};
class MSocket
{
public:
	MSocket(MTYPE nType = MTypeTCP, int nProtocol = 0) {
		m_sock = socket(PF_INET, nType, nProtocol);
		m_type = nType;
		m_protocol = nProtocol;
	}
	MSocket(const MSocket& sock) {
		// m_sock = sock.m_sock;  // ����ֱ�ӿ��������� ֻ��ȥ����ͬ��Э����׽��֣� ʹ������ָ��
		m_sock = socket(PF_INET, sock.m_type, sock.m_protocol);
		m_type = sock.m_type;
		m_protocol = sock.m_protocol;
		m_addr = sock.m_addr;
	}
	MSocket& operator=(const MSocket& sock) {
		if (this != &sock) {
			m_sock = socket(PF_INET, sock.m_type, sock.m_protocol);
			m_type = sock.m_type;
			m_protocol = sock.m_protocol;
			m_addr = sock.m_addr;
		}
		return *this;
	}
	~MSocket(){
		close();
	}
	operator SOCKET() const { return m_sock;}
	operator SOCKET() { return m_sock; }
	bool operator ==(SOCKET sock) const {
		return m_sock == sock;
	}
	int bind(const std::string& ip, short port) {
		m_addr = MSockaddrIn(ip, port);
		return ::bind(m_sock, m_addr, m_addr.size());
	}
	int listen(int backlog = 5) {
		if (m_type != MTypeTCP) return -1;
		return ::listen(m_sock,backlog);
	}
	int accept(){}
	int connect(const std::string& ip, short port) {}
	// int send(const char* buffer, size_t size)
	int send(const MBuffer& buffer)	{
		return ::send(m_sock, buffer, buffer.size(), 0);
	}
	int recv(MBuffer& buffer){
		return ::recv(m_sock, buffer, buffer.size(), 0);
	}
	int sendto(const MBuffer& buffer, const MSockaddrIn& addr){
		return ::sendto(m_sock, buffer, buffer.size(), 0, addr, addr.size());
	}
	int recvfrom(MBuffer& buffer, MSockaddrIn& addr) {
		int len = addr.size();
		int ret = ::recvfrom(m_sock, buffer, buffer.size(), 0, addr, &len);
		if (ret > 0) {
			addr.update(); // Ҫ�ǵø��£� �õ������MSockaddrIn��ĳ�Ա����m_addr�У���û�и���
		}
		return ret;
	}
	void close() {
		if (m_sock != INVALID_SOCKET) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
		}
	}
private:
	SOCKET m_sock;
	MTYPE m_type;
	int m_protocol;
	MSockaddrIn m_addr;
};

typedef std::shared_ptr<MSocket> MSOCKET;

