import cv2
import numpy as np
import matplotlib.pyplot as plt

videofile_in = '/home/ag09/data/VITAL/muscle/CNS/01NVb-003-281/T2/small.mp4'

cap = cv2.VideoCapture(videofile_in)
# Check if video opened successfully
if cap.isOpened() == False:
    print('Unable to read video ' + videofile_in)

bounds = (638, 135, 821, 722)
bounds_p = (618, 202, 848, 646)

i = 0

rg, rb, gb = [], [], []
nnz_rg, nnz_rb, nnz_gb = [], [], []
nz_rg, nz_rb, nz_gb = [], [], []

while True:
    success, image_ = cap.read()

    if not success:
        break

    image = image_[int(bounds[1]):int(bounds[1] + bounds[3]), int(bounds[0]):int(bounds[0] + bounds[2]), :]

    r, g, b = image[..., 2].astype(np.float), image[..., 1].astype(np.float), image[..., 0].astype(np.float)

    # image wide rgb
    rg.append( np.mean(np.abs(r - g)) )
    rb.append(  np.mean(np.abs(r - b)) )
    gb.append ( np.mean(np.abs(g - b)) )

    # n pixels not gray
    nnz_rg.append( np.count_nonzero(np.abs(r - g)>1))
    nnz_rb.append(np.count_nonzero(np.abs(r - b) > 1))
    nnz_gb.append( np.count_nonzero(np.abs(g - b) > 1))

    # statistics of non gray pixels
    nz_rg.append(np.mean((r - g)[np.abs(r - g) > 1]))
    nz_rb.append(np.mean((r - b)[np.abs(r - b) > 1]))
    nz_gb.append(np.mean((g - b)[np.abs(g - b) > 1]))

    font = cv2.FONT_HERSHEY_SIMPLEX

    fontScale = 1
    color = (0, 0, 255)
    thickness = 2
    image = cv2.putText(image, 'R-G: {:.2f}'.format(rg[-1]), (50, 100), font, fontScale, color, thickness, cv2.LINE_AA)
    image = cv2.putText(image, 'R-B: {:.2f}'.format(rb[-1]), (50, 150), font, fontScale, color, thickness, cv2.LINE_AA)
    image = cv2.putText(image, 'G-B: {:.2f}'.format(gb[-1]), (50, 200), font, fontScale, color, thickness, cv2.LINE_AA)
    image = cv2.putText(image, 'R-G nnz: {}'.format(nnz_rg[-1]), (50, 250), font, fontScale, color, thickness, cv2.LINE_AA)
    image = cv2.putText(image, 'R-B nnz: {}'.format(nnz_rb[-1]), (50, 300), font, fontScale, color, thickness, cv2.LINE_AA)
    image = cv2.putText(image, 'G-B nnz: {}'.format(nnz_gb[-1]), (50, 350), font, fontScale, color, thickness, cv2.LINE_AA)
    image = cv2.putText(image, 'R-G nz: {:.2f}'.format(nz_rg[-1]), (50, 400), font, fontScale, color, thickness, cv2.LINE_AA)
    image = cv2.putText(image, 'R-B nz: {:.2f}'.format(nz_rb[-1]), (50, 450), font, fontScale, color, thickness, cv2.LINE_AA)
    image = cv2.putText(image, 'G-B nz: {:.2f}'.format(nz_gb[-1]), (50, 500), font, fontScale, color, thickness, cv2.LINE_AA)

    cv2.imwrite('/home/ag09/data/VITAL/muscle/CNS/01NVb-003-281/T2/small_files/frame_{:05d}.png'.format(i), image)
    i+=1
    print(i)


plt.subplot(1,3,1)
plt.plot(rg, 'r-', label='r-g')
plt.plot(rb, 'g-', label='r-b')
plt.plot(gb, 'b-', label='g-b')
plt.legend()
plt.title('Average Absolute difference')
plt.subplot(1,3,2)
plt.plot(nnz_rg, 'r-', label='r-g')
plt.plot(nnz_rb, 'g-', label='r-b')
plt.plot(nnz_gb, 'b-', label='g-b')
plt.title('NNZ in subtraction')
plt.legend()
plt.subplot(1,3,3)
plt.plot(nz_rg, 'r-', label='r-g')
plt.plot(nz_rb, 'g-', label='r-b')
plt.plot(nz_gb, 'b-', label='g-b')
plt.title('Average difference over NZNNZ')
plt.legend()
plt.show()
