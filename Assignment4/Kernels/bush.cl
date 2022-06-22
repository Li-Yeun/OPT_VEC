#define MAP_WIDTH 4096
#define MAP_HEIGHT 2732

inline uint ScaleColor( const uint c, const uint scale )
{
	const uint rb = (((c & 0xff00ff) * scale) >> 8) & 0x00ff00ff;
	const uint ag = (((c & 0xff00ff00) >> 8) * scale) & 0xff00ff00;
	return rb + ag;
}

__kernel void Remove(__global uint* pixels, __global __read_only int2* lastPos,  __global uint* backupBuffer, __global bool* lastTarget, __constant __read_only uint* spriteTypeIndexBuffer, int spriteFrameSize)
{   
    //x = spriteFrameSize * spriteFrameSize
    //y = total tanks

    int y = get_global_id(1);
    uint index = spriteTypeIndexBuffer[y];
    if(!lastTarget[index])
       return;

    int x = get_global_id(0);
    int v = x / spriteFrameSize;
    int u = x % spriteFrameSize;

    pixels[lastPos[index].x + (lastPos[index].y + v) * MAP_WIDTH + u] = backupBuffer[v * spriteFrameSize + u +  y * (spriteFrameSize + 1) * (spriteFrameSize + 1)];
}

__kernel void Backup(__global uint* pixels, __global __read_only float2* pos, __global uint* backupBuffer, __global bool* lastTarget, __constant __read_only uint* spriteTypeIndexBuffer, int spriteFrameSize)
{
    //x = spriteFrameSize * spriteFrameSize
    //y = total tanks

    int x = get_global_id(0);
    int y = get_global_id(1);

    uint index = spriteTypeIndexBuffer[y];

    int x1 = (int) pos[index].x - (spriteFrameSize >> 1), x2 = x1 + spriteFrameSize;
	int y1 = (int) pos[index].y - (spriteFrameSize >> 1), y2 = y1 + spriteFrameSize;
	if (x1 < 0 || y1 < 0 || x2 >= MAP_WIDTH || y2 >= MAP_HEIGHT)
	{
		// out of range; skip
		lastTarget[index] = 0;
		return;
	}

    int v = x / spriteFrameSize;
    int u = x % spriteFrameSize;

    backupBuffer[v * spriteFrameSize + u + y * (spriteFrameSize + 1) * (spriteFrameSize + 1) ] = pixels[x1 + (y1 + v)* MAP_WIDTH + u];
}

__kernel void SaveLastPos( __global __read_only float2* pos, __global int2* lastPos, __global bool* lastTarget, __constant __read_only uint* sprite, __constant __read_only uint* spriteFrameSizes)
{
    //x = totalTanks;

    int x = get_global_id(0);
    uint spriteFrameSize = spriteFrameSizes[sprite[x]];

    int x1 = (int) pos[x].x - (spriteFrameSize >> 1), x2 = x1 + spriteFrameSize;
	int y1 = (int) pos[x].y - (spriteFrameSize >> 1), y2 = y1 + spriteFrameSize;

    if (x1 < 0 || y1 < 0 || x2 >= MAP_WIDTH|| y2 >= MAP_HEIGHT)
	{
		return;
	}

    lastPos[x] = (int2)( x1, y1 );
    lastTarget[x] = 1;
}

__kernel void Draw(__global uint* pixels, __global __read_only uint* spritePixels, __constant __read_only uint* sprite, __global __read_only float2* pos, __global __read_only int* frame, __constant __read_only uint* spriteFrameSizes, __constant __read_only uint* spriteOffset, int spriteFrameCount)
{
    //x = (spriteFrameSize - 1) * (spriteFrameSize - 1)
    //y = totalTanks

    int x = get_global_id(0);
    int y = get_global_id(1);
    uint spriteFrameSize = spriteFrameSizes[sprite[y]];
    int spriteFrameSizeMinusOne = spriteFrameSize - 1;

    if(x >= spriteFrameSizeMinusOne * spriteFrameSizeMinusOne)
        return;
    
    int v = x / spriteFrameSizeMinusOne;
    int u = x % spriteFrameSizeMinusOne;

    int offset = spriteOffset[sprite[y]];

	int x1 = (int) pos[y].x - (spriteFrameSize >> 1), x2 = x1 + spriteFrameSize;
	int y1 = (int) pos[y].y - (spriteFrameSize >> 1), y2 = y1 + spriteFrameSize;

    if (x1 < 0 || y1 < 0 || x2 >= MAP_WIDTH|| y2 >= MAP_HEIGHT)
	{
		return;
	}

	// calculate bilinear weights - these are constant in this case.
	uint frac_x = (int)(255.0f * (pos[y].x - (int)( pos[y].x )));
	uint frac_y = (int)(255.0f * (pos[y].y - (int)( pos[y].y )));
	// Precalculations
	uint frac_x_inv = (255 - frac_x), frac_y_inv = (255 - frac_y);
	uint w0 = (frac_x * frac_y) >> 8;
	uint w1 = (frac_x_inv * frac_y) >> 8;
	uint w2 = (frac_x * frac_y_inv) >> 8;
	uint w3 = (frac_x_inv * frac_y_inv) >> 8;

	// draw the sprite frame
	uint stride = spriteFrameCount * spriteFrameSize;
	// Precalculations
    int dst = x1 + (y1 + v) * MAP_WIDTH + u;
    int src = frame[y] * spriteFrameSize + v * stride + u + offset;

    uint pix = ScaleColor( spritePixels[src], w0 )
        + ScaleColor( spritePixels[src + 1], w1 )
        + ScaleColor( spritePixels[src + stride], w2 )
        + ScaleColor( spritePixels[src + stride + 1], w3 );

    uint alpha = pix >> 24;
    
    pixels[dst] = ScaleColor( pix, alpha ) + ScaleColor( pixels[dst], 255 - alpha );
}