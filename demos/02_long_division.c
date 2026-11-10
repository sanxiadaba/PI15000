#include <stdint.h>
#include <stdio.h>

/* 这个有理数近似与 pi 的前 34 位小数一致。 */
enum { DECIMAL_DIGITS = 34 };

int main(void)
{
	uint64_t numerator = UINT64_C(430010946591069243);
	const uint64_t denominator = UINT64_C(136876735467187340);
	uint64_t remainder = numerator % denominator;
	/* 整数部分就是 3，只有一位，直接当字符写。 */
	const int whole = (int)(numerator / denominator);

	if (putchar((int)('0' + whole)) == EOF || putchar('.') == EOF)
	{
		return 1;
	}

	for (int i = 0; i < DECIMAL_DIGITS; ++i)
	{
		numerator = remainder * UINT64_C(10);
		if (putchar((int)('0' + numerator / denominator)) == EOF)
		{
			return 1;
		}
		remainder = numerator % denominator;
	}

	if (putchar('\n') == EOF)
	{
		return 1;
	}

	return fflush(stdout) != 0 || ferror(stdout) ? 1 : 0;
}
