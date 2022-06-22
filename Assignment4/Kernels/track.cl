#define MAP_WIDTH 4096
#define MAP_HEIGHT 2732

inline uint ScaleColor( const uint c, const uint scale )
{
	const uint rb = (((c & 0xff00ff) * scale) >> 8) & 0x00ff00ff;
	const uint ag = (((c & 0xff00ff00) >> 8) * scale) & 0xff00ff00;
	return rb + ag;
}

inline void Blend(__global uint* pixels, int x, int y, uint c, uint w )
{
	if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return;
	pixels[x + y * MAP_WIDTH] = ScaleColor( c, w ) + ScaleColor( pixels[x + y * MAP_WIDTH], 255 - w );
}

inline void BlendBilerp(__global uint* pixels, float x, float y, uint c, uint w )
{
	int2 intPos = (int2)( (int)x, (int)y );
	float frac_x = x - intPos.x;
	float frac_y = y - intPos.y;
	int w1 = (int)(256 * ((1 - frac_x) * (1 - frac_y)));
	int w2 = (int)(256 * (frac_x * (1 - frac_y)));
	int w3 = (int)(256 * ((1 - frac_x) * frac_y));
	int w4 = (int)(256 * (frac_x * frac_y));

	Blend(pixels, intPos.x, intPos.y, c, (w1 * w) >> 8 );
	Blend(pixels, intPos.x + 1, intPos.y, c, (w2 * w) >> 8 );
	Blend(pixels, intPos.x, intPos.y + 1, c, (w3 * w) >> 8 );
	Blend(pixels, intPos.x + 1, intPos.y + 1, c, (w4 * w) >> 8 );
}

__kernel void Track(__global uint* pixels, __global __read_only float2* oldPos, __global __read_only float2* dir, __global __read_only float* steer)
{
    int x = get_global_id(0);
    if (steer[x] >= -0.2f && steer[x] <= 0.2f)
    { 
		// draw tank tracks, only when not turning
        float2 perp = (float2) ( - dir[x].y, dir[x].x );
		float2 trackPos1 = oldPos[x] - 9 * dir[x] + 4.5f * perp;
		float2 trackPos2 = oldPos[x] - 9 * dir[x] - 5.5f * perp;

		BlendBilerp(pixels, trackPos1.x, trackPos1.y, 0, 12 );
		BlendBilerp(pixels, trackPos2.x, trackPos2.y, 0, 12 );
	}
}