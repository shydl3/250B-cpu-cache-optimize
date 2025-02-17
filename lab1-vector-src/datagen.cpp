#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>

#include <random>

int
main (int argc, char** argv) {
	srand(time(NULL));
	std::mt19937 rng(time(NULL));
	size_t elem_lg = 24;
	size_t collision_factor_lg = 7;
	size_t valcnt = 64;
	size_t qcnt = 1024*1024*4;

	printf( "usage: %s [elements_log] [collision_factor] [query_cnt_log] \n", argv[0] );

	if ( argc > 2 ) {
		elem_lg = atoi(argv[1]);
	}
	if ( argc > 3 ) {
		collision_factor_lg = atoi(argv[2]);
	}

	if ( argc > 4 ) {
		int qcnt_lg = atoi(argv[3]);
		qcnt = 1L << qcnt_lg;
	}


	size_t elem_cnt = (1L << elem_lg);
	size_t keyspace = (elem_lg-collision_factor_lg);
	uint64_t keymask = (1L<<keyspace)-1;
	size_t valbits = 16;
	uint32_t valmask = (1<<valbits)-1;
	
	printf( "Startind data generation. Elements: %ld, Key bits: %ld, Queries: %ld\n", elem_cnt, keyspace, qcnt );

	if ( elem_lg < 4 || elem_lg >= 42 ) {
		printf( "Exceeds arbitrary limitation on file size!\n" );
		exit(1);
	}
	if ( keyspace < 4 || keyspace >= 32 ) {
		printf( "Exceeds arbirary limitation on key collisions!\n" );
		exit(1);
	}

	char filename[128];
	sprintf(filename, "bfile_%02ld_%02ld.dat", elem_lg, valcnt);

	FILE* fout = fopen(filename, "wb" );

	size_t last_percentage = 0;
	for ( size_t i = 0; i < elem_cnt; i++ ) {
		uint64_t rv = (uint64_t)rng();
		rv = (rv<<32)|rng();
		uint64_t key = rv & keymask;
		fwrite(&key, sizeof(uint64_t), 1, fout);

		for ( size_t j = 0; j < valcnt; j++ ) {
			float val = rng() & valmask;
			fwrite(&val, sizeof(uint32_t), 1, fout);
		}

		size_t cur_percentage = i*100/elem_cnt;
		if ( cur_percentage > last_percentage ) {
			last_percentage = cur_percentage;
			printf( "Generated %02ld%%  data...\n", cur_percentage );
		}
	}

	fclose(fout);
	printf( "Generated %ld elements, 8-byte keys and %02ld x 4-byte float values\n",elem_cnt, valcnt );

	sprintf(filename, "qfile_%02ld.dat", elem_lg);
	FILE* foutq = fopen (filename, "wb" );
	for ( size_t i = 0; i < qcnt; i++ ) {
		uint64_t rv = (uint64_t)rng();
		rv = (rv<<32)|rng();
		uint64_t key = rv & keymask;
		fwrite(&key, sizeof(uint64_t), 1, foutq);

		key += (rng()%128);
		fwrite(&key, sizeof(uint64_t), 1, foutq);
		
		for ( size_t j = 0; j < valcnt; j++ ) {
			float val = rng() & valmask;
			fwrite(&val, sizeof(uint32_t), 1, fout);
		}
	}

	fclose(foutq);
	printf( "Generated %ld queries, 8-byte key pairs\n", qcnt );

	exit(0);
}
