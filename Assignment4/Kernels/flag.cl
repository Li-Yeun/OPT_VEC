#define MAP_WIDTH 4096
#define MAP_HEIGHT 2732

inline uint ScaleColor( const uint c, const uint scale )
{
	const uint rb = (((c & 0xff00ff) * scale) >> 8) & 0x00ff00ff;
	const uint ag = (((c & 0xff00ff00) >> 8) * scale) & 0xff00ff00;
	return rb + ag;
}

inline uint Read(__global uint* pixels, int x, int y )
{
	if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return 0;
	return pixels[x + y * MAP_WIDTH];
}

inline void Blend(__global uint* pixels, int x, int y, uint c, uint w )
{
	if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return;
	pixels[x + y * MAP_WIDTH] = ScaleColor( c, w ) + ScaleColor( pixels[x + y * MAP_WIDTH], 255 - w );
}

inline void Plot(__global uint* pixels, int x, int y, uint c )
{
	if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return;
	pixels[x + y * MAP_WIDTH] = c;
}

inline void PlotBilerp(__global uint* pixels, float x, float y, uint c )
{
	int2 intPos = (int2)( (int)x, (int)y );
	float frac_x = x - intPos.x;
	float frac_y = y - intPos.y;
	int w1 = (int)(256 * ((1 - frac_x) * (1 - frac_y)));
	int w2 = (int)(256 * (frac_x * (1 - frac_y)));
	int w3 = (int)(256 * ((1 - frac_x) * frac_y));
	int w4 = (int)(256 * (frac_x * frac_y));
	Blend(pixels, intPos.x, intPos.y, c, w1 );
	Blend(pixels, intPos.x + 1, intPos.y, c, w2 );
	Blend(pixels, intPos.x, intPos.y + 1, c, w3 );
	Blend(pixels, intPos.x + 1, intPos.y + 1, c, w4 );
}

__kernel void Backup(__global uint* pixels,  __global __read_only float2* pos, __global uint* backup, __global bool* hasBackup, int spriteWidth, int spriteHeight, int posOffset, int backUpOffset)
{
    // x = spriteWidth * spriteHeight * 4
    // y = total flags
    int x = get_global_id(0);
    int y = get_global_id(1);

    int x1 = (x / spriteHeight) % (spriteWidth);
    int y1 = x % spriteHeight;

    int type = x / (spriteWidth * spriteHeight);
    int a = type % 2;
    int b = type / 2;

    int index = x1 + y1 * spriteWidth;

    float2 p = pos[index + posOffset * y];

    backup[index * 4 + type + backUpOffset * y] = Read(pixels, (int)p.x + a, (int)p.y + b );

    hasBackup[y] = true;
}


__kernel void Draw(__global uint* pixels, __global __read_only uint* color,  __global float2* pos, int spriteWidth, int spriteHeight, int posOffset)
{
    // x = spriteWidth * spriteHeight * 4
    // y = total flags
    int x = get_global_id(0);
    int y = get_global_id(1);

    int x1 = (x / spriteHeight) % (spriteWidth);
    int y1 = x % spriteHeight;

    //int w = x % (spriteWidth * 4); // y
    //int type = w / spriteWidth;

    int index = x1 + y1 * spriteWidth;

    float2 p = pos[index + posOffset * y];

    PlotBilerp(pixels, p.x, p.y, color[index] );

    /*
    int group = x / (spriteWidth * spriteHeight);
    int2 intPos = (int2)( (int)p.x, (int)p.y );
	float frac_x = x - intPos.x;
	float frac_y = y - intPos.y;

    int w1 = ((int)(256 * ((1 - frac_x) * (1 - frac_y)))) * (1-a) * (1-b);
	int w2 = ((int)(256 * (frac_x * (1 - frac_y)))) * (a) * (1-b);
	int w3 = ((int)(256 * ((1 - frac_x) * frac_y))) * (1-a) * b;
	int w4 = ((int)(256 * (frac_x * frac_y))) * a * b;

	Blend(pixels, intPos.x + a, intPos.y + b, color[index], w1 + w2 + w3 + w4 );
    */
    

    /*
	int2 intPos = (int2)( (int)p.x, (int)p.y );

	float frac_x = x - intPos.x;
	float frac_y = y - intPos.y;

    int a = type / 2;
    int b = type % 2;

	int w1 = ((int)(256 * ((1 - frac_x) * (1 - frac_y)))) * (1-a) * (1-b);
	int w2 = ((int)(256 * (frac_x * (1 - frac_y)))) * (a) * (1-b);
	int w3 = ((int)(256 * ((1 - frac_x) * frac_y))) * (1-a) * b;
	int w4 = ((int)(256 * (frac_x * frac_y))) * a * b;


	Blend(pixels, intPos.x + a, intPos.y + b, color[index], w1 + w2 + w3 + w4);
    */
}


__kernel void Remove(__global uint* pixels, __global __read_only float2* pos, __global uint* backup,  __global bool* hasBackup, int spriteWidth, int spriteHeight, int posOffset, int backUpOffset)
{
    // x = spriteWidth * spriteHeight * 4
    // y = total flags
    int y = get_global_id(1);

    if(!hasBackup[y])
        return;

    int x = get_global_id(0);
    int x1 = (x / spriteHeight) % (spriteWidth);
    int y1 = x % spriteHeight;

    int type = x / (spriteWidth * spriteHeight);
    int a = type % 2;
    int b = type / 2;

    int index = x1 + y1 * spriteWidth;

    Plot(pixels, (int)pos[index + posOffset * y].x + a, (int)pos[index + posOffset * y].y + b, backup[index * 4 + type + backUpOffset * y] );
}