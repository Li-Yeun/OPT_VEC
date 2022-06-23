#define MAP_WIDTH 4096
#define MAP_HEIGHT 2732

inline uint AddBlend( const uint c1, const uint c2 )
{
	const uint r1 = (c1 >> 16) & 255, r2 = (c2 >> 16) & 255;
	const uint g1 = (c1 >> 8) & 255, g2 = (c2 >> 8) & 255;
	const uint b1 = c1 & 255, b2 = c2 & 255;
	const uint r = min( 255u, r1 + r2 );
	const uint g = min( 255u, g1 + g2 );
	const uint b = min( 255u, b1 + b2 );
	return (r << 16) + (g << 8) + b;
}

__kernel void Remove(__global uint* pixels, __constant __read_only int2* pos,  __global uint* backupBuffer, __global bool* lastTarget, int spriteFrameSize)
{   
    //x = spriteFrameSize * spriteFrameSize
    //y = total explosions

    int y = get_global_id(1);
    if(!lastTarget[y])
       return;

    int x = get_global_id(0);
    int v = x / spriteFrameSize;
    int u = x % spriteFrameSize;

    pixels[pos[y].x + (pos[y].y + v) * MAP_WIDTH + u] = backupBuffer[v * spriteFrameSize + u +  y * spriteFrameSize * spriteFrameSize];
}

__kernel void Backup(__global uint* pixels,  __constant __read_only int2* pos, __global uint* backupBuffer, __global bool* lastTarget, int spriteFrameSize)
{
    //x = spriteFrameSize * spriteFrameSize
    //y = total explosions

    int x = get_global_id(0);
    int y = get_global_id(1);

    int x1 = pos[y].x - (spriteFrameSize >> 1), x2 = x1 + spriteFrameSize;
	int y1 = pos[y].y - (spriteFrameSize >> 1), y2 = y1 + spriteFrameSize;
	if (x1 < 0 || y1 < 0 || x2 >= MAP_WIDTH || y2 >= MAP_HEIGHT)
	{
		// out of range; skip
		lastTarget[y] = 0;
		return;
	}

    int v = x / spriteFrameSize;
    int u = x % spriteFrameSize;

    backupBuffer[v * spriteFrameSize + u + y * spriteFrameSize * spriteFrameSize ] = pixels[x1 + (y1 + v)* MAP_WIDTH + u];
}

__kernel void SaveLastPos(__constant __read_only int2* pos, __global bool* lastTarget, int spriteFrameSize)
{
    //x = explosions;

    int x = get_global_id(0);

    int x1 = pos[x].x - (spriteFrameSize >> 1), x2 = x1 + spriteFrameSize;
	int y1 = pos[x].y - (spriteFrameSize >> 1), y2 = y1 + spriteFrameSize;

    if (x1 < 0 || y1 < 0 || x2 >= MAP_WIDTH|| y2 >= MAP_HEIGHT)
	{
		return;
	}

    lastTarget[x] = 1;
}

__kernel void DrawAdditive( __global uint* pixels, __global __read_only uint* spritePixels, __constant __read_only int2* pos,  __constant __read_only int* frame, int spriteFrameSize, int spriteFrameCount)
{
    // x = frameSize * frameSize
    //y = total explosions
    int x = get_global_id(0);
    int y = get_global_id(1);

    int x1 = pos[y].x - (spriteFrameSize >> 1), x2 = x1 + spriteFrameSize;
	int y1 = pos[y].y - (spriteFrameSize >> 1), y2 = y1 + spriteFrameSize;

    int v = x / spriteFrameSize;
    int u = x % spriteFrameSize;

    if (x1 < 0 || y1 < 0 || x2 >= MAP_WIDTH|| y2 >= MAP_HEIGHT)
	{
		return;
	}

	// draw the sprite frame
    int dst = x1 + u + (y1 + v) * MAP_WIDTH;
    pixels[dst] = AddBlend( pixels[dst], spritePixels[(frame[y] - 1) * spriteFrameSize + u + v  * spriteFrameCount * spriteFrameSize]);
}