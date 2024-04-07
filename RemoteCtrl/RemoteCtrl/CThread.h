#pragma once
#include "pch.h"
#include <Windows.h>
#include <atomic>
#include <vector>
#include <mutex>

class CThreadFuncBase {};  // 自己的类只要继承了CThreadFuncBase，并写好自己的成员函数； 就可以把功能传递到CThreadWorker；然后通过线程池传递到线程中；
typedef int(CThreadFuncBase::* FUNCTYPE)();  // ThreadFuncBase类的成员函数指针
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
	bool IsValid() { // 监控进程状态; 返回true表示有效，返回false表示线程异常或者终止
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
	void UpdateWorker(const CThreadWorker& worker = CThreadWorker()) { // 默认为空
		if (!worker.IsValid()) {
			m_worker.store(NULL);  // 置空
			return;
		}
		if (m_worker.load() != NULL) {
			CThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		m_worker.store(new CThreadWorker(worker));
	}
	// true表示空间，false表示已经分配了工作
	bool IsIdle() {
		return !m_worker.load()->IsValid();
	}

private:
	void ThreadWorker() {  // 线程里面要执行的内容就完全脱离了这个线程；  此线程就是一个框架；
		while (m_bStatus) {
			CThreadWorker worker = *m_worker.load();
			if (worker.IsValid()) {
				int ret = worker();    // 返回值等于0正常； 返回值不等于0提示错误信息；返回值小于0表示错误，提示错误信息并且终止；
				if (ret != 0) {
					CString str;
					str.Format(_T("thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				if (ret < 0) {
					// 目的：线程结束之后不结束掉线程,开启线程消耗资源大； 执行完后让线程置空，不结束；
					m_worker.store(NULL);  // 如果出错，就存储一个空；  
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
	bool m_bStatus;  // true表示线程正在运行； false表示线程关闭
	std::atomic<CThreadWorker*> m_worker;
};



class CThreadPool {
public:
	CThreadPool(){}
	CThreadPool(size_t size) {
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++) {
			m_threads[i] = new CThread();  // 使用指针后的初始化，需要主动调用new； 如果不使用指针，会自动调用CThread()的默认构造函数；
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
				for (size_t j = 0; j < i; j++) { // IDO：把已经启动的线程关闭掉
					m_threads[j]->Stop();
				}
				ret = false; // 线程池都启动成功，才认为是成功
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
	// 如果返回-1，说明没有空闲线程，分配失败； 大于等于0，表示分配第n个线程来做这个事情
	int DispatchWorker(const CThreadWorker& worker) {
		int index = -1;
		m_lock.lock();
		/*
		优化：把所有的空闲进程编号放在一个列表中，只需要判断列表的size()就可以知道有没有空闲进程；用掉一个就移除掉，空闲后再添加进去；
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
	std::vector<CThread*> m_threads;  // 改为：使用指针
};
