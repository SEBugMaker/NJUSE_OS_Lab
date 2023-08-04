#include "iostream"
#include "string"
#include "vector"
#include <cstring>

#define FIND_ERR    "File Not Found!\n"
#define OPEN_ERR    "File Cannot be opened!\n"
#define PARAM_ERR   "Parameter Error!\n"
#define COMMAND_ERR "Command Error!\n"

using namespace std;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

int BytsPerSec;				//每扇区字节数
int SecPerClus;				//每簇扇区数
int RsvdSecCnt;				//Boot记录占用的扇区数
int NumFATs;				//FAT表个数
int RootEntCnt;				//根目录最大文件数
int FATSz;					//FAT扇区数

extern "C" {
    // 用C规则编译指定的代码
    void asm_print(const char *, const int, const int);
}

void myPrint(const char *s, const int type) {
    asm_print(s, strlen(s), type);
}

// C++中string类没有提供split功能
vector<string> split(const string& str, const string& delim) {
    vector <string> res;
    if ("" == str) return res;
    // 先将要切割的字符串从string类型转换为char*类型
    char *strs = new char[str.length() + 1]; 
    strcpy(strs, str.c_str());

    char *d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    // 第一次调用时，strtok()必需给予参数s字符串，往后的调用则将参数s设置成NULL
    char *p = strtok(strs, d);
    while (p) {
        // 分割得到的字符串转换为string类型
        string s = p; 
        res.push_back(s); // 存入结果数组
        p = strtok(NULL, d);
    }

    return res;
}


//FAT1的偏移字节
int fatBase;
int fileRootBase;
int dataBase;
int BytsPerClus;
#pragma pack(1) /*指定按1字节对齐*/

class Node {
    string name;
    vector<Node*> next;
    string path;
    u32 FileSize;
    bool isFile = false;
    bool isVal = true;
    int dir_count = 0;
    int file_count = 0;
    char *content = new char[10000];
public:
    Node() = default;

    Node(string name, bool isVal);

    Node(string name, string path);

    Node(string name, u32 fileSize, bool isFile, string path);

    void setName(string name);
    
    void setPath(string path);

    void addChild(Node *child);

    void addFileChild(Node *child);

    void addDirChild(Node *child);

    string getName(); 

    string getPath();

    char* getContent();

    bool getIsFile();

    vector<Node*> getNext();

    bool getIsVal();

    u32 getFileSize();
};

Node::Node(string name, string path) {
    this->name = name;
    this->path = path;
}

Node::Node(string name, bool isVal) {
    this->name = name;
    this->isVal = isVal;
}

Node::Node(string name, u32 fileSize, bool isFile, string path) {
    this->name = name;
    this->FileSize = fileSize;
    this->isFile = isFile;
    this->path = path;
}

void Node::setName(string name){
    this->name = name;
}

void Node::setPath(string path){
    this->path = path;
}

void Node::addChild(Node *child) {
    this->next.push_back(child);
}

void Node::addFileChild(Node *child) {
    this->next.push_back(child);
    this->file_count++;
}

void Node::addDirChild(Node *child) {
    child->addChild(new Node(".", false));
    child->addChild(new Node("..", false));
    this->next.push_back(child);
    this->dir_count++;
}

string Node::getName(){
    return name;
}

string Node::getPath(){
    return this->path;
}

char* Node::getContent(){
    return content;
}

bool Node::getIsFile(){
    return isFile;
}

vector<Node*> Node::getNext(){
    return next;
}

bool Node::getIsVal(){
    return isVal;
}

u32 Node::getFileSize(){
    return FileSize;
}

class BPB
{
    u16 BPB_BytsPerSec; //每扇区字节数
    u8 BPB_SecPerClus;  //每簇扇区数
    u16 BPB_RsvdSecCnt; //Boot记录占用的扇区数
    u8 BPB_NumFATs;		//FAT表个数
    u16 BPB_RootEntCnt; //根目录最大文件数
    u16 BPB_TotSec16;
    u8 BPB_Media;
    u16 BPB_FATSz16; //FAT扇区数
    u16 BPB_SecPerTrk;
    u16 BPB_NumHeads;
    u32 BPB_HiddSec;
    u32 BPB_TotSec32; //如果BPB_FATSz16为0，该值为FAT扇区数
public:
    BPB() {};

    void init(FILE *fat12);
};

void BPB::init(FILE *fat12) {
    fseek(fat12, 11, SEEK_SET);   //BPB从偏移11个字节处开始
    fread(this, 1, 25, fat12); //BPB长度为25字节

    BytsPerSec = this->BPB_BytsPerSec; //初始化各个全局变量
    SecPerClus = this->BPB_SecPerClus;
    RsvdSecCnt = this->BPB_RsvdSecCnt;
    NumFATs = this->BPB_NumFATs;
    RootEntCnt = this->BPB_RootEntCnt;

    if (this->BPB_FATSz16 != 0){
        FATSz = this->BPB_FATSz16;
    }
    else{
        FATSz = this->BPB_TotSec32;
    }
    // ↓计算每簇的字节数，每个区域的初始位置的偏移量
    // 每簇字节数=每簇扇区数*每扇区字节数
    BytsPerClus = SecPerClus * BytsPerSec;
    // fatBase=Boot记录占用的扇区数*每扇区字节数
    fatBase = RsvdSecCnt * BytsPerSec; 
    // fileRootBase=（Boot记录占用的扇区数+FAT表个数* FAT扇区数）*每扇区字节数
    fileRootBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec; //根目录首字节的偏移数=boot+fat1&2的总字节数
    // dataBase=（Boot记录占用的扇区数+FAT表个数* FAT扇区数+（根目录最大文件数* 32+每扇区字节数-1）/每扇区字节数）*每扇区字节数
    dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
}
//BPB结束，长度25字节

//根目录条目
class RootEntry
{
    char DIR_Name[11];
    u8 DIR_Attr; //文件属性
    char reserved[10];
    u16 DIR_WrtTime;
    u16 DIR_WrtDate;
    u16 DIR_FstClus; //开始簇号
    u32 DIR_FileSize;
public:
    RootEntry() {};

    void initRootEntry(FILE *fat12, Node *root);

    bool isValidNameAt(int i);

    bool isEmptyName();

    bool isInvalidName();

    bool isFile();

    void generateFileName(char name[12]);

    void generateDirName(char name[12]);

    u32 getFileSize();

    u16 getFstClus() { return DIR_FstClus; }
};

int getFATValue(FILE *fat12, int num) {
    int base = RsvdSecCnt * BytsPerSec;
    // 从fatBase + num * 3 / 2读取2个字节（16位）。
    // 结合存储的小尾顺序和FAT项结构可以得到。num为偶去掉高4位,num为奇去掉低4位。
    int pos = base + num * 3 / 2;
    int type = num % 2;

    u16 bytes;
    u16 *bytesPtr = &bytes;
    fseek(fat12, pos, SEEK_SET);
    fread(bytesPtr, 1, 2, fat12);

    if (type == 0) {
        bytes = bytes << 4;
    }
    return bytes >> 4;
}

void RetrieveContent(FILE *fat12, int startClus, Node *child) {
    // dataBase=（Boot记录占用的扇区数+FAT表个数* FAT扇区数+（根目录最大文件数* 32+每扇区字节数-1）/每扇区字节数）*每扇区字节数
    int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    int currentClus = startClus;
    int value = 0;
    char *pointer = child->getContent();

    if (startClus == 0) return;

    while (value < 0xFF8) {
        value = getFATValue(fat12, currentClus);
        if (value == 0xFF7) {
            myPrint("Error!", 0);
            break;
        }

        int size = SecPerClus * BytsPerSec;
        char *str = (char*)malloc(size);
        char *content = str;
        int startByte = base + (currentClus - 2)*SecPerClus*BytsPerSec;    // 用FAT号计算在数据区中的位置。

        fseek(fat12, startByte, SEEK_SET);
        fread(content, 1, size, fat12);

        for (int i = 0; i < size; ++i) {
            *pointer = content[i];
            pointer++;
        }
        free(str);
        currentClus = value;
    }
}


void readChildren(FILE *fat12, int startClus, Node *root) {

    int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);

    int currentClus = startClus;
    int value = 0;
    // 0xFF8 End; 0xFF7 Bad Cluster
    while (value < 0xFF8) {
        value = getFATValue(fat12, currentClus);
        if (value == 0xFF7) {
            myPrint("Error!", 0);
            break;
        }

        int startByte = base + (currentClus - 2) * SecPerClus * BytsPerSec;

        int size = SecPerClus * BytsPerSec;
        int loop = 0;
        while (loop < size) {
            RootEntry *rootEntry = new RootEntry();
            fseek(fat12, startByte + loop, SEEK_SET);
            fread(rootEntry, 1, 32, fat12);

            loop += 32;

            if (rootEntry->isEmptyName() || rootEntry->isInvalidName()) {
                continue;
            }

            char tmpName[12];
            if ((rootEntry->isFile())) {
                rootEntry->generateFileName(tmpName);
                Node *child = new Node(tmpName, rootEntry->getFileSize(), true, root->getPath());
                root->addFileChild(child);
                RetrieveContent(fat12, rootEntry->getFstClus(), child);
            } else {
                rootEntry->generateDirName(tmpName);
                Node *child = new Node();
                child->setName(tmpName);
                child->setPath(root->getPath() + tmpName + "/");
                root->addDirChild(child);
                readChildren(fat12, rootEntry->getFstClus(), child);
            }
        }
    }
}


void RootEntry::initRootEntry(FILE *fat12, Node *root) {
    int base = fileRootBase;
    char realName[12];

    for (int i = 0; i < RootEntCnt; ++i) {
        fseek(fat12, base, SEEK_SET);
        fread(this, 1, 32, fat12);

        base += 32;

        if (isEmptyName() || isInvalidName()) 
            continue;

        if (this->isFile()) {
            generateFileName(realName);
            Node *child = new Node(realName, this->DIR_FileSize, true, root->getPath());
            root->addFileChild(child);
            RetrieveContent(fat12, this->DIR_FstClus, child);
        } else {
            generateDirName(realName);
            Node *child = new Node();
            child->setName(realName);
            child->setPath(root->getPath() + realName + "/");
            root->addDirChild(child);
            readChildren(fat12, this->getFstClus(), child);
        }
    }
}

bool RootEntry::isValidNameAt(int j) {
    return    ((this->DIR_Name[j] >= 'a') && (this->DIR_Name[j] <= 'z'))
            ||((this->DIR_Name[j] >= 'A') && (this->DIR_Name[j] <= 'Z'))
            ||((this->DIR_Name[j] >= '0') && (this->DIR_Name[j] <= '9'))
            ||((this->DIR_Name[j] == ' '));
}

bool RootEntry::isEmptyName() {
    return this->DIR_Name[0] == '\0';
}

bool RootEntry::isInvalidName() {
    int invalid = false;
    for (int k = 0; k < 11; ++k) {
        if (!this->isValidNameAt(k)) {
            invalid = true;
            break;
        }
    }
    return invalid;
}

bool RootEntry::isFile() {
    // 用DIR_Attr&0x10判断，结果为0是文件，否则为文件夹
    return (this->DIR_Attr & 0x10) == 0;
}

void RootEntry::generateFileName(char name[12]) {
    int tmp = -1;
    for (int j = 0; j < 11; j += 1) {
        if (this->DIR_Name[j] != ' ') {
            tmp += 1;
            name[tmp] = this->DIR_Name[j];
        } else {
            tmp += 1;
            name[tmp] = '.';
            while (this->DIR_Name[j] == ' ') {
                j += 1;
            }
            j -= 1;
        }
    }
    tmp += 1;
    name[tmp] = '\0';
}

void RootEntry::generateDirName(char *name) {
    int tmp = -1;
    for (int k = 0; k < 11; ++k) {
        if (this->DIR_Name[k] != ' ') {
            tmp += 1;
            name[tmp] = this->DIR_Name[k];
        } else {
            tmp += 1;
            name[tmp] = '\0';
            break;
        }
    }
}

u32 RootEntry::getFileSize() {
    return DIR_FileSize;
}

void outputCat(Node *root, string p, int &flag) {
    if(p[0] != '/'){
        p = "/" + p;
    }
    // 在根目录下
    if (p == root->getPath() + root->getName()) {
        if (root->getIsFile()) {
            flag = 1;
            if (root->getContent()[0] != 0) {
                myPrint(root->getContent(), 0);
                myPrint("\n", 0);
            }
            return;
        } else {
            flag = 2;
            return;
        }

    }
    // 空 or /
    if (p.length() <= root->getPath().length()) {
        return;
    }
    string tmp = p.substr(0, root->getPath().length());
    if (tmp == root->getPath()) {
        for (Node *q : root->getNext()) {
            outputCat(q, p, flag);
        }
    }
}

vector<string> handlePath(vector<string> tmp){
    vector<string> dirs;
    int doubleDots = 0;
    while (!tmp.empty())
    {
        if(tmp[tmp.size()-1] == ".."){
            doubleDots += 1;
            if (!tmp.empty()){
                tmp.pop_back();   
            }       
            while(tmp[tmp.size()-1] == "." || tmp[tmp.size()-1] == ".."){
                if(tmp[tmp.size()-1] == "."){
                        if (!tmp.empty()){
                        tmp.pop_back();   
                        }
                }else{
                    doubleDots += 1;
                    if (!tmp.empty()){
                        tmp.pop_back();   
                    }		
                }
            }
            while(doubleDots != 0){
                if (!tmp.empty()){
                    tmp.pop_back();   
                }
                doubleDots -= 1;
            }
            continue;
        }else if(tmp[tmp.size()-1] == "."){
            tmp.pop_back();
            continue;
        }else{
            dirs.push_back(tmp[tmp.size() - 1]);
            tmp.pop_back();
        }
    }
    return dirs;
}

void handleCAT(vector<string> commands, Node* root) {
    if (commands.size() == 2 && commands[1][0] != '-') {
        int flag = 0;
        string path = commands[1];
        vector<string> tmp = split(path,"/");
        vector<string> dirs = handlePath(tmp);
        path = "";
        for(int i=dirs.size()-1;i>=0;i--){
            path += dirs[i];
            if(i != 0){
                path += "/";
            }
        }
        outputCat(root, path, flag);
        if (flag == 0) {
            myPrint(FIND_ERR, 0);
        } else if (flag == 2) {
            myPrint(OPEN_ERR, 0);
        }
    } else {
        myPrint(PARAM_ERR, 0);
    }
}

void outputLS(Node *r) {
    string str;
    Node *p = r; 
    if (p->getIsFile()) {
        return;
    }
    else {
        str = p->getPath() + ":\n";
        const char *strPtr = str.c_str();
        myPrint(strPtr, 0);
        str.clear();
        // 打印每个next
        Node *q;
        int len = p->getNext().size();
        for (int i = 0; i < len; i++) {
            q = p->getNext()[i];
            if (!q->getIsFile()) {
                myPrint(q->getName().c_str(), 1);
                myPrint(" ", 0);
            } else {
                myPrint(q->getName().c_str(), 0);
                myPrint(" ",0);
            }
        }
        myPrint("\n", 0);
        for (int i = 0; i < len; ++i) {
            if (p->getNext()[i]->getIsVal()) {
                outputLS(p->getNext()[i]);
            }
        }
    }
}

void outputLSL(Node *r) {
    Node *p = r;
    if (p->getIsFile()) {
        return;
    }
    else {
        // 路径为文件夹
        int fileNum = 0;
        int dirNum = 0;
        for (int j = 0; j < p->getNext().size(); ++j) {
            if (p->getNext()[j]->getName() == "." || p->getNext()[j]->getName() == "..") {
                continue;
            }
            if (p->getNext()[j]->getIsFile()) {
                fileNum += 1;
            } else {
                dirNum += 1;
            }
        }

        myPrint(p->getPath().c_str(), 0);
        myPrint(" ", 0);
        myPrint(to_string(dirNum).c_str(), 0);
        myPrint(" ", 0);
        myPrint(to_string(fileNum).c_str(), 0);
        myPrint("\n", 0);

        //打印每个next
        Node *q;
        int len = p->getNext().size();
        for (int i = 0; i < len; i++) {
            q = p->getNext()[i];
            if (!q->getIsFile()) {
                if (q->getName() == "." || q->getName() == "..") {
                    myPrint(q->getName().c_str(), 1);
                    myPrint(" \n", 0);
                } else {
                    fileNum = 0;
                    dirNum = 0;
                    for (int j = 2; j < q->getNext().size(); ++j) {
                        if (q->getNext()[j]->getIsFile()) {
                            fileNum += 1;
                        } else {
                            dirNum += 1;
                        }
                    }
                    myPrint(q->getName().c_str(), 1);
                    myPrint(" ", 0);
                    myPrint(to_string(dirNum).c_str(), 0);
                    myPrint(" ", 0);
                    myPrint(to_string(fileNum).c_str(), 0);
                    myPrint("\n", 0);
                }
            } else {
                myPrint(q->getName().c_str(), 0);
                myPrint(" ",0);
                myPrint(to_string(q->getFileSize()).c_str(), 0);
                myPrint("\n",0);
            }
        }
        myPrint("\n", 0);
        for (int i = 0; i < len; ++i) {
            if (p->getNext()[i]->getIsVal()) {
                outputLSL(p->getNext()[i]);
            }
        }
    }
}

bool isL(string &s) {
    if (s[0] != '-') {
        return false;
    } else {
        // -ll 等同于 -l
        for (int i=1; i<s.size(); i++) {
            if (s[i] != 'l') return false;
        }
    }
    return true;
}

Node* findByName(Node *root, vector<string> dirs) {
    // 广度搜索
    if (dirs.empty()) return root;
    string name = dirs[dirs.size()-1];
    for (int i=0; i<root->getNext().size(); i++) {
        if (name == root->getNext()[i]->getName()) {
            dirs.pop_back();
            return findByName(root->getNext()[i], dirs);
        }
    }
    return nullptr;
}

Node* isDir(string &s, Node *root) {
    vector<string> dirs = handlePath(split(s,"/"));
    return findByName(root, dirs);
}

void handleLS(vector<string> commands, Node* root) {
    if (commands.size() == 1) {
        outputLS(root);
    } else {
        // handle -l
        bool hasL = false;
        bool hasDir = false;
        Node *toFind = root;
        for (int i=1; i<commands.size(); i++) {
            Node* newRoot = isDir(commands[i], root);
            if (isL(commands[i])) {
                hasL = true;
            } else if (!hasDir && newRoot != nullptr) {
                hasDir = true;
                toFind = newRoot;
            } else {
                myPrint(PARAM_ERR, 0);
                return;
            }
        }
        if (hasL) {
            outputLSL(toFind);
        }else{
            outputLS(toFind);
	}

    }
}

int main()
{
    FILE *fat12;
    fat12 = fopen("/home/suhao/hw02/os.img", "rb"); //打开FAT12的映像文件   

    BPB *bpb = new BPB();
    bpb->init(fat12);
    Node *root = new Node();
    root->setName("");
    root->setPath("/");
    RootEntry *rootEntry = new RootEntry();
    rootEntry->initRootEntry(fat12, root);
    while (true) {
        myPrint(">", 0);
        string input;
        getline(cin, input);
        vector<string> commands = split(input, " ");
        if (commands[0] == "exit") {
            myPrint("Program's Done!", 0);
            fclose(fat12);
            break;
        } else if (commands[0] == "cat") {
            handleCAT(commands, root);
        } else if (commands[0] == "ls") {
            handleLS(commands, root);
        } else {
            myPrint(COMMAND_ERR, 0);
            continue;
        }
    }

}
