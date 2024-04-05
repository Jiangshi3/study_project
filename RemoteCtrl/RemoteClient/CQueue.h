#pragma once
template<class T>
class CQueue
{// �̰߳�ȫ�Ķ��У�����IOCPʵ�֣�
public:
	CQueue();
	~CQueue();
	bool PushBack(const T& data);
	bool PopFront(T& data);
	size_t size();
	void clear();
private:
	static void threadEntry(void* arg);
	void threadMain();
private:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
public:
	typedef struct IocpParam {
		int nOperator; // ����
		T strData; // ����
		HANDLE hEvent;  // pop������Ҫ��
		IocpParam() {
			nOperator = -1;
		}
		IocpParam(int op, const char* pData, _beginthread_proc_type cb = NULL) {
			nOperator = op;
			strData = pData;
			cbFunc = cb;
		}
	}PPARAM; // Post Parameter ����Ͷ����Ϣ�Ľṹ��

	enum {
		QPush,
		QPop,
		QSize,
		QClear
	};
};

