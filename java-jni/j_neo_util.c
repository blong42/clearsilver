#include <string.h>
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

#include "j_neo_util.h"

void throwException(JNIEnv *env, const char* class_name, const char *message) {
  jclass ex_class = (*env)->FindClass(env, class_name);
  if (ex_class == NULL) {
    // Unable to find proper class!
    return;
  }
  (*env)->ThrowNew(env, ex_class, message);
}

void throwNullPointerException(JNIEnv *env, const char *message) {
  throwException(env, "java/lang/NullPointerException", message);
}

void throwRuntimeException(JNIEnv *env, const char *message) {
  throwException(env, "java/lang/RuntimeException", message);
}

void throwIOException(JNIEnv *env, const char *message) {
  throwException(env, "java/io/IOException", message);
}

void throwFileNotFoundException(JNIEnv *env, const char *message) {
  throwException(env, "java/io/FileNotFoundException", message);
}

void throwOutOfMemoryError(JNIEnv *env, const char *message) {
  throwException(env, "java/lang/OutOfMemoryError", message);
}

// Throws a runtime exception back to the Java VM appropriate for the type of
// error and frees the NEOERR that is passed in.
// TODO: throw more specific exceptions for errors like NERR_IO and NERR_NOMEM
int jNeoErr(JNIEnv *env, NEOERR *err) {
  STRING str;

  string_init(&str);
  if (nerr_match(err, NERR_PARSE)) {
    nerr_error_string(err, &str);
    throwRuntimeException(env, str.buf);
  } else if (nerr_match(err, NERR_IO)) {
    nerr_error_string(err, &str);
    throwIOException(env, str.buf);
  } else if (nerr_match(err, NERR_NOMEM)) {
    nerr_error_string(err, &str);
    throwOutOfMemoryError(env, str.buf);
  } else {
    nerr_error_traceback(err, &str);
    throwRuntimeException(env, str.buf);
  }

  nerr_ignore(&err);  // free err, otherwise it would leak
  string_clear(&str);

  return 0;
}

JNIEXPORT jint JNICALL Java_org_clearsilver_HDF__1init(
    JNIEnv *env, jclass objClass) {
  HDF *hdf = NULL;
  NEOERR *err;

  err = hdf_init(&hdf);
  if (err != STATUS_OK) {
    return jNeoErr(env, err);
  }
  return (jint) hdf;
}

JNIEXPORT void JNICALL Java_org_clearsilver_HDF__1dealloc(
    JNIEnv *env, jclass objClass, jint hdf_obj_ptr) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  hdf_destroy(&hdf);
}

JNIEXPORT jint JNICALL Java_org_clearsilver_HDF__1getIntValue(
    JNIEnv *env, jclass objClass, jint hdf_obj_ptr, jstring j_hdfname,
     jint default_value) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  int r;
  const char *hdfname;

  if (!j_hdfname) {
    throwNullPointerException(env, "hdfname argument was null");
    return 0;
  }

  hdfname = (*env)->GetStringUTFChars(env,j_hdfname, 0);

  r = hdf_get_int_value(hdf, hdfname, default_value);

  (*env)->ReleaseStringUTFChars(env,j_hdfname,hdfname);
  return r;
}

JNIEXPORT jstring JNICALL Java_org_clearsilver_HDF__1getValue(
    JNIEnv *env, jclass objClass, jint hdf_obj_ptr, jstring j_hdfname,
    jstring j_default_value) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  const char *r;
  const char *hdfname;
  const char *default_value;
  jstring retval;

  if (!j_hdfname) {
    throwNullPointerException(env, "hdfname argument was null");
    return 0;
  }
  hdfname = (*env)->GetStringUTFChars(env,j_hdfname,0);
  if (!j_default_value) {
    default_value = NULL;
  } else {
    default_value = (*env)->GetStringUTFChars(env, j_default_value, 0);
  }

  r = hdf_get_value(hdf, hdfname, default_value);

  (*env)->ReleaseStringUTFChars(env, j_hdfname, hdfname);
  retval = (r ? (*env)->NewStringUTF(env, r) : 0);
  if (default_value) {
    (*env)->ReleaseStringUTFChars(env, j_default_value, default_value);
  }
  return retval;
}

JNIEXPORT void JNICALL Java_org_clearsilver_HDF__1setValue(
    JNIEnv *env, jclass objClass,
    jint hdf_obj_ptr, jstring j_hdfname, jstring j_value) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  NEOERR *err;
  const char *hdfname;
  const char *value;

  if (!j_hdfname) {
    throwNullPointerException(env, "hdfname argument was null");
    return;
  }
  hdfname = (*env)->GetStringUTFChars(env, j_hdfname, 0);
  if (j_value) {
    value = (*env)->GetStringUTFChars(env, j_value, 0);
  } else {
    value = NULL;
  }
  err = hdf_set_value(hdf, hdfname, value);

  (*env)->ReleaseStringUTFChars(env, j_hdfname, hdfname);
  if (value) {
    (*env)->ReleaseStringUTFChars(env, j_value, value);
  }

  if (err != STATUS_OK) {
    // Throw an exception
    jNeoErr(env, err);
  }
}

JNIEXPORT void JNICALL Java_org_clearsilver_HDF__1removeTree(
    JNIEnv *env, jclass objClass,
    jint hdf_obj_ptr, jstring j_hdfname) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  NEOERR *err;
  const char *hdfname;

  if (!j_hdfname) {
    throwNullPointerException(env, "hdfname argument was null");
    return;
  }
  hdfname = (*env)->GetStringUTFChars(env, j_hdfname, 0);
  err = hdf_remove_tree(hdf, hdfname);

  (*env)->ReleaseStringUTFChars(env, j_hdfname, hdfname);

  if (err != STATUS_OK) {
    // Throw an exception
    jNeoErr(env, err);
  }
}

JNIEXPORT void JNICALL Java_org_clearsilver_HDF__1setSymLink(
    JNIEnv *env, jclass objClass,
    jint hdf_obj_ptr, jstring j_hdf_name_src, jstring j_hdf_name_dest) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  NEOERR *err;
  const char *hdf_name_src;
  const char *hdf_name_dest;

  if (!j_hdf_name_src) {
    throwNullPointerException(env, "hdf_name_src argument was null");
    return;
  }
  hdf_name_src = (*env)->GetStringUTFChars(env, j_hdf_name_src, 0);

  if (!j_hdf_name_dest) {
    throwNullPointerException(env, "hdf_name_dest argument was null");
    return;
  }
  hdf_name_dest = (*env)->GetStringUTFChars(env, j_hdf_name_dest, 0);

  err = hdf_set_symlink(hdf, hdf_name_src, hdf_name_dest);

  (*env)->ReleaseStringUTFChars(env, j_hdf_name_src, hdf_name_src);
  (*env)->ReleaseStringUTFChars(env, j_hdf_name_dest, hdf_name_dest);

  if (err != STATUS_OK) {
    // Throw an exception
    jNeoErr(env, err);
  }
}

JNIEXPORT jstring JNICALL Java_org_clearsilver_HDF__1dump(
    JNIEnv *env, jclass objClass,
    jint hdf_obj_ptr) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  NEOERR *err;
  STRING str;
  jstring retval;

  string_init(&str);
  err = hdf_dump_str(hdf, NULL, 0, &str);
  if (err != STATUS_OK) {
    // Throw an exception
    jNeoErr(env, err);
    retval = NULL;
  } else {
    retval =  (*env)->NewStringUTF(env,str.buf);
  }
  string_clear(&str);

  return retval;
}

NEOERR *jni_fileload_cb(void *ctx, HDF *hdf, const char *filename,
    char **contents) {
  FILELOAD_INFO *info = (FILELOAD_INFO *)ctx;
  jstring filename_str;
  jstring loaded_string;
  const char *c_loaded_string;

  // We assume that the hdf passed back is actually the hdf we already
  // have...
  if (hdf != info->hdf)
    return nerr_raise(NERR_ASSERT,
        "jni_fileload_cb: passed HDF pointer doesn't match hdf_obj_ptr");

  filename_str = (*info->env)->NewStringUTF(info->env, filename);
  (*info->env)->NewLocalRef(info->env, filename_str);

  loaded_string = (*info->env)->CallObjectMethod(info->env, info->fl_obj,
      info->fl_method, filename_str);
  if ((*info->env)->ExceptionCheck(info->env)) {
    (*info->env)->ExceptionDescribe(info->env);
    (*info->env)->ExceptionClear(info->env);
    return nerr_raise(NERR_ASSERT,
        "jni_fileload_cb: HDF.fileLoad returned exception, see STDERR");
  }
  c_loaded_string = (*info->env)->GetStringUTFChars(info->env, loaded_string,
      0);
  if (c_loaded_string) {
    *contents = strdup(c_loaded_string);
  }
  (*info->env)->ReleaseStringUTFChars(info->env, loaded_string,
                                      c_loaded_string);

  return STATUS_OK;
}

JNIEXPORT jboolean JNICALL Java_org_clearsilver_HDF__1readFile(
    JNIEnv *env, jobject objClass, jint hdf_obj_ptr, jstring j_filename,
    jboolean use_cb) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  NEOERR *err;
  const char *filename;
  jboolean retval;
  FILELOAD_INFO fl_info;

  if (use_cb == JNI_TRUE) {
    jclass hdfClass;

    fl_info.env = env;
    fl_info.fl_obj = objClass;
    fl_info.hdf = hdf;

    hdfClass = (*env)->GetObjectClass(env, objClass); 
    if (hdfClass == NULL) return JNI_FALSE;

    fl_info.fl_method = (*env)->GetMethodID(env, hdfClass,
        "fileLoad", "(Ljava/lang/String;)Ljava/lang/String;");
    if (fl_info.fl_method == NULL) return JNI_FALSE;

    hdf_register_fileload(hdf, &fl_info, jni_fileload_cb);
  }

  filename = (*env)->GetStringUTFChars(env, j_filename, 0);
  err = hdf_read_file(hdf, filename);
  (*env)->ReleaseStringUTFChars(env, j_filename, filename);
  if (use_cb == JNI_TRUE) hdf_register_fileload(hdf, NULL, NULL);
  if (err != STATUS_OK) {
    // Throw an exception.  jNeoErr handles all types of errors other than
    // NOT_FOUND, since that can mean different things in different contexts.
    // In this context, it means "file not found".
    if (nerr_match(err, NERR_NOT_FOUND)) {
      STRING str;
      string_init(&str);
      nerr_error_string(err, &str);
      throwFileNotFoundException(env, str.buf);
      string_clear(&str);
    } else {
      jNeoErr(env, err);
    }
  }
  retval = (err == STATUS_OK);
  return retval;
}

JNIEXPORT jboolean JNICALL Java_org_clearsilver_HDF__1writeFile(
    JNIEnv *env, jobject objClass, jint hdf_obj_ptr, jstring j_filename) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  NEOERR *err;
  const char *filename;
  jboolean retval;

  filename = (*env)->GetStringUTFChars(env, j_filename, 0);
  err = hdf_write_file(hdf, filename);
  (*env)->ReleaseStringUTFChars(env, j_filename, filename);
  if (err != STATUS_OK) {
    jNeoErr(env, err);
  }
  retval = (err == STATUS_OK);
  return retval;
}

JNIEXPORT jboolean JNICALL Java_org_clearsilver_HDF__1writeFileAtomic(
    JNIEnv *env, jobject objClass, jint hdf_obj_ptr, jstring j_filename) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  NEOERR *err;
  const char *filename;
  jboolean retval;

  filename = (*env)->GetStringUTFChars(env, j_filename, 0);
  err = hdf_write_file_atomic(hdf, filename);
  (*env)->ReleaseStringUTFChars(env, j_filename, filename);
  if (err != STATUS_OK) {
    jNeoErr(env, err);
  }
  retval = (err == STATUS_OK);
  return retval;
}

JNIEXPORT jboolean JNICALL Java_org_clearsilver_HDF__1readString(
    JNIEnv *env, jobject objClass, jint hdf_obj_ptr, jstring j_data) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  NEOERR *err;
  const char *data;
  jboolean retval;

  data = (*env)->GetStringUTFChars(env, j_data, 0);
  err = hdf_read_string(hdf, data);
  (*env)->ReleaseStringUTFChars(env, j_data, data);
  if (err != STATUS_OK) {
    jNeoErr(env, err);
  }
  retval = (err == STATUS_OK);
  return retval;
}

JNIEXPORT jstring JNICALL Java_org_clearsilver_HDF__1writeString(
    JNIEnv *env, jclass objClass, jint hdf_obj_ptr) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  NEOERR *err;
  char *output = NULL;
  jstring retval = NULL;

  err = hdf_write_string(hdf, &output);
  if (err != STATUS_OK) {
    jNeoErr(env, err);
  } else if (output) {
    retval = (*env)->NewStringUTF(env, output);
    free(output);
  }
  return retval;
}

JNIEXPORT jint JNICALL Java_org_clearsilver_HDF__1getObj(
    JNIEnv *env, jclass objClass, jint hdf_obj_ptr, jstring j_hdf_path) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  HDF *obj_hdf = NULL;
  const char *hdf_path;

  hdf_path = (*env)->GetStringUTFChars(env, j_hdf_path, 0);
  obj_hdf = hdf_get_obj(hdf, (char*)hdf_path);
  (*env)->ReleaseStringUTFChars(env, j_hdf_path, hdf_path);
  return (jint)obj_hdf;
}

JNIEXPORT jint JNICALL Java_org_clearsilver_HDF__1getChild(
    JNIEnv *env, jclass objClass, jint hdf_obj_ptr, jstring j_hdf_path) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  HDF *obj_hdf = NULL;
  const char *hdf_path;

  hdf_path = (*env)->GetStringUTFChars(env, j_hdf_path, 0);
  obj_hdf = hdf_get_child(hdf, (char*)hdf_path);
  (*env)->ReleaseStringUTFChars(env, j_hdf_path, hdf_path);
  return (jint)obj_hdf;
}

JNIEXPORT jint JNICALL Java_org_clearsilver_HDF__1objChild(
    JNIEnv *env, jclass objClass, jint hdf_obj_ptr) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  HDF *child_hdf = NULL;

  child_hdf = hdf_obj_child(hdf);
  return (jint)child_hdf;
}

JNIEXPORT jint JNICALL Java_org_clearsilver_HDF__1objNext(
    JNIEnv *env, jclass objClass, jint hdf_obj_ptr) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  HDF *next_hdf = NULL;

  next_hdf = hdf_obj_next(hdf);
  return (jint)next_hdf;
}

JNIEXPORT jstring JNICALL Java_org_clearsilver_HDF__1objName(
    JNIEnv *env, jclass objClass, jint hdf_obj_ptr) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  char *name;
  jstring retval = NULL;

  name = hdf_obj_name(hdf);
  if (name != NULL) {
    retval =  (*env)->NewStringUTF(env, name);
  }
  return retval;
}

JNIEXPORT jstring JNICALL Java_org_clearsilver_HDF__1objValue(
    JNIEnv *env, jclass objClass, jint hdf_obj_ptr) {
  HDF *hdf = (HDF *)hdf_obj_ptr;
  char *name;
  jstring retval = NULL;

  name = hdf_obj_value(hdf);
  if (name != NULL) {
    retval =  (*env)->NewStringUTF(env, name);
  }
  return retval;
}

JNIEXPORT void JNICALL Java_org_clearsilver_HDF__1copy
  (JNIEnv *env, jclass objClass, jint hdf_dest_ptr, jstring j_hdf_path, 
   jint hdf_src_ptr) {
  HDF *dest = (HDF *)hdf_dest_ptr;
  HDF *src = (HDF *)hdf_src_ptr;
  const char *hdf_path;

  hdf_path = (*env)->GetStringUTFChars(env, j_hdf_path, 0);
  hdf_copy(dest, hdf_path, src);
  (*env)->ReleaseStringUTFChars(env, j_hdf_path, hdf_path);
}

