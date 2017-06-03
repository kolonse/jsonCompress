#include "jc.h"

#define STRING_VALUE_MAX_LENGTH 1024*10
#define STACK_MAX_LENGTH 1024 * 10
#define ASSIGN_AND_SHIFT(o,n,p) \
	o[*n] = *p; \
	p ++; \
	(*n) ++;
#define ASSIGN_CHAR_AND_SHIFT(o,n,c) \
	o[*n] = c; \
	(*n) ++;
typedef enum {
	_ERROR_ = -1,
	_NONE_,
	_READ_KEY_,
	_READ_COLON_, //the mark of  : 
	_READ_KEY_VALUE_,
	_END_,

	_PARSE_OBJECT_,
	_PARSE_ARRAY_
}STATE;
typedef STATE* PSTATE  ;

#define STACK_STATE_NULL _ERROR_
/////////////////////////////////////// 算法函数  ////////////////////////////////////////

int AlgInit(const char* param);
void GetValueByKey(const char* in,int from,int to,char* out,int* size);
void GetKeyByValue(const char* in,int from,int to,char* out,int* size);
const char* _ReadToFlags(const char* flags,const char* str,char* out,int* outSize);
const char* _ReadTo(char endFlag,const char* str,char* out,int* outSize);
///////////////////////////////////////// end //////////////////////////////////////////////

#if USE_COMPRESS_ALG == ALG_NUMBER_HASH
#include "./algNumberHash.cc.kl"
#endif
const char* _ReadKey(const char* in);
const char* _ReadValue(const char* in,char* out,int* outSize);
const char* _ReadNumber(const char* in,char* out,int* outSize);
const char* _CompressArray(PSTATE stack, int* pos,STATE* state,const  char* in,char* jsonOut,int* outSize);
const char* _UncompressArray(PSTATE stack, int* pos,STATE* state, const char* in,char* jsonOut,int* outSize);
char* cpy(char *dst,const char *src)  
{
    while(*src != '\0')
	{
		*dst = *src ;
		dst ++ ;
		src ++ ;
	}
	return dst ;
}

void _StackInit(PSTATE stack, int* pos)
{
	*pos = 1;
	stack[0] = STACK_STATE_NULL;
}

STATE _CurState(PSTATE stack, int* pos)
{
	return stack[*pos - 1];
}

STATE _Pop(PSTATE stack, int* pos)
{
	if(*pos <= 0)
	{
		return STACK_STATE_NULL;
	}
	return stack[--(*pos)];
}

void _Push(PSTATE stack, int* pos,STATE state)
{
	stack[(*pos) ++] = state ;
}

const char* _CompressObject(PSTATE stack, int* pos,STATE* state, const char* in,char* jsonOut,int* outSize)
{
	char key[COMPRESS_KEY_MAX_LENGTH] = {0};
	int ki = 0;
	const char *p = in ;
	const char *t = 0 ;
	STATE tState = _READ_KEY_ ;
	STATE stackState = STACK_STATE_NULL;
	unsigned char end = 0 ;
	while(*p != '\0')
	{
		switch(*p)
		{
		case '"':
			switch(tState)
			{
			case _READ_KEY_:

				tState = _READ_COLON_ ;
				t = p + 1;
				p = _ReadKey(p + 1);
				GetValueByKey(in,t - in,p - in,key,&ki);
				*outSize = cpy(&(jsonOut[*outSize]),key) - jsonOut;
				p ++ ;
				break ;
			case _READ_KEY_VALUE_:
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _ReadValue(p,jsonOut,outSize);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				tState = _END_ ;
				break ;
			default:
				tState = _ERROR_;
				break ;
			}
			break;
		case ':':
			switch (tState)
			{
			case _READ_COLON_:
				tState = _READ_KEY_VALUE_ ;
				break;
			default:
				break;
			}
			ASSIGN_AND_SHIFT(jsonOut,outSize,p);
			break ;
		case '{':
			if(_READ_KEY_VALUE_ == tState)
			{
				_Push(stack,pos,_PARSE_OBJECT_);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _CompressObject(stack,pos,state,p,jsonOut,outSize);
				tState = _END_ ;
			}
			else
			{
				tState = _ERROR_;
			}
			break;
		case '[':
			// 处理 数组
			if(_READ_KEY_VALUE_ == tState)
			{
				_Push(stack,pos,_PARSE_ARRAY_);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _CompressArray(stack,pos,state,p,jsonOut,outSize);
				tState = _END_ ;
			}
			else
			{
				tState = _ERROR_;
			}
			break ;
		case '}':
			if( tState == _END_  || tState == _READ_KEY_)
			{
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				// 当前 object  处理完成
				end = 1;
			}
			else
			{
				tState = _ERROR_ ;
			}
			break ;
		case ',':
			if( tState == _END_ )
			{
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				tState = _READ_KEY_ ;
			}
			else
			{
				tState = _ERROR_ ;
			}
			break ;
		case '\r':
		case '\n':
		case ' ':
			// 无效字符直接去除
			break ;
		default:
			// 这里要处理 数字 true/false 情况
			switch(tState)
			{
			case _READ_KEY_VALUE_:
				p = _ReadNumber(p,jsonOut,outSize);
				tState = _END_ ;
				break ;
			default:
				tState = _ERROR_;
				break ;
			}
			break ;
		}
		if(*state == _ERROR_)
		{
			break;
		}
		if(end)
		{
			stackState = _Pop(stack,pos);
			if(stackState != _PARSE_OBJECT_)
			{
				*state = _ERROR_ ;
			}
			break ;
		}
		if(tState == _ERROR_)
		{
			*state = _ERROR_ ;
			break;
		}
	}

	return p;
}

const char* _UncompressObject(PSTATE stack, int* pos,STATE* state, const char* in,char* jsonOut,int* outSize)
{
	char key[UNCOMPRESS_KEY_MAX_LENGTH] = {0};
	char uckey[COMPRESS_KEY_MAX_LENGTH] = {0};
	int ki = 0,ucki = 0;
	const char *p = in ;
	STATE tState = _READ_KEY_ ;
	STATE stackState = STACK_STATE_NULL;
	unsigned char end = 0 ;
	while(*p != '\0')
	{
		switch(*p)
		{
		case '"':
			switch(tState)
			{
			case _READ_KEY_VALUE_:
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _ReadValue(p,jsonOut,outSize);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				tState = _END_ ;
				break ;
			default:
				tState = _ERROR_;
				break ;
			}
			break;
		case '{':
			if(_READ_KEY_VALUE_ == tState)
			{
				_Push(stack,pos,_PARSE_OBJECT_);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _UncompressObject(stack,pos,state,p,jsonOut,outSize);
				tState = _END_ ;
			}
			else
			{
				tState = _ERROR_;
			}
			break;
		case '[':
			// 处理 数组
			if(_READ_KEY_VALUE_ == tState)
			{
				_Push(stack,pos,_PARSE_ARRAY_);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _UncompressArray(stack,pos,state,p,jsonOut,outSize);
				tState = _END_ ;
			}
			else
			{
				tState = _ERROR_;
			}
			break ;
		case '}':
			if( tState == _END_ || tState == _READ_KEY_ )
			{
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				// 当前 object  处理完成
				end = 1;
			}
			else
			{
				tState = _ERROR_ ;
			}
			break ;
		case ',':
			if( tState == _END_ )
			{
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				tState = _READ_KEY_ ;
			}
			else
			{
				tState = _ERROR_ ;
			}
			break ;
		case '\r':
		case '\n':
		case ' ':
			// 无效字符直接去除
			break ;
		default:
			switch(tState)
			{
			case _READ_KEY_:
				ASSIGN_CHAR_AND_SHIFT(jsonOut,outSize,'"');
				_ReadTo(':',p,key,&ki);
				GetKeyByValue(key,0,ki,uckey,&ucki);
				*outSize = cpy(&(jsonOut[*outSize]),uckey) - jsonOut;
				ASSIGN_CHAR_AND_SHIFT(jsonOut,outSize,'"');
				ASSIGN_CHAR_AND_SHIFT(jsonOut,outSize,':');
				p += ki;
				if(*p != '\0')
				{
					p ++;
				}
				tState = _READ_KEY_VALUE_ ;
				break ;
			// 这里要处理 数字 true/false 情况
			case _READ_KEY_VALUE_:
				p = _ReadNumber(p,jsonOut,outSize);
				tState = _END_ ;
				break ;
			default:
				tState = _ERROR_;
				break ;
			}
			break ;
		}
		if(*state == _ERROR_)
		{
			break;
		}
		if(end)
		{
			stackState = _Pop(stack,pos);
			if(stackState != _PARSE_OBJECT_)
			{
				*state = _ERROR_ ;
			}
			break ;
		}
		if(tState == _ERROR_)
		{
			*state = _ERROR_ ;
			break;
		}
	}

	return p;
}

const char* _CompressArray(PSTATE stack, int* pos,STATE* state, const char* in,char* jsonOut,int* outSize)
{
	const char *p = in ;
	STATE tState = _READ_KEY_VALUE_ ;
	STATE stackState = STACK_STATE_NULL;
	unsigned char end = 0 ;
	while(*p != '\0')
	{
		switch(*p)
		{
		case '"':
			switch(tState)
			{
			case _READ_KEY_VALUE_:
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _ReadValue(p,jsonOut,outSize);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				tState = _END_ ;
				break ;
			default:
				tState = _ERROR_;
				break ;
			}
			break;
		case '{':
			if(_READ_KEY_VALUE_ == tState)
			{
				_Push(stack,pos,_PARSE_OBJECT_);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _CompressObject(stack,pos,state,p,jsonOut,outSize);
				tState = _END_ ;
			}
			else
			{
				tState = _ERROR_;
			}
			break;
		case '[':
			// 处理 数组
			if(_READ_KEY_VALUE_ == tState)
			{
				_Push(stack,pos,_PARSE_ARRAY_);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _CompressArray(stack,pos,state,p,jsonOut,outSize);
				tState = _END_ ;
			}
			else
			{
				tState = _ERROR_;
			}
			break ;
		case ']':
			if( tState == _END_ || tState == _READ_KEY_VALUE_)
			{
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				// 当前 object  处理完成
				end = 1;
			}
			else
			{
				tState = _ERROR_ ;
			}
			break ;
		case ',':
			if( tState == _END_ )
			{
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				tState = _READ_KEY_VALUE_ ;
			}
			else
			{
				tState = _ERROR_ ;
			}
			break ;
		case '\r':
		case '\n':
		case ' ':
			// 无效字符直接去除
			break ;
		default:
			// 这里要处理 数字 true/false 情况
			switch(tState)
			{
			case _READ_KEY_VALUE_:		
				p = _ReadNumber(p,jsonOut,outSize);
				tState = _END_ ;
				break ;
			default:
				tState = _ERROR_;
				break ;
			}
			break ;
		}
		if(*state == _ERROR_)
		{
			break;
		}
		if(end)
		{
			stackState = _Pop(stack,pos);
			if(stackState != _PARSE_ARRAY_)
			{
				*state = _ERROR_ ;
			}
			break ;
		}
		if(tState == _ERROR_)
		{
			*state = _ERROR_ ;
			break;
		}
	}

	return p;
}

const char* _UncompressArray(PSTATE stack, int* pos,STATE* state, const char* in,char* jsonOut,int* outSize)
{
	const char *p = in ;
	STATE tState = _READ_KEY_VALUE_ ;
	STATE stackState = STACK_STATE_NULL;
	unsigned char end = 0 ;
	while(*p != '\0')
	{
		switch(*p)
		{
		case '"':
			switch(tState)
			{
			case _READ_KEY_VALUE_:
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _ReadValue(p,jsonOut,outSize);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				tState = _END_ ;
				break ;
			default:
				tState = _ERROR_;
				break ;
			}
			break;
		case '{':
			if(_READ_KEY_VALUE_ == tState)
			{
				_Push(stack,pos,_PARSE_OBJECT_);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _UncompressObject(stack,pos,state,p,jsonOut,outSize);
				tState = _END_ ;
			}
			else
			{
				tState = _ERROR_;
			}
			break;
		case '[':
			// 处理 数组
			if(_READ_KEY_VALUE_ == tState)
			{
				_Push(stack,pos,_PARSE_ARRAY_);
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				p = _UncompressArray(stack,pos,state,p,jsonOut,outSize);
				tState = _END_ ;
			}
			else
			{
				tState = _ERROR_;
			}
			break ;
		case ']':
			if( tState == _END_ || tState == _READ_KEY_VALUE_ )
			{
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				// 当前 object  处理完成
				end = 1;
			}
			else
			{
				tState = _ERROR_ ;
			}
			break ;
		case ',':
			if( tState == _END_ )
			{
				ASSIGN_AND_SHIFT(jsonOut,outSize,p);
				tState = _READ_KEY_VALUE_ ;
			}
			else
			{
				tState = _ERROR_ ;
			}
			break ;
		case '\r':
		case '\n':
		case ' ':
			// 无效字符直接去除
			break ;
		default:
			// 这里要处理 数字 true/false 情况
			switch(tState)
			{
			case _READ_KEY_VALUE_:		
				p = _ReadNumber(p,jsonOut,outSize);
				tState = _END_ ;
				break ;
			default:
				tState = _ERROR_;
				break ;
			}
			break ;
		}
		if(*state == _ERROR_)
		{
			break;
		}
		if(end)
		{
			stackState = _Pop(stack,pos);
			if(stackState != _PARSE_ARRAY_)
			{
				*state = _ERROR_ ;
			}
			break ;
		}
		if(tState == _ERROR_)
		{
			*state = _ERROR_ ;
			break;
		}
	}

	return p;
}

const char* _ReadKey(const char* in)
{
	const char *p = in ;
	unsigned char escape = 0 ;/* the mark of \\   */
	unsigned char end = 0 ;
	while(*p != '\0')
	{
		switch (*p)
		{
		case '\\':
			if(escape)
			{
				escape = 0;
			}
			else
			{
				escape = 1;
			}		
			break;
		case '"':
			if(!escape)
			{
				// the end of string
				end = 1 ;
			}
			break ;
		default:
			break;
		}

		if(end)
		{
			break ;
		}
		else
		{
			p ++;
		}
	}
	return p;
}
const char* _ReadValue(const char* in,char* out,int* outSize)
{
	const char *p = in ;
	unsigned char escape = 0 ;/* the mark of \\   */
	unsigned char end = 0 ;
	while(*p != '\0')
	{
		switch (*p)
		{
		case '\\':
			if(escape)
			{
				escape = 0;
			}
			else
			{
				escape = 1;
			}		
			break;
		case '"':
			if(!escape)
			{
				// the end of string
				end = 1 ;
			}
			break ;
		default:
			break;
		}

		if(end)
		{
			break ;
		}
		else
		{
			ASSIGN_AND_SHIFT(out,outSize,p);
		}
	}
	return p;
}
const char* _ReadNumber(const char* in,char* out,int* outSize)
{
	const char *p = in ;
	while(*p != '\0')
	{
		if( *p == ',' || *p == '}' || *p == ']')
		{
			return p ;
		}
		else
		{
			ASSIGN_AND_SHIFT(out,outSize,p);
		}
	}
	return p;
}

void _CacuSystem(const char* systemString,const int system,int tenSystemValue,char* out,int* site)
{
	int v = 0 ;
	if( tenSystemValue < system )
	{
		out[*site] = systemString[tenSystemValue];
		(*site) ++;
	}
	else
	{
		v = tenSystemValue % system ;
		out[*site] = systemString[v];
		(*site) ++;
		_CacuSystem(systemString,system,(tenSystemValue - v) / system,out,site);
	}
}

const char* DIY_SYSTEM_STRING = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const int DIY_SYSTEM = 62;

extern "C" int Init(const char* str,const char* seq)
{
	int ret = 0;//AlgInit(str);
	char key[COMPRESS_KEY_MAX_LENGTH] = {0};
	int kSize = 0 ;
	char strIn[DIC_MAX_LENGTH * COMPRESS_KEY_MAX_LENGTH] = {0};
	int iSize = 0 ;
	int count = 0 ;
	char value[DIC_VALUE_MAX_LENGTH] = {0};
	int vSize = 0 ;
	while(*str != 0)
	{
		str = _ReadToFlags(seq,str,key,&kSize);
		if(kSize != 0)
		{
			key[kSize] = '\0';
			cpy(&(strIn[iSize]),key);
			iSize += kSize ;
			ASSIGN_CHAR_AND_SHIFT(strIn,(&iSize),':');
			vSize = 0 ;
			_CacuSystem(DIY_SYSTEM_STRING,DIY_SYSTEM,count,value,&vSize);
			value[vSize] = '\0';
			cpy(&(strIn[iSize]),value);
			iSize += vSize ;
			ASSIGN_CHAR_AND_SHIFT(strIn,(&iSize),'|');
			count ++ ;
		}
		if(*str == '\0')
		{
			break ;
		}
		str ++ ;
	}
	if( iSize > 0 )
	{
		strIn[iSize - 1] = '\0';
	}

	ret = AlgInit(strIn);
	return ret;
}

extern "C" int Compress(const char* jsonIn,char* jsonOut,int* outSize)
{
	STATE gStack[STACK_MAX_LENGTH];
	int gStackPos = 0;
	STATE state = _NONE_;
	const char *p = jsonIn ;
	*outSize = 0;
	_StackInit(gStack,&gStackPos);
	while(*p != '\0')
	{
		switch (*p)
		{
		case '{':
			_Push(gStack,&gStackPos,_PARSE_OBJECT_);
			ASSIGN_AND_SHIFT(jsonOut,outSize,p);
			p = _CompressObject(gStack,&gStackPos,&state,p,jsonOut,outSize);
			break;
		case '[':
			_Push(gStack,&gStackPos,_PARSE_ARRAY_);
			ASSIGN_AND_SHIFT(jsonOut,outSize,p);
			p = _CompressArray(gStack,&gStackPos,&state,p,jsonOut,outSize);
			break;
		case ' ':
		case '\r':
		case '\n':
			p ++;
			break ;
		default:
			state = _ERROR_;
			break;
		}
		if(state == _ERROR_)
		{
			return p - jsonIn;
		}
		//p ++;
	}
	if(_CurState(gStack,&gStackPos) != STACK_STATE_NULL)
	{
		return p - jsonIn;
	}
	return 0;
}

extern "C" int Uncompress(const char* jsonIn,char* jsonOut,int* outSize)
{
	STATE gStack[STACK_MAX_LENGTH];
	int gStackPos = 0;
	STATE state = _NONE_;
	const char *p = jsonIn ;
	*outSize = 0;
	_StackInit(gStack,&gStackPos);
	while(*p != '\0')
	{
		switch (*p)
		{
		case '{':
			_Push(gStack,&gStackPos,_PARSE_OBJECT_);
			ASSIGN_AND_SHIFT(jsonOut,outSize,p);
			p = _UncompressObject(gStack,&gStackPos,&state,p,jsonOut,outSize);
			break;
		case '[':
			_Push(gStack,&gStackPos,_PARSE_ARRAY_);
			ASSIGN_AND_SHIFT(jsonOut,outSize,p);
			p = _UncompressArray(gStack,&gStackPos,&state,p,jsonOut,outSize);
			break;
		case ' ':
		case '\r':
		case '\n':
			p ++;
			break ;
		default:
			state = _ERROR_;
			break;
		}
		if(state == _ERROR_)
		{
			return p - jsonIn;
		}
		//p ++;
	}
	if(_CurState(gStack,&gStackPos) != STACK_STATE_NULL)
	{
		return p - jsonIn;
	}
	return 0;
}
