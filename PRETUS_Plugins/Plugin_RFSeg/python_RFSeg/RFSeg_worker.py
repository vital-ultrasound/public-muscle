
import torch
import unet as rfunet
import numpy as np
from PIL import Image

net = None
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")


def initialize(input_size, python_path, modelname, verbose=False):
    global device
    global net
    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
    net = rfunet.UNet(input_size=input_size)
    net.to(device)
    net.eval()
    if verbose:
        print(net)
        print("the model is : " + modelname)

    net_params = torch.load('{}/{}'.format(python_path, modelname))
    net.load_state_dict(net_params)
    return True


def get_sobel_kernel(k=3):
    # get range
    range = np.linspace(-(k // 2), k // 2, k)
    # compute a grid the numerator and the axis-distances
    x, y = np.meshgrid(range, range)
    sobel_2D_numerator = x
    sobel_2D_denominator = (x ** 2 + y ** 2)
    sobel_2D_denominator[:, k // 2] = 1  # avoid division by zero
    sobel_2D = sobel_2D_numerator / sobel_2D_denominator
    return sobel_2D

def dowork(image_cpp, verbose=0):
    with torch.no_grad():
        #np.save('/home/ag09/data/VITAL/input.npy', image_cpp)
        image_cpp = image_cpp.transpose() # maybe do in cpp?
        #im = Image.fromarray(image_cpp)
        #im.save("/home/ag09/data/VITAL/input.png")
        image_cpp = torch.from_numpy(image_cpp).type(torch.float).to(device).unsqueeze(0).unsqueeze(0)/255.0
        try:
            # output must be int8
            segmentation_out = net(image_cpp)
            do_edges=False
            if do_edges:
                k_sobel = 3
                sobel_2D = get_sobel_kernel(k_sobel)
                sobel_filter_x = torch.nn.Conv2d(in_channels=1,
                                                out_channels=1,
                                                kernel_size=k_sobel,
                                                padding=k_sobel // 2,
                                                bias=False)
                sobel_filter_x.weight[:] = torch.from_numpy(sobel_2D)
                sobel_filter_x.to(device)


#                sobel_filter_y = nn.Conv2d(in_channels=1,
#                                                out_channels=1,
#                                                kernel_size=k_sobel,
#                                                padding=k_sobel // 2,
#                                                bias=False)
#                sobel_filter_y.weight[:] = torch.from_numpy(sobel_2D.T)
#                sobel_filter_y.to(device)
                segmentation_out[segmentation_out>0.5] = 1
                segmentation_out = sobel_filter_x(segmentation_out)

            segmentation = (segmentation_out.squeeze()*255.0).type(torch.uint8).cpu().numpy()
            segmentation = np.ascontiguousarray(segmentation.transpose())
            #n_pixels = np.count_nonzero(segmentation)
            #im = Image.fromarray(segmentation)
            #im.save("/home/ag09/data/VITAL/output.png")
            #np.save('/home/ag09/data/VITAL/output.npy', segmentation)
            return segmentation
        except Exception as inst:
            print("RFSeg_worker.py ::WARNING::  exception")
            print(type(inst))    # the exception instance
            print(inst.args)     # arguments stored in .args
            print(inst)          # _
