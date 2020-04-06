import numpy as np
import cv2

filename = "C:/Users/liup/Desktop/Carla_dataset/Carla_town02/seq_0/0000_gs_f.depth"
depth = np.loadtxt(filename)
print(depth.shape)

cv2.imshow('depth', 5./(depth+1e-8))
cv2.waitKey(0)
