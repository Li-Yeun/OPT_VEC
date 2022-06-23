#include "precomp.h"
#include "myapp.h"

TheApp* CreateApp() { return new MyApp(); }

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
int totalTanks, tankCounter;
int totalBushes, max_bush_frameSize;
int totalFlags, flagPosOffset, flagBackupOffset;
Timer t;
void MyApp::Init()
{
	//std::cout << sizeof(int2) << std::endl;
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
	bush[0] = new Sprite("assets/bush1.png", make_int2(2, 2), make_int2(31, 31), 10, 256);
	bush[1] = new Sprite("assets/bush2.png", make_int2(2, 2), make_int2(31, 31), 14, 256);
	bush[2] = new Sprite("assets/bush3.png", make_int2(2, 2), make_int2(31, 31), 20, 256);
	bush[0]->ScaleAlpha(96);
	bush[1]->ScaleAlpha(64);
	bush[2]->ScaleAlpha(128);

	uint bush1_sprite_size = bush[0]->frameSize * bush[0]->frameSize * bush[0]->frameCount;
	uint bush2_sprite_size = bush[1]->frameSize * bush[1]->frameSize * bush[1]->frameCount;
	uint bush3_sprite_size = bush[2]->frameSize * bush[2]->frameSize * bush[2]->frameCount;

	uint* bush_sprites = new uint[bush1_sprite_size + bush2_sprite_size + bush3_sprite_size];
	std::copy(bush[0]->pixels, bush[0]->pixels + bush1_sprite_size, bush_sprites);
	std::copy(bush[1]->pixels, bush[1]->pixels + bush2_sprite_size, bush_sprites + bush1_sprite_size);
	std::copy(bush[2]->pixels, bush[2]->pixels + bush3_sprite_size, bush_sprites + bush1_sprite_size + bush2_sprite_size);

	bushSpriteOffset = new uint[3];
	bushSpriteOffset[0] = 0;
	bushSpriteOffset[1] = bush1_sprite_size;
	bushSpriteOffset[2] = bush1_sprite_size + bush2_sprite_size;

	bushFrameSize = new uint[3];
	bushFrameSize[0] = bush[0]->frameSize;
	bushFrameSize[1] = bush[1]->frameSize;
	bushFrameSize[2] = bush[2]->frameSize;
	max_bush_frameSize = max(max(bushFrameSize[0], bushFrameSize[1]), bushFrameSize[2]);

	// pointer
	pointer = new SpriteInstance(new Sprite("assets/pointer.png"));
	// create armies
	for (int y = 0; y < group1; y++) for (int x = 0; x < group1; x++) // main groups
	{
		Actor* army1Tank = new Tank(tank1, make_int2(520 + x * 32, 2420 - y * 32), make_int2(5000, -500), 0, 0, tankCounter++);
		Actor* army2Tank = new Tank(tank2, make_int2(3300 - x * 32, y * 32 + 700), make_int2(-1000, 4000), 10, 1, tankCounter++);
		actorPool.push_back(army1Tank);
		actorPool.push_back(army2Tank);

	}
	for (int y = 0; y < group2; y++) for (int x = 0; x < group2; x++) // backup
	{
		Actor* army1Tank = new Tank(tank1, make_int2(40 + x * 32, 2620 - y * 32), make_int2(5000, -500), 0, 0, tankCounter++);
		Actor* army2Tank = new Tank(tank2, make_int2(3900 - x * 32, y * 32 + 300), make_int2(-1000, 4000), 10, 1, tankCounter++);
		actorPool.push_back(army1Tank);
		actorPool.push_back(army2Tank);
	}
	for (int y = 0; y < group3; y++) for (int x = 0; x < group3; x++) // small forward groups
	{
		Actor* army1Tank = new Tank(tank1, make_int2(1440 + x * 32, 2220 - y * 32), make_int2(3500, -500), 0, 0, tankCounter++);
		Actor* army2Tank = new Tank(tank2, make_int2(2400 - x * 32, y * 32 + 900), make_int2(1300, 4000), 128, 1, tankCounter++);
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
	

	//add sandstorm
	totalBushes = 7500;
	bushCount = new uint[3];
	bushCount[0] = 0;
	bushCount[1] = 0;
	bushCount[2] = 0;

	for (int i = 0; i < totalBushes; i += 4)
	{
		uint t[4] = { (4 * i) % 3, (4 * i + 1) % 3 , (4 * i + 2) % 3, (4 * i + 3) % 3 };
		bushCount[t[0]]++;
		bushCount[t[1]]++;
		bushCount[t[2]]++;
		bushCount[t[3]]++;
	}

	bushTypeIndex[0] = new uint[bushCount[0]];
	bushTypeIndex[1] = new uint[bushCount[1]];
	bushTypeIndex[2] = new uint[bushCount[2]];

	bushCounter = new uint[3];
	bushCounter[0] = 0;
	bushCounter[1] = 0;
	bushCounter[2] = 0;

	MyApp::bushType = new uint[totalBushes];
	MyApp::bushPos = new float2[totalBushes];
	MyApp::bushLastPos = new int2[totalBushes];
	MyApp::bushFrame = new int[totalBushes];
	MyApp::bushLastTarget = new bool[totalBushes];

	for (int i = 0; i < 7500; i += 4)
	{
		int x[4] = { RandomUInt() % map.bitmap->width, RandomUInt() % map.bitmap->width, RandomUInt() % map.bitmap->width, RandomUInt() % map.bitmap->width };
		int y[4] = { RandomUInt() % map.bitmap->height, RandomUInt() % map.bitmap->height, RandomUInt() % map.bitmap->height, RandomUInt() % map.bitmap->height };
		uint d[4] = { (RandomUInt() & 15) - 8, (RandomUInt() & 15) - 8, (RandomUInt() & 15) - 8, (RandomUInt() & 15) - 8 };
		uint t[4] = { (4 * i) % 3, (4 * i + 1) % 3 , (4 * i + 2) % 3, (4 * i + 3) % 3 };
		Sprite* s[4] = { bush[t[0]], bush[t[1]], bush[t[2]], bush[t[3]] };
		float2 p[4] = { make_float2(x[0], y[0]), make_float2(x[1], y[1]), make_float2(x[2], y[2]), make_float2(x[3], y[3]) };
		uint c[4] = { map.bitmap->pixels[x[0] + y[0] * map.bitmap->width], map.bitmap->pixels[x[1] + y[1] * map.bitmap->width], map.bitmap->pixels[x[2] + y[2] * map.bitmap->width], map.bitmap->pixels[x[3] + y[3] * map.bitmap->width] };
		sand.push_back(new Particle(s, p, c, d, i, t));
	}


	totalFlags = 2;
	uint flagCounter = 0;

	// place flags
	Surface* flagPattern = new Surface("assets/flag.png");

	flagPosOffset = flagPattern->width * flagPattern->height;
	flagBackupOffset = flagPosOffset * 4;

	uint* flagSprite = new uint[flagPosOffset];
	std::copy(flagPattern->pixels, flagPattern->pixels + flagPosOffset, flagSprite);

	flagPos = new float2[flagPattern->width * flagPattern->height * totalFlags];
	flagHasBackup = new bool[totalFlags];

	VerletFlag* flag1 = new VerletFlag(make_int2(3000, 848), flagPattern, flagCounter++);
	actorPool.push_back(flag1);
	VerletFlag* flag2 = new VerletFlag(make_int2(1076, 1870), flagPattern, flagCounter++);
	actorPool.push_back(flag2);


	bulletPos = new float2[maxBullets]{ float2{-1,-1} };
	bulletLastPos = new int2[maxBullets]{ int2{-1,-1} };
	bulletFrame = new int[maxBullets] {-1};
	bulletFrameCounter = new int[maxBullets] { -1 };
	bulletLastTarget = new bool[maxBullets] {false};

	flashSprite = new Sprite("assets/flash.png");
	bulletSprite = new Sprite("assets/bullet.png", make_int2(2, 2), make_int2(31, 31), 32, 256);
	int flash_sprite_size = flashSprite->frameSize * flashSprite->frameSize * flashSprite->frameCount;
	int bullet_sprite_size = bulletSprite->frameSize * bulletSprite->frameSize * bulletSprite->frameCount;

	uint* bullet_sprites = new uint[flash_sprite_size + bullet_sprite_size];
	std::copy(flashSprite->pixels, flashSprite->pixels + flash_sprite_size, bullet_sprites);
	std::copy(bulletSprite->pixels, bulletSprite->pixels + bullet_sprite_size, bullet_sprites + flash_sprite_size);


	// Sprite Explosion
	spriteExplosionSprite = new Sprite("assets/explosion1.png", 16);
	int spriteExplosion_sprite_size = spriteExplosionSprite->frameSize * spriteExplosionSprite->frameSize * spriteExplosionSprite->frameCount;

	uint* spriteExplosion_sprites = new uint[spriteExplosion_sprite_size];
	std::copy(spriteExplosionSprite->pixels, spriteExplosionSprite->pixels + spriteExplosion_sprite_size, spriteExplosion_sprites);

	spriteExplosionPos = new int2[maxSpriteExplosion]{ int2{-1,-1} };
	spriteExplosionLastPos = new int2[maxSpriteExplosion]{ int2{-1,-1} };
	spriteExplosionFrame = new int[maxSpriteExplosion] {-1};
	spriteExplosionLastTarget = new bool[maxSpriteExplosion] {false};


	// Particle Explosion
	particleMaxTotalPos = 0;

	uint size = tank1->frameSize;
	uint stride = tank1->frameSize * tank1->frameCount;

	// calculate max backup size for particle Explosion
	for (int u = 0; u < tank1->frameCount; u++)
	{
		uint* src = tank1->pixels + u * size;
		uint* src2 = tank2->pixels + u * size;
		int temp = 0;
		int temp2 = 0;
		for (uint y = 0; y < size; y++) for (uint x = 0; x < size; x++)
		{
			uint pixel = src[x + y * stride];
			uint alpha = pixel >> 24;
			if (alpha > 64) {temp+=2; }
			pixel = src2[x + y * stride];
			alpha = pixel >> 24;
			if (alpha > 64) { temp2 += 2; }
		}
		if (temp > particleMaxTotalPos)
		{
			particleMaxTotalPos = temp;
		}
		if (temp2 > particleMaxTotalPos)
		{
			particleMaxTotalPos = temp2;
		}
	}
	particleExplosionMaxPos = new uint[maxParticleExplosion] {0};
	particleExplosionFade = new uint[maxParticleExplosion]{ 0 };
	particleExplosionPos = new int2[maxParticleExplosion * particleMaxTotalPos]{ int2{-1,-1} };
	particleExplosionColor = new uint[maxParticleExplosion * particleMaxTotalPos] { 0 };

	// initialize map view
	map.UpdateView(screen, zoom);

	// Initialize Kernels
	tankDrawKernel = new Kernel("Kernels/tank.cl", "Draw");

	pointerDrawKernel = new Kernel("Kernels/pointer.cl", "Draw");
	pointerBackupBuffer = new Buffer(sqr(pointer->sprite->frameSize));
	pointerSpriteBuffer = new Buffer(pointer->sprite->frameSize * pointer->sprite->frameSize, CL_MEM_READ_ONLY, pointer->sprite->pixels); 


	deviceBuffer = new Buffer(map.width * map.height, 0, map.bitmap->pixels);
	screenBuffer = new Buffer(GetRenderTarget()->ID, Buffer::TARGET, 0);
	screen = 0;

	pointerDrawKernel->SetArgument(0, deviceBuffer);
	pointerDrawKernel->SetArgument(1, pointerSpriteBuffer);
	pointerDrawKernel->SetArgument(2, make_int2(0, 0));
	pointerDrawKernel->SetArgument(3, pointer->sprite->frameSize);
	pointerDrawKernel->SetArgument(4, pointerBackupBuffer);

	pointerRemoveKernel = new Kernel("Kernels/pointer.cl", "Remove");
	pointer->lastPos = make_int2(-20, -20);

	pointerRemoveKernel->SetArgument(0, deviceBuffer);
	pointerRemoveKernel->SetArgument(1, pointerBackupBuffer);
	pointerRemoveKernel->SetArgument(2, pointer->lastPos);
	pointerRemoveKernel->SetArgument(3, pointerLastTarget);
	pointerRemoveKernel->SetArgument(4, pointer->sprite->frameSize);

	screenKernel = new Kernel("Kernels/screen.cl", "renderToScreen");
	screenKernel->SetArgument(0, deviceBuffer);
	screenKernel->SetArgument(1, screenBuffer);
	screenKernel->SetArgument(2, map.view);
	screenKernel->SetArgument(3, map.dxy);

	tankSpriteBuffer = new Buffer(tank1_sprite_size + tank2_sprite_size, CL_MEM_READ_ONLY, tank_sprites);
	tankTeamBuffer = new Buffer(totalTanks / 4, CL_MEM_READ_ONLY, tankTeam);
	tankPosBuffer = new Buffer(totalTanks * 2, CL_MEM_READ_ONLY, tankPos);
	tankFrameBuffer = new Buffer(totalTanks, CL_MEM_READ_ONLY, tankFrame);

	tankDrawKernel->SetArgument(0, deviceBuffer);
	tankDrawKernel->SetArgument(1, tankSpriteBuffer);
	tankDrawKernel->SetArgument(2, tankTeamBuffer);
	tankDrawKernel->SetArgument(3, tankPosBuffer);
	tankDrawKernel->SetArgument(4, tankFrameBuffer);
	tankDrawKernel->SetArgument(5, tank1->frameSize);
	tankDrawKernel->SetArgument(6, tank1->frameCount);


	deviceBuffer->CopyToDevice(true);
	tankSpriteBuffer->CopyToDevice(true);
	pointerSpriteBuffer->CopyToDevice(true);
	tankTeamBuffer->CopyToDevice(true);
	tankPosBuffer->CopyToDevice(true);
	tankFrameBuffer->CopyToDevice(true);


	tankBackupKernel = new Kernel("Kernels/tank.cl", "Backup");

	tankBackupBuffer = new Buffer(totalTanks * sqr(tank1->frameSize + 1));
	tankLastTargetBuffer = new Buffer(totalTanks / 4, 0, tankLastTarget);

	tankBackupKernel->SetArgument(0, deviceBuffer);
	tankBackupKernel->SetArgument(1, tankPosBuffer);
	tankBackupKernel->SetArgument(2, tankBackupBuffer);
	tankBackupKernel->SetArgument(3, tankLastTargetBuffer);
	tankBackupKernel->SetArgument(4, tank1->frameSize);

	tankLastTargetBuffer->CopyToDevice(true);


	tankSaveLastPosKernel = new Kernel("Kernels/tank.cl", "SaveLastPos");

	tankLastPosBuffer = new Buffer(totalTanks * 2, 0, tankLastPos);

	tankSaveLastPosKernel->SetArgument(0, tankPosBuffer);
	tankSaveLastPosKernel->SetArgument(1, tankLastPosBuffer);
	tankSaveLastPosKernel->SetArgument(2, tankLastTargetBuffer);
	tankSaveLastPosKernel->SetArgument(3, tank1->frameSize);

	tankLastPosBuffer->CopyToDevice(true);

	tankRemoveKernel = new Kernel("Kernels/tank.cl", "Remove");
	tankRemoveKernel->SetArgument(0, deviceBuffer);
	tankRemoveKernel->SetArgument(1, tankLastPosBuffer);
	tankRemoveKernel->SetArgument(2, tankBackupBuffer);
	tankRemoveKernel->SetArgument(3, tankLastTargetBuffer);
	tankRemoveKernel->SetArgument(4, tank1->frameSize);

	// Tank tracks
	tankTrackKernel = new Kernel("Kernels/track.cl", "Track");

	tankOldPosBuffer = new Buffer(totalTanks * 2, CL_MEM_READ_ONLY, tankOldPos);
	tankDirBuffer = new Buffer(totalTanks * 2, CL_MEM_READ_ONLY, tankDir);
	tankSteerBuffer = new Buffer(totalTanks, CL_MEM_READ_ONLY, tankSteer);

	tankTrackKernel->SetArgument(0, deviceBuffer);
	tankTrackKernel->SetArgument(1, tankOldPosBuffer);
	tankTrackKernel->SetArgument(2, tankDirBuffer);
	tankTrackKernel->SetArgument(3, tankSteerBuffer);

	tankOldPosBuffer->CopyToDevice(true);
	tankDirBuffer->CopyToDevice(true);
	tankSteerBuffer->CopyToDevice(true);

	// Bushes
	bushDrawKernel = new Kernel("Kernels/bush.cl", "Draw");

	bushSpriteBuffer = new Buffer(bush1_sprite_size + bush2_sprite_size + bush3_sprite_size, 0, bush_sprites);
	bushTypeBuffer = new Buffer(totalBushes, CL_MEM_READ_ONLY, bushType);
	bushPosBuffer = new Buffer(totalBushes, 0, bushPos);
	bushFrameBuffer = new Buffer(totalBushes, 0, bushFrame);
	bushFrameSizeBuffer = new Buffer(3, CL_MEM_READ_ONLY, bushFrameSize);
	bushSpriteOffsetBuffer = new Buffer(3, CL_MEM_READ_ONLY, bushSpriteOffset);


	bushDrawKernel->SetArgument(0, deviceBuffer);
	bushDrawKernel->SetArgument(1, bushSpriteBuffer);
	bushDrawKernel->SetArgument(2, bushTypeBuffer);
	bushDrawKernel->SetArgument(3, bushPosBuffer);
	bushDrawKernel->SetArgument(4, bushFrameBuffer);
	bushDrawKernel->SetArgument(5, bushFrameSizeBuffer);
	bushDrawKernel->SetArgument(6, bushSpriteOffsetBuffer);
	bushDrawKernel->SetArgument(7, 256);

	bushSpriteBuffer->CopyToDevice(true);
	bushTypeBuffer->CopyToDevice(true);
	bushPosBuffer->CopyToDevice(true);
	bushFrameBuffer->CopyToDevice(true);
	bushFrameSizeBuffer->CopyToDevice(true);
	bushSpriteOffsetBuffer->CopyToDevice(true);


	bushLastTargetBuffer = new Buffer(totalBushes / 4, 0, bushLastTarget);
	for (int i = 0; i < 3; i++) {
		bushBackupKernel[i] = new Kernel("Kernels/bush.cl", "Backup");
		bushBackupBuffer[i] = new Buffer(bushCount[i] * sqr(bush[i]->frameSize + 1));
		bushTypeIndexBuffer[i] = new Buffer(bushCount[i], CL_MEM_READ_ONLY, bushTypeIndex[i]);

		bushBackupKernel[i]->SetArgument(0, deviceBuffer);
		bushBackupKernel[i]->SetArgument(1, bushPosBuffer);
		bushBackupKernel[i]->SetArgument(2, bushBackupBuffer[i]);
		bushBackupKernel[i]->SetArgument(3, bushLastTargetBuffer);
		bushBackupKernel[i]->SetArgument(4, bushTypeIndexBuffer[i]);
		bushBackupKernel[i]->SetArgument(5, bush[i]->frameSize);

		bushTypeIndexBuffer[i]->CopyToDevice(true);
	}
	bushLastTargetBuffer->CopyToDevice(true);


	bushSaveLastPosKernel = new Kernel("Kernels/bush.cl", "SaveLastPos");

	bushLastPosBuffer = new Buffer(totalBushes, 0, bushLastPos);

	bushSaveLastPosKernel->SetArgument(0, bushPosBuffer);
	bushSaveLastPosKernel->SetArgument(1, bushLastPosBuffer);
	bushSaveLastPosKernel->SetArgument(2, bushLastTargetBuffer);
	bushSaveLastPosKernel->SetArgument(3, bushTypeBuffer);
	bushSaveLastPosKernel->SetArgument(4, bushFrameSizeBuffer);

	bushLastPosBuffer->CopyToDevice(true);

	for (int i = 0; i < 3; i++) {
		bushRemoveKernel[i] = new Kernel("Kernels/bush.cl", "Remove");
		bushRemoveKernel[i]->SetArgument(0, deviceBuffer);
		bushRemoveKernel[i]->SetArgument(1, bushLastPosBuffer);
		bushRemoveKernel[i]->SetArgument(2, bushBackupBuffer[i]);
		bushRemoveKernel[i]->SetArgument(3, bushLastTargetBuffer);
		bushRemoveKernel[i]->SetArgument(4, bushTypeIndexBuffer[i]);
		bushRemoveKernel[i]->SetArgument(5, bush[i]->frameSize);
	}

	// Flags
	flagDrawKernel = new Kernel("Kernels/flag.cl", "Draw");

	flagSpriteBuffer = new Buffer(flagPosOffset, CL_MEM_READ_ONLY, flagSprite);
	flagPosBuffer = new Buffer(flagPosOffset * totalFlags * 2, CL_MEM_READ_ONLY, flagPos);

	flagDrawKernel->SetArgument(0, deviceBuffer);
	flagDrawKernel->SetArgument(1, flagSpriteBuffer);
	flagDrawKernel->SetArgument(2, flagPosBuffer);
	flagDrawKernel->SetArgument(3, flagPattern->width);
	flagDrawKernel->SetArgument(4, flagPattern->height);
	flagDrawKernel->SetArgument(5, flagPosOffset);

	flagSpriteBuffer->CopyToDevice(true);
	flagPosBuffer->CopyToDevice(true);

	flagBackupKernel = new Kernel("Kernels/flag.cl", "Backup");

	flagBackupBuffer = new Buffer(flagBackupOffset * totalFlags);
	flagHasBackupBuffer = new Buffer(max(totalFlags, 4) / 4, 0, flagHasBackup);

	flagBackupKernel->SetArgument(0, deviceBuffer);
	flagBackupKernel->SetArgument(1, flagPosBuffer);
	flagBackupKernel->SetArgument(2, flagBackupBuffer);
	flagBackupKernel->SetArgument(3, flagHasBackupBuffer);
	flagBackupKernel->SetArgument(4, flagPattern->width);
	flagBackupKernel->SetArgument(5, flagPattern->height);
	flagBackupKernel->SetArgument(6, flagPosOffset);
	flagBackupKernel->SetArgument(7, flagBackupOffset);

	flagHasBackupBuffer->CopyToDevice(true);

	flagRemoveKernel = new Kernel("Kernels/flag.cl", "Remove");

	flagRemoveKernel->SetArgument(0, deviceBuffer);
	flagRemoveKernel->SetArgument(1, flagPosBuffer);
	flagRemoveKernel->SetArgument(2, flagBackupBuffer);
	flagRemoveKernel->SetArgument(3, flagHasBackupBuffer);
	flagRemoveKernel->SetArgument(4, flagPattern->width);
	flagRemoveKernel->SetArgument(5, flagPattern->height);
	flagRemoveKernel->SetArgument(6, flagPosOffset);
	flagRemoveKernel->SetArgument(7, flagBackupOffset);

	// Bullets

	bulletDrawKernel = new Kernel("Kernels/bullet.cl", "Draw");

	bulletSpriteBuffer = new Buffer(flash_sprite_size + bullet_sprite_size, CL_MEM_READ_ONLY, bullet_sprites);
	bulletPosBuffer = new Buffer(maxBullets * 2, CL_MEM_READ_ONLY, bulletPos);
	bulletFrameBuffer = new Buffer(maxBullets, CL_MEM_READ_ONLY, bulletFrame);
	bulletFrameCounterBuffer = new Buffer(maxBullets, CL_MEM_READ_ONLY, bulletFrameCounter);


	bulletDrawKernel->SetArgument(0, deviceBuffer);
	bulletDrawKernel->SetArgument(1, bulletSpriteBuffer);
	bulletDrawKernel->SetArgument(2, bulletPosBuffer);
	bulletDrawKernel->SetArgument(3, bulletFrameBuffer);
	bulletDrawKernel->SetArgument(4, bulletSprite->frameSize);
	bulletDrawKernel->SetArgument(5, bulletFrameCounterBuffer);

	bulletSpriteBuffer->CopyToDevice(true);
	bulletPosBuffer->CopyToDevice(true);
	bulletFrameBuffer->CopyToDevice(true);
	bulletFrameCounterBuffer->CopyToDevice(true);

	bulletBackupKernel = new Kernel("Kernels/bullet.cl", "Backup");

	bulletBackupBuffer = new Buffer(maxBullets * sqr(bulletSprite->frameSize + 1));
	bulletLastTargetBuffer = new Buffer(maxBullets / 4, 0, bulletLastTarget);

	bulletBackupKernel->SetArgument(0, deviceBuffer);
	bulletBackupKernel->SetArgument(1, bulletPosBuffer);
	bulletBackupKernel->SetArgument(2, bulletBackupBuffer);
	bulletBackupKernel->SetArgument(3, bulletLastTargetBuffer);
	bulletBackupKernel->SetArgument(4, bulletSprite->frameSize);

	bulletLastTargetBuffer->CopyToDevice(true);


	bulletSaveLastPosKernel = new Kernel("Kernels/bullet.cl", "SaveLastPos");

	bulletLastPosBuffer = new Buffer(maxBullets * 2, CL_MEM_READ_ONLY, bulletLastPos);

	bulletSaveLastPosKernel->SetArgument(0, bulletPosBuffer);
	bulletSaveLastPosKernel->SetArgument(1, bulletLastPosBuffer);
	bulletSaveLastPosKernel->SetArgument(2, bulletLastTargetBuffer);
	bulletSaveLastPosKernel->SetArgument(3, bulletSprite->frameSize);

	bulletLastPosBuffer->CopyToDevice(true);

	bulletRemoveKernel = new Kernel("Kernels/bullet.cl", "Remove");
	bulletRemoveKernel->SetArgument(0, deviceBuffer);
	bulletRemoveKernel->SetArgument(1, bulletLastPosBuffer);
	bulletRemoveKernel->SetArgument(2, bulletBackupBuffer);
	bulletRemoveKernel->SetArgument(3, bulletLastTargetBuffer);
	bulletRemoveKernel->SetArgument(4, bulletSprite->frameSize);

	// spriteExplosion
	spriteExplosionDrawAdditiveKernel = new Kernel("Kernels/spriteExplosion.cl", "DrawAdditive");

	spriteExplosionSpriteBuffer = new Buffer(spriteExplosion_sprite_size, CL_MEM_READ_ONLY, spriteExplosion_sprites);
	spriteExplosionPosBuffer = new Buffer(maxSpriteExplosion * 2, CL_MEM_READ_ONLY, spriteExplosionPos);
	spriteExplosionFrameBuffer = new Buffer(maxSpriteExplosion, CL_MEM_READ_ONLY, spriteExplosionFrame);

	spriteExplosionDrawAdditiveKernel->SetArgument(0, deviceBuffer);
	spriteExplosionDrawAdditiveKernel->SetArgument(1, spriteExplosionSpriteBuffer);
	spriteExplosionDrawAdditiveKernel->SetArgument(2, spriteExplosionPosBuffer);
	spriteExplosionDrawAdditiveKernel->SetArgument(3, spriteExplosionFrameBuffer);
	spriteExplosionDrawAdditiveKernel->SetArgument(4, spriteExplosionSprite->frameSize);
	spriteExplosionDrawAdditiveKernel->SetArgument(5, spriteExplosionSprite->frameCount);

	spriteExplosionSpriteBuffer->CopyToDevice(true);
	spriteExplosionPosBuffer->CopyToDevice(true);
	spriteExplosionFrameBuffer->CopyToDevice(true);

	spriteExplosionBackupKernel = new Kernel("Kernels/spriteExplosion.cl", "Backup");

	spriteExplosionBackupBuffer = new Buffer(maxSpriteExplosion * spriteExplosionSprite->frameSize * spriteExplosionSprite->frameSize);
	spriteExplosionLastTargetBuffer = new Buffer(maxSpriteExplosion / 4, 0, spriteExplosionLastTarget);

	spriteExplosionBackupKernel->SetArgument(0, deviceBuffer);
	spriteExplosionBackupKernel->SetArgument(1, spriteExplosionPosBuffer);
	spriteExplosionBackupKernel->SetArgument(2, spriteExplosionBackupBuffer);
	spriteExplosionBackupKernel->SetArgument(3, spriteExplosionLastTargetBuffer);
	spriteExplosionBackupKernel->SetArgument(4, spriteExplosionSprite->frameSize);

	spriteExplosionLastTargetBuffer->CopyToDevice(true);

	spriteExplosionSaveLastPosKernel = new Kernel("Kernels/spriteExplosion.cl", "SaveLastPos");

	spriteExplosionLastPosBuffer = new Buffer(maxSpriteExplosion * 2, CL_MEM_READ_ONLY, spriteExplosionLastPos);

	spriteExplosionSaveLastPosKernel->SetArgument(0, spriteExplosionPosBuffer);
	spriteExplosionSaveLastPosKernel->SetArgument(1, spriteExplosionLastPosBuffer);
	spriteExplosionSaveLastPosKernel->SetArgument(2, spriteExplosionLastTargetBuffer);
	spriteExplosionSaveLastPosKernel->SetArgument(3, spriteExplosionSprite->frameSize);

	spriteExplosionLastPosBuffer->CopyToDevice(true);

	spriteExplosionRemoveKernel = new Kernel("Kernels/spriteExplosion.cl", "Remove");
	spriteExplosionRemoveKernel->SetArgument(0, deviceBuffer);
	spriteExplosionRemoveKernel->SetArgument(1, spriteExplosionLastPosBuffer);
	spriteExplosionRemoveKernel->SetArgument(2, spriteExplosionBackupBuffer);
	spriteExplosionRemoveKernel->SetArgument(3, spriteExplosionLastTargetBuffer);
	spriteExplosionRemoveKernel->SetArgument(4, spriteExplosionSprite->frameSize);

	// particleExplosion
	particleExplosionDrawKernel = new Kernel("Kernels/particleExplosion.cl", "Draw");

	particleExplosionPosBuffer =  new Buffer(maxParticleExplosion * particleMaxTotalPos * 2, CL_MEM_READ_ONLY, particleExplosionPos);
	particleExplosionMaxPosBuffer = new Buffer(maxParticleExplosion, CL_MEM_READ_ONLY, particleExplosionMaxPos);
	particleExplosionColorBuffer = new Buffer(maxParticleExplosion * particleMaxTotalPos , CL_MEM_READ_ONLY, particleExplosionColor);
	particleExplosionFadeBuffer = new Buffer(maxParticleExplosion * particleMaxTotalPos, CL_MEM_READ_ONLY, particleExplosionFade);

	particleExplosionDrawKernel->SetArgument(0, deviceBuffer);
	particleExplosionDrawKernel->SetArgument(1, particleExplosionPosBuffer);
	particleExplosionDrawKernel->SetArgument(2, particleExplosionMaxPosBuffer);
	particleExplosionDrawKernel->SetArgument(3, particleMaxTotalPos);
	particleExplosionDrawKernel->SetArgument(4, particleExplosionColorBuffer);
	particleExplosionDrawKernel->SetArgument(5, particleExplosionFadeBuffer);

	particleExplosionPosBuffer->CopyToDevice(true);
	particleExplosionMaxPosBuffer->CopyToDevice(true);
	particleExplosionColorBuffer->CopyToDevice(true);
	particleExplosionFadeBuffer->CopyToDevice(true);

	particleExplosionBackupKernel = new Kernel("Kernels/particleExplosion.cl", "Backup");

	particleExplosionBackupBuffer = new Buffer(maxParticleExplosion * particleMaxTotalPos * 4);

	particleExplosionBackupKernel->SetArgument(0, deviceBuffer);
	particleExplosionBackupKernel->SetArgument(1, particleExplosionPosBuffer);
	particleExplosionBackupKernel->SetArgument(2, particleExplosionMaxPosBuffer);
	particleExplosionBackupKernel->SetArgument(3, particleMaxTotalPos);
	particleExplosionBackupKernel->SetArgument(4, particleExplosionBackupBuffer);

	particleExplosionRemoveKernel = new Kernel("Kernels/particleExplosion.cl", "Remove");
	particleExplosionRemoveKernel->SetArgument(0, deviceBuffer);
	particleExplosionRemoveKernel->SetArgument(1, particleExplosionPosBuffer);
	particleExplosionRemoveKernel->SetArgument(2, particleExplosionMaxPosBuffer);
	particleExplosionRemoveKernel->SetArgument(3, particleMaxTotalPos);
	particleExplosionRemoveKernel->SetArgument(4, particleExplosionBackupBuffer);
	t.reset();
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
void MyApp::Tick(float deltaTime)
{
	/*
	deviceBuffer->CopyFromDevice(false);
	tankDrawKernel->Run(1);
	*/

	// draw the map
	//map.Draw( screen );
	// rebuild actor grid
	grid.Clear();
	grid.Populate(actorPool);
	// update and render actors
	// pointer->Remove();
	isBullet = false;
	isSpriteExplosion = false;
	isParticleExplosion = false;

	isNewBullet = false;
	isNewParticleExplosion = false;

	for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Tick();
	for (int i = 0; i < (int)actorPool.size(); i++) {

		isBullet = isBullet || (actorPool[i]->GetType() == Actor::BULLET);
		isSpriteExplosion = isSpriteExplosion || (actorPool[i]->GetType() == Actor::SPRITE_EXPLOSION);
		isParticleExplosion = isParticleExplosion || (actorPool[i]->GetType() == Actor::PARTICLE_EXPLOSION);

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
	}
	coolDown++;

	clFinish(Kernel::GetQueue());
	// report frame time
	static float frameTimeAvg = 10.0f; // estimate
	frameTimeAvg = 0.95f * frameTimeAvg + 0.05f * t.elapsed() * 1000;
	printf("frame time: %5.2fms\n", frameTimeAvg);
	t = Timer();

	pointerRemoveKernel->Run(pointer->sprite->frameSize * pointer->sprite->frameSize);
	for (int i = 0; i < 3; i++)
	{
		bushRemoveKernel[i]->Run2D(int2(bush[i]->frameSize * bush[i]->frameSize, bushCount[i]), int2(bush[i]->frameSize, 1));
	}
	particleExplosionRemoveKernel->Run2D(int2(particleMaxTotalPos * 4, maxParticleExplosion), int2(particleMaxTotalPos, 1));
	spriteExplosionRemoveKernel->Run2D(int2(spriteExplosionSprite->frameSize * spriteExplosionSprite->frameSize, maxSpriteExplosion), int2(spriteExplosionSprite->frameSize, 1));
	bulletRemoveKernel->Run2D(int2(bulletSprite->frameSize * bulletSprite->frameSize, maxBullets), int2(bulletSprite->frameSize, 1));
	flagRemoveKernel->Run2D(int2(flagBackupOffset, totalFlags), int2(2, totalFlags));
	tankRemoveKernel->Run2D(int2(tank1->frameSize * tank1->frameSize, totalTanks), int2(tank1->frameSize, 1));

	//for (int s = (int)sand.size(), i = s - 1; i >= 0; i--) sand[i]->Remove();
	//for (int s = (int)actorPool.size(), i = s - 1; i >= 0; i--) actorPool[i]->Remove();
	// 

	tankPosBuffer->CopyToDevice(false);
	tankFrameBuffer->CopyToDevice(false);
	tankOldPosBuffer->CopyToDevice(false);
	tankDirBuffer->CopyToDevice(false);
	tankSteerBuffer->CopyToDevice(false);

	bushPosBuffer->CopyToDevice(false);
	bushFrameBuffer->CopyToDevice(false);

	flagPosBuffer->CopyToDevice(false);

	tankTrackKernel->Run(totalTanks);

	tankBackupKernel->Run2D(int2(tank1->frameSize * tank1->frameSize, totalTanks), int2(tank1->frameSize, 1));
	tankSaveLastPosKernel->Run(totalTanks);
	tankDrawKernel->Run2D(int2((tank1->frameSize - 1)* (tank1->frameSize - 1), totalTanks), int2(tank1->frameSize - 1, 1));

	flagBackupKernel->Run2D(int2(flagBackupOffset, totalFlags), int2(totalFlags, 1));
	flagDrawKernel->Run2D(int2(flagPosOffset, totalFlags), int2(totalFlags, 1));

	if (isBullet) {
		if (isNewBullet)
		{
			bulletFrameBuffer->CopyToDevice(false);
		}

		bulletPosBuffer->CopyToDevice(false);
		bulletFrameCounterBuffer->CopyToDevice(false);

		bulletBackupKernel->Run2D(int2(bulletSprite->frameSize * bulletSprite->frameSize, maxBullets), int2(bulletSprite->frameSize, 1));
		bulletSaveLastPosKernel->Run(maxBullets);
		bulletDrawKernel->Run2D(int2((bulletSprite->frameSize - 1) * (bulletSprite->frameSize - 1), maxBullets), int2(bulletSprite->frameSize - 1, 1));

	}
	if (isSpriteExplosion)
	{
		spriteExplosionPosBuffer->CopyToDevice(false);
		spriteExplosionFrameBuffer->CopyToDevice(false);

		spriteExplosionBackupKernel->Run2D(int2(spriteExplosionSprite->frameSize * spriteExplosionSprite->frameSize, maxSpriteExplosion), int2(spriteExplosionSprite->frameSize, 1));
		spriteExplosionSaveLastPosKernel->Run(maxSpriteExplosion);
		spriteExplosionDrawAdditiveKernel->Run2D(int2((spriteExplosionSprite->frameSize) * (spriteExplosionSprite->frameSize), maxSpriteExplosion), int2(spriteExplosionSprite->frameSize, 1));
	}

	if (isParticleExplosion)
	{
		if (isNewParticleExplosion)
		{
			particleExplosionMaxPosBuffer->CopyToDevice(false);
			particleExplosionColorBuffer->CopyToDevice(false);
		}
		particleExplosionPosBuffer->CopyToDevice(false);
		particleExplosionFadeBuffer->CopyToDevice(false);

		particleExplosionBackupKernel->Run2D(int2(particleMaxTotalPos * 4, maxParticleExplosion), int2(particleMaxTotalPos, 1));
		particleExplosionDrawKernel->Run2D(int2(particleMaxTotalPos, maxParticleExplosion), int2(particleMaxTotalPos, 1));
	}

	int2 cursorPos = map.ScreenToMap(mousePos);
	pointer->lastPos = cursorPos;
	if (!pointerLastTarget) 
	{ 
		pointerLastTarget = 1; 
		pointerRemoveKernel->SetArgument(3, pointerLastTarget);
	}
	pointerRemoveKernel->SetArgument(2, pointer->lastPos);
	
	//pointer->Draw(map.bitmap, make_float2(cursorPos), 0);

	// bush draw
	for (int i = 0; i < 3; i++)
		bushBackupKernel[i]->Run2D(int2(bush[i]->frameSize * bush[i]->frameSize, bushCount[i]), int2(bush[i]->frameSize, 1));

	bushSaveLastPosKernel->Run(totalBushes);
	bushDrawKernel->Run2D(int2((max_bush_frameSize - 1) * (max_bush_frameSize - 1), totalBushes), int2(max_bush_frameSize - 1, 1));
	pointerDrawKernel->SetArgument(2, cursorPos);
	pointerDrawKernel->Run(pointer->sprite->frameSize * pointer->sprite->frameSize);
	
	// Write to screen
	screenKernel->SetArgument(2, map.view);
	screenKernel->SetArgument(3, map.dxy);
	
	screenKernel->Run(SCRWIDTH * SCRHEIGHT);

	// handle mouse
	HandleInput();
}