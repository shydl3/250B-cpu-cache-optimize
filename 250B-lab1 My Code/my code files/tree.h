#ifndef __MY_TREE_H__
#define __MY_TREE_H__

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <chrono>
#include <vector>
#include <tuple>
#include <thread>
#include <map>
#include <algorithm>

#define BPT_ORDER 256

class MyTree {
    public:
    // MyTree::MyNode is the original BST node obj
    // keep for compatability

    class BPlusNode;

    class MyNode{
        public:
            MyNode(uint64_t key, void* val, size_t valbytes);
            void* val;
            uint64_t key;
            size_t valcnt;
            MyNode* Find();

            MyNode* parent = NULL;
            MyNode* right = NULL;
            MyNode* left = NULL;

            size_t leafIndex;  // the index in the leaf. Updated when split
            BPlusNode* parentLeaf;
    };

    class BPlusNode {
        public:
            bool isLeaf;
            BPlusNode* parent;
            BPlusNode* next;
            BPlusNode* prev;
            
            std::vector<uint64_t> keys; // 存储的键值
            std::vector<BPlusNode*> children; // 内部节点的child指针

            std::vector<MyNode*> values; // leaf node stores the values
            std::vector<size_t> valbytes; // 存储每个 values[i] 的字节大小

            BPlusNode(bool leaf = false): isLeaf(leaf), parent(nullptr), next(nullptr), prev(nullptr) {}
    };


    MyTree();
    void Insert(MyNode* nn);

    BPlusNode* root;
    MyNode* Find(uint64_t key);
    BPlusNode* FindLeaf(uint64_t key);
    MyNode* Next(MyNode* current);

    void InsertIntoLeaf(BPlusNode* leaf, uint64_t key, void* val, size_t valcnt);
    void SplitChild(BPlusNode* parent, int index, BPlusNode* child);
    void insertNonFull(BPlusNode* node, uint64_t key, MyNode* nn);
    void PrintTree();

    uint32_t Distance(void* val0, MyNode* mynode);

};

void traverse_queries(MyTree* tree,
    const std::vector<std::tuple<MyTree::MyNode*, uint64_t>>& queries,
    size_t start_index, size_t end_index,
    uint64_t* ret);

#endif