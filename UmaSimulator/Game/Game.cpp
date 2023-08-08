#include <iostream>
#include <cassert>
#include "Game.h"
using namespace std;
static bool randBool(mt19937_64& rand, double p)
{
  return rand() % 65536 < p * 65536;
}

void Game::newGame(mt19937_64& rand, bool enablePlayerPrint, int newUmaId, int newCards[6], int newZhongMaBlueCount[5], int newZhongMaExtraBonus[6])
{
  playerPrint = enablePlayerPrint;
  umaId = newUmaId;
  umaData = &GameDatabase::AllUmas[umaId];
  for (int i = 0; i < 6; i++)
  {
      cardId[i] = newCards[i];
      cardData[i] = &GameDatabase::AllCards[newCards[i]];
  }
  assert(cardData[0]->cardType == 5 && "���ſ����ڵ�һ��λ��");
  for (int i = 0; i < 5; i++)
    zhongMaBlueCount[i] = newZhongMaBlueCount[i];
  for (int i = 0; i < 6; i++)
    zhongMaExtraBonus[i] = newZhongMaExtraBonus[i];

  turn = 0;
  vital = 100;
  maxVital = 100;
  isQieZhe = false;
  isAiJiao = false; 
  failureRateBias = 0;
  skillPt = 120;
  skillPt += 170 * 5 / GameConstants::ScorePtRate;//���м��ܻ����pt


  for (int i = 0; i < 5; i++)
    fiveStatusLimit[i] = GameConstants::BasicFiveStatusLimit[i]; //ԭʼ��������
  for (int i = 0; i < 5; i++)
    fiveStatusLimit[i] += zhongMaBlueCount[i] * 7 * 2; //��������--�������ֵ
  for (int i = 0; i < 5; i++)
    fiveStatusLimit[i] += rand() % 20; //��������--�����μ̳��������


  for (int i = 0; i < 5; i++)
    fiveStatus[i] = umaData->fiveStatusInitial[i]; //�������ʼֵ
    //fiveStatus[i] = GameDatabase::AllUmas[umaId].fiveStatusInitial[i]; //�������ʼֵ
  for (int i = 0; i < 6; i++)//֧Ԯ����ʼ�ӳ�
  {
    for (int j = 0; j < 5; j++)
      addStatus(j, cardData[i]->initialBonus[j]);
    skillPt += cardData[i]->initialBonus[5];
  }
  for (int i = 0; i < 5; i++)
    addStatus(i, zhongMaBlueCount[i] * 7); //����


  motivation = 3;
  for (int i = 0; i < 6; i++)
    cardJiBan[i] = cardData[i]->initialJiBan;
  cardJiBan[6] = 0; 
  cardJiBan[7] = 0;
  for (int i = 0; i < 5; i++)
    trainLevelCount[i] = 0;
  isRacing = false;


  venusLevelYellow = 0;
  venusLevelRed = 0;
  venusLevelBlue = 0;
  for (int i = 0; i < 8; i++)
    venusSpiritsBottom[i] = 0;
  for (int i = 0; i < 6; i++)
    venusSpiritsUpper[i] = 0;
  venusAvailableWisdom = 0;
  venusIsWisdomActive = false;


  venusCardFirstClick = false;
  venusCardUnlockOutgoing = false;
  venusCardIsQingRe = false;
  venusCardQingReContinuousTurns = 0;
  for (int i = 0; i < 5; i++)
    venusCardOutgoingUsed[i] = false;

  initRandomGenerators();

  stageInTurn = 0;
  calculateVenusSpiritsBonus();
  randomDistributeCards(rand); 
}

void Game::initRandomGenerators()
{
  for (int i = 0; i < 8; i++)
  {
    std::vector<int> probs = { 100,100,100,100,100,50 }; //�������ʣ����������Ǹ�
    if (i < 6)
    {
      int cardType = cardData[i]->cardType;
      int deYiLv = cardData[i]->deYiLv;
      if (cardType >= 0 && cardType < 5)//���������ǿ�
        probs[cardType] += deYiLv;
      else //���˿�����ĸ��ʽϸ�
        probs[5] += 50;
    }
    else //���³�������
      probs[5] += 50;

    cardDistributionRandom[i] = std::discrete_distribution<>(probs.begin(), probs.end());
  }

  for (int i = 0; i < 8; i++)
    venusSpiritTypeRandom[i] = std::discrete_distribution<>(GameConstants::VenusSpiritTypeProb[i], GameConstants::VenusSpiritTypeProb[i + 1]);

}

void Game::randomDistributeCards(std::mt19937_64& rand)
{
  //assert(stageInTurn == 0 || turn == 0);
  stageInTurn = 1;
  if (isRacing)
    return;//�������÷��俨�飬��Ҫ��stageInTurn
  //�Ƚ�6�ſ����䵽ѵ����
  for (int i = 0; i < 5; i++)
    for (int j = 0; j < 8; j++)
      cardDistribution[i][j] = false;

  double blueVenusHintBonus = 1 + 0.01 * GameConstants::BlueVenusLevelHintProbBonus[venusLevelBlue];

  for (int i = 0; i < 8; i++)
  {
    if (turn < 2 && i == 0)//ǰ���غ����Ų���
    {
      assert(cardData[0]->cardType == 5 && "���ſ����ڵ�һ��λ��");
      continue;
    }

    int cardType = i < 6 ? cardData[i]->cardType : 6;

    int whichTrain = cardDistributionRandom[i](rand);//���ĸ�ѵ��
    if (whichTrain < 5)//û��
      cardDistribution[whichTrain][i] = true;

    //�Ƿ���hint
    if (i < 6 && cardType >= 0 && cardType < 5)//���������ǿ�
    {
      double hintProb = 0.06 * blueVenusHintBonus * (1 + 0.01 * cardData[i]->hintProbIncrease);
      bernoulli_distribution d(hintProb);
      cardHint[i] = d(rand);
    }
    else if (i < 6)
      cardHint[i] = false;
  }
  //������Ƭ
  if (turn < 2 || venusSpiritsBottom[7] != 0)//����Ƭ
  {
    for (int i = 0; i < 8; i++)
      spiritDistribution[i] = 0;
  }
  else
  {
    bool allowTwoSpirits = venusSpiritsBottom[6] == 0;//��������λ
    for (int i = 0; i < 8; i++)
    {
      int spiritType = venusSpiritTypeRandom[i](rand) + 1;
      int spiritColor = rand() % 3;
      int spirit = spiritType + spiritColor * 8;

      //�����Ƿ�Ϊ˫��Ƭ
      bool twoSpirits = false;
      if (allowTwoSpirits)
      {
        if (i < 5)//���ѵ��
        {
          bool isShining = false;//�Ƿ�����
          for (int card = 0; card < 6; card++)
          {
            if (cardDistribution[i][card])//����������ѵ��
            {
              isShining |= cardData[card]->getCardEffect(*this, i, cardJiBan[card], cardData[card]->effectFactor).youQing > 0;
            }
          }
          if (isShining)
          {
            if (i < 4)twoSpirits = true;
            else if (rand() % 5 < 2)twoSpirits = true;//������Ȧ40%˫��Ƭ
          }
        }
        else
        {
          if (rand() % 5 == 0)twoSpirits = true;//������20%˫��Ƭ
        }
      }
      if (twoSpirits)spirit += 32;//+32����������Ƭ
      spiritDistribution[i] = spirit;
    }

  }

  calculateTrainingValue();

  //�������ѵ������6�����߸�����ͷ�������·��俨��
  bool have6orMoreHeads = false;
  for (int i = 0; i < 5; i++)
  {
    int c = 0;
    for (int j = 0; j < 8; j++)
      if (cardDistribution[i][j])
        c++;
    if (c >= 6)
    {
      have6orMoreHeads = true;
      break;
    }
  }
  if (have6orMoreHeads)
    randomDistributeCards(rand);

}

void Game::calculateTrainingValue()
{
  for (int trainType = 0; trainType < 5; trainType++)
  {
    calculateTrainingValueSingle(trainType);
  }
}
void Game::addStatus(int idx, int value)
{
  fiveStatus[idx] += value;
  if (fiveStatus[idx] > fiveStatusLimit[idx])
    fiveStatus[idx] = fiveStatusLimit[idx];
  if (fiveStatus[idx] < 1)
    fiveStatus[idx] = 1;
}
void Game::addVital(int value)
{
  vital += value;
  if (vital > maxVital)
    vital = maxVital;
  if (vital < 0)
    vital = 0;
}
void Game::addMotivation(int value)
{
  motivation += value;
  if (motivation > 5)
    motivation = 5;
  if (vital < 1)
    motivation = 1;
}
void Game::addJiBan(int idx, int value)
{
  if (idx < 6 && isAiJiao)value += 2;
  cardJiBan[idx] += value;
  if (cardJiBan[idx] > 100)cardJiBan[idx] = 100;
}
void Game::addTrainingLevelCount(int item, int value)
{
  assert(item >= 0 && item < 5 && "addTrainingLevelCount���Ϸ�ѵ��");
  trainLevelCount[item] += value;
  if (trainLevelCount[item] > 48)trainLevelCount[item] = 48;
}
void Game::addAllStatus(int value)
{
  for (int i = 0; i < 5; i++)addStatus(i, value);
}
void Game::addSpirit(std::mt19937_64& rand, int s)
{
  if (s > 32)//������Ƭ
  {
    addSpirit(rand, s % 32);
    addSpirit(rand, s % 32);
    return;
  }

  int place = -1;//�ڵڼ�����Ƭ��
  for (int i = 0; i < 8; i++)
  {
    if (venusSpiritsBottom[i] == 0)
    {
      place = i;
      break;
    }
  }
  if (place == -1)return;//��Ƭ������
  venusSpiritsBottom[place] = s;

  //ѵ���ȼ�����+1
  {
    int type = s % 8 - 1;
    if (type < 5 && type >= 0)
      addTrainingLevelCount(type, 1);
  }

  if (place % 2 == 1)//�ڶ���������Ƭ
  {
    int sL = venusSpiritsBottom[place - 1];
    int colorL = sL / 8;
    int typeL = sL % 8;
    int typeR = s % 8;

    int type = typeL;
    if (rand() % 5 == 0)
      type = typeR;//��20%�������Ҳ���Ƭ������
    int sU = type + 8 * colorL;//�ϲ���Ƭ
    int layer2Place = place / 2;
    venusSpiritsUpper[layer2Place] = sU;


    if(layer2Place%2==1)//������������Ƭ
    {
      int sL = venusSpiritsUpper[layer2Place - 1];
      int colorL = sL / 8;
      int typeL = sL % 8;
      int typeR = sU % 8;

      int type = typeL;
      if (rand() % 5 == 0)
        type = typeR;//��20%�������Ҳ���Ƭ������
      int sU2 = type + 8 * colorL;//�ϲ���Ƭ
      int layer3Place = 4 + layer2Place / 2;
      venusSpiritsUpper[layer3Place] = sU2;

    }
  }

  if (place == 7)//��Ƭ������
  {
    int wiseColor = -1;
    int color1 = venusSpiritsBottom[0] / 8;
    int color2 = venusSpiritsBottom[4] / 8;
    if (color1 == color2)//1��λ��5��λͬɫ
    {
      wiseColor = color1;
    }
    else//��һ���ĸ���
    {
      int count = 0; 
      for (int i = 0; i < 8; i++)
      {
        int c = venusSpiritsBottom[i] / 8;
        if (c == color1)count += 1;
        else if (c == color2)count -= 1;
      }
      if (count > 0)wiseColor = color1;
      else if (count < 0)wiseColor = color2;
      else//�������
      {
        wiseColor = rand() % 2 ? color1 : color2;
      }
    }

    venusAvailableWisdom = wiseColor + 1;//123�ֱ��Ǻ�����
  }
  calculateVenusSpiritsBonus();

}
void Game::activateVenusWisdom()
{
  assert(venusAvailableWisdom != 0);
  assert(venusIsWisdomActive == false);
  venusIsWisdomActive = true;
  if (venusAvailableWisdom == 1)//����
  {
    if (venusLevelRed < 5)
      venusLevelRed += 1;
    addVital(50);
    motivation = 5;
    //������Ŀ�������ﴦ��
  }
  if (venusAvailableWisdom == 2)//����
  {
    if (venusLevelBlue < 5)
      venusLevelBlue += 1;
    for (int i = 0; i < 6; i++)
    {
      if (cardData[i]->cardType < 5)
        cardHint[i] = true;
    }
    //������Ŀ�������ﴦ��
  }
  if (venusAvailableWisdom == 3)//����
  {
    if (venusLevelYellow < 5)
      venusLevelYellow += 1;
    //����ѵ���������ﴦ��
  }

  calculateTrainingValue();//���¼���ѵ��ֵ
}
void Game::clearSpirit()
{

}
int Game::calculateFailureRate(int trainType) const
{
  //������ϵ�ѵ��ʧ���ʣ����κ��� A*x^2 + B*x + C + 0.5 * trainLevel
  //���Ӧ����2%����
  static const double A = 0.0245;
  static const double B[5] = { -3.77,-3.74,-3.76,-3.81333,-3.286 };
  static const double C[5] = { 130,127,129,133.5,80.2 };

  double f = A * vital * vital + B[trainType] * vital + C[trainType] + 0.5 * getTrainingLevel(trainType);
  int fr = round(f);
  if (vital > 60)fr = 0;//�����Ƕ��κ�������������103ʱ�������fr����0��������Ҫ�ֶ�����
  if (fr < 0)fr = 0;
  if (fr > 99)fr = 99;//����ϰ���֣�ʧ�������99%
  fr += failureRateBias;
  if (fr < 0)fr = 0;
  if (fr > 100)fr = 100;
  return fr;
}
void Game::calculateVenusSpiritsBonus()
{
  for (int i = 0; i < 6; i++)
    spiritBonus[i] = 0;
  //����ײ�
  for (int i = 0; i < 8; i++)
  {
    int s = venusSpiritsBottom[i];
    int type = s % 8 - 1;//012345��Ӧ����������pt
    if (type == -1)//����Ƭ��
      break;
    int color = s / 8; //012��Ӧ������
    spiritBonus[type] += 1;
  }
  //����ڶ���
  for (int i = 0; i < 4; i++)
  {
    int s = venusSpiritsUpper[i];
    int sL = venusSpiritsBottom[i * 2];//������Ƭ
    int sR = venusSpiritsBottom[i * 2 + 1];//������Ƭ
    int type = s % 8 - 1;//012345��Ӧ����������pt
    if (type == -1)//����Ƭ��
      break;
    if (sL / 8 == sR / 8)//�����Ҳ���Ƭ��ɫ��ͬ
      spiritBonus[type] += 2;
    else
      spiritBonus[type] += 3;
  }

  //���������
  for (int i = 0; i < 2; i++)
  {
    int s = venusSpiritsUpper[i + 4];
    int sL = venusSpiritsUpper[i * 2];//������Ƭ
    int sR = venusSpiritsUpper[i * 2 + 1];//������Ƭ
    int type = s % 8 - 1;//012345��Ӧ����������pt
    if (type == -1)//����Ƭ��
      break;
    if (sL / 8 == sR / 8)//�����Ҳ���Ƭ��ɫ��ͬ
      spiritBonus[type] += 2;
    else
      spiritBonus[type] += 3;
  }
}
std::array<int, 6> Game::calculateBlueVenusBonus(int trainType) const
{
  std::array<int, 6> value = { 0,0,0,0,0,0 };
  int cardCount = 0;
  for (int i = 0; i < 6; i++)
  {
    if (cardDistribution[trainType][i])
    {
      int cardType = cardData[i]->cardType;
      if (cardType < 5)//����������
      {
        cardCount++;
        for (int j = 0; j < 6; j++)
          value[j] += GameConstants::BlueVenusRelatedStatus[cardType][j];
      }
    }
  }
  for (int j = 0; j < 6; j++)
  {
    if (value[j] > 0)//��������
      value[j] += spiritBonus[j];
  }
  value[5] += 20 * cardCount;
  return value;
}
void Game::runRace(int basicFiveStatusBonus, int basicPtBonus)
{
  int cardRaceBonus = 0;
  for (int card = 0; card < 6; card++)
  {
    cardRaceBonus += cardData[card]->saiHou;
  }
  double raceMultiply = 1 + 0.01 * cardRaceBonus;
  if (venusAvailableWisdom == 1 && venusIsWisdomActive)//����
    raceMultiply *= 1.35;
  int fiveStatusBonus = floor(raceMultiply * basicFiveStatusBonus);
  int ptBonus = floor(raceMultiply * basicPtBonus);
  addAllStatus(fiveStatusBonus);
  skillPt += basicPtBonus;
}
void Game::handleVenusOutgoing(int chosenOutgoing)
{
  venusCardOutgoingUsed[chosenOutgoing] = true;
  assert(cardData[0]->cardType == 5 && "���ſ����ڵ�һ��λ��");
  if (chosenOutgoing == 0)//��
  {
    addVital(45);
    addMotivation(1);
    skillPt += 24;
    skillPt += 10;//���ܵȼ�
    addJiBan(0, 5);
  }
  else if (chosenOutgoing == 1)//��
  {
    addVital(32);
    addMotivation(1);
    addStatus(0, 12);
    addStatus(4, 12);
    skillPt += 10;//���ܵȼ�
    addJiBan(0, 5);
  }
  else if (chosenOutgoing == 2)//��
  {
    maxVital += 4;
    addVital(32);
    addMotivation(1);
    addStatus(1, 8);
    addStatus(2, 8);
    addStatus(3, 8);
    skillPt += 15;//���ܵȼ�
    addJiBan(0, 5);

  }
  else if (chosenOutgoing == 3)//��1
  {
    addVital(45);
    addMotivation(1);
    addStatus(1, 15);
    addStatus(2, 15);
    addStatus(3, 15);
    addJiBan(0, 5);

  }
  else if (chosenOutgoing == 4)//��2
  {
    addVital(52);
    addMotivation(1);
    addAllStatus(9);
    skillPt += 36;
    skillPt += 50;//���ܵȼ�
    addJiBan(0, 5);
    venusCardIsQingRe = true;
  }
  else assert(false && "δ֪�����ų���");
}
void Game::handleVenusThreeChoicesEvent(std::mt19937_64& rand, int chosenColor)
{
  printEvents("����Ů����ѡһ�¼�");
  int spiritType = chosenColor * 8 + rand() % 6 + 1;//��Ƭ����
  addSpirit(rand, spiritType);
  assert(cardData[0]->cardType == 5 && "���ſ����ڵ�һ��λ��");
  addJiBan(0, 5);
  if (chosenColor == 0)
  {
    skillPt += 4;
    if(venusCardIsQingRe)
      skillPt += 5;
  }
  else if (chosenColor == 1)
  {
    addStatus(0, 4);
    if (venusCardIsQingRe)
      skillPt += 4;
  }
  else if (chosenColor == 2)
  {
    addStatus(1, 4);
    if (venusCardIsQingRe)
      skillPt += 4;
  }

  if (venusCardUnlockOutgoing)
    venusCardIsQingRe = true;//�����Ƿ���ʧ��checkEventAfterTrain�ﴦ��
}
void Game::calculateTrainingValueSingle(int trainType)
{
  //�������ˣ��������������Լ�ֵ
  failRate[trainType] = calculateFailureRate(trainType);//����ʧ����

  vector<CardTrainingEffect> effects;
  for (int card = 0; card < 6; card++)
  {
    if (cardDistribution[trainType][card])//����������ѵ��
    {
      effects.push_back(cardData[card]->getCardEffect(*this, trainType, cardJiBan[card], cardData[card]->effectFactor));
    }
  }
  //�����Ů���ѵ��
  //1.��ͷ������
  int cardNum = effects.size();
  double cardNumMultiplying = 1 + 0.05 * cardNum;
  //2.��Ȧ(����ѵ��)���ʣ�ע���Ƿ������Ѿ���getCardEffect�￼�ǹ���
  double youQingMultiplying = 1;
  for (int i = 0; i < cardNum; i++)
    youQingMultiplying *= (1 + 0.01 * effects[i].youQing);
  //3.ѵ������
  double xunLianBonusTotal = 0;
  for (int i = 0; i < cardNum; i++)
    xunLianBonusTotal += effects[i].xunLian;
  double xunLianMultiplying = 1 + 0.01 * xunLianBonusTotal;
  //4.�ɾ�����
  double ganJingBasic = 0.1 * (motivation - 3);
  double ganJingBonusTotal = 0;
  for (int i = 0; i < cardNum; i++)
    ganJingBonusTotal += effects[i].ganJing;
  double ganJingMultiplying = 1 + ganJingBasic * (1 + 0.01 * ganJingBonusTotal);

  //�벻ͬ�����޹ص��ܱ���
  double totalMultiplying = cardNumMultiplying * youQingMultiplying * xunLianMultiplying * ganJingMultiplying;

  //5.����ֵ
  int trainLv = getTrainingLevel(trainType);
  int basicValue[6] = { 0,0,0,0,0,0 };
  for (int i = 0; i < cardNum; i++)
  {
    for (int j = 0; j < 6; j++)
      basicValue[j] += effects[i].bonus[j];
  }
  for (int j = 0; j < 6; j++)
  {
    int b = GameConstants::TrainingBasicValue[trainType][trainLv][j];
    if(b>0)//��������
      basicValue[j] += b;
    else
      basicValue[j] = 0;
  }

  //6.�ɳ���
  double growthRates[6] = { 1,1,1,1,1,1 };
  for (int j = 0; j < 5; j++)
    growthRates[j] = 1.0 + 0.01 * umaData->fiveStatusBonus[j];
    //growthRates[j] = 1.0 + 0.01 * GameDatabase::AllUmas[umaId].fiveStatusBonus[j];

  //�²�����ֵ
  int totalValue[6];
  for (int j = 0; j < 6; j++)
  {
    int v = int(totalMultiplying * basicValue[j] * growthRates[j]);//����ȡ����
    if (v > 100)v = 100;
    totalValue[j] = v;
  }
  
  //7.��Ƭ
  for (int j = 0; j < 6; j++)
  {
    if (totalValue[j] > 0)//��������
      totalValue[j] += spiritBonus[j];
  } 
    
  //8.Ů��ȼ��ӳ�
  double venusMultiplying = 1.00 + 0.01 * (
    GameConstants::VenusLevelTrainBonus[venusLevelRed]
    + GameConstants::VenusLevelTrainBonus[venusLevelBlue]
    + GameConstants::VenusLevelTrainBonus[venusLevelYellow]
    );

  for (int j = 0; j < 6; j++)
  {
    totalValue[j] = int(venusMultiplying * totalValue[j]);
  }

  //����
  int vitalChange=GameConstants::TrainingBasicValue[trainType][trainLv][6];
  for (int i = 0; i < cardNum; i++)
    vitalChange += effects[i].vitalBonus;
  if (vitalChange < 0)//��������ʱ������Ů��ȼ�
  {
    vitalChange = round(vitalChange*(1- 0.01*GameConstants::RedVenusLevelVitalCostDown[venusLevelRed]));
  }


  for (int j = 0; j < 6; j++)
  {
    trainValue[trainType][j] = totalValue[j];
  }
  trainValue[trainType][6] = vitalChange;
}

bool Game::applyTraining(std::mt19937_64& rand, int chosenTrain, bool useVenus, int chosenSpiritColor, int chosenOutgoing, int forceThreeChoicesEvent)
{
  assert(stageInTurn == 1);
  stageInTurn = 2;
  if (isRacing)
  {
    if (useVenus)
    {
      if (venusAvailableWisdom != 0)
        activateVenusWisdom();//��checkEventAfterTrain()��ر�Ů�������Ƭ
      else
        return false;
    }
    if (turn != TOTAL_TURN - 1)//����GrandMaster
      runRace(GameConstants::NormalRaceFiveStatusBonus, GameConstants::NormalRacePtBonus);
    addJiBan(6, 4);//���³��+4

    int newSpirit = (rand() % 6 + 1) + (rand() % 3) * 8;//�����������Ƭ
    addSpirit(rand, newSpirit);
    addSpirit(rand, newSpirit);
    return true;//GUR,WBC,SWBC,GrandMaster�ĳ�������checkEventAfterTrain()�ﴦ�������������
  }
  if (chosenTrain == 5)//��Ϣ
  {
    if (useVenus)
    {
      if (venusAvailableWisdom != 0)
        activateVenusWisdom();//��checkEventAfterTrain()��ر�Ů�������Ƭ
      else
        return false;
    }
    if (isXiaHeSu())
    {
      addVital(40);
      addMotivation(1);
    }
    else
    {
      int r = rand() % 100;
      if (r < 25)
        addVital(70);
      else if (r < 82)
        addVital(50);
      else
        addVital(30);
    }

    addSpirit(rand, spiritDistribution[chosenTrain]);
  }
  else if (chosenTrain == 7)//����
  {
    if (turn <= 12 || turn >= 72)
    {
      printEvents("ǰ13�غϺ����6�غ��޷�����");
      return false;
    }
    if (useVenus)
    {
      if (venusAvailableWisdom != 0)
        activateVenusWisdom();//��checkEventAfterTrain()��ر�Ů�������Ƭ
      else
        return false;
    }
    runRace(2, 40);//���ԵĽ���
    addSpirit(rand, spiritDistribution[chosenTrain]);
  }
  else if (chosenTrain == 6)//���
  {
    if (isXiaHeSu())
    {
      printEvents("�ĺ���ֻ����Ϣ���������");
      return false;
    }
    if(!isOutgoingLegal(chosenOutgoing))
    {
      printEvents("���Ϸ������");
      return false;
    }
    assert(!isXiaHeSu() && "�ĺ��޲��������");
    assert(isOutgoingLegal(chosenOutgoing) && "���Ϸ������");
    if (useVenus)
    {
      if (venusAvailableWisdom != 0)
        activateVenusWisdom();//��checkEventAfterTrain()��ر�Ů�������Ƭ
      else
        return false;
    }
    if (chosenOutgoing < 5)//�������
      handleVenusOutgoing(chosenOutgoing);
    else if (chosenOutgoing == 6)//��ͨ���
    {
      //���ò�����ˣ���50%��2���飬50%��1����10����
      if (rand() % 2)
        addMotivation(2);
      else
      {
        addMotivation(1);
        addVital(10);
      }
    }

    addSpirit(rand, spiritDistribution[chosenTrain]);
  }
  else if (chosenTrain <= 4 && chosenTrain >= 0)//����ѵ��
  {
    if (useVenus)
    {
      if (venusAvailableWisdom != 0)
        activateVenusWisdom();//��checkEventAfterTrain()��ر�Ů�������Ƭ
      else
        return false;
    }
    if (rand() % 100 < failRate[chosenTrain])//ѵ��ʧ��
    {
      if (failRate[chosenTrain] >= 20 && (rand() % 100 < failRate[chosenTrain]))//ѵ����ʧ�ܣ�������Ϲ�µ�
      {
        printEvents("ѵ����ʧ�ܣ�");
        addStatus(chosenTrain, -10);
        if (fiveStatus[chosenTrain] > 1200)
          addStatus(chosenTrain, -10);//��Ϸ��1200���Ͽ����Բ��۰룬�ڴ�ģ�������Ӧ1200���Ϸ���
        //�����2��10�������ĳ�ȫ����-4���������
        for (int i = 0; i < 5; i++)
        {
          addStatus(i, -4);
          if (fiveStatus[i] > 1200)
            addStatus(i, -4);//��Ϸ��1200���Ͽ����Բ��۰룬�ڴ�ģ�������Ӧ1200���Ϸ���
        }
        addMotivation(-3);
        addVital(10);
      }
      else//Сʧ��
      {
        printEvents("ѵ��Сʧ�ܣ�");
        addStatus(chosenTrain, -5);
        if (fiveStatus[chosenTrain] > 1200)
          addStatus(chosenTrain, -5);//��Ϸ��1200���Ͽ����Բ��۰룬�ڴ�ģ�������Ӧ1200���Ϸ���
        addMotivation(-1);
      }
    }
    else
    {
      //�ȼ���ѵ��ֵ
      for (int i = 0; i < 5; i++)
        addStatus(i, trainValue[chosenTrain][i]);
      skillPt += trainValue[chosenTrain][5];
      addVital(trainValue[chosenTrain][6]);

      //�
      for (int i = 0; i < 8; i++)
      {
        if (cardDistribution[chosenTrain][i])
        {
          assert(cardData[0]->cardType == 5 && "���ſ����ڵ�һ��λ��");
          if (i == 0) //���ŵ�һ��+4�
            addJiBan(i, 4);
          else
            addJiBan(i, 7);
          if (i == 6)skillPt += 2;//���³�
          if (i >= 6)continue;//���³��ͼ���
        }
      }

      //���(hint)
      vector<int> hintCards;
      for (int i = 0; i < 6; i++)
      {
        if (cardDistribution[chosenTrain][i] && cardHint[i])
        {
          hintCards.push_back(i);
        }
      }

      auto applyHint= [this](int i)  {
        addJiBan(i, 5);
        auto& hintBonus = cardData[i]->hintBonus;
        for (int i = 0; i < 5; i++)
          addStatus(i, hintBonus[i]);
        skillPt += hintBonus[5];
      };

      
      //����
      if (venusIsWisdomActive && venusAvailableWisdom == 2)
      {
        auto blueVenusBonus = calculateBlueVenusBonus(chosenTrain);
        for (int i = 0; i < 5; i++)
          addStatus(i, blueVenusBonus[i]);
        skillPt += blueVenusBonus[5];

        for (auto i = 0; i < hintCards.size(); i++)
          applyHint(hintCards[i]);
      }
      else//һ��ֻ��һ�ſ��ĺ�������Ч�����ǿ���
      {
        if (hintCards.size() != 0)
        {
          int hintCard = hintCards[rand() % hintCards.size()];
          applyHint(hintCard);
        }
      }

      //����Ƭ
      addSpirit(rand, spiritDistribution[chosenTrain]);

      //�����Ů�����ڵ�ѵ��
      assert(cardData[0]->cardType == 5 && "���ſ����ڵ�һ��λ��");
      if (cardDistribution[chosenTrain][0])
      {
        if (!venusCardFirstClick)//��һ�ε�
        {
          printEvents("��һ�ε�Ů��");
          venusCardFirstClick = true;
          addAllStatus(3);
          addVital(10);
          addJiBan(0, 10);
        }
        else
        {
          //��ѡһ�¼����ʣ���ʱ�²�Ϊ40%*(1+��Ů��ȼ��ӳ�)
          bool activateThreeChoicesEvent = randBool(rand,getThreeChoicesEventProb(useVenus));
          if (forceThreeChoicesEvent == 1)
            activateThreeChoicesEvent = true;
          else if (forceThreeChoicesEvent == -1)
              activateThreeChoicesEvent = false;
          if (activateThreeChoicesEvent)
            handleVenusThreeChoicesEvent(rand, chosenSpiritColor);
        }

      }

      //ѵ���ȼ�����+2
      if(!isXiaHeSu())
        addTrainingLevelCount(chosenTrain, 2);
    }
  }
  else
  {
    printEvents("δ֪��ѵ����Ŀ");
    return false;
  }
  return true;
}

float Game::getSkillScore() const
{
  float scorePtRate = isQieZhe ? GameConstants::ScorePtRateQieZhe : GameConstants::ScorePtRate;
  return scorePtRate * skillPt;
}

int Game::finalScore() const
{
  int total = 0;
  for (int i = 0; i < 5; i++)
    total += GameConstants::FiveStatusFinalScore[min(fiveStatus[i],fiveStatusLimit[i])];
  
  total += getSkillScore();
  return total;
}

bool Game::isEnd() const
{
  return turn >= TOTAL_TURN;
}

int Game::getTrainingLevel(int item) const
{
  int level ;
  if (venusIsWisdomActive && venusAvailableWisdom == 1)//��Ů��
    level = 5;
  else if(isXiaHeSu())
    level = 4;
  else
  {
    assert(trainLevelCount[item] <= 48, "ѵ���ȼ���������48");
    level = trainLevelCount[item] / 12;
    if (level > 4)level = 4;
  }
  return level;
}

bool Game::isOutgoingLegal(int chosenOutgoing) const
{
  assert(chosenOutgoing >= 0 && chosenOutgoing <= 5 && "δ֪�����");
  if (isXiaHeSu())return false;//�ĺ��޲��������
  if (chosenOutgoing == 5)return true;//��ͨ���
  //ʣ�µ����������
  if (!venusCardUnlockOutgoing)return false;
  if (venusCardOutgoingUsed[chosenOutgoing])return false;
  if (chosenOutgoing == 0 || chosenOutgoing == 1 || chosenOutgoing == 2)return true;
  else if (chosenOutgoing == 3)
    return venusCardOutgoingUsed[0] && venusCardOutgoingUsed[1] && venusCardOutgoingUsed[2];
  else if (chosenOutgoing == 4)
    return venusCardOutgoingUsed[3];
  else return false;//δ֪



}

bool Game::isXiaHeSu() const
{
  return (turn >= 36 && turn <= 39) || (turn >= 60 && turn <= 63);
}

double Game::getThreeChoicesEventProb(bool useVenusIfFull) const
{
  if (!venusCardFirstClick)return 0.0;
  if (venusCardIsQingRe)return 1.0;
  if (venusAvailableWisdom == 2 && useVenusIfFull)return 1.0;
  //��ѡһ�¼����ʣ���ʱ�²�Ϊ40%*(1+��Ů��ȼ��ӳ�)
  return GameConstants::VenusThreeChoicesEventProb * (1 + 0.01 * GameConstants::BlueVenusLevelHintProbBonus[venusLevelBlue]);

}



void Game::checkEventAfterTrain(std::mt19937_64& rand)
{
  assert(stageInTurn == 2);
  stageInTurn = 0;
  //Ů��᲻������
  if (venusCardFirstClick && (!venusCardUnlockOutgoing))
  {
    if (randBool(rand, GameConstants::VenusUnlockOutgoingProbEveryTurn))//����
    {
      printEvents("Ů�����������");
      //���Ƥ�ѧ����Ů��
      venusCardUnlockOutgoing = true;
      venusCardIsQingRe = true;
      addAllStatus(6);
      skillPt += 12;
      assert(cardData[0]->cardType == 5 && "���ſ����ڵ�һ��λ��");
      addJiBan(0, 5);
    }
  }

  //Ů�������Ƿ����
  if(venusCardIsQingRe)
  {
    if (randBool(rand, GameConstants::VenusQingReDeactivateProb[venusCardQingReContinuousTurns]))
    {
      printEvents("Ů�����Ƚ���");
      venusCardIsQingRe = false;
      venusCardQingReContinuousTurns = 0;
    }
    else venusCardQingReContinuousTurns++;
  }

  //������̶ֹ��¼�
  if (turn == 23)//��һ�����
  {
    //GUR
    int raceFiveStatusBonus = 10;
    int racePtBonus = 50;
    if (venusLevelYellow >= 1)
      raceFiveStatusBonus += 2;//���1������+10���ĳ�ƽ��
    if (venusLevelRed >= 1)
      racePtBonus += 20;
    runRace(raceFiveStatusBonus, racePtBonus);
    if (venusLevelBlue >= 1)
      skillPt += 10;//���1���ܷ�����

    //��ѵ���ȼ�
    for (int i = 0; i < 5; i++)
      addTrainingLevelCount(i, 8);

    //�����ѡһ�¼���ѡ���Ի�������
    //Ϊ�˼򻯣�ֱ����Ϊȫ����+5
    if (maxVital - vital >= 50)
      addVital(20);
    else 
      addAllStatus(5);

    //Ů�񤵤ޡ��ҤȤ䤹��
    if (venusCardUnlockOutgoing)
    {
      addVital(19);
      skillPt += 36;
      skillPt += 50;//���ܵ�Ч
      assert(cardData[0]->cardType == 5 && "���ſ����ڵ�һ��λ��");
      addJiBan(0, 5);
    }

    printEvents("GUR����");

  }
  else if (turn == 29)//�ڶ���̳�
  {
    for (int i = 0; i < 5; i++)
      addStatus(i, zhongMaBlueCount[i] * 6); //�����ӵ���ֵ
    for (int i = 0; i < 5; i++)
      addStatus(i, zhongMaExtraBonus[i]); //�籾���ӵ���ֵ
    skillPt += zhongMaExtraBonus[5];

    printEvents("�ڶ���̳�");
  }
  else if (turn == 47)//�ڶ������
  {
    //WBC
    int raceFiveStatusBonus = 15;
    int racePtBonus = 60;
    if (venusLevelYellow >= 2)
      raceFiveStatusBonus += 4;//���2������+10���ĳ�ƽ��
    if (venusLevelRed >= 2)
      racePtBonus += 30;
    runRace(raceFiveStatusBonus, racePtBonus);
    if (venusLevelBlue >= 2)
      skillPt += 20;//���2���ܷ�����

    //��ѵ���ȼ�
    for (int i = 0; i < 5; i++)
      addTrainingLevelCount(i, 8);

    //�����ѡһ�¼���ѡ���Ի�������
    if (maxVital - vital >= 50)
      addVital(30);
    else
      addAllStatus(8);
    printEvents("WBC����");
  }
  else if (turn == 48)//�齱
  {
    int rd = rand() % 100;
    if (rd < 16)//��Ȫ��һ�Ƚ�
    {
      addVital(30);
      addAllStatus(10);
      addMotivation(2);

      printEvents("�齱�����������Ȫ/һ�Ƚ�");
    }
    else if (rd < 16 + 27)//���Ƚ�
    {
      addVital(20);
      addAllStatus(5);
      addMotivation(1);
      printEvents("�齱��������˶��Ƚ�");
    }
    else if (rd < 16 + 27 + 46)//���Ƚ�
    {
      addVital(20);
      printEvents("�齱������������Ƚ�");
    }
    else//��ֽ
    {
      addMotivation(-1);
      printEvents("�齱��������˲�ֽ");
    }
  }
  else if (turn == 49)//������
  {
    skillPt += 170 / GameConstants::ScorePtRate;//����ֱ�ӵȼ۳�pt
    printEvents("���еȼ�+1");
  }
  else if (turn == 53)//������̳�&���³�������
  {
    for (int i = 0; i < 5; i++)
      addStatus(i, zhongMaBlueCount[i] * 6); //�����ӵ���ֵ
    for (int i = 0; i < 5; i++)
      addStatus(i, zhongMaExtraBonus[i]); //�籾���ӵ���ֵ
    skillPt += zhongMaExtraBonus[5];
    printEvents("������̳�");
    if (cardJiBan[6] >= 60)//����������
    {
      addMotivation(1);
      skillPt += 170 / GameConstants::ScorePtRate;//����ֱ�ӵȼ۳�pt
      printEvents("���еȼ�+1");
    }
    else
    {
      addVital(-5);
      skillPt += 25;
    }
  }
  else if (turn == 70)//������
  {
    skillPt += 170 / GameConstants::ScorePtRate;//����ֱ�ӵȼ۳�pt
    printEvents("���еȼ�+1");
  }
  else if (turn == 71)//���������
  {
    //SWBC
    int raceFiveStatusBonus = 20;
    int racePtBonus = 70;
    if (venusLevelYellow >= 3)
      raceFiveStatusBonus += 6;//���3������+10���ĳ�ƽ��
    if (venusLevelRed >= 3)
      racePtBonus += 45;
    runRace(raceFiveStatusBonus, racePtBonus);
    if (venusLevelBlue >= 3)
      skillPt += 30;//���3���ܷ�����

    //��ѵ���ȼ�
    for (int i = 0; i < 5; i++)
      addTrainingLevelCount(i, 8);

    printEvents("SWBC����");
  }
  else if (turn == 76)//���һսǰ����ѡһ
  {
    int totalLevel = venusLevelRed + venusLevelBlue + venusLevelYellow;
    int maxLevel = max(venusLevelRed, max(venusLevelBlue, venusLevelYellow));
    if (maxLevel >= 4)
    {
      addAllStatus(10);
      skillPt += 50;//���ܵ�Ч
      if (maxLevel >= 5)
        skillPt += 20;//�����ۿ�
      if (totalLevel >= 12)
        skillPt += 40;//�����ۿ�
    }
  }
  else if (turn == 77)//���һս
  {
    //GrandMasters
    int raceFiveStatusBonus = 20;
    int racePtBonus = 80;
    runRace(raceFiveStatusBonus, racePtBonus);

    //Ů���¼�
    if (venusCardOutgoingUsed[4])//����������
    {
      addAllStatus(12);
      skillPt += 12;
    }
    else
    {
      addAllStatus(8);
    }

    //����
    if (cardJiBan[7] >= 100)
    {
      addAllStatus(5);
      skillPt += 20;
    }
    else if (cardJiBan[7] >= 80)
    {
      addAllStatus(3);
      skillPt += 10;
    }
    else
    {
      skillPt += 5;
    }

    //�������߰���
    addAllStatus(25);
    skillPt += 80;

  }

  //ģ���������¼�

  //֧Ԯ�������¼��������һ������5�
  if (randBool(rand, 0.3))
  {
    int card = rand() % 6;
    addJiBan(card, 5);

    printEvents("ģ������¼���" + GameDatabase::AllCards[cardId[card]].cardName + " ���+5");
  }

  //ģ�����߰���������¼�
  if (turn < 72)
  {
    addAllStatus(1);
    printEvents("ģ������¼���ȫ����+1");
  }

  //������
  if (randBool(rand, 0.1))
  {
    addVital(10);
    printEvents("ģ������¼�������+10");
  }

  //������
  if (randBool(rand, 0.02))
  {
    addMotivation(1);
    printEvents("ģ������¼�������+1");
  }

  //������
  if (randBool(rand, 0.04))
  {
    addMotivation(-1);
    printEvents("ģ������¼���\033[0m\033[33m����-1\033[0m\033[32m");
  }
  
  //�����Ů�������Ƭ
  if (venusIsWisdomActive)
  {
    venusIsWisdomActive = false;
    venusAvailableWisdom = 0;
    for (int i = 0; i < 8; i++)
      venusSpiritsBottom[i] = 0;
    for (int i = 0; i < 6; i++)
      venusSpiritsUpper[i] = 0;
    for (int i = 0; i < 6; i++)
      spiritBonus[i] = 0;
  }

  //�غ���+1
  turn++;
  if (turn < TOTAL_TURN)
  {
    isRacing = umaData->races[turn] & TURN_RACE;
    //isRacing = GameDatabase::AllUmas[umaId].races[turn] & TURN_RACE;
  }
  else
  {
    printEvents("���ɽ���!");
    printEvents("��ĵ÷��ǣ�" + to_string(finalScore()));
  }

}

void Game::applyTrainingAndNextTurn(std::mt19937_64& rand, int chosenTrain, bool useVenus, int chosenSpiritColor, int chosenOutgoing, int forceThreeChoicesEvent)
{
  assert(stageInTurn == 1);
  assert(turn < TOTAL_TURN && "Game::applyTrainingAndNextTurn��Ϸ�ѽ���");
  bool suc = applyTraining(rand, chosenTrain, useVenus, chosenSpiritColor, chosenOutgoing, forceThreeChoicesEvent);
  assert(suc && "Game::applyTrainingAndNextTurnѡ���˲��Ϸ���ѵ��");

  checkEventAfterTrain(rand);
  if (isEnd()) return;

  if (isRacing)
  {
    if (venusAvailableWisdom == 0)//�����غ����޷���Ů���ٽ���һ���غ�
    {
      randomDistributeCards(rand);//��stageInTurn�ĳ�1
      bool useVenus = false;
      applyTrainingAndNextTurn(rand, -1, useVenus, -1, -1, -1);
    }
  }

  randomDistributeCards(rand);

}
