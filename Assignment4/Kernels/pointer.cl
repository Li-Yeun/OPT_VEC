#define SCRWIDTH 1920
#define SCRHEIGHT 1080
#define MAP_WIDTH 4096
#define MAP_HEIGHT 2732

float3 RGB8toRGB32F( uint c )
{
	float s = 1 / 256.0f;
	int r = (c >> 16) & 255;
	int g = (c >> 8) & 255;
	int b = c & 255;
	return (float3)(r * s, g * s, b * s);
}

__kernel void Remove(__global uint* pixels, __global uint* backUp, int2 lastPos, int lastTarget, int spriteFrameSize)
{
    if(lastTarget == 0) return;
    int threadIdx = get_global_id( 0 );
    int xt = threadIdx % spriteFrameSize;
    int yt = threadIdx / spriteFrameSize;
    int x = lastPos.x - (spriteFrameSize >> 1) + xt;
    int y = lastPos.y - (spriteFrameSize >> 1) + yt;

    if (x < 0 || y < 0 || x >= MAP_WIDTH|| y >= MAP_HEIGHT)
	{
		return;
	}
    uint pix = backUp[spriteFrameSize * yt + xt];
    uint alpha = pix >> 24;
    //if(alpha == 0) return;

    int dst = x + y * MAP_WIDTH;
    pixels[dst] = pix;
}

__kernel void Draw(__global uint* pixels, __global uint* spritePixels, int2 pos, int spriteFrameSize, __global uint* backUp)
{
    int threadIdx = get_global_id( 0 );
    int xt = threadIdx % spriteFrameSize;
    int yt = threadIdx / spriteFrameSize;
    int x = pos.x - (spriteFrameSize >> 1) + xt;
    int y = pos.y - (spriteFrameSize >> 1) + yt;

    if (x < 0 || y < 0 || x >= MAP_WIDTH|| y >= MAP_HEIGHT)
	{
		return;
	}

    uint pix = spritePixels[spriteFrameSize * yt + xt];
    uint alpha = pix >> 24;
    int dst = x + y * MAP_WIDTH;
    backUp[spriteFrameSize * yt + xt] = pixels[dst];
    if(alpha == 0) return;

    
    
    pixels[dst] = pix;
    //write_imagef( target, (int2)(x, y), (float4)(RGB8toRGB32F(pix), 1 ) );
}