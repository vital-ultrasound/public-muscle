

import sys
import torch
import matplotlib.pyplot as plt
import numpy as np
import RFSeg_worker as rfseg_worker
import SimpleITK as sitk
import time
import rfutils as utils

def resample(image, desired_size):
    size = desired_size
    origin = image.GetOrigin()
    spacing = [(s2 - 1) * sp2 / (s1 - 1) for s1, s2, sp2 in zip(desired_size, image.GetSize(), image.GetSpacing())]

    ref = sitk.Image(size, sitk.sitkInt8)
    ref.SetOrigin(origin)
    ref.SetSpacing(spacing)

    # resample
    identity_transform = sitk.AffineTransform(image.GetDimension())
    identity_transform.SetIdentity()
    image = sitk.Resample(image, ref, identity_transform, sitk.sitkLinear, 0)

    return image



def read_data(path, desired_size):
    image = sitk.ReadImage(path)
    image = resample(image, desired_size)
    image, info_im = utils.sitkToTorch(image, transpose=True)
    image = image.type(torch.float).numpy()

    return image

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Missing one argument-input image")
        exit(-1)
    im_file = sys.argv[1]
    print('Input image: {}'.format(im_file))

    modelfolder = 'model'
    modelname = 'model_5l_AugPlus.pth'
    desired_size = (128, 128)
    X_test = read_data(path=im_file, desired_size=desired_size)
    # apply transforms
    X_test = X_test/255.0
    imsize = X_test.shape

    rfseg_worker.initialize(imsize, modelfolder, modelname, verbose=True)
    imidx = 0


    print('Run the model inference')
    startt = time.time()
    pred_segmentation = rfseg_worker.dowork(X_test)
    endt = time.time()
    print('Elapsed time: {}s'.format(endt - startt))
    pred_segmentation = pred_segmentation[0, ...].squeeze()

    pred_segmentation_t = (pred_segmentation.cpu().numpy() > 0.5).astype(np.uint8)
    pred_area = np.sum(pred_segmentation_t)

        # Perform a sanity check on some random test samples
    plt.subplot(1,2,1)
    plt.imshow(X_test.transpose(), cmap='gray')
    plt.title('Input')
    #

    plt.subplot(1,2,2)
    plt.imshow(X_test.transpose(), cmap='gray')
    plt.imshow(pred_segmentation_t.transpose(), alpha=pred_segmentation_t.transpose()*0.7, cmap='copper')
    plt.title('Predicted')
    plt.show()
