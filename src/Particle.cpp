// Aaron Warren - March 31 2019
// Only changed dt to divide by framerate instead

#include "Particle.h"


Particle::Particle() {

	// initialize particle with some reasonable values first;
	//
	velocity.set(0, 0, 0);
	acceleration.set(0, 0, 0);
	position.set(0, 0, 0);
	forces.set(0, 0, 0);
	lifespan = 5;
	birthtime = 0;
	radius = .1;
	damping = .99;
	mass = 1;
	color = ofColor::aquamarine;
}

void Particle::draw() {
	ofSetColor(color);
//	ofSetColor(ofMap(age(), 0, lifespan, 255, 10), 0, 0);
	ofDrawSphere(position, radius);
}

//Removed dt and made it so that the various parts only got divided by the framerate instead
void Particle::integrate() {

	// check for 0 framerate to avoid divide errors
	//
	float framerate = ofGetFrameRate();
	if (framerate < 1.0) return;

	// interval for this step
	//
	//float dt = 1.0 / framerate;

	// update position based on velocity
	//
	position += (velocity / framerate);

	// update acceleration with accumulated paritcles forces
	// remember :  (f = ma) OR (a = 1/m * f)
	//
	ofVec3f accel = acceleration;    // start with any acceleration already on the particle
	accel += (forces * (1.0 / mass));
	velocity += (accel / framerate);

	// add a little damping for good measure
	//
	velocity *= damping;

	// clear forces on particle (they get re-added each step)
	//
	forces.set(0, 0, 0);
}

//  return age in seconds
//
float Particle::age() {
	return (ofGetElapsedTimeMillis() - birthtime)/1000.0;
}


