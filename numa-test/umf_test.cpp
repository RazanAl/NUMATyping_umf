
#include <umf/mempolicy.h>
#include <umf/memspace.h>
#include <jemalloc/jemalloc.h>
#include <umf/pools/pool_jemalloc.h>

#include <assert.h>
#include <stdbool.h>
#include <pthread.h>

#ifndef _WIN32
#include <unistd.h>
#endif
// #include <iostream>
#include <umf/ipc.h>
#include <umf/memory_pool.h>
#include <umf/pools/pool_proxy.h>
#include <umf/pools/pool_scalable.h>
#include <umf/providers/provider_level_zero.h>
#include <umf/providers/provider_os_memory.h>

#include <numa.h>
#include <numaif.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


// Function to create a memory provider which allocates memory from the specified NUMA node
// by using umfMemspaceCreateFromNumaArray
int createMemoryProviderFromArray(umf_memory_provider_handle_t *hProvider,
                                  unsigned numa) {
    int ret = 0;
    umf_result_t result;
    umf_memspace_handle_t hMemspace = NULL;
    umf_mempolicy_handle_t hPolicy = NULL;

    // Create a memspace - memspace is a list of memory sources.
    // In this example, we create a memspace that contains single numa node;
    result = umfMemspaceCreateFromNumaArray(&numa, 1, &hMemspace);
    if (result != UMF_RESULT_SUCCESS) {
        fprintf(stderr, "umfMemspaceCreateFromNumaArray() failed.\n");
        return -1;
    }

    // Create a mempolicy - mempolicy defines how we want to use memory from memspace.
    // In this example, we want to bind memory to the specified numa node.
    result = umfMempolicyCreate(UMF_MEMPOLICY_BIND, &hPolicy);
    if (result != UMF_RESULT_SUCCESS) {
        ret = -1;
        fprintf(stderr, "umfMempolicyCreate failed().\n");
        goto error_memspace;
    }

    // Create a memory provider using the memory space and memory policy
    result = umfMemoryProviderCreateFromMemspace(hMemspace, hPolicy, hProvider);
    if (result != UMF_RESULT_SUCCESS) {
        ret = -1;
        fprintf(stderr, "umfMemoryProviderCreateFromMemspace failed().\n");
        goto error_mempolicy;
    }

    // After creating the memory provider, we can destroy the memspace and mempolicy
error_mempolicy:
    umfMempolicyDestroy(hPolicy);
error_memspace:
    umfMemspaceDestroy(hMemspace);
    return ret;
}

#define NUM_NODES 1
static umf_memory_provider_handle_t NUMA_HANDLES[NUM_NODES];
umf_memory_pool_handle_t jemalloc_pool[NUM_NODES];
    

__attribute__((constructor))
void umf_alloc_init() {
    void* ptr = NULL;
    umf_memspace_handle_t hMemspace= NULL;
    umf_mempolicy_handle_t hPolicy = NULL;
    for (unsigned i = 0; i < NUM_NODES; ++i) {
        umf_result_t r = umfMemspaceCreateFromNumaArray(&i, 1, &hMemspace);
        if (r != UMF_RESULT_SUCCESS) {
            assert(false && "Could not create space");
        }
        umf_result_t policy = umfMempolicyCreate(UMF_MEMPOLICY_BIND, &hPolicy);
        if (policy != UMF_RESULT_SUCCESS) {
            assert(false && "Could not create policy");
        }
        umf_result_t h = umfMemoryProviderCreateFromMemspace(hMemspace, hPolicy, &NUMA_HANDLES[i]);
        if (h != UMF_RESULT_SUCCESS) {
            assert(false && "Could not create policy");
        }
        umf_result_t pool = umfPoolCreate(umfJemallocPoolOps(), NUMA_HANDLES[i], NULL,  UMF_POOL_CREATE_FLAG_DISABLE_TRACKING, &jemalloc_pool[i]);
        if(pool != UMF_RESULT_SUCCESS){
            assert(false && "Could not create pool");
        }
        umfMempolicyDestroy(hPolicy);
        umfMemspaceDestroy(hMemspace);
        size_t sz;
    
        ptr = umfPoolAlignedMalloc(jemalloc_pool[i], 1*1024*1024*1024, sizeof(char));
        printf("Allocated pool %d \n", i);
        if(ptr == NULL){
            assert(false && "Could not allocate pool");
        }
        for(int j = 0; j < 100*1024*1024; j+=4096){
            ((char*)ptr)[j] = 'a';
        }
        if(umfPoolFree(jemalloc_pool[i], NULL) != UMF_RESULT_SUCCESS){
            assert(false && "Could not free pool");
        }
    }
}


inline static __attribute__((always_inline))  void* umf_alloc(unsigned NodeId, size_t size, size_t allign);

inline static __attribute__((always_inline))  void* umf_alloc(unsigned NodeId, size_t size, size_t allign){
	//return mallocx(size,0);
    // void *ptr = malloc(size);

	//std::cout<<"here";
	assert(true==true);
    // void *ptr = umfFastJemallocMalloc(jemalloc_pool[NodeId], size);
	void *ptr = umfPoolMalloc(jemalloc_pool[NodeId], size);
	(ptr && "Bad alloc");
    return ptr;
}

inline static __attribute__((always_inline))  void umf_free(unsigned NodeId, void* p);
inline static __attribute__((always_inline))  void umf_free(unsigned NodeId, void* p){
	// free(p);
	//return;
    // if(umfFastJemallocFree(jemalloc_pool[NodeId], p) != UMF_RESULT_SUCCESS){
	if(umfPoolFree(jemalloc_pool[NodeId],p) != UMF_RESULT_SUCCESS){
        assert(false && "Could not free pool");
    }
}





pthread_t* threads;
size_t* args;
size_t NUM_THREADS;
pthread_barrier_t bar;
pthread_mutex_t lk;
int allocator = 0;

struct timespec start, end;
int* buffer;
int BUFFER_SZ = 40*1024*1024;

void global_init(){
	threads = malloc(NUM_THREADS*sizeof(pthread_t));
	args = malloc(NUM_THREADS*sizeof(size_t));
	pthread_barrier_init(&bar, NULL, NUM_THREADS);
    pthread_mutex_init(&lk,NULL);		
	buffer = malloc(sizeof(int)*BUFFER_SZ);
	//buffer = umf_alloc(alloc_on_node, sizeof(int)*BUFFER_SZ, 64);
	assert(RAND_MAX > BUFFER_SZ);
}

void global_cleanup(){
	free(threads);
	free(args);
	pthread_barrier_destroy(&bar);
    pthread_mutex_destroy(&lk);
}

void local_init(){}
void local_cleanup(){}

void* thread_main(void* args){
	size_t tid = *((size_t*)args);
	local_init();
	printf("Hello, World from %zu\n",tid); /*printf() is specified as thread-safe as of C11*/
	numa_run_on_node(0);
	pthread_barrier_wait(&bar);
	if(tid==0){
		clock_gettime(CLOCK_MONOTONIC,&start);
	}
	pthread_barrier_wait(&bar);
	
	// do something
	printf("Allocation: %d\n",allocator);
	int BUFFER_SZ = 1024*1024;
	void** buffer = calloc(BUFFER_SZ,sizeof(void*));
	void * old;
	for(int i = 0; i<1000000; i++){
		if(allocator == 1){
			if(i >= 1){
				old = buffer[(i%BUFFER_SZ)];
			}
			else{
				old = buffer[i%BUFFER_SZ];
			}
			//printf("old is  %p\n", old);
			if(old!=0){umf_free(0,old);}
			// buffer[i%BUFFER_SZ] = umfFastJemallocMalloc(jemalloc_pool[0], 64);
			buffer[i%BUFFER_SZ] = umf_alloc(0, 64, 64);	
			//printf("old is going to be %p\n", buffer[i%BUFFER_SZ]);
			if(i==0){
				printf("TID: %zu\n",tid);
			}
		}
		else{
			void* old = buffer[i%BUFFER_SZ];
			if(old!=0){free(old);}
			buffer[i%BUFFER_SZ] = malloc(64);	
		}
	}
	
    
	pthread_barrier_wait(&bar);
	if(tid==0){
		clock_gettime(CLOCK_MONOTONIC,&end);
	}
	local_cleanup();
	
	return 0;
}




int main(int argc, const char* argv[]){
	
	// parse args
	// if(argc==2){
	// 	allocator = atoi( argv[1] );
	// }
	// parse args
	NUM_THREADS = 1;
	if(argc==3){
		allocator = atoi( argv[1] );
		NUM_THREADS = atoi( argv[2] );
	}
	global_init();
	printf("Num Threads = %zu\n",NUM_THREADS);
	// launch threads
	int ret; size_t i;
	for(i=1; i<NUM_THREADS; i++){
		args[i]=i;
		printf("creating thread %zu\n",args[i]);
		ret = pthread_create(&threads[i], NULL, &thread_main, &args[i]);
		if(ret){
			printf("ERROR; pthread_create: %d\n", ret);
			exit(-1);
		}
	}
	i = 0;
	thread_main(&i); // master also calls thread_main
	
	// join threads
	for(size_t i=1; i<NUM_THREADS; i++){
		ret = pthread_join(threads[i],NULL);
		if(ret){
			printf("ERROR; pthread_join: %d\n", ret);
			exit(-1);
		}
		printf("joined thread %zu\n",i);
	}
	
	global_cleanup();
	
	unsigned long long elapsed_ns;
	elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
	printf("Elapsed (ns): %llu\n",elapsed_ns);
	double elapsed_s = ((double)elapsed_ns)/1000000000.0;
	printf("Elapsed (s): %lf\n",elapsed_s);
	printf("%lf\n",elapsed_s);
	
}
