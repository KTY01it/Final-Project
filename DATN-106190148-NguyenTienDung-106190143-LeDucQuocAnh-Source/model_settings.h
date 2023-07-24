#ifndef TENSORFLOW_LITE_MICRO_DIGIT_RECOGNITION_MODEL_SETTINGS_H_
#define TENSORFLOW_LITE_MICRO_DIGIT_RECOGNITION_MODEL_SETTINGS_H_

constexpr int kNumCols = 40;
constexpr int kNumRows = 40;
constexpr int kNumChannels = 3;

constexpr int kMaxImageSize = kNumCols * kNumRows * kNumChannels;

constexpr int kCategoryCount = 10; // Số nhãn được phân loại bởi mô hình
constexpr int k0Index = 0; // Chỉ số của nhãn "0" trong mảng kết quả phân loại.
constexpr int k1Index = 1; // Chỉ số của nhãn "1" trong mảng kết quả phân loại.
constexpr int k2Index = 2; // Chỉ số của nhãn "2" trong mảng kết quả phân loại.
constexpr int k3Index = 3; // Chỉ số của nhãn "3" trong mảng kết quả phân loại.
constexpr int k4Index = 4; // Chỉ số của nhãn "4" trong mảng kết quả phân loại.
constexpr int k5Index = 5; // Chỉ số của nhãn "5" trong mảng kết quả phân loại.
constexpr int k6Index = 6; // Chỉ số của nhãn "6" trong mảng kết quả phân loại.
constexpr int k7Index = 7; // Chỉ số của nhãn "7" trong mảng kết quả phân loại.
constexpr int k8Index = 8; // Chỉ số của nhãn "8" trong mảng kết quả phân loại.
constexpr int k9Index = 9; // Chỉ số của nhãn "9" trong mảng kết quả phân loại.
extern const char* kCategoryLabels[kCategoryCount];

#endif  // TENSORFLOW_LITE_MICRO_DIGIT_RECOGNITION_MODEL_SETTINGS_H_
