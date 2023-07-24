#include <cstdlib>
#include <cstring>
#include <iostream>

#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#include "app_camera_esp.h"
#include "esp_camera.h"
#include "model_settings.h"
#include "image_provider.h"
#include "esp_main.h"

static const char* TAG = "app_camera";

static uint16_t *display_buf; // buffer to hold data to be sent to display

// Get the camera module ready
TfLiteStatus InitCamera() {
  #if CLI_ONLY_INFERENCE
    ESP_LOGI(TAG, "CLI_ONLY_INFERENCE enabled, skipping camera init");
    return kTfLiteOk;
  #endif
  // if display support is present, initialise display buf
  #if DISPLAY_SUPPORT
    if (display_buf == NULL) {
      display_buf = (uint16_t *) heap_caps_malloc(96 * 2 * 96 * 2 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (display_buf == NULL) {
      ESP_LOGE(TAG, "Couldn't allocate display buffer");
      return kTfLiteError;
    }
  #endif

  int ret = app_camera_init();
  if (ret != 0) {
    MicroPrintf("Camera init failed\n");
    return kTfLiteError;
  }
  MicroPrintf("Camera Initialized\n");
  return kTfLiteOk;
}

void *image_provider_get_display_buf()
{
  return (void *) display_buf;
}

// sd
static esp_err_t initi_sd_card(void)
{  
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 3,
    };
    sdmmc_card_t *card;
    esp_err_t err = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK)
    {
        return err;
    }
    return ESP_OK;
}

// Processing Image
void histogramequalization(camera_fb_t* fb) {
    // Calculate histogram
    int histogram[256] = {0};
    for (int i = 0; i < fb->len; i += 2) {
        histogram[fb->buf[i]]++;
    }

    // Calculate cumulative histogram
    int cumulativeHistogram[256] = {0};
    cumulativeHistogram[0] = histogram[0];
    for (int i = 1; i < 256; i++) {
        cumulativeHistogram[i] = cumulativeHistogram[i - 1] + histogram[i];
    }

    // Calculate equalized image
    int totalPixels = fb->len / 2;
    for (int i = 0; i < fb->len; i += 2) {
        uint8_t pixelValue = fb->buf[i];
        uint8_t equalizedValue = (cumulativeHistogram[pixelValue] * 255) / totalPixels;
        fb->buf[i] = equalizedValue;
        fb->buf[i + 1] = equalizedValue; // Assuming 16-bit per pixel image
    }
}
void thresholdingBinary(camera_fb_t* fb, uint8_t threshold) {
  for (int i = 0; i < fb->len; i++) {
    if (fb->buf[i] < threshold) {
      fb->buf[i] = 255; // Đặt giá trị thành mức xám tối đa (trắng)
    } else {
      fb->buf[i] = 0; // Đặt giá trị thành mức xám tối thiểu (đen)
    }
  }
}
static camera_fb_t* crop_image(camera_fb_t *fb, unsigned short cropLeft, unsigned short cropRight, unsigned short cropTop, unsigned short cropBottom)
{
    unsigned int maxTopIndex = cropTop * fb->width * 2;
    unsigned int minBottomIndex = ((fb->width*fb->height) - (cropBottom * fb->width)) * 2;
    unsigned short maxX = fb->width - cropRight; // In pixels
    unsigned short newWidth = fb->width - cropLeft - cropRight;
    unsigned short newHeight = fb->height - cropTop - cropBottom;
    size_t newJpgSize = newWidth * newHeight *2;

    unsigned int writeIndex = 0;
    // Loop over all bytes
    for(int i = 0; i < fb->len; i+=2){
        // Calculate current X, Y pixel position
        int x = (i/2) % fb->width;

        // Crop from the top
        if(i < maxTopIndex){ continue; }

        // Crop from the bottom
        if(i > minBottomIndex){ continue; }

        // Crop from the left
        if(x <= cropLeft){ continue; }

        // Crop from the right
        if(x > maxX){ continue; }

        // If we get here, keep the pixels
        fb->buf[writeIndex++] = fb->buf[i];
        fb->buf[writeIndex++] = fb->buf[i+1];
    }

    // Set the new dimensions of the framebuffer for further use.
    fb->width = newWidth;
    fb->height = newHeight;
    fb->len = newJpgSize;
  return fb;
}
void inc_brightness(camera_fb_t *fb, int c) {
  for(int i= 0; i<fb->len;i++){
    if (fb->buf[i]+c>255){
        fb->buf[i]=fb->buf[i];
    } else {
      fb->buf[i]=fb->buf[i]+c;
    }
  }
}
void erosion(camera_fb_t* fb, int kernelSize) {
    int width = fb->width;
    int height = fb->height;
    int channels = 2; // Assuming 16-bit per pixel image

    // Create a temporary buffer for storing the eroded image
    uint8_t* erodedBuffer = (uint8_t*)malloc(fb->len);
    memcpy(erodedBuffer, fb->buf, fb->len);

    // Perform erosion operation
    for (int y = kernelSize; y < height - kernelSize; y++) {
        for (int x = kernelSize; x < width - kernelSize; x++) {
            // Find the minimum pixel value within the kernel
            uint8_t minPixelValue = 255;
            for (int ky = -kernelSize; ky <= kernelSize; ky++) {
                for (int kx = -kernelSize; kx <= kernelSize; kx++) {
                    int offsetX = x + kx;
                    int offsetY = y + ky;
                    uint8_t pixelValue = fb->buf[(offsetY * width + offsetX) * channels];
                    if (pixelValue < minPixelValue) {
                        minPixelValue = pixelValue;
                    }
                }
            }

            // Update the pixel value in the eroded buffer
            erodedBuffer[(y * width + x) * channels] = minPixelValue;
        }
    }

    // Copy the eroded buffer back to the original buffer
    memcpy(fb->buf, erodedBuffer, fb->len);

    // Free the temporary buffer
    free(erodedBuffer);
}
void saveBmp(camera_fb_t* fb,const char* filename) {
  
  uint16_t* img = (uint16_t*)fb->buf;
  uint16_t imgWidth = fb->width;
  uint16_t imgHeight = fb->height;
  
  
  // BMP file header
  uint32_t fileSize = imgWidth * imgHeight * 3 + 54;
  uint8_t bmpHeader[54] = {
    'B', 'M',                                     // Signature
    (uint8_t)(fileSize), (uint8_t)(fileSize >> 8), (uint8_t)(fileSize >> 16), (uint8_t)(fileSize >> 24),  // File size
    0, 0, 0, 0,                                   // Reserved
    54, 0, 0, 0,                                  // Offset to image data
    40, 0, 0, 0,                                  // Header size
    (uint8_t)(imgWidth), (uint8_t)(imgWidth >> 8), 0, 0,             // Image width
    (uint8_t)(imgHeight), (uint8_t)(imgHeight >> 8), 0, 0,           // Image height
    1, 0,                                         // Number of color planes
    24, 0,                                        // Bits per pixel (RGB)
    0, 0, 0, 0,                                   // Compression method (none)
    0, 0, 0, 0,                                   // Image size
    0, 0, 0, 0,                                   // Horizontal resolution (pixels per meter)
    0, 0, 0, 0,                                   // Vertical resolution (pixels per meter)
    0, 0, 0, 0,                                   // Number of colors (default)
    0, 0, 0, 0                                    // Number of important colors
  };

  // Write BMP file header
    //char photo_name[50];
    //sprintf(photo_name, "/sdcard/fb%li.jpg", fb->timestamp.tv_sec);

    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("err: fopen failed\n");
    }
    else
    {
        fwrite(bmpHeader, 1, 54, file);

  // Write image data
  int padding = (4 - ((imgWidth * 3) % 4)) % 4; // Calculate padding bytes
  uint8_t pixelBuffer[3];
  for (int y = imgHeight - 1; y >= 0; y--) {     // Bắt đầu từ hàng cuối cùng
    for (int x = 0; x < imgWidth; x++) {
      uint16_t color = img[y * imgWidth + x];
      uint8_t r = ((color) >> 0);    // R: 5 bit, dịch sang phải 8 bit
      uint8_t g = ((color) >> 0);    // G: 6 bit, dịch sang phải 3 bit
      uint8_t b = ((color) << 0);    // B: 5 bit, dịch sang trái 3 bit
  
      // Chuẩn bị dữ liệu pixel
      pixelBuffer[0] = b;
      pixelBuffer[1] = g;
      pixelBuffer[2] = r;
  
      // Ghi dữ liệu pixel
      fwrite(pixelBuffer, 1, 3, file);
    }
    // Ghi các byte padding
    for (int p = 0; p < padding; p++) {
      fwrite(&padding, 1, 1, file);
    }
    }
    fclose(file);
    }
}
void saveJPG(uint8_t * jpg_buf,size_t jpg_size,const char *file_path){
    FILE *file = fopen(file_path, "w");
    if (file == NULL)
    {
        printf("err: fopen failed\n");
    }
    else
    {
        fwrite(jpg_buf, 1, jpg_size, file);
        fclose(file);
    }
}
// Get an image from the camera module
TfLiteStatus GetImage(int image_width, int image_height, int channels, float* image_data) {
  esp_err_t err = initi_sd_card();
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Sdcard init failed with error 0x%x", err);
  }
  
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGE(TAG, "Camera capture failed");
    return kTfLiteError;
  }

  // printf("SaveImageThanhCong\n");

  #if DISPLAY_SUPPORT
  // In case if display support is enabled, we initialise camera in rgb mode
  // Hence, we need to convert this data to grayscale to send it to tf model
  // For display we extra-polate the data to 192X192
  for (int i = 0; i < kNumRows; i++) {
    for (int j = 0; j < kNumCols; j++) {
      uint16_t pixel = ((uint16_t *) (fb->buf))[i * kNumCols + j];

      // for inference
      uint8_t hb = pixel & 0xFF;
      uint8_t lb = pixel >> 8;
      uint8_t r = (lb & 0x1F) << 3;
      uint8_t g = ((hb & 0x07) << 5) | ((lb & 0xE0) >> 3);
      uint8_t b = (hb & 0xF8);

      /**
       * Gamma corected rgb to greyscale formula: Y = 0.299R + 0.587G + 0.114B
       * for effiency we use some tricks on this + quantize to [-128, 127]
       */
      int8_t grey_pixel = ((305 * r + 600 * g + 119 * b) >> 10) - 128;

      image_data[i * kNumCols + j] = grey_pixel;

      // to display
      display_buf[2 * i * kNumCols * 2 + 2 * j] = pixel;
      display_buf[2 * i * kNumCols * 2 + 2 * j + 1] = pixel;
      display_buf[(2 * i + 1) * kNumCols * 2 + 2 * j] = pixel;
      display_buf[(2 * i + 1) * kNumCols * 2 + 2 * j + 1] = pixel;
    }
  }
  #else
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    fb = crop_image(fb,196,244,167,263);
    saveBmp(fb,"/sdcard/crop.bmp");
    MicroPrintf("Image Captured\n");

    ////////////Crop 5 anh nho
    camera_fb_t *tempfb = (camera_fb_t *)malloc(sizeof(camera_fb_t));
  
    tempfb->buf=fb->buf;
    tempfb->width = fb->width;
    tempfb->height = fb->height;
    tempfb->len = fb->len;

    camera_fb_t *pic1; //, *pic2, *pic3, *pic4, *pic5;
//
//     for(int i=1; i<6; i++) {
//       if(i==1) {
//         fb->buf = tempfb->buf;
//         fb->width = tempfb->width;
//         fb->height = tempfb->height;
//         fb->len = tempfb->len;

//         pic1 = crop_image(fb,1,159,6,4);
//         inc_brightness(pic1,80);
//         histogramequalization(pic1);
//         thresholdingBinary(pic1,115);

//         saveBmp(pic1,"/sdcard/pic1.bmp");
//         /*
//         // Create a buffer for the JPG in psram
//         uint8_t * jpg_buf1 = (uint8_t *) heap_caps_malloc(200000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

//           if(jpg_buf1 == NULL){
//             printf("Malloc failed to allocate buffer for JPG.\n");
//         }else{
//             size_t jpg_size1 = 0;

//             // Convert the RAW image into JPG
//             // The parameter "31" is the JPG quality. Higher is better.
//             fmt2jpg(pic1->buf, pic1->len, pic1->width, pic1->height, pic1->format, 31, &jpg_buf1, &jpg_size1);
//             // printf("Converted JPG size: %d bytes \n", jpg_size);
//             saveJPG(jpg_buf1,jpg_size1,"/sdcard/pic2.jpg");
//         }
//         */
//         for (int j = 0; j < pic1->width * pic1->height * 3; j++) {
//           image_data[j] = (pic1->buf)[j] / 255.0f;
//         }

//         esp_camera_fb_return(pic1);

//       } else if (i==2) {
//         fb->buf = tempfb->buf;
//         fb->width = tempfb->width;
//         fb->height = tempfb->height;
//         fb->len = tempfb->len;

//         pic2 = crop_image(fb,40,120,8,2);
//         inc_brightness(pic2,80);
//         histogramequalization(pic2);
//         thresholdingBinary(pic2,115);

//         saveBmp(pic2,"/sdcard/pic2.bmp");
//         /*
//         // Create a buffer for the JPG in psram
//         uint8_t * jpg_buf2 = (uint8_t *) heap_caps_malloc(200000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

//           if(jpg_buf2 == NULL){
//             printf("Malloc failed to allocate buffer for JPG.\n");
//         }else{
//             size_t jpg_size2 = 0;

//             // Convert the RAW image into JPG
//             // The parameter "31" is the JPG quality. Higher is better.
//             fmt2jpg(pic2->buf, pic2->len, pic2->width, pic2->height, pic2->format, 31, &jpg_buf2, &jpg_size2);
//             // printf("Converted JPG size: %d bytes \n", jpg_size);
//             saveJPG(jpg_buf2,jpg_size2,"/sdcard/pic2.jpg");
//         }
//         */
//         for (int k = 0; k < pic2->width * pic2->height * 3; k++) {
//           image_data[k] = (pic2->buf)[k] / 255.0f;
//         }

//         esp_camera_fb_return(pic2);

//       } else if (i==3) {
//         fb->buf = tempfb->buf;
//         fb->width = tempfb->width;
//         fb->height = tempfb->height;
//         fb->len = tempfb->len;

//         pic3 = crop_image(fb,1,159,6,4);
//         inc_brightness(pic3,80);
//         histogramequalization(pic3);
//         thresholdingBinary(pic3,115);

//         saveBmp(pic3,"/sdcard/pic3.bmp");
//         /*
//         // Create a buffer for the JPG in psram
//         uint8_t * jpg_buf3 = (uint8_t *) heap_caps_malloc(200000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

//           if(jpg_buf3 == NULL){
//             printf("Malloc failed to allocate buffer for JPG.\n");
//         }else{
//             size_t jpg_size3 = 0;

//             // Convert the RAW image into JPG
//             // The parameter "31" is the JPG quality. Higher is better.
//             fmt2jpg(pic3->buf, pic3->len, pic3->width, pic3->height, pic3->format, 31, &jpg_buf3, &jpg_size3);
//             // printf("Converted JPG size: %d bytes \n", jpg_size);
//             saveJPG(jpg_buf3,jpg_size3,"/sdcard/pic3.jpg");
//         }
//         */
//         for (int l = 0; l < pic3->width * pic3->height * 3; l++) {
//           image_data[l] = (pic3->buf)[l] / 255.0f;
//         }

//         esp_camera_fb_return(pic3);
//       } else if (i==4) {
//         fb->buf = tempfb->buf;
//         fb->width = tempfb->width;
//         fb->height = tempfb->height;
//         fb->len = tempfb->len;

//         pic4 = crop_image(fb,1,159,6,4);
//         inc_brightness(pic4,80);
//         histogramequalization(pic4);
//         thresholdingBinary(pic4,115);

//         saveBmp(pic4,"/sdcard/pic4.bmp");
//         /*
//         // Create a buffer for the JPG in psram
//         uint8_t * jpg_buf4 = (uint8_t *) heap_caps_malloc(200000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

//           if(jpg_buf4 == NULL){
//             printf("Malloc failed to allocate buffer for JPG.\n");
//         }else{
//             size_t jpg_size4 = 0;

//             // Convert the RAW image into JPG
//             // The parameter "31" is the JPG quality. Higher is better.
//             fmt2jpg(pic4->buf, pic4->len, pic4->width, pic4->height, pic4->format, 31, &jpg_buf4, &jpg_size4);
//             // printf("Converted JPG size: %d bytes \n", jpg_size);
//             saveJPG(jpg_buf4,jpg_size4,"/sdcard/pic4.jpg");
//         }
//         */
//         for (int m = 0; m < pic4->width * pic4->height * 3; m++) {
//           image_data[m] = (pic4->buf)[m] / 255.0f;
//         }

//         esp_camera_fb_return(pic4);
//       } else if (i==5) {
//         fb->buf = tempfb->buf;
//         fb->width = tempfb->width;
//         fb->height = tempfb->height;
//         fb->len = tempfb->len;

//         pic5 = crop_image(fb,1,159,6,4);
//         inc_brightness(pic5,80);
//         histogramequalization(pic5);
//         thresholdingBinary(pic5,115);

//         saveBmp(pic5,"/sdcard/pic5.bmp");
//         /*
//         // Create a buffer for the JPG in psram
//         uint8_t * jpg_buf5 = (uint8_t *) heap_caps_malloc(200000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

//           if(jpg_buf5 == NULL){
//             printf("Malloc failed to allocate buffer for JPG.\n");
//         }else{
//             size_t jpg_size5 = 0;

//             // Convert the RAW image into JPG
//             // The parameter "31" is the JPG quality. Higher is better.
//             fmt2jpg(pic5->buf, pic5->len, pic5->width, pic5->height, pic5->format, 31, &jpg_buf5, &jpg_size5);
//             // printf("Converted JPG size: %d bytes \n", jpg_size);
//             saveJPG(jpg_buf5,jpg_size5,"/sdcard/pic5.jpg");
//         }
//         */
//         for (int n = 0; n < pic5->width * pic5->height * 3; n++) {
//           image_data[n] = (pic5->buf)[n] / 255.0f;
//         }

//         esp_camera_fb_return(pic5);
//       }      
//     }
    
   
    // pic1 = crop_image(fb,1,159,6,4);
    // pic2 = crop_image(fb,40,120,8,2);
    pic1 = crop_image(fb,76,84,6,4);
    // pic4 = crop_image(fb,120,40,6,4);
    // pic5 = crop_image(fb,159,1,6,4);

    inc_brightness(pic1,80);
    histogramequalization(pic1);
    thresholdingBinary(pic1,115);

    saveBmp(pic1,"/sdcard/pic3.bmp");

    // Create a buffer for the JPG in psram
    uint8_t * jpg_buf = (uint8_t *) heap_caps_malloc(200000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

      if(jpg_buf == NULL){
        printf("Malloc failed to allocate buffer for JPG.\n");
    }else{
        size_t jpg_size = 0;

        // Convert the RAW image into JPG
        // The parameter "31" is the JPG quality. Higher is better.
        fmt2jpg(pic1->buf, pic1->len, pic1->width, pic1->height, pic1->format, 31, &jpg_buf, &jpg_size);
        // printf("Converted JPG size: %d bytes \n", jpg_size);
        saveJPG(jpg_buf,jpg_size,"/sdcard/pic2.jpg");
    }
    
    MicroPrintf("SaveImageThanhCong\n");

    // We have initialised camera to grayscale
    // Just quantize to int8_t (^0x80) - định cấu trúc nhằm làm giảm kích thước ảnh
    for (int i = 0; i < pic1->width * pic1->height * 3; i++) {
      image_data[i] = jpg_buf[i] / 255.0f;
    }

  #endif

    esp_camera_fb_return(fb);
    /* here the esp camera can give you grayscale image directly */
    return kTfLiteOk;
}
