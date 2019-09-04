#include "ofApp.h"
#include "Util.h"

/*

Keybinds:
	F1 is the easy cam
	F2 is the tracking cam
	F3 is the bottom cam
	F4 is the front cam
	
	Up arrow is for forward
	Down arrow is for backward
	Right arrow is for going right
	Left arrow is for going left
	Spacebar is for going up

*/

//--------------------------------------------------------------
void ofApp::setup(){
	bDrawTree = false;
	bDrawLeafs = false;

	bWireframe = false;
	bRoverLoaded = false;
	bTerrainSelected = true;
	
	bAltKeyDown = false;
	bCtrlKeyDown = false;
	bDisplayPoints = false;
	bPointSelected = false;
	bGrounded = false;

	bStart = false; // Press Space to start
	bOver = false;  // Game over when Lander lands
	bEmitterStart = false;
	bcalc = false;

	thrustForceMag = 5.0f;
	terrainGravityMag = 3.711f;
	radius = 5;

	levels = 8; //Remember to change to around 8 or higher for final version
	altitude = 0;

	score = 10000;
	startTime = 0;
	tempTime = 0;
	timer = 0;
	landingZone = 0;

	// texture loading
	//
	ofDisableArbTex();     // disable rectangular textures

	// load textures
	//
	if (!ofLoadImage(particleTex, "images/dot.png")) {
		cout << "Particle Texture File: images/dot.png not found" << endl;
		ofExit();
	}

	// load the shader
	//
#ifdef TARGET_OPENGLES
	shader.load("shaders_gles/shader");
#else
	shader.load("shaders/shader");
#endif

	ofSetVerticalSync(true);
	ofEnableSmoothing();
	ofEnableDepthTest();

	initLightingAndMaterials();

	//Loads terrain, octree, and vehicle
	//-Aaron Warren
	
	// models created/tweaked and skinned by Shahbaz Singh Mansahia
	if (mars.loadModel("geo/Lunar_Lander_mars_terrain_model.obj")){			//CUSTOM TERRAIN MODEL NOT LOADING!; Fixed, specified wrong location
	//if (mars.loadModel("geo/mars-low-v2.obj")) {							//PLAN B; DEFAULT TERRAIN
		mars.setRotation(0, 180, 0, 0, 1);
		mars.setScaleNormalization(false);
		printf("Map loaded, creating octree...\n");

		float time = ofGetElapsedTimef();
		octree.create(mars.getMesh(0), levels);
		printf("Setup complete in %.0fms\n", (ofGetElapsedTimef() - time) * 1000);
	}
	else {
		printf("Map could not be loaded.\n");
		ofExit(0);
	}
	if (rover.loadModel("geo/Lunar_Lander_lander_texture.obj")){						//CUSTOM ROVER MODEL NOT LOADING!; Fixed, specified wrong location
	//if (rover.loadModel("geo/lander.obj")) {								//PLAN B; DEFAULT ROVER
		rover.setScale(.001, .001, .001);
		rover.setRotation(0, 180, 0, 0, 1);
		rover.setPosition(0, 5, 0);

		bRoverLoaded = true;

		cout << "Vehicle loaded at position: " << rover.getPosition() << endl;
	}
	else {
		printf("Vehicle could not be loaded.\n");
		ofExit(0);
	}

	//Loads sounds and makes sure the bg noise will loop
	//-Aaron Warren
	if (thrustSound.load("sound/Constant Rocket Engines.mp3") &&
		martianWind.load("sound/InSight Lander Mars Wind.mp3")) {
		thrustSound.setVolume(0.5);
		martianWind.setVolume(0.2);
		martianWind.setLoop(true);
		cout << "Sound loaded successfully..." << endl;
	}
	else {
		cout << "Sound could not be loeaded" << endl;
	}

	//Dummy particle used to manage vehicle
	//-Aaron Warren
	vehicle = new Particle();
	vehicle->lifespan = -1;
	vehicle->mass = 1;
	vehicle->position = rover.getPosition();

	easyCam.setPosition(-20.6871, 12.2888, -11.4966);
	easyCam.lookAt(glm::vec3(0, 0, 0));
	easyCam.setDistance(10);
	easyCam.setNearClip(.1);
	easyCam.setFov(65.5);

	//Have to use glm::vec3 since using vehicle->position redners cam inside of model
	trackingCam.setPosition(12.0669, 14.7858, 13.9889);
	trackingCam.lookAt(glm::vec3(vehicle->position.x, vehicle->position.y, vehicle->position.z));
	trackingCam.setNearClip(.1);

	bottomCam.setPosition(glm::vec3(vehicle->position.x, vehicle->position.y + .125, vehicle->position.z));
	bottomCam.lookAt(glm::vec3(0, 1, 0));
	bottomCam.setNearClip(.1);

	frontCam.setPosition(glm::vec3(vehicle->position.x, vehicle->position.y + 1, vehicle->position.z));
	frontCam.lookAt(glm::vec3(0, 0, 5));
	frontCam.setNearClip(.1);

	currentCam = &easyCam;
	
	//System used to manage forces on vehicle
	//-Aaron Warren
	vehicleSys = new ParticleSystem();
	vehicleSys->add(*vehicle);
	vehicle = &(vehicleSys->particles.at(0));
	gravity = ofVec3f(0, -terrainGravityMag, 0);
	gForce = new GravityForce(gravity);
	tForce = new TurbulenceForce(ofVec3f(-0.5, -0.5, -0.5), ofVec3f(.5, .5, .5));
	thrustForce = new ThrustForce();
	iForce = new ImpulseForce();
	vehicleSys->addForce(thrustForce);
	vehicleSys->addForce(gForce);
	vehicleSys->addForce(tForce);
	vehicleSys->addForce(iForce);

	emitter = new ParticleEmitter();
	emitter->setOneShot(true);
	emitter->setEmitterType(DiscEmitter);
	emitter->setGroupSize(100);
	emitter->particleColor = ofColor::orange;
	emitter->particleRadius = .001;
	emitter->setRandomLife(true);
	emitter->setLifespanRange(ofVec2f(0.1, 1));
	emitter->setVelocity(ofVec3f(0, 5, 0));
	emitter->sys->addForce(new TurbulenceForce(ofVec3f(-10, 0, -10), ofVec3f(10, 0, 10)));

	//		Dynamic light added as it is a good aide for 3d positioning 
	//		- Shahbaz Singh Mansahia

	dynamicLight.setup();												// Put this here because otherwise we have no reference for the rover position
	dynamicLight.enable();
	dynamicLight.setSpotlight();
	dynamicLight.setScale(.05);
	dynamicLight.setSpotlightCutOff(10);
	dynamicLight.setAttenuation(.2, .001, .001);
	dynamicLight.setAmbientColor(ofFloatColor(1, 1, 1));
	dynamicLight.setDiffuseColor(ofFloatColor(1, 1, 1));
	dynamicLight.setSpecularColor(ofFloatColor(1, 1, 1));
	dynamicLight.rotate(180, ofVec3f(0, 1, 0));
	dynamicLight.setPosition((ofVec3f) (rover.getPosition(), rover.getPosition() + 10, rover.getPosition()));
	dynamicLight.rotate(90, ofVec3f(1, 0, 0));
	cout << "Setup complete." << endl;
}

//--------------------------------------------------------------
void ofApp::update(){
	//Checks if space was hit before starting game
	if (bStart) {
		//Updates rover model and thrust emitter position to coincide with vehicle particle
		//-Aaron Warren
		rover.setPosition(vehicle->position.x, vehicle->position.y, vehicle->position.z);
		emitter->setPosition(ofVec3f(vehicle->position.x, vehicle->position.y, vehicle->position.z));

		Ray altRay = Ray(Vector3(vehicle->position.x, vehicle->position.y, vehicle->position.z), 
			Vector3(vehicle->position.x, vehicle->position.y - 200, vehicle->position.z));
		TreeNode altNode;
		if (octree.intersect(altRay, octree.root, altNode)) {
			altitude = glm::length(octree.mesh.getVertex(altNode.points[0]) - glm::vec3(vehicle->position));
		}

		//Checks if there is a collision with the ground and then counteracts down force to stop lander
		//After that it will wait until lander is slowed to a point and then brute forces a full stop
		//-Aaron Warren
		checkCollisions(); 
		
		/*		// REFACTORING LOOP TO AVOID CLIPPING THROUGH GROUND AND IMPLEMENTING LANDING - Shahbaz

		//	Basic collision detection; stops the ship when it crashes into the ground slowly
		//	-Aaron Warren
		if (bGrounded && vehicle->velocity.y <= 0.05 && vehicle->velocity.y >= -.05) {
			vehicle->velocity.set(0, 0, 0);
			vehicle->acceleration.set(0, 0, 0);
			vehicle->forces.set(0, 0, 0);
		}
		*/

		// Shahbaz Singh Mansahia
		// REFACTORED CONDITIONAL:
		//cout << vehicle->velocity.y << endl;
		if (bGrounded) {
			//if (vehicle->velocity.y <= -1000.05 && vehicle->velocity.y != 0) {
			vehicle->velocity.set(0, 0, 0);
			vehicle->acceleration.set(0, 0, 0);
			vehicle->forces.set(0, 0, 0);
			bOver = true;						// triggers Game over
			//cout << "GAME OVER" << endl;
			//}
			
			//else {
			//	vehicle->velocity.set(0, 0, 0);
			//	vehicle->acceleration.set(0, 0, 0);
			//	vehicle->forces.set(0, 0, 0);
			//}
			
		}

		
		tempTime = ofGetElapsedTimeMillis() / 1000;
		cout << "tempTime: " << tempTime << endl;
		if (!bOver) {
			timer = tempTime - startTime;
		}
		
		trackingCam.lookAt(vehicle->position);
		bottomCam.setPosition(vehicle->position.x, vehicle->position.y + .125, vehicle->position.z);
		frontCam.setPosition(vehicle->position.x, vehicle->position.y + 1, vehicle->position.z);

		//Makes sure the bg sound is playing at all times when game is started
		if (!martianWind.isPlaying()) martianWind.play();

		//Moves the vehicle and updates vehicle managing system
		vehicleMove();
		vehicleSys->update();
		emitter->update();
		// to follow the rover position
		dynamicLight.setPosition((ofVec3f)(rover.getPosition(), rover.getPosition() + 10, rover.getPosition()));

		/*				for reference of the landing zone scoring system
		ofSetColor(ofColor::blue);
		ofDrawPlane(5, 5, -4, 1, 1); 1*1
		LANDED ON: 5.08399, 3.00873, 5.66579
		ofSetColor(ofColor::green);
		ofDrawPlane(-7.5, -10, -1.2, 1, 1); 1*1
		LANDED ON: -7.4531, 2.49968, -9.2564
		ofSetColor(ofColor::orangeRed);
		ofDrawPlane(-4.2, 1.4, -1.4, 1, 1); 1*1
		LANDED ON: 6.11987, 2.54543, -5.72112
		ofSetColor(ofColor::brown);
		ofDrawPlane(6, -6, -1.6, 1, 1); 1*1
		LANDED ON: -4.09708, 1.06109, 1.69916
		*/

		//	Game over Implementation
		//	Shahbaz Singh Mansahia
		if (bOver) {
			//cout << vehicle->position << endl;
			float vx = vehicle->position.x, vy = vehicle->position.y, vz = vehicle->position.z;
			if ((vx >= 4.45 && vx <= 5.8) && (vy >= 2.48 && vy <= 3.13) && (vz >= 5.11 && vz <= 6.22)) {			// Blue ;DOUBLE CHECKED
				landingZone = 1;
				if (!bcalc) {
					bcalc = true;
					score = score - (timer / 10);
				}
			}
			else if ((vx >= -8.2 && vx <= -6.8) && (vy >= 2.35 && vy <= 3.05) && (vz >= -10.5 && vz <= - 8.95)) {	// Green; DOUBLE CHECKED
				landingZone = 2;
				if (!bcalc) {
					bcalc = true;
					score = score - (timer * 100);
				}
			}
			else if ((vx >= 5.53 && vx <= 6.59) && (vy >= 2.26 && vy <= 2.68) && (vz >= -6.16 && vz <= -5.00)) {	// Orange-Red; DOUBLE CHECKED
				landingZone = 3;
				if (!bcalc) {
					bcalc = true;
					score = score - (timer * 10);
				}
			}
			else if ((vx >= -4.75 && vx <= -3.57) && (vy >= 1.02 && vy <= 1.07) && (vz >= 1.02 && vz <= 2.14)) {	// Brown; DOUBLE CHECKED
				landingZone = 4;
				if (!bcalc) {
					bcalc = true;
					score = score - timer;
				}
			}
			score = (score < 0) ? 0 : score;

		}

	}
}

// Handles collision detection
//-Aaron Warren
void ofApp::checkCollisions() {
	TreeNode intersectedNode;

	//Checks if the vehicle particle intersects any point in the octree
	if (octree.intersect(vehicle->position, octree.root, intersectedNode)) {
		//If it does then the lander can only move up
		bGrounded = true;
		//Removes turbulence and gravity
		tForce->set(ofVec3f(0, 0, 0), ofVec3f(0, 0, 0));
		gForce->set(ofVec3f(0, 0, 0));
		//Counteracts current velocity to stop it from moving entirely
		ofVec3f normal = octree.mesh.getNormal(intersectedNode.points.at(0));
		ofVec3f vec = ofGetFrameRate() * -1 * vehicle->velocity;
		ofVec3f force = 1.6 * (vec.dot(normal) * normal);
		iForce->set(force);
	}
	else {
		//Otherwise it is in the air and therefore must have turbulence, gravity, and omnidirectional movement
		bGrounded = false;
		tForce->set(ofVec3f(-0.5, -0.5, -0.5), ofVec3f(0.5, 0.5, 0.5));
		gForce->set(gravity);
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofSetBackgroundColor(ofColor::black);
	//if(!bHide) gui.draw();

	currentCam->begin();
	ofPushMatrix();

	drawLandingZone();

	//keyLight.draw();						// Just for troubleshooting and lighting experimentation/optimization
	//fillLight.draw();
	//rimLight.draw();

	//Draws terrain and vehicle
	if (bWireframe) {                    // wireframe mode  (include axis)
		ofDisableLighting();
		ofSetColor(ofColor::slateGray);
		mars.drawWireframe();
		if (bRoverLoaded) {
			rover.drawWireframe();
			if (!bTerrainSelected) drawAxis(rover.getPosition());
		}
		if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));
	}
	else {
		ofEnableLighting();              // shaded mode
		mars.drawFaces();

		if (bRoverLoaded) {
			rover.drawFaces();
			if (!bTerrainSelected) drawAxis(rover.getPosition());
		}
		if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));
	}

	if (bDisplayPoints) {
		glPointSize(3);
		ofSetColor(ofColor::green);
		mars.drawVertices();
	}

	//Draws location clicked
	if (bPointSelected) {
		ofSetColor(ofColor::blue);
		ofDrawSphere(selectedPoint, .1);
	}

	ofNoFill();

	//Draws octree and leaves
	if (bDrawTree) octree.draw(levels, 0);
	else if (bDrawLeafs) {
		ofSetColor(ofColor::white);
		octree.drawLeafNodes();
	}


	// Uses shaders to render particles from radialEmitterExample-shader example
	// -Aaron Warren
	loadVbo();
	glDepthMask(GL_FALSE);
	ofSetColor(ofColor::orange);

	// this makes everything look glowy :)
	//
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofEnablePointSprites();

	shader.begin();
	particleTex.bind();
	vbo.draw(GL_POINTS, 0, (int)emitter->sys->particles.size());
	particleTex.unbind();

	shader.end();

	ofDisablePointSprites();
	ofDisableBlendMode();
	ofEnableAlphaBlending();

	// set back the depth mask
	//
	glDepthMask(GL_TRUE);

	ofPopMatrix();
	currentCam->end();

	// Draws start, altitude, and frame rate text
	// -Aaron Warren
	if (bStart && !bOver) {
		drawText();
	}
	
	//	Draws 'Game over' and Points System
	//	- Shahbaz Singh Mansahia
	if (bOver) {
		ofSetColor(ofColor::red);
		ofDrawBitmapString("Game Over!", (ofGetWindowWidth() / 2) - 85, (ofGetWindowHeight()) / 2 - 5);
		switch (landingZone) {
			case 0:
				ofSetColor(ofColor::white);
				ofDrawBitmapString("Points: 0", (ofGetWindowWidth() / 2) - 87, (ofGetWindowHeight()) / 2 + 10);	// CHECKED
				break;
			case 1:
				ofSetColor(ofColor::blue);
				ofDrawBitmapString("Points: " + std::to_string(score), (ofGetWindowWidth() / 2) - 87, (ofGetWindowHeight()) / 2 + 10);	// CHECKED
				break;
			case 2:
				ofSetColor(ofColor::green);
				ofDrawBitmapString("Points: " + std::to_string(score), (ofGetWindowWidth() / 2) - 87, (ofGetWindowHeight()) / 2 + 10);	// CHECKED
				break;
			case 3:
				ofSetColor(ofColor::orangeRed);
				ofDrawBitmapString("Points: " + std::to_string (score), (ofGetWindowWidth() / 2) - 87, (ofGetWindowHeight()) / 2 + 10);	// CHECKED
				break;
			case 4:
				ofSetColor(ofColor::brown);																			
				ofDrawBitmapString("Points: " + std::to_string (score), (ofGetWindowWidth() / 2) - 87, (ofGetWindowHeight()) / 2 + 10);	// CHECKED
				break;

			default:
				cout << "ERROR: Invalid Value for 'landingZone'" << endl;
				break;
		}
	}
	else {
		ofSetColor(ofColor::white);
		ofDrawBitmapString("Press Spacebar to Start", (ofGetWindowWidth() / 2) - 92, ofGetWindowHeight() / 2 - 5);
	}
}

//Draw altitudes and frame rate on screen
//-Aaron Warren
void ofApp::drawText() {
	ofSetColor(ofColor::white);
	string altText = "Altitude: " + std::to_string(altitude);
	int framerate = ofGetFrameRate();
	string fpsText = "Frame Rate: " + std::to_string(framerate);

	string timerText = "Time: " + std::to_string(timer);
	//string velocityX = "Vel X: " + std::to_string(vehicle->velocity.x);
	//string velocityY = "Vel Y: " + std::to_string(vehicle->velocity.y);
	//ofDrawBitmapString(velocityX, 10, 27);
	//ofDrawBitmapString(velocityY, 10, 39);

	ofDrawBitmapString(altText, 10, 15);
	ofDrawBitmapString(fpsText, ofGetWindowWidth() - 130, 15);
	ofDrawBitmapString(timerText, 10, 40);
}

//Draws landing zones
//-Aaron Warren
void ofApp::drawLandingZone() {
	ofSetColor(ofColor::gray);

	//Saves current transformations
	ofPushMatrix();
	//Rotates next drawings so they lay on the terrain
	ofRotateX(100);
	ofSetColor(ofColor::blue);
	ofDrawPlane(5, 5, -4, 1, 1);
	ofSetColor(ofColor::green);
	ofDrawPlane(-7.5, -10, -1.2, 1, 1);
	ofSetColor(ofColor::orangeRed);
	ofDrawPlane(-4.2, 1.4, -1.4, 1, 1);
	ofSetColor(ofColor::brown);
	ofDrawPlane(6, -6, -1.6, 1, 1);
	//Restores previous transformations
	ofPopMatrix();
}

void ofApp::drawAxis(ofVec3f location) {

	ofPushMatrix();
	ofTranslate(location);

	ofSetLineWidth(1.0);

	//// X Axis
	//ofSetColor(ofColor(255, 0, 0));
	//ofDrawLine(ofPoint(0, 0, 0), ofPoint(1, 0, 0));


	//// Y Axis
	//ofSetColor(ofColor(0, 255, 0));
	//ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 1, 0));

	//// Z Axis
	//ofSetColor(ofColor(0, 0, 255));
	//ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 0, 1));

	ofPopMatrix();
}


//Vehicle movement and thrust sound
//-Aaron Warren
void ofApp::vehicleMove() {

	ofVec3f movement = ofVec3f(0, 0, 0);

	if (bSpace) {
		movement += ofVec3f(0, 1, 0);
		emitter->start();
		if(!thrustSound.isPlaying()) thrustSound.play();
	}
	else {
		emitter->stop();
		thrustSound.stop();
	}
	if (bUp && !bGrounded) movement += ofVec3f(0, 0, 0.5);
	if (bDown && !bGrounded) movement += ofVec3f(0, 0, -0.5);
	if (bLeft && !bGrounded)movement += ofVec3f(-0.5, 0, 0);
	if (bRight && !bGrounded) movement += ofVec3f(0.5, 0, 0);
	
	thrustForce->set(movement, thrustForceMag);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

	switch (key) {
	case '1':
		cout << currentCam->getPosition() << endl;
		//printf("Brown: %f\nRed: %f\n", (float)brownX, (float)redX);
		break;
	case ' ':
		if (!bStart) {				// added this to get the start time
			bStart = true;
			startTime = ofGetElapsedTimeMillis() / 1000;
			//cout << "StartTime: " << startTime << endl;
		}
		bStart = true;
		bSpace = true;
		break;
	case 'C':
	case 'c':
		//if (currentCam.getMouseInputEnabled()) currentCam.disableMouseInput();
		//else currentCam.enableMouseInput();
		break;
	case 'F':
	case 'f':
		ofToggleFullscreen();
		break;
	case 'H':
	case 'h':
		bHide = !bHide;
		break;
	case 'o':
		bDrawTree = !bDrawTree;
		break;
	case 'l':
		bDrawLeafs = !bDrawLeafs;
	case 's':
		savePicture();
		break;
	case 'u':
		break;
	case 'v':
		togglePointsDisplay();
		break;
	case 'V':
		break;
	case 'w':
		toggleWireframeMode();
		break;
	case OF_KEY_ALT:
		bAltKeyDown = true;
		break;
	// Key toggles for movement
	// -Aaron Warren
	case OF_KEY_CONTROL:
		bCtrlKeyDown = true;
		break;
	case OF_KEY_SHIFT:
		break;
	case OF_KEY_DEL:
		break;
	case OF_KEY_UP:
		bUp = true;
		break;
	case OF_KEY_DOWN:
		bDown = true;
		break;
	case OF_KEY_RIGHT:
		bRight = true;
		break;
	case OF_KEY_LEFT:
		bLeft = true;
		break;
	// Key toggles to change current camera
	// -Aaron Warren
	case OF_KEY_F1:
		currentCam = &easyCam;
		break;
	case OF_KEY_F2:
		currentCam = &trackingCam;
		break;
	case OF_KEY_F3:
		currentCam = &bottomCam;
		break;
	case OF_KEY_F4:
		currentCam = &frontCam;
		break;
	case 't':
	case 'T':
		easyCam.lookAt(vehicle->position);
		break;
	case 'r':
	case 'R':
		if (bPointSelected) easyCam.lookAt(selectedPoint);
		break;
	default:
		break;
	}
}

void ofApp::toggleWireframeMode() {
	bWireframe = !bWireframe;
}

void ofApp::toggleSelectTerrain() {
	bTerrainSelected = !bTerrainSelected;
}

void ofApp::togglePointsDisplay() {
	bDisplayPoints = !bDisplayPoints;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
	switch (key) {
	case OF_KEY_ALT:
		bAltKeyDown = false;
		break;
	case OF_KEY_CONTROL:
		bCtrlKeyDown = false;
		break;
	case OF_KEY_SHIFT:
		break;
	case OF_KEY_UP:
		bUp = false;
		break;
	case OF_KEY_DOWN:
		bDown = false;
		break;
	case OF_KEY_RIGHT:
		bRight = false;
		break;
	case OF_KEY_LEFT:
		bLeft = false;
		break;
	case ' ':
		bSpace = false;
		break;
	default:
		break;

	}
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	ofVec3f mouse(mouseX, mouseY);
	ofVec3f rayPoint = currentCam->screenToWorld(mouse);
	ofVec3f rayDir = rayPoint - currentCam->getPosition();
	rayDir.normalize();
	Ray ray = Ray(Vector3(rayPoint.x, rayPoint.y, rayPoint.z),
		Vector3(rayDir.x, rayDir.y, rayDir.z));
	vector<TreeNode> intersected;

	float time = ofGetElapsedTimef();

	if (octree.intersect(ray, octree.root, intersected)) {
		bPointSelected = true;
		float closest = INT_MAX;
		int closestIndex = 0;
		for (int i = 0; i < intersected.size(); i++) {
			glm::vec3 vertex = octree.mesh.getVertex(intersected[i].points[0]);
			float distance = glm::length(vertex - currentCam->getPosition());
			if (closest > distance) {
				closest = distance;
				closestIndex = i;
			}
		}
		selectedPoint = octree.mesh.getVertex(intersected[closestIndex].points[0]);
		printf("Found intersect in %0.5fms\n", (ofGetElapsedTimef() - time) * 1000);
		//cout << selectedPoint << endl << endl;							// for basic testing optimization
	}
	else {
		bPointSelected = false;
		printf("No intersect found in %0.5fms\n", (ofGetElapsedTimef() - time) * 1000);
	}
}

bool ofApp::octreePointSelection() {
	ofMesh mesh = mars.getMesh(0);

	float nearestDistance = 0;
	bPointSelected = false;

	ofVec2f mouse(mouseX, mouseY);
	vector<ofVec3f> selection;

	for (int i = 0; i < selectedNode.points.size(); i++) {
		ofVec3f vert = mesh.getVertex(selectedNode.points.at(i));
		ofVec3f posScreen = currentCam->worldToScreen(vert);
		float distance = posScreen.distance(mouse);
		if (distance < selectionRange) {
			selection.push_back(vert);
			bPointSelected = true;
		}
	}

	if (bPointSelected) {
		float distance = 0;
		for (int i = 0; i < selection.size(); i++) {
			ofVec3f point = currentCam->worldToCamera(selection[i]);

			// In camera space, the camera is at (0,0,0), so distance from 
			// the camera is simply the length of the point vector
			//
			float curDist = point.length();

			if (i == 0 || curDist < distance) {
				distance = curDist;
				selectedPoint = selection[i];
			}
		}
	}

	return bPointSelected;
}

//
//  ScreenSpace Selection Method: 
//  This is not the octree method, but will give you an idea of comparison
//  of speed between octree and screenspace.
//
//  Select Target Point on Terrain by comparing distance of mouse to 
//  vertice points projected onto screenspace.
//  if a point is selected, return true, else return false;
//
bool ofApp::doPointSelection() {

	ofMesh mesh = mars.getMesh(0);
	int n = mesh.getNumVertices();
	float nearestDistance = 0;
	int nearestIndex = 0;

	bPointSelected = false;

	ofVec2f mouse(mouseX, mouseY);
	vector<ofVec3f> selection;

	// We check through the mesh vertices to see which ones
	// are "close" to the mouse point in screen space.  If we find 
	// points that are close, we store them in a vector (dynamic array)
	//
	for (int i = 0; i < n; i++) {
		ofVec3f vert = mesh.getVertex(i);
		ofVec3f posScreen = currentCam->worldToScreen(vert);
		float distance = posScreen.distance(mouse);
		if (distance < selectionRange) {
			selection.push_back(vert);
			bPointSelected = true;
		}
	}

	//  if we found selected points, we need to determine which
	//  one is closest to the eye (camera). That one is our selected target.
	//
	if (bPointSelected) {
		float distance = 0;
		for (int i = 0; i < selection.size(); i++) {
			ofVec3f point = currentCam->worldToCamera(selection[i]);

			// In camera space, the camera is at (0,0,0), so distance from 
			// the camera is simply the length of the point vector
			//
			float curDist = point.length();

			if (i == 0 || curDist < distance) {
				distance = curDist;
				selectedPoint = selection[i];
			}
		}
	}
	return bPointSelected;
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

void ofApp::initLightingAndMaterials() {

	static float ambient[] =
	{ .5f, .5f, .5, 1.0f };
	static float diffuse[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float position[] =
	{ 5.0, 5.0, 5.0, 0.0 };

	static float lmodel_ambient[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float lmodel_twoside[] =
	{ GL_TRUE };


	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, position);


	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
	/*																// remove comments for default lighting mode
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	//	glEnable(GL_LIGHT1);
	glShadeModel(GL_SMOOTH);
	*/
	/*
	keyLight.setup();
	rimLight.setup();
	fillLight.setup();
	keyLight.
	*/
	/*
	Lights.push_back(keyLight);
	Lights.push_back(fillLight);
	Lights.push_back(rimLight);
	*/
	/*
			Added lighting effects; experimented with 3 point lighting but stuck with just key light
			- Shahbaz Singh Mansahia
	*/
	keyLight.setup();
	keyLight.enable();
	keyLight.setAreaLight(1, 1);
	

	/*															// experimental light
	keyLight.setAmbientColor(ofFloatColor(3, 4, 12));
	keyLight.setDiffuseColor(ofFloatColor(2, 2, 143));
	keyLight.setSpecularColor(ofFloatColor(2, 2, 171));
	*/
																// BLUE LIGHT; KIND OF LIKE DIFFUSED MOONLIGHT
	/*
	keyLight.setAmbientColor(ofFloatColor(67, 82, 116));
	keyLight.setDiffuseColor(ofFloatColor(104, 116, 143));
	keyLight.setSpecularColor(ofFloatColor(142, 151, 171));
	*/

																	// WHITE LIGHT
	keyLight.setAmbientColor(ofFloatColor(1, 1, 1));
	keyLight.setDiffuseColor(ofFloatColor(1, 1, 1));
	keyLight.setSpecularColor(ofFloatColor(1, 1, 1));
	
	/*																// SHARP RED LIGHT
	keyLight.setAmbientColor(ofFloatColor(153, 0, 0));
	keyLight.setDiffuseColor(ofFloatColor(204, 0, 0));
	keyLight.setSpecularColor(ofFloatColor(255, 0, 0));
	*/
	keyLight.rotate(45, ofVec3f(0, 1, 0));
	keyLight.rotate(-45, ofVec3f(1, 0, 0));
	keyLight.setSpotlightCutOff(75);
	keyLight.setPosition(33, 15, 33);


	fillLight.setup();
	fillLight.enable();
	fillLight.setSpotlight();
	fillLight.setScale(.05);
	fillLight.setSpotlightCutOff(0);						//Turned off for better effect
	fillLight.setAttenuation(2, .001, .001);
	fillLight.setAmbientColor(ofFloatColor(1, 1, 1));
	fillLight.setDiffuseColor(ofFloatColor(1, 1, 1));
	fillLight.setSpecularColor(ofFloatColor(1, 1, 1));
	fillLight.rotate(-10, ofVec3f(1, 0, 0));
	fillLight.rotate(-45, ofVec3f(0, 1, 0));
	fillLight.setPosition(-5, 5.68, 5);

	rimLight.setup();
	rimLight.enable();
	rimLight.setSpotlight();
	rimLight.setScale(.05);
	rimLight.setSpotlightCutOff(0);				// Turned it off for a better effect
	rimLight.setAttenuation(.2, .001, .001);

	rimLight.setAmbientColor(ofFloatColor(1, 1, 1));
	rimLight.setDiffuseColor(ofFloatColor(1, 1, 1));
	rimLight.setSpecularColor(ofFloatColor(1, 1, 1));
	rimLight.rotate(180, ofVec3f(0, 1, 0));
	rimLight.setPosition(0, 5.55, -14);
	rimLight.rotate(30, ofVec3f(1, 0, 0));


	//				failed attempt at generating a lighting system by serializing ofLight objects using a vector
	//				- Shahbaz Singh Mansahia
	/*
	for (int i = 0; i < Lights.size(); ++i) {
		ofLight *temp = Lights.at(i);
		
		//temp->setup();
		//temp->enable();

		switch (i) {
		case 0:						// key light
			temp->setAreaLight(1, 1);
			temp->setAmbientColor(ofFloatColor(153, 0, 0));
			temp->setDiffuseColor(ofFloatColor(204, 0, 0));
			temp->setSpecularColor(ofFloatColor(255, 0, 0));
			break;
		case 1:						// fill light
			temp->setSpotlight();
			temp->setScale(0.05);
			temp->setSpotlightCutOff(100);
			temp->setAttenuation(0.2, 0.001, 0.001);

			temp->setAmbientColor(ofFloatColor(0, 0, 91));
			temp->setDiffuseColor(ofFloatColor(0, 0, 122));
			temp->setSpecularColor(ofFloatColor(0, 0, 153));
			
			break;
		case 2:						// rim light
			temp->setSpotlight();
			temp->setScale(0.05);
			temp->setSpotlightCutOff(100);
			temp->setAttenuation(0.2, 0.001, 0.001);

			temp->setAmbientColor(ofFloatColor(0, 0, 0));
			temp->setDiffuseColor(ofFloatColor(128, 128, 128));
			temp->setSpecularColor(ofFloatColor(64, 64, 64));
			break;
		default:
			cout << "Error in light vector init()" << endl;
		}

		
	}
	*/

}

void ofApp::savePicture() {
	ofImage picture;
	picture.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
	picture.save("screenshot.png");
	cout << "picture saved" << endl;
}

void ofApp::setCameraTarget() {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

	ofVec3f point;
	mouseIntersectPlane(ofVec3f(0, 0, 0), currentCam->getZAxis(), point);

	if (rover.loadModel(dragInfo.files[0])) {
		rover.setScaleNormalization(false);
		rover.setScale(.005, .005, .005);
		rover.setPosition(point.x, point.y, point.z);
		bRoverLoaded = true;
	}
	else cout << "Error: Can't load model" << dragInfo.files[0] << endl;
}

bool ofApp::mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point) {
	glm::vec3 mouse(mouseX, mouseY, 0);
	ofVec3f rayPoint = currentCam->screenToWorld(mouse);
	ofVec3f rayDir = rayPoint - currentCam->getPosition();
	rayDir.normalize();
	return (rayIntersectPlane(rayPoint, rayDir, planePoint, planeNorm, point));
}

// load vertex buffer in preparation for rendering
//
void ofApp::loadVbo() {
	if (emitter->sys->particles.size() < 1) return;

	vector<ofVec3f> sizes;
	vector<ofVec3f> points;
	for (int i = 0; i < emitter->sys->particles.size(); i++) {
		points.push_back(emitter->sys->particles[i].position);
		sizes.push_back(ofVec3f(radius));
	}
	// upload the data to the vbo
	//
	int total = (int)points.size();
	vbo.clear();
	vbo.setVertexData(&points[0], total, GL_STATIC_DRAW);
	vbo.setNormalData(&sizes[0], total, GL_STATIC_DRAW);
}