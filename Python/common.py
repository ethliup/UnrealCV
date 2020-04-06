import os
import sys
import glob
import numpy as np
import imageio
from liegroups.numpy import SO3

from client import client 

class Unbuffered(object):
   def __init__(self, stream):
       self.stream = stream
   def write(self, data):
       self.stream.write(data)
       self.stream.flush()
   def __getattr__(self, attr):
       return getattr(self.stream, attr)

def ConnectUnreal():    
    response = client.connect()
    if response == False:
        print('UnrealCV server is not running. Run the game please...')
        sys.exit
    else:
        print(response)

def MoveToPose(pose):
    cmd = 'vset /pose '+repr(pose[0])+' '+repr(pose[1])+' '+repr(pose[2])+' '+repr(pose[3])+' '+repr(pose[4])+' '+repr(pose[5])+' '+repr(pose[6])
    client.request(cmd)

def RenderImage(camId, mode, fileName, rowToCapture=999999):
    cmd = 'vget /camera/' + repr(camId) + '/' + mode + ' ' + repr(rowToCapture) + ' ' + fileName
    client.request(cmd) 

def ClearFolder(path_to_folder):
  for root, dirs, files in os.walk(path_to_folder):
    for f in files:
      os.unlink(os.path.join(root, f))
  for d in dirs:
    shutil.rmtree(os.path.join(root, d))

def GenerateMotionBlurredImage(dirpath, outDir, outFileName):
    imageFileList = []
    for (dirpath, dirnames, filenames) in os.walk(dirpath):
        imageFileList.extend(filenames)
        break

    bOutImageInitialized = False
    for file in imageFileList:
        file = dirpath + '/' + file
        img = imageio.imread(file)
        img = img.astype(float)
        #os.remove(file)

        if bOutImageInitialized == False:
            outImage = img
            bOutImageInitialized = True
            continue
        outImage = outImage + img
    outImage = outImage / len(imageFileList)

    os.makedirs(outDir, exist_ok=True)
    imageio.imwrite(outDir + '/' + outFileName, outImage.astype(np.uint8))

def GenerateRollingShutterImage(dirpath, H, outDir, outFileName):
    im_gs=imageio.imread(dirpath+'/0.png').astype(float)
    _,W,C=im_gs.shape
    im_rs = np.zeros((H, W, C)).astype(float)

    for i in range(H):
      im_gs=imageio.imread(dirpath+'/'+str(i)+'.png').astype(float)
      im_rs[i,:,:]=im_gs[0,:,:]

    imageio.imwrite(outDir+'/'+outFileName, im_rs.astype(np.uint8))

def GenerateRollingShutterDepth(dirpath, H, outDir, outFileName):
    depth = np.loadtxt(dirpath+'/0.exr').astype(float)
    W = depth.shape[0]
    depth = np.zeros((H,W)).astype(float)

    for i in range(H):
      temp = np.loadtxt(dirpath+'/'+str(i)+'.exr').astype(float)
      depth[i,:]=temp
    np.savetxt(outDir+'/'+outFileName, depth)
    
def QuaternionsToTangents(qs):
    N=qs.shape[0]
    tangents=np.empty([N,3])
    for i in range(N):
        q = qs[i]
        tangents[i]=SO3.log(SO3.from_quaternion(q))
    return tangents

def T_inv(T):
    R = T[:, :3]
    t = T[:, 3:4]

    R_inv = np.transpose(R)
    t_inv = np.matmul(R_inv, t)
    t_inv = -1. * t_inv
    return np.concatenate([R_inv, t_inv], axis=1)

def T_mul(T1, T2):
  R1 = T1[:, :3]
  t1 = T1[:, 3:4]

  R2 = T2[:, :3]
  t2 = T2[:, 3:4]

  R = np.matmul(R1, R2)
  t = np.matmul(R1, t2) + t1
  return np.concatenate([R, t], axis=1)

def Load_cam_extrinsincs(path_to_file, camera_id):
    fd = open(path_to_file, 'r')

    T_camera_to_body = None
    cur_dev_name = None
    scalingFactor = None
    for line in fd:
        if '#' in line:
            continue

        line = line.replace('\n', '')
        line_ls = line.split(' ')

        if line_ls[0]=='devName':
            cur_dev_name=line_ls[1]

        if line_ls[0]=='scalingFactor':
            scalingFactor=float(line_ls[1])

        if line_ls[0]=='T':
            qw  = float(line_ls[1])
            qx  = float(line_ls[2])
            qy  = float(line_ls[3])
            qz  = float(line_ls[4])
            x  = float(line_ls[5]) #* scalingFactor
            y  = float(line_ls[6]) #* scalingFactor
            z  = float(line_ls[7]) #* scalingFactor

            if cur_dev_name=='cam'+str(camera_id):
                R = SO3.from_quaternion(np.array([qw,qx,qy,qz])).as_matrix()
                t = np.expand_dims(np.array([x,y,z]), 1)
                T_camera_to_body = np.concatenate((R, t), axis=1)
                break
    return T_camera_to_body, scalingFactor



#GenerateRollingShutterImage('F:/Unreal4/2019_06_04_unreal_simple_town_rolling-shutter/temp', 1, 'F:/Unreal4/2019_06_04_unreal_simple_town_rolling-shutter/', 'test.png')
#GenerateRollingShutterDepth('F:/Unreal4/2019_06_04_unreal_simple_town_rolling-shutter/temp', 'F:/Unreal4/2019_06_04_unreal_simple_town_rolling-shutter/', 'test.exr')    


