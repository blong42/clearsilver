#include <jni.h>
#include "org_clearsilver_HDF.h"

#include "cs_config.h"
#include "util/neo_err.h"
#include "util/neo_misc.h"
#include "util/neo_str.h"
#include "util/neo_hdf.h"
#include "cgi/cgi.h"
#include "cgi/cgiwrap.h"
#include "cgi/date.h"
#include "cgi/html.h"


jfieldID _hdfobjFldID = NULL;

int jNeoErr (JNIEnv *env, NEOERR *err) {
  STRING str;
  jclass newExcCls = (*env)->FindClass(env, "java/lang/RuntimeException");

  if (newExcCls == 0) {
    // unable to find proper class!
    return;
  }

  string_init (&str);
  if (nerr_match(err, NERR_PARSE))
  {
    nerr_error_string (err, &str);
    (*env)->ThrowNew(env, newExcCls, str.buf);
  }
  else
  {
    nerr_error_traceback (err, &str);
    (*env)->ThrowNew(env, newExcCls, str.buf);
  }
  string_clear (&str);

  return 0;
}

JNIEXPORT jint JNICALL Java_org_clearsilver_HDF__1init
 (JNIEnv *env, jobject obj) {
  HDF *hdf = NULL;
  NEOERR *err;

  //  if (!_hdfobjFldID) {
  //    jclass objClass = (*env)->GetObjectClass(env,obj);
  //    _hdfobjFldID = (*env)->GetFieldID(env,objClass,"hdfptr","i");
  //  }

  err = hdf_init(&hdf);
  if (err) return jNeoErr(env,err);
  return (jint) hdf;
}

JNIEXPORT void JNICALL Java_org_clearsilver_HDF__1dealloc
(JNIEnv *env, jclass objClass, jint hdf_obj_ptr) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  hdf_destroy(&hdf);
}

JNIEXPORT jint JNICALL Java_org_clearsilver_HDF__1getIntValue
 (JNIEnv *env, jclass objClass, 
  jint hdf_obj_ptr, jstring j_hdfname, jint default_value) {

  HDF *hdf = (HDF *)hdf_obj_ptr;
  int r;
  const char *hdfname;

  if (!j_hdfname) { return 0; } // throw
  
  hdfname = (*env)->GetStringUTFChars(env,j_hdfname,0);

  r = hdf_get_int_value(hdf,(char *) hdfname,default_value);

  (*env)->ReleaseStringUTFChars(env,j_hdfname,hdfname);
  return r;
  
}

JNIEXPORT jstring JNICALL Java_org_clearsilver_HDF__1getValue
 (JNIEnv *env, jclass objClass, 
  jint hdf_obj_ptr, jstring j_hdfname, jstring j_default_value) {

  HDF *hdf = (HDF *)hdf_obj_ptr;
  const char *r;
  const char *hdfname;
  const char *default_value;

  if (!j_hdfname || !j_default_value) { return 0; } // throw
  
  hdfname = (*env)->GetStringUTFChars(env,j_hdfname,0);
  default_value = (*env)->GetStringUTFChars(env,j_default_value,0);

  r = hdf_get_value(hdf,(char *)hdfname,(char *)default_value);

  (*env)->ReleaseStringUTFChars(env,j_hdfname,hdfname);
  return (r ? (*env)->NewStringUTF(env,r) : 0);
}



JNIEXPORT void JNICALL Java_org_clearsilver_HDF__1setValue
 (JNIEnv *env, jclass objClass,
  jint hdf_obj_ptr, jstring j_hdfname, jstring j_value) {
    HDF *hdf = (HDF *)hdf_obj_ptr;
    NEOERR *err;
    const char *hdfname;
    const char *value;

    if (!j_hdfname || !j_value) { return; }

    hdfname = (*env)->GetStringUTFChars(env,j_hdfname,0);
    value   = (*env)->GetStringUTFChars(env,j_value,0);

    err = hdf_set_value(hdf, (char *)hdfname,(char *)value);

    (*env)->ReleaseStringUTFChars(env, j_hdfname, hdfname);
    (*env)->ReleaseStringUTFChars(env, j_value, value);

}

JNIEXPORT jstring JNICALL Java_org_clearsilver_HDF__1dump
    (JNIEnv *env, jclass objClass,
	jint hdf_obj_ptr) {
    HDF *hdf = (HDF *)hdf_obj_ptr;
    NEOERR *err;
    STRING str;
    jstring retval;

    string_init(&str);
    err = hdf_dump_str(hdf, NULL, 0, &str);
    if (err) {} // throw exception
    retval =  (*env)->NewStringUTF(env,str.buf);
    string_clear(&str);

    return retval;
}
