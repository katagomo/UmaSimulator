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
        cout << "当前RP角色：" << rpText["name"] << endl;
    }
    catch (...)
    {
        cout << "读取配置信息出错：roleplay.json" << endl;
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
  //const double radicalFactor = 5;//激进度
  //const int threadNum = 16; //线程数
 // const int searchN = 12288; //每个选项的蒙特卡洛模拟的局数

  //激进度为k，模拟n局时，标准差约为sqrt(1+k^2/(2k+1))*1200/(sqrt(n))
  //标准差大于30时会严重影响判断准确度

  Search search;
  vector<Evaluator> evaluators;

  int lastTurn = -1;
  int scoreFirstTurn = 0;   // 第一回合分数
  int scoreLastTurn = 0;   // 上一回合分数

  // 检查工作目录
  wchar_t buf[10240];
  GetModuleFileNameW(0, buf, 10240);
  filesystem::path exeDir = filesystem::path(buf).parent_path();
  filesystem::current_path(exeDir);
  //std::cout << "当前工作目录：" << filesystem::current_path() << endl;
  cout << "当前程序目录：" << exeDir << endl;
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
      std::cout << "找不到" + currentGameStagePath + "，可能是游戏未开始或小黑板未正常工作" << endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));//延迟几秒，避免刷屏
    }
    ifstream fs(currentGameStagePath);
    if (!fs.good())
    {
      cout << "读取文件错误" << endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));//延迟几秒，避免刷屏
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
      cout << "出现错误" << endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));//延迟几秒，避免刷屏
      continue;
    }
    if (game.turn == lastTurn)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(300));//检查是否有更新
      continue;
    }
    lastTurn = game.turn;
    if (game.venusIsWisdomActive)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      continue;
    }

    if (game.turn == 0)//第一回合，或者重启ai的第一回合
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
    
    //最后几回合降低激进度
    double modifiedRadicalFactor = GameConfig::radicalFactor * (1 - exp(-double(TOTAL_TURN - game.turn) / 10.0));
    
    search.runSearch(game, evaluators.data(), GameConfig::searchN, TOTAL_TURN, 0, GameConfig::threadNum, modifiedRadicalFactor);
    //cout << endl << rpText["finish"] << endl;
    cout << endl << rpText["analyze"] << " >>" << endl;
    {
      auto policy = search.extractPolicyFromSearchResults(1);
      if (game.venusAvailableWisdom != 0)
      {
        cout << "使用女神率：";
        printPolicy(policy.useVenusPolicy);
        cout << endl;
      }
      if (!game.isRacing)
      {
        if (game.venusAvailableWisdom != 0)
        {
          cout << "在" << (policy.useVenusPolicy > 0.5 ? "" : " 不 ") << "使用女神的前提下：";
        }

        cout << "速耐力根智：";
        for (int i = 0; i < 5; i++)
          printPolicy(policy.trainingPolicy[i]);
        cout << endl;

        cout << "休息，外出，比赛：";
        for (int i = 0; i < 3; i++)
          printPolicy(policy.trainingPolicy[5 + i]);
        cout << endl;

        cout << "女神三选一事件：红，蓝，黄：";
        for (int i = 0; i < 3; i++)
          printPolicy(policy.threeChoicesEventPolicy[i]);
        cout << endl;

        cout << "五个女神外出以及普通外出：";
        for (int i = 0; i < 6; i++)
          printPolicy(policy.outgoingPolicy[i] * policy.trainingPolicy[6]);
        cout << endl << "<<" << endl;

        // 输出运气分
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
            cout << rpText["luck"] << " | 本局：";
            print_luck(maxScore - scoreFirstTurn);
            cout << " | 本回合：" << maxScore - scoreLastTurn
                 << " | 评分预测: " << maxScore << endl;

            double raceLoss = maxScore - max(search.allChoicesValue[0][7].avgScoreMinusTarget, search.allChoicesValue[1][7].avgScoreMinusTarget);
            if (raceLoss < 5e5)//raceLoss大约1e6如果不能比赛
                cout << "比赛亏损（用于选择比赛回合，以完成粉丝数目标）：" << raceLoss << endl;
            cout << "----" << endl;
            cout.flush();
        }
        scoreLastTurn = maxScore;

        // 输出本回合决策
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
                cout << "和 ";
                std::size_t outgoingPolicy = findMaxIndex(policy.outgoingPolicy);
                switch (outgoingPolicy) {
                case 0:
                    cout << (GameConfig::noColor ? "" : "\033[31m") << "红女神";
                    break;
                case 1:
                    cout << (GameConfig::noColor ? "" : "\033[36m") << "蓝女神";
                    break;
                case 2:
                    cout << (GameConfig::noColor ? "" : "\033[33m") << "黄女神";
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
        
        // 女神相关决策        
        if (policy.useVenusPolicy > 0.6) {
            cout << (GameConfig::noColor ? "" : "\033[32m") << " 要使用";
        } else if (policy.useVenusPolicy > 0.4) {
            cout << (GameConfig::noColor ? "" : "\033[33m") << " 可以使用";
        } else if (policy.useVenusPolicy == 0) {
            cout << (GameConfig::noColor ? "" : "\033[0m") << " 无";
        } else {
            cout << (GameConfig::noColor ? "" : "\033[31m") << " 不使用";
        }
        cout << (GameConfig::noColor ? "" : "\033[0m") << "女神Buff";
        cout << " |  神团三选一：";
        std::size_t godChoice = findMaxIndex(policy.threeChoicesEventPolicy);
        switch (godChoice) {
        case 0:
            cout << (GameConfig::noColor ? "" : "\033[41m") << "1-红";
            break;
        case 1:
            cout << (GameConfig::noColor ? "" : "\033[44m") << "2-蓝";
            break;
        case 2:
            cout << (GameConfig::noColor ? "" : "\033[43m") << "3-黄";
            break;
        }
        cout << (GameConfig::noColor ? "" : "\033[0m") << endl;
      } // if isRacing
      else
      {
          cout << rpText["career"] << endl;
      }
    } // 输出结果Block

  } // while

}