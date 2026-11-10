#include <stdio.h>

/* 普通浮点除法：2/3 的精度就到这儿了。 */
int main(void)
{
	const int numerator = 2;
	const int denominator = 3;

	if (printf("%.6f\n", (double)numerator / (double)denominator) < 0)
	{
		return 1;
	}

	return fflush(stdout) != 0 || ferror(stdout) ? 1 : 0;
}
