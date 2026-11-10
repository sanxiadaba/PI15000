/*
 * 用 spigot 算法逐组生成 pi 的数字。
 *
 * 级数以 10000 为基数，每消耗 14 个级数项产出一组 4 位数字，也就是平均
 * 每个数字 3.5 项。余数数组随轮次从高位往低位收缩，空间 O(n)、时间 O(n^2)。
 *
 * 每组算完可能满 10000，也就是比 4 位多出一位：那一位属于刚写下的前一组，
 * 必须回填进去。漏掉这一步，几十万位以内就会在某个位置算错一位，随后整组
 * 变成 10000 而无法用 4 位表示。
 */

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

enum
{
	BASE = 10000,
	INITIAL_REMAINDER = BASE / 5,
	TERMS_PER_BLOCK = 14,
	DIGITS_PER_BLOCK = 4,
	DEFAULT_DECIMAL_DIGITS = 15000
};

static void print_usage(FILE *stream, const char *program)
{
	fprintf(stream,
		"用法：%s [小数位数]\n"
		"省略参数时输出 %d 位；位数是截断的，不做四舍五入。\n",
		program, DEFAULT_DECIMAL_DIGITS);
}

static int parse_decimal_digits(const char *text, size_t *value)
{
	char *end = NULL;
	uintmax_t parsed;

	if (text[0] < '0' || text[0] > '9')
	{
		return 0;
	}

	errno = 0;
	parsed = strtoumax(text, &end, 10);
	if (errno == ERANGE || *end != '\0' || parsed > (uintmax_t)SIZE_MAX)
	{
		return 0;
	}

	*value = (size_t)parsed;
	return 1;
}

/* 把 carry 加到 digits[0..count) 这个十进制数上，处理连续的 9。 */
static int add_carry(char *digits, size_t count, uint64_t carry)
{
	while (carry != 0)
	{
		unsigned digit;

		if (count == 0)
		{
			return 0;
		}
		--count;
		digit = (unsigned)(digits[count] - '0') + (unsigned)(carry % 10);
		carry /= 10;
		if (digit >= 10)
		{
			digit -= 10;
			++carry;
		}
		digits[count] = (char)('0' + digit);
	}

	return 1;
}

static int write_pi(size_t decimal_digits)
{
	size_t significant_digits;
	size_t blocks;
	size_t terms;
	size_t digit_count;
	size_t written = 0;
	uint64_t accumulator = 0;
	uint64_t *remainders;
	char *digits;
	int status = 0;

	if (decimal_digits > (SIZE_MAX - 16) / 8)
	{
		fprintf(stderr, "位数太大。\n");
		return 0;
	}

	significant_digits = decimal_digits + 1; /* 整数位 3 加上 d 位小数 */
	blocks = (significant_digits + DIGITS_PER_BLOCK - 1) / DIGITS_PER_BLOCK;
	terms = blocks * TERMS_PER_BLOCK;
	digit_count = blocks * DIGITS_PER_BLOCK;

	/*
	 * 内层循环里 accumulator 不超过 2002 * BASE，remainders[] 不超过 2 * terms，
	 * 于是 accumulator * index + remainders[index] * BASE < 2002 * BASE * terms。
	 * 限住 terms 就保证这个乘积落在 uint64_t 里；实际上 malloc 会先失败。
	 */
	if (terms > UINT64_C(3000000000))
	{
		fprintf(stderr, "位数太大。\n");
		return 0;
	}

	remainders = malloc((terms + 1) * sizeof(*remainders));
	digits = malloc(digit_count);
	if (remainders == NULL || digits == NULL)
	{
		fprintf(stderr, "内存不足，算不了这么多位。\n");
		free(remainders);
		free(digits);
		return 0;
	}

	for (size_t i = 0; i <= terms; ++i)
	{
		remainders[i] = INITIAL_REMAINDER;
	}

	for (size_t depth = terms; depth > 0; depth -= TERMS_PER_BLOCK)
	{
		size_t index = depth;
		uint64_t low = accumulator % BASE;
		uint64_t group;

		accumulator = low;
		while (index > 0)
		{
			uint64_t denominator = UINT64_C(2) * (uint64_t)index - 1;

			accumulator = accumulator * (uint64_t)index +
				remainders[index] * BASE;
			remainders[index] = accumulator % denominator;
			accumulator /= denominator;
			--index;
		}

		group = low + accumulator / BASE;
		if (group >= BASE)
		{
			if (!add_carry(digits, written, group / BASE))
			{
				fprintf(stderr, "内部计算溢出。\n");
				goto done;
			}
			group %= BASE;
		}

		for (size_t k = DIGITS_PER_BLOCK; k > 0; --k)
		{
			digits[written + k - 1] = (char)('0' + (int)(group % 10));
			group /= 10;
		}
		written += DIGITS_PER_BLOCK;
	}

	if (written < significant_digits)
	{
		fprintf(stderr, "内部计算不足位数。\n");
		goto done;
	}

	if (fwrite(digits, 1, 1, stdout) != 1)
	{
		fprintf(stderr, "写输出失败。\n");
		goto done;
	}
	if (decimal_digits > 0)
	{
		if (fputc('.', stdout) == EOF ||
			fwrite(digits + 1, 1, decimal_digits, stdout) != decimal_digits)
		{
			fprintf(stderr, "写输出失败。\n");
			goto done;
		}
	}
	if (fputc('\n', stdout) == EOF)
	{
		fprintf(stderr, "写输出失败。\n");
		goto done;
	}

	status = 1;

done:
	free(remainders);
	free(digits);
	return status;
}

int main(int argc, char **argv)
{
	size_t decimal_digits = DEFAULT_DECIMAL_DIGITS;
	int written;

#ifdef _WIN32
	/* 各平台都输出 LF，文件比对不用再管换行。 */
	(void)_setmode(_fileno(stdout), _O_BINARY);
#endif

	if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))
	{
		print_usage(stdout, argv[0]);
		return 0;
	}
	if (argc > 2 || (argc == 2 && !parse_decimal_digits(argv[1], &decimal_digits)))
	{
		print_usage(stderr, argv[0]);
		return 2;
	}

	written = write_pi(decimal_digits);
	if (!written)
	{
		return 1;
	}

	return fflush(stdout) != 0 || ferror(stdout) ? 1 : 0;
}
