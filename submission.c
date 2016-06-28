//My variables:
// Game data:
float origin[3];
float sphereRadius;
float asteroidRadius;
float dangerZoneRadius;

// Motion data:
float velocity[3];
float position[3];
float ZRState[12];

// Movement data:
float mass;
float lastVelocity[3];
float acceleration[3];
float maxForce;
float lastForceMagnitude;


//The new file


//Declare any variables shared between functions here
bool isOn;
bool photosTaken[6];

void init(){
	//This function is called once when your code is first loaded.
	isOn = true;

	//IMPORTANT: make sure to set any variables that need an initial value.
	//Do not assume variables will be set to 0 automatically!
	
	// My new stuff:
	// Setting game data:
    origin[0] = 0.f;
    origin[1] = 0.f;
    origin[2] = 0.f;

    sphereRadius = 0.11f;
    asteroidRadius = 0.2f;
    dangerZoneRadius = 0.31f;

    // Setting movement data:
    mass = 5.248637919480517f;
    assignVectorToVector(lastVelocity, origin);
	maxForce = 0.05f;
    lastForceMagnitude = 0.f;
}

void loop(){
    DEBUG(("%i", isOn));
    movementStart();
	//This function is called once per second.  Use it to control the satellite.
	if (game.getNextFlare() >= 4 && game.getMemoryFilled() > 0) {
	    DEBUG(("[Mode: Flare upload]"));
	    upload();
	} else if (game.getNextFlare() < 4 && game.getNextFlare() != -1) {
	    DEBUG(("[Mode: Flare turn off]"));
	   game.turnOff();
       isOn = false;
	} else {
	    DEBUG(("[Mode: Normal]"));
	    //turn on if off
        if (!isOn) {
	        DEBUG(("[Sub-Mode: Turning on]"));
            game.turnOn();
            isOn = true;
        }
        //normal loop stuff
        if (game.getMemoryFilled() == game.getMemorySize()) {
	        DEBUG(("[Sub-Mode: Uploading]"));
            //upload photos if in photozone
            upload();
        } else {
	        DEBUG(("[Sub-Mode: Taking photo]"));
            //take photos
            takePhoto();
        }
	}
	movementEnd();
}

void basicMove(float targetPosition[3], float targetVelocityMagnitude) {
    // This function is designed to take a targetPosition, and a targetVelocityMagnitude to be at when it reaches that position.
    // If you want to stop there, set the targetVelocity to 0, and this function will simply use setPositionTarget.
    // If, however, you do not wish to stop, this will try to be as close as possible to the target velocity when it arrives.
    // If is entirely possible, that even with a reasonably low targetVelocityMagitude it will not be able to reach it.
    if (targetVelocityMagnitude == 0) {
        // Setting position target if you want to stop there.
        api.setPositionTarget(targetPosition);
    } else {
        // Setting up the required vectors.
        float targetVelocity[3];
        float requiredAcceleration[3];
        float requiredForce[3];
        // Initially just gets the direction of the required velocity, which is identical to the displacement between where you are and where you want to be.
        mathVecSubtract(targetVelocity, targetPosition, position, 3);
        if (!mathVecMagnitude(targetVelocity, 3)) {
            // If silly stuff is happening and this is zero, don't try to apply forces.
            api.setPositionTarget(targetPosition);
            return;
        }
        // Then it sets the magnitude of the targetVelocity to what you want it to be.
        setMagnitude(targetVelocity, targetVelocityMagnitude);
        // Then it works out how much you have to accelerate to make it to that this second.
        // a = (v - u) / t (But t is 1).
        mathVecSubtract(requiredAcceleration, targetVelocity, velocity, 3);
        // Then, F = ma
        multiplyVectorByScalar(requiredForce, requiredAcceleration, mass);
        // But, if it couldn't actually apply this much force, it reduces it to the maximum force it can apply.
        // (Found to be ~0.05N through experimentation).
        // While acctually applying more would be fine, it would make mass predictions wildly inaccurate.
        if (mathVecMagnitude(requiredForce, 3) > maxForce) {
            setMagnitude(requiredForce, maxForce);
        }
        // Here it actually sets the force.
        api.setForces(requiredForce);
        // And here it records it for use in updateMass()
        lastForceMagnitude = mathVecMagnitude(requiredForce, 3);
    }
}

void move(float targetPosition[3]) {
    // This function is designed to get from place to place, using basicMove(), but to account for the asteroid.
    float wayPoint[3]; // The current way point to be used.
    bool isWayPoint = false; // Is the point you end up going to a wayPoint?
    closestPointInIntervalToPoint(wayPoint, origin, position, targetPosition); // The wayPoint is the closes point to the asteroid.
    // But, if it is inside the raidus, it needs to be adjusted.
    int x = 0;
    while (mathVecMagnitude(wayPoint, 3) <= dangerZoneRadius + sphereRadius + 0.02f && x < 5) {
        x ++;
        isWayPoint = true;
        setMagnitude(wayPoint, dangerZoneRadius + sphereRadius + 0.02f);
        closestPointInIntervalToPoint(wayPoint, origin, position, wayPoint); // Setting to check if there is a point along this line.
    }
    // If the path passes through the origin for instance, the wayPoint will be null
    // Don't use paths which pass through the origin.
    if (isWayPoint && mathVecMagnitude(wayPoint, 3)) {
        basicMove(wayPoint, 0.04f);
    } else {
        basicMove(targetPosition, 0.0f);
    }
}

void updateMass() {
    // This function takes the force applied last second, and the acceleration experiences since then, and uses it to predict your mass.
    // THIS FUNCTION WILL BE INCORRECT IF BLACK BOX FUNCTIONALITY HAS OVERRIDDEN MOTION!
    // e.g. Run out of fuel, hit asteroid, hit other sphere.
    float accelerationMagnitude = mathVecMagnitude(acceleration, 3);
    // If either lastForceMagnitude or accelerationMagnitude is zero, this function should not run.
    // If lastForceMagnitude is zero, a force was not applied last frame, and as such, the equation would be invalid.
    // Acceleration magntidue should never be zero while lastForceMagnitude is not zero, but weird things happen, and it is not worth the risk.
    // As such, if either or them are zero, them multiplied together will be zero, and this will not run.
    if (lastForceMagnitude * accelerationMagnitude) {
        // F = ma
        // a = F / m
        float newMass = lastForceMagnitude / accelerationMagnitude;
        // Takes the average of the mass predicted by this, and what you thought the mass was before,
        // to prevent anomolous [or black box] motion from having too large an effect.
        mass = (mass + newMass) / 2;
    }
}

// This must be called at the start of every loop.
void movementStart() {
    // Setting motion data:
    api.getMyZRState(ZRState);
    //api.getOtherZRState(OZRState);
    for (int i = 0; i < 3; i++) {
        position[i] = ZRState[i];
        velocity[i] = ZRState[i + 3];
    }
    mathVecSubtract(acceleration, velocity, lastVelocity, 3);
    updateMass();
    lastForceMagnitude = 0.f; // Very important in mass calculation [Positioning in loop included].
}

// This must be called at the end of every loop.
void movementEnd() {
    // More setting motion data:
    assignVectorToVector(lastVelocity, velocity);
}

void nextPhotoPos(float position[3]) {
    unsigned short int i = nextPhotoID();
    game.getPOILoc(position, i % 3);
    setMagnitude(position, (i < 3) ? 0.475 : 0.385);
}

int nextPhotoID() {
    for(unsigned short int i = 0; i < 6; i++) {
        if(!photosTaken[i]) {
            return i;
        }
    }
    return 0;
}

void takePhoto() {
    //DEBUG(("[Time: %i][TAKE PHOTO CALLED]", api.getTime()));
    float poiPos[3];
    float toGoTo[3];
    game.getPOILoc(poiPos, nextPhotoID());
    nextPhotoPos(toGoTo);
    move(toGoTo);
    lookAt(poiPos, toGoTo);
    DEBUG(("[atApproxLocation: %i][approxFacePoint: %i]", atApproxLocation(toGoTo), approxFacePoint(poiPos)));
    if(atApproxLocation(toGoTo) && approxFacePoint(poiPos)) {
        //DEBUG(("[TAKING PHOTO]"));
        game.takePic(nextPhotoID());
    }
}

///UTILITY
// bool inShadow (float pointx, float pointy, float pointz) {
// 	//if on shadow side of the asteroid and within yz radius
// 	if (pointx > 0 && sqrtf(mathSquare(pointy) + mathSquare(pointz)) <= 0.2) {
// 		return true;
// 	} else {
// 		return false;
// 	}
// }

void setMagnitude(float vector[3], float magnitude) {
	// Normalising a vector is to make its magnitude one, but preserve its direction.
	// As such, if you normalise it, its magnitude is now one, and so if you then multiply it by a particular amount, that will be the magnitude.
    mathVecNormalize(vector, 3);
    multiplyVectorByScalar(vector, vector, magnitude);
}

void multiplyVectorByScalar(float final[3], float vector[3], float scalar) {
	// Multiplying each component of a vector will multiply the magnitude by that much.
    for (int i = 0; i < 3; i++) {
        final[i] = vector[i] * scalar;
    }
}

void lookAt(float lookAt[3], float lookFrom[3]){
    float direction[3];
    mathVecSubtract(direction, lookAt, lookFrom, 3);
    mathVecNormalize(direction, 3);
    api.setAttitudeTarget(direction);
}

bool atApproxLocation(float vector[3]){
    float displacement[3];
    mathVecSubtract(displacement, vector, position, 3);
    DEBUG(("[Distance: %f]", mathVecMagnitude(displacement, 3)));
    return (
        mathVecMagnitude(displacement, 3) <= 0.02f
    );
}

bool approxFacePoint(float vector[3]){
    float vectorToPoint[3];
    float attitude[3];
    for(int i = 0; i < 3; i++){
        attitude[i] = ZRState[3 + i];
    }
    mathVecSubtract(vectorToPoint, vector, ZRState, 3);
    mathVecNormalize(vectorToPoint, 3);
    return (
        angleBetweenVectors(attitude, vectorToPoint) <= 0.5f
    );
}

float angleBetweenVectors(float vector1[3], float vector2[3]){
    float angle = 0;
    angle = acosf((mathVecInner(vector1, vector2, 3))/(fabsf(mathVecMagnitude(vector1, 3)) * fabsf(mathVecMagnitude(vector2, 3))));
    angle = angle * (3.141593 / 180);
    return angle;
}

void upload(){
    float target[3];
    assignVectorToVector(target, position);
    setMagnitude(target, 0.55);
    move(target);
    if(atApproxLocation(target)){
        game.uploadPic();
    }
}

float distanceFromPointToInterval(float point[3], float lineStart[3], float lineEnd[3]) {
	float q[3]; // The closest point in the interval to point.
	closestPointInIntervalToPoint(q, point, lineStart, lineEnd);
	float displacement[3];
	mathVecSubtract(displacement, point, q, 3);
	return mathVecMagnitude(displacement, 3);
}

void closestPointInIntervalToPoint(float pointToFill[3], float point[3], float lineStart[3], float lineEnd[3]) {
	// See documentation/distanceFromPointToLine.txt to see what I am doing here.
	float n[3]; // The normal vector of the plane, the line passing through lineStart and lineEnd.
	mathVecSubtract(n, lineEnd, lineStart, 3);
	float a[3]; // Point. A point on the place, sorry to do this, but I wanted nice inputs but also couldn't stand the maths with long names.
	for (int i = 0; i < 3; i++) {
		a[i] = point[i];
	}
	float q[3]; // The point of intersection of the plane and the line. <-- What we are looking for.
	float b[3]; // Any point on the line.
	for (int i = 0; i < 3; i++) {
		b[i] = lineStart[i];
	}
	// You really need to read the documentation for this (and the stuff below the if statement):
	float t = (n[0]*a[0] + n[1]*a[1] + n[2]*a[2] - n[0]*b[0] - n[1]*b[1] - n[2]*b[2]) / (mathSquare(n[0]) + mathSquare(n[1]) + mathSquare(n[2]));
	// But, if t < 0 or t > 1, the point q is not between lineStart and lineEnd.
	// Which I believe means that the closest point on the line to the point must be one of the ends.
	if (t < 0 || t > 1) {
		float displacement1[3];
		mathVecSubtract(displacement1, point, lineStart, 3);
		float displacement2[3];
		mathVecSubtract(displacement2, point, lineEnd, 3);
		if (mathVecMagnitude(displacement1, 3) <= mathVecMagnitude(displacement2, 3)) {
			for (int i = 0; i < 3; i++) {
				pointToFill[i] = lineStart[i];
			}
			return;
		}
		for (int i = 0; i < 3; i++) {
			pointToFill[i] = lineEnd[i];
		}
		return;
	}
	// But, if the point is between them...
	float tn[3];
	multiplyVectorByScalar(tn, n, t);
	mathVecAdd(q, b, tn, 3);
	for (int i = 0; i < 3; i++) {
		pointToFill[i] = q[i];
	}
}
// Sets one vector to equal another.
void assignVectorToVector(float vectorToChange[3], float vectorToChangeOtherVectorInto[3]) {
	for (int i = 0; i < 3; i++) {
		vectorToChange[i] = vectorToChangeOtherVectorInto[i];
	}
}
