#include "pview_funcs.h"

int pp_parse_args( struct stuff **, int argc, char *argv[], char *fromfname, void * );
void pp_clk_init();
void readrc( struct stuff **stp );

GLuint pviewPickbuffer[16384];

#if defined(_MSC_VER) && defined(_DEBUG)
void plugin_init();
#else //_MSC_VER
extern "C" {
void plugin_init();
};
#endif //_MSC_VER

char _pickmsg[1024] = "";

#ifdef EMBEDHACK
std::string _pviewmsg;
#endif //EMBEDHACK

void pviewInit(const char *_filename)
{
    pp_clk_init();

    ppui.view = new Fl_Gview(0,0,100,100,"Pview");

    static Point black = {0,0,0};
    ppui.view->bgcolor(&black);

    parti_add_commands(pp_parse_args,"partiview",NULL);
//    parti_add_reader( pp_read, "partiview", NULL );
    plugin_init();
//    pp_ui_init();
//  ppui.view->add_drawer( drawjunk, NULL, NULL, NULL, 0 );
    ppui.view->pickbuffer(COUNT(pviewPickbuffer),pviewPickbuffer);

    ppui.view->zspeed = 5;
    ppui.view->farclip(2500);
    ppui.censize = 1.0;
    ppui.pickrange = 3.5;

    ppui.view->movingtarget(0);
    ppui.view->msg = msg;

    ppui.playspeed = 1;
//  ppui.playframe->lstep(10);

    parti_object("g1",NULL,1);
    readrc(&ppui.st);

    if (_filename)
        specks_read(&ppui.st,findfile(NULL,(char *)_filename));

//  parti_set_timebase( ppui.st, 0.0 );
//  parti_set_timestep( ppui.st, 0.0 );
//  parti_set_running( ppui.st, 0 );
//  parti_set_fwd( ppui.st, 1 );

//  ppui.view->set_swap_func(swap);

//    ppui.view->notifier( pp_viewchanged, ppui.st );
//    ppui_refresh( ppui.st );
}

void pviewEval(const char *_command)
{
    char command[1024];
    char *start;
    char *end;
    strcpy(command,_command);
    start = command;

    do
    {
        end = strchr(start,';');
        if (end) {
            *end = '\0';
            end++;
        }

        if (strlen(start) <= 0) break;

        #define MAXARGS 128
        char *av[MAXARGS];
        int  ac;
        char *s = start;
        for (ac = 0; ac < MAXARGS-1; ac++) {
            av[ac] = strtok(s, " \t\n");
            if (av[ac] == NULL) break;
            s = NULL;
        }
        av[ac] = NULL;
        specks_parse_args(&ppui.st,ac,av);

        start = end;

    } while (end);

    return;
}

void _pview_pickmsg(const char *_fmt, ... )
{
    va_list args;
    va_start(args,_fmt);
    vsprintf(_pickmsg,_fmt,args);
    va_end(args);
}

bool pviewPicked(void)
{
    if (strlen(_pickmsg)>0)
        return true;

    return false;
}

#ifdef EMBEDHACK
bool pviewMessage(void)
{
    return !_pviewmsg.empty();
}
#endif //EMBEDHACK
