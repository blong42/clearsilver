#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ClearSilver.h"

/* #define DEBUG_MODE 1
 */

typedef struct {
  HDF*      hdf;
  NEOERR* err; 	
} perlHDF; 

typedef struct {
  CSPARSE* cs;
  NEOERR* err;
} perlCS;	

typedef perlHDF* ClearSilver__HDF;
typedef perlCS* ClearSilver__CS;

static char* g_sort_func_name;

static void debug(char* fmt, ...)
{
#ifdef DEBUG_MODE
  va_list argp;
  va_start(argp, fmt);
  vprintf(fmt, argp);
  va_end(argp);
#endif
}

static NEOERR *output (void *ctx, char *s)
{
  sv_catpv((SV*)ctx, s);

  return STATUS_OK;
}

static int sortFunction(const void* in_a, const void* in_b)
{
  HDF** hdf_a;
  HDF** hdf_b;
  perlHDF a, b;
  SV* sv_a;
  SV* sv_b;	
  int count;	
  int ret;

  dSP;

  hdf_a = (HDF**)in_a;
  hdf_b = (HDF**)in_b;

  /* convert to a type Perl can access */
  a.hdf = *hdf_a;
  a.err = STATUS_OK;
  b.hdf = *hdf_b;
  b.err = STATUS_OK;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  sv_a = sv_newmortal();
  sv_setref_pv(sv_a, "ClearSilver::HDF", (void*)&a);

  sv_b = sv_newmortal();
  sv_setref_pv(sv_b, "ClearSilver::HDF", (void*)&b);

  XPUSHs(sv_a);
  XPUSHs(sv_b);

  PUTBACK;

  count = call_pv(g_sort_func_name, G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("Big trouble\n");

  PUTBACK;

  ret = POPi;

  FREETMPS;
  LEAVE;

  return ret;
}





MODULE = ClearSilver		PACKAGE = ClearSilver::HDF	PREFIX = perlhdf_

ClearSilver::HDF
perlhdf_new(self)
        char* self
    PREINIT:	
	ClearSilver__HDF hdf;
    CODE:
	debug("%s\n", self);
	hdf = (ClearSilver__HDF)malloc(sizeof(perlHDF));
	if (!hdf) {
	  RETVAL = NULL;
	} else {
	  hdf->err = hdf_init(&(hdf->hdf));
	  RETVAL = hdf;
	}
    OUTPUT:
        RETVAL

void
perlhdf_DESTROY(hdf)
        ClearSilver::HDF hdf;
    CODE:
        debug("hdf_DESTROY:%x\n", hdf);
        hdf_destroy(&(hdf->hdf));


int
perlhdf_setValue(hdf, key, value)
	ClearSilver::HDF hdf
	char* key
	char* value 
    CODE:
        hdf->err = hdf_set_value(hdf->hdf, key, value);
	if (hdf->err == STATUS_OK) {
	    RETVAL = 0;
	} else {
	    RETVAL = 1;
	}	
    OUTPUT:
        RETVAL


char*
perlhdf_getValue(hdf, key, default_value)
	ClearSilver::HDF hdf
	char* key
	char* default_value
    CODE:
        RETVAL = hdf_get_value(hdf->hdf, key, default_value);
    OUTPUT:
        RETVAL


int
perlhdf_copy(hdf, name, src);
        ClearSilver::HDF hdf
        char* name
        ClearSilver::HDF src
    CODE:
        hdf->err = hdf_copy(hdf->hdf, name, src->hdf);
        if (hdf->err == STATUS_OK) {
            RETVAL = 0;
        } else {
            RETVAL = 1;
        }
    OUTPUT:
        RETVAL

int
perlhdf_readFile(hdf, filename)
	ClearSilver::HDF hdf
	char* filename
    CODE:
        hdf->err = hdf_read_file(hdf->hdf, filename);
	if (hdf->err == STATUS_OK) {
	    RETVAL = 1;
	} else {
	    RETVAL = 0;
	}	
    OUTPUT:
        RETVAL

int
perlhdf_writeFile(hdf, filename)
       ClearSilver::HDF hdf
       char* filename
    CODE:
        hdf->err = hdf_write_file(hdf->hdf, filename);
       if (hdf->err == STATUS_OK) {
           RETVAL = 1;
       } else {
           RETVAL = 0;
       }
    OUTPUT:
        RETVAL

ClearSilver::HDF
perlhdf_getObj(hdf, name)
	ClearSilver::HDF hdf;
	char* name
    PREINIT:	
	HDF* tmp_hdf;
	perlHDF* perlhdf;
    CODE:
	do {
	    tmp_hdf = hdf_get_obj(hdf->hdf, name);
	    if (!tmp_hdf) {
	        RETVAL = NULL;
		break;
	    }
	    perlhdf = (perlHDF*)malloc(sizeof(perlHDF));
	    if (!perlhdf) {
	        RETVAL = NULL;
	        break;
	    }
            perlhdf->hdf = tmp_hdf;
	    perlhdf->err = STATUS_OK;
	    RETVAL = perlhdf;
	} while (0);
    OUTPUT:
        RETVAL

ClearSilver::HDF
perlhdf_objChild(hdf)
	ClearSilver::HDF hdf;
    PREINIT:	
	HDF* tmp_hdf;
	perlHDF* child;
    CODE:
	do {
	    tmp_hdf = hdf_obj_child(hdf->hdf);
	    if (!tmp_hdf) {
	        RETVAL = NULL;
		break;
	    }
	    child = (perlHDF*)malloc(sizeof(perlHDF));
	    if (!child) {
	        RETVAL = NULL;
	        break;
	    }
            child->hdf = tmp_hdf;
	    child->err = STATUS_OK;
	    RETVAL = child;
	} while (0);
    OUTPUT:
        RETVAL
		

ClearSilver::HDF
perlhdf_getChild(hdf, name)
	ClearSilver::HDF hdf;
	char* name;
    PREINIT:	
	HDF* tmp_hdf;
	perlHDF* child;
    CODE:
	do {
	    tmp_hdf = hdf_get_child(hdf->hdf, name);
	    if (!tmp_hdf) {
	        RETVAL = NULL;
		break;
	    }
	    child = (perlHDF*)malloc(sizeof(perlHDF));
	    if (!child) {
	        RETVAL = NULL;
	        break;
	    }
            child->hdf = tmp_hdf;
	    child->err = STATUS_OK;
	    RETVAL = child;
	} while (0);
    OUTPUT:
        RETVAL

char*
perlhdf_objValue(hdf)
	ClearSilver::HDF hdf;
    CODE:
	RETVAL = hdf_obj_value(hdf->hdf);
    OUTPUT:
        RETVAL  

char*
perlhdf_objName(hdf)
	ClearSilver::HDF hdf;
    CODE:
	RETVAL = hdf_obj_name(hdf->hdf);
    OUTPUT:
        RETVAL  

ClearSilver::HDF
perlhdf_objNext(hdf)
	ClearSilver::HDF hdf;
    PREINIT:	
	HDF* tmp_hdf;
	perlHDF* next;
    CODE:
	do {
	    tmp_hdf = hdf_obj_next(hdf->hdf);
	    if (!tmp_hdf) {
	        RETVAL = NULL;
		break;
	    }
	    next = (perlHDF*)malloc(sizeof(perlHDF));
	    if (!next) {
	      RETVAL = NULL;
	      break;
            }
	    next->hdf = tmp_hdf;
	    next->err = STATUS_OK;
	    RETVAL = next;
	} while (0);
    OUTPUT:
        RETVAL

int
perlhdf_sortObj(hdf, func_name)
	ClearSilver::HDF hdf;
	char* func_name;
    PREINIT:	
        NEOERR* err;
    CODE:
	g_sort_func_name = func_name;
        err = hdf_sort_obj(hdf->hdf, sortFunction);
        RETVAL = 0;
    OUTPUT:
        RETVAL


int
perlhdf_setSymlink(hdf, src, dest)
	ClearSilver::HDF hdf;
	char* src;
	char* dest;
    PREINIT:
	NEOERR* err;
    CODE:
      	err = hdf_set_symlink (hdf->hdf, src, dest);
       	if (err == STATUS_OK) {
       	    RETVAL = 1;
       	} else {
       	    RETVAL = 0;
       	}
    OUTPUT:
        RETVAL


int
perlhdf_removeTree(hdf, name)
	ClearSilver::HDF hdf;
	char* name;
    PREINIT:	
        NEOERR* err;
    CODE:
        err = hdf_remove_tree(hdf->hdf, name);
       	if (err == STATUS_OK) {
       	    RETVAL = 1;
       	} else {
       	    RETVAL = 0;
       	}
    OUTPUT:
        RETVAL


MODULE = ClearSilver		PACKAGE = ClearSilver::CS	PREFIX = perlcs_

ClearSilver::CS
perlcs_new(self, hdf)
	char* self
        ClearSilver::HDF hdf;
    PREINIT:
        perlCS* cs;
    CODE:
	cs = (perlCS*)malloc(sizeof(perlCS));
	if (!cs) {
	  RETVAL = NULL;
	} else {
	  cs->err = cs_init(&(cs->cs), hdf->hdf);
	  if (cs->err == STATUS_OK) {
	    cs->err = cgi_register_strfuncs(cs->cs);
	  }
	  RETVAL = cs;
	}
    OUTPUT:
        RETVAL

void
perlcs_DESTROY(cs)
	ClearSilver::CS cs;
    CODE:
	debug("perlcs_DESTROY() is called\n");
	cs_destroy(&(cs->cs));

void
perlcs_displayError(cs)
	ClearSilver::CS cs;
    CODE:
	nerr_log_error(cs->err);

char *
perlcs_render(cs)
	ClearSilver::CS cs
    CODE:
    {
	SV *str = newSV(0);
	cs->err = cs_render(cs->cs, str, output);
	if (cs->err == STATUS_OK) {
	  ST(0) = sv_2mortal(str);
	} else {
	  SvREFCNT_dec(str);
	  ST(0) = &PL_sv_undef;
	}
	XSRETURN(1);
    }

int
perlcs_parseFile(cs, cs_file)
        ClearSilver::CS cs
	char* cs_file
    CODE:
	do {
	    cs->err =  cs_parse_file(cs->cs, cs_file);
	    if (cs->err != STATUS_OK) {
	        cs->err = nerr_pass(cs->err);
		RETVAL = 0;
		break;
	    }
	    RETVAL = 1;
        } while (0);
    OUTPUT:
        RETVAL

int
perlcs_parseString(cs, in_str)
        ClearSilver::CS cs
	char* in_str
    PREINIT:
	char* cs_str;
	int len;
    CODE:
	do {
	    len = strlen(in_str);
	    cs_str = (char *)malloc(len);
	    if (!cs_str) {
	        RETVAL = 0;
		break;
	    }
	    strcpy(cs_str, in_str);
            cs->err =  cs_parse_string(cs->cs, cs_str, len);
	    if (cs->err != STATUS_OK)
		RETVAL = 0;
	    RETVAL = 1;
       } while (0);
    OUTPUT:
        RETVAL



