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
};

} // namespace Tmpl8