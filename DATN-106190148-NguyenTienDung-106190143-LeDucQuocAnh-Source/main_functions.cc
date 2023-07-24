#include "main_functions.h"

#include "recognition_responder.h"
#include "image_provider.h"
#include "model_settings.h"
#include "number_recognition_model_data.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_log.h>
#include "esp_main.h"

#include <stdio.h>
#include <time.h>

#define UART_PORT UART_NUM_0
#define BAUD_RATE 9600

// Globals, used for compatibility with Arduino-style sketches.
namespace {
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
float* model_input = nullptr;


#ifdef CONFIG_IDF_TARGET_ESP32S3
constexpr int scratchBufSize = 39 * 1024;
#else
constexpr int scratchBufSize = 0;
#endif
// An area of memory to use for input, output, and intermediate arrays.
constexpr int kTensorArenaSize = 60 * 1024 + scratchBufSize;
static uint8_t *tensor_arena;//[kTensorArenaSize]; // Maybe we should move this to external
}  // namespace

static const char* TAG = "main_functions";

// The name of this function is important for Arduino compatibility.
void setup() {
  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(g_number_recognition_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf("Model provided is schema version %d not equal to supported "
                "version %d.", model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  if (tensor_arena == NULL) {
    tensor_arena = (uint8_t *) heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  if (tensor_arena == NULL) {
    printf("Couldn't allocate memory of %d bytes\n", kTensorArenaSize);
    return;
  }

  static tflite::MicroMutableOpResolver<6> micro_op_resolver;
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddMaxPool2D();
  micro_op_resolver.AddDepthwiseConv2D();
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddSoftmax();
  
  // Build an interpreter to run the model with.
  // NOLINTNEXTLINE(runtime-global-variables)
  interpreter = new tflite::MicroInterpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize);
  // interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  input = interpreter->input(0);
  model_input = input->data.f;
#ifndef CLI_ONLY_INFERENCE
  // Initialize Camera
  TfLiteStatus init_status = InitCamera();
  if (init_status != kTfLiteOk) {
    MicroPrintf("InitCamera failed\n");
    return;
  }
#endif  
  for (int a=1; a<6; a++) {
    // Get image from provider.
    if (kTfLiteOk != GetImage(kNumCols, kNumRows, kNumChannels, model_input)) {
      MicroPrintf("Image capture failed.");
    }

    ESP_LOGI(TAG, "Invoking interpreter");
    // Run the model on this input and make sure it succeeds.
    if (kTfLiteOk != interpreter->Invoke()) {
      MicroPrintf("Invoke failed.");
    }

    ESP_LOGI(TAG, "Showing results");
    TfLiteTensor* output = interpreter->output(0);
  
  // Gốc
/*
  for (int i=0; i < kCategoryCount; i++)
	{
  ESP_LOGI(TAG,"Label=%s, Prob=%f",kCategoryLabels[i], output->data.f[i] );
        
  FILE *file = fopen("/sdcard/predict.txt", "a");
    if (file == NULL)
    {
      printf("err: fopen failed\n");
    } else {
      int result = fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[i], output->data.f[i]);
      if (result > 0) {
        printf("Data written to file successfully.\n");
      } else {
        printf("Error writing data to file.\n");
      }
    fclose(file);
    }
  }
*/
  //Test Time
  
    FILE *file = fopen("/sdcard/predict.txt", "a");
      if (file == NULL)
      {
        printf("err: fopen failed\n");
      } else {
          fprintf(file, "Start Write Information --> \n");
      fclose(file);
      }

    for (int i=0; i < kCategoryCount; i++)
    {
    ESP_LOGI(TAG,"Label=%s, Prob=%f",kCategoryLabels[i], output->data.f[i] );
          
    FILE *file = fopen("/sdcard/predict.txt", "a");
      if (file == NULL)
      {
        printf("err: fopen failed\n");
      } else {
        time_t now = time(NULL);
        struct tm *timeinfo = localtime(&now);

        char data[100];

        sprintf(data, "Predict Label || Label=%s, Prob=%f || Date=%04d-%02d-%02d Time=%02d:%02d:%02d\n",
            kCategoryLabels[i], output->data.f[i],
            timeinfo->tm_year + 2016, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

        int result = fprintf(file, "%s\n", data);
        if (result > 0) {
          printf("Data written to file successfully.\n");
        } else {
          printf("Error writing data to file.\n");
        }
      fclose(file);
      }
    }
  }

  // uart_config_t uart_config = {
  //       .baud_rate = BAUD_RATE,
  //       .data_bits = UART_DATA_8_BITS,
  //       .parity = UART_PARITY_DISABLE,
  //       .stop_bits = UART_STOP_BITS_1,
  //       .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  //       .rx_flow_ctrl_thresh = 122,
  //   };
  //   uart_param_config(UART_PORT, &uart_config);
  //   uart_driver_install(UART_PORT, 1024, 0, 0, NULL, 0);

    
  //   const char* text_data = "Hello, UART!";
  //   uart_write_bytes(UART_PORT, text_data, strlen(text_data));

  vTaskDelay(1); // to avoid watchdog trigger

  
    // // Dữ liệu bạn muốn gửi
    // const char* kCategoryLabels[] = { "Label1", "Label2", "Label3" };
    // float outputData[] = { 0.1f, 0.5f, 0.9f };

    // // Gửi dữ liệu qua UART
    // char buffer[64];
    // for (int i = 0; i < 3; i++) {
    //     int len = snprintf(buffer, sizeof(buffer), "Label=%s, Prob=%f\r\n", kCategoryLabels[i], outputData[i]);
    //     uart_write_bytes(UART_PORT, buffer, len);
    // }
}

// #ifndef CLI_ONLY_INFERENCE
// The name of this function is important for Arduino compatibility.
void loop() {
/*
  if (kTfLiteOk != GetImage(kNumCols, kNumRows, kNumChannels, input->data.f)) {
    MicroPrintf("Image capture failed.");
  }

  ESP_LOGI(TAG, "Invoking interpreter");
  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter->Invoke()) {
    MicroPrintf("Invoke failed.");
  }

  ESP_LOGI(TAG, "Showing results");
  TfLiteTensor* output = interpreter->output(0);

  FILE *file = fopen("/sdcard/predict.txt", "a");
    if (file == NULL)
    {
      printf("err: fopen failed\n");
    } else {
        fprintf(file, "Start Write Information --> \n");
    fclose(file);
    }

  for (int i=0; i < kCategoryCount; i++)
	{
  ESP_LOGI(TAG,"Label=%s, Prob=%f",kCategoryLabels[i], output->data.f[i] );
        
  FILE *file = fopen("/sdcard/predict.txt", "a");
    if (file == NULL)
    {
      printf("err: fopen failed\n");
    } else {
      time_t now = time(NULL);
      struct tm *timeinfo = localtime(&now);

      char data[100];

      sprintf(data, "Predict Label || Label=%s, Prob=%f || Date=%04d-%02d-%02d Time=%02d:%02d:%02d\n",
          kCategoryLabels[i], output->data.f[i],
          timeinfo->tm_year + 2016, timeinfo->tm_mon + 1, timeinfo->tm_mday,
          timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

      int result = fprintf(file, "%s\n", data);
      if (result > 0) {
        printf("Data written to file successfully.\n");
      } else {
        printf("Error writing data to file.\n");
      }
    fclose(file);
    }
  }
  vTaskDelay(1); // to avoid watchdog trigger
*/
}
// #endif

#if defined(COLLECT_CPU_STATS)
  long long total_time = 0;
  long long start_time = 0;
  extern long long softmax_total_time;
  extern long long dc_total_time;
  extern long long conv_total_time;
  extern long long fc_total_time;
  extern long long pooling_total_time;
  extern long long add_total_time;
  extern long long mul_total_time;
#endif

#if defined CLI_ONLY_INFERENCE
void run_inference(void *ptr) {
  /* Convert from uint8 picture data to int8 */
  for (int i = 0; i < kNumCols * kNumRows *3; i++) {
    input->data.int8[i] = ((uint8_t *) ptr)[i] ^ 0x80;
  }

#if defined(COLLECT_CPU_STATS)
  long long start_time = esp_timer_get_time();
#endif
  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter->Invoke()) {
    MicroPrintf("Invoke failed.");
  }

#if defined(COLLECT_CPU_STATS)
  long long total_time = (esp_timer_get_time() - start_time);
  printf("Total time = %lld\n", total_time / 1000);
  //printf("Softmax time = %lld\n", softmax_total_time / 1000);
  printf("FC time = %lld\n", fc_total_time / 1000);
  printf("DC time = %lld\n", dc_total_time / 1000);
  printf("conv time = %lld\n", conv_total_time / 1000);
  printf("Pooling time = %lld\n", pooling_total_time / 1000);
  printf("add time = %lld\n", add_total_time / 1000);
  printf("mul time = %lld\n", mul_total_time / 1000);

  /* Reset times */
  total_time = 0;
  //softmax_total_time = 0;
  dc_total_time = 0;
  conv_total_time = 0;
  fc_total_time = 0;
  pooling_total_time = 0;
  add_total_time = 0;
  mul_total_time = 0;
#endif

  TfLiteTensor* output = interpreter->output(0);

  for (int i=0; i < kCategoryCount; i++)
	{
  ESP_LOGI(TAG,"Label=%s, Prob=%f",kCategoryLabels[i], output->data.f[i] );
        
  FILE *file = fopen("/sdcard/predict.txt", "a");
    if (file == NULL)
    {
      printf("err: fopen failed\n");
    } else {
      int result = fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[i], output->data.f[i]);
      if (result > 0) {
        printf("Data written to file successfully.\n");
      } else {
        printf("Error writing data to file.\n");
      }
    fclose(file);
    }
  }
}
#endif
/*
  // Process the inference results.
  int8_t label0_score  = output->data.uint8[k0Index];
  int8_t label1_score  = output->data.uint8[k1Index];
  int8_t label2_score  = output->data.uint8[k2Index];
  int8_t label3_score  = output->data.uint8[k3Index];
  int8_t label4_score  = output->data.uint8[k4Index];
  int8_t label5_score  = output->data.uint8[k5Index];
  int8_t label6_score  = output->data.uint8[k6Index];
  int8_t label7_score  = output->data.uint8[k7Index];
  int8_t label8_score  = output->data.uint8[k8Index];
  int8_t label9_score  = output->data.uint8[k9Index];

  float label0_score_f = (label0_score - output->params.zero_point) * output->params.scale;
  float label1_score_f = (label1_score - output->params.zero_point) * output->params.scale;
  float label2_score_f = (label2_score - output->params.zero_point) * output->params.scale;
  float label3_score_f = (label3_score - output->params.zero_point) * output->params.scale;
  float label4_score_f = (label4_score - output->params.zero_point) * output->params.scale;
  float label5_score_f = (label5_score - output->params.zero_point) * output->params.scale;
  float label6_score_f = (label6_score - output->params.zero_point) * output->params.scale;
  float label7_score_f = (label7_score - output->params.zero_point) * output->params.scale;
  float label8_score_f = (label8_score - output->params.zero_point) * output->params.scale;
  float label9_score_f = (label9_score - output->params.zero_point) * output->params.scale;

  float label0_score_int = (label0_score) * 100 + 0.5;
  float label1_score_int = (label1_score) * 100 + 0.5;
  float label2_score_int = (label2_score) * 100 + 0.5;
  float label3_score_int = (label3_score) * 100 + 0.5;
  float label4_score_int = (label4_score) * 100 + 0.5;
  float label5_score_int = (label5_score) * 100 + 0.5;
  float label6_score_int = (label6_score) * 100 + 0.5;
  float label7_score_int = (label7_score) * 100 + 0.5;
  float label8_score_int = (label8_score) * 100 + 0.5;
  float label9_score_int = (label9_score) * 100 + 0.5;

  FILE *file = fopen("/sdcard/predict.txt", "a");
  if (file == NULL)
  {
    printf("err: fopen failed\n");
  } else {
    if (fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[0], label0_score_int) > 0)
    {
      printf("Data for Label 0 written to file successfully.\n");
    }
    if (fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[1], label1_score_int) > 0)
    {
      printf("Data for Label 1 written to file successfully.\n");
    }
    if (fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[2], label2_score_int) > 0)
    {
      printf("Data for Label 2 written to file successfully.\n");
    }
    if (fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[3], label3_score_int) > 0)
    {
      printf("Data for Label 3 written to file successfully.\n");
    }
    if (fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[4], label4_score_int) > 0)
    {
      printf("Data for Label 4 written to file successfully.\n");
    }
    if (fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[5], label5_score_int) > 0)
    {
      printf("Data for Label 5 written to file successfully.\n");
    }
    if (fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[6], label6_score_int) > 0)
    {
      printf("Data for Label 6 written to file successfully.\n");
    }
    if (fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[7], label7_score_int) > 0)
    {
      printf("Data for Label 7 written to file successfully.\n");
    }
    if (fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[8], label8_score_int) > 0)
    {
      printf("Data for Label 8 written to file successfully.\n");
    }
    if (fprintf(file, "Predict Label || Label=%s, Prob=%f ||\n", kCategoryLabels[9], label9_score_int) > 0)
    {
      printf("Data for Label 9 written to file successfully.\n");
    }
  fclose(file);
  }
*/