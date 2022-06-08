#include "precomp.h"

uint ReadBilerp( Surface& bitmap, float u, float v )
{
	// read from a bitmap with bilinear interpolation.
	// warning: not optimized.
	int iu1 = (int)u % bitmap.width, iv1 = (int)v % bitmap.height;
	int iu2 = (iu1 + 1) % bitmap.width, iv2 = (iv1 + 1) % bitmap.height;
	int fracu = (int)((u - floorf( u )) * 16383), fracv = (int)((v - floorf( v )) * 16383);
	return ScaleColor( bitmap.pixels[iu1 + iv1 * bitmap.width], ((16383 - fracu) * (16384 - fracv)) >> 20 ) +
		ScaleColor( bitmap.pixels[iu2 + iv1 * bitmap.width], (fracu * (16384 - fracv)) >> 20 ) +
		ScaleColor( bitmap.pixels[iu1 + iv2 * bitmap.width], ((16383 - fracu) * fracv) >> 20 ) +
		ScaleColor( bitmap.pixels[iu2 + iv2 * bitmap.width], (fracu * fracv) >> 20 );
}

Sprite::Sprite( const char* fileName )
{
	// load original bitmap
	Surface original( fileName );
	// copy to internal data
	frameCount = 1;
	frameSize = original.width;
	pixels = new uint[frameSize * frameSize];
	memcpy( pixels, original.pixels, frameSize * frameSize * 4 );
	// fix alpha
	for (int i = 0; i < frameSize * frameSize; i++)
	{
		pixels[i] &= 0xffffff;
		if (pixels[i] == 0xff00ff) pixels[i] = 0; else pixels[i] |= 0xff000000;
	}
}

Sprite::Sprite( const char* fileName, int frames )
{
	// load original bitmap
	Surface original( fileName );
	// copy to internal data
	frameCount = frames;
	frameSize = original.width / frameCount;
	pixels = new uint[frameSize * frameSize * frameCount];
	memcpy( pixels, original.pixels, frameSize * frameSize * frameCount * 4 );
}

Sprite::Sprite( const char* fileName, int2 topLeft, int2 bottomRight, int size, int frames )
{
	// load original bitmap
	Surface original( fileName );
	// update alpha
	const uint pixelCount = original.width * original.height;
	for (uint i = 0; i < pixelCount; i++)
	{
		original.pixels[i] &= 0xffffff;
		if (original.pixels[i] == 0xff00ff) original.pixels[i] = 0; else original.pixels[i] |= 0xff000000;
	}
	// blur alpha for better outlines
	uint* tmp = new uint[pixelCount], w = original.width;
	for (int j = 0; j < 4; j++)
	{
		for (uint i = w + 1; i < pixelCount - w; i++)
		{
			uint a1 = original.pixels[i + 1] >> 24, a2 = original.pixels[i - 1] >> 24;
			uint a3 = original.pixels[i + w] >> 24, a4 = original.pixels[i - w] >> 24;
			uint ac = original.pixels[i] >> 24;
			uint a = (2 * ac + a1 + a2 + a3 + a4) / 6;
			tmp[i] = (original.pixels[i] & 0xffffff) + (a << 24);
		}
		memcpy( original.pixels + w + 1, tmp + w + 1, pixelCount - (2 * w + 1) );
	}
	delete tmp;
	// create quad outline tables
	static float* xleft = 0, * xright = 0, * uleft, * vleft, * uright, * vright;
	if (!xleft)
	{
		xleft = new float[size], xright = new float[size];
		uleft = new float[size], uright = new float[size];
		vleft = new float[size], vright = new float[size];
		for (int i = 0; i < size; i++) xleft[i] = (float)size - 1, xright[i] = 0;
	}
	// produce rotated frames
	pixels = new uint[size * frames * size];
	memset( pixels, 0, size * frames * size * 4 );
	float2 p[4] = { make_float2( -1, -1 ), make_float2( 1, -1 ), make_float2( 1, 1 ), make_float2( -1, 1 ) };
	float2 uv[4] = {
		make_float2( (float)topLeft.x, (float)topLeft.y ), make_float2( (float)bottomRight.x, (float)topLeft.y ),
		make_float2( (float)bottomRight.x, (float)bottomRight.y ), make_float2( (float)topLeft.x, (float)bottomRight.y )
	};
	for (int miny = size, maxy = 0, frame = 0; frame < frames; frame++)
	{
		// rotate a square
		float2 pos[4];
		float angle = (2 * PI * frame) / frames;
		for (int j = 0; j < 4; j++)
			pos[j].x = (p[j].x * cosf( angle ) + p[j].y * sinf( angle )) * 0.35f * size + size / 2,
			pos[j].y = (p[j].x * sinf( angle ) - p[j].y * cosf( angle )) * 0.35f * size + size / 2;
		// populate outline tables
		for (int j = 0; j < 4; j++)
		{
			int h, vert0 = j, vert1 = (j + 1) & 3;
			if (pos[vert0].y > pos[vert1].y) h = vert0, vert0 = vert1, vert1 = h;
			const float y0 = pos[vert0].y, y1 = pos[vert1].y, rydiff = 1.0f / (y1 - y0);
			if (y0 == y1) continue;
			const int iy0 = max( 1, (int)y0 + 1 ), iy1 = min( size - 1, (int)y1 );
			float x0 = pos[vert0].x, dx = (pos[vert1].x - x0) * rydiff,
				u0 = uv[vert0].x, du = (uv[vert1].x - u0) * rydiff,
				v0 = uv[vert0].y, dv = (uv[vert1].y - v0) * rydiff;
			const float f = (float)iy0 - y0;
			x0 += dx * f, u0 += du * f, v0 += dv * f;
			for (int y = iy0; y <= iy1; y++, x0 += dx, u0 += du, v0 += dv)
			{
				if (x0 < xleft[y]) xleft[y] = x0, uleft[y] = u0, vleft[y] = v0;
				if (x0 > xright[y]) xright[y] = x0, uright[y] = u0, vright[y] = v0;
			}
			miny = min( miny, iy0 ), maxy = max( maxy, iy1 );
		}
		// fill the rotated quad using the outline tables
		for (int y = miny; y <= maxy; xleft[y] = (float)size - 1, xright[y++] = 0)
		{
			float x0 = xleft[y], x1 = xright[y], rxdiff = 1.0f / (x1 - x0),
				u0 = uleft[y], du = (uright[y] - u0) * rxdiff,
				v0 = vleft[y], dv = (vright[y] - v0) * rxdiff;
			const int ix0 = (int)x0 + 1, ix1 = min( SCRWIDTH - 2, (int)x1 );
			u0 += ((float)ix0 - x0) * du, v0 += ((float)ix0 - x0) * dv;
			uint* dest = pixels + frame * size + y * size * frames;
			for (int x = ix0; x <= ix1; x++, u0 += du, v0 += dv) dest[x] = ReadBilerp( original, u0, v0 );
		}
	}
	frameCount = frames;
	frameSize = size;
}

void Sprite::SetKernel(Buffer* posBuffer, Buffer* frameBuffer) 
{
	sprite_kernel = new Kernel("render.cl", "render");
	sprite_buffer = new Buffer(frameSize * frameSize * frameCount, 0, pixels);

	sprite_kernel->SetArgument(0, MyApp::deviceBuffer);
	sprite_kernel->SetArgument(1, sprite_buffer);
	sprite_kernel->SetArgument(2, posBuffer);
	sprite_kernel->SetArgument(3, frameBuffer);
	sprite_kernel->SetArgument(4, frameSize);
	sprite_kernel->SetArgument(5, frameCount);

}

void Sprite::ScaleAlpha( uint scale )
{
	for (int i = 0; i < frameSize * frameSize * frameCount; i++)
	{
		int a = ((pixels[i] >> 24) * scale) >> 8;
		pixels[i] = (pixels[i] & 0xffffff) + (a << 24);
	}
}

void SpriteInstance::Draw( Surface* target, float2 pos, int frame )
{
	// save the area of target that we are about to overwrite
	if (!backup) backup = new uint[sqr( sprite->frameSize + 1 )];
	int2 intPos = make_int2( pos );
	int x1 = intPos.x - sprite->frameSize / 2, x2 = x1 + sprite->frameSize;
	int y1 = intPos.y - sprite->frameSize / 2, y2 = y1 + sprite->frameSize;
	if (x1 < 0 || y1 < 0 || x2 >= target->width || y2 >= target->height)
	{
		// out of range; skip
		lastTarget = 0;
		return;
	}
	lastPos = make_int2( x1, y1 );
	lastTarget = target;

	int sprite_frameSize = sprite->frameSize;
	
	//MyApp::backup_kernel->SetArgument(1, backup_buffer);
	//MyApp::backup_kernel->SetArgument(2, lastPos.x);
	//MyApp::backup_kernel->SetArgument(3, lastPos.y);
	//MyApp::backup_kernel->Run(sprite_frameSize * sprite_frameSize);
	
	//MyApp::render_kernel->SetArgument(2, pos);
	//MyApp::render_kernel->SetArgument(3, frame);
	//MyApp::render_kernel->Run((sprite_frameSize-1)* (sprite_frameSize-1));
	

}

void SpriteInstance::DrawAdditive( Surface* target, float2 pos, int frame )
{
	// save the area of target that we are about to overwrite
	if (!backup) backup = new uint[sprite->frameSize * sprite->frameSize];
	int2 intPos = make_int2( pos );
	int x1 = intPos.x - sprite->frameSize / 2, x2 = x1 + sprite->frameSize;
	int y1 = intPos.y - sprite->frameSize / 2, y2 = y1 + sprite->frameSize;
	if (x1 < 0 || y1 < 0 || x2 >= target->width || y2 >= target->height)
	{
		// out of range; skip
		lastTarget = 0;
		return;
	}
	//MyApp::remove_kernel->SetArgument(1, backup_buffer);
	//MyApp::remove_kernel->SetArgument(2, lastPos.x);
	//MyApp::remove_kernel->SetArgument(3, lastPos.y);
	//MyApp::remove_kernel->Run(sprite->frameSize * sprite->frameSize);
	// draw the sprite frame
	for (int v = 0; v < sprite->frameSize; v++) for (int u = 0; u < sprite->frameSize; u++)
	{
		uint* dst = target->pixels + x1 + u + (y1 + v) * target->width;
		uint pix = sprite->pixels[frame * sprite->frameSize + u + v * sprite->frameCount * sprite->frameSize];
		*dst = AddBlend( *dst, pix );
	}
	// remember where we drew so it can be removed later
	lastPos = make_int2( x1, y1 );
	lastTarget = target;
}

void SpriteInstance::Remove()
{
	// use the stored pixels to restore the rectangle affected by the sprite.
	// note: sprites must be removed in reverse order to guarantee correct removal.
	if (lastTarget) for (int v = 0; v < sprite->frameSize; v++)
	{
		//MyApp::remove_kernel->SetArgument(2, lastPos.x);
		//MyApp::remove_kernel->SetArgument(3, lastPos.y);
		//MyApp::remove_kernel->Run(sprite->frameSize * sprite->frameSize);
	}
}