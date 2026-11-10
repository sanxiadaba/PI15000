/*
 * 用 spigot 算法逐组生成 pi 的数字。
 *
 * 级数以 1e9 为基数，每消耗 30 个级数项产出一组 9 位数字，也就是平均
 * 每个数字 3.33 项。余数数组随轮次从高位往低位收缩，空间 O(n)、时间 O(n^2)。
 *
 * 每组算完可能满 1e9，也就是比 9 位多出一位：那一位属于刚写下的前一组，
 * 必须回填进去。漏掉这一步，几万位之后就会在某个位置差一位，随后整组
 * 变成 1e9 而无法用 9 位表示。
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
#include <windows.h>
#endif

enum
{
	BASE = 1000000000,
	INITIAL_REMAINDER = BASE / 5,
	TERMS_PER_BLOCK = 30,
	DIGITS_PER_BLOCK = 9,
	DEFAULT_DECIMAL_DIGITS = 15000
};

/*
 * 提示信息是中文、源码是 UTF-8。Windows 控制台默认按本地代码页解释字节，
 * 直接写会变成乱码，所以对着控制台时改用宽字符；重定向到文件或管道时
 * 保持 UTF-8 字节，退出码和内容都不受终端影响。
 */
static void write_text(FILE *stream, const char *text)
{
#ifdef _WIN32
	if (_isatty(_fileno(stream)) != 0)
	{
		wchar_t wide[1024];
		int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, 1024);
		DWORD written;

		/* 末尾的 NUL 不算正文；失败就退回字节输出，至少不丢消息。 */
		if (length > 1 &&
			WriteConsoleW((HANDLE)_get_osfhandle(_fileno(stream)), wide,
				(DWORD)(length - 1), &written, NULL) != 0)
		{
			return;
		}
	}
#endif
	fputs(text, stream);
}

static void print_usage(FILE *stream, const char *program)
{
	char text[1024];

	snprintf(text, sizeof(text),
		"用法：%s [小数位数]\n"
		"省略参数时输出 %d 位；位数是截断的，不做四舍五入。\n",
		program, DEFAULT_DECIMAL_DIGITS);
	write_text(stream, text);
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

/* 把 carry 加进已写出的组里，返回 0 表示整串都被进位吃掉了（不可能发生）。 */
static int add_carry(uint32_t *groups, size_t count, uint64_t carry)
{
	while (carry != 0)
	{
		uint64_t value;

		if (count == 0)
		{
			return 0;
		}
		--count;
		value = (uint64_t)groups[count] + carry;
		groups[count] = (uint32_t)(value % BASE);
		carry = value / BASE;
	}

	return 1;
}

static int write_pi(size_t decimal_digits)
{
	size_t significant_digits; /* 整数位 3 加上 d 位小数 */
	size_t blocks;
	size_t terms;
	size_t digit_count;
	size_t written = 0;
	size_t length;
	uint64_t accumulator = 0;
	uint32_t *remainders;
	uint32_t *groups;
	char *text;
	int status = 0;

	/*
	 * 先挡尺寸计算本身会溢出的输入，再挡余数装不进 uint32_t 的输入。
	 * 内层循环里 accumulator < 3*BASE、remainders[] < 2*terms，于是
	 * accumulator*index + remainders[index]*BASE < 5*BASE*terms，
	 * 限住 terms 就保证这个乘积落在 uint64_t 里；实际上 malloc 会先失败。
	 */
	if (decimal_digits >= SIZE_MAX / 4)
	{
		write_text(stderr, "位数太大。\n");
		return 0;
	}

	significant_digits = decimal_digits + 1;
	blocks = (significant_digits + DIGITS_PER_BLOCK - 1) / DIGITS_PER_BLOCK;
	terms = blocks * TERMS_PER_BLOCK;
	digit_count = blocks * DIGITS_PER_BLOCK;

	if (terms > UINT32_MAX / 2)
	{
		write_text(stderr, "位数太大。\n");
		return 0;
	}

	remainders = malloc((terms + 1) * sizeof(*remainders));
	groups = malloc(blocks * sizeof(*groups));
	text = malloc(digit_count + 2);
	if (remainders == NULL || groups == NULL || text == NULL)
	{
		write_text(stderr, "内存不足，算不了这么多位。\n");
		free(remainders);
		free(groups);
		free(text);
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
				(uint64_t)remainders[index] * BASE;
			remainders[index] = (uint32_t)(accumulator % denominator);
			accumulator /= denominator;
			--index;
		}

		group = low + accumulator / BASE;
		while (group >= BASE)
		{
			if (!add_carry(groups, written, group / BASE))
			{
				write_text(stderr, "内部计算溢出。\n");
				goto done;
			}
			group %= BASE;
		}
		groups[written++] = (uint32_t)group;
	}

	if (written != blocks)
	{
		write_text(stderr, "内部计算不足位数。\n");
		goto done;
	}

	for (size_t i = 0; i < blocks; ++i)
	{
		snprintf(text + i * DIGITS_PER_BLOCK, DIGITS_PER_BLOCK + 1, "%09" PRIu32, groups[i]);
	}

	if (decimal_digits == 0)
	{
		text[1] = '\n';
		length = 2;
	}
	else
	{
		memmove(text + 2, text + 1, decimal_digits);
		text[1] = '.';
		text[decimal_digits + 2] = '\n';
		length = decimal_digits + 3;
	}

	if (fwrite(text, 1, length, stdout) != length)
	{
		write_text(stderr, "写输出失败。\n");
		goto done;
	}

	status = 1;

done:
	free(remainders);
	free(groups);
	free(text);
	return status;
}

int main(int argc, char **argv)
{
	size_t decimal_digits = DEFAULT_DECIMAL_DIGITS;

#ifdef _WIN32
	/* 各平台都输出 LF，文件比对不用再管换行。 */
	(void)_setmode(_fileno(stdout), _O_BINARY);
	(void)_setmode(_fileno(stderr), _O_BINARY);
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

	if (!write_pi(decimal_digits))
	{
		return 1;
	}

	return fflush(stdout) != 0 || ferror(stdout) ? 1 : 0;
}
