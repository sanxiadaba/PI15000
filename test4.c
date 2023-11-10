#include <stdio.h>

// 用长除法、竖式乘法计算级数
// 数组的第一个元素存储的是最后结果的整数部分
// 后2800存储最后结果的小数部分
int ans_arr[2801] = {0};
// 展开到2800
int n = 2800;

int main(void)
{
	// 从里向外展开
	for (int j = n; j >= 1; j--)
	{
		// 进位 本位 商 余数  乘法、除法用到的变量
		int jin = 0;
		int ben = 0;
		int shang = 0;
		int yushu = 0;

		// 乘法，ans_arr每一个元素都乘以变量（注意进位）
		for (int i = n; i > 0; i--)
		{
			ben = (ans_arr[i] * j + jin) % 10;
			jin = (ans_arr[i] * j + jin) / 10;
			ans_arr[i] = ben;
		}
		// 对于乘，第一个元素在这特殊处理，是因为计算到第一个元素后就没了
		// 计算到这，不用考虑进位、本位
		ans_arr[0] =  ans_arr[0]  * j + jin;

		// 除法，ans_arr每一个元素都除以变量（注意余数）
		for (int i = 0; i < n + 1; i++)
		{
			shang = (ans_arr[i] + 10 * yushu) / (2 * j + 1);
			yushu = (ans_arr[i] + 10 * yushu) % (2 * j + 1);
			ans_arr[i] = shang;
		}

		// 加上算法里的1
		ans_arr[0] += 1;
	}

	// 先打印整数部分
	printf("%d.", ans_arr[0]);
	// 打印小数部分
	for (int i = 1; i < 2800; i++)
	{
		printf("%d", ans_arr[i]);
	}
	return 0;
}
