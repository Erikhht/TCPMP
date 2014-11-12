#include <stdlib.h>
#include <stdio.h>

int main(int pn,const char** ps)
{
	FILE *e,*f;
	int head[3];
	int pos,n,i;
	char buf[4096];

	if (pn<=1)
	{
		printf("usage: %s <input> <output>\n",ps[0]);
		return 1;
	}

	srand((unsigned)time(NULL));
	
	f = fopen(ps[1],"rb");
	e = fopen(ps[2],"wb+");
	if (!f || !e)
		return 1;

	fseek(f,0,SEEK_END);

	head[0] = 0x44434241;
	head[1] = ftell(f);
	head[2] = pos = rand();
	fwrite(head,1,sizeof(head),e);

	fseek(f,0,SEEK_SET);
	while ((n=fread(buf,1,sizeof(buf),f))>0)
	{
		for (i=0;i<n;++i)
			buf[i] = buf[i] ^ pos++;
		fwrite(buf,1,n,e);
	}


	return 0;
}