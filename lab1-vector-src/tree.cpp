#include "tree.h"
#include <algorithm>  // for std::lower_bound
#include <cstring>    // for memcpy

#define BPT_ORDER 4  // B+树的阶数（最大子节点数）

// 构造函数（叶子节点）
BPlusTree::Node::Node(bool leaf) {
    this->leaf = leaf;
    this->parent = nullptr;
    this->next = nullptr;  // 叶子节点链表
}

// 计算叶子节点间的距离
uint32_t BPlusTree::Node::Distance(Node* node) {
    if (this->values.empty() || node->values.empty()) {
        return UINT32_MAX;  // 无数据时返回最大距离
    }

    uint32_t dist = 0;
    uint32_t* arr1 = (uint32_t*)this->values[0];  // 取第一个值进行比较
    uint32_t* arr2 = (uint32_t*)node->values[0];
    
    size_t valcnt = std::min(this->values.size(), node->values.size());
    for (size_t i = 0; i < valcnt; i++) {
        uint32_t d = (arr1[i] > arr2[i]) ? (arr1[i] - arr2[i]) : (arr2[i] - arr1[i]);
        dist += d;
    }
    return dist;
}

// B+树构造函数
BPlusTree::BPlusTree() {
    root = new Node(true);  // 初始时，根节点是叶子节点
}

// 插入数据到B+树
void BPlusTree::Insert(uint64_t key, void* value, size_t valbytes) {
    Node* leaf = FindLeaf(key); // 先找到叶子节点
    InsertIntoLeaf(leaf, key, value, valbytes);
}

// 查找对应 key 所在的叶子节点
BPlusTree::Node* BPlusTree::FindLeaf(uint64_t key) {
    Node* cur = root;
    while (!cur->leaf) {
        int i = 0;
        while (i < cur->keys.size() && key >= cur->keys[i]) {
            i++;
        }
        cur = cur->children[i];  // 进入子节点
    }
    return cur;
}

// 插入到叶子节点
void BPlusTree::InsertIntoLeaf(Node* leaf, uint64_t key, void* value, size_t valbytes) {
    // 在 keys 数组中找到合适的位置
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    int idx = it - leaf->keys.begin();

    leaf->keys.insert(leaf->keys.begin() + idx, key);
    
    void* new_value = malloc(valbytes);
    memcpy(new_value, value, valbytes);
    leaf->values.insert(leaf->values.begin() + idx, new_value);

    if (leaf->keys.size() > BPT_ORDER - 1) {
        SplitLeaf(leaf);
    }
}

// 叶子节点拆分
void BPlusTree::SplitLeaf(Node* leaf) {
    int mid = leaf->keys.size() / 2;
    Node* newLeaf = new Node(true);

    newLeaf->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
    newLeaf->values.assign(leaf->values.begin() + mid, leaf->values.end());

    leaf->keys.resize(mid);
    leaf->values.resize(mid);

    newLeaf->next = leaf->next;
    leaf->next = newLeaf;
    newLeaf->parent = leaf->parent;

    if (!leaf->parent) {
        root = new Node(false);
        root->keys.push_back(newLeaf->keys[0]);
        root->children.push_back(leaf);
        root->children.push_back(newLeaf);
        leaf->parent = newLeaf->parent = root;
    } else {
        InsertIntoParent(leaf, newLeaf->keys[0], newLeaf);
    }
}

// 插入到内部节点
void BPlusTree::InsertIntoParent(Node* left, uint64_t key, Node* right) {
    Node* parent = left->parent;
    auto it = std::lower_bound(parent->keys.begin(), parent->keys.end(), key);
    int idx = it - parent->keys.begin();

    parent->keys.insert(parent->keys.begin() + idx, key);
    parent->children.insert(parent->children.begin() + idx + 1, right);
    right->parent = parent;

    if (parent->keys.size() >= BPT_ORDER) {
        SplitInternal(parent);
    }
}

// 内部节点拆分
void BPlusTree::SplitInternal(Node* node) {
    int mid = node->keys.size() / 2;
    Node* newNode = new Node(false);

    newNode->keys.assign(node->keys.begin() + mid + 1, node->keys.end());
    newNode->children.assign(node->children.begin() + mid + 1, node->children.end());

    uint64_t midKey = node->keys[mid];

    node->keys.resize(mid);
    node->children.resize(mid + 1);

    newNode->parent = node->parent;

    for (auto child : newNode->children) {
        child->parent = newNode;
    }

    if (!node->parent) {
        root = new Node(false);
        root->keys.push_back(midKey);
        root->children.push_back(node);
        root->children.push_back(newNode);
        node->parent = newNode->parent = root;
    } else {
        InsertIntoParent(node, midKey, newNode);
    }
}

// 查找某个 key 的叶子节点
BPlusTree::Node* BPlusTree::Find(uint64_t key) {
    Node* leaf = FindLeaf(key);
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it != leaf->keys.end() && *it == key) {
        return leaf;  // 返回叶子节点
    }
    return nullptr;
}

// 获取中序遍历的下一个叶子节点（范围查询）
BPlusTree::Node* BPlusTree::Next(Node* node) {
    if (!node) return nullptr;
    return node->next;
}

// 处理并行查询
void traverse_queries(BPlusTree* tree, std::vector<std::tuple<uint64_t, uint64_t>> queries, int tid, int threadcnt, uint64_t* ret) {
    uint64_t ans_sum = 0;
    for (size_t i = tid; i < queries.size(); i += threadcnt) {
        auto [from, to] = queries[i];

        BPlusTree::Node* nn = tree->Find(from);
        if (!nn) continue;

        uint64_t min_dist = UINT32_MAX;

        BPlusTree::Node* n = nn;
        while (n && !n->keys.empty() && n->keys[0] <= to) {
            uint32_t ndist = n->Distance(nn);
            min_dist = std::min(min_dist, (uint64_t)ndist);
            n = tree->Next(n);
        }
        ans_sum += min_dist;
    }
    *ret = ans_sum;
}
