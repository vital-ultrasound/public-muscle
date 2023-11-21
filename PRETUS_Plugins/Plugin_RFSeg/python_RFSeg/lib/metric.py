__author__ = "Hamideh Kerdegari"
__copyright__ = "Copyright 2021"
__credits__ = ["Hamideh Kerdegari"]
__license__ = "Hamideh Kerdegari"
__version__ = "0.0.1"
__maintainer__ = "Hamideh Kerdegari"
__email__ = "hamideh.kerdegari@gmail.com"
__status__ = "R&D"

import keras.backend
from keras import backend as K
import tensorflow as tf


def mean_iou(y_true, y_pred):
    y_pred = K.cast(K.greater(y_pred, .5), dtype='float32') # .5 is the threshold
    inter = K.sum(K.sum(K.squeeze(y_true * y_pred, axis=3), axis=2), axis=1)
    union = K.sum(K.sum(K.squeeze(y_true + y_pred, axis=3), axis=2), axis=1) - inter
    return K.mean((inter + K.epsilon()) / (union + K.epsilon()))


def dice_coef(y_true, y_pred):
    smooth = 1.
    y_true_f = tf.reshape(tf.dtypes.cast(y_true, tf.float32), [-1])
    y_pred_f = tf.reshape(tf.dtypes.cast(y_pred, tf.float32), [-1])
    intersection = tf.reduce_sum(y_true_f * y_pred_f)
    return (2. * intersection + smooth) / (tf.reduce_sum(y_true_f) + tf.reduce_sum(y_pred_f) + smooth)