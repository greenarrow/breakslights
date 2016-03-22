#include "common.h"
#include "animation.h"

#define HUE_REGION 40

static void test(byte h, byte l)
{
	struct colour result = hltorgb(h, l);
	printf("HL %3u %3u RGB %3u %3u %3u\n", h, l, result.r, result.g,
								result.b);
}

int main(void)
{
	byte h;

	/* spectrum of full colours */
	for (h = 0; h < HUE_REGION * 6; h += HUE_REGION / 2)
		test(h, 127);

	/* shades of red */
	test(0, 0);
	test(0, 63);
	test(0, 191);
	test(0, 255);

	/* shades of green */
	test(80, 0);
	test(80, 255);

	/* shades of blue */
	test(160, 0);
	test(160, 255);

	return 0;
}
