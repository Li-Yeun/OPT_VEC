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

inline uint ScaleColor( const uint c, const uint scale )
{
	const uint rb = (((c & 0xff00ff) * scale) >> 8) & 0x00ff00ff;
	const uint ag = (((c & 0xff00ff00) >> 8) * scale) & 0xff00ff00;
	return rb + ag;
}

__kernel void renderToScreen(__global uint* pixels, write_only image2d_t target, int4 view, int2 dxy)
{
    int threadIdx = get_global_id( 0 );
    int x = threadIdx % SCRWIDTH;
    int y = threadIdx / SCRWIDTH;

    uint y_fp = (view.y << 14) + y * dxy.y;
    uint x_fp = view.x << 14;
    const uint y_frac = y_fp & 16383;
    const uint x_frac = x_fp & 16383;

    uint mapPixel = ((y_fp >> 14) * MAP_WIDTH) + ((x_fp + (dxy.x * x)) >> 14);
    const uint w1 = ((16383 - x_frac) * (16383 - y_frac)) >> 20; // integer
	const uint w3 = ((16383 - x_frac) * y_frac) >> 20;
	const uint w2 = (x_frac * (16383 - y_frac)) >> 20;
	const uint w4 = 255 - (w1 + w2 + w3);
    
    const uint p1 = pixels[mapPixel];
    const uint p2 = pixels[mapPixel + 1];
    const uint p3 = pixels[mapPixel + MAP_WIDTH];
    const uint p4 = pixels[mapPixel + MAP_WIDTH + 1];

    uint pixel = ScaleColor( p1, w1 ) + ScaleColor( p2, w2 ) + ScaleColor( p3, w3 ) + ScaleColor( p4, w4 );

    write_imagef( target, (int2)(x, y), (float4)(RGB8toRGB32F(pixel), 1 ) );
}