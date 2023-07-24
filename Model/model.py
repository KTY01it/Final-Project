import tensorflow as tf
from tensorflow import keras
from keras import Sequential
from keras.layers import Dense
from keras import Input
import matplotlib.pyplot as plt
import numpy as np
import cv2 as cv
from keras.utils import load_img, img_to_array
from keras.models import Sequential
from keras.layers import Dense, Dropout, Conv2D, MaxPooling2D, Flatten
from keras.optimizers import Adam


import os
train_image_files_path='C:/Users/ADMIN/Downloads/newdata/datanumber2/filetrain'
valid_image_files_path='C:/Users/ADMIN/Downloads/newdata/datanumber2/filevalid'

label=['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']

from keras.preprocessing.image import ImageDataGenerator
train_data_gen = ImageDataGenerator(rescale=1/255)
valid_data_gen = ImageDataGenerator(rescale=1/255)

train_generator = train_data_gen.flow_from_directory(
    train_image_files_path,
    target_size=(31,43),
    class_mode='categorical'
)

valid_generator = valid_data_gen.flow_from_directory(
    valid_image_files_path,
    target_size=(31,43),
    class_mode='categorical'
)

model=Sequential()

#LopCNN1
model.add(Conv2D(4,(3,3), activation='relu',input_shape=(31,43,3)))
model.add(MaxPooling2D(2,2))

#LopCNN2
model.add(Conv2D(32,(3,3), activation='relu'))
model.add(MaxPooling2D(2,2))

#LopCNN3
model.add(Conv2D(64,(3,3), activation='relu'))
model.add(MaxPooling2D(2,2))

model.add(Flatten())
model.add(Dense(70,activation=tf.nn.relu))
model.add(Dense(10, activation=tf.nn.softmax))

model.summary()

model.compile(optimizer=Adam(learning_rate=0.001),
              loss='categorical_crossentropy',
              metrics=['acc'])

EPOCHS=80
history=model.fit(
    train_generator,
    steps_per_epoch=10,
    epochs=EPOCHS,
    verbose=1,
    validation_data=valid_generator,
    validation_steps=5
)

fig, axes = plt.subplots(1,2, figsize=(18, 6))
# Plot training & validation accuracy values
axes[0].plot(history.history['acc'])
axes[0].plot(history.history['val_acc'])
axes[0].set_title('Model accuracy')
axes[0].set_ylabel('Accuracy')
axes[0].set_xlabel('Epoch')
axes[0].legend(['Train', 'Validation'], loc='upper left')

# Plot training & validation loss values
axes[1].plot(history.history['loss'])
axes[1].plot(history.history['val_loss'])
axes[1].set_title('Model loss')
axes[1].set_ylabel('Loss')
axes[1].set_xlabel('Epoch')
axes[1].legend(['Train', 'Validation'], loc='upper left')
plt.show()

img_gray = cv.imread('C:/Users/ADMIN/Downloads/PIC1.jpg')

# resized_image = cv.resize(img_gray, (31, 43))

plt.imshow(img_gray)

x=tf.keras.utils.img_to_array(img_gray)
# x = np.transpose(x, (1, 0, 2))
x=np.expand_dims(x,axis=0)
images= np.vstack([x])
y_predict = model.predict(images, batch_size=10)
predicted_numbers = np.argmax(y_predict, axis=1)
print('Gia tri du doan:', label[np.argmax(y_predict)])