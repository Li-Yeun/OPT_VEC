#include "precomp.h"

TheApp* CreateApp() { return new MyApp(); }

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
int totalTanks;
void MyApp::Init()
{	
	Kernel::InitCL();
	// initialize map view
	map.UpdateView(screen, zoom);

	//render_kernel = new Kernel("render.cl", "render");
	//Kernel* tank1_kernel = new Kernel("render.cl", "render");
	int group1 = 16, group2 = 12, group3 = 8, particle_count = 7500;
	totalTanks = (group1 * group1 + group2 * group2 + group3 * group3);
	tankPos = new float2 * [2];
	tankPos[0] = new float2[totalTanks];
	tankPos[1] = new float2[totalTanks];
	tankFrame = new int* [2];
	tankFrame[0] = new int[totalTanks];
	tankFrame[1] = new int[totalTanks];
	tankPosBuffer = new Buffer * [2];
	tankPosBuffer[0] = new Buffer(totalTanks * 2, 0, tankPos[0]);
	tankPosBuffer[1] = new Buffer(totalTanks * 2, 0, tankPos[1]);
	tankFrameBuffer = new Buffer * [2];
	tankFrameBuffer[0] = new Buffer(totalTanks, 0, tankFrame[0]);
	tankFrameBuffer[1] = new Buffer(totalTanks, 0, tankFrame[1]);

	bushPos = new float2*[3];
	bushPos[0] = new float2[particle_count / 3];
	bushPos[1] = new float2[particle_count / 3];
	bushPos[2] = new float2[particle_count / 3];
	bushFrame = new int*[3];
	bushFrame[0] = new int[particle_count / 3];
	bushFrame[1] = new int[particle_count / 3];
	bushFrame[2] = new int[particle_count / 3];
	bushPosBuffer = new Buffer * [3];
	bushPosBuffer[0] = new Buffer(2 * particle_count / 3, 0, bushPos[0]);
	bushPosBuffer[1] = new Buffer(2 * particle_count / 3, 0, bushPos[1]);
	bushPosBuffer[2] = new Buffer(2 * particle_count / 3, 0, bushPos[2]);
	bushFrameBuffer = new Buffer *[3];
	bushFrameBuffer[0] = new Buffer(particle_count / 3, 0, bushFrame[0]);
	bushFrameBuffer[1] = new Buffer(particle_count / 3, 0, bushFrame[1]);
	bushFrameBuffer[2] = new Buffer(particle_count / 3, 0, bushFrame[2]);

	deviceBuffer = new Buffer(map.width * map.height, Buffer::DEFAULT, Map::bitmap->pixels);

	// load tank sprites
	tank1 = new Sprite( "assets/tanks.png", make_int2( 128, 100 ), make_int2( 310, 360 ), 36, 256 );
	tank1->SetKernel(tankPosBuffer[0], tankFrameBuffer[0]);
	tank2 = new Sprite( "assets/tanks.png", make_int2( 327, 99 ), make_int2( 515, 349 ), 36, 256 );
	tank2->SetKernel(tankPosBuffer[1], tankFrameBuffer[1]);
	// load bush sprite for dust streams
	bush[0] = new Sprite( "assets/bush1.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 10, 256 );
	bush[1] = new Sprite( "assets/bush2.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 14, 256 );
	bush[2] = new Sprite( "assets/bush3.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 20, 256 );
	bush[0]->ScaleAlpha( 96 );
	bush[1]->ScaleAlpha( 64 );
	bush[2]->ScaleAlpha( 128 );
	bush[0]->SetKernel(bushPosBuffer[0], bushFrameBuffer[0]);
	bush[1]->SetKernel(bushPosBuffer[1], bushFrameBuffer[1]);
	bush[2]->SetKernel(bushPosBuffer[2], bushFrameBuffer[2]);
	cout << "SPRITES LOADED..." << endl;
	// pointer
	pointer = new SpriteInstance( new Sprite( "assets/pointer.png" ) );
	// create armies
	int id1 = 0, id2 = 0;
	tankLastTarget = new int* [2];
	tankLastTarget[0] = new int[totalTanks];
	tankLastTarget[1] = new int[totalTanks];


	for (int y = 0; y < group1; y++) for (int x = 0; x < group1; x++) // main groups
	{
		Tank* army1Tank = new Tank( tank1, make_int2( 520 + x * 32, 2420 - y * 32 ), make_int2( 5000, -500 ), 0, 0, id1++ );
		Tank* army2Tank = new Tank( tank2, make_int2( 3300 - x * 32, y * 32 + 700 ), make_int2( -1000, 4000 ), 10, 1, id2++ );
		tankPool.push_back(army1Tank);
		tankPool.push_back(army2Tank);

	}
	for (int y = 0; y < group2; y++) for (int x = 0; x < group2; x++) // backup
	{
		Tank* army1Tank = new Tank( tank1, make_int2( 40 + x * 32, 2620 - y * 32 ), make_int2( 5000, -500 ), 0, 0, id1++ );
		Tank* army2Tank = new Tank( tank2, make_int2( 3900 - x * 32, y * 32 + 300 ), make_int2( -1000, 4000 ), 10, 1, id2++);
		tankPool.push_back(army1Tank);
		tankPool.push_back(army2Tank);

	}
	for (int y = 0; y < group3; y++) for (int x = 0; x < group3; x++) // small forward groups
	{
		Tank* army1Tank = new Tank( tank1, make_int2( 1440 + x * 32, 2220 - y * 32 ), make_int2( 3500, -500 ), 0, 0, id1++);
		Tank* army2Tank = new Tank( tank2, make_int2( 2400 - x * 32, y * 32 + 900 ), make_int2( 1300, 4000 ), 128, 1, id2++ );
		tankPool.push_back(army1Tank);
		tankPool.push_back(army2Tank);
	}
	cout << "SANITY CHECK" << endl;
	cout << "ARMY 1 TANKS: " << id1 << " == " << totalTanks << endl;
	cout << "ARMY 2 TANKS: " << id2 << " == " << totalTanks << endl;
	// load mountain peaks
	Surface mountains( "assets/peaks.png" );
	for (int y = 0; y < mountains.height; y++) for (int x = 0; x < mountains.width; x++)
	{
		uint p = mountains.pixels[x + y * mountains.width];
		if ((p & 0xffff) == 0) peaks.push_back( make_float3( make_int3( x * 8, y * 8, (p >> 16) & 255 ) ) );
	}
	// add sandstorm
	int id = 0;
	for( int i = 0; i < particle_count; i++, id++)
	{
		int x = RandomUInt() % map.bitmap->width;
		int y = RandomUInt() % map.bitmap->height;
		int d = (RandomUInt() & 15) - 8;
		sand.push_back( new Particle( bush[i % 3], make_int2( x, y ), map.bitmap->pixels[x + y * map.bitmap->width], d , id / 3, i % 3));
	}
	// place flags
	Surface* flagPattern = new Surface( "assets/flag.png" );
	VerletFlag* flag1 = new VerletFlag( make_int2( 3000, 848 ), flagPattern );
	actorPool.push_back( flag1 );
	VerletFlag* flag2 = new VerletFlag( make_int2( 1076, 1870 ), flagPattern );
	actorPool.push_back( flag2 );
	
	
	cout << "ACTORS SPAWNED..." << endl;
	
	//spriteBuffer = new Buffer(tank1->frameSize * tank1->frameSize * tank1->frameCount, 0, tank1->pixels);

	tankLastPosBuffer = new Buffer * [2];
	tankLastPosBuffer[0] = new Buffer(totalTanks * 2);
	tankLastPosBuffer[1] = new Buffer(totalTanks * 2);
	tankBackUpBuffer = new Buffer * [2];
	tankBackUpBuffer[0] = new Buffer(totalTanks * sqr(tank1->frameSize + 1));
	tankBackUpBuffer[1] = new Buffer(totalTanks * sqr(tank2->frameSize + 1));
	tankLastTargetBuffer = new Buffer * [2];
	tankLastTargetBuffer[0] = new Buffer(totalTanks, 0, tankLastTarget[0]);
	tankLastTargetBuffer[1] = new Buffer(totalTanks, 0, tankLastTarget[1]);

	

	/*render_kernel->SetArgument(0,deviceBuffer);
	render_kernel->SetArgument(1, spriteBuffer);
	render_kernel->SetArgument(2, tankPosBuffer);
	render_kernel->SetArgument(3, tankFrameBuffer);*/
	cout << "BUFFERS INITIALIZED..." << endl;

	//spriteBuffer->CopyToDevice(true);


	saveLastPos_kernel = new Kernel * [2];
	saveLastPos_kernel[0] = new Kernel("render.cl", "saveLastPos");
	saveLastPos_kernel[0]->SetArgument(0, tankPosBuffer[0]);
	saveLastPos_kernel[0]->SetArgument(1, tankLastPosBuffer[0]);
	saveLastPos_kernel[0]->SetArgument(2, tankLastTargetBuffer[0]);
	saveLastPos_kernel[0]->SetArgument(3, tank1->frameSize);
	cout << "saveLastPos KERNEL 0 SET..." << endl;
	saveLastPos_kernel[1] = new Kernel("render.cl", "saveLastPos");
	saveLastPos_kernel[1]->SetArgument(0, tankPosBuffer[1]);
	saveLastPos_kernel[1]->SetArgument(1, tankLastPosBuffer[1]);
	saveLastPos_kernel[1]->SetArgument(2, tankLastTargetBuffer[1]);
	saveLastPos_kernel[1]->SetArgument(3, tank2->frameSize);
	cout << "saveLastPos KERNEL 1 SET..." << endl;

	backup_kernel = new Kernel * [2];
	backup_kernel[0] = new Kernel("render.cl", "backup");
	backup_kernel[0]->SetArgument(0, deviceBuffer);
	backup_kernel[0]->SetArgument(1, tankBackUpBuffer[0]);
	backup_kernel[0]->SetArgument(2, tankLastPosBuffer[0]);
	backup_kernel[0]->SetArgument(3, tank1->frameSize);
	cout << "backup KERNEL 0 SET..." << endl;
	backup_kernel[1] = new Kernel("render.cl", "backup");
	backup_kernel[1]->SetArgument(0, deviceBuffer);
	backup_kernel[1]->SetArgument(1, tankBackUpBuffer[1]);
	backup_kernel[1]->SetArgument(2, tankLastPosBuffer[1]);
	backup_kernel[1]->SetArgument(3, tank2->frameSize);
	cout << "backup KERNEL 1 SET..." << endl;

	remove_kernel = new Kernel * [2];
	remove_kernel[0] = new Kernel("render.cl", "remove");
	remove_kernel[0]->SetArgument(0, deviceBuffer);
	remove_kernel[0]->SetArgument(1, tankBackUpBuffer[0]);
	remove_kernel[0]->SetArgument(2, tankLastPosBuffer[0]);
	remove_kernel[0]->SetArgument(3, tankLastTargetBuffer[0]);
	remove_kernel[0]->SetArgument(4, tank1->frameSize);
	cout << "remove KERNEL 0 SET..." << endl;
	remove_kernel[1] = new Kernel("render.cl", "remove");
	remove_kernel[1]->SetArgument(0, deviceBuffer);
	remove_kernel[1]->SetArgument(1, tankBackUpBuffer[1]);
	remove_kernel[1]->SetArgument(2, tankLastPosBuffer[1]);
	remove_kernel[1]->SetArgument(3, tankLastTargetBuffer[1]);
	remove_kernel[1]->SetArgument(4, tank2->frameSize);
	cout << "remove KERNEL 1 SET..." << endl;
	cout << "KERNELS SET..." << endl;


	tank1->sprite_buffer->CopyToDevice(true);
	tank2->sprite_buffer->CopyToDevice(true);
	cout << "SPRITE BUFFERS COPIED TO DEVICE..." << endl;
	deviceBuffer->CopyToDevice();
	cout << "MAP BUFFER COPIED TO DEVICE..." << endl;
	cout << "INIT FINISHED!" << endl;


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
	// draw the map
	map.Draw( screen );
	// rebuild actor grid
	grid.Clear();
	grid.Populate( actorPool );
	grid.Populate(tankPool);
	// update and render actors
	pointer->Remove();
	for (int s = (int)sand.size(), i = s - 1; i >= 0; i--) sand[i]->Remove();
	for (int s = (int)actorPool.size(), i = s - 1; i >= 0; i--) actorPool[i]->Remove();

	//for (int s = (int)tankPool.size(), i = s - 1; i >= 0; i--) tankPool[i]->Remove();

	cout << "CHECK 0" << endl;
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

	for (int i = 0; i < (int)tankPool.size(); i++) if (!tankPool[i]->Tick())
	{
		// actor got deleted, replace by last in list
		Tank* lastActor = tankPool.back();
		Tank* toDelete = tankPool[i];
		tankPool.pop_back();
		if (lastActor != toDelete) tankPool[i] = lastActor;
		delete toDelete;
		i--;
	}
	coolDown++;
	//for (int s = (int)actorPool.size(), i = 0; i < s; i++)
	//{
	//	actorPool[i]->Draw();
	//}
	//for (int s = (int)tankPool.size(), i = 0; i < s; i++)
	//{
	//	tankPool[i]->Draw();
	//}
	cout << "CHECK 1" << endl;
	tankPosBuffer[0]->CopyToDevice(true);
	tankPosBuffer[1]->CopyToDevice(true);
	cout << "CHECK 2" << endl;
	tankFrameBuffer[0]->CopyToDevice(true);
	tankFrameBuffer[1]->CopyToDevice(true);
	cout << "CHECK 3" << endl;
	remove_kernel[0]->Run2D(int2(36 * 36, totalTanks), int2(36, 1));
	remove_kernel[1]->Run2D(int2(36 * 36, totalTanks), int2(36, 1));
	cout << "CHECK 4" << endl;

	saveLastPos_kernel[0]->Run(totalTanks);
	saveLastPos_kernel[1]->Run(totalTanks);
	cout << "CHECK 5" << endl;

	backup_kernel[0]->Run2D(int2(36 * 36, totalTanks), int2(36, 1));
	backup_kernel[1]->Run2D(int2(36 * 36, totalTanks), int2(36, 1));
	cout << "CHECK 6" << endl;

	tank1->sprite_kernel->Run2D(int2(35 * 35, totalTanks), int2(35, 1));
	tank2->sprite_kernel->Run2D(int2(35 * 35, totalTanks), int2(35, 1));
	cout << "CHECK 7" << endl;
	bush[0]->sprite_kernel->Run2D(int2((bush[0]->frameSize - 1) * (bush[0]->frameSize - 1), 2500), int2(bush[0]->frameSize - 1, 1));
	bush[1]->sprite_kernel->Run2D(int2((bush[1]->frameSize - 1) * (bush[1]->frameSize - 1), 2500), int2(bush[1]->frameSize - 1, 1));
	bush[2]->sprite_kernel->Run2D(int2((bush[2]->frameSize - 1) * (bush[2]->frameSize - 1), 2500), int2(bush[2]->frameSize - 1, 1));
	cout << "CHECK 8" << endl;
	//for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Draw();

	deviceBuffer->CopyFromDevice(true);
	cout << "CHECK 9" << endl;

	clFinish(Kernel::GetQueue());
	cout << "CHECK 10" << endl;
	int2 cursorPos = map.ScreenToMap( mousePos );
	pointer->Draw( map.bitmap, make_float2( cursorPos ), 0 );
	// handle mouse
	HandleInput();
	// report frame time
	static float frameTimeAvg = 10.0f; // estimate
	frameTimeAvg = 0.95f * frameTimeAvg + 0.05f * t.elapsed() * 1000;
	printf( "frame time: %5.2fms\n", frameTimeAvg );
}