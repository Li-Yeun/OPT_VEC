#define MAP_WIDTH 4096
#define MAP_HEIGHT 2732

inline uint ScaleColor( const uint c, const uint scale )
{
	const uint rb = (((c & 0xff00ff) * scale) >> 8) & 0x00ff00ff;
	const uint ag = (((c & 0xff00ff00) >> 8) * scale) & 0xff00ff00;
	return rb + ag;
}

void inline Plot(__global uint* pixels, int x, int y, uint c )
{
	if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return;
	pixels[x + y * MAP_WIDTH] = c;
}

uint inline Read(__global uint* pixels, int x, int y )
{
	if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return 0;
	return pixels[x + y * MAP_WIDTH];
}

void inline Blend(__global uint* pixels, int x, int y, uint c, uint w )
{
	if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return;

	int dst = x + y * MAP_WIDTH;
	uint out = ScaleColor( c, w );

    uint oldpixel = atomic_add(pixels + dst, 0);
    // Keep trying to write to the buffer until the buffer held the same pixel you read before.
    while (true) {
        uint newoldpixel = atomic_cmpxchg(pixels + dst, oldpixel, out + ScaleColor(oldpixel, 255 - w));
        if (newoldpixel == oldpixel) break;
        else oldpixel = newoldpixel;
    }
}

void inline BlendBilerp( __global uint* pixels, int x, int y, uint c, uint w )
{
	float frac_x = x - x;
	float frac_y = y - y;
	int w1 = (int)(256 * ((1 - frac_x) * (1 - frac_y)));
	int w2 = (int)(256 * (frac_x * (1 - frac_y)));
	int w3 = (int)(256 * ((1 - frac_x) * frac_y));
	int w4 = (int)(256 * (frac_x * frac_y));
	Blend(pixels, x, y, c, (w1 * w) >> 8 );
	Blend(pixels, x + 1, y, c, (w2 * w) >> 8 );
	Blend(pixels, x, y + 1, c, (w3 * w) >> 8 );
	Blend(pixels, x + 1, y + 1, c, (w4 * w) >> 8 );
}

__kernel void Remove(__global uint* pixels, __global __read_only int2* pos, __global __read_only uint* maxPos, int maxTotalPos,  __global uint* backupBuffer)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if(x >= maxPos[y] * 4)
		return;

	int x1 = x / 4; 
    int group = x % 4;
	int a = group % 2;
	int b = group / 2;

	Plot(pixels, pos[x1 + y * maxTotalPos].x + a, pos[x1 + y * maxTotalPos].y + b, backupBuffer[x1 * 4 + group + y * maxTotalPos * 4]);
}

__kernel void Backup(__global uint* pixels, __global __read_only int2* pos, __global __read_only uint* maxPos, int maxTotalPos,  __global uint* backupBuffer)
{
    int x = get_global_id(0);
	int y = get_global_id(1);
	if(x >= maxPos[y] * 4)
		return;

	int x1 = x / 4; 
    int group = x % 4;
	int a = group % 2;
	int b = group / 2;

	backupBuffer[x1 * 4 + group + y * maxTotalPos * 4] = Read(pixels, pos[x1 + y * maxTotalPos].x + a,  pos[x1 + y * maxTotalPos].y + b );
}

__kernel void Draw(__global uint* pixels, __global __read_only int2* pos, __global __read_only uint* maxPos, int maxTotalPos, __global __read_only uint* color, __global __read_only uint* fade)
{
    int x = get_global_id(0);
	int y = get_global_id(1);
	if(x >= maxPos[y])
		return;

	BlendBilerp(pixels, pos[x + y * maxTotalPos].x, pos[x + y * maxTotalPos].y, color[x + y * maxTotalPos], fade[y]);
}