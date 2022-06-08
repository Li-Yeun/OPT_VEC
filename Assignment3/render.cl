#define MAP_WIDTH 4096
#define MAP_HEIGHT 2732
#define SPRITE_FRAMECOUNT 256
#define SPRITE_FRAMESIZE 36

inline uint ScaleColor( const uint c, const uint scale )
{
	const uint rb = (((c & 0xff00ff) * scale) >> 8) & 0x00ff00ff;
	const uint ag = (((c & 0xff00ff00) >> 8) * scale) & 0xff00ff00;
	return rb + ag;
}
__kernel void render(__global uint* buffer, __global uint* sprite, __global float2* pos, __global int* frame)
{
    int x = get_global_id(0);
    int v = x / (SPRITE_FRAMESIZE - 1);
    int u = x % (SPRITE_FRAMESIZE - 1);
    int y = get_global_id(1);

	// calculate bilinear weights - these are constant in this case.
    int x1 = pos[y].x - SPRITE_FRAMESIZE / 2, x2 = x1 + SPRITE_FRAMESIZE;
	int y1 = pos[y].y - SPRITE_FRAMESIZE / 2, y2 = y1 + SPRITE_FRAMESIZE;

	uint frac_x = (int)(255.0f * (pos[y].x - (int)( pos[y].x )));
	uint frac_y = (int)(255.0f * (pos[y].y - (int)( pos[y].y )));
	uint w0 = (frac_x * frac_y) >> 8;
	uint w1 = ((255 - frac_x) * frac_y) >> 8;
	uint w2 = (frac_x * (255 - frac_y)) >> 8;
	uint w3 = ((255 - frac_x) * (255 - frac_y)) >> 8;
	// draw the sprite frame
	uint stride = SPRITE_FRAMECOUNT * SPRITE_FRAMESIZE;
	uint dst = x1 + (y1 + v) * MAP_WIDTH + u;
	uint src = frame[y] * SPRITE_FRAMESIZE + v * stride + u;
	//for (int v = 0; v < SPRITE_FRAMESIZE - 1; v++)
	//for (int u = 0; u < SPRITE_FRAMESIZE - 1; u++)
    uint p0 = ScaleColor( sprite[src], w0 );
    uint p1 = ScaleColor( sprite[src + 1], w1 );
    uint p2 = ScaleColor( sprite[src + stride], w2 );
    uint p3 = ScaleColor( sprite[src + stride + 1], w3 );
    uint pix = p0 + p1 + p2 + p3;
    uint alpha = pix >> 24;
    buffer[dst] = ScaleColor( pix, alpha ) + ScaleColor(buffer[dst] , 255 - alpha );
}
__kernel void saveLastPos( __global float2* pos, __global int2* lastPos, __global int* lastTarget)
{
    int x = get_global_id(0);

	int2 intPos = (int2)(pos[x].x, pos[x].y);
	int x1 = intPos.x - SPRITE_FRAMESIZE / 2, x2 = x1 + SPRITE_FRAMESIZE;
	int y1 = intPos.y - SPRITE_FRAMESIZE / 2, y2 = y1 + SPRITE_FRAMESIZE;
	if (x1 < 0 || y1 < 0 || x2 >= MAP_WIDTH || y2 >= MAP_HEIGHT)
	{
		// out of range; skip
		lastTarget[x] = 0;
		return;
	}
	lastPos[x] = (int2)( x1, y1 );
	lastTarget[x] = 1;
}

__kernel void backup(__global uint* buffer, __global uint* backup, __global int2* lastPos)
{
    int x = get_global_id(0);
    int y = get_global_id(1);   

    int v = x / (SPRITE_FRAMESIZE);
    int u = x % (SPRITE_FRAMESIZE);

    backup[v * SPRITE_FRAMESIZE + u + y * (SPRITE_FRAMESIZE + 1)* (SPRITE_FRAMESIZE + 1)] = buffer[lastPos[y].x + (lastPos[y].y + v) * MAP_WIDTH + u];
}

__kernel void remove(__global uint* buffer, __global uint* backup, __global int2* lastPos, __global int* lastTarget)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if(!lastTarget[y]) return;
    
    int v = x / (SPRITE_FRAMESIZE);
    int u = x % (SPRITE_FRAMESIZE);

    buffer[lastPos[y].x + (lastPos[y].y + v) * MAP_WIDTH + u] = backup[v * SPRITE_FRAMESIZE + u +  y *  (SPRITE_FRAMESIZE + 1 )* (SPRITE_FRAMESIZE + 1)];
}
