#define MAXLINE 1000
#define MAXINPUT 700000

typedef struct Node {
    int port;
    struct Node* left;
    struct Node * right; 
} PrefixTree;

typedef struct Node2{
    int port;
    struct Node2* son00;
    struct Node2* son01;
    struct Node2* son10;
    struct Node2* son11;
} PrefixTree2;

typedef struct line{
    unsigned int ip;
    int prefix_len;
    int port;
} Info;

PrefixTree * root;
PrefixTree2 * root2;

PrefixTree * init_tree();
PrefixTree2 * init_tree_2bit();
int insert_info_2bit(Info * new_info);
int insert_info (Info* new_info);
int lookup (int ip);
int lookup_2bit (int ip);
