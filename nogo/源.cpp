#pragma GCC optimize(2)
#pragma GCC optimize(3)//开启o2,o3优化加快编译速度。O2会尝试更多的寄存器级的优化以及指令级的优化，它会在编译期间占用更多的内存和编译时间 ，O3在O2的基础上进行更多的优化，例如使用伪寄存器网络，普通函数的内联，以及针对循环的更多优化。
#pragma GCC optimize("Ofast,no-stack-protector")//编译器对函数的栈会形成三级保护，这种保护会占用一定内存，这行代码会关闭保护省去这些操作，能提高一定性能。

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
#define EXPLORE 0.1//ucb公式参数


int my_board[9][9] = { 0 };
int my_node_count = 0;

inline bool inBorder(int x, int y) {//判断是否在棋盘中
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
    //Node的棋盘状态记录：包括评估值current_value,盘面current_board
    //实现有判断是否达terminal；simulation阶段的单步default policy
public:
    Node();
    signed char current_board[9][9] = { 0 };
    int col = 0;//棋子颜色
    Node* parent = NULL;
    Node* children[81];
    int visit_times = 0;//被访问次数
     
    int countChildrenNum = 0;
    int maxChildrenNum = 0;
    double quality_value = 0.0;
    int available_choices[81];
    void my_getAviliableAction();           //得到可行的行动
    bool my_dfsAir(int fx, int fy);         //判断是否有气
    bool my_judgeAvailable(int fx, int fy); //判断是否可下
    double my_quickEvaluate();              //快速估胜率
    Node* bestChild(double C);
    Node* expand();
    Node* my_treePolicy();
    double my_defaultPolicy();
    void backup(double reward);
};
Node::Node()//构造函数，同时置零完成初始化
{
    memset(current_board, 0, sizeof(current_board));//将指针变量 current_board所指向的前 sizeof(current_board) 字节的内存单元用一个“整数” 0替换，注意 0 是 int 型。available_choices是 void* 型的指针变量，所以它可以为任何类型的数据进行初始化。
    memset(available_choices, 0, sizeof(available_choices));//将指针变量 available_choices,d所指向的前 sizeof(current_board) 字节的内存单元用一个“整数” 0替换，
}

//蒙特卡洛树搜索的Backpropagation阶段，输入前面获取需要expend的节点和新执行Action的reward，反馈给expend节点和上游所有节点并更新对应数据。
void Node::backup(double reward)//记录选当前子节点胜率
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

double Node::my_quickEvaluate()//快速估胜率
{
    int n1 = 0, n2 = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
        {
            bool f1 = my_judgeAvailable(i, j);//判断该点下当前颜色合法，假设为当前为黑色
            col = -col;//改变颜色
            bool f2 = my_judgeAvailable(i, j);
            col = -col;//操作完成后还原改变的颜色
            if (f1 && !f2)//可黑不可白
                n1++;
            else if (!f1 && f2)//可白不可黑
                n2++;
        }
    return n2 - n1;//当前棋盘可下黑白棋的差距，根据不围棋的规则就相当于获胜的概率
}

bool Node::my_dfsAir(int fx, int fy)//判断是否有气
{
    dfs_air_visit[fx][fy] = true;
    bool flag = false;//默认没气
    for (int dir = 0; dir < 4; dir++) //某一位置有气的条件为周围有空格或者有一个邻近的同色有气(反证法进一步证明邻近的有一个有气即都有气)
    {
        int dx = fx + cx[dir], dy = fy + cy[dir];
        if (inBorder(dx, dy))//判断是否落子是否在棋盘中，在棋盘中进入判断
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

bool Node::my_judgeAvailable(int fx, int fy)//判断是否若是在这块下是否合法
{
    if (current_board[fx][fy])//还未下棋直接返回
        return false;
    current_board[fx][fy] = col;
    memset(dfs_air_visit, 0, sizeof(dfs_air_visit));
    if (!my_dfsAir(fx, fy)) //判定自杀
    {
        current_board[fx][fy] = 0;
        return false;
    }
    for (int dir = 0; dir < 4; dir++) //不围对方
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

//Simulation阶段，从当前节点快速落子模拟运算至终局，返回reward
double Node::my_defaultPolicy()
{
    return my_quickEvaluate();
}

void Node::my_getAviliableAction()//得到可行的活动
{
    memset(available_choices, 0, sizeof(available_choices));//将指针变量 available_choices所指向的前 sizeof(available_choices) 字节的内存单元用一个“整数” 0替换，注意 0 是 int 型。available_choices是 void* 型的指针变量，所以它可以为任何类型的数据进行初始化。
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (my_judgeAvailable(i, j))//遍历棋盘每个点,,判断是否合法
            {
                int act = i * 9 + j;//计算已经遍历过的点的个数
                available_choices[maxChildrenNum++] = act;//该点最大可选择的点数量为目前已经遍历过的点的个数      
            }
}

Node* Node::my_treePolicy()
{
    //Selection与Expansion阶段。传入当前需要开始搜索的节点，根据UCB1值返回最好的需要expend的节点，注意如果节点是叶子结点直接返回。
    //基本策略是先找当前未选择过的子节点，如果有多个则随机选。如果都选择过就找权衡过exploration/exploitation的UCB值最大的，如果UCB值相等则随机选。
    if (maxChildrenNum == 0) //当treePolicy到达叶子 节点时(node->state.available_choices.empty() && node->children.empty())
    {
        return this;
    }
    if (countChildrenNum >= maxChildrenNum)
    {
        Node* p = bestChild(EXPLORE);//当前最佳的子节点
        return p->my_treePolicy();
    }
    else//countChildrenNum < maxChildrenNum
        return expand();
}

Node* Node::expand()//扩展下一个节点
{
    int a = available_choices[countChildrenNum];
    int x = a / 9;
    int y = a % 9;
    Node* new_node = new Node;
    children[countChildrenNum++] = new_node;
    new_node->parent = this;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            new_node->current_board[i][j] = current_board[i][j];//更新该点棋盘状态
    new_node->col = -col;//转变下棋方
    new_node->current_board[x][y] = col;
    new_node->my_getAviliableAction();
    return new_node;
}

//通过ucb公式计算,返回ucb值最大的子节点,通过权衡exploration和exploitation(这是强化学习里面强调的两个核心问题,网址:https://zhuanlan.zhihu.com/p/410556975)后选择得分最高的子节点
Node* Node::bestChild(double C)
{
    double max_score = -2e50;        
    Node* best_child = NULL;
    for (int i = 0; i < countChildrenNum; i++)
    {
        Node* p = children[i];
        double score = p->quality_value / (p->visit_times) + 2 * C * sqrt(log(2 * visit_times) / (p->visit_times));//运用ucb公式计算下一步下在该子节点的ucb值,ucb公式介绍:https://blog.csdn.net/songyunli1111/article/details/83384738
        if (score > max_score)
        {
            max_score = score;
            best_child = p;
        }
    }
    return best_child;//返回ucb值最大的子节点
}

/*
代码没有设置界面，通过实现接受botzone网站的数据进行建设。
借用c中的json库实现与botzone平台交互，使得代码得以实现。
实现蒙特卡洛树搜索算法，传入一个根节点，
在有限的时间内根据之前已经探索过的树结构expand新节点和更新数据，
然后返回只要exploitation最高的子节点。
蒙特卡洛树搜索包含四个步骤，Selection、Expansion、Simulation、Backpropagation。
前两步使用tree policy找到值得探索的节点。
第三步使用default policy也就是在选中的节点上随机算法选一个子节点并计算reward。
最后一步使用backup也就是把reward更新到所有经过的选中节点的节点上。
进行预测时，只需要根据Q值选择exploitation最大的节点即可，找到下一个最优的节点。
*/



int main()
{
	srand((unsigned)time(0));
	string str;
	int x, y;
    ofstream outfile("E:\\nogo\\NoGoAIForBotzone-master\\nogo_best\\nogo.txt");
	getline(cin, str);//接受botzone网站传输的棋局数据，储存到str中
	//开始计时并计算时间
	int start = clock();
	int timeout = (int)(TIME_OUT_SET * (double)CLOCKS_PER_SEC);//0.98*0.1

	Json::Reader reader;
	Json::Value my_input;
	reader.parse(str, my_input);//借用json库中的parse函数将接受都字符串类型数据全部转换为 JavaScript 对象，存到input中

	int first_node = 0;
    Node* node = new Node;
    if (my_input["requests"][first_node]["x"].asInt() == -1)//确定下一步棋子的颜色，跟上一步棋子颜色取反
        node->col = 1;//col为1，则为黑色，col为-1，为白色
    else
        node->col = -1;

    int color = node->col;
    //分析自己收到的输入和自己过往的输出，并恢复状态
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
    if (x != -1) //对方为黑子
    {
        my_board[x][y] = -color;
        my_node_count++;
    }

    for (int i = 0; i < 9; i++)//载入目前已经有的棋盘状态
    {
        for (int j = 0; j < 9; j++) {
            node->current_board[i][j] = my_board[i][j];
        }
    }
    node->my_getAviliableAction();

    //在时间运行的实践内不停搜索，不断加深搜索的深度和宽度
    while (clock()-start<timeout)
    {
        my_node_count++;
        Node* expand_node = node->my_treePolicy();
        double reward = expand_node->my_defaultPolicy();//计算下载该点胜率
        expand_node->backup(reward);
    }

    //输出
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
    Json::FastWriter my_writer;//输出快速字符串型
    char buffer[4096];
    sprintf(buffer, "已经下过的棋数:%d", my_node_count);
    my_ret["debug"] = buffer;
    outfile <<"response"<< endl; // Bot 此回合的输出信息（response）
    outfile<< my_action;
    outfile <<"debug" << endl;
    outfile << buffer<<endl;// 调试信息，将被写入log，最大长度为1KB
    outfile << "data"  << endl;   // Bot 此回合的保存信息，将在下回合输入【注意不会保留在 Log 中】
    outfile << my_ret["data"] << endl;
    outfile << "globaldata" << endl;
    outfile << my_ret["globaldata"] << endl;
    cout << my_writer.write(my_ret) << endl;
    // Bot 的全局保存信息，将会在下回合输入，对局结束后也会保留，下次对局可以继续利用【注意不会保留在 Log 中】
    outfile.close();
    return 0;
}