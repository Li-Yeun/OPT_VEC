#pragma once

namespace Tmpl8
{

class MyApp : public TheApp
{
public:
	// game flow methods
	void Init();
	void HandleInput();
	void Tick( float deltaTime );
	void Shutdown() { /* implement if you want to do something on exit */ }
	// input handling
	void MouseUp( int button ) { mouseDown = false; }
	void MouseDown( int button ) { mouseDown = true; }
	void MouseMove( int x, int y ) { mousePos.x = x, mousePos.y = y; }
	void MouseWheel( float y );
	void KeyUp( int key ) { /* implement if you want to handle keys */ }
	void KeyDown( int key ) { /* implement if you want to handle keys */ }
	// data members
	float zoom = 100;							// map zoom
	int2 mousePos, dragStart, focusStart;		// mouse / map interaction
	bool mouseDown = false;						// keeping track of mouse button status
	Sprite* tank1, *tank2;						// tank sprites
	Sprite* bush[3];							// bush sprite
	SpriteInstance* pointer;					// mouse pointer sprite
	// static data, for global access
	static inline Map map;						// the map
	static inline vector<Actor*> actorPool;		// actor pool
	static inline vector<float3> peaks;			// mountain peaks to evade
	static inline vector<Particle*> sand;		// sand particles
	static inline Grid grid;					// actor grid for faster range queries
	static inline int coolDown = 0;				// used to prevent simultaneous firing

	// Output buffer
	static inline Buffer* deviceBuffer;
	Buffer* screenBuffer;
	
	// Screen draw kernel
	static inline Kernel* screenKernel;

	//Pointer
	static inline Kernel* pointerDrawKernel;
	static inline Kernel* pointerRemoveKernel;
	int pointerLastTarget = 0;
	static inline Buffer* pointerBackupBuffer;
	static inline Buffer* pointerSpriteBuffer;

	// tanks
	static inline uint* tankSprite;
	static inline bool* tankTeam;
	static inline float2* tankPos;
	static inline int2* tankLastPos;
	static inline int* tankFrame;
	static inline bool* tankLastTarget;


	static inline Kernel* tankDrawKernel;
	static inline Kernel* tankBackupKernel;
	static inline Kernel* tankSaveLastPosKernel;
	static inline Kernel* tankRemoveKernel;

	static inline Buffer* tankSpriteBuffer;
	static inline Buffer* tankTeamBuffer;
	static inline Buffer* tankPosBuffer;
	static inline Buffer* tankLastPosBuffer;
	static inline Buffer* tankFrameBuffer;
	static inline Buffer* tankBackupBuffer;
	static inline Buffer* tankLastTargetBuffer;

	// tank tracks
	static inline float2* tankOldPos;
	static inline float2* tankDir;
	static inline float* tankSteer;

	static inline Kernel* tankTrackKernel;

	static inline Buffer* tankOldPosBuffer;
	static inline Buffer* tankDirBuffer;
	static inline Buffer* tankSteerBuffer;

	// dust particles
	static inline uint* bushCount;
	static inline uint* bushCounter;
	static inline uint* bushTypeIndex[3];

	static inline uint* bushFrameSize;
	static inline uint* bushSpriteOffset;
	static inline uint* bushType;
	static inline float2* bushPos;
	static inline int2* bushLastPos;
	static inline int* bushFrame;
	static inline bool* bushLastTarget;

	static inline Kernel* bushDrawKernel;
	static inline Kernel* bushBackupKernel[3];
	static inline Kernel* bushSaveLastPosKernel;
	static inline Kernel* bushRemoveKernel[3];

	static inline Buffer* bushSpriteBuffer;
	static inline Buffer* bushFrameSizeBuffer;
	static inline Buffer* bushSpriteOffsetBuffer;
	static inline Buffer* bushTypeBuffer;
	static inline Buffer* bushTypeIndexBuffer[3];
	static inline Buffer* bushPosBuffer;
	static inline Buffer* bushLastPosBuffer;
	static inline Buffer* bushFrameBuffer;
	static inline Buffer* bushBackupBuffer[3];
	static inline Buffer* bushLastTargetBuffer;

	// Flags
	static inline uint* flagSprite;
	static inline float2* flagPos;
	static inline bool* flagHasBackup;

	static inline Kernel* flagDrawKernel;
	static inline Kernel* flagBackupKernel;
	static inline Kernel* flagRemoveKernel;

	static inline Buffer* flagSpriteBuffer;
	static inline Buffer* flagPosBuffer;
	static inline Buffer* flagBackupBuffer;
	static inline Buffer* flagHasBackupBuffer;

	// bullets
	static inline Sprite* flashSprite;
	static inline Sprite* bulletSprite;
	static inline uint bulletCounter = 0;
	static inline uint maxBullets = 100;
	static inline float2* bulletPos;
	static inline int2* bulletLastPos;
	static inline int* bulletFrame;
	static inline int* bulletFrameCounter;
	static inline bool* bulletLastTarget;

	static inline Kernel* bulletDrawKernel;
	static inline Kernel* bulletBackupKernel;
	static inline Kernel* bulletSaveLastPosKernel;
	static inline Kernel* bulletRemoveKernel;

	static inline Buffer* bulletSpriteBuffer;
	static inline Buffer* bulletBackupBuffer;
	static inline Buffer* bulletPosBuffer;
	static inline Buffer* bulletLastPosBuffer;
	static inline Buffer* bulletFrameBuffer;
	static inline Buffer* bulletFrameCounterBuffer;
	static inline Buffer* bulletLastTargetBuffer;

	static inline bool isBullet = false;
	static inline bool isNewBullet = false;

	// spriteExplosion
	static inline Sprite* spriteExplosionSprite;
	static inline uint spriteExplosionCounter = 0;
	static inline uint maxSpriteExplosion = 50;

	static inline int2* spriteExplosionPos;
	static inline int2* spriteExplosionLastPos;
	static inline int* spriteExplosionFrame;
	static inline bool* spriteExplosionLastTarget;

	static inline Kernel* spriteExplosionDrawAdditiveKernel;
	static inline Kernel* spriteExplosionBackupKernel;
	static inline Kernel* spriteExplosionSaveLastPosKernel;
	static inline Kernel* spriteExplosionRemoveKernel;

	static inline Buffer* spriteExplosionSpriteBuffer;
	static inline Buffer* spriteExplosionBackupBuffer;
	static inline Buffer* spriteExplosionPosBuffer;
	static inline Buffer* spriteExplosionLastPosBuffer;
	static inline Buffer* spriteExplosionFrameBuffer;
	static inline Buffer* spriteExplosionLastTargetBuffer;

	static inline bool isSpriteExplosion = false;

	// particleExplosion
	static inline uint particleExplosionCounter = 0;
	static inline uint maxParticleExplosion = 50;
	static inline int particleMaxTotalPos;

	static inline uint* particleExplosionColor;
	static inline int2* particleExplosionPos;
	static inline uint* particleExplosionMaxPos;
	static inline uint* particleExplosionFade;


	static inline Kernel* particleExplosionDrawKernel;
	static inline Kernel* particleExplosionBackupKernel;
	static inline Kernel* particleExplosionRemoveKernel;

	static inline Buffer* particleExplosionBackupBuffer;
	static inline Buffer* particleExplosionPosBuffer;
	static inline Buffer* particleExplosionMaxPosBuffer;
	static inline Buffer* particleExplosionColorBuffer;
	static inline Buffer* particleExplosionFadeBuffer;

	static inline bool isParticleExplosion = false;
	static inline bool isNewParticleExplosion = false;
};

} // namespace Tmpl8