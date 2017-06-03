#include <node.h>
#include <v8.h>
#include "../jc.h"
#include <stdlib.h>
#include <string.h>
using namespace v8;

#define COMMPRESS_MAX_LENGTH 1024 * 10  // 压缩的最大长度 只支持 10k 如果10k 那么原字符串返回

int gInitSuccess = 0 ; // 必须保证初始成功才能调用后续函数
Handle<Value> GlobalInit(const Arguments& args) {
	HandleScope scope;
	int result = 0;
	const char* param = 0;
	const char* seq = 0 ;
	if (args.Length() != 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		result = -1 ;
	}
	else
	{
		String::Utf8Value pParam(args[0]);
		String::Utf8Value pSeq(args[1]);
		param = (char*)*pParam;
		seq = (char*)*pSeq;
		result = Init(param,seq);
		gInitSuccess = 1 ;
	}
	return scope.Close(Integer::New(result));
}

//FILE* fp = NULL ;
Handle<Value> JCompress(const Arguments& args) {
	HandleScope scope;
	char out[COMMPRESS_MAX_LENGTH] = {0};
	int size = 0 ;
	int result = 0 ;
	if (args.Length() != 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	if(!gInitSuccess)
	{
		return scope.Close(args[0]);
	}
	String::Utf8Value ppIn(args[0]);
	const char* pIn = *ppIn;
	Local<Integer> pLen = args[1]->ToInteger();
	int32_t len = pLen->Int32Value();
	
	if( len >= COMMPRESS_MAX_LENGTH )
	{
		return scope.Close(args[0]);
	}
	
	result = Compress(pIn,out,&size);
	if(result != 0)
	{
		return scope.Close(args[0]);
	}
	
	out[size] = '\0';
	Local<v8::String> res = String::New(out,size);
	return scope.Close(res);
}


Handle<Value> JUncompress(const Arguments& args) {
	HandleScope scope;
	char out[COMMPRESS_MAX_LENGTH] = {0};
	int size = 0 ;
	int result = 0 ;
	if (args.Length() != 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	if(!gInitSuccess)
	{
		return scope.Close(args[0]);
	}	
	String::Utf8Value ppIn(args[0]);
	const char* pIn = (char*)*ppIn ;
	
	Local<Integer> pLen = args[1]->ToInteger();
    int32_t len = pLen->Int32Value();
	result = Uncompress(pIn,out,&size);
	if(result != 0)
	{
		return scope.Close(args[0]);
	}
	out[size] = '\0';
	Local<v8::String> res = String::New(out,size);
	return scope.Close(res);
}

void JCInit(Handle<Object> exports) {
	exports->Set(String::NewSymbol("GlobalInit"),
		FunctionTemplate::New(GlobalInit)->GetFunction());

	exports->Set(String::NewSymbol("JCompress"),
		FunctionTemplate::New(JCompress)->GetFunction());

	exports->Set(String::NewSymbol("JUncompress"),
		FunctionTemplate::New(JUncompress)->GetFunction());
}

NODE_MODULE(jclib, JCInit);