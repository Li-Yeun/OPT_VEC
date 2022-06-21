#define MAP_WIDTH 4096
#define MAP_HEIGHT 2732

__kernel void backup(__global uint* backupBuffer)
{

}
__kernel void draw(__global uint* pixels, int spriteFrameSize)
{
    /*
    for(int x = 0; x < MAP_WIDTH/2; x++)
    {
        for(int y = 0; y < MAP_HEIGHT/2; y++)
            pixels[x + y * MAP_WIDTH] = 0xFF0000;
    }*/

    	// save the area of target that we are about to overwrite

	// OPT: Precalculation
	int frameSizeTimes4 = spriteFrameSize * 4;

	// OPT: Ternary operator
	backup = backup ? backup : new uint[sqr( frameSize + 1 )];
	int2 intPos = make_int2( pos );
	// OPT: Bit-shifts
	int x1 = intPos.x - (frameSize >> 1), x2 = x1 + frameSize;
	int y1 = intPos.y - (frameSize >> 1), y2 = y1 + frameSize;
	if (x1 < 0 || y1 < 0 || x2 >= target->width || y2 >= target->height)
	{
		// out of range; skip
		lastTarget = 0;
		return;
	}
	// OPT: Precalculations
	uint* dst_start = target->pixels + x1 + y1 * target->width;
#ifdef UNROLLING
	if ((frameSize & (frameSize - 1)) == 0)
		for (int v = 0; v < frameSize; v += 2)
		{
			uint* new_backup = backup + v * frameSize;
			uint* new_dst_start = dst_start + v * target->width;
			memcpy( new_backup, new_dst_start, frameSizeTimes4 );
			memcpy( new_backup + frameSize, new_dst_start + target->width, frameSizeTimes4 );
		}
	else
	#endif
		for (int v = 0; v < frameSize; v++)
		{
			memcpy( backup + v * frameSize, dst_start + v * target->width, frameSizeTimes4 );
		}
	lastPos = make_int2( x1, y1 );
	lastTarget = target;
	// calculate bilinear weights - these are constant in this case.
	uint frac_x = (int)(255.0f * (pos.x - floorf( pos.x )));
	uint frac_y = (int)(255.0f * (pos.y - floorf( pos.y )));
	// Precalculations
	uint frac_x_inv = (255 - frac_x), frac_y_inv = (255 - frac_y);
	uint w0 = (frac_x * frac_y) >> 8;
	uint w1 = (frac_x_inv * frac_y) >> 8;
	uint w2 = (frac_x * frac_y_inv) >> 8;
	uint w3 = (frac_x_inv * frac_y_inv) >> 8;
	// draw the sprite frame
	uint stride = sprite->frameCount * frameSize;
	// Precalculations
	uint* src_start = sprite->pixels + frame * frameSize;
	int frameSizeMinusOne = frameSize - 1;
#ifdef UNROLLING
	if ((frameSizeMinusOne & (frameSizeMinusOne - 1)) == 0)
		for (int v = 0; v < frameSizeMinusOne; v += 2)
		{
			uint* new_dst_start = dst_start + v * target->width;
			uint* new_src_start = src_start + v * stride;
			uint* dst = new_dst_start;
			uint* src = new_src_start;
			for (int u = 0; u < frameSizeMinusOne; u++, src++, dst++)
			{
				uint pix = ScaleColor( src[0], w0 )
					+ ScaleColor( src[1], w1 )
					+ ScaleColor( src[stride], w2 )
					+ ScaleColor( src[stride + 1], w3 );
				uint alpha = pix >> 24;
				*dst = ScaleColor( pix, alpha ) + ScaleColor( *dst, 255 - alpha );
				u++, src++, dst++;
				pix = ScaleColor( src[0], w0 )
					+ ScaleColor( src[1], w1 )
					+ ScaleColor( src[stride], w2 )
					+ ScaleColor( src[stride + 1], w3 );
				alpha = pix >> 24;
				*dst = ScaleColor( pix, alpha ) + ScaleColor( *dst, 255 - alpha );
			}
			dst = new_dst_start + target->width;
			src = new_src_start + stride;
			for (int u = 0; u < frameSizeMinusOne; u++, src++, dst++)
			{
				uint pix = ScaleColor( src[0], w0 )
					+ ScaleColor( src[1], w1 )
					+ ScaleColor( src[stride], w2 )
					+ ScaleColor( src[stride + 1], w3 );
				uint alpha = pix >> 24;
				*dst = ScaleColor( pix, alpha ) + ScaleColor( *dst, 255 - alpha );
				u++, src++, dst++;
				pix = ScaleColor( src[0], w0 )
					+ ScaleColor( src[1], w1 )
					+ ScaleColor( src[stride], w2 )
					+ ScaleColor( src[stride + 1], w3 );
				alpha = pix >> 24;
				*dst = ScaleColor( pix, alpha ) + ScaleColor( *dst, 255 - alpha );
			}
		}
	else
	#endif
		for (int v = 0; v < frameSizeMinusOne; v++)
		{
			uint* dst = dst_start + v * target->width;
			uint* src = src_start + v * stride;
			for (int u = 0; u < frameSizeMinusOne; u++, src++, dst++)
			{
				uint pix = ScaleColor( src[0], w0 )
					+ ScaleColor( src[1], w1 )
					+ ScaleColor( src[stride], w2 )
					+ ScaleColor( src[stride + 1], w3 );
				uint alpha = pix >> 24;
				*dst = ScaleColor( pix, alpha ) + ScaleColor( *dst, 255 - alpha );
			}
		}
}