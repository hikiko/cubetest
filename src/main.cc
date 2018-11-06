#include <GL/glut.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool init();
static void cleanup();
static unsigned int load_cubemap();

static void display();
static void reshape(int x, int y);
static void keyboard(unsigned char key, int x, int y);
static void mouse(int bn, int state, int x, int y);
static void motion(int x, int y);

struct Texture {
	unsigned int id;
	int width;
	int height;
	unsigned int format;
	unsigned int size;
	unsigned char *data;
};

struct Header {
	char magic[8];
	uint32_t glfmt;
	uint16_t flags;
	uint16_t levels;
	uint32_t width, height;
	struct {
		uint32_t offset, size;
	} datadesc[20];
	char unused[8];
};

static unsigned int cubemap;
static float cam_phi, cam_theta;

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(1024, 768);
	glutCreateWindow("cubetest");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	if(!init())
		return 1;

	atexit(cleanup);

	glutMainLoop();
	return 0;
}

static bool load_teximage(const char *fname, Texture *tex)
{
	FILE *fp;
	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to open file: %s: %s\n", fname, strerror(errno));
		return false;
	}

	Header hdr;
	if(fread(&hdr, 1, sizeof hdr, fp) != sizeof hdr) {
		fprintf(stderr, "failed to read image file header: %s: %s\n", fname, strerror(errno));
		fclose(fp);
		return false;
	}
	if(memcmp(hdr.magic, "COMPTEX0", sizeof hdr.magic) != 0 || hdr.levels < 0 ||
			hdr.levels > 20 || !hdr.datadesc[0].size) {
		fprintf(stderr, "%s is not a compressed texture file, or is corrupted\n", fname);
		fclose(fp);
		return false;
	}

	tex->data = new unsigned char[hdr.datadesc[0].size];

	if(fread(tex->data, 1, hdr.datadesc[0].size, fp) != hdr.datadesc[0].size) {
		fprintf(stderr, "unexpected EOF while reading texture: %s\n", fname);
		fclose(fp);
		delete [] tex->data;
		return false;
	}

	tex->width = hdr.width;
	tex->height = hdr.height;
	tex->format = hdr.glfmt;
	tex->size = hdr.datadesc[0].size;

	fclose(fp);

	return true;
}

static unsigned int load_cubemap()
{
	static const char *names[6] = {"data/right.tex", "data/left.tex", "data/up.tex",
								   "data/down.tex", "data/back.tex", "data/front.tex"};

	Texture tex;
	glGenTextures(1, &tex.id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex.id);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	for(int i=0; i<6; i++) {
		if(!load_teximage(names[i], &tex)) {
			glDeleteTextures(1, &tex.id);
			return 0;
		}
		unsigned int face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
		glCompressedTexImage2D(face, 0, tex.format, tex.width, tex.height, 0, tex.size, tex.data); 
		delete [] tex.data;
	}

	return tex.id;
}

static bool init()
{
	if(!(cubemap = load_cubemap())) {
		return false;
	}
	glEnable(GL_TEXTURE_CUBE_MAP);

	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);

	float planes[][4] = {{1, 0, 0, 0}, {0,1, 0, 0}, {0, 0, 1, 0}};

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	glTexGenfv(GL_S, GL_OBJECT_PLANE, planes[0]);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, planes[1]);
	glTexGenfv(GL_R, GL_OBJECT_PLANE, planes[2]);

	return true;
}

static void cleanup()
{
	glDeleteTextures(1, &cubemap);
}

static void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(cam_phi, 1, 0, 0);
	glRotatef(cam_theta, 0, 1, 0);

	glutSolidSphere(10, 10, 5);

	glutSwapBuffers();
}

static void reshape(int x, int y)
{
	glViewport(0, 0, x, y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, (float)x / (float)y, 0.5, 100);
}

static void keyboard(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);
	default:
		break;
	}
}

static int prev_x, prev_y;
static bool bnstate[6];

static void mouse(int bn, int state, int x, int y)
{
	prev_x = x;
	prev_y = y;
	bnstate[bn - GLUT_LEFT_BUTTON] = state == GLUT_DOWN;
}

static void motion(int x, int y)
{
	if(bnstate[0]) {
		cam_phi += (y - prev_y) * 0.5;
		cam_theta += (x - prev_x) * 0.5;

		if(cam_phi < -90) cam_phi = -90;
		if(cam_phi > 90) cam_phi = 90;

		glutPostRedisplay();
	}

	prev_y = y;
	prev_x = x;
}
