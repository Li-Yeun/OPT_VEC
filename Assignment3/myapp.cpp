#include "precomp.h"

TheApp* CreateApp() { return new MyApp(); }

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
int totalTanks;
int totalBushes;
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

	int bush_size[3];
	bush_size[0] = bush[0]->frameSize * bush[0]->frameSize * bush[0]->frameCount;
	bush_size[1] = bush[1]->frameSize * bush[1]->frameSize * bush[1]->frameCount;
	bush_size[2] = bush[2]->frameSize * bush[2]->frameSize * bush[2]->frameCount;

	// pointer
	pointer = new SpriteInstance( new Sprite( "assets/pointer.png" ) );
	// create armies
	int id = 0;
	int group1 = 16, group2 = 12, group3 = 8;
	totalTanks = (group1 * group1 + group2 * group2 + group3 * group3) * 2;
	tankPos = new float2[totalTanks];
	tankTrackPos = new float2[totalTanks];
	tankFrame = new int[totalTanks];
	tankLastTarget = new int[totalTanks];
	tankSprite = new bool[totalTanks];
	tankDir = new float2[totalTanks];
	tankSteer = new float[totalTanks];

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

	int totalBushes = 7500;

	for (int i = 0; i < 3; i++)
	{
		bushCount[i] = 7500 / 3;

		bushPos[i] = new float2[bushCount[i]];
		bushFrame[i] = new int[bushCount[i]];
		bushLastTarget[i] = new int[bushCount[i]];
	}
	// add sandstorm
	for( int i = 0; i < totalBushes; i++ )
	{
		int x = RandomUInt() % map.bitmap->width;
		int y = RandomUInt() % map.bitmap->height;
		int d = (RandomUInt() & 15) - 8;
		sand.push_back( new Particle( bush[i % 3], make_int2( x, y ), map.bitmap->pixels[x + y * map.bitmap->width], d , i/3, i%3));
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
	
	// Tanks
	{
		spriteBuffer = new Buffer(tank1_size + tank2_size, 0, tank_sprites);

		tankPosBuffer = new Buffer(totalTanks * 2, 0, tankPos);
		tankLastPosBuffer = new Buffer(totalTanks * 2);
		mapZBuffer = new Buffer(map.width * map.height);
		tankBackupZBuffer = new Buffer(totalTanks * sqr(tank1->frameSize + 1));
		tankBackUpBuffer = new Buffer(totalTanks * sqr(tank1->frameSize + 1));
		tankLastTargetBuffer = new Buffer(totalTanks, 0, tankLastTarget);

		tankFrameBuffer = new Buffer(totalTanks, 0, tankFrame);

		tankSpriteBuffer = new Buffer(totalTanks / 4, CL_MEM_READ_ONLY, tankSprite);

		tankSteerBuffer = new Buffer(totalTanks, 0, tankSteer);
		tankDirBuffer = new Buffer(totalTanks * 2, 0, tankDir);
		tankTrackPosBuffer = new Buffer(totalTanks * 2, 0, tankTrackPos);

		render_kernel->SetArgument(0, deviceBuffer);
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
		backup_kernel->SetArgument(2, tankLastTargetBuffer);
		backup_kernel->SetArgument(3, tankLastPosBuffer);
		backup_kernel->SetArgument(4, tank1->frameSize);
		backup_kernel->SetArgument(5, mapZBuffer);
		backup_kernel->SetArgument(6, tankBackupZBuffer);

		remove_kernel = new Kernel("render.cl", "remove");
		remove_kernel->SetArgument(0, deviceBuffer);
		remove_kernel->SetArgument(1, tankBackUpBuffer);
		remove_kernel->SetArgument(2, tankLastPosBuffer);
		remove_kernel->SetArgument(3, tankLastTargetBuffer);
		remove_kernel->SetArgument(4, tank1->frameSize);
		remove_kernel->SetArgument(5, mapZBuffer);
		remove_kernel->SetArgument(6, tankBackupZBuffer);

		track_kernel = new Kernel("render.cl", "trackrender");
		track_kernel->SetArgument(0, deviceBuffer);
		track_kernel->SetArgument(1, tankTrackPosBuffer);
		track_kernel->SetArgument(2, tankSteerBuffer);
		track_kernel->SetArgument(3, tankDirBuffer);
		track_kernel->SetArgument(4, tankLastTargetBuffer);


	}
	// bushes
	for(int i = 0; i < 3; i++)
	{ 
		bushRender_kernel[i] = new Kernel("render.cl", "bushrender");

		bushSpriteBuffer[i] = new Buffer(bush_size[i] , 0, bush[i]->pixels);

		bushPosBuffer[i] = new Buffer(bushCount[i] * 2, 0, bushPos[i]);
		bushLastPosBuffer[i] = new Buffer(bushCount[i] * 2);
		bushBackUpBuffer[i] = new Buffer(bushCount[i] * sqr(bush[i]->frameSize + 1));
		bushBackUpZBuffer[i] = new Buffer(bushCount[i] * sqr(bush[i]->frameSize + 1));
		bushLastTargetBuffer[i] = new Buffer(bushCount[i], 0, bushLastTarget[i]);

		bushFrameBuffer[i] = new Buffer(bushCount[i], 0, bushFrame[i]);

		bushRender_kernel[i]->SetArgument(0, deviceBuffer);
		bushRender_kernel[i]->SetArgument(1, bushSpriteBuffer[i]);
		bushRender_kernel[i]->SetArgument(2, bushPosBuffer[i]);
		bushRender_kernel[i]->SetArgument(3, bushFrameBuffer[i]);
		bushRender_kernel[i]->SetArgument(4, bush[i]->frameSize);
		bushRender_kernel[i]->SetArgument(5, bush[i]->frameCount);


		//deviceBuffer->CopyToDevice(true);
		bushSpriteBuffer[i]->CopyToDevice(true);

		bushSaveLastPos_kernel[i] = new Kernel("render.cl", "saveLastPos");
		bushSaveLastPos_kernel[i]->SetArgument(0, bushPosBuffer[i]);
		bushSaveLastPos_kernel[i]->SetArgument(1, bushLastPosBuffer[i]);
		bushSaveLastPos_kernel[i]->SetArgument(2, bushLastTargetBuffer[i]);
		bushSaveLastPos_kernel[i]->SetArgument(3, bush[i]->frameSize);

		bushBackup_kernel[i] = new Kernel("render.cl", "backup");
		bushBackup_kernel[i]->SetArgument(0, deviceBuffer);
		bushBackup_kernel[i]->SetArgument(1, bushBackUpBuffer[i]);
		bushBackup_kernel[i]->SetArgument(2, bushLastTargetBuffer[i]);
		bushBackup_kernel[i]->SetArgument(3, bushLastPosBuffer[i]);
		bushBackup_kernel[i]->SetArgument(4, bush[i]->frameSize);
		bushBackup_kernel[i]->SetArgument(5, mapZBuffer);
		bushBackup_kernel[i]->SetArgument(6, bushBackUpZBuffer[i]);

		bushRemove_kernel[i] = new Kernel("render.cl", "remove");
		bushRemove_kernel[i]->SetArgument(0, deviceBuffer);
		bushRemove_kernel[i]->SetArgument(1, bushBackUpBuffer[i]);
		bushRemove_kernel[i]->SetArgument(2, bushLastPosBuffer[i]);
		bushRemove_kernel[i]->SetArgument(3, bushLastTargetBuffer[i]);
		bushRemove_kernel[i]->SetArgument(4, bush[i]->frameSize);
		bushRemove_kernel[i]->SetArgument(5, mapZBuffer);
		bushRemove_kernel[i]->SetArgument(6, bushBackUpZBuffer[i]);
	}
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

	map.Draw( screen );
	// rebuild actor grid
	grid.Clear();
	grid.Populate( actorPool );
	// update and render actors
	pointer->Remove();
	// Remove Particles
	for (int i = 0; i < 3; i++)
	{
		int frameSize = bush[i]->frameSize;
		bushRemove_kernel[i]->Run2D(int2(frameSize * frameSize, bushCount[i]), int2(frameSize, 1));
	}
	// Remove Tanks
	{
		remove_kernel->Run2D(int2(36 * 36, totalTanks), int2(36, 1));
	}


	//for (int s = (int)sand.size(), i = s - 1; i >= 0; i--) sand[i]->Remove();
	//for (int s = (int)actorPool.size(), i = s - 1; i >= 0; i--)
	{
	//	if (!actorPool[i]->GetType() == Actor::TANK)
	//		actorPool[i]->Remove();
	}

	//for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Draw();
	//for (int s = (int)actorPool.size(), i = 0; i < s; i++)
	//{
	//	if(!actorPool[i]->GetType() == Actor::TANK)
	//		actorPool[i]->Draw();
	//}
	//deviceBuffer->CopyToDevice(true);



	// Perform Ticks
	for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Tick();
	for (int i = 0; i < (int)actorPool.size(); i++)
	{

		if (!actorPool[i]->Tick())
		{
			// actor got deleted, replace by last in list
			Actor* lastActor = actorPool.back();
			Actor* toDelete = actorPool[i];
			actorPool.pop_back();
			if (lastActor != toDelete) actorPool[i] = lastActor;
			if (actorPool[i]->GetType() == Actor::TANK)
			{
				//tankLastTarget[toDelete->id] = 0;
				//tankLastTargetBuffer->CopyToDevice(true);
			}
			delete toDelete;
			i--;
		}
	}


	for (int i = 0; i < 3; i++)
	{
		bushPosBuffer[i]->CopyToDevice(true);
		bushFrameBuffer[i]->CopyToDevice(true);

	}
	tankPosBuffer->CopyToDevice(true);
	tankFrameBuffer->CopyToDevice(true);
	tankDirBuffer->CopyToDevice(true);
	tankSteerBuffer->CopyToDevice(true);
	tankTrackPosBuffer->CopyToDevice(true);



	coolDown++;

	// Draw bushes
	for (int i = 0; i < 3; i++)
	{
		int frameSize = bush[i]->frameSize;
		bushRemove_kernel[i]->Run2D(int2(frameSize * frameSize, bushCount[i]), int2(frameSize, 1));

		bushSaveLastPos_kernel[i]->Run(bushCount[i]);
		bushBackup_kernel[i]->Run2D(int2(frameSize * frameSize, bushCount[i]), int2(frameSize, 1));

		bushRender_kernel[i]->Run2D(int2((frameSize - 1) * (frameSize - 1), bushCount[i]), int2((frameSize - 1), 1));
	}
	// Draw Tanks
	{
		track_kernel->Run(totalTanks);
		saveLastPos_kernel->Run(totalTanks);
		backup_kernel->Run2D(int2(36 * 36, totalTanks), int2(36, 1));

		render_kernel->Run2D(int2(35 * 35, totalTanks), int2(35, 1));
	}

	deviceBuffer->CopyFromDevice();
	// draw the map
	for (int s = (int)actorPool.size(), i = s - 1; i >= 0; i--)
	{
		if (!actorPool[i] && actorPool[i]->GetType() != Actor::TANK)
			actorPool[i]->Remove();
	}
	for (int s = (int)actorPool.size(), i = 0; i < s; i++)
	{
		if (actorPool[i]->GetType() != Actor::TANK)
			actorPool[i]->Draw();
	}

	int2 cursorPos = map.ScreenToMap( mousePos );
	pointer->Draw( map.bitmap, make_float2( cursorPos ), 0 );
	// handle mouse
	HandleInput();
	// report frame time
	static float frameTimeAvg = 10.0f; // estimate
	frameTimeAvg = 0.95f * frameTimeAvg + 0.05f * t.elapsed() * 1000;
	printf( "frame time: %5.2fms\n", frameTimeAvg );
	
	
}