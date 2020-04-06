import os
import sys
import math
import shutil
from shutil import copyfile
import numpy as np
from liegroups.numpy import SO3

from client import client
from common import *

# configuration
DATASET_FOLDER  = "C:/Users/liup/Desktop/Carla_RS_dataset/Carla_town01"

FPS=20
RESOLUTION_H=448
RESOLUTION_W=640
READOUT_TIME=1.0/float(FPS)/float(RESOLUTION_H)
SEQ_LEN = 3
FRAME_INDEX=-1
USE_EXISTING_VEL=False
RENDER_DEPTH=False

VEL_MAX_t = 2.5
VEL_MIN_t = 1.0
VEL_MAX_w = 1
VEL_MIN_w = 0

REPLICA = 15

START_FRAME_ID = 105

def capture_rs_image_seq(seq_id, _T_actor, T_cam0_to_body, T_body_to_cam0, scalingFactor):
    # create folders 
    seq_path = os.path.join(DATASET_FOLDER, 'seq_'+str(seq_id))
    if not os.path.exists(seq_path):
        os.makedirs(seq_path)

    if not USE_EXISTING_VEL:
        # cam_vel: wx, wy, wz, tx, ty, tz
        # cam_vel: row0_to_row_others
        wx = np.random.uniform(VEL_MIN_w, VEL_MAX_w)
        wy = np.random.uniform(VEL_MIN_w, VEL_MAX_w)
        wz = np.random.uniform(VEL_MIN_w, VEL_MAX_w)
        tx = np.random.uniform(VEL_MIN_t, VEL_MAX_t)
        ty = np.random.uniform(VEL_MIN_t, VEL_MAX_t)*0.1
        tz = np.random.uniform(VEL_MIN_t, VEL_MAX_t)

        wx = wx if np.random.rand()>0.5 else -wx
        wy = wy if np.random.rand()>0.5 else -wy
        wz = wz if np.random.rand()>0.5 else -wz
        tx = tx if np.random.rand()>0.5 else -tx
        ty = ty if np.random.rand()>0.5 else -ty
        tz = tz if np.random.rand()>0.5 else -tz

        cam_vel = np.array([wx,wy,wz,tx,ty,tz]) 

        # save ground truth vel to file
        fid=open(os.path.join(seq_path, 'gt_vel.log'),'w')
        fid.write('# Ground truth velocity in camera frame\n')
        fid.write('# wx, wy, wz, x, y, z\n')
        fid.write('%f %f %f %f %f %f\n' % (cam_vel[0],cam_vel[1],cam_vel[2],cam_vel[3],cam_vel[4],cam_vel[5]))
        fid.close()
    else:
        _vel=open(os.path.join(seq_path, 'gt_vel.log'),'r').readlines()
        for v in _vel:
            if '#' in v:
                continue
            v = v.replace('\n','')
            v = v.split(' ')
            cam_vel = [float(x) for x in v]
            cam_vel = np.array(cam_vel)

    # get T_actor matrix: from body to world
    _T_actor = _T_actor.split(',')
    q = np.array([float(_T_actor[0]), float(_T_actor[1]), float(_T_actor[2]), float(_T_actor[3])])
    t = np.array([float(_T_actor[4]), float(_T_actor[5]), float(_T_actor[6])])
    t = np.expand_dims(t,1)
    R_actor = SO3.from_quaternion(q).as_matrix()
    T_actor = np.concatenate((R_actor, t), axis=1)
    
    for frame_id in range(SEQ_LEN):
        # render rolling shutter image
        t_offset = frame_id / float(FPS) 
        if FRAME_INDEX>0 and FRAME_INDEX != frame_id:
            continue

        # render all global shutter images
        for r in range(RESOLUTION_H):
            t_r = r * READOUT_TIME + t_offset
            _T_cam = cam_vel * t_r
            R_cam = SO3.exp(_T_cam[:3]).as_matrix()
            t_cam = _T_cam[3:6] / scalingFactor
            t_cam = np.expand_dims(t_cam, 1)

            # T_cam: row0_to_row_r
            T_cam = np.concatenate((R_cam, t_cam), axis=1)
            # T_cam: row_r_to_row0
            #T_cam = T_inv(T_cam)
            # T_actor_r: from body to world
            T_actor_r = T_mul(T_actor, T_mul(T_cam0_to_body, T_mul(T_cam, T_body_to_cam0)))

            # convert to quanternion
            q_actor_r = SO3.from_matrix(T_actor_r[:,:3], normalize=True).to_quaternion()
            norm = math.sqrt(q_actor_r[0]**2+q_actor_r[1]**2+q_actor_r[2]**2+q_actor_r[3]**2) 
            q_actor_r = q_actor_r / norm
            t_actor_r = T_actor_r[:,3:4].squeeze()
            pose_actor_r = np.array([q_actor_r[0],
                                    q_actor_r[1],
                                    q_actor_r[2],
                                    q_actor_r[3],
                                    t_actor_r[0],
                                    t_actor_r[1],
                                    t_actor_r[2]])

            # Move actor and render images
            MoveToPose(pose_actor_r)
            RenderImage(0, 'lit', os.path.join(DATASET_FOLDER, 'tmp/'+str(r)+'.png'), r)
            
            if RENDER_DEPTH:
                RenderImage(0, 'depth', os.path.join(DATASET_FOLDER, 'tmp/'+str(r)+'.exr'), r)

            # render global shutter image & depth
            if r==0:
                RenderImage(0, 'lit', seq_path+'/'+str(frame_id).zfill(4)+'_gs_f.png')
                if RENDER_DEPTH:
                    RenderImage(0, 'depth', seq_path+'/'+str(frame_id).zfill(4)+'_gs_f.exr')
                    os.rename(seq_path+'/'+str(frame_id).zfill(4)+'_gs_f.exr', seq_path+'/'+str(frame_id).zfill(4)+'_gs_f.depth')

            if r==RESOLUTION_H//2:
                RenderImage(0, 'lit', seq_path+'/'+str(frame_id).zfill(4)+'_gs_m.png')
                if RENDER_DEPTH:
                    RenderImage(0, 'depth', seq_path+'/'+str(frame_id).zfill(4)+'_gs_m.exr')
                    os.rename(seq_path+'/'+str(frame_id).zfill(4)+'_gs_m.exr', seq_path+'/'+str(frame_id).zfill(4)+'_gs_m.depth')

        # synthesize rolling shutter image
        GenerateRollingShutterImage(DATASET_FOLDER+'/tmp', RESOLUTION_H, seq_path, str(frame_id).zfill(4)+'_rs.png')
        if RENDER_DEPTH:
            GenerateRollingShutterDepth(DATASET_FOLDER+'/tmp', RESOLUTION_H, seq_path, str(frame_id).zfill(4)+'_rs.depth')


def main():
    ###### initialize folders #######
    if not os.path.exists(DATASET_FOLDER):
        os.makedirs(DATASET_FOLDER)

    if not os.path.exists(os.path.join(DATASET_FOLDER, 'tmp')):
        os.makedirs(os.path.join(DATASET_FOLDER, 'tmp'))

    ###### initialize camera settings #######
    sys.stdout = Unbuffered(sys.stdout)
    ConnectUnreal()

    cmd = 'vset /fov 90'
    client.request(cmd)

    cmd = 'vset /resolution ' + str(RESOLUTION_H) + ' ' + str(RESOLUTION_W)
    client.request(cmd)

    fileName = DATASET_FOLDER + '/intrinsics.txt'
    cmd = 'vget /camera/K ' + fileName
    client.request(cmd)

    # get extrinsic parameters
    fileName = DATASET_FOLDER + '/extrinsics.txt'
    cmd = 'vget /camera/extrin ' + fileName
    client.request(cmd) 

    # save rolling shutter intrinsics
    fid=open(DATASET_FOLDER+'/rs_calib.txt', 'w')
    fid.write('#Rolling shutter camera intrinsic parameters\n')
    fid.write('FPS: %d\n' % FPS)
    fid.write('ROW_READOUT_TIME(ms): %.8f\n' % (READOUT_TIME*1000.))
    fid.write('RESOLUTION_H: %d\n' % RESOLUTION_H)
    fid.write('RESOLUTION_W: %d\n' % RESOLUTION_W)
    fid.close()

    ###### read in actor poses ######
    path = os.path.join(DATASET_FOLDER, 'actor_global_poses.txt')
    _T_actor_global_poses = open(path).readlines()
    # T_actor_global_poses: body_to_world: qx, qy, qz, qw, tx, ty, tz
    T_actor_global_poses = []
    for x in _T_actor_global_poses:
        if '#' in x:
            continue
        if '\n' in x:
            x = x.replace('\n','')
        T_actor_global_poses.append(x)

    ###### read in camera extrinsics ######
    T_cam0_to_body, scalingFactor = Load_cam_extrinsincs(os.path.join(DATASET_FOLDER, 'extrinsics.txt'), 0)
    T_body_to_cam0 = T_inv(T_cam0_to_body)

    T_l2r = np.array([[-1.,0.,0.,0.],
                    [0.,-1.,0.,0.],
                    [0.,0.,-1.,0.]])

    ###### Render images ######
    np.random.seed(0)
    for _T_actor in T_actor_global_poses:
        seq_id = T_actor_global_poses.index(_T_actor)
        for i in range(seq_id*REPLICA, (seq_id+1)*REPLICA):
            if i < START_FRAME_ID:
            	continue
            capture_rs_image_seq(i, _T_actor, T_cam0_to_body, T_body_to_cam0, scalingFactor)

    # clean up temp folder
    shutil.rmtree(DATASET_FOLDER+'/tmp/')    

if __name__=='__main__':
    main()
