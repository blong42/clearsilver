// Copyright 2006 Brandon Long 
// All Rights Reserved.
//
// This code is made available under the terms of the ClearSilver License.
// http://www.clearsilver.net/license.hdf

#ifndef __J_NEO_UTIL_H_
#define __J_NEO_UTIL_H_ 1

typedef struct _fileload_info {
  JNIEnv *env;
  jobject fl_obj;
  HDF *hdf;
  jmethodID fl_method;
} FILELOAD_INFO;

NEOERR *jni_fileload_cb(void *ctx, HDF *hdf, const char *filename,
    char **contents);

#endif  // __J_NEO_UTIL_H_
