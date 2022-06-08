#include "precomp.h"

TheApp* CreateApp() { return new MyApp(); }

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
int totalTanks;
void MyApp::Init()
{	
	render_kernel = new Kernel("render.cl", "render");
	// load tank sprites
	tank1 = new Sprite( "assets/tanks.png", make_int2( 128, 100 ), make_int2( 310, 360 ), 36, 256 );
	tank2 = new Sprite( "assets/tanks.png", make_int2( 327, 99 ), make_int2( 515, 349 ), 36, 256 );
	int tank1_size = tank1->frameSize * tank1->frameSize * tank1->frameCount;
	int tank2_size = tank2->frameSize * tank2->frameSize * tank2->frameCount;

	uint* tank_sprites = new uint[tank1_size + tank2_size];
	std::copy(tank1->pixels, tank1->pixels + tank1_size, tank_sprites);
	std::copy(tank2->pixels, tank2->pixels + tank2_size, tank_sprites + tank1_size);

	// load bush sprite for dust streams
	bush[0] = new Sprite( "assets/bush1.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 10, 256 );
	bush[1] = new Sprite( "assets/bush2.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 14, 256 );
	bush[2] = new Sprite( "assets/bush3.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 20, 256 );
	bush[0]->ScaleAlpha( 96 );
	bush[1]->ScaleAlpha( 64 );
	bush[2]->ScaleAlpha( 128 );
	// pointer
	pointer = new SpriteInstance( new Sprite( "assets/pointer.png" ) );
	// create armies
	int id = 0;
	int group1 = 16, group2 = 12, group3 = 8;
	totalTanks = (group1 * group1 + group2 * group2 + group3 * group3) * 2;
	tankPos = new float2[totalTanks];
	tankFrame = new int[totalTanks];
	tankLastTarget = new int[totalTanks];
	tankSprite = new bool[totalTanks];

	for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) // main groups
	{
		Tank* army1Tank = new Tank( tank1, make_int2( 520 + x * 32, 2420 - y * 32 ), make_int2( 5000, -500 ), 0, 0, id++ );
		Tank* army2Tank = new Tank( tank2, make_int2( 3300 - x * 32, y * 32 + 700 ), make_int2( -1000, 4000 ), 10, 1, id++ );
		actorPool.push_back(army1Tank);
		actorPool.push_back(army2Tank);

	}
	for (int y = 0; y < 12; y++) for (int x = 0; x < 12; x++) // backup
	{
		Tank* army1Tank = new Tank( tank1, make_int2( 40 + x * 32, 2620 - y * 32 ), make_int2( 5000, -500 ), 0, 0, id++ );
		Tank* army2Tank = new Tank( tank2, make_int2( 3900 - x * 32, y * 32 + 300 ), make_int2( -1000, 4000 ), 10, 1, id++);
		actorPool.push_back(army1Tank);
		actorPool.push_back(army2Tank);

	}
	for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) // small forward groups
	{
		Tank* army1Tank = new Tank( tank1, make_int2( 1440 + x * 32, 2220 - y * 32 ), make_int2( 3500, -500 ), 0, 0, id++);
		Tank* army2Tank = new Tank( tank2, make_int2( 2400 - x * 32, y * 32 + 900 ), make_int2( 1300, 4000 ), 128, 1, id++ );
		actorPool.push_back(army1Tank);
		actorPool.push_back(army2Tank);
	}
	// load mountain peaks
	Surface mountains( "assets/peaks.png" );
	for (int y = 0; y < mountains.height; y++) for (int x = 0; x < mountains.width; x++)
	{
		uint p = mountains.pixels[x + y * mountains.width];
		if ((p & 0xffff) == 0) peaks.push_back( make_float3( make_int3( x * 8, y * 8, (p >> 16) & 255 ) ) );
	}
	// add sandstorm
	for( int i = 0; i < 7500; i++ )
	{
		int x = RandomUInt() % map.bitmap->width;
		int y = RandomUInt() % map.bitmap->height;
		int d = (RandomUInt() & 15) - 8;
		sand.push_back( new Particle( bush[i % 3], make_int2( x, y ), map.bitmap->pixels[x + y * map.bitmap->width], d ) );
	}
	// place flags

	Surface* flagPattern = new Surface( "assets/flag.png" );
	VerletFlag* flag1 = new VerletFlag( make_int2( 3000, 848 ), flagPattern );
	actorPool.push_back( flag1 );
	VerletFlag* flag2 = new VerletFlag( make_int2( 1076, 1870 ), flagPattern );
	actorPool.push_back( flag2 );

	// initialize map view
	map.UpdateView( screen, zoom );

	deviceBuffer = new Buffer(map.width * map.height, 0, Map::bitmap->pixels);
	spriteBuffer = new Buffer(tank1_size + tank2_size, 0, tank_sprites);

	tankPosBuffer = new Buffer(totalTanks * 2, 0, tankPos);
	tankLastPosBuffer = new Buffer(totalTanks * 2);
	tankBackUpBuffer = new Buffer(totalTanks * sqr(tank1->frameSize+1));
	tankLastTargetBuffer = new Buffer(totalTanks, 0, tankLastTarget);

	tankFrameBuffer = new Buffer(totalTanks, 0, tankFrame);

	tankSpriteBuffer = new Buffer(totalTanks / 4 , CL_MEM_READ_ONLY, tankSprite);

	render_kernel->SetArgument(0,deviceBuffer);
	render_kernel->SetArgument(1, spriteBuffer);
	render_kernel->SetArgument(2, tankPosBuffer);
	render_kernel->SetArgument(3, tankFrameBuffer);
	render_kernel->SetArgument(4, tank1->frameSize);
	render_kernel->SetArgument(5, tank1->frameCount);
	render_kernel->SetArgument(6, tankSpriteBuffer);
	
	deviceBuffer->CopyToDevice(true);
	spriteBuffer->CopyToDevice(true);
	tankSpriteBuffer->CopyToDevice(true);


	saveLastPos_kernel = new Kernel("render.cl", "saveLastPos");
	saveLastPos_kernel->SetArgument(0, tankPosBuffer);
	saveLastPos_kernel->SetArgument(1, tankLastPosBuffer);
	saveLastPos_kernel->SetArgument(2, tankLastTargetBuffer);
	saveLastPos_kernel->SetArgument(3, tank1->frameSize);

	backup_kernel = new Kernel("render.cl", "backup");
	backup_kernel->SetArgument(0, deviceBuffer);
	backup_kernel->SetArgument(1, tankBackUpBuffer);
	backup_kernel->SetArgument(2, tankLastPosBuffer);
	backup_kernel->SetArgument(3, tank1->frameSize);

	remove_kernel = new Kernel("render.cl", "remove");
	remove_kernel->SetArgument(0, deviceBuffer);
	remove_kernel->SetArgument(1, tankBackUpBuffer);
	remove_kernel->SetArgument(2, tankLastPosBuffer);
	remove_kernel->SetArgument(3, tankLastTargetBuffer);
	remove_kernel->SetArgument(4, tank1->frameSize);

}

// -----------------------------------------------------------
// Advanced zooming
// -----------------------------------------------------------
void MyApp::MouseWheel( float y )
{
	// fetch current pointer location
	int2 pointerPos = map.ScreenToMap( mousePos );
	// adjust zoom
	zoom -= 10 * y; 
	if (zoom < 20) zoom = 20; 
	if (zoom > 100) zoom = 100;
	// adjust focus so that pointer remains stationary, if possible
	map.UpdateView( screen, zoom );
	int2 newPointerPos = map.ScreenToMap( mousePos );
	map.SetFocus( map.GetFocus() + (pointerPos - newPointerPos) );
	map.UpdateView( screen, zoom );
}

// -----------------------------------------------------------
// Process mouse input
// -----------------------------------------------------------
void MyApp::HandleInput()
{
	// anything that happens only once at application start goes here
	static bool wasDown = false, dragging = false;
	if (mouseDown && !wasDown) dragging = true, dragStart = mousePos, focusStart = map.GetFocus();
	if (!mouseDown) dragging = false;
	wasDown = mouseDown;
	if (dragging)
	{
		int2 delta = dragStart - mousePos;
		delta.x = (int)((delta.x * zoom) / 32);
		delta.y = (int)((delta.y * zoom) / 32);
		map.SetFocus( focusStart + delta );
		map.UpdateView( screen, zoom );
	}
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void MyApp::Tick( float deltaTime )
{
	int tanks = 0;
	Timer t;
	// draw the map
	map.Draw( screen );
	// rebuild actor grid
	grid.Clear();
	grid.Populate( actorPool );
	// update and render actors
	pointer->Remove();
	//for (int s = (int)sand.size(), i = s - 1; i >= 0; i--) sand[i]->Remove();
	//for (int s = (int)actorPool.size(), i = s - 1; i >= 0; i--)
	{
	//	if (!actorPool[i]->GetType() == Actor::TANK)
	//		actorPool[i]->Remove();
	}

	//for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Tick();
	for (int i = 0; i < (int)actorPool.size(); i++)
	{

		if (!actorPool[i]->Tick())
		{
			// actor got deleted, replace by last in list
			Actor* lastActor = actorPool.back();
			Actor* toDelete = actorPool[i];
			actorPool.pop_back();
			if (lastActor != toDelete) actorPool[i] = lastActor;
			delete toDelete;
			i--;

		}
		if (actorPool[i]->GetType() == Actor::TANK)
			tanks++;
	}

	coolDown++;
	//for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Draw();
	//for (int s = (int)actorPool.size(), i = 0; i < s; i++)
	//{
	//	if(!actorPool[i]->GetType() == Actor::TANK)
	//		actorPool[i]->Draw();
	//}
	//deviceBuffer->CopyToDevice(true);

	tankPosBuffer ->CopyToDevice(true);
	tankFrameBuffer->CopyToDevice(true);
	remove_kernel->Run2D(int2(36 * 36, tanks), int2(36, 1));

	saveLastPos_kernel->Run(tanks);
	backup_kernel->Run2D(int2(36 * 36, tanks), int2(36, 1));

	render_kernel->Run2D(int2(35 * 35, tanks), int2(35, 1));

	deviceBuffer->CopyFromDevice(true);

	int2 cursorPos = map.ScreenToMap( mousePos );
	pointer->Draw( map.bitmap, make_float2( cursorPos ), 0 );
	// handle mouse
	HandleInput();
	// report frame time
	static float frameTimeAvg = 10.0f; // estimate
	frameTimeAvg = 0.95f * frameTimeAvg + 0.05f * t.elapsed() * 1000;
	printf( "frame time: %5.2fms\n", frameTimeAvg );
}