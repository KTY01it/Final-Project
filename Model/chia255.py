import cv2

# Đường dẫn tới tệp ảnh
image_path = 'C:/Users/ADMIN/Downloads/datanumber/PIC1.BMP'

# Đọc ảnh từ tệp
image = cv2.imread(image_path)
if image is None:
    print("Không thể đọc ảnh")
    exit()

# Hiển thị ảnh trước khi chia
cv2.imshow("Trước khi chia", image)
cv2.waitKey(0)

# Chia ảnh cho 255
image_normalized = image.astype(float) / 255.0

# Hiển thị ảnh sau khi chia
cv2.imshow("Sau khi chia", image_normalized)
cv2.waitKey(0)
