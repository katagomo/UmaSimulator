#include <iostream>
#include <iomanip> 
#include <sstream>
#include <fstream>
#include <cassert>
#include <thread>  // for std::this_thread::sleep_for
#include <chrono>  // for std::chrono::seconds

#include "../Game/Game.h"
#include "../GameDatabase/GameDatabase.h"
#include "../GameDatabase/GameConfig.h"
#include "../Search/Search.h"
#include "../External/utils.h"

#include "windows.h"
#include <filesystem>
#include <cstdlib>
using namespace std;
using json = nlohmann::json;

template <typename T, std::size_t N>
std::size_t findMaxIndex(const T(&arr)[N]) {
    return std::distance(arr, std::max_element(arr, arr + N));
}

map<string, string> rpText;
void loadRole()
{
    try
    {
        ifstream ifs("db/roleplay.json");
        stringstream ss;
        ss << ifs.rdbuf();
        ifs.close();

        rpText.clear();
        json j = json::parse(ss.str(), nullptr, true, true);
        json entry = j.at(GameConfig::role);
        for (auto & item : entry.items())
        {
            rpText[item.key()] = UTF8_To_string(item.value());
        }
        cout << "��ǰRP��ɫ��" << rpText["name"] << endl;
    }
    catch (...)
    {
        cout << "��ȡ������Ϣ����roleplay.json" << endl;
    }
}

void print_luck(int luck)
{
    int u = -1000;
    int sigma = 1200;
    string color = "";
    if (luck > 20000) u = 27000;

    if (!GameConfig::noColor)
    {
        if (luck > u + sigma / 2)
            color = "\033[32m"; // 2 3 1
        else if (luck > u - sigma / 2)
            color = "\033[33m";
        else
            color = "\033[31m";
        cout << color << luck << "\033[0m";
    }
    else
        cout << luck;
}

void main_test6()
{
  //const double radicalFactor = 5;//������
  //const int threadNum = 16; //�߳���
 // const int searchN = 12288; //ÿ��ѡ������ؿ���ģ��ľ���

  //������Ϊk��ģ��n��ʱ����׼��ԼΪsqrt(1+k^2/(2k+1))*1200/(sqrt(n))
  //��׼�����30ʱ������Ӱ���ж�׼ȷ��

  Search search;
  vector<Evaluator> evaluators;

  int lastTurn = -1;
  int scoreFirstTurn = 0;   // ��һ�غϷ���
  int scoreLastTurn = 0;   // ��һ�غϷ���

  // ��鹤��Ŀ¼
  wchar_t buf[10240];
  GetModuleFileNameW(0, buf, 10240);
  filesystem::path exeDir = filesystem::path(buf).parent_path();
  filesystem::current_path(exeDir);
  //std::cout << "��ǰ����Ŀ¼��" << filesystem::current_path() << endl;
  cout << "��ǰ����Ŀ¼��" << exeDir << endl;
  GameConfig::load("./aiConfig.json");
  GameDatabase::loadUmas("./db/uma");
  GameDatabase::loadCards("./db/card");
  if(GameConfig::extraCardData == true)
    GameDatabase::loadDBCards("./db/cardDB.json");
  loadRole();   // roleplay

  string currentGameStagePath = string(getenv("LOCALAPPDATA"))+ "/UmamusumeResponseAnalyzer/packets/currentGS.json";


  for (int i = 0; i < GameConfig::threadNum; i++)
      evaluators.push_back(Evaluator(NULL, 128));

  while (true)
  {
    while (!filesystem::exists(currentGameStagePath))
    {
      std::cout << "�Ҳ���" + currentGameStagePath + "����������Ϸδ��ʼ��С�ڰ�δ��������" << endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));//�ӳټ��룬����ˢ��
    }
    ifstream fs(currentGameStagePath);
    if (!fs.good())
    {
      cout << "��ȡ�ļ�����" << endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));//�ӳټ��룬����ˢ��
      continue;
    }
    ostringstream tmp;
    tmp << fs.rdbuf();
    fs.close();

    string jsonStr = tmp.str();
    Game game;
    bool suc = game.loadGameFromJson(jsonStr);
    if (!suc)
    {
      cout << "���ִ���" << endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));//�ӳټ��룬����ˢ��
      continue;
    }
    if (game.turn == lastTurn)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(300));//����Ƿ��и���
      continue;
    }
    lastTurn = game.turn;
    if (game.venusIsWisdomActive)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      continue;
    }

    if (game.turn == 0)//��һ�غϣ���������ai�ĵ�һ�غ�
    {
      scoreFirstTurn = 0;
      scoreLastTurn = 0;
    }
    game.print();
    cout << endl;
    cout << rpText["name"] << rpText["calc"];

    auto printPolicy = [](float p)
    {
      cout << fixed << setprecision(1);
      if (!GameConfig::noColor)
      {
        if (p >= 0.3)cout << "\033[33m";
        //else if (p >= 0.1)cout << "\033[32m";
        else cout << "\033[36m";
      }
      cout << p * 100 << "% ";
      if (!GameConfig::noColor)cout << "\033[0m";
    };
    
    //��󼸻غϽ��ͼ�����
    double modifiedRadicalFactor = GameConfig::radicalFactor * (1 - exp(-double(TOTAL_TURN - game.turn) / 10.0));
    
    search.runSearch(game, evaluators.data(), GameConfig::searchN, TOTAL_TURN, 0, GameConfig::threadNum, modifiedRadicalFactor);
    //cout << endl << rpText["finish"] << endl;
    cout << endl << rpText["analyze"] << " >>" << endl;
    {
      auto policy = search.extractPolicyFromSearchResults(1);
      if (game.venusAvailableWisdom != 0)
      {
        cout << "ʹ��Ů���ʣ�";
        printPolicy(policy.useVenusPolicy);
        cout << endl;
      }
      if (!game.isRacing)
      {
        if (game.venusAvailableWisdom != 0)
        {
          cout << "��" << (policy.useVenusPolicy > 0.5 ? "" : " �� ") << "ʹ��Ů���ǰ���£�";
        }

        cout << "���������ǣ�";
        for (int i = 0; i < 5; i++)
          printPolicy(policy.trainingPolicy[i]);
        cout << endl;

        cout << "��Ϣ�������������";
        for (int i = 0; i < 3; i++)
          printPolicy(policy.trainingPolicy[5 + i]);
        cout << endl;

        cout << "Ů����ѡһ�¼����죬�����ƣ�";
        for (int i = 0; i < 3; i++)
          printPolicy(policy.threeChoicesEventPolicy[i]);
        cout << endl;

        cout << "���Ů������Լ���ͨ�����";
        for (int i = 0; i < 6; i++)
          printPolicy(policy.outgoingPolicy[i] * policy.trainingPolicy[6]);
        cout << endl << "<<" << endl;

        // ���������
        float maxScore = -1e6;
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                float s = search.allChoicesValue[i][j].avgScoreMinusTarget;
                if (s > maxScore)maxScore = s;
            }
        }
        if (game.turn == 0 || scoreFirstTurn == 0)
        {
            scoreFirstTurn = maxScore;
        }
        else
        {
            cout << rpText["luck"] << " | ���֣�";
            print_luck(maxScore - scoreFirstTurn);
            cout << " | ���غϣ�" << maxScore - scoreLastTurn
                 << " | ����Ԥ��: " << maxScore << endl;

            double raceLoss = maxScore - max(search.allChoicesValue[0][7].avgScoreMinusTarget, search.allChoicesValue[1][7].avgScoreMinusTarget);
            if (raceLoss < 5e5)//raceLoss��Լ1e6������ܱ���
                cout << "������������ѡ������غϣ�����ɷ�˿��Ŀ�꣩��" << raceLoss << endl;
            cout << "----" << endl;
            cout.flush();
        }
        scoreLastTurn = maxScore;

        // ������غϾ���
        cout << (GameConfig::noColor ? "" : "\033[1m\033[33m") << rpText["name"] << rpText["decision"] << ": "
             << (GameConfig::noColor ? "" : "\033[32m");

        std::size_t trainChoice = findMaxIndex(policy.trainingPolicy);
        switch (trainChoice) {
            case 0:
                cout << rpText["speed"];
                break;
            case 1:
                cout << rpText["stamina"];
                break;
            case 2:
                cout << rpText["power"];
                break;
            case 3:
                cout << rpText["guts"];
                break;
            case 4:
                cout << rpText["wisdom"];
                break;
            case 5:
                cout << rpText["rest"];
                break;
            case 6:
            {
                cout << "�� ";
                std::size_t outgoingPolicy = findMaxIndex(policy.outgoingPolicy);
                switch (outgoingPolicy) {
                case 0:
                    cout << (GameConfig::noColor ? "" : "\033[31m") << "��Ů��";
                    break;
                case 1:
                    cout << (GameConfig::noColor ? "" : "\033[36m") << "��Ů��";
                    break;
                case 2:
                    cout << (GameConfig::noColor ? "" : "\033[33m") << "��Ů��";
                    break;
                case 3:
                    cout << (GameConfig::noColor ? "" : "\033[36m") << rpText["venus1"];
                    break;
                case 4:
                    cout << (GameConfig::noColor ? "" : "\033[36m") << rpText["venus2"];
                    break;
                case 5:
                    cout << (GameConfig::noColor ? "" : "\033[35m") << rpText["date"];
                }
                cout << (GameConfig::noColor ? "" : "\033[0m") << " " << rpText["out"];
                break;
            }
            case 7:
                cout << rpText["race"];
                break;
        }  // switch
        cout << (GameConfig::noColor ? "" : "\033[0m") << " | ";
        
        // Ů����ؾ���        
        if (policy.useVenusPolicy > 0.6) {
            cout << (GameConfig::noColor ? "" : "\033[32m") << " Ҫʹ��";
        } else if (policy.useVenusPolicy > 0.4) {
            cout << (GameConfig::noColor ? "" : "\033[33m") << " ����ʹ��";
        } else if (policy.useVenusPolicy == 0) {
            cout << (GameConfig::noColor ? "" : "\033[0m") << " ��";
        } else {
            cout << (GameConfig::noColor ? "" : "\033[31m") << " ��ʹ��";
        }
        cout << (GameConfig::noColor ? "" : "\033[0m") << "Ů��Buff";
        cout << " |  ������ѡһ��";
        std::size_t godChoice = findMaxIndex(policy.threeChoicesEventPolicy);
        switch (godChoice) {
        case 0:
            cout << (GameConfig::noColor ? "" : "\033[41m") << "1-��";
            break;
        case 1:
            cout << (GameConfig::noColor ? "" : "\033[44m") << "2-��";
            break;
        case 2:
            cout << (GameConfig::noColor ? "" : "\033[43m") << "3-��";
            break;
        }
        cout << (GameConfig::noColor ? "" : "\033[0m") << endl;
      } // if isRacing
      else
      {
          cout << rpText["career"] << endl;
      }
    } // ������Block

  } // while

}