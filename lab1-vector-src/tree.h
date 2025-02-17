#ifndef __BPLUS_TREE_H__
#define __BPLUS_TREE_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <tuple>
#include <thread>

#define BPT_ORDER 4  // B+树阶数（每个节点最大子节点数）

class BPlusTree {
public:
    class Node {
    public:
        bool leaf; // 是否为叶子节点
        Node* parent; // 父节点
        std::vector<uint64_t> keys; // 存储的键值
        std::vector<Node*> children; // 内部节点的子指针（仅内部节点使用）
        std::vector<void*> values; // 叶子节点存储数据（仅叶子节点使用）
        std::vector<size_t> valbytes; // 存储每个 values[i] 的字节大小

        size_t valcnt;
        Node* next; // 叶子节点的链表指针（仅叶子节点使用）

        Node(bool leaf);
        uint32_t Distance(Node* node); // 计算数据距离
    };

    BPlusTree();
    void Insert(uint64_t key, void* value, size_t valbytes); // 插入数据
    Node* Find(uint64_t key); // 查找 key
    Node* Next(Node* node); // 获取下一个叶子节点

private:
    Node* root; // B+树的根节点

    // 内部辅助函数
    Node* FindLeaf(uint64_t key); // 查找 key 所在的叶子节点
    void InsertIntoLeaf(Node* leaf, uint64_t key, void* value, size_t valbytes); // 插入叶子节点
    void SplitLeaf(Node* leaf); // 叶子节点分裂
    void InsertIntoParent(Node* left, uint64_t key, Node* right); // 插入父节点
    void SplitInternal(Node* node); // 内部节点分裂
};

// 并行查询函数
void traverse_queries(BPlusTree* tree, std::vector<std::tuple<uint64_t, uint64_t>> queries, int tid, int threadcnt, uint64_t* ret);

#endif // __BPLUS_TREE_H__
