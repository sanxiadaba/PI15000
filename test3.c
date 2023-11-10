#include <stdio.h>
#define ll long long int
//  用长除法计算大数相除
ll a = 430010946591069243;
ll b = 136876735467187340;

ll ans_arr[100] = {0};

int main(void)
{
	ll shang = 0;
	ll yushu = 0;
	for (int i = 0; i < 100; i++)
	{
		shang = a / b;
		yushu = a - (b * shang);
		ans_arr[i] = shang;
		a = yushu * 10;
	}

	printf("%I64d.", ans_arr[0]);
	for (int i = 1; i < 100; i++)
	{
		printf("%I64d", ans_arr[i]);
	}
	return 0;
}
