import os
import shutil
import numpy as np
from skimage import io

src_root_dir='/media/peidong/Windows1/Users/liup/Desktop/Carla_RS_dataset'
tar_root_dir='/home/peidong/Desktop/Carla_RS_dataset'

if not os.path.exists(tar_root_dir):
	os.makedirs(tar_root_dir)

list_towns = os.listdir(src_root_dir)

for town in list_towns:
	src_path_town = os.path.join(src_root_dir, town)
	tar_path_town = os.path.join(tar_root_dir, town)

	if not os.path.exists(os.path.join(src_path_town, 'extrinsics.txt')):
		continue

	if not os.path.exists(tar_path_town):
		os.makedirs(tar_path_town)

	# copy meta data files 
	shutil.copyfile(os.path.join(src_path_town, 'actor_global_poses.txt'),
		            os.path.join(tar_path_town, 'actor_global_poses.txt'))

	shutil.copyfile(os.path.join(src_path_town, 'extrinsics.txt'),
		            os.path.join(tar_path_town, 'extrinsics.txt'))

	shutil.copyfile(os.path.join(src_path_town, 'intrinsics.txt'),
		            os.path.join(tar_path_town, 'intrinsics.txt'))

	# process sequences 
	list_sequences = os.listdir(src_path_town)
	for seq in list_sequences:
		if not 'seq_' in seq:
			continue

		src_path_seq = os.path.join(src_path_town, seq)
		tar_path_seq = os.path.join(tar_path_town, seq)

		if not os.path.isfile(os.path.join(src_path_seq, 'poses_cami_to_cam0.txt')):
			continue

		if not os.path.exists(tar_path_seq):
			os.makedirs(tar_path_seq)

		# copy ground truth pose files 
		shutil.copyfile(os.path.join(src_path_seq, 'poses_cami_to_cam0.txt'),
			            os.path.join(tar_path_seq, 'poses_cami_to_cam0.txt'))

		# compress image files 
		list_sensors = os.listdir(src_path_seq)
		for sensor in list_sensors:
			src_path_sensor = os.path.join(src_path_seq, sensor)
			tar_path_sensor = os.path.join(tar_path_seq, sensor)

			if not os.path.isdir(src_path_sensor):
				continue

			if not os.path.exists(tar_path_sensor):
				os.makedirs(tar_path_sensor)

			# compress images 
			list_images = os.listdir(src_path_sensor)
			for image in list_images:
				if '.png' in image:
					src_im = io.imread(os.path.join(src_path_sensor, image))[:,:,:3].copy()
					io.imsave(os.path.join(tar_path_sensor, image.replace('.png', '.jpg')), src_im)
					print('compressed', os.path.join(src_path_sensor, image))

				if '.exr' in image:
					src_im = np.loadtxt(os.path.join(src_path_sensor, image))
					np.save(os.path.join(tar_path_sensor, image.replace('.exr', '.npy')), src_im)
