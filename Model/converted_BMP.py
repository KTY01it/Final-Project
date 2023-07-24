import cv2

# Đường dẫn đến tệp ảnh JPG
jpg_path = 'C:/Users/ADMIN/Downloads/datanumber/filetest/9/9 (5).jpg'

# Đọc ảnh từ tệp JPG
image = cv2.imread(jpg_path)

if image is None:
    print("Không thể đọc ảnh!")
    exit()

resize_image = cv2.resize(image,(31, 43))
# Đường dẫn đến tệp ảnh BMP đầu ra
bmp_path = 'C:/Users/ADMIN/Downloads/datanumber/Image9.bmp'

# Chuyển đổi ảnh sang định dạng BMP và lưu vào tệp
cv2.imwrite(bmp_path, resize_image)

print("Đã chuyển đổi ảnh thành công sang định dạng BMP!")
