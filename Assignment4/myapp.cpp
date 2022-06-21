#include "precomp.h"

TheApp* CreateApp() { return new MyApp(); }

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
int totalTanks, tankCounter;
void MyApp::Init()
{	
	std::cout << sizeof(int2) << std::endl;
	//std::cout << sizeof(uint) << std::endl;
	// 
	// load tank sprites
	tank1 = new Sprite("assets/tanks.png", make_int2(128, 100), make_int2(310, 360), 36, 256);
	tank2 = new Sprite("assets/tanks.png", make_int2(327, 99), make_int2(515, 349), 36, 256);

	// Initialize variables
	int tank1_sprite_size = tank1->frameSize * tank1->frameSize * tank1->frameCount;
	int tank2_sprite_size = tank2->frameSize * tank2->frameSize * tank2->frameCount;

	uint* tank_sprites = new uint[tank1_sprite_size + tank2_sprite_size];
	std::copy(tank1->pixels, tank1->pixels + tank1_sprite_size, tank_sprites);
	std::copy(tank2->pixels, tank2->pixels + tank2_sprite_size, tank_sprites + tank1_sprite_size);

	int group1 = 16, group2 = 12, group3 = 8;
	totalTanks = (group1 * group1 + group2 * group2 + group3 * group3) * 2;
	tankCounter = 0;

	tankTeam = new bool[totalTanks];
	MyApp::tankPos = new float2[totalTanks];
	MyApp::tankLastPos = new int2[totalTanks];
	MyApp::tankFrame = new int[totalTanks];
	MyApp::tankLastTarget = new bool[totalTanks];

	//tank tracks
	MyApp::tankOldPos = new float2[totalTanks];
	MyApp::tankDir = new float2[totalTanks];
	MyApp::tankSteer = new float[totalTanks];

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
	for (int y = 0; y < group1; y++) for (int x = 0; x < group1; x++) // main groups
	{
		Actor* army1Tank = new Tank( tank1, make_int2( 520 + x * 32, 2420 - y * 32 ), make_int2( 5000, -500 ), 0, 0, tankCounter++);
		Actor* army2Tank = new Tank( tank2, make_int2( 3300 - x * 32, y * 32 + 700 ), make_int2( -1000, 4000 ), 10, 1, tankCounter++);
		actorPool.push_back( army1Tank );
		actorPool.push_back( army2Tank );

	}
	for (int y = 0; y < group2; y++) for (int x = 0; x < group2; x++) // backup
	{
		Actor* army1Tank = new Tank( tank1, make_int2( 40 + x * 32, 2620 - y * 32 ), make_int2( 5000, -500 ), 0, 0, tankCounter++);
		Actor* army2Tank = new Tank( tank2, make_int2( 3900 - x * 32, y * 32 + 300 ), make_int2( -1000, 4000 ), 10, 1, tankCounter++);
		actorPool.push_back( army1Tank );
		actorPool.push_back( army2Tank );
	}
	for (int y = 0; y < group3; y++) for (int x = 0; x < group3; x++) // small forward groups
	{
		Actor* army1Tank = new Tank( tank1, make_int2( 1440 + x * 32, 2220 - y * 32 ), make_int2( 3500, -500 ), 0, 0, tankCounter++);
		Actor* army2Tank = new Tank( tank2, make_int2( 2400 - x * 32, y * 32 + 900 ), make_int2( 1300, 4000 ), 128, 1, tankCounter++);
		actorPool.push_back( army1Tank );
		actorPool.push_back( army2Tank );
	}

	/*
	// load mountain peaks
	Surface mountains( "assets/peaks.png" );
	for (int y = 0; y < mountains.height; y++) for (int x = 0; x < mountains.width; x++)
	{
		uint p = mountains.pixels[x + y * mountains.width];
		if ((p & 0xffff) == 0) peaks.push_back( make_float3( make_int3( x * 8, y * 8, (p >> 16) & 255 ) ) );
	}
	// add sandstorm
	for (int i = 0; i < 7500; i += 4)
	{
		int x[4] = { RandomUInt() % map.bitmap->width, RandomUInt() % map.bitmap->width, RandomUInt() % map.bitmap->width, RandomUInt() % map.bitmap->width };
		int y[4] = { RandomUInt() % map.bitmap->height, RandomUInt() % map.bitmap->height, RandomUInt() % map.bitmap->height, RandomUInt() % map.bitmap->height };
		uint d[4] = { (RandomUInt() & 15) - 8, (RandomUInt() & 15) - 8, (RandomUInt() & 15) - 8, (RandomUInt() & 15) - 8 };
		Sprite* s[4] = { bush[(4 * i) % 3], bush[(4 * i + 1) % 3], bush[(4 * i + 2) % 3], bush[(4 * i + 3) % 3] };
		float2 p[4] = { make_float2( x[0], y[0] ), make_float2( x[1], y[1] ), make_float2( x[2], y[2] ), make_float2( x[3], y[3] ) };
		uint c[4] = { map.bitmap->pixels[x[0] + y[0] * map.bitmap->width], map.bitmap->pixels[x[1] + y[1] * map.bitmap->width], map.bitmap->pixels[x[2] + y[2] * map.bitmap->width], map.bitmap->pixels[x[3] + y[3] * map.bitmap->width] };
		sand.push_back( new Particle( s, p, c, d ) );
	}
	// place flags
	Surface* flagPattern = new Surface( "assets/flag.png" );
	VerletFlag* flag1 = new VerletFlag( make_int2( 3000, 848 ), flagPattern );
	actorPool.push_back( flag1 );
	VerletFlag* flag2 = new VerletFlag( make_int2( 1076, 1870 ), flagPattern );
	actorPool.push_back( flag2 );
	*/
	// initialize map view
	map.UpdateView( screen, zoom );
	
	// Initialize Kernels
	tankDrawKernel = new Kernel("Kernels/renderer.cl", "Draw");

	deviceBuffer = new Buffer(map.width * map.height, 0, map.bitmap->pixels);

	tankSpriteBuffer = new Buffer(tank1_sprite_size + tank2_sprite_size, 0, tank_sprites);
	tankTeamBuffer = new Buffer(totalTanks / 4, 0, tankTeam);
	tankPosBuffer = new Buffer(totalTanks * 2, 0, tankPos);
	tankFrameBuffer = new Buffer(totalTanks, 0, tankFrame);

	tankDrawKernel->SetArgument(0, deviceBuffer);
	tankDrawKernel->SetArgument(1, tankSpriteBuffer);
	tankDrawKernel->SetArgument(2, tankTeamBuffer);
	tankDrawKernel->SetArgument(3, tankPosBuffer);
	tankDrawKernel->SetArgument(4, tankFrameBuffer);
	tankDrawKernel->SetArgument(5, tank1->frameSize);
	tankDrawKernel->SetArgument(6, tank1->frameCount);


	deviceBuffer->CopyToDevice(true);
	tankSpriteBuffer->CopyToDevice(true);
	tankTeamBuffer->CopyToDevice(true);
	tankPosBuffer->CopyToDevice(true);
	tankFrameBuffer->CopyToDevice(true);


	tankBackupKernel = new Kernel("Kernels/renderer.cl", "Backup");

	tankBackupBuffer = new Buffer(totalTanks * sqr(tank1->frameSize + 1));
	tankLastTargetBuffer = new Buffer(totalTanks / 4, 0, tankLastTarget);

	tankBackupKernel->SetArgument(0, deviceBuffer);
	tankBackupKernel->SetArgument(1, tankPosBuffer);
	tankBackupKernel->SetArgument(2, tankBackupBuffer);
	tankBackupKernel->SetArgument(3, tankLastTargetBuffer);
	tankBackupKernel->SetArgument(4, tank1->frameSize);

	tankLastTargetBuffer->CopyToDevice(true);


	tankSaveLastPosKernel = new Kernel("Kernels/renderer.cl", "SaveLastPos");

	tankLastPosBuffer = new Buffer(totalTanks * 2, 0, tankLastPos);

	tankSaveLastPosKernel->SetArgument(0, tankPosBuffer);
	tankSaveLastPosKernel->SetArgument(1, tankLastPosBuffer);
	tankSaveLastPosKernel->SetArgument(2, tankLastTargetBuffer);
	tankSaveLastPosKernel->SetArgument(3, tank1->frameSize);

	tankLastPosBuffer->CopyToDevice(true);

	tankRemoveKernel = new Kernel("Kernels/renderer.cl", "Remove");
	tankRemoveKernel->SetArgument(0, deviceBuffer);
	tankRemoveKernel->SetArgument(1, tankLastPosBuffer);
	tankRemoveKernel->SetArgument(2, tankBackupBuffer);
	tankRemoveKernel->SetArgument(3, tankLastTargetBuffer);
	tankRemoveKernel->SetArgument(4, tank1->frameSize);

	// Tank tracks
	tankTrackKernel = new Kernel("Kernels/track.cl", "Track");
	tankOldPosBuffer = new Buffer(totalTanks * 2, 0, tankOldPos);
	tankDirBuffer = new Buffer(totalTanks * 2, 0, tankDir);
	tankSteerBuffer = new Buffer(totalTanks, 0, tankSteer);

	//.....
	tankTrackKernel->SetArgument(0, deviceBuffer);
	tankTrackKernel->SetArgument(1, tankOldPosBuffer);
	tankTrackKernel->SetArgument(2, tankDirBuffer);
	tankTrackKernel->SetArgument(3, tankSteerBuffer);

	tankOldPosBuffer->CopyToDevice(true);
	tankDirBuffer->CopyToDevice(true);
	tankSteerBuffer->CopyToDevice(true);


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
	Timer t;

	/*
	deviceBuffer->CopyFromDevice(false);
	tankDrawKernel->Run(1);
	*/

	// draw the map
	map.Draw( screen );
	// rebuild actor grid
	grid.Clear();
	grid.Populate( actorPool );
	// update and render actors
	pointer->Remove();

	tankRemoveKernel->Run2D(int2(tank1->frameSize * tank1->frameSize, totalTanks), int2(tank1->frameSize, 1));
	//for (int s = (int)sand.size(), i = s - 1; i >= 0; i--) sand[i]->Remove();
	//for (int s = (int)actorPool.size(), i = s - 1; i >= 0; i--) actorPool[i]->Remove();
	// 
	//for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Tick();
	for (int i = 0; i < (int)actorPool.size(); i++) if (!actorPool[i]->Tick())
	{
		// actor got deleted, replace by last in list
		Actor* lastActor = actorPool.back();
		Actor* toDelete = actorPool[i];
		actorPool.pop_back();
		if (lastActor != toDelete) actorPool[i] = lastActor;
		delete toDelete;
		i--;
	}
	coolDown++;

	tankPosBuffer->CopyToDevice(true);
	tankFrameBuffer->CopyToDevice(true);
	tankOldPosBuffer->CopyToDevice(true);
	tankDirBuffer->CopyToDevice(true);
	tankSteerBuffer->CopyToDevice(true);

	tankTrackKernel->Run(totalTanks);

	tankBackupKernel->Run2D(int2(tank1->frameSize * tank1->frameSize, totalTanks), int2(tank1->frameSize, 1));
	tankSaveLastPosKernel->Run(totalTanks);
	tankDrawKernel->Run2D(int2((tank1->frameSize - 1)* (tank1->frameSize - 1), totalTanks), int2(tank1->frameSize - 1, 1));

	deviceBuffer->CopyFromDevice(true);
	//for (int s = (int)actorPool.size(), i = 0; i < s; i++) actorPool[i]->Draw();
	//for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Draw();
	int2 cursorPos = map.ScreenToMap( mousePos );
	pointer->Draw( map.bitmap, make_float2( cursorPos ), 0 );
	// handle mouse
	HandleInput();
	// report frame time
	static float frameTimeAvg = 10.0f; // estimate
	frameTimeAvg = 0.95f * frameTimeAvg + 0.05f * t.elapsed() * 1000;
	printf( "frame time: %5.2fms\n", frameTimeAvg );
}