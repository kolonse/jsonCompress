#ifdef _DEBUG
#include "jc.h"
#include <stdio.h>
#include <Windows.h>
#define TREE_NODE_NIL_STATUS(tree,x) ((tree[x][0] & 0x40) >> 6)
int main()
{
	char *in = "{\"roomID\":\"101,10\",\"tableID\":3,\"chairID\":1,\"gunValue\":\"350\",\"gunID\":14,\"count\":1,\"list\":[],\"list2\":{},\"fishList\":[{\"fishID\":14,\"fishType\":2},{\"fishID\":14,\"fishType\":2},{\"fishID\":14,\"fishType\":2},432,\"fdsfsd\"]}";
	char *in2 = "{\"key1\":fdsf,\"list\":[]}";
	char out[1024] = {0};
	char out2[1024] = {0};
	int size = 0;
	int size2 = 0;

	char test[10][10] = {0};
	
	//Init("1:13|2:1|3:11|4:2|5:3|6:4|7:9|8:4|9:6|a:7|b:8|c:10|d:12|e:14");
	
	Init("roomID|tableID|chairID|gunValue|gunID|fishList|fishID|fishType|count","|");
	
	DWORD all = 0;
	for(int i = 0;i < 1;i ++)
	{
		DWORD b = GetTickCount();
		int code = Compress(in2,out,&size);

		printf("code:%d %s\n",code,out);

		int code2 = Uncompress(out,out2,&size2);

		printf("code2:%d %s\n",code2,out2);

		all += GetTickCount() - b ;
	}
	
	printf("cost :%lf s\n",all / 1000.0);
	//unsigned short x = TREE_NODE_NIL_STATUS(test,1);
	system("pause");
	return 0;
}
#endif
