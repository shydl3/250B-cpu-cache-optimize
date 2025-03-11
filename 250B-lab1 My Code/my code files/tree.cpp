#include "tree.h"
#include <algorithm>  
#include <cstring>
#include <immintrin.h>
#include <iostream>
#include <queue>

// int cnt_i = 0;

MyTree::MyNode::MyNode(uint64_t key, void* val, size_t valbytes) {
	this->key = key;
	this->val = malloc(valbytes);
	memcpy(this->val, val, valbytes);
	this->parent = NULL;
	this->right = NULL;
	this->left = NULL;
	this->valcnt = valbytes/sizeof(uint32_t);
}


MyTree::MyTree() {
    root = new BPlusNode(true);
    struct BPT_Record {
        uint64_t key;
        void* value;
    };
}


uint32_t MyTree::Distance(void* val0, MyNode* node) {
    uint32_t sum = 0;
    uint32_t* arr1 = reinterpret_cast<uint32_t*>(val0);
    uint32_t* arr2 = reinterpret_cast<uint32_t*>(node->val);
    size_t count = node->valcnt;

    // 初始化 256 位的累加器
    __m256i vsum = _mm256_setzero_si256();
    size_t i = 0;

    // 预取数据，提高缓存命中率
    _mm_prefetch(reinterpret_cast<const char*>(arr1), _MM_HINT_T0);
    _mm_prefetch(reinterpret_cast<const char*>(arr2), _MM_HINT_T0);

    // 处理 8 个 uint32_t
    for (; i + 7 < count; i += 8) {
        // 预取后续数据，减少缓存未命中
        _mm_prefetch(reinterpret_cast<const char*>(&arr1[i + 8]), _MM_HINT_T0);
        _mm_prefetch(reinterpret_cast<const char*>(&arr2[i + 8]), _MM_HINT_T0);

        // 加载 8 个 32-bit 整数（非对齐加载）
        __m256i v1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(arr1 + i));
        __m256i v2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(arr2 + i));

        // 计算绝对差值（使用 AVX2 内置指令）
        __m256i abs_diff = _mm256_abs_epi32(_mm256_sub_epi32(v1, v2));

        // 累加绝对差值
        vsum = _mm256_add_epi32(vsum, abs_diff);
    }

    // **优化后的水平求和**
    // _mm256_hadd_epi32 可以快速计算相邻元素的和
    __m256i sum1 = _mm256_hadd_epi32(vsum, vsum);
    __m256i sum2 = _mm256_hadd_epi32(sum1, sum1);
    __m128i sum3 = _mm_add_epi32(_mm256_castsi256_si128(sum2), _mm256_extracti128_si256(sum2, 1));
    
    // 提取最终结果
    sum += _mm_cvtsi128_si32(sum3);

    // 处理剩余不足 8 个元素的部分
    for (; i < count; i++) {
        sum += (arr1[i] > arr2[i]) ? (arr1[i] - arr2[i]) : (arr2[i] - arr1[i]);
    }

    return sum;
}


MyTree::MyNode* MyTree::Find(uint64_t key) {
    if (!root) return nullptr;
    
    // 1. 根据 key 找到对应的叶子节点（可能不是包含目标 key 最左侧的叶子）
    BPlusNode* leaf = FindLeaf(key);
    if (!leaf) return nullptr;

    // 2. 向前查找：如果前一个叶子存在，并且其最后一个 key 等于目标 key，则说明目标 key 的重复记录可能跨叶分布，
    //    应该移动到左侧叶子继续查找。
    while (leaf->prev && !leaf->prev->keys.empty() && leaf->prev->keys.back() == key) {
        leaf = leaf->prev;
    }

    // 二分查找
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it == leaf->keys.end() || *it != key) {
        return nullptr;
    }
    int found_i = it - leaf->keys.begin();
    return leaf->values[found_i];
}


MyTree::BPlusNode* MyTree::FindLeaf(uint64_t key) {
    MyTree::BPlusNode* cur = root;
    while (cur && !cur->isLeaf) {
        // pick which child to descend
        size_t i = 0;
        while (i < cur->keys.size() && key >= cur->keys[i]) {
            i++;
        }
        cur = cur->children[i];
    }
    return cur;
}


MyTree::MyNode* MyTree::Next(MyNode* current) {
    BPlusNode* leaf = current->parentLeaf;
    size_t idx = current->leafIndex;
    if (idx + 1 < leaf->values.size()) {
        return leaf->values[idx + 1];
    } else if (leaf->next && !leaf->next->values.empty()) {
        return leaf->next->values[0];
    } else {
        return nullptr;
    }
}


void MyTree::SplitChild(BPlusNode* parent, int index, BPlusNode* child) {
    // 创建新子节点，新节点继承child的叶子属性
    BPlusNode* newChild = new BPlusNode(child->isLeaf);
    // 插入到父节点的children列表中，新节点紧跟在child之后
    parent->children.insert(parent->children.begin() + index + 1, newChild);

    if (child->isLeaf) {
        // 对于叶节点，选择分裂点：左节点保留前 splitIndex 个记录，右节点获得剩余记录
        int splitIndex = BPT_ORDER;  // 保留child中前BPT_ORDER个数据
        // 由右节点的第一个 key 作为父节点的索引
        uint64_t promotedKey = child->keys[splitIndex];
        parent->keys.insert(parent->keys.begin() + index, promotedKey);

        // 将child的右半部分 key 和对应的 values 拷贝到 newChild 中
        newChild->keys.assign(child->keys.begin() + splitIndex, child->keys.end());
        newChild->values.assign(child->values.begin() + splitIndex, child->values.end());

        // 保留child的左半部分记录
        child->keys.resize(splitIndex);
        child->values.resize(splitIndex);

        // 更新 child 中每个节点的 leafIndex 和 parentLeaf 指针
        for (size_t j = 0; j < child->values.size(); j++) {
            child->values[j]->leafIndex = j;
            child->values[j]->parentLeaf = child;
        }
        // 更新 newChild 中每个节点的 leafIndex 和 parentLeaf 指针
        for (size_t j = 0; j < newChild->values.size(); j++) {
            newChild->values[j]->leafIndex = j;
            newChild->values[j]->parentLeaf = newChild;
        }

        // 更新叶节点链表
        newChild->next = child->next;
        if (newChild->next != nullptr) {
            newChild->next->prev = newChild;
        }
        child->next = newChild;
        newChild->prev = child;
    } else {
        // 内部节点的分裂逻辑保持不变
        uint64_t midKey = child->keys[BPT_ORDER - 1];
        parent->keys.insert(parent->keys.begin() + index, midKey);
        
        newChild->keys.assign(child->keys.begin() + BPT_ORDER, child->keys.end());
        child->keys.resize(BPT_ORDER - 1);
        
        newChild->children.assign(child->children.begin() + BPT_ORDER, child->children.end());
        child->children.resize(BPT_ORDER);
    }
}



void MyTree::insertNonFull(BPlusNode* node, uint64_t key, MyNode* nn) {
    // std::cout << "Inserting key=" << key << " val=" << *(uint64_t*)val << " into node" << std::endl;
    if (node->isLeaf) {
        // 对于叶子节点，使用 upper_bound 插入，确保相同 key 的新数据累积到右侧
        auto it = std::upper_bound(node->keys.begin(), node->keys.end(), key);
        size_t index = it - node->keys.begin();
        node->keys.insert(it, key);
        node->values.insert(node->values.begin() + index, nn);

        nn->leafIndex = index;
        nn->parentLeaf = node;

        for (size_t j = index + 1; j < node->values.size(); j++) {
            node->values[j]->leafIndex = j;
        }

    } else {
        // 在内部节点中寻找合适的子节点
        int i = node->keys.size() - 1;
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;
        // 如果目标子节点已满，先进行分裂
        if (node->children[i]->keys.size() == 2 * BPT_ORDER - 1) {
            SplitChild(node, i, node->children[i]);
            // 分裂后，如果key大于新插入的索引，选择右侧子节点
            if (key > node->keys[i]) {
                i++;
            }
        }
        insertNonFull(node->children[i], key, nn);
    }
}


void MyTree::Insert(MyNode* nn) {
    if (root == nullptr) {
        root = new BPlusNode(true);
        root->keys.push_back(nn->key);
        root->values.push_back(nn);
    } else {
        // 若根节点已满，则先分裂根节点
        if (root->keys.size() == 2 * BPT_ORDER - 1) {
            BPlusNode* newRoot = new BPlusNode(false);
            newRoot->children.push_back(root);
            SplitChild(newRoot, 0, root);
            root = newRoot;
        }
        insertNonFull(root, nn->key, nn);
    }
    // cnt_i++;
    // if (cnt_i == 100) exit(0);
}

// int cnt_find = 0;
// int cnt_n = 0;

void traverse_queries(MyTree* tree,
    const std::vector<std::tuple<MyTree::MyNode*, uint64_t>>& queries,
    size_t start_index, size_t end_index,
    uint64_t* ret) 
{
    uint64_t ans_sum = 0;

    for (size_t i = start_index; i < end_index; ++i) {
        std::tuple<MyTree::MyNode*, uint64_t> q = queries[i];
        MyTree::MyNode* from = std::get<0>(q);
        uint64_t to = std::get<1>(q);

        MyTree::MyNode* nn = tree->Find(from->key);
        if (!nn) continue;
        else {
            uint64_t min_dist = tree->Distance(from->val, nn);
            MyTree::MyNode* n = tree->Next(nn);
            while (n) {
                if (n->key <= to) {
                    uint32_t ndist = tree->Distance(from->val, n);
                    if (ndist < min_dist)
                        min_dist = ndist;
                } else break;
                n = tree->Next(n);
            }
            ans_sum += min_dist;
        }
    }
    *ret = ans_sum;
}
