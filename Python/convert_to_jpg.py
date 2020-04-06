import os
import shutil
import numpy as np
from skimage import io

src_root_dir='/home/peidong/leonhard/project/infk/cvg/liup/mydata/Unreal/2019_10_20_Carla_RS_dataset'

for root, dirs, files in os.walk(src_root_dir):
	for file in files:
		if not '.png' in file:
			continue

		src_image_path=os.path.join(root, file)
		tar_image_path=src_image_path.replace('.png', '.jpg')

		print(src_image_path)
		
		src_im = io.imread(src_image_path)
		if src_im.ndim>2:
			src_im = src_im[:,:,:3].copy()
					
		io.imsave(tar_image_path, src_im)
		# os.remove(src_image_path)

		