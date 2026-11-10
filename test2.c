#include <stdio.h>

/* Demonstrate the precision of an ordinary floating-point division. */
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
