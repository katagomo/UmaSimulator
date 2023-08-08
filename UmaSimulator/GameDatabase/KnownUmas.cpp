#include "GameDatabase.h"
#include "UmaData.h"
#include "../External/utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
using json = nlohmann::json;
using namespace std;

unordered_map<int, UmaData> GameDatabase::AllUmas;

void GameDatabase::loadUmas(const string& dir) 
{
    try
    {
        for (auto entry : filesystem::directory_iterator(dir + "/"))
        {
            //cout << entry.path() << endl;
            if (entry.path().extension() == ".json")
            {
                try
                {
                    ifstream ifs(entry.path());
                    stringstream ss;
                    ss << ifs.rdbuf();
                    ifs.close();
                    json j = json::parse(ss.str(), nullptr, true, true);

                    UmaData jdata;
                    j.get_to(jdata);
                    cout << "�������� #" << jdata.gameId << " " << jdata.name << endl;
                    if (AllUmas.count(jdata.gameId) > 0)
                        cout << "�����ظ������� #" << jdata.gameId << endl;
                    else
                        AllUmas[jdata.gameId] = jdata;
                }
                catch (exception& e)
                {
                    cout << "������ϢJSON����: " << entry.path() << endl << e.what() << endl;
                }
            }
        }
        cout << "������ " << AllUmas.size() << " ��������������" << endl;        
    }
    catch (exception& e)
    {
        cout << "��ȡ������Ϣ����: " << endl << e.what() << endl;
    }
    catch (...)
    {
        cout << "��ȡ������Ϣ����δ֪����" << endl;
    }
}