#define MAP_WIDTH 4096
#define SPRITE_FRAMECOUNT 256
#define SPRITE_FRAMESIZE 36

inline uint ScaleColor( const uint c, const uint scale )
{
	const uint rb = (((c & 0xff00ff) * scale) >> 8) & 0x00ff00ff;
	const uint ag = (((c & 0xff00ff00) >> 8) * scale) & 0xff00ff00;
	return rb + ag;
}
__kernel void render(__global uint* buffer, __global uint* sprite, float2 pos, int frame)
{
    int x = get_global_id(0);
    int v = x / (SPRITE_FRAMESIZE - 1);
    int u = x % (SPRITE_FRAMESIZE - 1);
    //for (int y = 0; y < 400; y++)
	//	for (int x = 0; x < 200; x++)
	//buffer[x + y * MAP_WIDTH] = 0xFF00FFFF;

	// calculate bilinear weights - these are constant in this case.
    int x1 = pos.x - SPRITE_FRAMESIZE / 2, x2 = x1 + SPRITE_FRAMESIZE;
	int y1 = pos.y - SPRITE_FRAMESIZE / 2, y2 = y1 + SPRITE_FRAMESIZE;

	uint frac_x = (int)(255.0f * (pos.x - (int)( pos.x )));
	uint frac_y = (int)(255.0f * (pos.y - (int)( pos.y )));
	uint w0 = (frac_x * frac_y) >> 8;
	uint w1 = ((255 - frac_x) * frac_y) >> 8;
	uint w2 = (frac_x * (255 - frac_y)) >> 8;
	uint w3 = ((255 - frac_x) * (255 - frac_y)) >> 8;
	// draw the sprite frame
	uint stride = SPRITE_FRAMECOUNT * SPRITE_FRAMESIZE;
	uint dst = x1 + (y1 + v) * MAP_WIDTH + u;
	uint src = frame * SPRITE_FRAMESIZE + v * stride + u;
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