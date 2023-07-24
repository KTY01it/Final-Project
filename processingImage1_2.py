import cv2 as cv
import numpy as np
import imutils
from matplotlib import pyplot as plt
from flatten_json import flatten
from scipy.ndimage import median_filter

img_path = 'C:/Users/ADMIN/Downloads/datanumber2/picture7.jpg'
img = cv.imread(img_path)
img_gray = cv.imread(img_path, cv.IMREAD_GRAYSCALE)
crop2_img_name = 'gray_image.jpg' 
cv.imshow('gray_image', img_gray)
# cv.imwrite(crop2_img_name, img_gray)


# rotate = imutils.rotate(img_gray, 3)

cropped_image = img_gray[168:223, 225:473]
crop_img_name = 'cropped_image.jpg' 
cv.imshow('cropped_image', cropped_image)
# cv.imwrite(crop_img_name, cropped_image)

# brightness = 10 
# contrast = 2.3 
# image2 = cv.addWeighted(cropped_image, contrast, np.zeros(cropped_image.shape, rotate.dtype), 0, brightness)
# # cv.imshow('image_his3', image2)

# kernel2 = np.array([[0, -1, 0],
#                     [-1, 5,-1],
#                     [0, -1, 0]])
# image_sharp = cv.filter2D(src=image2, ddepth=-1, kernel=kernel2)
#     # cv.imshow('image_his5', image_sharp)

# blur1 = cv.GaussianBlur(image_sharp,(3, 3),0)
# sharpe = cv.subtract(image2, blur1)
#     # cv.imshow('blur1', blur1)

cropped2 = cropped_image[0:54, 208:248]
crop2_img_name = 'cropped2.jpg' 
cv.imshow('cropped2', cropped2)
# cv.imwrite(crop2_img_name, cropped2)

img_blur = cv.blur(src=cropped2, ksize=(3,3)) # Using the blur function to blur an image where ksize is the kernel size
blur_img_name = 'blur_image.jpg' 
cv.imwrite(blur_img_name, img_blur)

hist,bins = np.histogram(img_blur.flatten(), 256, [0,256])
cdf = hist.cumsum()
cdf_normalized = cdf * float(hist.max())/cdf.max()
cdf_m = np.ma.masked_equal(cdf, 0)
cdf_m = (cdf_m - cdf_m.min())*255 / (cdf_m.max()-cdf_m.min())
cdf = np.ma.filled(cdf_m, 0) .astype('uint8')
img_his = cdf[img_blur]
hist_img_name = 'hist_image.jpg' 
cv.imshow('hist_image', img_his)
# cv.imwrite(hist_img_name, img_his)

    
thresh, img_binary = cv.threshold(img_his, thresh=40, maxval=255, type=cv.THRESH_BINARY_INV)
binary_img_name = 'threshold_image.jpg' 
cv.imshow('threshold_image', img_binary)
# cv.imwrite(binary_img_name, img_binary)
    
kernel = np.ones((3, 3), np.uint8)
img_erosion = cv.erode(img_binary, kernel, iterations=1)
# img_dilation = cv.dilate(img_binary, kernel, iterations=1)
erosion_img_name = 'img_erosion.jpg'
# cv.imwrite(erosion_img_name, img_erosion)
# cv.imshow('image_his1', img_his)
# cv.imshow('img_binary', img_binary)
cv.imshow('img_erosion', img_erosion)
# cv.imshow('img_dilation', img_dilation)
cv.waitKey(0)
cv.destroyAllWindows()