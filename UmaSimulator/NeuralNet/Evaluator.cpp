#include <cassert>
#include <iostream>
#include "Evaluator.h"

void Evaluator::evaluate(const Game* games, const float* targetScores, int mode, int gameNum)
{
  assert(gameNum <= maxBatchsize);
  if (model == NULL)//û�����磬��д�߼�
  {
    if (mode == 0)//value�������վֲſɼ���
    {
      for (int i = 0; i < gameNum; i++)
      {
        const Game& game = games[i];
        assert(game.isEnd() && "��������ʱ��ֻ����Ϸ������ſɼ���value");
        int score = game.finalScore();
        if (score >= targetScores[i])
          valueResults[i].winrate = 1.0;
        else valueResults[i].winrate = 0.0;
        valueResults[i].avgScoreMinusTarget = score - targetScores[i];
      }
    }
    else if (mode == 1)//policy����д�߼������ŵ�ѡ����1����������0
    {
      for (int i = 0; i < gameNum; i++)
      {
        policyResults[i] = handWrittenPolicy(games[i]);
      }
    }
  }
  else
  {
    assert(false && "��ûд");
  }
}

Evaluator::Evaluator(Model* model, int maxBatchsize):model(model), maxBatchsize(maxBatchsize)
{
  inputBuf.resize(NNINPUT_CHANNELS_V1 * maxBatchsize);
  outputBuf.resize(NNOUTPUT_CHANNELS_V1 * maxBatchsize);
  valueResults.resize(maxBatchsize);
  policyResults.resize(maxBatchsize);
  
}


static double vitalEvaluation(int vital,int maxVital)
{
  if (vital <= 50)
    return 2.0 * vital;
  else if (vital <= 70)
    return 1.5 * (vital - 50) + vitalEvaluation(50, maxVital);
  else if (vital <= maxVital - 10)
    return 1.0 * (vital - 70) + vitalEvaluation(70, maxVital);
  else
    return 0.5 * (vital - (maxVital - 10)) + vitalEvaluation(maxVital - 10, maxVital);
}

static const int fixedBonusAfterTurn[TOTAL_TURN] = //������غ�֮��ĸ����¼���swbc�ȣ��Ĺ̶���������
{
  166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,
  150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,//wbcǰ
  115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,//swbcǰ������һ����ʿ���
  74,74,74,74,74,64//���6�غ�
};

static const int nearestBigRaceTurn[TOTAL_TURN] = //��������Ĵ�������ж��ٻغ�
{
  23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
  23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
  23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
  5,4,3,2,1,0
};
static const int nearestBigRace[TOTAL_TURN] = //����Ĵ�������ĸ�
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,
};
//��������ϵ��
const float reserveRedFactorGUR = 1;
const float reserveRedFactorWBC = 3;
const float reserveRedFactorSWBC = 6;
const float reserveRedFactorGM = 6; 
static const int reserveRedFactor[TOTAL_TURN] =
{
  reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,reserveRedFactorGUR,
  reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,reserveRedFactorWBC,
  reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,reserveRedFactorSWBC,
  reserveRedFactorGM,reserveRedFactorGM,reserveRedFactorGM,reserveRedFactorGM,reserveRedFactorGM,reserveRedFactorGM
};



const double jibanValue = 2;
const double venusValue_first = 40;
const double venusValue_beforeOutgoing = 10;
const double venusValue_afterOutgoing = 20;
const double venusValue_activated = 6;
const double spiritValue = 25;
const double vitalFactor = 0.7;
const double smallFailValue = -30;
const double bigFailValue = -90;
const double wizFailValue = 5;
const double statusWeights[5] = { 1.0,1.0,1.0,1.0,1.0 };
const double ptWeight = 0.5;
const double restValueFactor = 1.2;//��Ϣ��ֵȨ��
const float remainStatusFactorEachTurn = 16;//������ʱ��ÿ�غ�Ԥ������
const double outgoingBonusIfNotFullMotivation = 30;//������ʱ���Ů���������


ModelOutputPolicyV1 Evaluator::handWrittenPolicy(const Game& game0)
{
  ModelOutputPolicyV1 policy=
  {
    {0,0,0,0,0,0,0,0},
    0,
    {0,0,0,0,0,0},
    {0,0,0}
  };
  if(game0.isEnd())
    return policy;

  int chosenTrain = 0;
  bool useVenus = false;
  int chosenOutgoing = 0;
  int chosenSpiritColor = 0;



  Game game = game0;//����һ����������Ϊ�п���Ҫ��Ů��


  float reserveRedBigRaceBonus = 0;//�쵽������ˣ��Ѻ�����
  if (game.venusAvailableWisdom == 1)
  {

  }

  if (game.isRacing)//�����غ�
  {
    if (game.venusAvailableWisdom != 1)
      useVenus = false;//���Ǻ�϶�����
    else
    {
      int nearestBigRaceTurnNum = nearestBigRaceTurn[game.turn];
      if (nearestBigRaceTurnNum == 0)
        useVenus = true;
      else
      {
        int nearestBigRaceId=nearestBigRace[game.turn];
        if((nearestBigRaceId==0&&nearestBigRaceTurnNum<2)
          || (nearestBigRaceId == 1 && nearestBigRaceTurnNum < 3)
          || (nearestBigRaceId == 2 && nearestBigRaceTurnNum < 5)
          || (nearestBigRaceId == 3 && nearestBigRaceTurnNum < 5)
          )
          useVenus = false;
        else
          useVenus = true;
      }
    }
  }
  else//����ѵ���غ�
  {
    if (game.venusAvailableWisdom != 0)
    {
      if (game.venusAvailableWisdom != 1)
      {
        useVenus = true;//���Ǻ죬������Ϣ�϶�Ҫ���������Ϣ������Ĵ�����useVenus���false
      }
      else
      {
        //�쵽����������
        int nearestBigRaceTurnNum = nearestBigRaceTurn[game.turn];
        if (nearestBigRaceTurnNum == 0)
          useVenus = true;
        else if (game.vital<50 || game.motivation<5)
          useVenus = true;
        else
        {
          int nearestBigRaceId = nearestBigRace[game.turn];
          if ((nearestBigRaceId == 0 && nearestBigRaceTurnNum < 2)
            || (nearestBigRaceId == 1 && nearestBigRaceTurnNum < 3)
            || (nearestBigRaceId == 2 && nearestBigRaceTurnNum < 5)
            || (nearestBigRaceId == 3 && nearestBigRaceTurnNum < 5)
            )
            useVenus = false;
          else
            useVenus = true;
        }

      }
    }
    
    if(useVenus)
      game.activateVenusWisdom();


    int vitalAfterRest = std::min(game.maxVital, 50 + game.vital);
    double restValue = restValueFactor * (vitalEvaluation(vitalAfterRest, game.maxVital) - vitalEvaluation(game.vital, game.maxVital));
    //std::cout << restValue << " "<<game.vital<<std::endl;
    if (game.venusSpiritsBottom[7] == 0)restValue += spiritValue;

    double bestValue = -10000;
    int bestTrain = -1;
    for (int item = 0; item < 5; item++)
    {

      double expectSpiritNum = int(game.spiritDistribution[item] / 32) + 1;
      double value = 0;
      assert(game.cardData[0]->cardType == 5 && "���ſ����ڵ�һ��λ��");

      int cardHintNum = 0;//����hint���ȡһ�������Դ�ֵ�ʱ��ȡƽ��
      for (int head = 0; head < 6; head++)
      {
        if (game.cardDistribution[item][head] && game.cardHint[head])
          cardHintNum++;
      }
      if (game.venusIsWisdomActive && game.venusAvailableWisdom == 2)//��������hint��Ч
        cardHintNum = 1;


      for (int head = 0; head < 6; head++)
      {
        if (!game.cardDistribution[item][head])
          continue;
        if (head == 0)
        {
          if (!game.venusCardFirstClick)
            value += venusValue_first;
          else if (!game.venusCardUnlockOutgoing)
          {
            expectSpiritNum += 0.5;
            value += venusValue_beforeOutgoing;
          }
          else if (!game.venusCardIsQingRe)
          {
            expectSpiritNum += 0.5;
            value += venusValue_afterOutgoing;
          }
          else
          {
            expectSpiritNum += 1;
            value += venusValue_activated;
          }


          //ѡ�ȼ���͵�������ɫ����Ƭ
          if (game.venusLevelRed <= game.venusLevelBlue && game.venusLevelRed <= game.venusLevelYellow)
            chosenSpiritColor = 0;
          else if (game.venusLevelBlue <= game.venusLevelYellow && game.venusLevelBlue < game.venusLevelRed)
            chosenSpiritColor = 1;
          else
            chosenSpiritColor = 2;
          //�������˱���ѡ��
          if (game.motivation < 5)
            chosenSpiritColor = 0;

        }
        else
        {
          if (game.cardJiBan[head] < 80)
          {
            float jibanAdd = 7;
            if (game.isAiJiao)jibanAdd += 2;
            if (game.cardHint[head])
            {
              jibanAdd += 5 / cardHintNum;
              if (game.isAiJiao)jibanAdd += 2 / cardHintNum;
            }
            jibanAdd = std::min(float(80 - game.cardJiBan[head]), jibanAdd);

            value += jibanAdd * jibanValue;
          }
        }
        if (game.cardHint[head])
        {
          for (int i = 0; i < 5; i++)
            value += game.cardData[head]->hintBonus[i] * statusWeights[i] / cardHintNum;
          value += game.cardData[head]->hintBonus[5] * ptWeight / cardHintNum;
        }

      }
      //����
      if (game.venusAvailableWisdom == 2 && game.venusIsWisdomActive)
      {
        auto blueBonus = game.calculateBlueVenusBonus(item);
        for (int i = 0; i < 5; i++)
          value += blueBonus[i] * statusWeights[i];
        value += blueBonus[5] * ptWeight;
      }


      if (game.venusSpiritsBottom[7] > 0)
        expectSpiritNum = 0;
      else if (game.venusSpiritsBottom[6] > 0)
        expectSpiritNum = std::min(1.0, expectSpiritNum);
      else if (game.venusSpiritsBottom[5] > 0)
        expectSpiritNum = std::min(2.0, expectSpiritNum);
      value += spiritValue * expectSpiritNum;

      for (int i = 0; i < 5; i++)
      {
        //����Ҫ�������ԼӶ��٣���Ҫ�����Ƿ����
        float gain = game.trainValue[item][i];
          float remain = game.fiveStatusLimit[i] - game.fiveStatus[i] - fixedBonusAfterTurn[game.turn];
          if (gain > remain)gain = remain;
          float turnReserve = remainStatusFactorEachTurn * (TOTAL_TURN - game.turn);//�������޲���turnReserveʱ����Ȩ��

          float remainAfterTrain = remain - gain;

          if (remainAfterTrain < turnReserve)//��remain-turnReserve��remain�𽥽���Ȩ��
          {
            if (remain < turnReserve)//ѵ��ǰ�ͽ�����turnReserve����
            {
              gain = 0.5 * (remain * remain - remainAfterTrain * remainAfterTrain) / turnReserve;
            }
            else
            {
              gain = (remain - turnReserve) + 0.5 * turnReserve - 0.5 * remainAfterTrain * remainAfterTrain / turnReserve;
            }
          }

          
        value += gain * statusWeights[i];
      }
      double ptWeightThisGame = ptWeight / 1.8 * (game.isQieZhe ? GameConstants::ScorePtRateQieZhe : GameConstants::ScorePtRate);
      value += game.trainValue[item][5] * ptWeightThisGame;
      //value += vitalValue * game.trainValue[item][6];

      int vitalAfterTrain = std::min(game.maxVital, game.trainValue[item][6] + game.vital);
      value += vitalFactor * (vitalEvaluation(vitalAfterTrain, game.maxVital) - vitalEvaluation(game.vital, game.maxVital));
        

      double failRate = game.failRate[item];
      if (failRate > 0)
      {
        double failValueAvg = wizFailValue;
        if (item != 5)
        {
          double bigFailProb = failRate;
          if (failRate < 20)bigFailProb = 0;
          failValueAvg = 0.01 * bigFailProb * bigFailValue + (1 - 0.01 * bigFailProb) * smallFailValue;
        }
        value = 0.01 * failRate * failValueAvg + (1 - 0.01 * failRate) * value;
      }


      if (value > bestValue)
      {
        bestValue = value;
        bestTrain = item;
      }
    }
    chosenTrain = bestTrain;

    if (game.motivation < 5)//�������ˣ��ý������
    {
      if (game.venusCardUnlockOutgoing && (!game.venusCardOutgoingUsed[4]))//��Ů�����
        restValue += outgoingBonusIfNotFullMotivation;
    }

    if (restValue>bestValue)//��Ů������������û�о���Ϣ
    {
      useVenus = false;//������ʲôŮ�񣬶���ֵ����
      if (game.venusCardUnlockOutgoing && !game.venusCardOutgoingUsed[4] && !game.isXiaHeSu())
      {
        chosenTrain = 6;
        chosenOutgoing =
          (!game.venusCardOutgoingUsed[2]) ? 2 :
          (!game.venusCardOutgoingUsed[0]) ? 0 :
          (!game.venusCardOutgoingUsed[1]) ? 1 :
          (!game.venusCardOutgoingUsed[3]) ? 3 :
          4;
      }
      else
        chosenTrain = 5;
    }
    
  }
  policy.trainingPolicy[chosenTrain] = 1.0;
  if (useVenus)policy.useVenusPolicy = 1.0;
  policy.threeChoicesEventPolicy[chosenSpiritColor] = 1.0;
  policy.outgoingPolicy[chosenOutgoing] = 1.0;
  return policy;
}
