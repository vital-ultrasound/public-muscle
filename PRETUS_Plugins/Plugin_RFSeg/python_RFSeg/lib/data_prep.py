__author__ = "Hamideh Kerdegari"
__copyright__ = "Copyright 2021"
__credits__ = ["Hamideh Kerdegari"]
__license__ = "Hamideh Kerdegari"
__version__ = "0.0.1"
__maintainer__ = "Hamideh Kerdegari"
__email__ = "hamideh.kerdegari@gmail.com"
__status__ = "R&D"

import os
import numpy as np
from skimage.io import imread
from skimage.transform import resize


#import train and test pathes here.
train_path = "/home/hk20/Group2-MUSCLE/muscle-data/Train"
test_path = "/home/hk20/Group2-MUSCLE/muscle-data/Test"


IMG_WIDTH = 128
IMG_HEIGHT = 128

def read_data(path: str):
    IMG_WIDTH = 128
    IMG_HEIGHT = 128
    X = []
    Y = []
    for root, dirs, files in os.walk(path, topdown=False):
        for file in files:
            if file.endswith(".png") and "img" in root and "mask" not in root:
                img_path = os.path.join(root, file)
                mask_path = os.path.join(root.replace("img", "mask"), file.replace("input", "label"))
                if os.path.exists(mask_path):
                    img = imread(img_path)
                    img = np.expand_dims(resize(img, (IMG_HEIGHT, IMG_WIDTH), mode='constant', preserve_range=True), axis=-1)

                    mask = imread(mask_path)
                    mask = (mask < 127) * 1.0
                    mask = np.expand_dims(resize(mask, (IMG_HEIGHT, IMG_WIDTH), mode='constant', preserve_range=True), axis=-1)


                    X.append(img)
                    Y.append(mask)


    return np.array(X), np.array(Y).astype(float)

