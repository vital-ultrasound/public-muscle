"""
This script helps identify good pairs of images/labels and save them to a folder.
"""

import os
from glob import glob
# for gui
import tkinter
from tkinter import messagebox
import PIL
from PIL import Image
from PIL import ImageTk
import cv2




in_folder = '/home/hk20/Group2-MUSCLE/TETANUS/01NVb-003-251/T2/01NVb-003-251-2_annotations'
out_folder = '/home/hk20/Group2-MUSCLE/TETANUS/01NVb-003-251/T2/01NVb-003-251-2_annotations_clean'

####Here, imaging and mask that are clean are saved in separate folders.
# Directory path
img_dir = '/home/hk20/Group2-MUSCLE/TETANUS/01NVb-003-251/T2/01NVb-003-251-2_annotations_clean/img'
mask_dir = '/home/hk20/Group2-MUSCLE/TETANUS/01NVb-003-251/T2/01NVb-003-251-2_annotations_clean/mask'

# Make directory
os.makedirs(img_dir)
os.makedirs(mask_dir)


os.makedirs('{}'.format(out_folder), exist_ok=True)

def produce_tri_image(image_fn, label_fn):
    cv_img, height, width, no_channels = load_image(image_fn)
    cv_label, _, _, _ = load_image(label_fn)
    cv_label_color = cv_label.copy()
    cv_label_color[...,1] = 0
    overlay = cv2.addWeighted(cv_img,1,cv_label_color,0.2,0)
    total_image = cv2.hconcat((cv_img, cv_label, overlay))
    return total_image

def load_image(filename):
    cvimg = cv2.imread(filename)
    height, width, no_channels = cvimg.shape
    # OpenCV represents images in BGR order; however PIL represents
    # images in RGB order, so we need to swap the channels
    cvimg = cv2.cvtColor(cvimg, cv2.COLOR_BGR2RGB)
    return cvimg, height, width, no_channels

# Callback for the "<" button
def back_image():
    global photo
    global image_idx
    global input_images

    image_idx -= 1
    if image_idx < 0:
        image_idx = len(input_images)-1

    statusbar.insert(tkinter.INSERT, 'Loading:\n')
    statusbar.insert(tkinter.INSERT, '- {}\n'.format(input_images[image_idx]))
    statusbar.insert(tkinter.INSERT, '- {}\n'.format(label_images[image_idx]))
    statusbar.see(tkinter.END)

    total_image = produce_tri_image(input_images[image_idx], label_images[image_idx])

    photo = PIL.ImageTk.PhotoImage(image = PIL.Image.fromarray(total_image))
    canvas.create_image(0, 0, image=photo, anchor=tkinter.NW)
    window.title('Image {}/{}'.format(image_idx, len(input_images)))

# Callback for the ">" button
def next_image():
    global photo
    global image_idx
    global input_images

    image_idx += 1
    if image_idx >= len(input_images):
        result = messagebox.askyesno(title='Finished!', message="You have gone through {} images. Do you wish to re-start?".format(len(input_images)))
        if result == False:
            exit()
        image_idx = 0

    statusbar.insert(tkinter.INSERT, 'Loading:\n')
    statusbar.insert(tkinter.INSERT, '- {}\n'.format(input_images[image_idx]))
    statusbar.insert(tkinter.INSERT, '- {}\n'.format(label_images[image_idx]))
    statusbar.see(tkinter.END)

    total_image = produce_tri_image(input_images[image_idx], label_images[image_idx])

    photo = PIL.ImageTk.PhotoImage(image = PIL.Image.fromarray(total_image))
    canvas.create_image(0, 0, image=photo, anchor=tkinter.NW)
    window.title('Image {}/{}'.format(image_idx, len(input_images)))

# Callback for the "save + >" button
def save_and_next_image():
    global image_idx
    global input_images

    # save image
    filename = input_images[image_idx]
    labelname = label_images[image_idx]
    cv_img, _, _, _ = load_image(filename)
    cv_label, _, _, _ = load_image(labelname)
    cv_img = cv2.cvtColor(cv_img, cv2.COLOR_BGR2YUV)[...,0]
    cv_label = cv2.cvtColor(cv_label, cv2.COLOR_BGR2YUV)[..., 0]


    # save it to the destination folder

    fn  = '{}/{}'.format(img_dir, os.path.basename(filename))
    cv2.imwrite(fn, cv_img)
    fn = '{}/{}'.format(mask_dir, os.path.basename(labelname))
    cv2.imwrite(fn, cv_label)

    statusbar.insert(tkinter.INSERT, 'Save image to {}\n'.format(out_folder))

    next_image()


""" First find all inputs and labels"""

input_images = [y for x in os.walk(in_folder) for y in glob(os.path.join(x[0], '*input*.png'))]
label_images = [y for x in os.walk(in_folder) for y in glob(os.path.join(x[0], '*label*.png'))]

input_images.sort()
label_images.sort()

print(len(input_images))
#print(input_images)
print(len(label_images))


"""Initialize the GUI"""
# initialize the window toolkit along with the two image panels
image_idx = 0
window = tkinter.Tk()
window.title('Image {}/{}'.format(image_idx, len(input_images)))

# load image
total_image = produce_tri_image(input_images[image_idx], label_images[image_idx])

# Create a canvas that can fit the above image
canvas = tkinter.Canvas(window, width = total_image.shape[1], height = total_image.shape[0])
canvas.pack()

# Use PIL (Pillow) to convert the NumPy ndarray to a PhotoImage
photo = PIL.ImageTk.PhotoImage(image = PIL.Image.fromarray(total_image))

# Add a PhotoImage to the Canvas
canvas.create_image(0, 0, image=photo, anchor=tkinter.NW)

# buttons
bottomframe = tkinter.Frame(window)
bottomframe.pack( side = tkinter.BOTTOM )
btn_back=tkinter.Button(bottomframe, text="<", width=10, command=back_image)
btn_back.pack(side=tkinter.LEFT, anchor=tkinter.CENTER, expand=True)
btn_next=tkinter.Button(bottomframe, text=">", width=10, command=next_image)
btn_next.pack(side=tkinter.LEFT, anchor=tkinter.CENTER, expand=True)
btn_snext=tkinter.Button(bottomframe, text="Save >", width=10, command=save_and_next_image)
btn_snext.pack(side=tkinter.LEFT, anchor=tkinter.CENTER, expand=True)

statusbar = tkinter.Text(window, height=5, width=200)
statusbar.pack()

window.mainloop()
