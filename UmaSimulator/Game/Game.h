#pragma once
#include <random>
#include <array>
#include "../GameDatabase/GameDatabase.h"

struct Game
{
  //����״̬����������ǰ�غϵ�ѵ����Ϣ
  int umaId;//�����ţ���KnownUmas.cpp
  int turn;//�غ�������0��ʼ����77����
  int vital;//������������vital������Ϊ��Ϸ��������е�
  int maxVital;//��������
  bool isQieZhe;//����
  bool isAiJiao;//����
  int failureRateBias;//ʧ���ʸı�������ϰ����=2����ϰ����=-2
  int fiveStatus[5];//��ά���ԣ�1200���ϲ�����
  //int fiveStatusUmaBonus[5];//��������ӳ�
  int fiveStatusLimit[5];//��ά�������ޣ�1200���ϲ�����
  int skillPt;//���ܵ�
  int motivation;//�ɾ�����1��5�ֱ��Ǿ����������õ�
  int cardId[6];//6�ſ���id
  int cardJiBan[8];//����ſ��ֱ�012345�����³�6������7
  int trainLevelCount[5];//���ѵ���ĵȼ��ļ�����ʵ��ѵ���ȼ�=min(5,t/12+1)
  int zhongMaBlueCount[5];//����������Ӹ���������ֻ��3��
  int zhongMaExtraBonus[6];//����ľ籾�����Լ����ܰ����ӣ���Ч��pt����ÿ�μ̳мӶ��١�ȫ��ʦ�����ӵ���ֵ��Լ��30��30��200pt
  int isRacing;//����غ��Ƿ��ڱ���
  //bool raceTurns[TOTAL_TURN];//��Щ�غ��Ǳ��� //��umaId�������GameDatabase::AllUmas����

  //Ů�����
  int venusLevelYellow;//Ů��ȼ�
  int venusLevelRed;
  int venusLevelBlue;

  int venusSpiritsBottom[8];//�ײ���Ƭ��8*��ɫ+���ԡ���ɫ012��Ӧ�����ƣ�����123456��Ӧ����������pt��������spirit������Ϊ��Ϸ��������е�
  int venusSpiritsUpper[4 + 2];//��˳��ֱ��ǵڶ���͵��������Ƭ�������ײ���Ƭһ�¡�*2����*3�ֳ���
  int venusAvailableWisdom;//�����Ů����ǣ�123�ֱ��Ǻ����ƣ�0��û��
  bool venusIsWisdomActive;//�Ƿ�����ʹ�����

  //���ſ�ר��
  bool venusCardFirstClick;// �Ƿ��Ѿ���������ſ�
  bool venusCardUnlockOutgoing;// �Ƿ�������
  bool venusCardIsQingRe;// ����zone
  int venusCardQingReContinuousTurns;//Ů�����������˼����غ�
  bool venusCardOutgoingUsed[5];// �ù���Щ���У������Ǻ�������������

  //��ǰ�غϵ�ѵ����Ϣ
  //0֧Ԯ����δ���䣬1֧Ԯ��������ϻ������ʼǰ��2ѵ������������������0�����̶ֹ��¼�������¼���������һ���غ�
  //stageInTurn=0ʱ�������������������ֵ��stageInTurn=1ʱ�����������������policy
  int stageInTurn;
  bool cardDistribution[5][8];//֧Ԯ���ֲ������ſ��ֱ�012345�����³�6������7
  bool cardHint[6];//���ſ��ֱ���û�������
  int spiritDistribution[5 + 3];//��Ƭ�ֲ�����������ѵ��01234����Ϣ5�����6������7����Ϊ2��Ƭ�����32

  //ͨ�������õ���Ϣ
  int spiritBonus[6];//��Ƭ�ӳ�
  int trainValue[5][7];//��һ�����ǵڼ���ѵ�����ڶ���������������������pt����
  int failRate[5];//ѵ��ʧ����

  //����������������ڷ��俨�����Ƭ
  //ǰ���ǵ�����Ϊ����
  std::discrete_distribution<> cardDistributionRandom[8];
  std::discrete_distribution<> venusSpiritTypeRandom[8];

  // ��ǰ��Ϸ������
  // ָ��ָֻ��̬���ݣ��൱�ڴ��Int64���ݣ�����Ӱ�������
  UmaData* umaData;

  // ��ǰ��Ϸ�Ŀ���
  // ָ��ָֻ��̬���ݣ��൱�ڴ��Int64���ݣ�����Ӱ�������
  SupportCard* cardData[6];

  //��ʾ���
  bool playerPrint;//�������ʱ����ʾ������Ϣ

  //��Ϸ����:
  //newGame();
  //for (int t = 0; t < TOTAL_TURN; t++)
  //{
  //  if (!isRacing)//����ѵ���غ�
  //  {
  //    randomDistributeCards();
  //    PLAYER_CHOICE;
  //    applyTraining();
  //    checkEventAfterTrain();
  //  }
  //  else//�����غ�
  //  {
  //    randomDistributeCards();//ֻ��stageInTurn�ĳ�1
  //    if(venusAvailableWisdom!=0)//�Ƿ�ʹ��Ů����ǣ�����ʹ�õ�ʱ��ֱ���������߲�
  //    {
  //      PLAYER_CHOICE;
  //    }
  //    applyTraining();//�������ֻ��Ů�񣬲��ɱ��
  //    checkEventAfterTrain();//�����Ӷ�������������ﴦ��
  //  }
  //}
  //finalScore();
  //



  void newGame(std::mt19937_64& rand,
    bool enablePlayerPrint,
    int newUmaId,
    int newCards[6],
    int newZhongMaBlueCount[5],
    int newZhongMaExtraBonus[6]);//������Ϸ�����֡�umaId��������

  bool loadGameFromJson(std::string jsonStr);

  void initRandomGenerators();

  void randomDistributeCards(std::mt19937_64& rand);//������俨�����Ƭ
  void calculateTrainingValue();//��������ѵ���ֱ�Ӷ��٣�������ʧ����

  // ����ѵ����ı仯��������Ϸ����򷵻�false���ұ�֤�����κ��޸�
  // ���У�chosenTrain����ѡ���ѵ����01234�ֱ������������ǣ�5����Ϣ��6�������7�Ǳ�����
  // useVenus�Ǽ���Ů���������Ƿ���Ů��
  // chosenSpiritColor�Ǽ������Ů����ѡһ�¼���ѡ�����Ƭ��ɫ�������Ʒֱ�012
  // chosenOutgoing����������ѡ��������Ŀ�������������ֱ���01234����ͨ�����5��
  //ע����ͨ�غ���14�ֿ��ܣ�5��ѵ��������һ��ѵ�����ܻ����Ů����ѡһ��������������Ϣ��������5�ֳ��У��������غ�ֻ�п�����Ů������ѡ��
  // forceThreeChoicesEvent��ǿ���ٻ���ѡһ�¼���1Ϊǿ���ٻ���-1Ϊǿ�Ʋ��ٻ���0Ϊ�������������ٻ����������ý�����ai����
  bool applyTraining(
    std::mt19937_64& rand, 
    int chosenTrain, 
    bool useVenus, 
    int chosenSpiritColor, 
    int chosenOutgoing,
    int forceThreeChoicesEvent = 0);
  void checkEventAfterTrain(std::mt19937_64& rand);//���̶��¼�������¼�����������һ���غ�

  void applyTrainingAndNextTurn(
    std::mt19937_64& rand,
    int chosenTrain,
    bool useVenus,
    int chosenSpiritColor,
    int chosenOutgoing,
    int forceThreeChoicesEvent = 0);//һֱ������У�ֱ����һ����Ҫ��Ҿ���

  int finalScore() const;//�����ܷ�
  bool isEnd() const;//

  //��������
  void activateVenusWisdom();//ʹ��Ů�����
  int getTrainingLevel(int item) const;//����ѵ���ȼ�����0��ʼ����Ϸ���k����������k-1������Ů����5��
  bool isOutgoingLegal(int chosenOutgoing) const;//�������Ƿ��ǿ��Խ��е�
  bool isXiaHeSu() const;//�Ƿ�Ϊ�ĺ���
  double getThreeChoicesEventProb(bool useVenusIfFull) const;//�����Ů����¼��ĸ���
  //void runTestGame();

  void getNNInputV1(float* buf, float targetScore, int mode) const;//���������룬mode=0��value��1��policy
  void print() const;//�ò�ɫ������ʾ��Ϸ����
  float getSkillScore() const;//���ܷ֣�����������֮ǰҲ������ǰ��ȥ
  void printFinalStats() const;//��ʾ���ս��

  void addStatus(int idx, int value);//��������ֵ�����������
  void addAllStatus(int value);//�����������ֵ
  void addVital(int value);//�������������������
  void addMotivation(int value);//��������
  void addJiBan(int idx,int value);//����������ǰ���
  void addTrainingLevelCount(int item, int value);//����ѵ���ȼ�������ÿ12Ϊ1����ѵ��+2����Ƭ+1���������
  void addSpirit(std::mt19937_64& rand, int s);//�����Ƭ
  void clearSpirit();//�����Ƭ
  int calculateFailureRate(int trainType) const;//����ѵ��ʧ����
  void calculateVenusSpiritsBonus();//������Ƭ�ӳ�  
  std::array<int,6> calculateBlueVenusBonus(int trainType) const;//���㿪��Ů��ļӳ�
  void runRace(int basicFiveStatusBonus, int basicPtBonus);//�ѱ��������ӵ����Ժ�pt�ϣ������ǲ�������ӳɵĻ���ֵ


  //һЩ���ڸ��ӵ��¼���������
  void handleVenusOutgoing(int chosenOutgoing);//Ů�����
  void handleVenusThreeChoicesEvent(std::mt19937_64& rand, int chosenColor);//Ů����ѡһ�¼�

  //��ʾ�¼�
  void printEvents(std::string s) const;//����ɫ������ʾ�¼�

  void calculateTrainingValueSingle(int trainType);//����ÿ��ѵ���Ӷ���
};

