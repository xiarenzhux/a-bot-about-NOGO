#pragma GCC optimize(2)
#pragma GCC optimize(3)//����o2,o3�Ż��ӿ�����ٶȡ�O2�᳢�Ը���ļĴ��������Ż��Լ�ָ����Ż��������ڱ����ڼ�ռ�ø�����ڴ�ͱ���ʱ�� ��O3��O2�Ļ����Ͻ��и�����Ż�������ʹ��α�Ĵ������磬��ͨ�������������Լ����ѭ���ĸ����Ż���
#pragma GCC optimize("Ofast,no-stack-protector")//�������Ժ�����ջ���γ��������������ֱ�����ռ��һ���ڴ棬���д����رձ���ʡȥ��Щ�����������һ�����ܡ�

#include "jsoncpp/json.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <fstream>
using namespace std;

#define TIME_OUT_SET 0.98
#define EXPLORE 0.1//ucb��ʽ����


int my_board[9][9] = { 0 };
int my_node_count = 0;

inline bool inBorder(int x, int y) {//�ж��Ƿ���������
    return x >= 0 && y >= 0 && x < 9 && y < 9;
}
bool dfs_air_visit[9][9] = { 0 };
const int cx[] = { -1, 0, 1, 0 };
const int cy[] = { 0, -1, 0, 1 };

struct Action
{
    int x = -1;
    int y = -1;
};

class Node {
    //Node������״̬��¼����������ֵcurrent_value,����current_board
    //ʵ�����ж��Ƿ��terminal��simulation�׶εĵ���default policy
public:
    Node();
    signed char current_board[9][9] = { 0 };
    int col = 0;//������ɫ
    Node* parent = NULL;
    Node* children[81];
    int visit_times = 0;//�����ʴ���
     
    int countChildrenNum = 0;
    int maxChildrenNum = 0;
    double quality_value = 0.0;
    int available_choices[81];
    void my_getAviliableAction();           //�õ����е��ж�
    bool my_dfsAir(int fx, int fy);         //�ж��Ƿ�����
    bool my_judgeAvailable(int fx, int fy); //�ж��Ƿ����
    double my_quickEvaluate();              //���ٹ�ʤ��
    Node* bestChild(double C);
    Node* expand();
    Node* my_treePolicy();
    double my_defaultPolicy();
    void backup(double reward);
};
Node::Node()//���캯����ͬʱ������ɳ�ʼ��
{
    memset(current_board, 0, sizeof(current_board));//��ָ����� current_board��ָ���ǰ sizeof(current_board) �ֽڵ��ڴ浥Ԫ��һ���������� 0�滻��ע�� 0 �� int �͡�available_choices�� void* �͵�ָ�����������������Ϊ�κ����͵����ݽ��г�ʼ����
    memset(available_choices, 0, sizeof(available_choices));//��ָ����� available_choices,d��ָ���ǰ sizeof(current_board) �ֽڵ��ڴ浥Ԫ��һ���������� 0�滻��
}

//���ؿ�����������Backpropagation�׶Σ�����ǰ���ȡ��Ҫexpend�Ľڵ����ִ��Action��reward��������expend�ڵ���������нڵ㲢���¶�Ӧ���ݡ�
void Node::backup(double reward)//��¼ѡ��ǰ�ӽڵ�ʤ��
{
    Node* p = this;
    while (p)
    {
        p->visit_times++;
        p->quality_value += reward;
        reward = -reward;
        p = p->parent;
    }
}

double Node::my_quickEvaluate()//���ٹ�ʤ��
{
    int n1 = 0, n2 = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
        {
            bool f1 = my_judgeAvailable(i, j);//�жϸõ��µ�ǰ��ɫ�Ϸ�������Ϊ��ǰΪ��ɫ
            col = -col;//�ı���ɫ
            bool f2 = my_judgeAvailable(i, j);
            col = -col;//������ɺ�ԭ�ı����ɫ
            if (f1 && !f2)//�ɺڲ��ɰ�
                n1++;
            else if (!f1 && f2)//�ɰײ��ɺ�
                n2++;
        }
    return n2 - n1;//��ǰ���̿��ºڰ���Ĳ�࣬���ݲ�Χ��Ĺ�����൱�ڻ�ʤ�ĸ���
}

bool Node::my_dfsAir(int fx, int fy)//�ж��Ƿ�����
{
    dfs_air_visit[fx][fy] = true;
    bool flag = false;//Ĭ��û��
    for (int dir = 0; dir < 4; dir++) //ĳһλ������������Ϊ��Χ�пո������һ���ڽ���ͬɫ����(��֤����һ��֤���ڽ�����һ��������������)
    {
        int dx = fx + cx[dir], dy = fy + cy[dir];
        if (inBorder(dx, dy))//�ж��Ƿ������Ƿ��������У��������н����ж�
        {
            if (current_board[dx][dy] == 0)
                flag = true;
            if (current_board[dx][dy] == current_board[fx][fy] && !dfs_air_visit[dx][dy])
                if (my_dfsAir(dx, dy))
                {
                    flag = true;

                }
        }
    }
    return flag;
}

bool Node::my_judgeAvailable(int fx, int fy)//�ж��Ƿ�������������Ƿ�Ϸ�
{
    if (current_board[fx][fy])//��δ����ֱ�ӷ���
        return false;
    current_board[fx][fy] = col;
    memset(dfs_air_visit, 0, sizeof(dfs_air_visit));
    if (!my_dfsAir(fx, fy)) //�ж���ɱ
    {
        current_board[fx][fy] = 0;
        return false;
    }
    for (int dir = 0; dir < 4; dir++) //��Χ�Է�
    {
        int dx = fx + cx[dir], dy = fy + cy[dir];
        if (inBorder(dx, dy))
        {
            if (current_board[dx][dy] && !dfs_air_visit[dx][dy])
            {
                if (!my_dfsAir(dx, dy))
                {
                    current_board[fx][fy] = 0;
                    return false;
                }
            }
        }
    }
    current_board[fx][fy] = 0;
    return true;
}

//Simulation�׶Σ��ӵ�ǰ�ڵ��������ģ���������վ֣�����reward
double Node::my_defaultPolicy()
{
    return my_quickEvaluate();
}

void Node::my_getAviliableAction()//�õ����еĻ
{
    memset(available_choices, 0, sizeof(available_choices));//��ָ����� available_choices��ָ���ǰ sizeof(available_choices) �ֽڵ��ڴ浥Ԫ��һ���������� 0�滻��ע�� 0 �� int �͡�available_choices�� void* �͵�ָ�����������������Ϊ�κ����͵����ݽ��г�ʼ����
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (my_judgeAvailable(i, j))//��������ÿ����,,�ж��Ƿ�Ϸ�
            {
                int act = i * 9 + j;//�����Ѿ��������ĵ�ĸ���
                available_choices[maxChildrenNum++] = act;//�õ�����ѡ��ĵ�����ΪĿǰ�Ѿ��������ĵ�ĸ���      
            }
}

Node* Node::my_treePolicy()
{
    //Selection��Expansion�׶Ρ����뵱ǰ��Ҫ��ʼ�����Ľڵ㣬����UCB1ֵ������õ���Ҫexpend�Ľڵ㣬ע������ڵ���Ҷ�ӽ��ֱ�ӷ��ء�
    //�������������ҵ�ǰδѡ������ӽڵ㣬����ж�������ѡ�������ѡ�������Ȩ���exploration/exploitation��UCBֵ���ģ����UCBֵ��������ѡ��
    if (maxChildrenNum == 0) //��treePolicy����Ҷ�� �ڵ�ʱ(node->state.available_choices.empty() && node->children.empty())
    {
        return this;
    }
    if (countChildrenNum >= maxChildrenNum)
    {
        Node* p = bestChild(EXPLORE);//��ǰ��ѵ��ӽڵ�
        return p->my_treePolicy();
    }
    else//countChildrenNum < maxChildrenNum
        return expand();
}

Node* Node::expand()//��չ��һ���ڵ�
{
    int a = available_choices[countChildrenNum];
    int x = a / 9;
    int y = a % 9;
    Node* new_node = new Node;
    children[countChildrenNum++] = new_node;
    new_node->parent = this;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            new_node->current_board[i][j] = current_board[i][j];//���¸õ�����״̬
    new_node->col = -col;//ת�����巽
    new_node->current_board[x][y] = col;
    new_node->my_getAviliableAction();
    return new_node;
}

//ͨ��ucb��ʽ����,����ucbֵ�����ӽڵ�,ͨ��Ȩ��exploration��exploitation(����ǿ��ѧϰ����ǿ����������������,��ַ:https://zhuanlan.zhihu.com/p/410556975)��ѡ��÷���ߵ��ӽڵ�
Node* Node::bestChild(double C)
{
    double max_score = -2e50;        
    Node* best_child = NULL;
    for (int i = 0; i < countChildrenNum; i++)
    {
        Node* p = children[i];
        double score = p->quality_value / (p->visit_times) + 2 * C * sqrt(log(2 * visit_times) / (p->visit_times));//����ucb��ʽ������һ�����ڸ��ӽڵ��ucbֵ,ucb��ʽ����:https://blog.csdn.net/songyunli1111/article/details/83384738
        if (score > max_score)
        {
            max_score = score;
            best_child = p;
        }
    }
    return best_child;//����ucbֵ�����ӽڵ�
}

/*
����û�����ý��棬ͨ��ʵ�ֽ���botzone��վ�����ݽ��н��衣
����c�е�json��ʵ����botzoneƽ̨������ʹ�ô������ʵ�֡�
ʵ�����ؿ����������㷨������һ�����ڵ㣬
�����޵�ʱ���ڸ���֮ǰ�Ѿ�̽���������ṹexpand�½ڵ�͸������ݣ�
Ȼ�󷵻�ֻҪexploitation��ߵ��ӽڵ㡣
���ؿ��������������ĸ����裬Selection��Expansion��Simulation��Backpropagation��
ǰ����ʹ��tree policy�ҵ�ֵ��̽���Ľڵ㡣
������ʹ��default policyҲ������ѡ�еĽڵ�������㷨ѡһ���ӽڵ㲢����reward��
���һ��ʹ��backupҲ���ǰ�reward���µ����о�����ѡ�нڵ�Ľڵ��ϡ�
����Ԥ��ʱ��ֻ��Ҫ����Qֵѡ��exploitation���Ľڵ㼴�ɣ��ҵ���һ�����ŵĽڵ㡣
*/



int main()
{
	srand((unsigned)time(0));
	string str;
	int x, y;
    ofstream outfile("E:\\nogo\\NoGoAIForBotzone-master\\nogo_best\\nogo.txt");
	getline(cin, str);//����botzone��վ�����������ݣ����浽str��
	//��ʼ��ʱ������ʱ��
	int start = clock();
	int timeout = (int)(TIME_OUT_SET * (double)CLOCKS_PER_SEC);//0.98*0.1

	Json::Reader reader;
	Json::Value my_input;
	reader.parse(str, my_input);//����json���е�parse���������ܶ��ַ�����������ȫ��ת��Ϊ JavaScript ���󣬴浽input��

	int first_node = 0;
    Node* node = new Node;
    if (my_input["requests"][first_node]["x"].asInt() == -1)//ȷ����һ�����ӵ���ɫ������һ��������ɫȡ��
        node->col = 1;//colΪ1����Ϊ��ɫ��colΪ-1��Ϊ��ɫ
    else
        node->col = -1;

    int color = node->col;
    //�����Լ��յ���������Լ���������������ָ�״̬
    int turnID = my_input["responses"].size();
    for (int i = 0; i < turnID; i++) {
        x = my_input["requests"][i]["x"].asInt(), y = my_input["requests"][i]["y"].asInt();
        if (x != -1)
        {
            my_board[x][y] = -color;
        }
        x = my_input["responses"][i]["x"].asInt(), y = my_input["responses"][i]["y"].asInt();
        if (x != -1)
        {
            my_board[x][y] = color;
        }
    }
    x = my_input["requests"][turnID]["x"].asInt(), y = my_input["requests"][turnID]["y"].asInt();
    if (x != -1) //�Է�Ϊ����
    {
        my_board[x][y] = -color;
        my_node_count++;
    }

    for (int i = 0; i < 9; i++)//����Ŀǰ�Ѿ��е�����״̬
    {
        for (int j = 0; j < 9; j++) {
            node->current_board[i][j] = my_board[i][j];
        }
    }
    node->my_getAviliableAction();

    //��ʱ�����е�ʵ���ڲ�ͣ���������ϼ�����������ȺͿ��
    while (clock()-start<timeout)
    {
        my_node_count++;
        Node* expand_node = node->my_treePolicy();
        double reward = expand_node->my_defaultPolicy();//�������ظõ�ʤ��
        expand_node->backup(reward);
    }

    //���
    Json::Value my_ret;
    Json::Value my_action;

    Node* best_child = node->bestChild(0);
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (my_board[i][j] != best_child->current_board[i][j])
            {
                my_action["x"] = i;
                my_action["y"] = j;
                break;
            }
    my_ret["response"] = my_action;
    Json::FastWriter my_writer;//��������ַ�����
    char buffer[4096];
    sprintf(buffer, "�Ѿ��¹�������:%d", my_node_count);
    my_ret["debug"] = buffer;
    outfile <<"response"<< endl; // Bot �˻غϵ������Ϣ��response��
    outfile<< my_action;
    outfile <<"debug" << endl;
    outfile << buffer<<endl;// ������Ϣ������д��log����󳤶�Ϊ1KB
    outfile << "data"  << endl;   // Bot �˻غϵı�����Ϣ�������»غ����롾ע�ⲻ�ᱣ���� Log �С�
    outfile << my_ret["data"] << endl;
    outfile << "globaldata" << endl;
    outfile << my_ret["globaldata"] << endl;
    cout << my_writer.write(my_ret) << endl;
    // Bot ��ȫ�ֱ�����Ϣ���������»غ����룬�Ծֽ�����Ҳ�ᱣ�����´ζԾֿ��Լ������á�ע�ⲻ�ᱣ���� Log �С�
    outfile.close();
    return 0;
}