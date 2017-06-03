/**
*	2016.8.3 by kolonse
*	because the json proto is large some time,so this code be compress json to reduce our bytes
*	
*/
#ifndef __KOLONSE_JSON_COMPRESS__
#define __KOLONSE_JSON_COMPRESS__

#ifdef _WINDLL
	#ifdef _WIN32
		#define KL_EXPORT _declspec(dllexport)
	#else
		#define KL_EXPORT
	#endif
#else
	#define KL_EXPORT
#endif

#ifdef _DEBUG
#define ALG_DEBUG 1
#else
#define ALG_DEBUG 0
#endif

#define ALG_NUMBER_HASH 0

// select alg of compree
#define USE_COMPRESS_ALG ALG_NUMBER_HASH

// init : must be called when your system is starting.
// param : key[seq] key[seq]
// seq is dis of str
extern "C" KL_EXPORT int Init(const char* str,const char* seq);
/**
*	compress json string function
*	jsonOut must have be malloced,and buff size suggest same as jsonIn
*/
extern "C" KL_EXPORT int Compress(const char* jsonIn,char* jsonOut,int* outSize);
/**
*	uncompress json string function
*	jsonOut size suggest your max buff size
*/
extern "C" KL_EXPORT int Uncompress(const char* jsonIn,char* jsonOut,int* outSize);

#endif
