#pragma once
//֧Ԯ����ѵ��Ч��
//�Ȱѿ����䵽��Ӧѵ���Ȼ��ż���CardTrainingEffect
struct CardTrainingEffect
{
  double youQing;//����ӳɣ�û���ʾ���0
  double ganJing;//�ɾ��ӳ�
  double xunLian;//ѵ���ӳ�
  double bonus[6];//����������pt�ļӳ�
  int vitalBonus;//�����ظ�������Ҫ���ǲ�Ȧ��
  double failRateDrop; //ʧ���ʽ���
  double vitalCostDrop; //���������½�



  //int initialBonus[6];//��������������pt������
  //int initialJiBan;//��ʼ�
  //double saiHou;//����
  //int hintBonus[6];//Ϊ�˼򻯣��Ѻ��ļ��ܵ�Ч�ɶ�������
  //double hintProbIncrease;//��������������
  //double deYiLv;//������
};
