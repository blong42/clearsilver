#include <string.h>

#include <jni.h>
#include "org_clearsilver_CS.h"

#include "cs_config.h"
#include "util/neo_err.h"
#include "util/neo_misc.h"
#include "util/neo_str.h"
#include "util/neo_hdf.h"
#include "cgi/cgi.h"
#include "cgi/cgiwrap.h"
#include "cgi/date.h"
#include "cgi/html.h"
#include "cs/cs.h"

#include "j_neo_util.h"

jfieldID _csobjFldID = NULL;

static void jErr(JNIEnv *env, char *error_string) {
  jclass newExcCls = (*env)->FindClass(env, "java/lang/RuntimeException");
  if (newExcCls == 0) {
    // unable to find proper class!
    return;
  }

  (*env)->ThrowNew(env, newExcCls, error_string);
}

int jNeoErr (JNIEnv *env, NEOERR *err);

JNIEXPORT jint JNICALL Java_org_clearsilver_CS__1init
 (JNIEnv *env, jobject obj, jint hdf_obj_ptr) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  CSPARSE *cs = NULL;
  NEOERR *err;

  //  if (!_hdfobjFldID) {
  //    jclass objClass = (*env)->GetObjectClass(env,obj);
  //    _hdfobjFldID = (*env)->GetFieldID(env,objClass,"hdfptr","i");
  //  }

  err = cs_init(&cs, hdf);
  if (err != STATUS_OK) return jNeoErr(env,err);
  err = cgi_register_strfuncs(cs);
  if (err != STATUS_OK) return jNeoErr(env,err);

  return (jint) cs;
}

JNIEXPORT void JNICALL Java_org_clearsilver_CS__1dealloc
(JNIEnv *env, jclass objClass, jint cs_obj_ptr) {
  CSPARSE *cs = (CSPARSE *)cs_obj_ptr;
  cs_destroy(&cs);
}


JNIEXPORT void JNICALL Java_org_clearsilver_CS__1parseFile(JNIEnv *env,
    jobject objCS, jint cs_obj_ptr, jstring j_filename, jboolean use_cb) {
  CSPARSE *cs = (CSPARSE *)cs_obj_ptr;
  NEOERR *err;
  const char *filename;
  FILELOAD_INFO fl_info;

  if (!j_filename) { return; } // throw

  if (use_cb == JNI_TRUE) {
    jclass csClass;
    csClass = (*env)->GetObjectClass(env, objCS); 
    if (csClass == NULL) return;
    fl_info.env = env;
    fl_info.fl_obj = objCS;
    fl_info.hdf = cs->hdf;
    fl_info.fl_method = (*env)->GetMethodID(env, csClass,
        "fileLoad", "(Ljava/lang/String;)Ljava/lang/String;");
    if (fl_info.fl_method == NULL) return;
    cs_register_fileload(cs, &fl_info, jni_fileload_cb);
  }
  
  filename = (*env)->GetStringUTFChars(env,j_filename,0);

  err = cs_parse_file(cs,(char *)filename);
  if (use_cb == JNI_TRUE) cs_register_fileload(cs, NULL, NULL);
  if (err != STATUS_OK) { jNeoErr(env,err); return; }


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
  if (ms == NULL) { jErr(env, "parseStr failed"); return; } // throw error no memory
  len = strlen(ms);

  err = cs_parse_string(cs,ms,len);
  if (err) { jNeoErr(env,err); return; }

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
  int ws_strip_level = 0;
  int do_debug = 0;

  // TODO: perhaps we should pass in whether this is html as well...
  do_debug = hdf_get_int_value(cs->hdf, "ClearSilver.DisplayDebug", 0);
  ws_strip_level = hdf_get_int_value(cs->hdf, "ClearSilver.WhiteSpaceStrip", 0);
  
  string_init(&str);
  err = cs_render(cs, &str, render_cb);
  if (err) { 
    string_clear(&str);
    jNeoErr(env,err); 
    return NULL; 
  }

  if (ws_strip_level) {
    cgi_html_ws_strip(&str, ws_strip_level);
  }

  if (do_debug) {
    do {
      err = string_append (&str, "<hr>");
      if (err != STATUS_OK) break;
      err = string_append (&str, "<pre>");
      if (err != STATUS_OK) break;
      err = hdf_dump_str (cs->hdf, NULL, 0, &str);
      if (err != STATUS_OK) break;
      err = string_append (&str, "</pre>");
      if (err != STATUS_OK) break;
    } while (0);
    if (err) { 
      string_clear(&str);
      jNeoErr(env,err); 
      return NULL; 
    }
  }
  
  retval = (*env)->NewStringUTF(env, str.buf);
  string_clear(&str);

  return retval;
}

// Change global HDF
JNIEXPORT void JNICALL Java_org_clearsilver_CS__1setGlobalHdf
(JNIEnv *env, jobject objclass, jint cs_obj_ptr, jint hdf_obj_ptr) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  CSPARSE *cs = (CSPARSE *)cs_obj_ptr;
  cs->global_hdf = hdf;
}
