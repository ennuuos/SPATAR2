//The new file


//Declare any variables shared between functions here

bool photosTaken[6];






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
