import os
import shutil
import numpy as np
from skimage import io
import cv2

image_root_dir='/home/peidong/Desktop/Image_Carla_VO_sequences'
depth_root_dir='/home/peidong/Desktop/Depth_Carla_VO_sequences'

list_towns = sorted(os.listdir(image_root_dir))

for town in list_towns:
	image_path_town = os.path.join(image_root_dir, town)

	# process sequences 
	list_sequences = sorted(os.listdir(image_path_town))
	for seq in list_sequences:
		if not 'seq_' in seq:
			continue

		image_path_seq = os.path.join(image_path_town, seq)

		# compress image files 
		list_sensors = sorted(os.listdir(image_path_seq))
		for sensor in list_sensors:
			image_path_sensor = os.path.join(image_path_seq, sensor)

			if not os.path.isdir(image_path_sensor):
				continue

			list_images = sorted(os.listdir(image_path_sensor))
			temp = io.imread(os.path.join(image_path_sensor, list_images[0]))
			H,W,C = temp.shape
			nRows = 5
			nCols = 4
			all_images = np.zeros([nRows*H, nCols*W, C])

			for r in range(nRows):
				for c in range(nCols):
					src_image = io.imread(os.path.join(image_path_sensor, list_images[r*nCols+c]))/255.
					all_images[r*H:r*H+H, c*W:c*W+W, :]=src_image
			
			cv2.imshow('images', all_images)
			cv2.waitKey(20)

			print('process', image_path_seq)

			char = input('').split(" ")[0]
			if char=='d':
				print('delete', os.path.join(depth_root_dir, town+'/'+seq))
				print('delete', image_path_seq)
				print('\n')
				shutil.rmtree(os.path.join(depth_root_dir, town+'/'+seq))
				shutil.rmtree(image_path_seq)
