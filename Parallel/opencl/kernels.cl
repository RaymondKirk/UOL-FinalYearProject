#define width 800
#define height 600 

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

__kernel void motion_estimation(
	__read_only image2d_t prev,
	__read_only image2d_t curr,
	uint blockSize,
	__global int2 * motionVectors
)
{
	const int x = get_global_id(0), y = get_global_id(1);
	const int2 pos = { x * blockSize, y * blockSize };
	const int blockCount = get_global_size(0);

	if (pos.x < width && pos.y < height) {
		int mvPos = x + y * blockCount, i, j;

		//Boost performance with proof => ABS(SUMA - SUMB|SUMB - SUMA) = SAD
		int SUMA = 0, SUMB = 0;

		for (i = 0; i <= blockSize; i++) {
			for (j = 0; j <= blockSize; j++) {
				int2 refPos = { pos.x + i, pos.y + j };
				SUMA += read_imageui(prev, sampler, refPos).x;
				SUMB += read_imageui(curr, sampler, refPos).x;
			}
		}

		//Calculate SAD -VERTICAL SCAN SAD = abs(SUMA - SUMB)
		int searchMacro = blockSize, row, col, lowestSAD = INT_MAX, SAD = 0;

		if (abs(SUMA - SUMB) == 0) {
			motionVectors[mvPos] = pos;
			return;
		}

		//Calculate Intial SUMB of top left search window
		for (i = 0; i <= blockSize; i++) {
			for (j = 0; j <= blockSize; j++) {
				SUMB += read_imageui(curr, sampler, (int2)(pos.x - searchMacro + i, pos.y - searchMacro + j)).x;
			}
		}

		//Only recalculate bottom edge of SUMB for x and Right Edge for Y
		for (row = -searchMacro; row <= searchMacro; row++)
		{
			col = -searchMacro;
			for (j = 0; j <= blockSize; j++) {
				SUMB -= read_imageui(curr, sampler, (int2)(pos.x + row, pos.y + col + j)).x;
				SUMB += read_imageui(curr, sampler, (int2)(pos.x + row + blockSize, pos.y + col + j)).x;
			}

			for (; col <= searchMacro; col++)
			{
				int2 searchPos = { pos.x + row, pos.y + col };
				SUMB = 0;

				for (i = 0; i <= blockSize; i++) {
						SUMB -= read_imageui(curr, sampler, (int2)(pos.x + row + i, pos.y + col)).x;
						SUMB += read_imageui(curr, sampler, (int2)(pos.x + row + i, pos.y + col + blockSize)).x;
				}

				SAD = abs(SUMA - SUMB);

				if (SAD < lowestSAD) {
					lowestSAD = SAD;
					motionVectors[mvPos] = searchPos;
				}

				if (SAD < 1000)
					break;
			}
		}

	}
}

