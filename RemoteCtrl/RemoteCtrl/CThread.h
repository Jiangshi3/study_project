#pragma once
#include "pch.h"
#include <Windows.h>
#include <atomic>
#include <vector>
#include <mutex>

class CThreadFuncBase {};  // �Լ�����ֻҪ�̳���CThreadFuncBase����д���Լ��ĳ�Ա������ �Ϳ��԰ѹ��ܴ��ݵ�CThreadWorker��Ȼ��ͨ���̳߳ش��ݵ��߳��У�
typedef int(CThreadFuncBase::* FUNCTYPE)();  // ThreadFuncBase��ĳ�Ա����ָ��
class CThreadWorker {
public:
	CThreadWorker() :thiz(NULL), func(NULL) {};
	CThreadWorker(CThreadFuncBase* pobj, FUNCTYPE f): thiz(pobj), func(f) {}
	CThreadWorker(const CThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}
	CThreadWorker& operator=(const CThreadWorker& worker) {
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}
	// CThreadWorker& operator=(CThreadWorker&& worker) = delete;

	int operator()() {
		if (IsValid()) {
			return (thiz->*func)();
		}
		return -1;		
	}
	bool IsValid() const{
		return (thiz != NULL) && (func != NULL);
	}
private:
	CThreadFuncBase* thiz;
	FUNCTYPE func;
};


class CThread
{
public:
	CThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}
	~CThread() {
		Stop();
	}
	bool Start() {
		if (IsValid()) {
			return true;
		}
		m_hThread = (HANDLE)_beginthread(&CThread::ThreadEntry, 0, this);
		if (IsValid()) {
			m_bStatus = true;
		}
		return m_bStatus;
	}
	bool IsValid() { // ��ؽ���״̬; ����true��ʾ��Ч������false��ʾ�߳��쳣������ֹ
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE))
			return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}
	bool Stop() {
		if (!m_bStatus) 
			return true;
		if (WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0) {
			m_bStatus = false;
			UpdateWorker();
			return true;
		}
		UpdateWorker();
		return false;
	}
	void UpdateWorker(const CThreadWorker& worker = CThreadWorker()) { // Ĭ��Ϊ��
		if (!worker.IsValid()) {
			m_worker.store(NULL);  // �ÿ�
			return;
		}
		if (m_worker.load() != NULL) {
			CThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		m_worker.store(new CThreadWorker(worker));
	}
	// true��ʾ�ռ䣬false��ʾ�Ѿ������˹���
	bool IsIdle() {
		return !m_worker.load()->IsValid();
	}

private:
	void ThreadWorker() {  // �߳�����Ҫִ�е����ݾ���ȫ����������̣߳�  ���߳̾���һ����ܣ�
		while (m_bStatus) {
			CThreadWorker worker = *m_worker.load();
			if (worker.IsValid()) {
				int ret = worker();    // ����ֵ����0������ ����ֵ������0��ʾ������Ϣ������ֵС��0��ʾ������ʾ������Ϣ������ֹ��
				if (ret != 0) {
					CString str;
					str.Format(_T("thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				if (ret < 0) {
					// Ŀ�ģ��߳̽���֮�󲻽������߳�,�����߳�������Դ�� ִ��������߳��ÿգ���������
					m_worker.store(NULL);  // ��������ʹ洢һ���գ�  
				}
			}
			else {
				Sleep(1);
			}
		}
	}
	static void ThreadEntry(void* arg) {
		CThread* thiz = (CThread*)arg;
		thiz->ThreadWorker();
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus;  // true��ʾ�߳��������У� false��ʾ�̹߳ر�
	std::atomic<CThreadWorker*> m_worker;
};



class CThreadPool {
public:
	CThreadPool(){}
	CThreadPool(size_t size) {
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++) {
			m_threads[i] = new CThread();  // ʹ��ָ���ĳ�ʼ������Ҫ��������new�� �����ʹ��ָ�룬���Զ�����CThread()��Ĭ�Ϲ��캯����
		}
	}
	~CThreadPool() {
		Stop();
		m_threads.clear();
	}
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->Start() == false) {
				for (size_t j = 0; j < i; j++) { // IDO�����Ѿ��������̹߳رյ�
					m_threads[j]->Stop();
				}
				ret = false; // �̳߳ض������ɹ�������Ϊ�ǳɹ�
				break;
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) { 
			m_threads[i]->Stop();
		}
	}
	// �������-1��˵��û�п����̣߳�����ʧ�ܣ� ���ڵ���0����ʾ�����n���߳������������
	int DispatchWorker(const CThreadWorker& worker) {
		int index = -1;
		m_lock.lock();
		/*
		�Ż��������еĿ��н��̱�ŷ���һ���б��У�ֻ��Ҫ�ж��б��size()�Ϳ���֪����û�п��н��̣��õ�һ�����Ƴ��������к�����ӽ�ȥ��
		*/
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->IsIdle()) {
				m_threads[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}

private:
	std::mutex m_lock;
	std::vector<CThread*> m_threads;  // ��Ϊ��ʹ��ָ��
};
