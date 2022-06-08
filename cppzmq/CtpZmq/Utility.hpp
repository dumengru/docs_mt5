#ifndef __UTILITY_HPP__
#define __UTILITY_HPP__


#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>

#include <codecvt> 
#include "RdJson.hpp"
#include <zmq.h>
#include <vector>

#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"

//--- �ź�
class semaphore
{
public:
	semaphore(int value = 1) :count(value) {}

	void wait()
	{
		std::unique_lock<std::mutex> lck(mt);
		if (--count < 0)//��Դ��������߳�
			cv.wait(lck);
	}

	void signal()
	{
		std::unique_lock<std::mutex> lck(mt);
		if (++count <= 0)//���̹߳��𣬻���һ��
			cv.notify_one();
	}

public:
	int count;
	std::mutex mt;
	std::condition_variable cv;
};
semaphore _semaphore(0);

//--- �ָ��ַ���
std::vector<std::string> split(std::string str, std::string pattern)
{
	std::string::size_type pos;
	std::vector<std::string> result;
	str += pattern;//��չ�ַ����Է������
	int size = (int)str.size();
	for (int i = 0; i < size; i++)
	{
		pos = str.find(pattern, i);
		if (pos < size)
		{
			std::string s = str.substr(i, pos - i);
			result.push_back(s);
			i = (int)(pos + pattern.size() - 1);
		}
	}
	return result;
}



double double_format(double value)
{
	return value == DBL_MAX ? 0.0 : value;
}

#endif // __UTILITY_HPP__