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
	static inline Buffer* deviceBuffer;

	// Tanks
	static inline Kernel* render_kernel;
	static inline Kernel* remove_kernel;
	static inline Kernel* saveLastPos_kernel;
	static inline Kernel* backup_kernel;

	static inline Buffer* spriteBuffer;
	static inline Buffer* tankPosBuffer;
	static inline Buffer* tankLastPosBuffer;
	static inline Buffer* tankBackUpBuffer;
	static inline Buffer* tankLastTargetBuffer;

	static inline Buffer* tankFrameBuffer;
	static inline Buffer* tankSpriteBuffer;

	static inline Buffer* tankZBuffer;
	static inline Buffer* tankBackupZBuffer;

	static inline bool* tankSprite;
	static inline float2* tankPos;
	static inline int* tankFrame;
	static inline int* tankLastTarget;

	//Bushes
	static inline Kernel* bushRender_kernel[3];
	static inline Kernel* bushRemove_kernel[3];
	static inline Kernel* bushSaveLastPos_kernel[3];
	static inline Kernel* bushBackup_kernel[3];

	static inline Buffer* bushSpriteBuffer[3];
	static inline Buffer* bushPosBuffer[3];
	static inline Buffer* bushLastPosBuffer[3];
	static inline Buffer* bushBackUpBuffer[3];
	static inline Buffer* bushLastTargetBuffer[3];

	static inline Buffer* bushFrameBuffer[3];

	static inline float2* bushPos[3];
	static inline int* bushFrame[3];
	static inline int* bushLastTarget[3];

	static inline int bushCount[3];

	//Flags
	static inline Kernel* flagRender_kernel;
	static inline Kernel* flagRemove_kernel;
	static inline Kernel* flagSaveLastPos_kernel;
	static inline Kernel* flagBackup_kernel;

	static inline Buffer* flagSpriteBuffer;
	static inline Buffer* flagPosBuffer;
	static inline Buffer* flagLastPosBuffer;
	static inline Buffer* flagBackUpBuffer;
	static inline Buffer* flagLastTargetBuffer;

	static inline Buffer* flagFrameBuffer;

	static inline float2* flagPos;
	static inline int* flagFrame;
	static inline int* flagLastTarget;

	static inline int flagCount;

};

} // namespace Tmpl8