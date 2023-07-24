// Định nghĩa các thư viện cần dùng
#include <esp_system.h>
#include <nvs_flash.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

#include "driver/gpio.h" // Sử dụng button của ESP32-Cam
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#include "esp_camera.h"

//#define BUTTON_PIN 3

//static void button_handler(void *arg);
// khởi tạo hàm chụp ảnh là một thành viên của cấu trúc camera_fb_t
static camera_fb_t* take_photo(void *arg);
// Cấu hình các chân cho Camera của module
#define CAM_PIN_PWDN    32 
#define CAM_PIN_RESET   -1 //software reset will be performed
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

#define CONFIG_XCLK_FREQ 20000000 
#define CONFIG_OV2640_SUPPORT 1
#define CONFIG_OV7725_SUPPORT 1
#define CONFIG_OV3660_SUPPORT 1
#define CONFIG_OV5640_SUPPORT 1

// Truyền cho camera_config các biến thành viên của camera_config_t là các tham số để cấu hình cho camera
static esp_err_t init_camera(void)
{
    camera_config_t camera_config = {
        .pin_pwdn  = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,

        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        .xclk_freq_hz = CONFIG_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_RGB565,
        .frame_size = FRAMESIZE_VGA,

        .jpeg_quality = 12,
        .fb_count = 1,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY};//CAMERA_GRAB_LATEST. Sets when buffers should be filled
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        return err;
    }
    return ESP_OK;
}

static esp_err_t initi_sd_card(void)
{   // Xác định một biến host thuộc lớp sdmmc_host_t để lấy API hoặc làm việc với SD hoặc MMC
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    // Định nghĩa biến slot_config để cấu hình với khe cắm thẻ SD
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    // Tuỳ chọn gắn hệ thống tập tin
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, // Giá trị boolean trả về là có định nghĩa thẻ SD hay không nếu thẻ SD không được gắn vào khe
        .max_files = 3, // Số file tối đa được mở ra cùng lúc
    };
    sdmmc_card_t *card; // Khai báo 1 biến là thành viên của cấu trúc sdmmc_card_t
    // Truyền cho nó các tham số để gắn tệp FAT
    esp_err_t err = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK)
    {
        return err;
    }
    return ESP_OK;
}
// Khởi tạo hàm lấy giá trị buffer là bộ đệm được chụp từ Camera
static camera_fb_t* take_photo(void *arg)
{
    printf("Starting Taking Picture!\n");
    camera_fb_t *pic = esp_camera_fb_get();
    printf("Finished Taking Picture!\n");
    return pic;
}

// Xây dụng hàm lưu ảnh JPEG
void saveJPG(camera_fb_t *fb,const char* filename){ // Định nghĩa hàm với 2 tham số đầu ra
    // Cấp phát bộ nhớ đệm chọ jpg_buf để lưu trữ ảnh JPG được chuyển đổi từ dữ liệu bitmap
    // Cấp phát bộ nhớ trong vùng phạm psram (RAM của SPI) bằng heap_caps_mallo với kích thước 200000 byte
    uint8_t * jpg_buf = (uint8_t *) heap_caps_malloc(200000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if(jpg_buf == NULL){
    printf("Malloc failed to allocate buffer for JPG.\n");
    }else{
    size_t jpg_size = 0; // Khởi tạo biến jpg_size để lưu trữ kích thước của ảnh JPG sau khi chuyển đổi

    // Tạo một đối tượng memory_stream để quản lý bộ đệm ảnh JPG.
    // Chuyển đổi dữ liệu hình ảnh từ định dạng gốc thành định dạng JPG thông qua hàm convert_image.
    fmt2jpg(fb->buf, fb->len, fb->width, fb->height,fb->format, 31, &jpg_buf, &jpg_size);
    printf("Converted JPG size: %d bytes \n", jpg_size);
    FILE *file = fopen(filename, "w"); // Mở tập tin có tên từ filename để ghi ảnh JPG vào chế độ ghi w
    if (file == NULL)
    {
        printf("err: fopen failed\n");
    }
    else
    {
        fwrite(jpg_buf, 1, jpg_size, file); // Ghi dữ liệu từ jpg_buf (ảnh jpg đã chuyển đổi) vào tập tin đã mở từ file
        fclose(file); // đóng file
    }
    free(jpg_buf); // Giải phóng bộ nhớ cấp phát cho jpg_buf
    esp_camera_fb_return(fb); // Trả lại frame buffer của máy ảnh về trình điều khiển máy ảnh (camera driver) để tái sử dụng
    }
}
void saveBmp(camera_fb_t* fb,const char* filename) { // Định nghĩa hàm với 2 tham số đầu ra
  // Khởi tạo các biến img, imgWidth, imgHeight để lưu trữ thông tin về dữ liệu hình ảnh từ framebuffer - khung bộ đệm
  uint16_t* img = (uint16_t*)fb->buf; // Con trỏ tới dữ liệu hình ảnh
  uint16_t imgWidth = fb->width; // Height và Width là kích thước chiều rộng chiều cao của hình ảnh
  uint16_t imgHeight = fb->height;
    
  // BMP file header
  uint32_t fileSize = imgWidth * imgHeight * 3 + 54; // tính toán kích thước của tệp BMP
  uint8_t bmpHeader[54] = { // Tạo một mảng bmpHeader có 54 bytes để lưu trữ thông tin của tệp bitmap
    'B', 'M',                                     // Chữ ký
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
    FILE *file = fopen(filename, "w"); // Mở tệp để ghi dữ liêu BMP
    if (file == NULL)
    {
        printf("err: fopen failed\n");
    }
    else
    {
        fwrite(bmpHeader, 1, 54, file); // Ghi tiêu đề của tệp bitmap kích thước 54 bytes

  // Write image data - Ghi dữ liệu của ảnh
  // Padding là phần ngoài của ảnh được thêm vào để tăng tính toàn vẹn cho ảnh
  int padding = (4 - ((imgWidth * 3) % 4)) % 4; // Đảm bảo số lượng kích thước mỗi hàng là bội số của 4
  uint8_t pixelBuffer[3]; // Khai báo một mảng có 3 bytes để lưu trữ giá trị màu của mỗi pixel trong ảnh
  // Vì ảnh BMP được lưu từ trên xuống
  for (int y = imgHeight - 1; y >= 0; y--) {     // Bắt đầu từ hàng cuối cùng
    for (int x = 0; x < imgWidth; x++) { // Chạy qua từng pixel của hàng hiện tại
      uint16_t color = img[y * imgWidth + x]; // Lấy giá trị màu của pixel tại vị trí hiện tại từ frame buffer
      // Trích xuất màu từ giá trị màu của pixel color
      uint8_t r = ((color) >> 0);    // R: 5 bit, dịch sang phải 8 bit
      uint8_t g = ((color) >> 0);    // G: 6 bit, dịch sang phải 3 bit
      uint8_t b = ((color) << 0);    // B: 5 bit, dịch sang trái 3 bit
  
      // Chuẩn bị dữ liệu pixel - bằng cách lưu các giá trị màu r, g, b vào các mảng pixelBuffer
      pixelBuffer[0] = b;
      pixelBuffer[1] = g;
      pixelBuffer[2] = r;
  
      // Ghi dữ liệu pixel vào tệp bmp với độ dài là 3 bytes
      fwrite(pixelBuffer, 1, 3, file);
    }
    // Ghi các byte padding như đã tính ở bước 1
    // Bội số của 4 ở đây được yêu cầu bởi hệ thống máy tính thì thường sử dụng kiến trúc 32bit hoặc 64 bit
    for (int p = 0; p < padding; p++) {
      fwrite(&padding, 1, 1, file);
    }
    }
    fclose(file); // Dòng này đóng tệp BMP sau khi đã ghi xong dữ liệu ảnh.
    }
}
// Cân bằng lượng đồ màu của ảnh tử fb giúp tăng cường độ tương phản và cải thiện chất lượng hình ảnh
void histogramequalization(camera_fb_t* pic) { 
    // Khởi tạo mảng histogram có 256 phần tử, mỗi phần tử đại diện cho giá trị màu từ 0 đến 255
    // Mảng histogram sử dụng để đếm số lần xuất hiện của mỗi giá trị màu trong ảnh
    int histogram[256] = {0};
    // Vòng lặp duyệt qua từng pixel của ảnh từ khung bộ đệm (pic) - đếm số lần xuất hiện của các giá trị màu từ 0 đến 255
    for (int i = 0; i < pic->len; i += 2) {
        histogram[pic->buf[i]]++; // tăng giá trị tương ứng trong mảng histogram cho mỗi giá trị màu pixel
    }

    // khởi tạo mảng cumulative histogram với giá trị 0 cho tất cả phần tử
    int cumulativeHistogram[256] = {0};
    cumulativeHistogram[0] = histogram[0]; // Sử dụng tính toán giá trị tươnng ứng của lược đồ cân bằng
    for (int i = 1; i < 256; i++) {
        // Vòng lặp này tính toán giá trị tích lũy của mảng histogram. Kết quả của mảng cumulativeHistogram là một lược đồ màu đã cân bằng.
        cumulativeHistogram[i] = cumulativeHistogram[i - 1] + histogram[i];
    }
// Giá trị tích luỹ là lấy giá trị đầu tiên của phần từ trong mảng cumulativeHistogram[0] thì sẽ bẳng với histogram[0]
// Nhưng tại giá trị thứ 2 thì sẽ là cumulativeHistogram[1] = cumulativeHistogram[0]+histogram[1]
// -> Sau quá trình này nó sẽ chứa các giá trị tích luỹ của histogram tạo thành lược đồ màu.

    // Số pixel trong ảnh được tính bằng cách chia độ dài pic->len cho 2 vì mỗi pixel được biểu diễn bằng 2 bytes trong ảnh
    int totalPixels = pic->len / 2;
    // Duyệt qua từng pixel và cân bằng histogram
    for (int i = 0; i < pic->len; i += 2) {
        uint8_t pixelValue = pic->buf[i]; // Lấy giá trị màu
        // Lấy giá gị màu mới bằng cách
        /*
        Lấy giá trị từ cumulativeHistogram tương ứng với giá trị mà của pixel
        Sau đó nhân với 255 rồi chia cho tổng số pixel trong ảnh
        -> Khi đó equalizedValue sẽ nằm trong khoảng từ 0 đến 255 tạo thành một lượng đồ cân bằng
        */
        uint8_t equalizedValue = (cumulativeHistogram[pixelValue] * 255) / totalPixels;
        pic->buf[i] = equalizedValue; // Gán giá trị màu cho pixel trong buffer 
        pic->buf[i + 1] = equalizedValue; // Vì ảnh là ảnh 16 bit RGB565 nên gán vào lớp thứ 2 của pixel
    }
}
void thresholdingBinary(camera_fb_t* pic, uint8_t threshold) { // định nghĩa hàm với 2 tham số đầu vào
// Vòng lặp kích thước là chiều dài của toàn bộ buffer của ảnh
  for (int i = 0; i < pic->len; i++) { 
    if (pic->buf[i] < threshold) { //gọi vị trí từng pixel trong buffer
    // Nếu giá trị pixel đó thấp hơn ngưỡng thì tăng lên 255 thành mức màu trắng
      pic->buf[i] = 255; // Đặt giá trị thành mức xám tối đa (trắng)
    } else {
    // Còn nếu cao hơn ngưỡng thì chuyển thành mức màu đen
      pic->buf[i] = 0; // Đặt giá trị thành mức xám tối thiểu (đen)
    }
  }
}
static camera_fb_t* crop_image(camera_fb_t *pic, unsigned short cropLeft, unsigned short cropRight, unsigned short cropTop, unsigned short cropBottom)
{   // Tính chỉ số tối đa cho hàng trên cùng được giữ
    unsigned int maxTopIndex = cropTop * pic->width * 2;
    // Tính chỉ số tối thiểu để hàng dưới cùng được giữ
    unsigned int minBottomIndex = ((pic->width*pic->height) - (cropBottom * pic->width)) * 2;
    // Tính toán tọa độ X tối đa (độc quyền) cho cột ngoài cùng bên phải được giữ lại
    unsigned short maxX = pic->width - cropRight; // In pixels
    // Tính chiều rộng và chiều cao mới sau khi cắt xén
    unsigned short newWidth = pic->width - cropLeft - cropRight;
    unsigned short newHeight = pic->height - cropTop - cropBottom;
    // Tính toán kích thước mới của bộ đệm khung sau khi cắt xén
    size_t newJpgSize = newWidth * newHeight *2;

    // Khởi tạo writeIndex cho bộ đệm khung mới
    unsigned int writeIndex = 0;
    // Lặp lại tất cả các byte trong bộ đệm khung ban đầu
    for(int i = 0; i < pic->len; i+=2){
        // Tính toán vị trí pixel X hiện tại dựa trên chỉ mục hiện tại
        int x = (i/2) % pic->width;

        // Cắt từ trên xuống: nếu chỉ mục hiện tại nằm trước maxTopIndex, hãy bỏ qua pixel này
        if(i < maxTopIndex){ continue; }

        // Cắt từ dưới lên: nếu chỉ mục hiện tại nằm sau minBottomIndex, hãy bỏ qua pixel này
        if(i > minBottomIndex){ continue; }

        // Cắt từ bên trái: nếu tọa độ X hiện tại nhỏ hơn hoặc bằng cropLeft, bỏ qua pixel này
        if(x <= cropLeft){ continue; }

        // Cắt từ bên phải: nếu tọa độ X hiện tại lớn hơn cropRight, bỏ qua pixel này
        if(x > maxX){ continue; }

        // Sao chép giá trị pixel hiện tại (2 byte) vào bộ đệm khung mới
        pic->buf[writeIndex++] = pic->buf[i];
        pic->buf[writeIndex++] = pic->buf[i+1];
    }

    // Trả lại bộ đệm khung đã sửa đổi với hình ảnh đã cắt.
    pic->width = newWidth;
    pic->height = newHeight;
    pic->len = newJpgSize;
  return pic;
}
void inc_brightness(camera_fb_t *fb, int c) {
    // Cách làm giống với lấy ngưỡng
    // Duyệt từng pixel trong bitmap bằng cách chạy vòng lặp
    // Nếu giá trị pixel + với giá trị độ sáng lớn hơn giá trị màu trắng thì giữ nguyên giá trị pixel tại vị trí đó
    // Nếu không thì sẽ tăng giá trị pixel trong bitmap bằng cách cộng thêm giá trị độ sáng
  for(int i= 0; i<fb->len;i++){
    if (fb->buf[i]+c>255){
        fb->buf[i]=fb->buf[i];
    } else {
      fb->buf[i]=fb->buf[i]+c;
    }
  }
}
void erosion(camera_fb_t* pic, int kernelSize) {
    // Gán kích thước của buffer vào biến height và width
    int width = pic->width;
    int height = pic->height;
    int channels = 3; // ảnh là ảnh RGB565 nên số lớp channels là 3

    // Tạo không gian bộ đệp lưu trữ tạm thời kích thước bằng kích thước buffer
    uint8_t* erodedBuffer = (uint8_t*)malloc(pic->len);
    // Gán dữ liệu từ bộ đệm buffer và chiều dài vào biến được khởi tạo
    memcpy(erodedBuffer, pic->buf, pic->len);

    // Vòng lặp duyệt qua các hàng của ảnh (trừ lề đáy và đỉnh có kích thước kernelSize )
    for (int y = kernelSize; y < height - kernelSize; y++) {
        // Vòng lặp duyệt qua các cột của hình (trừ lề trái và lề phải có kích thước kernelSize )
        for (int x = kernelSize; x < width - kernelSize; x++) {
            // Khởi tạo biến giá trị 255 đại diện cho pixel trắng
            uint8_t minPixelValue = 255;
            // Vòng lặp duyệt qua các hàng của kernel (vùng xói mòn)
            for (int ky = -kernelSize; ky <= kernelSize; ky++) {
                // Vòng lặp duyệt qua các cột của kernel (vùng xói mòn)
                for (int kx = -kernelSize; kx <= kernelSize; kx++) {
                    int offsetX = x + kx; // tính toán toạ độ X của pixel dựa trên vị trí x và vị trí cột kx
                    int offsetY = y + ky;
                    // lấy mức giá trị xám (đen) của pixel tại toạ độ trong hình ảnh gốc và lưu vào pixelValue
                    uint8_t pixelValue = pic->buf[(offsetY * width + offsetX) * channels];
                    if (pixelValue < minPixelValue) {
                        minPixelValue = pixelValue; // Tìm giá trị pixel nhỏ nhất trong vùng xói mòn
                    }
                }
            }

            // cập nhật giá trị nhỏ nhất trong vùng xói mòn vào vị trí tại hàng y và cột x của biến erodedBuffer
            erodedBuffer[(y * width + x) * channels] = minPixelValue;
        }
    }

    // Sau khi thực hiện xong phép toán của xói mòn -> sao chép các giá trị pixel trở lại frame buffer
    memcpy(pic->buf, erodedBuffer, pic->len);

    // Giải phóng bộ nhớ cho erodedBuffer
    free(erodedBuffer);
}

void app_main()
{
    esp_err_t err;
    err = init_camera();
    if (err != ESP_OK)
    {
        printf("err: %s\n", esp_err_to_name(err));
        return;
    }
    
    err = initi_sd_card();
    if (err != ESP_OK)
    {
        printf("err: %s\n", esp_err_to_name(err));
        return;
    }

    camera_fb_t * pic = take_photo(NULL);
    saveBmp(pic,"/sdcard/pic.bmp");
    printf("SaveImageThanhCong\n");
    
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    pic = crop_image(pic,165,265,125,295);
    saveBmp(pic,"/sdcard/piccrop.bmp");
    printf("SaveImageThanhCong\n");
    ////////////Crop 5 anh nho

    // Tạo một không gian để lưu trữ các dữ liệu chụp từ camera - kich thước bằng kích thước của buffer
    camera_fb_t *tempfb = (camera_fb_t *)malloc(sizeof(camera_fb_t)); 
    if (tempfb == NULL) {
    // Xử lý lỗi không thể cấp phát bộ nhớ
    printf("Failed to allocate memory for tempfb\n");
    return;
    }

    // Gán các giá trị dữ liệu được chụp vào con trỏ được khởi tạo trong bộ nhớ đệm
    // Dùng để gọi ra mỗi khi chụp xong và giải phóng bộ nhớ
    tempfb->buf=pic->buf;
    tempfb->width = pic->width;
    tempfb->height = pic->height;
    tempfb->len = pic->len;

    
    camera_fb_t *pic1,*pic2,*pic3,*pic4,*pic5;

    ////// Anh 1///////////////////
    pic1 = crop_image(pic,5,180,10,3);
    inc_brightness(pic1,80);
    histogramequalization(pic1);
    thresholdingBinary(pic1,115);
    //erosion(pic1,3);
    //saveBmp(pic1,"/sdcard/pic1.bmp");
    saveJPG(pic1,"/sdcard/pic1.jpg");
    printf("SaveImage1ThanhCong\n");
  
    ////// Anh 2///////////////////
    pic->buf = tempfb->buf;
    pic->width = tempfb->width;
    pic->height = tempfb->height;
    pic->len = tempfb->len;
    pic2 = crop_image(pic,45,132,10,3);
    inc_brightness(pic2,80);
    histogramequalization(pic2);
    thresholdingBinary(pic2,115);
    //erosion(pic2,3);
    //saveBmp(pic2,"/sdcard/pic2.bmp");
    saveJPG(pic2,"/sdcard/pic2.jpg");
    printf("SaveImage2thanhCong\n");

    // ////// Anh 3///////////////////
    // pic->buf = tempfb->buf;
    // pic->width = tempfb->width;
    // pic->height = tempfb->height;
    // pic->len = tempfb->len;
    // pic3 = crop_image(pic,88,90,10,3);
    // inc_brightness(pic3,80);
    // histogramequalization(pic3);
    // thresholdingBinary(pic3,115);
    // //erosion(pic3,3);
    // //saveBmp(pic3,"/sdcard/pic3.bmp");
    // saveJPG(pic3,"/sdcard/pic3.jpg");
    // printf("SaveImage3thanhCong\n");

    // ////// Anh 4///////////////////
    // pic->buf = tempfb->buf;
    // pic->width = tempfb->width;
    // pic->height = tempfb->height;
    // pic->len = tempfb->len;
    // pic4 = crop_image(pic,130,47,10,3);
    // inc_brightness(pic4,80);
    // histogramequalization(pic4);
    // thresholdingBinary(pic4,115);
    // //erosion(pic4,3);
    // //saveBmp(pic4,"/sdcard/pic4.bmp");
    // saveJPG(pic4,"/sdcard/pic4.jpg");
    // printf("SaveImage4hanhCong\n");

    // ////// Anh 5///////////////////
    // pic->buf = tempfb->buf;
    // pic->width = tempfb->width;
    // pic->height = tempfb->height;
    // pic->len = tempfb->len;
    // pic5 = crop_image(pic,170,5,10,3);
    // inc_brightness(pic5,80);
    // histogramequalization(pic5);
    // thresholdingBinary(pic5,115);
    // //erosion(pic5,3);
    // //saveBmp(pic5,"/sdcard/pic5.bmp");
    // saveJPG(pic5,"/sdcard/pic5.jpg");
    // printf("SaveImage5thanhCong\n");
   
    esp_camera_fb_return(pic);
    
    free(tempfb);
    /////////////////////////////////////////////////////////////
    // int cropCoordinates[5][4] = {
    // {5, 180, 5, 5},
    // {35, 145, 0, 0},
    // {70, 115, 0, 0},
    // {105, 80, 0, 0},
    // {140, 45, 0, 0}
    // };
    // for (int i = 0; i < 5; i++) {
    // pic->buf = tempfb->buf;
    // pic->width = tempfb->width;
    // pic->height = tempfb->height;
    // pic->len = tempfb->len;
    // pic = crop_image(pic, cropCoordinates[i][0], cropCoordinates[i][1], cropCoordinates[i][2], cropCoordinates[i][3]);
    // // Lưu ảnh vào file
    // char photo_name[50];
    // sprintf(photo_name, "/sdcard/pic%d.bmp", i+1);
    // saveBmp(pic, photo_name);
    
    // printf("SaveImage%dthanhCong\n", i+1);
    
    // // Giải phóng bộ nhớ của ảnh đã cắt
    // esp_camera_fb_return(pic);
    
    // // Delay giữa các lần chụp ảnh
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    // free(tempfb);
    // //Tang sang
    // inc_brightness(pic,80);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);

    // //Anh nhi phan
    // thresholdingBinary(pic,115);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);

    // //Save anh
    // saveBmp(pic);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // esp_camera_fb_return(pic);

    // camera_fb_t* pic1, *pic2,*pic3,*pic4,*pic5;
    // pic1 = crop_image(pic,1,175,1,55);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // saveBmp(pic1);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    
}