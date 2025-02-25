#include "tree.h"

MyTree::MyNode::MyNode(uint64_t key, void* val, size_t valbytes) {
	this->key = key;
	this->val = malloc(valbytes);
	memcpy(this->val, val, valbytes);
	this->parent = NULL;
	this->right = NULL;
	this->left = NULL;
	this->valcnt = valbytes/sizeof(uint32_t);
}

uint32_t MyTree::MyNode::Distance(MyTree::MyNode* node) {
	uint32_t dist = 0;
	uint32_t* arr1 = (uint32_t*)this->val;
	uint32_t* arr2 = (uint32_t*)node->val;
	for ( size_t i = 0; i < this->valcnt; i++ ) {
		uint32_t d = (arr1[i] > arr2[i]) ? (arr1[i] - arr2[i]) : (arr2[i] - arr1[i]);
		dist += d;
	}
	return dist;
}

MyTree::MyTree() {
}

void 
MyTree::Insert(MyNode* node) {
	if ( root == NULL ) {
		root = node;
		return;
	}

	MyNode* curpointer = root;
	bool done = false;
	while(!done) {
		if ( node->key >= curpointer->key ) {
			if ( curpointer->right == NULL ) {
				curpointer->right = node;
				node->parent = curpointer;
				done = true;
			} else {
				curpointer = curpointer->right;
			}
		} else {
			if ( curpointer->left == NULL ) {
				curpointer->left = node;
				node->parent = curpointer;
				done = true;
			} else {
				curpointer = curpointer->left;
			}
		}
	}
}

MyTree::MyNode* 
MyTree::Find(uint64_t key) {
	MyNode* curpointer = root;
	while(true) {
		if ( key == curpointer->key ) {
			if ( curpointer->left != NULL && curpointer->left->key == key ) {
				curpointer = curpointer->left;
			} else return curpointer;
		} else if ( key > curpointer->key ) {
			if ( curpointer->right == NULL ) {
				return NULL;
			} else {
				curpointer = curpointer->right;
			}
		} else {
			if ( curpointer->left == NULL ) {
				return NULL;
			} else {
				curpointer = curpointer->left;
			}
		}
	}
}

MyTree::MyNode* 
MyTree::Next(MyTree::MyNode* node) {
	if (!node) return NULL;
	if (node->right) {
		MyNode* nextNode = node->right;
		while (nextNode->left) {
			nextNode = nextNode->left;
		}
		return nextNode;
	}

	MyNode* cur = node;
	while (cur->parent && cur == cur->parent->right) {
		cur = cur->parent;
	}
	return cur->parent;
}

void traverse_queries(MyTree* tree, std::vector<std::tuple<MyTree::MyNode*, uint64_t>> queries, int tid, int threadcnt, uint64_t* ret) {
	uint64_t ans_sum = 0;
	for ( size_t i = tid; i < queries.size(); i += threadcnt ) {
		std::tuple<MyTree::MyNode*, uint64_t> q = queries[i];
		MyTree::MyNode* from = std::get<0>(q);
		uint64_t to = std::get<1>(q);
		//printf( "Query: %lx %lx\n", from->key,  to );

		MyTree::MyNode* nn = tree->Find(from->key);
		if ( nn == NULL ) {
			//printf( "Not found!\n" );
		} else {
			uint64_t min_dist = nn->Distance(from);
			
			MyTree::MyNode* n = tree->Next(nn);
			while ( n != NULL ) {
				if ( n->key <= to ) {
					uint32_t ndist = n->Distance(from);
					if ( ndist < min_dist ) min_dist = ndist;
					//printf( "%lx ", n->key );
				}else break;
				n = tree->Next(n);
			}
			ans_sum += min_dist;
			//printf( "Found! min_dist: %u\n", min_dist );
		}

	}
	*ret = ans_sum;
	//printf( "%d/%d -- %lx %lx\n", tid, threadcnt, *ret, ans_sum );
}

