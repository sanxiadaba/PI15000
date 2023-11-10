#include <stdio.h>

// 计算PI前n位

// 计算多少位PI
#define PI_NUM 20000
#define NUM (PI_NUM * 14 / 4)
#define int long long int

// 数组a用来保存误差
int a[NUM + 1];
int b = NUM;
int c = NUM;
int d = 0, e = 0;

int main(void)
{
	for (int i = 0; i < NUM + 1; i++)
	{
		// 问题一：为什么乘以两千
		a[i] = 2000;
	}

	// 问题二：迭代算法的形式不一样啊
	while (c != 0)
	{
		b = c;
		e = d % 10000;
		d = e;
		while (b != 0)
		{
			d = d * b +  a[b] * 10000;
			a[b] = d % (2 * b - 1);
			d /= (2 * b - 1);
			b--;
		}
		// 问题三：为什么要-=14
		c -= 14;
		printf("%04d", e + d / 10000);
	}
	return 0;
}

