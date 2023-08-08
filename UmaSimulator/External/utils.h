#ifndef UTILS_HPP
#define UTILS_HPP

#include "string"
#include "../External/json.hpp"
using json = nlohmann::json;

// ģ�庯�����ܷ���cpp�������Ҳ�������
template <typename T>
json arrayToJson(T* arr, int len)
{
	json j;
	for (int i = 0; i < len; ++i)
		j += arr[i];
	return j;
}

template <typename T>
int jsonToArray(const json& j, T* buf, int bufSize)
{
	int count = 0;
	if (!j.is_array() || bufSize <= 0)
		throw "Must be array";

	for (auto it : j) {
		buf[count++] = it;
		if (count >= bufSize) break;
	}
	return count;
}

// ����windows.h�ĺ�����Ҫ����cpp�������Ⱦȫ�������ռ�
std::string string_To_UTF8(const std::string& str);
std::string UTF8_To_string(const std::string& str);
#endif