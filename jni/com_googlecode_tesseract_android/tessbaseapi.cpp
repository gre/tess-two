/*
 * Copyright 2011, Google Inc.
 * Copyright 2011, Robert Theis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <malloc.h>
#include <jni.h>
#include "baseapi.h"
#include "ocrclass.h"
#include "allheaders.h"

static jfieldID field_mNativeData;
static jmethodID method_onProgressValues;

struct native_data_t {
  tesseract::TessBaseAPI api;
  void *data;
  bool debug;

  l_int32 lastProgress;
  bool cancel_ocr;

  JNIEnv *cachedEnv;
  jobject* cachedObject;

  bool isStateValid() {
    if (cancel_ocr == false && cachedEnv != NULL && cachedObject != NULL) {
      return true;
    } else {
      return false;
    }
  }

  void initStateVariables(JNIEnv* env, jobject *object) {
    cancel_ocr = false;
    cachedEnv = env;
    cachedObject = object;
    lastProgress = 0;
  }

  void resetStateVariables() {
    cancel_ocr = false;
    cachedEnv = NULL;
    cachedObject = NULL;
    lastProgress = 0;
  }

  native_data_t() {
    lastProgress = 0;
    data = NULL;
    debug = false;
    cachedEnv = NULL;
    cachedObject = NULL;
    cancel_ocr = false;
  }
};

/**
 * Callback for Tesseract's monitor to cancel recognition.
 */
bool cancelFunc(void* cancel_this, int words) {
  native_data_t *nat = (native_data_t*)cancel_this;
  return nat->cancel_ocr;
}

/**
 * Callback for Tesseract's monitor to update progress.
 */
bool progressJavaCallback(void* progress_this, int progress, int left, int right,
		int top, int bottom) {
  native_data_t *nat = (native_data_t*)progress_this;

  if (nat->isStateValid()) {
    if (progress > nat->lastProgress || left != 0 || right != 0 || top != 0 || bottom != 0) {
      nat->cachedEnv->CallVoidMethod(*(nat->cachedObject), method_onProgressValues, progress,
              (jint) left, (jint) right, (jint) top, (jint) bottom);
      nat->lastProgress = progress;
    }
  }
  return true;
}

static inline native_data_t * get_native_data(JNIEnv *env, jobject object) {
  return (native_data_t *) (env->GetLongField(object, field_mNativeData));
}

#ifdef __cplusplus
extern "C" {
#endif

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  JNIEnv *env;

  if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
    return -1;
  }

  return JNI_VERSION_1_6;
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeClassInit(JNIEnv* env,
                                                                       jclass clazz) {

  field_mNativeData = env->GetFieldID(clazz, "mNativeData", "J");
  method_onProgressValues = env->GetMethodID(clazz, "onProgressValues", "(IIIII)V");
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeConstruct(JNIEnv* env,
                                                                       jobject object) {

  native_data_t *nat = new native_data_t;

  if (nat == NULL) {
    return;
  }

  env->SetLongField(object, field_mNativeData, (jlong) nat);
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeFinalize(JNIEnv* env,
                                                                      jobject object) {

  native_data_t *nat = get_native_data(env, object);

  // Since Tesseract doesn't take ownership of the memory, we keep a pointer in the native
  // code struct. We need to free that pointer when we release our instance of Tesseract or
  // attempt to set a new image using one of the nativeSet* methods.
  if (nat->data != NULL)
    free(nat->data);
  nat->data = NULL;

  if (nat != NULL)
    delete nat;
}

jboolean Java_com_googlecode_tesseract_TessBaseAPI_nativeInit(JNIEnv *env,
                                                                      jobject thiz,
                                                                      jstring dir,
                                                                      jstring lang) {

  native_data_t *nat = get_native_data(env, thiz);

  const char *c_dir = env->GetStringUTFChars(dir, NULL);
  const char *c_lang = env->GetStringUTFChars(lang, NULL);

  jboolean res = JNI_TRUE;

  if (nat->api.Init(c_dir, c_lang)) {
    res = JNI_FALSE;
  } else {
  }

  env->ReleaseStringUTFChars(dir, c_dir);
  env->ReleaseStringUTFChars(lang, c_lang);

  return res;
}

jboolean Java_com_googlecode_tesseract_TessBaseAPI_nativeInitOem(JNIEnv *env,
                                                                         jobject thiz,
                                                                         jstring dir,
                                                                         jstring lang,
                                                                         jint mode) {

  native_data_t *nat = get_native_data(env, thiz);

  const char *c_dir = env->GetStringUTFChars(dir, NULL);
  const char *c_lang = env->GetStringUTFChars(lang, NULL);

  jboolean res = JNI_TRUE;

  if (nat->api.Init(c_dir, c_lang, (tesseract::OcrEngineMode) mode)) {
    res = JNI_FALSE;
  } else {
  }

  env->ReleaseStringUTFChars(dir, c_dir);
  env->ReleaseStringUTFChars(lang, c_lang);

  return res;
}

jstring Java_com_googlecode_tesseract_TessBaseAPI_nativeGetInitLanguagesAsString(JNIEnv *env,
                                                                                         jobject thiz) {

  native_data_t *nat = get_native_data(env, thiz);

  const char *text = nat->api.GetInitLanguagesAsString();

  jstring result = env->NewStringUTF(text);

  return result;
}


void Java_com_googlecode_tesseract_TessBaseAPI_nativeSetImageBytes(JNIEnv *env,
                                                                           jobject thiz,
                                                                           jbyteArray data,
                                                                           jint width,
                                                                           jint height,
                                                                           jint bpp,
                                                                           jint bpl) {

  jbyte *data_array = env->GetByteArrayElements(data, NULL);
  int count = env->GetArrayLength(data);
  unsigned char* imagedata = (unsigned char *) malloc(count * sizeof(unsigned char));

  // This is painfully slow, but necessary because we don't know
  // how many bits the JVM might be using to represent a byte
  for (int i = 0; i < count; i++) {
    imagedata[i] = (unsigned char) data_array[i];
  }

  env->ReleaseByteArrayElements(data, data_array, JNI_ABORT);

  native_data_t *nat = get_native_data(env, thiz);
  nat->api.SetImage(imagedata, (int) width, (int) height, (int) bpp, (int) bpl);

  // Since Tesseract doesn't take ownership of the memory, we keep a pointer in the native
  // code struct. We need to free that pointer when we release our instance of Tesseract or
  // attempt to set a new image using one of the nativeSet* methods.
  if (nat->data != NULL)
    free(nat->data);
  nat->data = imagedata;
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeSetRectangle(JNIEnv *env,
                                                                          jobject thiz,
                                                                          jint left,
                                                                          jint top,
                                                                          jint width,
                                                                          jint height) {

  native_data_t *nat = get_native_data(env, thiz);

  nat->api.SetRectangle(left, top, width, height);
}

jstring Java_com_googlecode_tesseract_TessBaseAPI_nativeGetUTF8Text(JNIEnv *env,
                                                                            jobject thiz) {

  native_data_t *nat = get_native_data(env, thiz);
  nat->initStateVariables(env, &thiz);

  ETEXT_DESC monitor;
  monitor.progress_callback = progressJavaCallback;
  monitor.cancel = cancelFunc;
  monitor.cancel_this = nat;
  monitor.progress_this = nat;

  char *text = nat->api.GetUTF8Text(&monitor);

  jstring result = env->NewStringUTF(text);

  free(text);
  nat->resetStateVariables();

  return result;
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeStop(JNIEnv *env,
                                                                  jobject thiz) {

  native_data_t *nat = get_native_data(env, thiz);

  // Stop by setting a flag that's used by the monitor
  nat->resetStateVariables();
  nat->cancel_ocr = true;
}

jint Java_com_googlecode_tesseract_TessBaseAPI_nativeMeanConfidence(JNIEnv *env,
                                                                            jobject thiz) {

  native_data_t *nat = get_native_data(env, thiz);

  return (jint) nat->api.MeanTextConf();
}

jintArray Java_com_googlecode_tesseract_TessBaseAPI_nativeWordConfidences(JNIEnv *env,
                                                                                  jobject thiz) {

  native_data_t *nat = get_native_data(env, thiz);

  int *confs = nat->api.AllWordConfidences();

  if (confs == NULL) {
    return NULL;
  }

  int len, *trav;
  for (len = 0, trav = confs; *trav != -1; trav++, len++)
    ;


  jintArray ret = env->NewIntArray(len);


  env->SetIntArrayRegion(ret, 0, len, confs);

  delete[] confs;

  return ret;
}

jboolean Java_com_googlecode_tesseract_TessBaseAPI_nativeSetVariable(JNIEnv *env,
                                                                             jobject thiz,
                                                                             jstring var,
                                                                             jstring value) {

  native_data_t *nat = get_native_data(env, thiz);

  const char *c_var = env->GetStringUTFChars(var, NULL);
  const char *c_value = env->GetStringUTFChars(value, NULL);

  jboolean set = nat->api.SetVariable(c_var, c_value) ? JNI_TRUE : JNI_FALSE;

  env->ReleaseStringUTFChars(var, c_var);
  env->ReleaseStringUTFChars(value, c_value);

  return set;
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeClear(JNIEnv *env,
                                                                   jobject thiz) {

  native_data_t *nat = get_native_data(env, thiz);

  nat->api.Clear();

  // Call between pages or documents etc to free up memory and forget adaptive data.
  nat->api.ClearAdaptiveClassifier();

  // Since Tesseract doesn't take ownership of the memory, we keep a pointer in the native
  // code struct. We need to free that pointer when we release our instance of Tesseract or
  // attempt to set a new image using one of the nativeSet* methods.
  if (nat->data != NULL)
    free(nat->data);
  nat->data = NULL;
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeEnd(JNIEnv *env,
                                                                 jobject thiz) {

  native_data_t *nat = get_native_data(env, thiz);

  nat->api.End();

  // Since Tesseract doesn't take ownership of the memory, we keep a pointer in the native
  // code struct. We need to free that pointer when we release our instance of Tesseract or
  // attempt to set a new image using one of the nativeSet* methods.
  if (nat->data != NULL)
    free(nat->data);
  nat->data = NULL;
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeSetDebug(JNIEnv *env,
                                                                      jobject thiz,
                                                                      jboolean debug) {

  native_data_t *nat = get_native_data(env, thiz);

  nat->debug = (debug == JNI_TRUE) ? TRUE : FALSE;
}

jint Java_com_googlecode_tesseract_TessBaseAPI_nativeGetPageSegMode(JNIEnv *env,
                                                                            jobject thiz) {

  native_data_t *nat = get_native_data(env, thiz);

  return nat->api.GetPageSegMode();
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeSetPageSegMode(JNIEnv *env,
                                                                            jobject thiz,
                                                                            jint mode) {

  native_data_t *nat = get_native_data(env, thiz);

  nat->api.SetPageSegMode((tesseract::PageSegMode) mode);
}

jlong Java_com_googlecode_tesseract_TessBaseAPI_nativeGetResultIterator(JNIEnv *env,
                                                                                jobject thiz) {
  native_data_t *nat = get_native_data(env, thiz);

  return (jlong) nat->api.GetIterator();
}

jstring Java_com_googlecode_tesseract_TessBaseAPI_nativeGetHOCRText(JNIEnv *env,
                                                                            jobject thiz, jint page) {

  native_data_t *nat = get_native_data(env, thiz);
  nat->initStateVariables(env, &thiz);

  ETEXT_DESC monitor;
  monitor.progress_callback = progressJavaCallback;
  monitor.cancel = cancelFunc;
  monitor.cancel_this = nat;
  monitor.progress_this = nat;

  char *text = nat->api.GetHOCRText(page, &monitor);

  jstring result = env->NewStringUTF(text);

  free(text);
  nat->resetStateVariables();

  return result;
}

jstring Java_com_googlecode_tesseract_TessBaseAPI_nativeGetBoxText(JNIEnv *env,
                                                                           jobject thiz, jint page) {

  native_data_t *nat = get_native_data(env, thiz);

  char *text = nat->api.GetBoxText(page);

  jstring result = env->NewStringUTF(text);

  free(text);

  return result;
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeSetInputName(JNIEnv *env,
                                                                          jobject thiz,
                                                                          jstring name) {
  native_data_t *nat = get_native_data(env, thiz);
  const char *c_name = env->GetStringUTFChars(name, NULL);
  nat->api.SetInputName(c_name);
  env->ReleaseStringUTFChars(name, c_name);
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeSetOutputName(JNIEnv *env,
                                                                           jobject thiz,
                                                                           jstring name) {
  native_data_t *nat = get_native_data(env, thiz);
  const char *c_name = env->GetStringUTFChars(name, NULL);
  nat->api.SetOutputName(c_name);
  env->ReleaseStringUTFChars(name, c_name);
}

void Java_com_googlecode_tesseract_TessBaseAPI_nativeReadConfigFile(JNIEnv *env,
                                                                            jobject thiz,
                                                                            jstring fileName) {
  native_data_t *nat = get_native_data(env, thiz);
  const char *c_file_name = env->GetStringUTFChars(fileName, NULL);
  nat->api.ReadConfigFile(c_file_name);
  env->ReleaseStringUTFChars(fileName, c_file_name);
}
#ifdef __cplusplus
}
#endif
