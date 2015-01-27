#include <stdio.h>
#include <GL/gl.h>		   // Open Graphics Library (OpenGL) header
#include <GL/glut.h>	   // The GL Utility Toolkit (GLUT) Header
#include <math.h>
#include <vector>
#include <cmath>
#include <time.h>
#include <string.h>

#define KEY_ESCAPE 27
#define	ENTER 13
#define SPACE 32
#define PI 3.141592//653589793238462643383279502884197169399375105820974944592307816406286
#define FPS 25				//number of frames rendered persecond

//defining constants for the frame_type
#define SELECTION_SCREEN 	1
#define PLAYING_SCREEN		2
#define PAUSE_SCREEN		3
#define LEVEL_CHANGE_SCREEN	4
#define SETTING_SCREEN		5
#define INSTRUCTION_SCREEN	6

GLuint ObjList;

typedef struct {
    int width;
	int height;
	char* title;

	float field_of_view_angle;
	float z_near;
	float z_far;
} glutWindow;
glutWindow win;

typedef struct{ 					//All these variables have to be changed for each car
	int ID;
	float maxTurn;
	float maxSpeed;
	float acc;
	float turnControl;
	char* objFileName;
} car;
car myCar;

typedef struct {
	int id;
	int numCheckpoints;
	float friction;
	float start_x;
	float start_z;
	float X_min_world;
	float Z_min_world;
	float X_max_world;
	float Z_max_world;
	time_t maxTime[3];
} raceTrack;
raceTrack currTrack;

bool* activeKey = new bool[260];
bool space_pressed_to_start_race = false;
bool music_on = false;
bool sound_effects_on = false;
bool third_person_view = false;
bool round_complete;
bool timer_stopped;

int frame_type;
int selector;
int level;					//level 0 => learner, 1=> Decent Driver, 2=> Professional
int v;

float speed=0;
float turn=0;
float initialx =0;
float initialz=0;
float rotY=0;

time_t userTime;
time_t startTime;
time_t finalTime;

/*
*	The Sphere!!
*/

// #define DRAW_WIREFRAME 

class SolidSphere
{
protected:
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> texcoords;
    std::vector<GLushort> indices;

public:
    SolidSphere(float radius, unsigned int rings, unsigned int sectors)
    {
        float const R = 1./(float)(rings-1);
        float const S = 1./(float)(sectors-1);
        int r, s;

        vertices.resize(rings * sectors * 3);
        normals.resize(rings * sectors * 3);
        texcoords.resize(rings * sectors * 2);
        std::vector<GLfloat>::iterator v = vertices.begin();
        std::vector<GLfloat>::iterator n = normals.begin();
        std::vector<GLfloat>::iterator t = texcoords.begin();
        for(r = 0; r < rings; r++) for(s = 0; s < sectors; s++) {
                float const y = sin( -M_PI_2 + M_PI * r * R );
                float const x = cos(2*M_PI * s * S) * sin( M_PI * r * R );
                float const z = sin(2*M_PI * s * S) * sin( M_PI * r * R );

                *t++ = s*S;
                *t++ = r*R;

                *v++ = x * radius;
                *v++ = y * radius;
                *v++ = z * radius;

                *n++ = x;
                *n++ = y;
                *n++ = z;
        }

        indices.resize(rings * sectors * 4);
        std::vector<GLushort>::iterator i = indices.begin();
        for(r = 0; r < rings-1; r++) for(s = 0; s < sectors-1; s++) {
                *i++ = r * sectors + s;
                *i++ = r * sectors + (s+1);
                *i++ = (r+1) * sectors + (s+1);
                *i++ = (r+1) * sectors + s;
        }
    }

    void draw(GLfloat x, GLfloat y, GLfloat z)
    {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslatef(x,y,z);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
        glNormalPointer(GL_FLOAT, 0, &normals[0]);
        glTexCoordPointer(2, GL_FLOAT, 0, &texcoords[0]);
        glDrawElements(GL_QUADS, indices.size(), GL_UNSIGNED_SHORT, &indices[0]);
        glPopMatrix();
    }
};
SolidSphere sphere(1, 12, 24);


//Initializing variables for texturing
GLuint texture[4];
struct Image {
    unsigned long sizeX;
    unsigned long sizeY;
    char *data;
};
typedef struct Image Image;
#define checkImageWidth 64
#define checkImageHeight 64
GLubyte checkImage[checkImageWidth][checkImageHeight][3];
void makeCheckImage(void){
    int i, j, c;
    for (i = 0; i < checkImageWidth; i++) {
        for (j = 0; j < checkImageHeight; j++) {
            c = ((((i&0x8)==0)^((j&0x8)==0)))*255;
            checkImage[i][j][0] = (GLubyte) c;
            checkImage[i][j][1] = (GLubyte) c;
            checkImage[i][j][2] = (GLubyte) c;
        }
    }
}
int ImageLoad(char *filename, Image *image) {
    FILE *file;
    unsigned long size; // size of the image in bytes.
    unsigned long i; // standard counter.
    unsigned short int planes; // number of planes in image (must be 1)
    unsigned short int bpp; // number of bits per pixel (must be 24)
    char temp; // temporary color storage for bgr-rgb conversion.
    // make sure the file is there.
    if ((file = fopen(filename, "rb"))==NULL){
        printf("File Not Found : %s\n",filename);
        return 0;
    }
    // seek through the bmp header, up to the width/height:
    fseek(file, 18, SEEK_CUR);
    // read the width
    if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
        printf("Error reading width from %s.\n", filename);
        return 0;
    }
    //printf("Width of %s: %lu\n", filename, image->sizeX);
    // read the height
    if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {
        printf("Error reading height from %s.\n", filename);
        return 0;
    }
    //printf("Height of %s: %lu\n", filename, image->sizeY);
    // calculate the size (assuming 24 bits or 3 bytes per pixel).
    size = image->sizeX * image->sizeY * 3;
    // read the planes
    if ((fread(&planes, 2, 1, file)) != 1) {
        printf("Error reading planes from %s.\n", filename);
        return 0;
    }
    if (planes != 1) {
        printf("Planes from %s is not 1: %u\n", filename, planes);
        return 0;
    }
    // read the bitsperpixel
    if ((i = fread(&bpp, 2, 1, file)) != 1) {
        printf("Error reading bpp from %s.\n", filename);
        return 0;
    }
    if (bpp != 24) {
        printf("Bpp from %s is not 24: %u\n", filename, bpp);
        return 0;
    }
    // seek past the rest of the bitmap header.
    fseek(file, 24, SEEK_CUR);
    // read the data.
    image->data = (char *) malloc(size);
    if (image->data == NULL) {
        printf("Error allocating memory for color-corrected image data");
        return 0;
    }
    if ((i = fread(image->data, size, 1, file)) != 1) {
        printf("Error reading image data from %s.\n", filename);
        return 0;
    }
    for (i=0;i<size;i+=3) { // reverse all of the colors. (bgr -> rgb)
        temp = image->data[i];
        image->data[i] = image->data[i+2];
        image->data[i+2] = temp;
    }
    // we're done.
    return 1;
}

void loadTexture(){
    Image *image1;
    // allocate space for texture
    image1 = (Image *) malloc(sizeof(Image));
    if (image1 == NULL) {
        printf("Error allocating space for image");
        exit(0);
    }
    if (!ImageLoad("media/road1.bmp", image1)) {
        exit(1);
    }
	// if(image1 == NULL){
	//     printf("Image was not returned from loadTexture\n");
	//     exit(0);
	// }
	makeCheckImage();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// Create Texture
	glGenTextures(1, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); //scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); //scale linearly when image smalled than texture
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);

    if (!ImageLoad("media/grass02.bmp", image1)) {
        exit(1);
    }
	// if(image1 == NULL){
	//     printf("Image was not returned from loadTexture\n");
	//     exit(0);
	// }
	makeCheckImage();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// Create Texture
	glGenTextures(2, &texture[1]);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); //scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); //scale linearly when image smalled than texture
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);

	if (!ImageLoad("media/night-sky.bmp", image1)) {
        exit(1);
    }
	// if(image1 == NULL){
	//     printf("Image was not returned from loadTexture\n");
	//     exit(0);
	// }
	makeCheckImage();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// Create Texture
	glGenTextures(3, &texture[2]);
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); //scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); //scale linearly when image smalled than texture
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);

	if (!ImageLoad("media/Brick01.bmp", image1)) {
        exit(1);
    }
	// if(image1 == NULL){
	//     printf("Image was not returned from loadTexture\n");
	//     exit(0);
	// }
	makeCheckImage();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// Create Texture
	glGenTextures(3, &texture[3]);
	glBindTexture(GL_TEXTURE_2D, texture[3]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); //scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); //scale linearly when image smalled than texture
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);

//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

}

/*
* we do not need the object
*/
void loadObj(char *fname)
{
	FILE *fp;
	int read;
	GLfloat x, y, z, xprev1=0, zprev1=0, xprev2=0, zprev2=0;
	char ch;
	ObjList=glGenLists(1);
	fp=fopen(fname,"r");
	if (!fp)
	{
		printf("can't open file %s\n", fname);
		exit(1);
	}
	glPointSize(2.0);
	float flag1=0, flag2=0;
	glNewList(ObjList, GL_COMPILE);
	{
		glPushMatrix();

		glRotatef(180, 0.0f, 1.0f, 0.0f);
		glTranslatef(0.0f, 0.0f, 10.0f);

		glTranslatef(10.0f, 0.0f, -2.0f);
		// glTranslatef(initialx-sin(rotY),1,initialz-cos(rotY));
		// glTranslatef(-initialx+sin(rotY),0,-initialz+cos(rotY));
		// glRotatef(180, 0.0f, -1.0f, 0.0f);
		glBegin(GL_POINTS);
		while(!(feof(fp)))
		{
			read=fscanf(fp,"%c %f %f %f",&ch,&x,&y,&z);
			if(read==4&&ch=='v')
			{
				glTexCoord2f(flag1, flag2);
				glVertex3f(x,y,z);
				if (flag2==0)
				{
					flag2 =1;
				}
				else {
					flag2 = 0;
				}
				if (flag1==0)
				{
					flag1 =1;
				}
				else {
					flag1 = 0;
				}
			}
		}
		glEnd();
		glPopMatrix();
	}
	glEndList();
	fclose(fp);
}

/*void StartKeyOperations(){
	if (activeKey[256])
	{
		selector--;
	}
	else if (activeKey[257])
	{
		selector++;
	}
}*/

void keyOperations() {
	if (activeKey['w'] || activeKey['W']){
		if (speed > (-1)*myCar.maxSpeed)
		{
			// printf("the speed of the car is not able to go beyond %f, where limit is %s\n", speed, glGetString(GL_VERSION));
			speed -= myCar.acc;
		}
	}
	if (activeKey['s'] || activeKey['S']){
		if (speed < myCar.maxSpeed)
		{
			speed += myCar.acc;
		}
	}
	if (activeKey['a'] || activeKey['A']){
		if (turn > (-1)*myCar.maxTurn)
		{
			turn -= myCar.turnControl;
		}
	}
	if (activeKey['d'] || activeKey['D']){
		if (turn < myCar.maxTurn)
		{
			turn += myCar.turnControl;
		}
	}
	// printf("x-coord = %f, and z-coord = %f\n => rotation = %f\n", initialx, initialz, rotY);
	speed -= speed*(currTrack.friction);
	rotY = rotY*180/PI; 
	rotY += (turn*speed)/FPS;
	if (rotY >= 180)
	{
		rotY -= 360;
	}
	else if (rotY < -180)
	{
		rotY += 360;
	}
	if ((speed != 0)&& (turn != 0))			//automatic reduction of turn
	{
		if (turn*speed > 0)
		{
			turn -= 1*speed;
		}
		else {
			turn += 1*speed;
		}
	}
	rotY = (rotY*PI)/180;
	//Restricting the movement of the car, so that it does not go out of the frame ...
	if (initialz > currTrack.Z_max_world)
	{
		speed = 0;
		initialz = currTrack.Z_max_world;
	}
	if (initialz < currTrack.Z_min_world)
	{
		speed =0;
		initialz = currTrack.Z_min_world;
	}
	if (initialx > currTrack.X_max_world + 5.0)
	{
		speed = 0;
		initialx = currTrack.X_max_world + 5.0;
	}
	else if (initialx < currTrack.X_min_world + 5.0)
	{
		speed =0;
		initialx = currTrack.X_min_world + 5.0;
	}
	initialx += speed*sin(rotY);
	initialz += speed*cos(rotY);
}

/*
*	The Display of the game play
*/
void display() 
{
	int lenghOfQuote;
	char quote[4][80];
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		     // Clear Screen and Depth Buffer
	glLoadIdentity();
	glTranslatef(0.0f,0.0f,-10.0f);			
	keyOperations();
	// glRotatef(initialy, 0.0f, 1.0f, 0.0f);
	if (space_pressed_to_start_race)
	{
		gluLookAt(initialx,1,initialz,
			initialx-sin(rotY),1,initialz-cos(rotY),
			0,1,0);
	}
	else {
		gluLookAt((currTrack.X_max_world + currTrack.X_min_world)/2 + 150,100,(currTrack.Z_max_world + currTrack.Z_min_world)/2+200,
			(currTrack.X_max_world + currTrack.X_min_world)/2,1,(currTrack.Z_max_world + currTrack.Z_min_world)/2,
			0,1,0);
		// glScalef(0.01, 0.01, 0.01)
	}
	// glTranslatef(0.0f, 0.0f, initialz);
	
	glPushMatrix();
	glTranslatef(6,-2.00,5);
	if (space_pressed_to_start_race)
	{
		/*
		* drawing the enclosing box
		*/
		glColor3f(0.5f, 0.8f, 0.5f);
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glBegin(GL_QUADS);
		for (int i = currTrack.Z_min_world; i < currTrack.Z_max_world; i+=10.0f)
		{
			for (int j = currTrack.X_min_world; j < currTrack.X_max_world; j+=10.0f)
			{
				glTexCoord2f(0.0, 0.0);
					glVertex3f(j,-0.0001f, i+10.0f);
				glTexCoord2f(0.0, 1.0);
					glVertex3f(j,-0.0001f,i);
				glTexCoord2f(1.0, 1.0);
					glVertex3f(j+10.0f,-0.0001f,i);
				glTexCoord2f(1.0, 0.0);
					glVertex3f(j+10.0f,-0.0001f, i+10.0f);			
			}
		}
		glEnd();
		glColor3f(0.75f, 0.75f, 0.75f);
		glBindTexture(GL_TEXTURE_2D, texture[2]);
		// DrawEnclosingSphere(300, 10, 10);
		glBegin(GL_QUADS);
		for (int i = currTrack.Z_min_world; i < currTrack.Z_max_world; i+=25.0f)
		{
			for (int j = currTrack.X_min_world; j < currTrack.X_max_world; j+=25.0f)
			{
				glTexCoord2f(0.0, 0.0);
					glVertex3f(j,25.0f, i+25.0f);
				glTexCoord2f(0.0, 1.0);
					glVertex3f(j,25.0f,i);
				glTexCoord2f(1.0, 1.0);
					glVertex3f(j+25.0f,25.0f,i);
				glTexCoord2f(1.0, 0.0);
					glVertex3f(j+25.0f,25.0f, i+25.0f);			
			}
		}
		glEnd();

		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, texture[3]);
		// DrawEnclosingSphere(300, 10, 10);
		glBegin(GL_QUADS);
		for (int i = currTrack.X_min_world; i < currTrack.X_max_world; i+=25.0f)
		{
			glTexCoord2f(0.0, 0.0);
				glVertex3f(i, 0.0f, currTrack.Z_min_world);
			glTexCoord2f(1.0, 0.0);
				glVertex3f(i + 25.0f, 0.0f, currTrack.Z_min_world);
			glTexCoord2f(1.0, 1.0);
				glVertex3f(i + 25.0f, 25.0f, currTrack.Z_min_world);
			glTexCoord2f(0.0, 1.0);
				glVertex3f(i, 25.0f, currTrack.Z_min_world);
		}
		for (int i = currTrack.X_min_world; i < currTrack.X_max_world; i+=25.0f)
		{
			glTexCoord2f(0.0, 0.0);
				glVertex3f(i, 0.0f, currTrack.Z_max_world);
			glTexCoord2f(1.0, 0.0);
				glVertex3f(i + 25.0f, 0.0f, currTrack.Z_max_world);
			glTexCoord2f(1.0, 1.0);
				glVertex3f(i + 25.0f, 25.0f, currTrack.Z_max_world);
			glTexCoord2f(0.0, 1.0);
				glVertex3f(i, 25.0f, currTrack.Z_max_world);
		}
			for (int i = currTrack.Z_min_world; i < currTrack.Z_max_world; i+=25.0f)
		{
			glTexCoord2f(0.0, 0.0);
				glVertex3f(currTrack.X_min_world, 0.0f,i);
			glTexCoord2f(1.0, 0.0);
				glVertex3f(currTrack.X_min_world, 0.0f,i+25.0f);
			glTexCoord2f(1.0, 1.0);
				glVertex3f(currTrack.X_min_world, 25.0f,i+25.0f);
			glTexCoord2f(0.0, 1.0);
				glVertex3f(currTrack.X_min_world, 25.0f,i);
		}
			for (int i = currTrack.Z_min_world; i < currTrack.Z_max_world; i+=25.0f)
		{
			glTexCoord2f(0.0, 0.0);
				glVertex3f(currTrack.X_max_world, 0.0f,i);
			glTexCoord2f(1.0, 0.0);
				glVertex3f(currTrack.X_max_world, 0.0f,i+25.0f);
			glTexCoord2f(1.0, 1.0);
				glVertex3f(currTrack.X_max_world, 25.0f,i+25.0f);
			glTexCoord2f(0.0, 1.0);
				glVertex3f(currTrack.X_max_world, 25.0f,i);
		}
		glEnd();
	}

	/*
	* drawing the track
	*/
	glColor3f(3.0f,3.0f,3.0f);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	if (currTrack.id == 2) {
		glBegin(GL_QUADS);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( -69.6982f, 0.0000f, 146.3044f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( -44.6982f, 0.0000f, 146.3044f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( -44.6982f, 0.0000f, -3.6956f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( -69.6982f, 0.0000f, -3.6956f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( -69.6982f, 0.0000f, -3.6956f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( -44.6982f, 0.0000f, -3.6956f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( -41.5202f, 0.0000f, -20.8055f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( -65.0125f, 0.0000f, -29.3560f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( -65.0125f, 0.0000f, -29.3560f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( -41.5202f, 0.0000f, -20.8055f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( -32.8378f, 0.0000f, -35.8439f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( -51.9889f, 0.0000f, -51.9136f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( -51.9889f, 0.0000f, -51.9136f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( -32.8378f, 0.0000f, -35.8439f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( -19.5355f, 0.0000f, -47.0058f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( -32.0355f, 0.0000f, -68.6564f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( -32.0355f, 0.0000f, -68.6564f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( -19.5355f, 0.0000f, -47.0058f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( -3.2180f, 0.0000f, -52.9449f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( -7.5592f, 0.0000f, -77.5651f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( -7.5592f, 0.0000f, -77.5651f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( -3.2180f, 0.0000f, -52.9449f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 14.1469f, 0.0000f, -52.9449f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 18.4881f, 0.0000f, -77.5651f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 18.4881f, 0.0000f, -77.5651f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 14.1469f, 0.0000f, -52.9449f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 30.4644f, 0.0000f, -47.0058f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 42.9644f, 0.0000f, -68.6564f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 42.9644f, 0.0000f, -68.6564f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 30.4644f, 0.0000f, -47.0058f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 43.7667f, 0.0000f, -35.8439f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 62.9178f, 0.0000f, -51.9136f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 62.9178f, 0.0000f, -51.9136f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 43.7667f, 0.0000f, -35.8439f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 52.4491f, 0.0000f, -20.8055f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 75.9414f, 0.0000f, -29.3560f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 75.9414f, 0.0000f, -29.3560f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 52.4491f, 0.0000f, -20.8055f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 55.4644f, 0.0000f, -3.7045f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 80.4644f, 0.0000f, -3.7045f);

			glTexCoord2f(0.0, 0.0);
			 glVertex3f( 80.4644f, 0.0000f, -3.7045f);
			glTexCoord2f(1.0, 0.0);
			 glVertex3f( 55.4644f, 0.0000f, -3.7045f);
			glTexCoord2f(1.0, 1.0);
			 glVertex3f( 55.4316f, 0.0000f, 146.3044f);
			glTexCoord2f(0.0, 1.0);
			 glVertex3f( 80.4316f, 0.0000f, 146.3044f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 80.4316f, 0.0000f, 146.3044f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 55.4316f, 0.0000f, 146.3044f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 52.4491f, 0.0000f, 163.3138f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 75.9414f, 0.0000f, 171.8643f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 75.9414f, 0.0000f, 171.8643f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 52.4491f, 0.0000f, 163.3138f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 43.7667f, 0.0000f, 178.3522f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 62.9178f, 0.0000f, 194.4219f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 62.9178f, 0.0000f, 194.4219f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 43.7667f, 0.0000f, 178.3522f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 30.4644f, 0.0000f, 189.5141f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 42.9644f, 0.0000f, 211.1647f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 42.9644f, 0.0000f, 211.1647f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 30.4644f, 0.0000f, 189.5141f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 14.1468f, 0.0000f, 195.4532f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 18.4880f, 0.0000f, 220.0734f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 18.4880f, 0.0000f, 220.0734f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 14.1468f, 0.0000f, 195.4532f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( -3.2180f, 0.0000f, 195.4532f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( -7.5592f, 0.0000f, 220.0734f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( -7.5592f, 0.0000f, 220.0734f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( -3.2180f, 0.0000f, 195.4532f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( -19.5356f, 0.0000f, 189.5141f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( -32.0356f, 0.0000f, 211.1647f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( -32.0356f, 0.0000f, 211.1647f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( -19.5356f, 0.0000f, 189.5141f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( -32.8378f, 0.0000f, 178.3522f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( -51.9889f, 0.0000f, 194.4219f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( -51.9889f, 0.0000f, 194.4219f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( -32.8378f, 0.0000f, 178.3522f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( -41.5202f, 0.0000f, 163.3138f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( -65.0125f, 0.0000f, 171.8643f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( -65.0125f, 0.0000f, 171.8643f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( -41.5202f, 0.0000f, 163.3138f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( -44.6982f, 0.0000f, 146.3044f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( -69.6982f, 0.0000f, 146.3044f);

		glEnd();

		// glEnable()
		glColor3f(1.0,1.0,0.0);
		 
		 glTranslatef(-58.0f,4.0f,72.0f);
		 if (v<1){
		glutSolidCube(3);
		} 
		glTranslatef(10.0f,0.0f,-106.0f);
		if (v<2){
		glutSolidCube(3);
		} 
		glTranslatef(42.5f,0.0f,-31.0f);
		if (v<3){
		glutSolidCube(3);
		} 
		glTranslatef(63.5f,0.0f,31.0f);
		if (v<4){
		glutSolidCube(3);
		}
		glTranslatef(10.0f,0.0f,107.0f);
		if (v<5){
		glutSolidCube(3);
		}
		glTranslatef(-23.0f,0.0f,120.0f);
		if (v<6){
		glutSolidCube(3);
		}
		glTranslatef(-39.0f,0.0f,15.0f);
		if (v<7){
		glutSolidCube(3);
		}
		glTranslatef(-54.0f,0.0f,-31.0f);
		if (v<8){
		glutSolidCube(3);
		}
		if (initialx > -70.0f && initialx < -45.0f && initialz > 70.0f && initialz < 75.0f && v==0 && !round_complete)
		{
			v = 1;
			}
		if (initialx > -55.0f && initialx < -40.0f && initialz > -40.0f && initialz < -30.0f && v==1 && !round_complete)
		{
			v = 2;
			}
		if (initialx > -10.0f && initialx < 0.0f && initialz > -65.0f && initialz < -55.0f && v==2 && !round_complete)
		{
			v = 3;
			}
		if (initialx > 50.0f && initialx < 65.0f && initialz > -40.0f && initialz < -30.0f && v==3 && !round_complete)
		{
			v = 4;
			}
		if (initialx > 55.0f && initialx < 80.0f && initialz > 70.0f && initialz < 75.0f && v==4 && !round_complete)
		{
			v = 5;
			}
		if (initialx > 35.0f && initialx < 55.0f && initialz > 188.0f && initialz < 198.0f && v==5 && !round_complete)
		{
			v = 6;
			}
		if (initialx > 0.0f && initialx < 10.0f && initialz > 198.0f && initialz < 218.0f && v==6 && !round_complete)
		{
			v = 7;
			}
		if (initialx > -55.0f && initialx < -40.0f && initialz > 172.0f && initialz < 182.0f && v==7 && !round_complete)
		{
			v = 8;
			}
	}
	else if (currTrack.id == 3) {	
		glBegin(GL_QUADS);
		
		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 0.2048f, 0.0000f, 16.3703f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 0.2048f, 0.0000f, 86.3703f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 25.2048f, 0.0000f, 86.3703f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 25.2048f, 0.0000f, 16.3703f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 29.8147f, 0.0000f, -53.3839f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 0.2048f, 0.0000f, 16.3703f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 25.2048f, 0.0000f, 16.3703f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 54.8147f, 0.0000f, -53.3839f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( 29.8148f, 0.0000f, -123.4015f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 29.8147f, 0.0000f, -53.3839f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 54.8147f, 0.0000f, -53.3839f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 54.8148f, 0.0000f, -123.4015f);

		glTexCoord2f(0.0, 0.0);
		 glVertex3f( -4.2518f, 0.0000f, -193.4343f);
		glTexCoord2f(0.0, 1.0);
		 glVertex3f( 29.8148f, 0.0000f, -123.4015f);
		glTexCoord2f(1.0, 1.0);
		 glVertex3f( 54.8148f, 0.0000f, -123.4015f);
		glTexCoord2f(1.0, 0.0);
		 glVertex3f( 20.7482f, 0.0000f, -193.4343f);

		glEnd();
		 
		glColor3f(1.0,1.0,0.0);
		 
		glTranslatef(13.0f,4.0f,50.0f);
		if (v<1){
			glEnable(GL_LIGHTING);
			GLfloat cyan[] = {1.f, 1.0f, 0.0f, 1.f};
			glMaterialfv(GL_FRONT, GL_DIFFUSE, cyan);
			GLfloat lightpos0[] = {5., -100., 100., 0.};
			glLightfv(GL_LIGHT0, GL_POSITION, lightpos0);
			glEnable(GL_LIGHT0);
			sphere.draw(0, 0, -5);
			glDisable(GL_LIGHTING);
		}
		glTranslatef(14.0f,0.0f,-65.0f);
		if (v<2){
			glShadeModel(GL_SMOOTH);
			glEnable(GL_LIGHTING);
			GLfloat lightpos0[] = {0.0f, 0., 10., 0.};
			glLightfv(GL_LIGHT0, GL_POSITION, lightpos0);
			glEnable(GL_LIGHT0);
			sphere.draw(0, 0, -5);
			glDisable(GL_LIGHTING);
		} 
		glTranslatef(15.0f,0.0f,-72.0f);
		if (v<3){
			glShadeModel(GL_SMOOTH);
			glEnable(GL_LIGHTING);
			GLfloat lightpos0[] = {.5, 1., 1., 0.};
			glLightfv(GL_LIGHT0, GL_POSITION, lightpos0);
			glEnable(GL_LIGHT0);
			sphere.draw(0, 0, -5);
			glDisable(GL_LIGHTING);
		} 
		glTranslatef(-17.0f,0.0f,-71.0);
		if (v<4){
			glShadeModel(GL_SMOOTH);
			glEnable(GL_LIGHTING);
			GLfloat lightpos0[] = {.5, 1., 1., 0.};
			glLightfv(GL_LIGHT0, GL_POSITION, lightpos0);
			glEnable(GL_LIGHT0);
			sphere.draw(0, 0, -5);
			glDisable(GL_LIGHTING);
		}
		glTranslatef(-25.0f,-4.0f,158.0f);
		
		if (initialx > 0.0f && initialx < 24.0f && initialz > 49.0f && initialz < 51.0f && v==0 && !round_complete)
		{
			v = 1;
			}
		if (initialx > 16.0f && initialx < 38.0f && initialz > -16.0f && initialz < -14.0f && v==1 && !round_complete)
		{
			v = 2;
			}
		if (initialx > 31.0f && initialx < 53.0f && initialz > -88.0f && initialz < -86.0f && v==2 && !round_complete)
		{
			v = 3;
			}
		if (initialx > 14.0f && initialx < 36.0f && initialz > -159.0f && initialz < -157.0f && v==3 && !round_complete)
		{
			v = 4;
			}
	}
	else {
		currTrack.id++;
	}

	/*
	* Loading the object - car
	*/
	glTranslatef(initialx-sin(rotY),1,initialz-cos(rotY));
	glColor3f(1.0,0.23,0.27);
	glScalef(0.6,0.6,0.6);
	glRotatef(rotY, 0.0f, 1.0f, 0.0f);
	// glCallList(ObjList);
	glRotatef(rotY, 0.0f, -1.0f, 0.0f);
	// glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
	// glTranslatef(-(initialx-sin(rotY)),0,-(initialz-cos(rotY)));

	glPopMatrix();
	/*
	* Trying to do some HUD here
	*/
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, win.width, win.height, 0.0, -1.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	glClear(GL_DEPTH_BUFFER_BIT);

	// The top quad -- header
	glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 0.0f);
		glVertex2f(0.0, 0.0);
		glVertex2f(win.width, 0.0);
		glVertex2f(win.width, 100.0);
		glVertex2f(0.0, 100.0);
	glEnd();

	/*
	*	On End of game
	*/
	if ((v == currTrack.numCheckpoints) || (userTime > currTrack.maxTime[level]))
	{
		round_complete = true;
		if (!timer_stopped)
		{
			finalTime = userTime;
			timer_stopped = true;
			userTime = 0;
		}
		glBegin(GL_QUADS);
			glColor3f(0.1f, 0.1f, 0.1f);
			glVertex2f(200.0, 120.0);
			glVertex2f(win.width-200.0, 120.0);
			glVertex2f(win.width-200.0, win.height -120.0);
			glVertex2f(200.0, win.height -120.0);

			glColor3f(0.8f, 0.8f, 0.8f);
			glVertex2f(220.0, 140.0);
			glVertex2f(win.width-220.0, 140.0);
			glVertex2f(win.width-220.0, win.height -140.0);
			glVertex2f(220.0, win.height -140.0);
		glEnd();

		glTranslatef(500.0f, 260.0, 0.0f);
		glScalef(0.5, 0.5, 1.0);
		glRotatef(180, 1.0, 0.0, 0.0);
		if (v == currTrack.numCheckpoints)
		{
			sprintf(quote[0], "Level Completed!!");
			sprintf(quote[1], "Your Timing: %d seconds", finalTime/CLOCKS_PER_SEC);
			// sprintf(quote[2], "Best Timing: %d seconds", userTime/CLOCKS_PER_SEC);
		}
		else {
			sprintf(quote[0], "GAME OVER");
			sprintf(quote[1], "Time Up! Better Luck Next Time!");
			// sprintf(quote[2], " ");
		}
		glPushMatrix();
		glPointSize(3.0f);
		lenghOfQuote = (int)strlen(quote[0]);
		for (int i = 0; i < lenghOfQuote; i++)
		{
			glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
			glutStrokeCharacter(GLUT_STROKE_ROMAN, quote[0][i]);
			// glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, quote[0][i]);
		}
		lenghOfQuote = (int)strlen(quote[1]);
		if (v == currTrack.numCheckpoints)
		{
			glTranslatef(-(lenghOfQuote*52), -174, 0.0);
		}
		else {
			glTranslatef(-(lenghOfQuote*39), -174, 0.0);
		}
		for (int i = 0; i < lenghOfQuote; i++)
		{
			glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
			glScalef(0.5, 0.5, 1.0);
			glutStrokeCharacter(GLUT_STROKE_ROMAN, quote[1][i]);
			glScalef(2.0, 2.0, 1.0);
		}

		glRotatef(180, -1.0, 0.0, 0.0);
		glScalef(2.0, 2.0, 1.0);
		glTranslatef(-1*(win.width/2 + 150.0f),-350.0f, 0.0f);
	}

	/*
	*	Car stats on the car switching page
	*/
	if (!space_pressed_to_start_race){
		int bar_length;
		glBegin(GL_QUADS);
			glColor3f(0.0f, 0.0f, 0.0f);
			glVertex2f(win.width - 600.0, 150.0);
			glVertex2f(win.width -  30.0, 150.0);
			glVertex2f(win.width -  30.0, 360.0);
			glVertex2f(win.width - 600.0, 360.0);
		glEnd();
		for (int attr = 0; attr < 4; ++attr)
		{
			if (attr == 0)
			{
				bar_length = myCar.maxSpeed * 60;
			}
			else if (attr == 1)
			{
				bar_length = myCar.acc * 3000;
			}
			else if (attr == 2)
			{
				bar_length = myCar.turnControl * 120;
			}
			else if (attr == 3)
			{
				bar_length = myCar.maxTurn*3;
			}
			glBegin(GL_QUADS);
				glColor3f(0.4f, 0.4f, 0.4f);
				glVertex2f(win.width - 360.0, 180.0 + attr*40);
				glVertex2f(win.width -  50.0, 180.0 + attr*40);
				glVertex2f(win.width -  50.0, 200.0 + attr*40);
				glVertex2f(win.width - 360.0, 200.0 + attr*40);
			glEnd();
			glBegin(GL_QUADS);
				glColor3f((attr==0) || (attr==3), (attr==2) || (attr==3), (attr == 1));
				glVertex2f(win.width - 360.0, 180.0 + attr*40);
				glVertex2f(win.width - 360.0 + bar_length, 180.0 + attr*40);
				glVertex2f(win.width - 360.0 + bar_length, 200.0 + attr*40);
				glVertex2f(win.width - 360.0, 200.0 + attr*40);
			glEnd();
		}
		/*
		*	Writing in the stats name in box
		*/
		glTranslatef(win.width/2 + 300.0f, 160.0, 0.0f);
		glScalef(0.2, 0.2, 1.0);
		glRotatef(180, 1.0, 0.0, 0.0);
		sprintf(quote[0], "Maximum Speed");
		sprintf(quote[1], "Acceleration");
		sprintf(quote[2], "Steer Control");
		sprintf(quote[3], "Maximum Turn");
		glPushMatrix();
		glPointSize(3.0f);
		for (int jj = 0; jj < 4; ++jj)
		{
			lenghOfQuote = (int)strlen(quote[jj]);
			glTranslatef(-(lenghOfQuote*70), -200, 0.0);
			for (int i = 0; i < lenghOfQuote; i++)
			{
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				glutStrokeCharacter(GLUT_STROKE_ROMAN, quote[jj][i]);
			}
		}
		glRotatef(180, -1.0, 0.0, 0.0);
		glScalef(5.0, 5.0, 1.0);
		glTranslatef(-1*(win.width/2 + 290),-310.0f, 0.0f);
	}

	if (space_pressed_to_start_race){
		/*
		*	Bottom Panel
		*/
		glBegin(GL_QUADS);
			glColor3f(0.3f, 0.3f, 0.3f);
			glVertex2f(0.0, win.height - 100);
			glVertex2f(win.width, win.height - 100);
			glVertex2f(win.width, win.height);
			glVertex2f(0.0, win.height);

			glVertex2f(0.0, 100);
			glVertex2f(15, 100);
			glVertex2f(35, win.height);
			glVertex2f(0.0, win.height);

			glVertex2f(win.width, 100);
			glVertex2f(win.width - 15, 100);
			glVertex2f(win.width - 35, win.height);
			glVertex2f(win.width, win.height);
		glEnd();
		/*
		*	The Space of Speedometer
		*/
		glShadeModel(GL_SMOOTH);
		glBegin(GL_POLYGON);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glVertex2f(200.0, win.height);
			glColor4f(0.4f, 0.4f, 0.4f, 1.0f);
			glVertex2f(200.0, win.height - 100);
			glVertex2f(300.0, win.height - 150);
			glVertex2f(400.0, win.height - 150);
			glVertex2f(500.0, win.height - 100);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glVertex2f(500.0, win.height);
		glEnd();
		glShadeModel(GL_FLAT);
		/*
		*	Building the Speedometer
		*/
		glTranslatef(win.width/4, win.height-60, 0.0f);
		glRotatef(60*sqrt(speed*speed) + 50, 0.0f, 0.0f, 1.0f);
		glBegin(GL_TRIANGLES);
			glColor3f(1.0f, 0.0f, 0.0f);
			glVertex2f(2.0f, 0.0f);
			glVertex2f(0.0f,100.0f);
			glVertex2f(-2.0f,0.0f);
		glEnd();
		char Speed_Display[30];
		glRotatef(-60*sqrt(speed*speed) - 50, 0.0f, 0.0f, 1.0f);
			/*
			And Printing that speed as well
			*/
			glScalef(0.2, 0.2, 0.2);
			glRotatef(180, -1.0, 0.0, 0.0);
			glTranslatef(-350.0f,-200.0f, 0.0f);
			sprintf(Speed_Display, "%.0f km/h", -50*speed);
			lenghOfQuote = (int)strlen(Speed_Display);
			for (int i = 0; i < lenghOfQuote; i++)
			{
				glColor4f(0.0f, 0.0f, 0.1f, 1.0f);
				glutStrokeCharacter(GLUT_STROKE_ROMAN, Speed_Display[i]);
			}
			glTranslatef(350.0f, 200.0f, 0.0f);
			glScalef(5, 5, 5);
			glRotatef(180, 1.0, 0.0, 0.0);
		glTranslatef(-1*win.width/4, -1*(win.height-60), 0.0f);
		/*
		*	Building the Steering Wheel
		*/
		glTranslatef(win.width/2, win.height+100, 0.0f);
		glRotatef(2*turn, 0.0f, 0.0f, 1.0f);
		glBegin(GL_TRIANGLE_STRIP);
			for (int ii = 0.6; ii < 7; ii += 1.0)
			{
				glColor3f((ii%2), 0.0f, 0.0f);
				glVertex2f(sin(ii)*300, cos(ii)*300);
			}
		glEnd();
		glRotatef(-2*turn, 0.0f, 0.0f, 1.0f);
		glTranslatef(-1*win.width/2, -1*(win.height+100), 0.0f);
	}
	/*
	* Trying to write some stuff inside the HUD created.
	*/
	glScalef(0.4, 0.4, 0.5);
	glTranslatef(0.0f,150.0f, 0.0f);
	glRotatef(180, 1.0, 0.0, 0.0);
	if (space_pressed_to_start_race)
	{
		userTime = clock()-startTime;
		if (timer_stopped)
		{
			sprintf(quote[0], "Time: %0.2f", (float)(finalTime)/CLOCKS_PER_SEC);
		}
		else {
			sprintf(quote[0], "Time: %0.2f", (float)(userTime)/CLOCKS_PER_SEC);
		}
	
    	lenghOfQuote = (int)strlen(quote[0]);
    	glPushMatrix();
	    for (int i = 0; i < lenghOfQuote; i++)
	    {
	    	glColor4f(0.0f, 0.0f, 0.1f, 1.0f);
	        glutStrokeCharacter(GLUT_STROKE_ROMAN, quote[0][i]);
	    }
	    glPopMatrix();
    }
	else {
		sprintf(quote[0], "Press Space to start Race");
		sprintf(quote[1], "Press ' c ' to change car");
		sprintf(quote[2], "Press ' t ' to change track");
		for (int jj = 0; jj < 3; ++jj)
	    {
	    	lenghOfQuote = (int)strlen(quote[jj]);
		    for (int i = 0; i < lenghOfQuote; i++)
		    {
		    	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		        glutStrokeCharacter(GLUT_STROKE_ROMAN, quote[jj][i]);
		    }
		    glTranslatef(-70*(lenghOfQuote), -200, 0.0);
		}
	}

	//Make sure we can render in 3d again
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	if (frame_type == PAUSE_SCREEN)
	{
		return;
	}
	glutSwapBuffers();
}


/*
*	The Display when the game starts
*/
void startDisplay()
{
	char quote[4][80];
	int lenghOfQuote;
	glClear(GL_COLOR_BUFFER_BIT);					//Clear the colour buffer (more buffers later on)
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -10.0f);

	/*
	* Trying to do some HUD here
	*/
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, win.width, win.height, 0.0, -1.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	glClear(GL_DEPTH_BUFFER_BIT);

	if (frame_type == SELECTION_SCREEN)
	{
		glPointSize(12.0f);
		glBegin(GL_POINTS);
			glColor3f(0.0f, 0.0f, 0.0f);
			glVertex2f(win.width/2 - 150 - 30, 190+selector);
			glVertex2f(win.width/2 + 150 - 30, 190+selector);
			// glVertex2f(win.width/2 + 50, 100/*-150+selector*200*/);
		glEnd();
		// glBegin(GL_QUADS);
		// 	glColor3f(1.0f, 1.0f, 0.0f);
		// 	glVertex2f(0.0, 0.0);
		// 	glVertex2f(win.width, 0.0);
		// 	glVertex2f(win.width, win.height);
		// 	glVertex2f(0.0, win.height);
		// glEnd();

		/*
		* Trying to write some stuff inside the HUD created.
		*/
		// glutStrokeCharacter(GLUT_STROKE_ROMAN, 1/*myCharString*/);
		glTranslatef(win.width/2 + 85,150.0f, 0.0f);
		glScalef(0.28, 0.25, 1.0);
		glRotatef(180, 1.0, 0.0, 0.0);
		sprintf(quote[0], "Start Game");
		sprintf(quote[1], " Game Level");
		sprintf(quote[2], " Settings");
		sprintf(quote[3], "How to play");
		sprintf(quote[4], "Exit  ");
	    glPushMatrix();
	    glPointSize(3.0f);
	    for (int jj = 0; jj < 5; ++jj)
	    {
	    	lenghOfQuote = (int)strlen(quote[jj]);
	    	glTranslatef(-(lenghOfQuote*(77 - 22*(jj==3))), -200, 0.0);
		    for (int i = 0; i < lenghOfQuote; i++)
		    {
		    	glColor4f(0.0f, 0.1f, 0.2f, 1.0f);
		        // glColor3f((UpwardsScrollVelocity/10)+300+(l*10),(UpwardsScrollVelocity/10)+300+(l*10),0.0);
		        glutStrokeCharacter(GLUT_STROKE_ROMAN, quote[jj][i]);
		    }
		}
	    glPopMatrix();
	}
	else if (frame_type == PAUSE_SCREEN)
	{
		glPointSize(12.0f);
		glBegin(GL_POINTS);
			glColor3f(0.0f, 0.0f, 0.0f);
			glVertex2f(win.width/2 - 150 - 30, 190+selector);
			glVertex2f(win.width/2 + 150 - 30, 190+selector);
		glEnd();
		/*
		*	Highlighting the currently chosen level
		*/
		// glBegin(GL_QUADS);
		// 	glVertex2f();
		// glEnd();
		/*
		* Trying to write some stuff inside the HUD created.
		*/
		glTranslatef(win.width/2 + 82,150.0f, 0.0f);
		glScalef(0.28, 0.25, 1.0);
		glRotatef(180, 1.0, 0.0, 0.0);
		sprintf(quote[0], "Resume Game");
		sprintf(quote[1], "Settings ");
		sprintf(quote[2], "Main Menu");
	    glPushMatrix();
	    glPointSize(3.0f);
	    for (int jj = 0; jj < 3; ++jj)
	    {
	    	lenghOfQuote = (int)strlen(quote[jj]);
	    	glTranslatef(-(lenghOfQuote*77), -200, 0.0);
		    for (int i = 0; i < lenghOfQuote; i++)
		    {
		    	glColor4f(0.0f, 0.0f, 0.0f, 0.2f);
		        glutStrokeCharacter(GLUT_STROKE_ROMAN, quote[jj][i]);
		    }
		}
	    glPopMatrix();
	}
	else if (frame_type == LEVEL_CHANGE_SCREEN)
	{
		glPointSize(12.0f);
		glBegin(GL_POINTS);
			glColor3f(0.0f, 0.0f, 0.0f);
			glVertex2f(win.width/2 - 150 - 30, 190+selector);
			glVertex2f(win.width/2 + 150 - 30, 190+selector);
		glEnd();
		/*
		* Trying to write some stuff inside the HUD created.
		*/
		glTranslatef(win.width/2 + 90,150.0f, 0.0f);
		glScalef(0.28, 0.25, 1.0);
		glRotatef(180, 1.0, 0.0, 0.0);
		sprintf(quote[0], "Learner Level");
		sprintf(quote[1], "Decent-Driver");
		sprintf(quote[2], "Professional");
		sprintf(quote[3], "Back to Menu");
	    glPushMatrix();
	    glPointSize(3.0f);
	    for (int jj = 0; jj < 4; ++jj)
	    {
	    	// if (jj == level)
	    	// {
	    	// 	strcat(quote[jj], "      current level");
	    	// }
	    	lenghOfQuote = (int)strlen(quote[jj]);
	    	glTranslatef(-(lenghOfQuote*65), -200, 0.0);
		    for (int i = 0; i < lenghOfQuote; i++)
		    {
		    	glColor4f(0.0f + 0.8*(jj == level), 0.0f + 0.8*(jj == level), 0.0f, 1.0f);
		        glutStrokeCharacter(GLUT_STROKE_ROMAN, quote[jj][i]);
		    }
		}
	    glPopMatrix();
	}
	else if (frame_type == SETTING_SCREEN)
	{
		glPointSize(12.0f);
		glBegin(GL_POINTS);
			glColor3f(0.0f, 0.0f, 0.0f);
			glVertex2f(win.width/2 - 150 - 30, 190+selector);
			glVertex2f(win.width/2 + 150 - 30, 190+selector);
		glEnd();
		/*
		* Trying to write some stuff inside the HUD created.
		*/
		glTranslatef(win.width/2 - 230,150.0f, 0.0f);
		glScalef(0.28, 0.25, 1.0);
		glRotatef(180, 1.0, 0.0, 0.0);
		sprintf(quote[0], "Game Music");
		sprintf(quote[1], "Sound Effects");
		sprintf(quote[2], "1st Person View");
		sprintf(quote[3], "Back to Menu");
	    glPushMatrix();
	    glPointSize(3.0f);
	    for (int jj = 0; jj < 3; ++jj)
	    {
	    	// if (jj == level)
	    	// {
	    	// 	strcat(quote[jj], "      current level");
	    	// }
	    	lenghOfQuote = (int)strlen(quote[jj]);
	    	glTranslatef(-(lenghOfQuote*69), -200, 0.0);
		    for (int i = 0; i < lenghOfQuote; i++)
		    {
		    	glColor4f(0.0f + (jj == 2), 0.0f + (jj == 2), 0.0f + (jj == 2), 1.0f);
		        glutStrokeCharacter(GLUT_STROKE_ROMAN, quote[jj][i]);
		    }
		}
		glTranslatef(200.0f, -200.0f, 0.0f);
		for (int i = 0; i < lenghOfQuote; i++)
		{
			glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		    glutStrokeCharacter(GLUT_STROKE_ROMAN, quote[3][i]);
		}
		glTranslatef(520.0f, 800.0f, 0.0f);
		if (music_on)
		{
			sprintf(quote[0], "<On>");
		}
		else {
			sprintf(quote[0], "<Off>");
		}
		if (sound_effects_on)
		{
			sprintf(quote[1], "<On>");
		}
		else {
			sprintf(quote[1], "<Off>");
		}
		if (third_person_view)
		{
			sprintf(quote[2], "<Off>");
		} else {
			sprintf(quote[2], "<On>");
		}
	    glPushMatrix();
	    glPointSize(3.0f);
	    for (int jj = 0; jj < 3; ++jj)
	    {
	    	// if (jj == level)
	    	// {
	    	// 	strcat(quote[jj], "      current level");
	    	// }
	    	lenghOfQuote = 5;									//Because all of them are <On> or <Off> .. so that they dont shake on change
	    	glTranslatef(-(lenghOfQuote*65), -200, 0.0);
		    for (int i = 0; i < lenghOfQuote; i++)
		    {
		    	glColor4f(0.0f + (jj == 2), 0.0f + (jj == 2), 0.0f + (jj == 2), 1.0f);
		        glutStrokeCharacter(GLUT_STROKE_ROMAN, quote[jj][i]);
		    }
		}
	    glPopMatrix();
	}
	else if (frame_type == INSTRUCTION_SCREEN)
	{
		char option[14][80];
		glPointSize(12.0f);
		/*
		* Trying to write some stuff inside the HUD created.
		*/
		glTranslatef(win.width/2 - 300,100.0f, 0.0f);
		glScalef(0.2, 0.18, 1.0);
		glRotatef(180, 1.0, 0.0, 0.0);
		sprintf(option[0], "Controls:             ");
		sprintf(option[1], "W : Accelerate forward");
		sprintf(option[2], "S : Accelerate backward");
		sprintf(option[3], "A : Turn Steering left");
		sprintf(option[4], "D : Turn Steering right");
		sprintf(option[5], "Instructions:          ");
		sprintf(option[6], "On start game, press C to change car,");
		sprintf(option[7], "and change track by pressing T.");
		sprintf(option[8], "Then press SPACE to get rolling. ");
		sprintf(option[9], "Collect all checkpoint items in");
		sprintf(option[10], "sequential order to complete game in");
		sprintf(option[11], "as less time as possible.");
	    glPushMatrix();
	    glPointSize(3.0f);
	    for (int jj = 0; jj < 12; ++jj)
	    {
	    	lenghOfQuote = (int)strlen(option[jj]);
	    	
		    for (int i = 0; i < lenghOfQuote; i++)
		    {
		    	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		        glutStrokeCharacter(GLUT_STROKE_ROMAN, option[jj][i]);
		    }
		    glTranslatef(-(lenghOfQuote*67), -200, 0.0);
		}
	    glPopMatrix();
	}
	//Make sure we can render in 3d again
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	if (frame_type == PLAYING_SCREEN)
	{
		display();
	}
	glutSwapBuffers();
}

void initialize () 
{
    glMatrixMode(GL_PROJECTION);												// select projection matrix
    glViewport(0, 0, win.width, win.height);									// set the viewport
    glMatrixMode(GL_PROJECTION);												// set matrix mode
    glLoadIdentity();															// reset projection matrix
    GLfloat aspect = (GLfloat) win.width / win.height;
	gluPerspective(win.field_of_view_angle, aspect, win.z_near, win.z_far);		// set up a perspective projection matrix
    glMatrixMode(GL_MODELVIEW);													// specify which matrix is the current matrix
    glShadeModel( GL_SMOOTH );
    glClearDepth( 1.0f );														// specify the clear value for the depth buffer
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );						// specify implementation-specific hints
	glClearColor(0.4, 0.3, 0.5, 1.0);											// specify clear values for the color buffers								
	loadTexture();
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);


}


void keyboard ( unsigned char key, int mousePositionX, int mousePositionY )		
{ 
  switch ( key ) 
  {
    case KEY_ESCAPE:
    	if (frame_type == PLAYING_SCREEN)
    	{
    		frame_type = PAUSE_SCREEN;
    	}
    	else if (frame_type == PAUSE_SCREEN)
    	{
    		frame_type = PLAYING_SCREEN;
    	}
    	else if (frame_type = LEVEL_CHANGE_SCREEN)
    	{
    		frame_type = SELECTION_SCREEN;
    		selector = 0;
    	}
    	else if (frame_type = INSTRUCTION_SCREEN)
    	{
    		frame_type = SELECTION_SCREEN;
    		selector = 0;
    	}
    	break;
    
    case 't':
    	if ((frame_type == PLAYING_SCREEN) && (!space_pressed_to_start_race))
    	{
			currTrack.id ++;
			currTrack.id = currTrack.id %4;
			if (currTrack.id == 2)
			{
				currTrack.X_max_world = 100.0f;
				currTrack.X_min_world =-100.0f;
				currTrack.Z_max_world = 250.0f;
				currTrack.Z_min_world = -100.0f;
			}
			if (currTrack.id == 3)
			{
				currTrack.X_min_world = -10.0f;
				currTrack.X_max_world = 60.0f;
				currTrack.Z_min_world = -200.0f;
				currTrack.Z_max_world = 100.0f;
			}
    		break;
    	}

    case 'c':
    	if ((frame_type == PLAYING_SCREEN) && (!space_pressed_to_start_race))
    	{
			myCar.ID ++;
			if (myCar.ID == 4)
			{
				myCar.ID = 0;
			}
			if (myCar.ID == 0)
			{
				myCar.maxTurn = 65;
				myCar.maxSpeed = 4.0;
				myCar.acc = 0.02;
				myCar.turnControl = 1.8;
				myCar.objFileName = "media/avi.obj";
			}
			else if (myCar.ID == 1)
			{
				myCar.maxTurn = 80;
				myCar.maxSpeed = 3.0;
				myCar.acc = 0.019;
				myCar.turnControl = 2.0;
				myCar.objFileName = "media/car.obj";
			}
			else if (myCar.ID == 2)
			{
				myCar.maxTurn = 40;
				myCar.maxSpeed = 5.0;
				myCar.acc = 0.05;
				myCar.turnControl = 1.7;
				myCar.objFileName = "media/my.obj";
			}
			else if (myCar.ID == 3)
			{
				myCar.maxTurn = 90;
				myCar.maxSpeed = 0.8;
				myCar.acc = 0.1;
				myCar.turnControl = 2.5;
				myCar.objFileName = "media/box.obj";
			}
    		break;
    	}

    case SPACE:
    	if ((frame_type == PLAYING_SCREEN) && !(space_pressed_to_start_race))
    	{
	    	if (currTrack.id == 2)
	    	{
	    		currTrack.friction = 0.01;
	    		currTrack.numCheckpoints = 8;
	    		currTrack.start_x = -55.6982f;
	    		currTrack.start_z = 146.3044f;
	    		currTrack.X_max_world = 100.0f;
	    		currTrack.X_min_world =-100.0f;
	    		currTrack.Z_max_world = 250.0f;
	    		currTrack.Z_min_world = -100.0f;
	    		currTrack.maxTime[0] = 45* CLOCKS_PER_SEC;
	    		currTrack.maxTime[1] = 36* CLOCKS_PER_SEC;
	    		currTrack.maxTime[2] = 30* CLOCKS_PER_SEC;
	    	}
	    	if (currTrack.id == 3)
	    	{
	    		currTrack.friction = 0.02;
	    		currTrack.numCheckpoints = 4;
	    		currTrack.start_x = 14.8148f;
	    		currTrack.start_z =  92.4015f;
	    		currTrack.X_min_world = -10.0f;
	    		currTrack.X_max_world = 80.0f;
	    		currTrack.Z_min_world = -200.0f;
	    		currTrack.Z_max_world = 100.0f;
	    		currTrack.maxTime[0] = 25* CLOCKS_PER_SEC;
	    		currTrack.maxTime[1] = 18* CLOCKS_PER_SEC;
	    		currTrack.maxTime[2] = 15* CLOCKS_PER_SEC;
	    	}
	    	initialx = currTrack.start_x;
	    	initialz = currTrack.start_z;
	    	rotY = 0;
	    	speed = 0;
	    	turn = 0;
	    	v = 0;
	    	space_pressed_to_start_race = true;
	    	round_complete = false;
	    	timer_stopped = false;
	    	startTime = clock();
	    	break;
	    }

    case ENTER:
    	if (frame_type == SELECTION_SCREEN)
    	{
    		if (selector == 0)
    		{
    			frame_type = PLAYING_SCREEN;
    		}
    		else if (selector == 50)
    		{
    			frame_type = LEVEL_CHANGE_SCREEN;
    			selector = 0;
    		}
    		else if (selector == 100)
    		{
    			frame_type = SETTING_SCREEN;
    			selector = 0;
    		}
    		else if (selector == 150)
    		{
    			frame_type = INSTRUCTION_SCREEN;
    			selector = 0;
    		}
    		else if (selector == 200)
    		{
    			exit(0);
    		}
    	}
    	else if (frame_type == PAUSE_SCREEN)
    	{
    		if (selector == 0)
    		{
    			frame_type = PLAYING_SCREEN;
    		}
    		else if (selector == 50)
    		{
    			frame_type = SETTING_SCREEN;
    			selector = 0;
    		}
    		else if (selector == 100)
    		{
    			frame_type = SELECTION_SCREEN;
    			selector = 0;
    			userTime = 0;
    			v = 0;
    			space_pressed_to_start_race = false;
    		}
    	}
    	else if (frame_type == LEVEL_CHANGE_SCREEN)
    	{
    		if (selector == 150)
    		{
    			frame_type = SELECTION_SCREEN;
    			selector = 0;
    		}
    		else if (selector == 0)
    		{
    			level = 0;
    		}
    		else if (selector == 50)
    		{
    			level =1;
    		}
    		else if (selector == 100)
    		{
    			level = 2;
    		}
    	}
    	else if (frame_type == SETTING_SCREEN)
    	{
    		if (selector == 150)
    		{
    			frame_type = SELECTION_SCREEN;
    			selector = 0;
    		}
    		else if (selector == 0)
    		{
    			music_on = !(music_on);
    		}
    		else if (selector == 50)
    		{
    			sound_effects_on = !sound_effects_on;
    		}
    		else if (selector == 100)
    		{
    			third_person_view = !third_person_view;
    		}
    	}
    default:
	activeKey[key] = true;
      break;
  }
}

void keyUp (unsigned char key, int x, int y){
	activeKey[key] = false;
}

//For Special Keys
void SpecialKeyboard ( int key, int mousePositionX, int mousePositionY )		
{
	if (key == GLUT_KEY_UP)
	{
		key = 256;
		if (frame_type == SELECTION_SCREEN)
		{
			selector -= 50;
			if (selector == -50)
			{
				selector = 200;
			}
		}
		else if (frame_type == PAUSE_SCREEN)
		{
			selector -= 50;
			if (selector == -50)
			{
				selector = 100;
			}
		}
		else if (frame_type == LEVEL_CHANGE_SCREEN)
		{
			selector -= 50;
			if (selector == -50)
			{
				selector = 150;
			}
		}
		else if (frame_type == SETTING_SCREEN)
		{
			selector -= 50;
			if (selector == -50)
			{
				selector = 150;
			}
			if (selector == 100)
			{
				selector = 50;
			}
		}
	}
	else if (key == GLUT_KEY_DOWN)
	{
		key = 257;
		if (frame_type == SELECTION_SCREEN)
		{
			selector += 50;
			selector = selector%250;
		}
		else if (frame_type == PAUSE_SCREEN)
		{
			selector += 50;
			selector = selector%150;
		}
		else if (frame_type == LEVEL_CHANGE_SCREEN)
		{
			selector += 50;
			selector = selector%200;
		}
		else if (frame_type == SETTING_SCREEN)
		{
			selector += 50;
			selector = selector%200;
			if (selector == 100)
			{
				selector = 150;
			}
		}
	}
	else if (key == GLUT_KEY_LEFT)
	{
		key = 258;
	}
	else if (key == GLUT_KEY_RIGHT)
	{
		key = 259;
	}
	else {
		return;
	}
	// activeKey[key] = true;
}

int main(int argc, char **argv) 
{
	// set window values
	win.width = 1380;
	win.height = 600;
	win.field_of_view_angle = 45;
	win.z_near = 1.0f;
	win.z_far = 500.0f;

	myCar.maxTurn = 65;
	myCar.maxSpeed = 4.0;
	myCar.acc = 0.02;
	myCar.turnControl = 1.8;
	myCar.objFileName = "media/avi.obj";

	myCar.ID = 0;
	currTrack.id = 2;
	currTrack.numCheckpoints = 8;
	level = 0;
	frame_type = SELECTION_SCREEN;

	// initialize and run program
	glutInit(&argc, argv);                                      // GLUT initialization
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );  // Display Mode
	glutInitWindowSize(win.width,win.height);					// set window size
	glutCreateWindow("Car Simulator");									// create Window
	glutDisplayFunc(startDisplay);									// register Display Function
	glutIdleFunc( startDisplay );									// register Idle Function
	glutKeyboardFunc( keyboard );
	glutKeyboardUpFunc(keyUp);								// register Keyboard Handler
	glutSpecialFunc( SpecialKeyboard );
	// glutSpecialUpFunc(SpecialKeyUp);								// register Special Keyboard Handler
	printf("the speed of the car is not able to go beyond %f, where limit is %s\n", speed, glGetString(GL_VERSION));

	// loadObj(myCar.objFileName);								//replace porsche.obj with radar.obj or any other .obj to display it
	initialize();
	glutMainLoop();											// run GLUT mainloop
	return 0;
}
