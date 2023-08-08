//����ѵ������ֵ�㷨
#include <iostream>
#include <random>
#include <sstream>
#include <cassert>
#include <thread>
#include <atomic>
#include "../Game/Game.h"
#include "../NeuralNet/Evaluator.h"
#include "../Search/Search.h"
#include "../External/termcolor.hpp"

#include "../GameDatabase/GameDatabase.h"
#include "../GameDatabase/GameConfig.h"

using namespace std;

const bool handWrittenEvaluationTest = true;
const int threadNum = 16;
const int threadNumInner = 1;
const double radicalFactor = 5;//������
const int searchN = handWrittenEvaluationTest ? 1 : 3072;


const int totalGames = handWrittenEvaluationTest ? 1200000 : 10000000;
const int gamesEveryThread = totalGames / threadNum;



int umaId = 101101;//���Ϸ�
int cards[6] = { 301374,301344,300104,300194,300114,301074 };//���ţ��߷壬���������������˾��
//��д�߼�Ӧ��Ϊ27699��5��1000000�֣�


std::atomic<double> totalScore = 0;
std::atomic<double> totalScoreSqr = 0;

std::atomic<int> bestScore = 0;
std::atomic<int> n = 0;
vector<atomic<int>> segmentStats= vector<atomic<int>>(500);//100��һ�Σ�500��


void worker()
{
  random_device rd;
  auto rand = mt19937_64(rd());

/*
  for (int i = 0; i < 6; ++i) {

      GameDatabase::AllCards[cards[i]].cardValueInit(4);

  }
*/
  //int umaId = 5;//��֮��
  //int cards[6] = { 1,2,14,10,11,15 };
  // 
  //int umaId = 4;
  //int cards[6] = { 1,2,14,4,5,31 };

  int zhongmaBlue[5] = { 18,0,0,0,0 };
  int zhongmaBonus[6] = { 20,0,40,0,20,200 };

  Search search;
  vector<Evaluator> evaluators;
  for (int i = 0; i < threadNumInner; i++)
    evaluators.push_back(Evaluator(NULL, 128));

  for (int gamenum = 0; gamenum < gamesEveryThread; gamenum++)
  {

    Game game;
    game.newGame(rand, false, umaId, cards, zhongmaBlue, zhongmaBonus);

    while(!game.isEnd())
    {
      ModelOutputPolicyV1 policy;
      if (handWrittenEvaluationTest) {
        policy = Evaluator::handWrittenPolicy(game);
      }
      else {
        search.runSearch(game, evaluators.data(), searchN, TOTAL_TURN, 27000, threadNumInner, radicalFactor);
        policy = search.extractPolicyFromSearchResults(1);
      }
      Search::runOneTurnUsingPolicy(rand, game, policy, true);
    }
    //cout << termcolor::red << "���ɽ�����" << termcolor::reset << endl;
    int score = game.finalScore();
    n += 1;
    totalScore += score;
    totalScoreSqr += score * score;
    for (int i = 0; i < 500; i++)
    {
      int refScore = i * 100;
      if (score >= refScore)
      {
        segmentStats[i] += 1;
      }
    }

    int bestScoreOld = bestScore;
    while (score > bestScoreOld && !bestScore.compare_exchange_weak(bestScoreOld, score)) {
      // ���val����old_max������max_val��ֵ����old_max����ô�ͽ�max_val��ֵ����Ϊval
      // ���max_val��ֵ�Ѿ��������̸߳��£���ô�Ͳ����κ����飬����old_max�ᱻ����Ϊmax_val����ֵ
      // Ȼ�������ٴν��бȽϺͽ���������ֱ���ɹ�Ϊֹ
    }


    //game.print();
    if (!handWrittenEvaluationTest)
    {
      game.printFinalStats();
      cout << n << "�֣�������=" << searchN << "��ƽ����" << totalScore / n << "����׼��" << sqrt(totalScoreSqr / n - totalScore * totalScore / n / n) << "����߷�" << bestScore << endl;
      cout
        << "29500�ָ���=" << float(segmentStats[295]) / n << ","
        << "30000�ָ���=" << float(segmentStats[300]) / n << ","
        << "30500�ָ���=" << float(segmentStats[305]) / n << ","
        << "31000�ָ���=" << float(segmentStats[310]) / n << ","
        << "31500�ָ���=" << float(segmentStats[315]) / n << ","
        << "32000�ָ���=" << float(segmentStats[320]) / n << ","
        << "32500�ָ���=" << float(segmentStats[325]) / n << endl;
    }
  }

}

void main_test5()
{


    // ��鹤��Ŀ¼
    GameDatabase::loadUmas("../db/uma");
    GameDatabase::loadCards("../db/card");


  for (int i = 0; i < 200; i++)segmentStats[i] = 0;


  std::vector<std::thread> threads;
  for (int i = 0; i < threadNum; ++i) {
    threads.push_back(std::thread(worker));
  }
  for (auto& thread : threads) {
    thread.join();
  }

  cout << n << "�֣�������=" << searchN << "��ƽ����" << totalScore / n << "����׼��" << sqrt(totalScoreSqr / n - totalScore * totalScore / n / n) << "����߷�" << bestScore << endl;

}