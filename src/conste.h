#ifndef _CONSTE_H_
#define _CONSTE_H_

int conste_read(struct stuff **stp, int argc, char *argv[], char *filename, void *);
int conste_parse_args(struct stuff **_stp,int _argc,char *_argv[], char *filename, void *);

#ifdef __cplusplus
extern "C" {
 extern void conste_init();
 void conste_draw(struct stuff *st);
}
#else
extern void conste_init();
void conste_draw(struct stuff *st);
#endif

#endif /* _CONSTE_H_ */
