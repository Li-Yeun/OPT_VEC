#define MAP_WIDTH 4096
#define MAP_HEIGHT 2732
//#define SPRITE_FRAMECOUNT 256
//#define SPRITE_FRAMESIZE 36

inline uint ScaleColor( const uint c, const uint scale )
{
	const uint rb = (((c & 0xff00ff) * scale) >> 8) & 0x00ff00ff;
	const uint ag = (((c & 0xff00ff00) >> 8) * scale) & 0xff00ff00;
	return rb + ag;
}
inline void Blend(__global uint* buffer, int x, int y, uint c, uint w )
{
	if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return;
	buffer[x + y * MAP_WIDTH] = ScaleColor( c, w ) + ScaleColor( buffer[x + y * MAP_WIDTH], 255 - w );
}

inline void BlendBilerp(__global uint* buffer, float x, float y, uint c, uint w )
{
	int2 intPos = (int2) ( (int)x, (int)y );
	float frac_x = x - intPos.x;
	float frac_y = y - intPos.y;
	int w1 = (int)(256 * ((1 - frac_x) * (1 - frac_y)));
	int w2 = (int)(256 * (frac_x * (1 - frac_y)));
	int w3 = (int)(256 * ((1 - frac_x) * frac_y));
	int w4 = (int)(256 * (frac_x * frac_y));
	Blend(buffer, intPos.x, intPos.y, c, (w1 * w) >> 8 );
	Blend(buffer, intPos.x + 1, intPos.y, c, (w2 * w) >> 8 );
	Blend(buffer, intPos.x, intPos.y + 1, c, (w3 * w) >> 8 );
	Blend(buffer, intPos.x + 1, intPos.y + 1, c, (w4 * w) >> 8 );
    
}

__kernel void render(__global uint* buffer, __global uint* sprite, __global float2* pos, __global int* frame, int sprite_frameSize, int sprite_frameCount, __constant bool* tank_sprite)
{
    int x = get_global_id(0);
    int v = x / (sprite_frameSize - 1);
    int u = x % (sprite_frameSize - 1);
    int y = get_global_id(1);

    int offset = tank_sprite[y] * sprite_frameSize * sprite_frameSize * sprite_frameCount;

	// calculate bilinear weights - these are constant in this case.
    int x1 = pos[y].x - sprite_frameSize / 2, x2 = x1 + sprite_frameSize;
	int y1 = pos[y].y - sprite_frameSize / 2, y2 = y1 + sprite_frameSize;

    if (x1 < 0 || y1 < 0 || x2 >= MAP_WIDTH|| y2 >= MAP_HEIGHT)
	{
		return;
	}

	uint frac_x = (int)(255.0f * (pos[y].x - (int)( pos[y].x )));
	uint frac_y = (int)(255.0f * (pos[y].y - (int)( pos[y].y )));
	uint w0 = (frac_x * frac_y) >> 8;
	uint w1 = ((255 - frac_x) * frac_y) >> 8;
	uint w2 = (frac_x * (255 - frac_y)) >> 8;
	uint w3 = ((255 - frac_x) * (255 - frac_y)) >> 8;
	// draw the sprite frame
	uint stride = sprite_frameCount * sprite_frameSize;
	uint dst = x1 + (y1 + v) * MAP_WIDTH + u;
	uint src = frame[y] * sprite_frameSize + v * stride + u;
	//for (int v = 0; v < SPRITE_FRAMESIZE - 1; v++)
	//for (int u = 0; u < SPRITE_FRAMESIZE - 1; u++)
    uint p0 = ScaleColor( sprite[src + offset], w0 );
    uint p1 = ScaleColor( sprite[src + 1 + offset], w1 );
    uint p2 = ScaleColor( sprite[src + stride + offset], w2 );
    uint p3 = ScaleColor( sprite[src + stride + 1 + offset], w3 );
    uint pix = p0 + p1 + p2 + p3;
    uint alpha = pix >> 24;

    // Get old pixel from a nonsense automic operation
    uint oldpixel = atomic_add(buffer + dst, 0);
    // Keep trying to write to the buffer until the buffer held the same pixel you read before.
    while (true) {
        uint newoldpixel = atomic_cmpxchg(buffer + dst, oldpixel, ScaleColor(pix, alpha) + ScaleColor(oldpixel, 255 - alpha));
        if (newoldpixel == oldpixel) break;
        else oldpixel = newoldpixel;
    }
}
__kernel void saveLastPos( __global float2* pos, __global int2* lastPos, __global int* lastTarget, int sprite_frameSize)
{
    int x = get_global_id(0);

	int2 intPos = (int2)(pos[x].x, pos[x].y);
	int x1 = intPos.x - sprite_frameSize / 2, x2 = x1 + sprite_frameSize;
	int y1 = intPos.y - sprite_frameSize / 2, y2 = y1 + sprite_frameSize;
	if (x1 < 0 || y1 < 0 || x2 >= MAP_WIDTH || y2 >= MAP_HEIGHT)
	{
		// out of range; skip
		lastTarget[x] = 0;
		return;
	}
	lastPos[x] = (int2)( x1, y1 );
	lastTarget[x] = 1;
}

__kernel void backup(__global uint* buffer, __global uint* backup, __global int2* lastPos, int sprite_frameSize, __global int* ZBuffer, __global int* backupZBuffer)
{
    int x = get_global_id(0);
    int y = get_global_id(1);   

    int v = x / (sprite_frameSize);
    int u = x % (sprite_frameSize);

    int z = atomic_add(ZBuffer + lastPos[y].x + (lastPos[y].y + v) * MAP_WIDTH + u, 0);
    while (true) {
        backup[v * sprite_frameSize + u + y * sprite_frameSize * sprite_frameSize] = atomic_add(buffer + lastPos[y].x + (lastPos[y].y + v) * MAP_WIDTH + u, 0);
        int newz = atomic_cmpxchg(ZBuffer + lastPos[y].x + (lastPos[y].y + v) * MAP_WIDTH + u, z, z + 1);
        if (newz == z) 
        { 
            backupZBuffer[v * sprite_frameSize + u + y * sprite_frameSize * sprite_frameSize] = z;
            break; 
        }
        else z = newz;
    }
    //backup[v * sprite_frameSize + u + y * sprite_frameSize* sprite_frameSize] = buffer[lastPos[y].x + (lastPos[y].y + v) * MAP_WIDTH + u];
}

__kernel void remove(__global uint* buffer, __global uint* backup, __global int2* lastPos, __global int* lastTarget, int sprite_frameSize, __global int* ZBuffer, __global int* backupZBuffer)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if(!lastTarget[y]) return;

    int v = x / (sprite_frameSize);
    int u = x % (sprite_frameSize);

    //buffer[lastPos[y].x + (lastPos[y].y + v) * MAP_WIDTH + u] = backup[v * sprite_frameSize + u +  y * sprite_frameSize * sprite_frameSize];

    // If we are not the sprite that drew first for this pixel, skip
    if (backupZBuffer[v * sprite_frameSize + u + y * sprite_frameSize * sprite_frameSize] != 0) return;

    // Get old pixel from a nonsense automic operation
    uint oldpixel = atomic_add(buffer + lastPos[y].x + (lastPos[y].y + v) * MAP_WIDTH + u, 0);
    // Keep trying to write to the buffer until the buffer held the same pixel you read before.
    while (true) {
        uint newoldpixel = atomic_cmpxchg(buffer + lastPos[y].x + (lastPos[y].y + v) * MAP_WIDTH + u, 
            oldpixel, backup[v * sprite_frameSize + u + y * sprite_frameSize * sprite_frameSize]);
        if (newoldpixel == oldpixel) break;
        else oldpixel = newoldpixel;
    }
    ZBuffer[lastPos[y].x + (lastPos[y].y + v) * MAP_WIDTH + u] = 0;
}


__kernel void bushrender(__global uint* buffer, __global uint* sprite, __global float2* pos, __global int* frame, int sprite_frameSize, int sprite_frameCount)
{
    int x = get_global_id(0);
    int v = x / (sprite_frameSize - 1);
    int u = x % (sprite_frameSize - 1);
    int y = get_global_id(1);

	// calculate bilinear weights - these are constant in this case.
    int x1 = pos[y].x - sprite_frameSize / 2, x2 = x1 + sprite_frameSize;
	int y1 = pos[y].y - sprite_frameSize / 2, y2 = y1 + sprite_frameSize;

	if (x1 < 0 || y1 < 0 || x2 >= MAP_WIDTH|| y2 >= MAP_HEIGHT)
	{
		return;
	}
    
	uint frac_x = (int)(255.0f * (pos[y].x - (int)( pos[y].x )));
	uint frac_y = (int)(255.0f * (pos[y].y - (int)( pos[y].y )));
	uint w0 = (frac_x * frac_y) >> 8;
	uint w1 = ((255 - frac_x) * frac_y) >> 8;
	uint w2 = (frac_x * (255 - frac_y)) >> 8;
	uint w3 = ((255 - frac_x) * (255 - frac_y)) >> 8;
	// draw the sprite frame
	uint stride = sprite_frameCount * sprite_frameSize;
	uint dst = x1 + (y1 + v) * MAP_WIDTH + u;
	uint src = frame[y] * sprite_frameSize + v * stride + u;
	//for (int v = 0; v < SPRITE_FRAMESIZE - 1; v++)
	//for (int u = 0; u < SPRITE_FRAMESIZE - 1; u++)
    uint p0 = ScaleColor( sprite[src], w0 );
    uint p1 = ScaleColor( sprite[src + 1], w1 );
    uint p2 = ScaleColor( sprite[src + stride], w2 );
    uint p3 = ScaleColor( sprite[src + stride + 1], w3 );
    uint pix = p0 + p1 + p2 + p3;
    uint alpha = pix >> 24;

    // Get old pixel from a nonsense automic operation
    uint oldpixel = atomic_add(buffer + dst, 0);
    // Keep trying to write to the buffer until the buffer held the same pixel you read before.
    while (true) {
        uint newoldpixel = atomic_cmpxchg(buffer + dst, oldpixel, ScaleColor(pix, alpha) + ScaleColor(oldpixel, 255 - alpha));
        if (newoldpixel == oldpixel) break;
        else oldpixel = newoldpixel;
    }
}
__kernel void trackrender(__global uint* buffer, __global float2* pos, __global float* steer, __global float2* dir)
{   
    int x = get_global_id(0);
	if(steer[x] >= -0.2f && steer[x] <= 0.2f)
    {
		// draw tank tracks, only when not turning
		float2 perp = (float2) ( - dir[x].y, dir[x].x );
        
		float2 trackPos1 = pos[x] - 9 * dir[x] + 4.5f * perp;
		float2 trackPos2 = pos[x] - 9 * dir[x] - 5.5f * perp;
		BlendBilerp(buffer, trackPos1.x, trackPos1.y, 0, 12 );
		BlendBilerp(buffer, trackPos2.x, trackPos2.y, 0, 12 );
	}
}
