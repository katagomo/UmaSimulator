#include <random>
#include <cassert>
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>
#include <future>
#include "Search.h"
#include "../GameDatabase/GameConfig.h"
#include <iostream>
using namespace std;

static void softmax(float* f, int n)
{
  float max = -1e30;
  for (int i = 0; i < n; i++)
    if (f[i] > max)max = f[i];

  float total = 0;
  for (int i = 0; i < n; i++)
  {
    f[i] = exp(f[i] - max);
    total += f[i];
  }

  float totalInv = 1 / total;
  for (int i = 0; i < n; i++)
    f[i] *= totalInv;
}

void Search::runSearch(const Game& game, Evaluator* evaluators,
  int eachSamplingNum, int maxDepth, int targetScore, int threadNum, double radicalFactor)
{
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 8 + 4 + 6; j++)
    {
      allChoicesValue[i][j].winrate = -1;
      allChoicesValue[i][j].avgScoreMinusTarget = -1e6;
    }

  std::random_device rd;
  auto rand = std::mt19937_64(rd());
  //gamesBuf.assign(eachSamplingNum,rootGame);
  int activateVenusChoices = game.venusAvailableWisdom > 0 ? 2 : 1;
  for (int useVenus = 0; useVenus < activateVenusChoices; useVenus++)
  {
    //�ȿ����ǲ���ֻ�б���
    if (game.isRacing)
    {
      allChoicesValue[useVenus][0] = evaluateSingleAction(game, evaluators, eachSamplingNum, maxDepth, targetScore, threadNum, radicalFactor,
        rand, -1, useVenus, -1, -1, -1);
    }
    else
    {
      //���ѵ��
     if (GameConfig::debugPrint && useVenus)
       cout << endl << "- ����Ů��Buff��" << endl;

      //��Ӧλ��01234��Ů����ѡһ�¼���Ӧ8 9 10 11���������죬�����ƣ�����Ϣ5�����6��12~17������7
      for (int item = 0; item < 5; item++)
      {
        assert(GameDatabase::AllCards[game.cardId[0]].cardType == 5 && "���ſ����ڵ�һ��λ��");
        if (game.cardDistribution[item][0])//�����������Ҫ������ѡһ�¼�
        {
          double threeChoicesProb = game.getThreeChoicesEventProb(useVenus);
          //Ů����������
          double bestWinrateIfThreeChoices = -1;//��ѡһ������õ��������
          double bestScoreIfThreeChoices = -1e6;
          if (threeChoicesProb > 0.0)
          {
            for (int chosenSpiritColor = 0; chosenSpiritColor < 3; chosenSpiritColor++)
            {
              auto value = evaluateSingleAction(
                game, evaluators, eachSamplingNum, maxDepth, targetScore, threadNum, radicalFactor,
                rand, item, useVenus, chosenSpiritColor, -1, 1);
              allChoicesValue[useVenus][8 + 1 + chosenSpiritColor] = value;
              if (value.avgScoreMinusTarget > bestScoreIfThreeChoices)
                bestScoreIfThreeChoices = value.avgScoreMinusTarget;
              if (value.winrate > bestWinrateIfThreeChoices)
                bestWinrateIfThreeChoices = value.winrate;
            }
          }
          else
          {
            for (int chosenSpiritColor = 0; chosenSpiritColor < 3; chosenSpiritColor++)
            {
              allChoicesValue[useVenus][8 + 1 + chosenSpiritColor].avgScoreMinusTarget = -1e6;
              allChoicesValue[useVenus][8 + 1 + chosenSpiritColor].winrate = 0.0;
            }
          }

          //Ů�񲻳��������
          if (threeChoicesProb < 1.0)
          {
            allChoicesValue[useVenus][8] = evaluateSingleAction(
              game, evaluators, eachSamplingNum, maxDepth, targetScore, threadNum, radicalFactor, 
              rand, item, useVenus, -1, -1,  -1);
          }
          else
          {
            allChoicesValue[useVenus][8].avgScoreMinusTarget = -1e6;
            allChoicesValue[useVenus][8].winrate = 0.0;
          }

          //�����Ͳ�����ȡƽ��
          allChoicesValue[useVenus][item].avgScoreMinusTarget = allChoicesValue[useVenus][8].avgScoreMinusTarget * (1 - threeChoicesProb) + bestScoreIfThreeChoices * threeChoicesProb;
          allChoicesValue[useVenus][item].winrate = allChoicesValue[useVenus][8].winrate * (1 - threeChoicesProb) + bestWinrateIfThreeChoices * threeChoicesProb;

        }
        else//Ů�������ѵ��
        {
          allChoicesValue[useVenus][item] = evaluateSingleAction(
            game, evaluators, eachSamplingNum, maxDepth, targetScore, threadNum, radicalFactor,
            rand, item, useVenus, -1, -1, -1);
        }
      }

      //��Ϣ
      //cout << endl << "- ���ڷ�����Ϣ";
      allChoicesValue[useVenus][5] = evaluateSingleAction(
        game, evaluators, eachSamplingNum, maxDepth, targetScore, threadNum, radicalFactor,
        rand, 5, useVenus, -1, -1, -1);

      //����
      //cout << endl << "- ���ڷ�������";
      if (game.turn > 12 && game.turn < 72)
      {
        allChoicesValue[useVenus][7] = evaluateSingleAction(
          game, evaluators, eachSamplingNum, maxDepth, targetScore, threadNum, radicalFactor,
          rand, 7, useVenus, -1, -1, -1);
      }

      //���
      //cout << endl << "- ���ڷ�������";
      if(!game.isXiaHeSu())
      {
        double bestWinrate = -1;//��ѡһ������õ��������
        double bestScore = -1e6;
        for (int chosenOutgoing= 0; chosenOutgoing < 6; chosenOutgoing++)
        {
          if (!game.isOutgoingLegal(chosenOutgoing))
            continue;
          auto value = evaluateSingleAction(
            game, evaluators, eachSamplingNum, maxDepth, targetScore, threadNum, radicalFactor,
            rand, 6, useVenus, -1, chosenOutgoing, -1);
          allChoicesValue[useVenus][8 + 4 + chosenOutgoing] = value;
          if (value.avgScoreMinusTarget > bestScore)
            bestScore = value.avgScoreMinusTarget;
          if (value.winrate > bestWinrate)
            bestWinrate = value.winrate;
        }
        allChoicesValue[useVenus][6].avgScoreMinusTarget = bestScore;
        allChoicesValue[useVenus][6].winrate = bestWinrate;
      }

    }
  }
}

static double getWeightedAvg(const ModelOutputValueV1* allResults, int n, double p)
{
  vector<double> allScores(n);
  for (int i = 0; i < n; i++)
    allScores[i] = allResults[i].avgScoreMinusTarget;
  sort(allScores.begin(), allScores.end());//��С����

  double weightSum = 0;
  double weightedScoreSum = 0;
  for (int i = 0; i < n; i++)
  {
    double w = (i + 0.5) / n;
    w = pow(w, p);
    weightSum += w;
    weightedScoreSum += w * allScores[i];
  }
  return weightedScoreSum / weightSum;
}

//�������н��
static void evaluateSingleActionStoreAll(ModelOutputValueV1* allResults, const Game& game, Evaluator* evaluators, int eachSamplingNum, int maxDepth, int targetScore, int threadNum,
  std::mt19937_64& rand, int chosenTrain, bool useVenus, int chosenSpiritColor, int chosenOutgoing, int forceThreeChoicesEvent)
{
  if (threadNum == 1)
  {
    int batchsize = evaluators->maxBatchsize;
    int batchNum = (eachSamplingNum - 1) / evaluators->maxBatchsize + 1;
    eachSamplingNum = batchNum * evaluators->maxBatchsize;//�չ�������batchsize

    std::vector<float> targetScores;
    targetScores.assign(batchsize, targetScore);

    Game gameCopy = game;

    gameCopy.playerPrint = false;
    std::vector<Game> gamesBuf;

    for (int batch = 0; batch < batchNum; batch++)
    {
      gamesBuf.assign(batchsize, gameCopy);

      //���ߵ�һ��
      for (int i = 0; i < batchsize; i++)
      {
        gamesBuf[i].applyTrainingAndNextTurn(rand, chosenTrain, useVenus, chosenSpiritColor, chosenOutgoing, forceThreeChoicesEvent);
      }

      for (int depth = 0; depth < maxDepth; depth++)
      {
        evaluators->evaluate(gamesBuf.data(), targetScores.data(), 1, batchsize);//����policy
        bool distributeCards = (depth != maxDepth - 1);//���һ��Ͳ����俨���ˣ�ֱ�ӵ����������ֵ


        bool allFinished = true;
        for (int i = 0; i < batchsize; i++)
        {
          Search::runOneTurnUsingPolicy(rand, gamesBuf[i], evaluators->policyResults[i], distributeCards);
          if (!gamesBuf[i].isEnd())allFinished = false;
        }
        if (allFinished)break;
      }
      evaluators->evaluate(gamesBuf.data(), targetScores.data(), 0, batchsize);//����value
      for (int i = 0; i < batchsize; i++)
      {
        allResults[batch * batchsize + i].avgScoreMinusTarget = evaluators->valueResults[i].avgScoreMinusTarget;
        allResults[batch * batchsize + i].winrate = evaluators->valueResults[i].winrate;
      }

    }
  }
  else
  {
    int eachSamplingNumEveryThread = eachSamplingNum / threadNum;
    if (eachSamplingNumEveryThread <= 0)eachSamplingNumEveryThread = 1;

    int batchNumEveryThread = (eachSamplingNumEveryThread - 1) / evaluators->maxBatchsize + 1;
    eachSamplingNumEveryThread = batchNumEveryThread * evaluators->maxBatchsize;//ÿ���̶߳��չ�������batchsize
    eachSamplingNum = eachSamplingNumEveryThread * threadNum;


    std::vector<std::mt19937_64> rands;
    for (int i = 0; i < threadNum; i++)
      rands.push_back(std::mt19937_64(rand()));



    std::vector<std::thread> threads;
    for (int i = 0; i < threadNum; ++i) {
      threads.push_back(std::thread(

        [i, &allResults, &game, &evaluators, eachSamplingNumEveryThread, maxDepth, targetScore, &rands, chosenTrain, useVenus, chosenSpiritColor, chosenOutgoing, forceThreeChoicesEvent]() {

          evaluateSingleActionStoreAll(allResults + i * eachSamplingNumEveryThread, game, evaluators + i, eachSamplingNumEveryThread, maxDepth, targetScore, 1,
          rands[i],
          chosenTrain, useVenus, chosenSpiritColor, chosenOutgoing, forceThreeChoicesEvent);
        })
      );
        
        
    }
    for (auto& thread : threads) {
      thread.join();
    }


  }
}

ModelOutputValueV1 Search::evaluateSingleAction(const Game& game, Evaluator* evaluators,
  int eachSamplingNum, int maxDepth, int targetScore,
  int threadNum, double radicalFactor,


  std::mt19937_64& rand,
  int chosenTrain,
  bool useVenus,
  int chosenSpiritColor,
  int chosenOutgoing,
  int forceThreeChoicesEvent)
{
    if (GameConfig::debugPrint) {
        cout << "."; cout.flush();
    }
  int eachSamplingNumEveryThread = eachSamplingNum / threadNum;
  if (eachSamplingNumEveryThread <= 0)eachSamplingNumEveryThread = 1;

  int batchNumEveryThread = (eachSamplingNumEveryThread - 1) / evaluators->maxBatchsize + 1;
  eachSamplingNumEveryThread = batchNumEveryThread * evaluators->maxBatchsize;//ÿ���̶߳��չ�������batchsize
  eachSamplingNum = eachSamplingNumEveryThread * threadNum;



  vector< ModelOutputValueV1> allResults(eachSamplingNum);
  evaluateSingleActionStoreAll
  (
    allResults.data(),
    game, evaluators, eachSamplingNum, maxDepth, targetScore, threadNum,
    rand,
    chosenTrain, useVenus, chosenSpiritColor, chosenOutgoing, forceThreeChoicesEvent);

  double score = getWeightedAvg(allResults.data(), eachSamplingNum, radicalFactor);

  ModelOutputValueV1 r;
  r.avgScoreMinusTarget = score;
  r.winrate = 0.5;
  return r;
}

ModelOutputPolicyV1 Search::extractPolicyFromSearchResults(int mode, float delta)
{
  ModelOutputPolicyV1 policy;
  if (delta == 0)
  {
    if (mode == 0)delta = 0.02;
    else delta = 30;
  }
  float deltaInv = 1 / delta;

  bool useVenus;//�ȿ����ò���Ů��
  {
    float bestValueNotUseVenus = -1e30;
    for (int i = 0; i < 8; i++)
    {
      if (allChoicesValue[0][i].extract(mode) > bestValueNotUseVenus)bestValueNotUseVenus = allChoicesValue[0][i].extract(mode);
    }
    float bestValueUseVenus = -1e30;
    for (int i = 0; i < 8; i++)
    {
      if (allChoicesValue[1][i].extract(mode) > bestValueUseVenus)bestValueUseVenus = allChoicesValue[1][i].extract(mode);
    }
    useVenus = bestValueUseVenus > bestValueNotUseVenus;

    float useVenusPolicy[2] = { bestValueUseVenus * deltaInv,bestValueNotUseVenus * deltaInv };
    softmax(useVenusPolicy, 2);
    policy.useVenusPolicy = useVenusPolicy[0];
  }

  //ѵ��8ѡ1
  for (int i = 0; i < 8; i++)
    policy.trainingPolicy[i] = deltaInv * allChoicesValue[int(useVenus)][i].extract(mode);
  softmax(policy.trainingPolicy, 8);

  //��ѡһ�¼�
  for (int i = 0; i < 3; i++)
    policy.threeChoicesEventPolicy[i] = deltaInv * allChoicesValue[int(useVenus)][8 + 1 + i].extract(mode);
  softmax(policy.threeChoicesEventPolicy, 3);

  //ѡ���
  for (int i = 0; i < 6; i++)
    policy.outgoingPolicy[i] = deltaInv * allChoicesValue[int(useVenus)][8 + 4 + i].extract(mode);
  softmax(policy.outgoingPolicy, 6);

  return policy;
}

void Search::runOneTurnUsingPolicy(std::mt19937_64& rand, Game& game, const ModelOutputPolicyV1& policy, bool distributeCards)
{
  if (game.isEnd())return;
  bool useVenus = false;
  int chosenSpiritColor = -1;
  int chosenTrain = -1;
  int chosenOutgoing = -1;

  //auto policy = Evaluator::handWrittenPolicy(game);
  {
    float bestPolicy = 0.001;
    for (int i = 0; i < 8; i++)
    {
      float p = policy.trainingPolicy[i];
      if (p > bestPolicy)
      {
        chosenTrain = i;
        bestPolicy = p;
      }
    }
  }
  useVenus = policy.useVenusPolicy >= 0.5;
  {
    float bestPolicy = 0.001;
    for (int i = 0; i < 3; i++)
    {
      float p = policy.threeChoicesEventPolicy[i];
      if (p > bestPolicy)
      {
        chosenSpiritColor = i;
        bestPolicy = p;
      }
    }
  }
  {
    float bestPolicy = 0.001;
    for (int i = 0; i < 6; i++)
    {
      float p = policy.outgoingPolicy[i];
      if (p > bestPolicy)
      {
        chosenOutgoing = i;
        bestPolicy = p;
      }
    }
  }
  if(distributeCards)
    game.applyTrainingAndNextTurn(rand, chosenTrain, useVenus, chosenSpiritColor, chosenOutgoing);
  else
    game.applyTraining(rand, chosenTrain, useVenus, chosenSpiritColor, chosenOutgoing);

}
