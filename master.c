//The new file


//Declare any variables shared between functions here
float ZRState[12];
bool isOn;
bool photosTaken[6];

void init(){
	//This function is called once when your code is first loaded.
	isOn = true;

	//IMPORTANT: make sure to set any variables that need an initial value.
	//Do not assume variables will be set to 0 automatically!
}

void loop(){
    //update self position
    api.getMyZRState(ZRState);
	//This function is called once per second.  Use it to control the satellite.
	if(game.getNextFlare() == -1) {
	    //turn on if off
        if (!isOn) {
            game.turnOn();
            isOn = true;
        }
        //normal loop stuff
        if (game.getMemoryFilled() == game.getMemorySize()) {
            //upload photos if in photozone
            upload();
        } else {
            //take photos
            takePhoto();
        }
	}
    else {
        //emergency shutdown
        if(game.getNextFlare() < 4) {
            if (!inShadow(ZRState[0], ZRState[1], ZRState[2])) {
                game.turnOff();
                isOn = false;
            }
        }
        //prepare for flare
        if (game.getMemoryFilled() > 0) {
            upload();
        }
	}
}

void move(float targetPos[3]){
    float destPos[3];
    if ((ZRState[0] > 0 && targetPos[0] < 0) || (ZRState[0] < 0 && targetPos[0] > 0)){//avoid asteroid
            destPos[0] = 0;
            destPos[1] = ZRState[1];
            destPos[2] = ZRState[2];
        setMagnitude(destPos,0.6);
        if(destPos[1] == 0 && destPos[2] == 0){
            destPos[1] = 0.6;
        }
    }
    else{
        memcpy(destPos,targetPos,sizeof(float)*3);
    }
    float targetVel[3];
    mathVecSubtract(targetVel,destPos,ZRState,3) ;
    if (mathVecMagnitude(targetVel, 3) > 0.35)
        api.setVelocityTarget(targetVel);
    else
        api.setPositionTarget(destPos);
}

float nextPhotoPos() {
    float position[3];
    unsigned short int i = nextPhotoID();
    game.getPOILoc(position, i % 3);
    setMagnitude(position, (i < 3) ? 0.475 : 0.385);
    return *position;
}

int nextPhotoID() {
    for(unsigned short int i = 0; i < 6; i++) {
        if(!photosTaken[i]) {
            return i;
        }
    }
}

void takePhoto() {
    float pos[3];
    game.getPOILoc(pos, nextPhotoID());
    move(pos);
    lookAt(pos, nextPhotoPos());
    
    if(atAppoxLocation(pos) && approxFacePoint(pos)) {
        game.takePic(nextPhotoID());
    }
}


///UTILITY
bool inShadow (float pointx, float pointy, float pointz) {
	//if on shadow side of the asteroid and within yz radius
	if (pointx > 0 && sqrtf(mathSquare(pointy) + mathSquare(pointz)) <= 0.2) {
		return true;
	} else {
		return false;
	}
}

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



void lookAt(float lookAt[3], float pointToLook[3]){
    float direction[3];
    mathVecSubtract(vecBetween, pointToLook, lookAt, 3);
    mathVecNormalize(direction, 3);
    api.setAttitudeTarget(direction);
}

void setMagnitude(float vector[3], float magnitude){
    mathVecNormalize(vector, 3);
    multiplyVectorByScalar(vector, vector, magnitude);
}

bool atApproxLocation(float vector[3]){
    float displacement[3];
    mathVecSubtract(displacement, vector, ZRState, 3);
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
    setMagnitude(target, 0.55);
    move(target);
    if(atApproxLocation(target)){
        game.uploadPic();
    }
}

