#include <stdio.h>

/*
 * 用十进制数组做长乘长除，逐项算
 *   pi / 2 = 1 + 1/3 * (1 + 2/5 * (1 + 3/7 * (...)))
 * 每一项乘 term 再除以 2*term+1，从级数尾部往前推。
 */
enum
{
	DECIMAL_DIGITS = 2800,
	GUARD_DIGITS = 32,
	STORED_DIGITS = DECIMAL_DIGITS + GUARD_DIGITS,
	SERIES_TERMS = (STORED_DIGITS + 1) * 14 / 4 + 14
};

static int answer[STORED_DIGITS + 1];

int main(void)
{
	for (int term = SERIES_TERMS; term >= 1; --term)
	{
		int carry = 0;
		for (int i = STORED_DIGITS; i > 0; --i)
		{
			const int value = answer[i] * term + carry;
			answer[i] = value % 10;
			carry = value / 10;
		}
		answer[0] = answer[0] * term + carry;

		const int denominator = 2 * term + 1;
		int remainder = 0;
		for (int i = 0; i <= STORED_DIGITS; ++i)
		{
			const int value = answer[i] + 10 * remainder;
			answer[i] = value / denominator;
			remainder = value % denominator;
		}
		answer[0] += 1;
	}

	if (printf("%d.", answer[0]) < 0)
	{
		return 1;
	}
	for (int i = 1; i <= DECIMAL_DIGITS; ++i)
	{
		if (putchar('0' + answer[i]) == EOF)
		{
			return 1;
		}
	}

	if (putchar('\n') == EOF)
	{
		return 1;
	}

	return fflush(stdout) != 0 || ferror(stdout) ? 1 : 0;
}
