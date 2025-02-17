#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <chrono>
#include <vector>
#include <tuple>
#include <thread>

#ifndef __MY_TREE_H__
#define __MY_TREE_H__
class MyTree {
	public:
	class MyNode {
		public:
		MyNode(uint64_t key, void* val, size_t valbytes);
		void* val;
		size_t valcnt;
		uint64_t key;
		uint32_t Distance(MyNode* node);


		MyNode* parent = NULL;
		MyNode* right = NULL;
		MyNode* left = NULL;
		private:

	};
	
	MyTree();
	void Insert(MyNode* node);
	MyNode* Find(uint64_t key);
	MyNode* Next(MyNode* node);
	private:

	MyNode* root = NULL;
};

void traverse_queries(MyTree* tree, std::vector<std::tuple<MyTree::MyNode*, uint64_t>> queries, int tid, int threadcnt, uint64_t* ret);

#endif
