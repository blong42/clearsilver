#include <string.h>

#include <jni.h>
#include "org_clearsilver_CS.h"

#include "util/neo_err.h"
#include "util/neo_misc.h"
#include "util/neo_str.h"
#include "util/neo_hdf.h"
#include "cgi/cgi.h"
#include "cgi/cgiwrap.h"
#include "cgi/date.h"
#include "cgi/html.h"
#include "cs/cs.h"


jfieldID _csobjFldID = NULL;

JNIEXPORT jint JNICALL Java_org_clearsilver_CS__1init
 (JNIEnv *env, jobject obj, jint hdf_obj_ptr) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  CSPARSE *cs = NULL;
  NEOERR *err;

  //  if (!_hdfobjFldID) {
  //    jclass objClass = (*env)->GetObjectClass(env,obj);
  //    _hdfobjFldID = (*env)->GetFieldID(env,objClass,"hdfptr","i");
  //  }

  err = cs_init(&cs,hdf);
  if (err) { } // throw error
  return (jint) cs;
}

JNIEXPORT void JNICALL Java_org_clearsilver_CS__1dealloc
(JNIEnv *env, jclass objClass, jint cs_obj_ptr) {
  CSPARSE *cs = (CSPARSE *)cs_obj_ptr;
  cs_destroy(&cs);
}


JNIEXPORT void JNICALL Java_org_clearsilver_CS__1parseFile 
    (JNIEnv *env, jclass objClass, jint cs_obj_ptr, 
     jstring j_filename) {
  
  CSPARSE *cs = (CSPARSE *)cs_obj_ptr;
  NEOERR *err;
  const char *filename;

  if (!j_filename) { return; } // throw
  
  filename = (*env)->GetStringUTFChars(env,j_filename,0);

  err = cs_parse_file(cs,(char *)filename);
  if (err) {} // throw error

  (*env)->ReleaseStringUTFChars(env,j_filename,filename);

}

JNIEXPORT void JNICALL Java_org_clearsilver_CS__1parseStr
(JNIEnv *env, jclass objClass, jint cs_obj_ptr,
 jstring j_contentstring) {

  CSPARSE *cs = (CSPARSE *)cs_obj_ptr;
  NEOERR *err;
  char *ms;
  int len;
  const char *contentstring;

  if (!j_contentstring) { return; } // throw
  
  contentstring = (*env)->GetStringUTFChars(env,j_contentstring,0);

  ms = strdup(contentstring);
  if (ms == NULL) {} // throw error no memory
  len = strlen(ms);

  err = cs_parse_string(cs,ms,len);
  if (err) {} // throw error

  (*env)->ReleaseStringUTFChars(env,j_contentstring,contentstring);

}

static NEOERR *render_cb (void *ctx, char *buf)
{
  STRING *str= (STRING *)ctx;

  return nerr_pass(string_append(str, buf));
}


JNIEXPORT jstring JNICALL Java_org_clearsilver_CS__1render
(JNIEnv *env, jclass objClass, jint cs_obj_ptr) {
  CSPARSE *cs = (CSPARSE *)cs_obj_ptr;
  STRING str;
  NEOERR *err;
  jstring retval;
  
  string_init(&str);
  err = cs_render(cs,&str,render_cb);
  if (err) {} // throw error
  
  retval = (*env)->NewStringUTF(env,str.buf);
  string_clear(&str);

  return retval;
  
}
