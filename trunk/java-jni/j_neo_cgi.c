#include <jni.h>
#include "CGI.h"

#include "cs_config.h"
#include "util/neo_err.h"
#include "util/neo_misc.h"
#include "util/neo_str.h"
#include "util/neo_hdf.h"
#include "cgi/cgi.h"
#include "cgi/cgiwrap.h"
#include "cgi/date.h"
#include "cgi/html.h"


jfieldID _cgiobjFldID = NULL;

int jNeoErr (JNIEnv *env, NEOERR *err);

JNIEXPORT jint JNICALL Java_CGI__1init
 (JNIEnv *env, jobject obj) {
  CGI *cgi = NULL;
  NEOERR *err;

  if (!_cgiobjFldID) {
    jclass objClass = (*env)->GetObjectClass(env,obj);
    _cgiobjFldID = (*env)->GetFieldID(env,objClass,"_cgiobj","I");
  }

  err = cgi_init(&cgi,NULL);
  if (err) return jNeoErr(env,err);
  return (jint) cgi;
 }

JNIEXPORT void JNICALL Java_CGI_parse
 (JNIEnv *env, jobject obj) {
  NEOERR *err;
  CGI *cgi = (CGI *)((*env)->GetIntField(env,obj,_cgiobjFldID));

  err = cgi_parse(cgi);
  if (err) { jNeoErr(env,err); return; }

}

