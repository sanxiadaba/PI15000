#include <stdio.h>

/* Demonstrate the precision of an ordinary floating-point division. */
int main(void)
{
	const int numerator = 2;
	const int denominator = 3;

	printf("%.6f\n", (double)numerator / (double)denominator);
	return 0;
}
