#include <stdio.h>
#include <math.h>

int main()
{
	int i;
	for (i=0;i<=200;++i)
	{
		int v = (int)pow(10,(50+i)/49.828);
		printf("%d,",v);
		if (i%10==9) printf("\n");
	}
	return 0;
}