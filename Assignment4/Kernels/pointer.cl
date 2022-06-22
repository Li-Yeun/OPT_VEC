#define SCRWIDTH 1920
#define SCRHEIGHT 1080

float3 RGB8toRGB32F( uint c )
{
	float s = 1 / 256.0f;
	int r = (c >> 16) & 255;
	int g = (c >> 8) & 255;
	int b = c & 255;
	return (float3)(r * s, g * s, b * s);
}

__kernel void Draw(write_only image2d_t target, __global uint* spritePixels, int2 pos, int spriteFrameSize)
{
    int threadIdx = get_global_id( 0 );
    int xt = threadIdx % spriteFrameSize;
    int yt = threadIdx / spriteFrameSize;
    int x = pos.x - (spriteFrameSize >> 1) + xt;
    int y = pos.y - (spriteFrameSize >> 1) + yt;

    if (x < 0 || y < 0 || x >= SCRWIDTH|| y >= SCRHEIGHT)
	{
		return;
	}

    uint pix = spritePixels[spriteFrameSize * yt + xt];
    uint alpha = pix >> 24;
    if(alpha == 0) return;

    write_imagef( target, (int2)(x, y), (float4)(RGB8toRGB32F(pix), 1 ) );
}