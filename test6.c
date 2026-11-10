#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
	BASE = 10000,
	INITIAL_REMAINDER = BASE / 5,
	TERMS_PER_BLOCK = 14,
	DIGITS_PER_BLOCK = 4,
	DEFAULT_DECIMAL_DIGITS = 15000
};

static void print_usage(const char *program)
{
	fprintf(stderr, "Usage: %s [decimal-digits]\n", program);
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

static int write_char(int character)
{
	return putchar(character) == EOF ? 0 : 1;
}

static int write_span(const char *digits, size_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		if (!write_char(digits[i]))
		{
			return 0;
		}
	}

	return 1;
}

static int write_pi(size_t decimal_digits)
{
	size_t significant_digits;
	size_t blocks;
	size_t terms;
	uint64_t *remainders;
	uint64_t accumulator = 0;
	size_t decimals_remaining = decimal_digits;

	if (decimal_digits == SIZE_MAX || decimal_digits + 1 > SIZE_MAX - 3)
	{
		fprintf(stderr, "Requested precision is too large.\n");
		return 0;
	}

	significant_digits = decimal_digits + 1;
	blocks = (significant_digits + 3) / DIGITS_PER_BLOCK;
	if (blocks > SIZE_MAX / TERMS_PER_BLOCK)
	{
		fprintf(stderr, "Requested precision is too large.\n");
		return 0;
	}
	terms = blocks * TERMS_PER_BLOCK;

	/*
	 * Inside the loop accumulator < 2 * index and remainders[index] < 2 * index,
	 * so accumulator * index + remainders[index] * BASE < 2 * terms^2 + 20000 * terms.
	 * The threshold below keeps that product inside uint64_t; in practice the
	 * allocation limit is reached long before this one.
	 */
	if (terms > UINT64_C(3000000000))
	{
		fprintf(stderr, "Requested precision is too large.\n");
		return 0;
	}
	if (terms >= SIZE_MAX / sizeof(*remainders))
	{
		fprintf(stderr, "Requested precision is too large.\n");
		return 0;
	}

	remainders = malloc((terms + 1) * sizeof(*remainders));
	if (remainders == NULL)
	{
		fprintf(stderr, "Unable to allocate memory for the requested precision.\n");
		return 0;
	}

	for (size_t i = 0; i <= terms; ++i)
	{
		remainders[i] = INITIAL_REMAINDER;
	}

	for (size_t depth = terms; depth > 0; depth -= TERMS_PER_BLOCK)
	{
		char chunk[DIGITS_PER_BLOCK + 1];
		size_t index = depth;

		const uint64_t low = accumulator % BASE;
		accumulator = low;
		while (index > 0)
		{
			const uint64_t denominator = UINT64_C(2) * (uint64_t)index - 1;
			accumulator = accumulator * (uint64_t)index +
				remainders[index] * BASE;
			remainders[index] = accumulator % denominator;
			accumulator /= denominator;
			--index;
		}

		const uint64_t group = low + accumulator / BASE;
		if (group >= BASE ||
			snprintf(chunk, sizeof(chunk), "%04llu", (unsigned long long)group) != DIGITS_PER_BLOCK)
		{
			free(remainders);
			fprintf(stderr, "Internal calculation error.\n");
			return 0;
		}

		const size_t first = (depth == terms) ? 1 : 0;
		const size_t available = DIGITS_PER_BLOCK - first;
		const size_t count = decimals_remaining < available ? decimals_remaining : available;

		if (first != 0 &&
			(!write_char(chunk[0]) || (decimal_digits > 0 && !write_char('.'))))
		{
			free(remainders);
			fprintf(stderr, "Unable to write output.\n");
			return 0;
		}

		if (!write_span(chunk + first, count))
		{
			free(remainders);
			fprintf(stderr, "Unable to write output.\n");
			return 0;
		}
		decimals_remaining -= count;
	}

	free(remainders);
	return decimals_remaining == 0 && write_char('\n');
}

int main(int argc, char **argv)
{
	size_t decimal_digits = DEFAULT_DECIMAL_DIGITS;
	static char output_buffer[BUFSIZ * 4];

	if (argc == 2 && strcmp(argv[1], "--help") == 0)
	{
		print_usage(argv[0]);
		return 0;
	}
	if (argc > 2 || (argc == 2 && !parse_decimal_digits(argv[1], &decimal_digits)))
	{
		print_usage(argv[0]);
		return 2;
	}

	(void)setvbuf(stdout, output_buffer, _IOFBF, sizeof(output_buffer));
	if (!write_pi(decimal_digits))
	{
		return 1;
	}

	return fflush(stdout) != 0 || ferror(stdout) ? 1 : 0;
}
