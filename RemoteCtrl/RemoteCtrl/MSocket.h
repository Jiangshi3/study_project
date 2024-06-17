#pragma once
#include <WinSock2.h>
#include <memory>

/*
* 1 易用性
*	a 简化参数
*	b 类型适配（参数适配）
*	c 流程简化
* 2 易移植性（高内聚，低耦合）
*	a 核心功能到底是什么？
*	b 业务逻辑是什么？
*	【哪些是给用户去做的，哪些是自己去管理】
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
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str());  // inet_addr(); 将字符串形式的IP地址转换成32位整形数据，并转换成网络字节序列；
	}
	MSockaddrIn(const MSockaddrIn& addr) {
		memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
		m_ip = addr.GetIP();
		m_port = addr.GetPort();
		//m_ip = addr.m_ip;  // TODO :老师没写错，可以这样写
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
	} // 这里传出的一个地址，有可能会更改，所以需要更新
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
	inline int size() const { return sizeof(sockaddr_in); } // 返回sockaddr_in的大小
private:
	sockaddr_in m_addr;
	std::string m_ip;
	short m_port;
};

// MBuffer就可以直接使用string的函数，比如：.size();
class MBuffer :public std::string {
public:
	MBuffer(size_t size = 0) : std::string() {     // 子类的构造函数，父类的构造函数也要调用
		if (size > 0) {
			resize(size);
			// memset((void*)c_str(), 0, size);
			memset(*this, 0, this->size());        // 第一个参数*this，是因为有操作符重载
		}
	}
	MBuffer(void* buffer, size_t size):std::string() { // 子类的构造函数，父类的构造函数也要调用
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	MBuffer(const char* str) {
		// operator=(str); // 会无限递归造成堆栈溢出
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}
	void Update(void* buffer, size_t size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	~MBuffer() {
		std::string::~basic_string(); // 这个析构函数不是virtual，所以需要手动调用基类的析构函数
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
	MTypeTCP = 1,  // 刚好与上面的宏定义对应
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
		// m_sock = sock.m_sock;  // 不能直接拷贝出来； 只能去创建同样协议的套接字； 使用智能指针
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
			addr.update(); // 要记得更新； 拿到后放入MSockaddrIn类的成员变量m_addr中，还没有更新
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

