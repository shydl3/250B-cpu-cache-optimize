#include "tree.h"
#include <algorithm>  // for std::lower_bound
#include <cstring>    // for memcpy
#include <immintrin.h> // for the AVX2

#define BPT_ORDER 4  // B+树的阶数（最大子节点数）

// 构造函数（叶子节点）
BPlusTree::Node::Node(bool leaf) {
    this->leaf = leaf;
    this->parent = nullptr;
    this->next = nullptr;  // 叶子节点链表
}

//计算叶子节点间的距离
uint32_t BPlusTree::Node::Distance(Node* node) {
    if (this->values.empty() || node->values.empty()) {
        return UINT32_MAX;  // 无数据时返回最大距离
    }

    uint32_t* arr1 = (uint32_t*)this->values[0];  // 取第一个值进行比较
    uint32_t* arr2 = (uint32_t*)node->values[0];
    
    size_t valcnt1 = this->valbytes[0] / sizeof(uint32_t);
    size_t valcnt2 = node->valbytes[0] / sizeof(uint32_t);
    size_t valcnt = std::min(valcnt1, valcnt2);

    // printf("valcnt1 : %lu\n", valcnt1);
    // printf("valcnt2 : %lu\n", valcnt2);
	// exit(0);

    // size_t valcnt = std::min(this->values.size(), node->values.size());

    uint32_t dist = 0;
    for (size_t i = 0; i < valcnt; i++) {
        uint32_t d = (arr1[i] > arr2[i]) ? (arr1[i] - arr2[i]) : (arr2[i] - arr1[i]);
        dist += d;
    }

    // printf("Distance between nodes: %u\n", dist);
    // exit(0);

    return dist;
}




// AVX2 计算叶子节点间的距离
// uint32_t BPlusTree::Node::Distance(Node* node) {
//     if (this->values.empty() || node->values.empty()) {
//         return UINT32_MAX;  // 无数据时返回最大距离
//     }

//     uint32_t dist = 0;
//     uint32_t* arr1 = (uint32_t*)this->values[0];  // 取第一个值进行比较
//     uint32_t* arr2 = (uint32_t*)node->values[0];

//     size_t valcnt = std::min(this->values.size(), node->values.size());
//     size_t i = 0;

//     __m256i sumVec = _mm256_setzero_si256();  // 初始化累加向量

//     // 以 8 个 uint32_t 为一组进行 AVX2 计算
//     for (; i + 8 <= valcnt; i += 8) {
//         __m256i v1 = _mm256_loadu_si256((__m256i*)(arr1 + i));
//         __m256i v2 = _mm256_loadu_si256((__m256i*)(arr2 + i));
//         __m256i diff = _mm256_sub_epi32(_mm256_max_epu32(v1, v2), _mm256_min_epu32(v1, v2));
//         sumVec = _mm256_add_epi32(sumVec, diff);
//     }

//     // 累加 AVX2 计算的结果
//     alignas(32) uint32_t temp[8];
//     _mm256_store_si256((__m256i*)temp, sumVec);
//     for (int j = 0; j < 8; ++j) {
//         dist += temp[j];
//     }

//     // 处理剩余的部分（不足 8 的部分）
//     for (; i < valcnt; i++) {
//         uint32_t d = (arr1[i] > arr2[i]) ? (arr1[i] - arr2[i]) : (arr2[i] - arr1[i]);
//         dist += d;
//     }

//     return dist;
// }





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
    if (!new_value) {
        printf("Error: malloc failed for value allocation\n");
        exit(1);
    }
    memcpy(new_value, value, valbytes);
    leaf->values.insert(leaf->values.begin() + idx, new_value);
    leaf->valbytes.insert(leaf->valbytes.begin() + idx, valbytes); // 存储字节大小


    if (leaf->keys.size() > BPT_ORDER - 1) {
        SplitLeaf(leaf);
    }
}

// 叶子节点拆分
void BPlusTree::SplitLeaf(Node* leaf) {
    int mid = leaf->keys.size() / 2;
    Node* newLeaf = new Node(true);

    // **分裂 `keys`、`values` 和 `valbytes`**
    newLeaf->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
    newLeaf->values.assign(leaf->values.begin() + mid, leaf->values.end());
    newLeaf->valbytes.assign(leaf->valbytes.begin() + mid, leaf->valbytes.end());

    leaf->keys.resize(mid);
    leaf->values.resize(mid);
    leaf->valbytes.resize(mid);

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
        if (!nn) {
            // printf("Thread %d: Key %lu not found in B+ tree!\n", tid, from);
            continue;
        }
        
        uint64_t min_dist = UINT32_MAX;
        // printf("Initial min_dist: %llu\n", (unsigned long long)min_dist);
        // exit(1);

        BPlusTree::Node* n = tree->Next(nn);
        while (n && !n->keys.empty() && n->keys[0] <= to) {
            uint32_t ndist = n->Distance(nn);

            if (ndist < min_dist) {
                min_dist = ndist;
                // printf("Updated min_dist: %llu\n", (unsigned long long)min_dist);
                // exit(1);
            } else break;

            
            // printf("min_dist: %llu\n", (unsigned long long)min_dist);
            // printf("ndist: %lu\n", (uint64_t)ndist);
            // printf("==================\n");
            // exit(0);

            n = tree->Next(n);
        }
        ans_sum += min_dist;
    }
    *ret = ans_sum;
}
