#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef USE_CONSTE

#ifdef WIN32
# include "winjunk.h"
#endif

#include "futil.h"

#ifdef __APPLE__
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

extern "C" {
#include "findfile.h"
};
#include "specks.h"
#include "partiviewc.h"

#include "conste.h"     // declare following as extern "C"

#define CONSTE_NONE  0
#define CONSTE_LINE  1
#define CONSTE_FIG   2

struct conste
{
    char name[64];
    GLuint line_index;  /* OpenGL display list index for line */
    GLuint fig_index;   /* OpenGL display list index for figure */
    char line_filename[256];
    char fig_filename[256];
    float lnear;
    float lfar;
    int state;
};

struct constestuff {
    conste *constes;
    int num_constes;
    float line_color[3];
    float fig_color[3];
};


extern "C" {
 void conste_init() {
    static char Conste[] = "conste";
    parti_add_reader( conste_read, Conste, NULL );
    parti_add_commands( conste_parse_args, Conste, NULL );
 }
}

int conste_build_line(conste *_cd)
{
    FILE *fp;
    char line[256];
    char token[256];
    float vec0[3];

    if (!_cd->line_filename) return 0;

    fp = fopen(_cd->line_filename,"r");
    if (!fp) {
        msg("Data file %s not found.",_cd->line_filename);
        return 0;
    }

    while (fgets(line,256,fp) != NULL)
    {
        sscanf(line,"%s",token);
        if (strcmp(token,"Lines:") == 0) {
            _cd->line_index = glGenLists(1);
            glNewList(_cd->line_index,GL_COMPILE_AND_EXECUTE);
            glBegin(GL_LINE_STRIP);
            while (fgets(line,256,fp) != NULL && line[0] != '}')
            {
                if (sscanf(line,"%f %f %f",&vec0[0],&vec0[1],&vec0[2]) < 3) {
                    glEnd();
                    glBegin(GL_LINE_STRIP);
                } else {
                    glVertex3fv(vec0);
                }
            }
            glEnd();
            glEndList();
        }
    }

    fclose(fp);

    return 1;
}

int conste_build_fig(conste *_cd)
{
    FILE *fp;
    float pos[3];
    float size,rot;
    float *points;
    int numPoints;
    int *indexes;
    int numIndexes;
    int i;

    if (!_cd->fig_filename) return 0;

    fp = fopen(_cd->fig_filename,"rb");
    if (!fp) {
        msg("Data file %s not found.",_cd->fig_filename);
        return 0;
    }

    _cd->fig_index = glGenLists(1);
    glNewList(_cd->fig_index,GL_COMPILE);

    fgetnf(fp, 3, pos, F_BINARY_LE);
    fgetnf(fp, 1, &size, F_BINARY_LE);
    fgetnf(fp, 1, &rot, F_BINARY_LE);
    numPoints = -1;
    fgetni(fp, 1, &numPoints, F_BINARY_LE);
    if(numPoints < 0 || numPoints > 65535) {
	msg("constellation file %s garbled", _cd->fig_filename);
	return 0;
    }
    points = new float[numPoints*2];
    fgetnf(fp, numPoints*2, points, F_BINARY_LE);
    fgetni(fp, 1, &numIndexes, F_BINARY_LE);
    indexes = new int[numIndexes];
    fgetni(fp, numIndexes, indexes, F_BINARY_LE);

    glPushMatrix();
/*    glMultMatrixf((float *)sssim2gview);*/
    glRotatef(90.0f,0.0f,0.0f,-1.0f);
    glRotatef(90.0f,0.0f,-1.0f,0.0f);
    if (size != 1.0f)
    {
        Point from,to;
        from.x[0] = 0.0f;
        from.x[1] = 0.0f;
        from.x[2] = 1.0f;
        to.x[0] = 0.5f * pos[0];
        to.x[1] = 0.5f * pos[1];
        to.x[2] = 0.5f * pos[2];
        float cpdist = sqrtf(to.x[0]*to.x[0]+to.x[1]*to.x[1]+to.x[2]*to.x[2]);
        to.x[0] /= cpdist;
        to.x[1] /= cpdist;
        to.x[2] /= cpdist;
        glTranslatef(to.x[0],to.x[1],to.x[2]);
        Matrix mat;
        grotation(&mat,&from,&to);
        glMultMatrixf(mat.m);
        glScalef(size/cpdist,size/cpdist,1.0);
        glRotatef(rot,0.0f,0.0f,1.0f);
    }
    glBegin(GL_LINE_STRIP);
    for (i=0;i<numIndexes;i++)
    {
        if (indexes[i] >= 0) {
            glVertex2f(-points[indexes[i]*2],points[indexes[i]*2+1]);
        } else {
            glEnd();
            glBegin(GL_LINE_STRIP);
        }
    }
    glEnd();
    glPopMatrix();

    delete [] points;
    delete [] indexes;

    glEndList();

    fclose(fp);

    return 1;
}

int conste_read_data(conste *_cd,char *_filename)
{
    FILE *fp;
    char line[256];
    char *filename;
    char token[256];
    char *start;
    float x,y,z;
    float lnear,lfar,dist;

    lnear = 0.0f;
    lfar = 0.0f;
    dist = 0.0f;

    _cd->line_index = 0;
    _cd->fig_index = 0;
    _cd->state = CONSTE_LINE | CONSTE_FIG;
    strcpy(_cd->line_filename,_filename);

    fp = fopen(_filename,"r");
    if (!fp) {
        msg("Data file %s not found.",_filename);
        return 0;
    }

    while (fgets(line,256,fp) != NULL)
    {
        sscanf(line,"%s",token);
        if (strcmp(token,"Name:") == 0) {
            line[strlen(line)-1] = '\0';
            start = &line[strlen("Name:")];
            while (*start == ' ') start++;
            strcpy(_cd->name,start);
        }
        else if (strcmp(token,"Art:") == 0) {
            sscanf(line,"%*s %s",_cd->fig_filename);
            filename = findfile(_filename,_cd->fig_filename);
            if (filename)
                strcpy(_cd->fig_filename,filename);
        }
        else if (strcmp(token,"Lines:") == 0) {
            while (fgets(line,256,fp) != NULL && line[0] != '}')
            {
                if (sscanf(line,"%f %f %f",&x,&y,&z) == 3) {
                    dist = x*x+y*y+z*z;
                    if (lnear == 0.0f || dist < lnear) lnear = dist; 
                    if (lfar == 0.0f || dist > lfar) lfar = dist; 
                }
            }
        }
    }
    _cd->lnear = sqrtf(lnear);
    _cd->lfar = sqrtf(lfar);

    fclose(fp);

    return 1;
}

int conste_read(struct stuff **_stp, int argc, char *argv[], char *_filename, void * )
{
    FILE *fp;
    char data_filename[256];
    char *filename;
    int i;
    struct stuff *_st = *_stp;

    if(0!=strcmp(argv[0], "conste"))
	return 0;

    if(argc!=2) {
	msg("conste: expected \"conste <file-containing-list-of-filenames>\"");
	return 0;
    }

    fp = fopen(argv[1],"r");
    if (!fp) {
        msg("conste %s: data file not found.", argv[1]);
        return 0;
    }

    constestuff *cs = new constestuff;
    if(fscanf(fp,"%i",&cs->num_constes) <= 0) {
	msg("conste %s: expected num_constes", argv[1]);
	return 0;
    }
    cs->constes = new conste[cs->num_constes];

    for (i=0;i<cs->num_constes;i++) {
        if(fscanf(fp,"%255s",data_filename) <= 0) {
	    msg("conste %s: couldn't read %d'th of %d constellation filenames", argv[1], i, cs->num_constes);
	    return 0;
	}
        filename = findfile(_filename,data_filename);
        if (filename)
            conste_read_data(&cs->constes[i],filename);
	else
	    msg("conste %s: couldn't find constellation file %s", argv[1], data_filename);
    }

    cs->line_color[0] = 0.400000f;
    cs->line_color[1] = 0.400000f;
    cs->line_color[2] = 0.800000f;

    cs->fig_color[0] = 0.500000f;
    cs->fig_color[1] = 0.500000f;
    cs->fig_color[2] = 0.500000f;

    _st->conste = 1;
    _st->constedata = cs;
    _st->textsize = 1.0f;

    fclose(fp);

    return 1;
}

void conste_draw(struct stuff *_st)
{
    int i;

    glPushMatrix();
    glRotatef(90.0f,0.0f,0.0f,1.0f);
    glRotatef(90.0f,1.0f,0.0f,0.0f);

    constestuff *cs = (reinterpret_cast<constestuff *>(_st->constedata));

    if (_st->usepoly)
    {
        glColor3fv(cs->line_color);
        for (i=0;i<cs->num_constes;i++) {
            if (cs->constes[i].state & CONSTE_LINE) {
                if (cs->constes[i].line_index == 0)
                    conste_build_line(&cs->constes[i]);
                glCallList(cs->constes[i].line_index);
            }
        }
    }

    if (_st->usetextures)
    {
        glColor3fv(cs->fig_color);
        for (i=0;i<cs->num_constes;i++) {
            if (cs->constes[i].state & CONSTE_FIG) {
                if (cs->constes[i].fig_index == 0)
                    conste_build_fig(&cs->constes[i]);
                glPushMatrix();
                glScalef((cs->constes[i].lnear + cs->constes[i].lfar) / 2.0f,
                         (cs->constes[i].lnear + cs->constes[i].lfar) / 2.0f,
                         (cs->constes[i].lnear + cs->constes[i].lfar) / 2.0f);
                glCallList(cs->constes[i].fig_index);
                glPopMatrix();
            }
        }
    }

    glPopMatrix();
}

int conste_parse_args(struct stuff **_stp,int _argc,char *_argv[], char *_filename, void *)
{
    char result[1024];
    int interest = 0;
    int i,j;
    struct stuff *_st = *_stp;

    if(_argc < 1 || 0!=strncmp(_argv[0], "conste", 3))
	return 0;

    constestuff *cs = (reinterpret_cast<constestuff *>(_st->constedata));

    if (_argc > 1) {
        if (!strcmp(_argv[0],"line") || !strcmp(_argv[0],"lines"))
            interest = CONSTE_LINE;
        else if (!strcmp(_argv[0],"fig") || !strcmp(_argv[0],"figs") ||
                 !strcmp(_argv[0],"figure") || !strcmp(_argv[0],"figures"))
            interest = CONSTE_FIG;
        else {
	    msg("conste {line|fig} {show|hide} [constname...|all] -or- {line|fig} color [R G B]");
            return 1;
	}

        if (!strcmp(_argv[1],"show")) {
            if (interest == CONSTE_LINE)
                strcpy(result,"show constellation lines : ");
            else
                strcpy(result,"show constellation figures : ");
            for (j=2;j<_argc;j++) {
                if (!strcmp(_argv[j],"all")) {
                    for (i=0;i<cs->num_constes;i++)
                        if (!(cs->constes[i].state & interest))
                            cs->constes[i].state += interest;
                    break;
                } else {
                    for (i=0;i<cs->num_constes;i++)
                        if (!strncmp(_argv[j],cs->constes[i].name,strlen(_argv[j])))
                            if (!(cs->constes[i].state & interest)) {
                                cs->constes[i].state += interest;
                                strcat(result,cs->constes[i].name);
                                strcat(result," ");
                            }
                }
            }
            msg(result);
        }
        else if (!strcmp(_argv[1],"hide")) {
            if (interest == CONSTE_LINE)
                strcpy(result,"hide constellation lines : ");
            else
                strcpy(result,"hide constellation figures : ");
            for (j=2;j<_argc;j++) {
                if (!strcmp(_argv[j],"all")) {
                    for (i=0;i<cs->num_constes;i++)
                        if (cs->constes[i].state & interest)
                            cs->constes[i].state -= interest;
                    strcat(result,"all ");
                    break;
                } else {
                    for (i=0;i<cs->num_constes;i++)
                        if (!strncmp(_argv[j],cs->constes[i].name,strlen(_argv[j])))
                            if (cs->constes[i].state & interest) {
                                cs->constes[i].state -= interest;
                                strcat(result,cs->constes[i].name);
                                strcat(result," ");
                            }
                }
            }
            msg(result);
        }
        else if (!strcmp(_argv[1],"color")) {
            if (interest == CONSTE_LINE) {
                if (_argc >= 4) {
		    cs->line_color[0] = atof(_argv[2]);
		    cs->line_color[1] = atof(_argv[3]);
		    cs->line_color[2] = atof(_argv[4]);
		}
		msg("lines color %.3f %.3f %.3f (RGB constellation line color)",
                    cs->fig_color[0],cs->fig_color[1],cs->fig_color[2]);
            } else {
                if (_argc >= 4) {
		    cs->fig_color[0] = atof(_argv[2]);
		    cs->fig_color[1] = atof(_argv[3]);
		    cs->fig_color[2] = atof(_argv[4]);
		}
		msg("figures color %.3f %.3f %.3f (RGB constellation figure color)",
		    cs->fig_color[0],cs->fig_color[1],cs->fig_color[2]);
            }
        }
    }
    return 1;
}

#endif /* USE_CONSTE */
