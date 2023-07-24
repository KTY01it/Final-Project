#ifndef TENSORFLOW_LITE_MICRO_DIGIT_RECOGNITION_IMAGE_PROVIDER_H_
#define TENSORFLOW_LITE_MICRO_DIGIT_RECOGNITION_IMAGE_PROVIDER_H_

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_log.h"

#ifndef CONFIG_PERSON_DETECTION_STATIC

// Returns buffer to be displayed
void *image_provider_get_display_buf();

TfLiteStatus GetImage(int image_width, int image_height, int channels, float* image_data);

TfLiteStatus InitCamera();

#endif /* CONFIG_PERSON_DETECTION_STATIC */

#endif  // TENSORFLOW_LITE_MICRO_DIGIT_RECOGNITION_IMAGE_PROVIDER_H_
