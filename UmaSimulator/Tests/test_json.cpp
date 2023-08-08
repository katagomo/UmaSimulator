#include <iostream>
#include <iomanip> 
#include <sstream>
#include <fstream>
#include <cassert>
#include <thread>  // for std::this_thread::sleep_for
#include <chrono>  // for std::chrono::seconds

#include "../Game/Game.h"
#include "../GameDatabase/GameConfig.h"
#include "../Search/Search.h"
#include "windows.h"
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <algorithm>
using namespace std;
using json = nlohmann::json;

wchar_t buf[10240];

#define COMPARE(key) if ( a.key != b.key ) { cout << (#key) << ": �ֶ� = " << a.key << ", ����ֵ = " << b.key << endl; ret = false; }

template <typename T>
bool compare_array(const T* a, const T* b, size_t len, const string& key)
{
	bool ret = true;
	for (int i=0; i<len; ++i)
		if (a[i] != b[i])
		{
			ret = false;
			break;
		}
	if (!ret) {
		cout << key << ": A: [";
		for (int i = 0; i < len; ++i)
			cout << a[i] << ", ";
		cout << "], B: [";
		for (int i = 0; i < len; ++i)
			cout << b[i] << ", ";
		cout << "]" << endl;
	}
	return ret;
		
}

bool compare_card_value(const SupportCard& a, const SupportCard& b)
{
	bool ret = true;
	//	CardValue level[5];	// ����ͻ�Ƶȼ�������
	COMPARE(youQingBasic);//����ӳ�
	COMPARE(ganJingBasic);//�ɾ��ӳ�
	COMPARE(xunLianBasic);//ѵ���ӳ�
	compare_array(a.bonusBasic, b.bonusBasic, 6, "bonusBasic");
	COMPARE(wizVitalBonusBasic);//������Ȧ�����ظ���
	compare_array(a.initialBonus, b.initialBonus, 6, "initialBonus");
	COMPARE(initialJiBan);//��ʼ�
	COMPARE(saiHou);//����
	compare_array(a.hintBonus, b.hintBonus, 6, "hintBonus");
	COMPARE(hintProbIncrease);//��������������
	COMPARE(deYiLv);//������
	COMPARE(failRateDrop); //ʧ���ʽ���
	COMPARE(vitalCostDrop); //���������½�
	return ret;
}

bool compare_card(const SupportCard& a, const SupportCard& b)
{
    bool ret = true;
    COMPARE(cardID);
	COMPARE(cardType);
	COMPARE(cardName);
	for (int i = 0; i < 5; ++i)
	{
		if (a.filled) {
			ret &= compare_card_value(a, b);
		}
	}
//	if (!ret)
//		cout << "(�ο�����): " << b.uniqueText << endl;
	//SkillList cardSkill;
	return ret;
}

void test_compare_cards()
{
	for (auto c : GameDatabase::AllCards)
	{
		cout << "- ��� " << c.second.cardName << endl;
		if (!GameDatabase::DBCards.count(c.first))
		{
			cout << "�޴˿�ƬID��" << c.first << endl;
		}
		auto dbCard = GameDatabase::DBCards[c.first];
		compare_card(c.second, dbCard);
	}
}

void main_test_json()
{
  // ��鹤��Ŀ¼
  GetModuleFileNameW(0, buf, 10240);
  filesystem::path exeDir = filesystem::path(buf).parent_path();
  filesystem::current_path(exeDir);
  std::cout << "��ǰ����Ŀ¼��" << filesystem::current_path() << endl;
  cout << "��ǰ����Ŀ¼��" << exeDir << endl;

  GameDatabase::loadUmas("db/uma");
  GameDatabase::loadCards("db/card");
  GameDatabase::loadDBCards("db/cardDB.json");
  GameConfig::load("aiConfig.json");
  cout << "�������" << endl;

  // todo: ������������ݵļ��
  test_compare_cards();
}