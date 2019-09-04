#pragma once

#include "ofMain.h"
#include "ofxAssimpModelLoader.h"
#include "ofxGui.h"
#include "box.h"
#include "ray.h"
#include "Octree.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void initLightingAndMaterials();
		void drawAxis(ofVec3f);
		void savePicture();
		void setCameraTarget();
		void togglePointsDisplay();
		void toggleWireframeMode();
		void toggleSelectTerrain();
		bool doPointSelection();
		bool octreePointSelection();
		void drawText();
		void vehicleMove();
		void checkCollisions();
		void loadVbo();
		void drawLandingZone();

		bool mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point);

		ofEasyCam easyCam;
		ofCamera trackingCam;
		ofCamera bottomCam;
		ofCamera frontCam;
		ofCamera *currentCam;

		ofSoundPlayer thrustSound;
		ofSoundPlayer martianWind;

		ofxAssimpModelLoader mars, rover;
		Box boundingBox;

		TreeNode selectedNode;

		ofVec3f selectedPoint;

		Octree octree;

		Particle *vehicle;
		ParticleSystem *vehicleSys;
		ParticleEmitter *emitter;

		ofVec3f gravity;
		TurbulenceForce *tForce;
		GravityForce *gForce;
		ThrustForce *thrustForce;
		ImpulseForce *iForce;

		float thrustForceMag;
		float terrainGravityMag;
		float altitude;

		int levels;

		bool bDrawTree;
		bool bDrawLeafs;
		
		bool bWireframe;
		bool bRoverLoaded;
		bool bTerrainSelected;

		bool bAltKeyDown;
		bool bCtrlKeyDown;
		bool bUp;
		bool bDown;
		bool bRight;
		bool bLeft;
		bool bSpace;
		bool bGrounded;
		bool bHide = false;

		bool bDisplayPoints;
		bool bPointSelected;

		bool bStart, bOver;
		bool bEmitterStart;

		const float selectionRange = 4.0;

		float radius;

		int landingZone;			// landing zone value. Made for passing data to draw f(x)
									// [0, 1, 2, 3, 4] for [none, blue, green, orangeRed, brown] respectively

		double score;				// for keeping score 
		int startTime;			// Time at the start of the game (post spacebar)
		int tempTime;			// temporary time keeping variable
		int timer;				// secondary score-keeping factor
		bool bcalc;
		//textures
		ofTexture particleTex;

		//shaders
		ofVbo vbo;
		ofShader shader;

		//lights
		ofLight keyLight, rimLight, fillLight, dynamicLight;
		vector <ofLight*> Lights;
};
