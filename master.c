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

float nextPhotoPos() {
    float position[3];
    unsigned short int i = nextPhotoID();
    game.getPOILoc(position, i % 3);
    setMagnitude(position, (i < 3) ? 0.475 : 0.385);
    return position;
}

int nextPhotoID() {
    for(unsigned short int i = 0; i < 6; i++) {
        if(!photosTaken[i]) {
            return i;
        }
    }
}

void takePhoto() {
    float pos[3] = getPOILoc(pos, nextPhotoID())
    lookAt(pos, nextPhotoPos());
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
