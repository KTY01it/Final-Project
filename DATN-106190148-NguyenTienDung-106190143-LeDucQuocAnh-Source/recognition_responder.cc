// #include "recognition_responder.h"
// #include "tensorflow/lite/micro/micro_log.h"

// #include "esp_main.h"
// #if DISPLAY_SUPPORT
// #include "image_provider.h"
// #include "esp_lcd.h"
// static QueueHandle_t xQueueLCDFrame = NULL;
// #endif

// void RespondToRecognition(float label0_score,float label1_score,float label2_score,float label3_score,
//                           float label4_score,float label5_score,float label6_score,float label7_score,
//                           float label8_score,float label9_score) {
//   int label0_score_int = (label0_score) * 100 + 0.5;
//   int label1_score_int = (label1_score) * 100 + 0.5;
//   int label2_score_int = (label2_score) * 100 + 0.5;
//   int label3_score_int = (label3_score) * 100 + 0.5;
//   int label4_score_int = (label4_score) * 100 + 0.5;
//   int label5_score_int = (label5_score) * 100 + 0.5;
//   int label6_score_int = (label6_score) * 100 + 0.5;
//   int label7_score_int = (label7_score) * 100 + 0.5;
//   int label8_score_int = (label8_score) * 100 + 0.5;
//   int label9_score_int = (label9_score) * 100 + 0.5;
//   #if DISPLAY_SUPPORT
//     if (xQueueLCDFrame == NULL) {
//       xQueueLCDFrame = xQueueCreate(2, sizeof(struct lcd_frame));
//       register_lcd(xQueueLCDFrame, NULL, false);
//     }

//     int color = 0x1f << 6; // red
//     if (person_score_int < 60) { // treat score less than 60% as no person
//       color = 0x3f; // green
//     }
//     app_lcd_color_for_detection(color);

//     // display frame (freed by lcd task)
//     lcd_frame_t *frame = (lcd_frame_t *) malloc(sizeof(lcd_frame_t));
//     frame->width = 96 * 2;
//     frame->height = 96 * 2;
//     frame->buf = image_provider_get_display_buf();
//     xQueueSend(xQueueLCDFrame, &frame, portMAX_DELAY);
//     (void) no_person_score;
//   #else
//   MicroPrintf("Score 0:%d%%, 1:%d%%, 2:%d%%, 3:%d%%, 4:%d%%, 5:%d%%, 6:%d%%, 7:%d%%, 8:%d%%, 9:%d%%",
//               label0_score_int, label1_score_int, label2_score_int, label3_score_int, label4_score_int, label5_score_int, label6_score_int, label7_score_int, label8_score_int, label9_score_int);
//   #endif
// }
