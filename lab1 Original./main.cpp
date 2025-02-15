#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>

#include <chrono>
#include <vector>
#include <tuple>
#include <thread>

#include "tree.h"

#define THREAD_MAX

int main (int argc, char** argv) {
	if ( argc < 5 ) {
		printf( "usage: %s [input filename] [val cnt] [query file] [thread cnt]\n", argv[0] );
		exit(1);
	}

	FILE* fin = fopen( argv[1], "rb");
	if ( NULL == fin ) {
		printf( "Failed to open input file\n" );
		exit(1);
	}
	size_t valcnt = atoi(argv[2]);
	if ( valcnt < 4 || valcnt > 8192 ) {
		printf( "Exceeds arbitrary limitation on value array size\n" );
		exit(1);
	}

	FILE* finq = fopen( argv[3], "rb" );
	if ( NULL == finq ) {
		printf( "Failed to open query file\n" );
		exit(1);
	}

	int thread_cnt = atoi(argv[4]);
	if ( thread_cnt < 1 ) thread_cnt = 1;
	if ( thread_cnt > 1024 ) thread_cnt = 1024;

	MyTree* tree = new MyTree();

	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

	void* valbuf = malloc(valcnt*sizeof(uint32_t));
	while (!feof(fin) ) {
		uint64_t key;
		size_t rres = fread(&key, sizeof(uint64_t), 1, fin);
		if ( rres != 1 ) continue;

		rres = fread(valbuf, sizeof(uint32_t), valcnt, fin);

		MyTree::MyNode* nn = new MyTree::MyNode(key, valbuf, valcnt*sizeof(uint32_t));
		tree->Insert(nn);
	}
	
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now-start);

	printf( "Done inserting all data. Elapsed %.02fs\n", elapsed.count() );
	fflush(stdout);

	std::vector<std::tuple<MyTree::MyNode*, uint64_t>> queries;
	while( !feof(finq) ) {
		uint64_t from, to;
		size_t rres = fread(&from, sizeof(uint64_t), 1, finq);
		if ( rres != 1 ) continue;
		rres = fread(&to, sizeof(uint64_t), 1, finq);
		if ( rres != 1 ) continue;

		rres = fread(valbuf, sizeof(uint32_t), valcnt, finq);
		MyTree::MyNode* nn = new MyTree::MyNode(from, valbuf, valcnt*sizeof(uint32_t));
		queries.push_back({nn,to});


	}

	printf( "Finished reading the query file. %ld requests.\n", queries.size() );
	fflush(stdout);

	uint64_t ans_sum = 0;;
	std::vector<std::thread*> threads;
	threads.reserve(thread_cnt);

	std::vector<uint64_t> return_cnts;
	return_cnts.reserve(thread_cnt);

	start = std::chrono::high_resolution_clock::now();

	for ( int i = 0; i < thread_cnt; i++ ) {
		return_cnts[i] = 0;
		threads[i] = new std::thread(traverse_queries, tree, queries, i, thread_cnt, &(return_cnts[i]));
	}

	for ( int i = 0; i < thread_cnt; i++ ) {
		threads[i]->join();
		ans_sum += return_cnts[i];
		//printf( "%lx\n", return_cnts[i] );
	}



	now = std::chrono::high_resolution_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - start);
	
	printf( "Answer_sum: %ld\n", ans_sum );
	printf( "Done querying. Elapsed %.06fs\n", elapsed.count() );
	fflush(stdout);

	return 0;
}
